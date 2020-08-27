#include <bitset>
#include <ctime>
#include <iostream>
#include <TPG.h>
#include <ale_interface.hpp>
#ifndef NOSDL
#include <SDL.h>
#endif
#include <unistd.h>
#include <FeatureMap.cc>

const vector < int > objectives = {0,1,2}; // one objective per rom
#define ALE_NUM_POINT_AUX_DOUBLES objectives.size() + 3 // 1 reward per rom + 3 aux rewards for reporting (instructions/teams/features per decision)
#define MAX_NUM_FRAMES_PER_EPISODE 18000
#define MAX_BEHAVIOUR_STEPS 18000
#define CHKP_MOD 2

#define BACKGROUND_DETECTION false
#define QUANTIZE false
#define SECAM false
#define ALE_DIM_BEHAVIOURAL (QUANTIZE ? 1344 : 33600)
#define ALE_SENSOR_RANGE_HIGH (QUANTIZE ? 255 : (SECAM ? 7 : 128))
#define ALE_SENSOR_RANGE_LOW 0

using namespace std;

/***********************************************************************************************************************/
point * initUniformPointALE(long gtime, long id, int phase){
   vector < double > pState;
   vector < double > bState;

   for (int i = 0; i < ALE_DIM_BEHAVIOURAL; i++)
      bState.push_back((int)(ALE_SENSOR_RANGE_LOW + (drand48() * (ALE_SENSOR_RANGE_HIGH - ALE_SENSOR_RANGE_LOW))));
   vector<double> rewards;
   for (int i = 0; i < ALE_NUM_POINT_AUX_DOUBLES; i++) rewards.push_back(0);
   point * p = new point(gtime, phase, id, pState, bState, rewards);
   return p;
}

