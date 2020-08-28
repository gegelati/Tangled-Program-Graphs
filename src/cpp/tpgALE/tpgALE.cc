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

#define ALE_NUM_POINT_AUX_DOUBLES 4 //gameScore, instructionsPerDecision, teamsPerDecision, featuresPerDecision
#define NUM_FIT_MODE 1
#define ALE_NUM_TEAM_DISTANCE_MEASURES 3
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

clock_t timeStartClockEval;
int gen;

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
void runEval(ALEInterface &aleEval, TPG &tpg, int t, int phase, bool visual, bool stateful, int &timeGenTotalInGame, long hostToReplay, long &eval, long &decision, FeatureMap &featureMap){
   bool timing = true;
   clock_t timeStartClockDecision;
   long decisionMilliseconds = 0;

   long totalGameFramesThisEval = 0;
   ActionVect legal_actions = aleEval.getMinimalActionSet();
   //ActionVect legal_actions = aleEval.getLegalActionSet();
   //gameplay data
   vector < double > behaviourSequence; behaviourSequence.reserve(MAX_BEHAVIOUR_STEPS); vector < double > tmpBehaviourSequence;
   vector < double > currentState; currentState.reserve(tpg.dimBehavioural()); currentState.resize(tpg.dimBehavioural());
   int currentAction;
   vector<double> rewards; rewards.reserve(ALE_NUM_POINT_AUX_DOUBLES); rewards.resize(ALE_NUM_POINT_AUX_DOUBLES);
   set <learner*, LearnerBidLexicalCompare> learnersRanked;
   set <learner*, LearnerBidLexicalCompare> winningLearners;
   int step,timeStartGame;
   //profile points data
   vector < double > fullState; fullState.reserve(tpg.dimBehavioural()); fullState.resize(tpg.dimBehavioural());
   set < long > fullFeatures; for (int i = 0; i < tpg.dimBehavioural(); i++) fullFeatures.insert(i);
   int prevProfileId = -1; int newProfilePoints = 0;
   //reporting data
   char tmpChar[80];
   set <long> featTmp;
   long evalInit = eval;
   long decisionInit = decision;
   long numDecisionInstructions, numDecisionInstructionsPerGameSum, numDecisionInstructionsPerEvalSum;
   long numVisitedTeamsPerGameSum, numVisitedTeamsPerEvalSum;
   long numDecisionFeaturesPerGameSum, numDecisionFeaturesPerEvalSum;
   vector < set < long > > decisionFeatures;
   set < team * > teamTeams;
   set < learner * > teamLearners;
   map < learner *, long > learnerUsage;
   set <team*> visitedTeams;

   vector <team*> teams; tpg.getTeams(teams,true);
   int meanScore=0;
   int minScore=0;
   int maxScore=0;
   for (size_t i = 0; i < teams.size(); i++){
      int scoreThisRoot=0;
      if (hostToReplay > -1 && teams[i]->id() != hostToReplay) continue;
      //get all features indexed by this policy (not just from active programs)
      set <long> features;
      tpg.policyFeatures(teams[i]->id(),features,false);
      //calculate number of evaluations for this team
      int numEval = tpg.episodesPerGeneration() - teams[i]->numOutcomes(_TRAIN_PHASE,tpg.hostFitnessMode());
      if ( phase == _TRAIN_PHASE) numEval = min(tpg.episodesPerGeneration(),tpg.numStoredOutcomesPerHost(_TRAIN_PHASE) - teams[i]->numOutcomes(_TRAIN_PHASE,tpg.hostFitnessMode()));
      else if (phase == _TEST_PHASE) numEval = tpg.numStoredOutcomesPerHost(_TEST_PHASE);
      else numEval = tpg.numStoredOutcomesPerHost(_VALIDATION_PHASE);
      if (tpg.verbose()){
         tpg.getAllNodes(teams[i], teamTeams, teamLearners);
         learnerUsage.clear();
         for(set<learner*> :: iterator it = teamLearners.begin(); it != teamLearners.end();++it)
            learnerUsage.insert(pair<learner*,long>((*it),0));
      }
      numDecisionInstructionsPerEvalSum = 0; numVisitedTeamsPerEvalSum = 0; numDecisionFeaturesPerEvalSum = 0;
      //run evaluations
      for (int e = 0; e < numEval; e++){
         if (tpg.verbose())
             cout << "runEval t " << t << " tm " << teams[i]->id() << " numOut " << teams[i]->numOutcomes(phase,tpg.hostFitnessMode());
         step = 0; for (int r = 0; r < ALE_NUM_POINT_AUX_DOUBLES; r++) rewards[r] = 0;
         numDecisionInstructionsPerGameSum = 0; numVisitedTeamsPerGameSum = 0; numDecisionFeaturesPerGameSum = 0;
         int decisionFeatCount = 0;
         timeStartGame = time(NULL);
         aleEval.reset_game();
         while (!aleEval.game_over()) {
            if (timing) timeStartClockDecision = clock();
            const ALEScreen screen = aleEval.getScreen();
            featureMap.getFeatures(screen,currentState,features,"");
            if (tpg.verbose() && visual) cout << endl << "=== policy trace ===========================================================" << endl;
            if (!stateful || (stateful && step == 0)) tpg.clearRegisters(teams[i]);
            int atomicAction = tpg.getAction(teams[i], currentState, true, learnersRanked, winningLearners, numDecisionInstructions, decisionFeatures, visitedTeams, tpg.verbose());
            if(visual){
               featureMap.saveFrames(true);
               featTmp.clear();
               //sprintf(tmpChar, "%08d%s",step,".pol");
               //featureMap.getFeatures(screen,currentState,features,tmpChar); //pol vis
               for (int df = 0; df < decisionFeatures.size(); df++)
                  featTmp.insert(decisionFeatures[df].begin(), decisionFeatures[df].end());
               sprintf(tmpChar, "%08d%s",step,".des");
               featureMap.getFeatures(screen,currentState,featTmp,tmpChar); //policy viz per decision
               featureMap.saveFrames(false);
            }
            //if(visual){featureMap.saveFrames(true); featureMap.getFeatures(screen,currentState,features,step); featureMap.saveFrames(false);}
            //cout << endl << "decisionFeatures";
            //for(set<long> :: iterator it = decisionFeatures.begin(); it != decisionFeatures.end();++it){
            // cout << " " << (*it);
            //}
            //cout << endl;
            rewards[0] += aleEval.act(legal_actions[(atomicAction*-1)-1]);//atomic actions are represented as negatives in tpg
            //cout << "act: " << legal_actions[(atomicAction*-1)-1] << " reward: " << rewards[0] << endl;
            if (drand48() < tpg.pAddProfilePoint() && teams[i]->id() != prevProfileId){
               featureMap.getFeatures(screen,fullState,fullFeatures,"profile");
               tpg.addProfilePoint(fullState,rewards,phase,t);
               prevProfileId = teams[i]->id();
               newProfilePoints++;
            }
            behaviourSequence.push_back((double)(atomicAction));
            step++;
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
               cout << endl << "tpg-activeNodes";
               for(set<learner*, LearnerBidLexicalCompare> :: iterator it = winningLearners.begin(); it != winningLearners.end();++it){
                  learnerUsage.find(*it)->second++;
                  cout << " s" << (*it)->id();
               }
               for(set<team*> :: iterator it = visitedTeams.begin(); it != visitedTeams.end();++it)
                 cout << " h" << (*it)->id();
               cout << endl;

               cout << endl << "lUsageOnTime";
               for (map<learner*,long>::iterator it=learnerUsage.begin(); it!=learnerUsage.end(); ++it){
                  if (winningLearners.find(it->first) != winningLearners.end())
                     cout << " 1";
                  else
                     cout << " 0";
               }
               cout << endl;
            }
            if (timing) decisionMilliseconds += (clock() - timeStartClockDecision) / (double)(CLOCKS_PER_SEC / 1000);
         }
         timeGenTotalInGame += (time(NULL) - timeStartGame);
         rewards[1] = (double)numDecisionInstructionsPerGameSum/step;//used for reporting, not as reward
         rewards[2] = (double)numVisitedTeamsPerGameSum/step;//used for reporting, not as reward
         rewards[3] = (double)numDecisionFeaturesPerGameSum/step;//used for reporting, not as reward
         decision+=step;
         //get behaviour sequence for last MAX_BEHAVIOUR_STEPS interactions
         //int start = min((1+tpg.dimBehavioural())*MAX_BEHAVIOUR_STEPS,(int)behaviourSequence.size());
         int start = min(MAX_BEHAVIOUR_STEPS,(int)behaviourSequence.size());
         for (size_t b = behaviourSequence.size()-start; b < behaviourSequence.size(); b++)
            tmpBehaviourSequence.push_back(behaviourSequence[b]);
         tpg.setOutcome(teams[i],tmpBehaviourSequence, rewards, phase, t);//behaviourSequence is actions only
         eval++;
         behaviourSequence.clear(); tmpBehaviourSequence.clear();
         scoreThisRoot+=rewards[0];
         if (tpg.verbose()){
          cout << fixed << setprecision(4);
          cout << " gameScore " << rewards[0] ;//<< " steps " << step  << " meanDecisionInst " << numDecisionInstructionsPerGameSum/(double)step;
            cout << " meanVisitedTeams " << numVisitedTeamsPerGameSum/(double)step;
           cout << " meanDecisionFeatures " << numDecisionFeaturesPerGameSum/(double)step << endl;
         }
         totalGameFramesThisEval += aleEval.getEpisodeFrameNumber();
         //cout << "totalGameFramesThisEval " << totalGameFramesThisEval << " enum " << aleEval.getEpisodeFrameNumber() << endl;
         aleEval.reset_game();
      }
      if (tpg.verbose()){
         cout << "tpgEvalStats policy " << teams[i]->id();
         cout << " meanDecisionInst " << numDecisionInstructionsPerEvalSum/(double)(decision-decisionInit);
         cout << " meanVisitedTeams " << numVisitedTeamsPerEvalSum/(double)(decision-decisionInit);
         cout << " meanDecisionFeatures " << numDecisionFeaturesPerEvalSum/(double)(decision-decisionInit);
         cout << " lUseAllGame";
         for (map<learner*,long>::iterator it=learnerUsage.begin(); it!=learnerUsage.end(); ++it){
            //only accurate when a single team has been eveluated in numEval games (under test)
            cout << " [" << it->first->id() << ":" << it->second/(double)(decision-decisionInit) << "]";
            it->second = 0;
         }
         cout << endl;
      }

      scoreThisRoot/=numEval;
      meanScore+=scoreThisRoot;
      if(scoreThisRoot>maxScore) maxScore = scoreThisRoot;
      if(scoreThisRoot<minScore) minScore = scoreThisRoot;
   }
   meanScore/=teams.size();
  // if (!timing) cout << "tpgALE::runEval t " << t << " numProfilePoints " << tpg.numProfilePoints() << " newProfilePoints " << newProfilePoints << " numEval " << eval - evalInit  << " numFrame " << decision - decisionInit << endl;

  cout<<std::setw(20)<<gen<<std::setw(20) <<minScore<<std::setw(20) <<meanScore<<std::setw(20) <<maxScore;

   if (timing){
      double sec = (clock() - timeStartClockEval) / (double)CLOCKS_PER_SEC;
      //cout << "tpgALE::runEval timing";
      cout /*<< " secondsThisEval " */<< std::setw(20) <<sec << endl;
     /* cout << " millisecondsPerDecision " << (double)decisionMilliseconds / (double)decision;
      cout << " decisionsPerSecond " <<(double)decision / sec;
      cout << " totalDecisionsThisEval " << decision;
      cout << " totalGameFramesThisEval " << totalGameFramesThisEval;
      cout << " framesPerSecond " <<  (double)totalGameFramesThisEval / sec << endl;*/
   }
}
/***********************************************************************************************************************/
int main(int argc, char** argv) {
   ifstream ifs; ofstream ofs;
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
   bool stateful = false;
   int statMod = -1;
   long tMain = 1;
   int tPickup = -1;
   int tStart = 0;
   bool verbose = false;
   bool visual = false;

   int option_char;
   while ((option_char = getopt (argc, argv, "C:f:i:MO:R:r:s:T:t:Vv")) != -1)
      switch (option_char)
      {
         case 'C': checkpoint = true; checkpointInPhase = atoi(optarg); break;
         case 'f': hostFitnessMode = atoi(optarg); break;
         case 'i': id = atoi(optarg); break;
         case 'M': stateful = true; break;
         case 'O': statMod = atoi(optarg); break;
         case 'R': replay = true; hostToReplay = atoi(optarg); break;
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
                   cout << "-r <ROM> (Atari 2600 ROM file, named as per ALE supported ROMs)" << endl;
                   cout << "-R <hostIdToReplay> (Load and replay a specific host ID. Requires checkpoint file and options -C, and -t.)" << endl;
                   cout << "-s <seed> (Random seed)" << endl;
                   cout << "-T <generations>" << endl;
                   cout << "-t <t> (When loading populations form a checkpoint, t is the generation of the checkpoint file. Requires checkpoint file and option -C.)" << endl;
                   cout << "-V (Run with visualization.)" << endl;
                   exit(0);
      }
   cout << "====== tpgALE parameters:" << endl << endl;
   cout << "chechkpoint " << checkpoint << endl;
   cout << "checkPointInMode " << checkpointInPhase << endl;
   cout << "hostFitnessMode " << hostFitnessMode << endl;
   cout << "statMod " << statMod << endl;
   cout << "replay " << replay << endl;
   cout << "rom " << rom << endl;
   cout << "hostToReplay " << hostToReplay << endl;
   cout << "seed " << seed << endl;
   cout << "stateful " << stateful << endl;
   cout << "tMain " << tMain << endl;
   cout << "tPickup " << tPickup << endl;
   cout << "visual " << visual << endl;

   //timing
   int timeGenSec0; int timeGenSec1; int timeGenTeams; int timeTotalInGame;
   int timeTemp; int timeInit; int timeSel; int timeCleanup;

   //ALE setup
   ALEInterface ale;
   ale.setBool("color_averaging",false);
   ale.setInt("random_seed", seed);
   ale.setInt("max_num_frames_per_episode", MAX_NUM_FRAMES_PER_EPISODE);
   ale.setFloat("repeat_action_probability", 0.25);//this is the default
   ale.setBool("display_screen", visual);
   ale.setBool("sound", visual);
   char romBin[80]; sprintf(romBin, "%s/%s%s","../roms/",rom,".bin");
   cout << "ROM " << romBin << endl;
   ale.loadROM(romBin);//(Also resets the system for new settings to take effect.)
   ActionVect legal_actions = ale.getMinimalActionSet();
   cout << "ale MinimalActionSet size " << ale.getMinimalActionSet().size()  << "|" << vecToStr(legal_actions)<< endl;
   char romBackground[80]; sprintf(romBackground, "%s/%s%s","backgrounds/",rom,".bg");
   FeatureMap featureMap("", QUANTIZE, SECAM, BACKGROUND_DETECTION);
   //featureMap.saveFrames(visual);
   cout << "ale FeatureMap size " << featureMap.numFeatures() << endl << endl;

   //TPG Parameter Setup
   TPG tpg(seed);
   tpg.id(id);
   tpg.dimBehavioural(featureMap.numFeatures());
   tpg.rangeBehavioural(featureMap.minFeatureVal(),featureMap.maxFeatureVal(),true);//discrete sensor inputs
   tpg.numPointAuxDouble(ALE_NUM_POINT_AUX_DOUBLES);
   tpg.numFitMode(NUM_FIT_MODE);
   tpg.hostFitnessMode(hostFitnessMode);
   tpg.setParams(initUniformPointALE);
   tpg.numAtomicActions(ale.getMinimalActionSet().size());
   tpg.verbose(verbose);
   long totalEval = 0;
   long totalFrame = 0;

   //loading populations from a chackpoint file
   if (checkpoint)
   {
      tpg.readCheckpoint(tPickup,checkpointInPhase);
      //tpg.recalculateLearnerRefs();
      //tpg.cleanup(tPickup,false);// don't prune learners because active/inactive is not accurate
      if (replay){
         tpg.clearNumEvals();
         //runEval(ale,tpg,tPickup,_VALIDATION_PHASE,visual,timeTotalInGame,hostToReplay,totalEval,totalFrame,featureMap);
         //hostToReplay = (tpg.getBestTeam(_VALIDATION_PHASE))->id();
         //////tpg.cleanup(tPickup,true,hostToReplay);
         //////tpg.resetOutcomes(_VALIDATION_PHASE);
         runEval(ale,tpg,tPickup,_TEST_PHASE,visual,stateful,timeTotalInGame,hostToReplay,totalEval,totalFrame,featureMap);
         //tpg.cleanup(tPickup,true,hostToReplay);
         tpg.printTeamInfo(tPickup,_TEST_PHASE,false,hostToReplay); cout << endl;
         tpg.printHostGraphsDFS((long)tPickup,true,_TEST_PHASE);//single best only
         tpg.writeCheckpoint(tPickup,_TEST_PHASE,false);
         cout << "Goodbye cruel world." << endl;
         return 0;
      }
   }
   if (!checkpoint){ timeTemp = time(NULL); tpg.initTeams(); timeInit = time(NULL)-timeTemp; }
   /* Main training loop. */

   cout << setprecision(2) << fixed << left;
   gen=0;
   timeStartClockEval = clock();
   cout<<std::setw(20) <<"generation"<<std::setw(20) <<"minScore"<<std::setw(20) <<"meanScore"<<std::setw(20) <<"maxScore"<<std::setw(20)<<"totalTime"<<endl;

    for (long t = tStart+1; t <= tMain; t++)
   {
      timeGenSec0 = time(NULL); timeTotalInGame = 0;
      timeTemp = time(NULL); tpg.genTeams(t); timeGenTeams = time(NULL) - timeTemp; //replacement
      runEval(ale,tpg,t,phase,visual,stateful,timeTotalInGame,-1,totalEval,totalFrame,featureMap); //evaluation
      tpg.hostDistanceMode((int)(drand48()*ALE_NUM_TEAM_DISTANCE_MEASURES)); //diversity switching
      timeTemp=time(NULL);tpg.selTeams(t); timeSel=time(NULL)-timeTemp; //selection
      //tpg.printTeamInfo(t,phase,true); cout << endl; //detailed team reporting (best only)
      timeTemp=time(NULL); tpg.cleanup(t, false); timeCleanup=time(NULL)-timeTemp; //delete teams, programs marked in selection (when to prune?)
      if (t % CHKP_MOD == 0) tpg.writeCheckpoint(t,_TRAIN_PHASE,true);
      timeGenSec1 = time(NULL);
      //cout << "TPG::genTime t " << t  << " sec " << timeGenSec1 - timeGenSec0 << " (" << timeTotalInGame << " InGame, ";
      //cout << timeGenTeams << " genTeams, " << timeInit  << " initTeams, " << timeSel << " selTeams, " << timeCleanup << " cleanup)" << " totalEval " << totalEval  << " totalFrame " << totalFrame << endl;
	gen++;
   }
   tpg.finalize();
   cout << "Goodbye cruel world." << endl;
   return 0;
}