/***********************************************************************************************************************/
void runEval(vector<ALEInterface *> &alev, int initialRom, TPG &tpg, ActionVect &legal_actions, int t, int phase, bool visual, bool stateful, 
      int &timeGenTotalInGame, int hostToReplay, long &eval, long &decision, FeatureMap &featureMap){
   bool timing = false;
   clock_t timeStartClockDecision;
   long decisionMilliseconds = 0;
   clock_t timeStartClockEval;
   long totalGameFramesThisEval = 0;
   if (timing) timeStartClockEval = clock();

   int activeRom = initialRom;

   //gameplay data
   vector < double > behaviourSequence; behaviourSequence.reserve(MAX_BEHAVIOUR_STEPS); //vector < double > tmpBehaviourSequence;
   vector < double > currentState; currentState.reserve(tpg.dimBehavioural()); currentState.resize(tpg.dimBehavioural());
   int currentAction;
   set <learner*, LearnerBidLexicalCompare> learnersRanked;
   set <learner*, LearnerBidLexicalCompare> winningLearners;
   vector <double> rewards; rewards.reserve(ALE_NUM_POINT_AUX_DOUBLES); rewards.resize(ALE_NUM_POINT_AUX_DOUBLES);
   int step;
   //profile points data
   vector < double > fullState; fullState.reserve(tpg.dimBehavioural()); fullState.resize(tpg.dimBehavioural());
   set < long > fullFeatures; for (int i = 0; i < tpg.dimBehavioural(); i++) fullFeatures.insert(i);
   int prevProfileId = -1; int newProfilePoints = 0;
   //reporting data
   char tmpChar[80];
   set <long> featTmp;
   long evalInit = eval;
   long decisionInit = decision;
   int timeStartGame;
   set <team*> visitedTeams;
   vector < set < long > > decisionFeatures;
   set < team * > teamTeams;
   set < learner * > teamLearners;
   map < learner *, long > learnerUsage;
   long numDecisionInstructions, numDecisionInstructionsPerGameSum, numDecisionInstructionsPerEvalSum;
   long numVisitedTeamsPerGameSum, numVisitedTeamsPerEvalSum;
   long numDecisionFeaturesPerGameSum, numDecisionFeaturesPerEvalSum;

   vector <team*> teams;
   //Possible to replay non-root teams
   if (hostToReplay > -1)
      tpg.getTeams(teams,false);
   else
      tpg.getTeams(teams,true);

   for (size_t i = 0; i < teams.size(); i++){
      if (hostToReplay > -1 && teams[i]->id() != hostToReplay) continue;
      //get all features indexed by this policy (not just from active programs)
      set <long> features;
      tpg.policyFeatures(teams[i]->id(),features,false);

      //calculate number of evaluations for this team
      int numEval = 0;
      if (phase == _TRAIN_PHASE)
         numEval = min(tpg.episodesPerGeneration(), (int)((tpg.numStoredOutcomesPerHost(_TRAIN_PHASE) * alev.size()) - teams[i]->numEval()));
      else numEval = tpg.numStoredOutcomesPerHost(_TEST_PHASE);   

      if (tpg.verbose()){
         tpg.getAllNodes(teams[i], teamTeams, teamLearners);
         learnerUsage.clear();
         for(set<learner*> :: iterator it = teamLearners.begin(); it != teamLearners.end();++it)
            learnerUsage.insert(pair<learner*,long>((*it),0));
      }
      numDecisionInstructionsPerEvalSum = 0; numVisitedTeamsPerEvalSum = 0; numDecisionFeaturesPerEvalSum = 0;
      //run evalutations
      for (int e = 0; e < numEval; e++){
         if (phase == _TRAIN_PHASE){
            //select rom
            if (e < alev.size() && teams[i]->numOutcomes(phase,e) == 0) activeRom = e; //every team gets one episode under each title
            else{
               do
                  activeRom = (int)(drand48()*alev.size());
               while (teams[i]->numOutcomes(phase,activeRom) >= tpg.numStoredOutcomesPerHost(_TRAIN_PHASE)); //select another rom if the team has enough evals for this one
            }
            tpg.hostFitnessMode(activeRom);
            //cout << "SELROM t " << t << " evalCount " << evalCount++ << " tm " << teams[i]->id() << " numEval " << numEval << " numEvalT " << teams[i]->numEval();
            //cout << " teams[i]->numOutcomes(phase,activeRom) " << teams[i]->numOutcomes(phase,activeRom) << " aRom " << activeRom << endl;
         }

         if (tpg.verbose()){
            cout << "runEval t " << t << " activeRom " << activeRom << " i/teamsToEval " << i << "/" << teams.size() <<  " tm " << teams[i]->id();
            cout << " numOut " << teams[i]->numOutcomes(phase,activeRom);
            cout << " numEval " << numEval << " eval " << e << endl;
         }
         step = 0; for (int r = 0; r < ALE_NUM_POINT_AUX_DOUBLES; r++) rewards[r] = 0;
         numDecisionInstructionsPerGameSum = 0; numVisitedTeamsPerGameSum = 0; numDecisionFeaturesPerGameSum = 0;
         int decisionFeatCount = 0;
         timeStartGame = time(NULL);
         alev[activeRom]->reset_game();
         //run game episode
         while (!alev[activeRom]->game_over()) {
            if (timing) timeStartClockDecision = clock();
            const ALEScreen screen = alev[activeRom]->getScreen();
            featureMap.getFeatures(screen,currentState,features,"step");
            //if (tpg.verbose() && visual) cout << endl << "=== policy trace ===========================================================" << endl;
            if (!stateful || (stateful && step == 0)) tpg.clearRegisters(teams[i]);
            int atomicAction = tpg.getAction(teams[i], currentState, true, learnersRanked, winningLearners, numDecisionInstructions, decisionFeatures, visitedTeams, tpg.verbose());
            //cout << "ACT " << atomicAction << endl;
            if(visual){
               featureMap.saveFrames(true);
               featTmp.clear();
               sprintf(tmpChar, "%08d%s",step,".pol");
               featureMap.getFeatures(screen,currentState,features,tmpChar); //pol vis
               for (int df = 0; df < decisionFeatures.size(); df++)
                  featTmp.insert(decisionFeatures[df].begin(), decisionFeatures[df].end());
               sprintf(tmpChar, "%08d%s",step,".des");
               featureMap.getFeatures(screen,currentState,featTmp,tmpChar); //policy viz per decision
               featureMap.saveFrames(false);
            }
            rewards[activeRom] += alev[activeRom]->act(legal_actions[(atomicAction*-1)-1]);//atomic actions are represented as negatives in tpg
            if (drand48() < tpg.pAddProfilePoint() && teams[i]->id() != prevProfileId){
               featureMap.getFeatures(screen,fullState,fullFeatures,"profile");
               tpg.addProfilePoint(fullState,rewards,phase,t);
               prevProfileId = teams[i]->id();
               newProfilePoints++;
            }
            behaviourSequence.push_back((double)(atomicAction)); 
            step++;
            //reporting
            numDecisionInstructionsPerGameSum += numDecisionInstructions;
            numDecisionInstructionsPerEvalSum += numDecisionInstructions;
            numVisitedTeamsPerGameSum += (visitedTeams.size());
            numVisitedTeamsPerEvalSum += (visitedTeams.size());
            featTmp.clear();
            for (int df = 0; df < decisionFeatures.size(); df++)
               featTmp.insert(decisionFeatures[df].begin(), decisionFeatures[df].end());
            numDecisionFeaturesPerGameSum += featTmp.size();
            numDecisionFeaturesPerEvalSum += featTmp.size();
            if (tpg.verbose()){
               cout << "tpg-activeNodes rom " << activeRom << " nodes";
               for(set<learner*, LearnerBidLexicalCompare> :: iterator it = winningLearners.begin(); it != winningLearners.end();++it){
                  learnerUsage.find(*it)->second++;
                  cout << " s" << (*it)->id();
                  //cout << endl << " numFeat " << (*it)->numFeatures() << endl;
               }
               for(set<team*> :: iterator it = visitedTeams.begin(); it != visitedTeams.end();++it)
                  cout << " h" << (*it)->id();
               cout << endl;

               //cout << endl << "lUsageOnTime";
               //for (map<learner*,long>::iterator it=learnerUsage.begin(); it!=learnerUsage.end(); ++it){
               //   if (winningLearners.find(it->first) != winningLearners.end())
               //      cout << " 1";
               //   else
               //      cout << " 0";
               //}
               //cout << endl;
            }
         }
         if (timing) decisionMilliseconds += (clock() - timeStartClockDecision) / (double)(CLOCKS_PER_SEC / 1000);
         //reporting
         timeGenTotalInGame += (time(NULL) - timeStartGame);
         rewards[alev.size()] = (double)numDecisionInstructionsPerGameSum/step;//used for reporting, not as reward
         rewards[alev.size()+1] = (double)numVisitedTeamsPerGameSum/step;//used for reporting, not as reward
         rewards[alev.size()+2] = (double)numDecisionFeaturesPerGameSum/step;//used for reporting, not as reward
         decision+=step;
         //get behaviour sequence for last MAX_BEHAVIOUR_STEPS interactions
         //int start = min((1+tpg.dimBehavioural())*MAX_BEHAVIOUR_STEPS,(int)behaviourSequence.size());
         //int start = min(MAX_BEHAVIOUR_STEPS,(int)behaviourSequence.size());
         //for (size_t b = behaviourSequence.size()-start; b < behaviourSequence.size(); b++)
         //tmpBehaviourSequence.push_back(behaviourSequence[b]);
         tpg.setOutcome(teams[i],behaviourSequence, rewards, phase, t);//behaviourSequence is actions only
         eval++;
         behaviourSequence.clear(); //tmpBehaviourSequence.clear();
         totalGameFramesThisEval += alev[activeRom]->getEpisodeFrameNumber();
         alev[activeRom]->reset_game();
      }
      if (tpg.verbose()){
         cout << "tpgEvalStats policy " << teams[i]->id();
         cout << " meanDecisionInst " << numDecisionInstructionsPerEvalSum/(double)(decision-decisionInit);
         cout << " meanVisitedTeams " << numVisitedTeamsPerEvalSum/(double)(decision-decisionInit);
         cout << " meanDecisionFeatures " << numDecisionFeaturesPerEvalSum/(double)(decision-decisionInit);
         cout << " lUseAllGame";
         for (map<learner*,long>::iterator it=learnerUsage.begin(); it!=learnerUsage.end(); ++it){
            //only accurate when a single team has been eveluated in numEval games (eg. under test)
            cout << " [" << it->first->id() << ":" << it->second/(double)(decision-decisionInit) << "]";
            it->second = 0;
         }
         cout << endl;
      }
   }
   if (!timing) cout << "tpgALEMultiTask::runEval t " << t << " numProfilePoints " << tpg.numProfilePoints() << " newProfilePoints " << newProfilePoints << " numEval " << eval - evalInit  << " numDecision " << decision - decisionInit << endl;

   if (timing){
      double sec = (clock() - timeStartClockEval) / (double)CLOCKS_PER_SEC;
      cout << "tpgALE::runEval timing";
      cout << " secondsThisEval " << sec;
      cout << " millisecondsPerDecision " << (double)decisionMilliseconds / (double)(decision-decisionInit);
      cout << " decisionsPerSecond " <<(double)(decision-decisionInit) / sec;
      cout << " totalDecisionsThisEval " << decision-decisionInit;
      cout << " totalGameFramesThisEval " << totalGameFramesThisEval;
      cout << " framesPerSecond " <<  (double)totalGameFramesThisEval / sec << endl;
   }
}
/***********************************************************************************************************************/
int main(int argc, char** argv) {
   // Command-line parameters
   bool checkpoint = false;
   int checkpointInPhase = -1;
   int hostFitnessMode = 1;
   long hostToReplay = -1;
   int id = -1;
   int phase = _TRAIN_PHASE;
   int seed = -1;
   bool replay = false;
   char * rom;
   int romSwitchMod = 1;
   bool stateful = false;
   int statMod = -1;
   long tMain = 1;
   int tPickup = -1;
   int tStart = 0;
   bool verbose = false;
   bool visual = false;

   int option_char;
   while ((option_char = getopt (argc, argv, "C:f:i:MO:R:r:S:s:T:t:Vv")) != -1)
      switch (option_char)
      {
         case 'C': checkpoint = true; checkpointInPhase = atoi(optarg); break;
         case 'f': hostFitnessMode = atoi(optarg); break;
         case 'i': id = atoi(optarg); break;
         case 'M': stateful = true; break;
         case 'O': statMod = atoi(optarg); break;
         case 'R': replay = true; hostToReplay = atoi(optarg); break;
         case 'S': romSwitchMod = atoi(optarg); break;
         case 'r': rom = optarg; break;
         case 's': seed = atoi(optarg); srand48(seed); break;
         case 'T': tMain = atoi(optarg); break;
         case 't': tStart = tPickup = atoi(optarg); break;
         case 'V': visual = true; break;
         case 'v': verbose = true; break;
         case '?':
                   cout << endl << "Command Line Options:" << endl << endl;
                   cout << "-C <mode to read checkpoint from> (Read a checkpoint created during TAIN_MODE:0, VALIDATION_MODE:1, or TEST_MODE:2. Requires checkpoint file and -t option.)" << endl;
                   cout << "-f <hostFitnessMode> (GameScore:0 Pillscore:1, Ghostscore:2)" << endl;
                   cout << "-i <id>" << endl;
                   cout << "-M (use memory)" << endl;
                   cout << "-O <statMod>" << endl;
                   cout << "-r <roms list> (File containing a list of Atari 2600 ROM files, named as per ALE supported ROMs)" << endl;
                   cout << "-R <hostIdToReplay> (Load and replay a specific host ID. Requires checkpoint file and options -C, and -t.)" << endl;
                   cout << "-S <romSwitchMod> (How often to switch roms. E.g. 1:every generation (default), 50:every 50 generations...)" << endl;
                   cout << "-s <seed> (Random seed)" << endl;
                   cout << "-T <generations>" << endl;
                   cout << "-t <t> (When loading populations form a checkpoint, t is the generation of the checkpoint file. Requires checkpoint file and option -C.)" << endl;
                   cout << "-V (Run with visualization.)" << endl;
                   exit(0);
      }
   cout << "====== tpgALEMultiTask parameters:" << endl << endl;
   cout << "chechkpoint " << checkpoint << endl;
   cout << "checkPointInMode " << checkpointInPhase << endl;
   cout << "hostFitnessMode " << hostFitnessMode << endl;
   cout << "statMod " << statMod << endl;
   cout << "replay " << replay << endl;
   cout << "rom " << rom << endl;
   cout << "hostToReplay " << hostToReplay << endl;
   cout << "seed " << seed << endl;
   cout << "tMain " << tMain << endl;
   cout << "tPickup " << tPickup << endl;
   cout << "visual " << visual << endl;

   //timing
   int timeGenSec0; int timeGenSec1; int timeGenTeams; int timeTotalInGame;
   int timeTemp; int timeInit; int timeSel; int timeCleanup;

   FeatureMap featureMap("", QUANTIZE, SECAM, BACKGROUND_DETECTION);

   //Multi-game ALE setup
   vector <ALEInterface *> alev;
   ifstream ifs;
   ifs.open(rom, ios::in);
   if (!ifs.is_open() || ifs.fail()){
      cerr << "open failed for file: " << rom << " error:" << strerror(errno) << '\n';
      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
   }
   string oneline;
   vector < string > outcomeFields;
   //set<Action> actionSet;
   cout << "roms";
   while (getline(ifs, oneline)){
      split(oneline,':',outcomeFields);
      alev.push_back(new ALEInterface);
      alev.back()->setBool("color_averaging",false);
      alev.back()->setInt("random_seed", seed);
      alev.back()->setInt("max_num_frames_per_episode", MAX_NUM_FRAMES_PER_EPISODE);
      alev.back()->setFloat("repeat_action_probability", 0.25);//this is the default
      alev.back()->setBool("display_screen", visual);
      alev.back()->setBool("sound", visual);
      char romBin[80]; sprintf(romBin, "%s/%s","../roms",outcomeFields[0].c_str());
      cout << " " << outcomeFields[0];
      alev.back()->loadROM(romBin);//(Also resets the system for new settings to take effect.)
   }

   cout << " alevSize " << alev.size() << endl;
   ActionVect legal_actions = alev[0]->getLegalActionSet();
   cout << "ale LegalActionSet size " << legal_actions.size()  << "|" << vecToStr(legal_actions) << endl << endl;
   int activeRom = 0;//(int)(drand48()*alev.size());//initial rom

   //TPG Parameter Setup
   TPG tpg(seed);
   tpg.id(id);
   tpg.dimBehavioural(featureMap.numFeatures());
   tpg.rangeBehavioural(featureMap.minFeatureVal(),featureMap.maxFeatureVal(),true);//discrete sensor inputs
   tpg.numPointAuxDouble(ALE_NUM_POINT_AUX_DOUBLES);
   tpg.numFitMode(alev.size());
   tpg.hostFitnessMode(activeRom);
   tpg.setParams(initUniformPointALE);
   tpg.numAtomicActions(legal_actions.size()); 
   tpg.verbose(verbose);
   long totalEval = 0;
   long totalFrame = 0;

   //loading populations from a checkpoint file
   if (checkpoint)
   {
      tpg.readCheckpoint(tPickup,checkpointInPhase);
      //tpg.recalculateLearnerRefs();
      //tpg.cleanup(tPickup,false);// don't prune learners because active/inactive is not accurate
      if (replay){
         tpg.clearNumEvals();
         if (hostFitnessMode >= 0){
            tpg.hostFitnessMode(hostFitnessMode);
            runEval(alev,hostFitnessMode,tpg,legal_actions,tPickup,_TEST_PHASE,visual,stateful,timeTotalInGame,hostToReplay,totalEval,totalFrame,featureMap);
         }
         else{
            for (int aRom = 0; aRom < alev.size(); aRom++){
               tpg.hostFitnessMode(aRom);
               runEval(alev,aRom,tpg,legal_actions,tPickup,_TEST_PHASE,visual,stateful,timeTotalInGame,hostToReplay,totalEval,totalFrame,featureMap);
            }
         }
         tpg.cleanup(tPickup,true,hostToReplay);
         tpg.printTeamInfo(tPickup,_TEST_PHASE,false,hostToReplay);
         tpg.printHostGraphsDFS((long)tPickup,true,_TEST_PHASE);//single best
         cout << "Goodbye cruel world." << endl;
         return 0;
      }
   }
   if (!checkpoint){ timeTemp = time(NULL); tpg.initTeams(); timeInit = time(NULL)-timeTemp; }
   /* Main training loop. */
   for (long t = tStart+1; t <= tMain; t++)
   {
      timeGenSec0 = time(NULL); timeTotalInGame = 0;
      timeTemp = time(NULL); tpg.genTeams(t); timeGenTeams = time(NULL) - timeTemp; //replacement
      runEval(alev,activeRom,tpg,legal_actions,t,phase,visual,stateful,timeTotalInGame,-1,totalEval,totalFrame,featureMap); //evaluation
      tpg.hostFitnessMode(activeRom);//this gets toyed with in runEval, so set it back to activeRom prio to selection
      timeTemp=time(NULL); tpg.setEliteTeams(t,_TRAIN_PHASE,objectives,true); tpg.selTeams(t); timeSel=time(NULL)-timeTemp; //selection
      //tpg.printTeamInfo(t,phase,false); //detailed team reporting
      timeTemp=time(NULL); tpg.cleanup(t, false); timeCleanup=time(NULL)-timeTemp; //delete teams, programs marked in selection (when to prune?)
      if (t % CHKP_MOD == 0) tpg.writeCheckpoint(t,_TRAIN_PHASE,true);
      timeGenSec1 = time(NULL);
      cout << "TPG::genTime t " << t  << " sec " << timeGenSec1 - timeGenSec0 << " (" << timeTotalInGame << " InGame, ";
      cout << timeGenTeams << " genTeams, " << timeInit  << " initTeams, " << timeSel << " selTeams, " << timeCleanup << " cleanup)" << " totalEval " << totalEval  << " totalFrame " << totalFrame;
      cout << " activeRom " << alev[activeRom]->getString("rom_file")  << endl;
      tpg.hostDistanceMode((int)(drand48()*NUM_TEAM_DISTANCE_MEASURES)); //diversity switching

      if (t % romSwitchMod == 0){ //rom switching
         int prevRom = activeRom;
         do
            activeRom = (int)(drand48()*alev.size());
         while (activeRom == prevRom);
      }
   }
   tpg.finalize();
   cout << "Goodbye cruel world." << endl;
   return 0;
}

