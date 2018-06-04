#include <string>  // for memcpy
#include <stdlib.h>  // for rand
#include <iterator>
#include <time.h>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include "TPG.h"
#include <sys/stat.h>

/********************************************************************************************/
TPG::TPG(int seed)
{
   team_count = 10000*seed;
   learner_count = 10000*seed;
   point_count = 10000*seed;

   _seed = seed;
   _id = -1;
   _simTime = 0;
}

TPG::~TPG()
{
   ;
}

/*********************************************************************************************/
void TPG::adjustIds(int m){
   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      (*leiter)->setId((*leiter)->id()*m);
   set < team * > :: iterator teiter, teiterEnd;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      (*teiter)->id((*teiter)->id()*m);

   }
}
/********************************************************************************************/
void TPG::checkRefCounts(const char *msg){
   int nrefs = 0;
   int sumTeamSizes = 0;
   set < learner * > :: iterator leiter, leiterEnd;
   set < team * > :: iterator teiter, teiterEnd;

   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      nrefs += (*leiter)->refs();
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      sumTeamSizes += (*teiter)->size();
   if(sumTeamSizes != nrefs){
      cerr << "nrefs " << nrefs << " sumTeamSize " << sumTeamSizes << endl;
      die(__FILE__, __FUNCTION__, __LINE__, msg);
   }
}
/********************************************************************************************/
void TPG::cleanup(long t, bool prune, long &hostToReplay)
{
   /* Delete learners that are no longer referenced. */
   /* Delete cached outcomes for deleted points, which are clones of each point stored locally by teams */

   set < point * > :: iterator poiter, poiterend;
   set < team *, teamIdComp > :: iterator teiter, teiterbegin, teiterend;
   set < learner * > :: iterator leiter, leiterend;

   vector < point * > pointvec;
   vector < learner * > learnervec;

   size_t size;
   size_t i;

   // Prune the learners
   if (prune)
      pruneLearners(hostToReplay);


   // Collect learners into learnervec.

   for(leiter = _L.begin(), leiterend = _L.end(); leiter != leiterend; leiter++)
      learnervec.push_back(*leiter);

   // Delete learners with zero references.

   size = learnervec.size();
   for(size_t i = 0; i < size; i++)
   {
      if(learnervec[i]->refs() == 0)
      {
         // Zero references, delete this learner.
         _L.erase(learnervec[i]);
         delete learnervec[i];
      }
   }

   int sumTeamSizes=0;
   int nrefs = 0;
   int sumNumOutcomes = 0;

   cout << "TPG::cleanup ";
   cout << _L.size() << " learners, " << _M.size() << " teams ";
   for(teiter = _M.begin(), sumTeamSizes = sumNumOutcomes = 0; teiter != _M.end(); teiter++)
   {
      sumTeamSizes += (*teiter)->size();
      sumNumOutcomes += (*teiter)->numOutcomes(_TRAIN_PHASE);
   }

   for(leiter = _L.begin(), nrefs = 0; leiter != _L.end(); leiter++)
      nrefs += (*leiter)->refs();

   cout << ", sumTeamSizes " << sumTeamSizes << ", nrefs " << nrefs << ", sumNumOutcomes " << sumNumOutcomes;
   cout << endl;

   if(sumTeamSizes != nrefs)
      die(__FILE__, __FUNCTION__, __LINE__, "something messed up during cleanup");
}

/********************************************************************************************/
void TPG::debugRefs(long t)
{
   set < team *, teamIdComp > :: iterator teiter, itBegin, teiterEnd;
   set < learner * > :: iterator leiter, leiterEnd;

   int nrefs = 0;
   int sumTeamSizes = 0;
   int sumNumOutcomes = 0;

   cout << "TPG::debugRefs ";
   cout << _L.size() << " learners, " << _M.size() << " teams";
   for(teiter = _M.begin(), sumTeamSizes = sumNumOutcomes = 0; teiter != _M.end(); teiter++)
   {
      sumTeamSizes += (*teiter)->size();
      sumNumOutcomes += (*teiter)->numOutcomes(_TRAIN_PHASE);
   }

   for(leiter = _L.begin(), nrefs = 0; leiter != _L.end(); leiter++)
      nrefs += (*leiter)->refs();

   cout << ", sumTeamSizes " << sumTeamSizes << ", nrefs " << nrefs << ", sumNumOutcomes " << sumNumOutcomes;
   cout << endl;

   if(sumTeamSizes != nrefs)
      die(__FILE__, __FUNCTION__, __LINE__, "something messed up during cleanup");

   //#endif
}

/********************************************************************************************
 * No diversity.
 */
void TPG::diversityMode0(long t){
   set < team *, teamIdComp > :: iterator teiter, teiterend;

   if (_normalizeFitness){
      double out;
      double maxOut = -numeric_limits<double>::max();
      double minOut = numeric_limits<double>::max();
      for(teiter = _M.begin(); teiter != _M.end(); teiter++)
         if ((*teiter)->root()){
            out = (*teiter)->getMeanOutcome(_TRAIN_PHASE,_MEAN_OUT_PROP,_fitMode,_fitMode,false);
            maxOut = max(out,maxOut);
            minOut = min(out,minOut);
         }
      for(teiter = _M.begin(); teiter != _M.end(); teiter++)
         if ((*teiter)->root()){
            (*teiter)->score(((*teiter)->getMeanOutcome(_TRAIN_PHASE,_MEAN_OUT_PROP,_fitMode,_fitMode,false)-minOut)/(maxOut-minOut));
            (*teiter)->fit((*teiter)->score());
         }
   }
   else {
      for(teiter = _M.begin(); teiter != _M.end(); teiter++)
         if ((*teiter)->root()){
            (*teiter)->score((*teiter)->getMeanOutcome(_TRAIN_PHASE,_MEAN_OUT_PROP,_fitMode,_fitMode,false));
            (*teiter)->fit((*teiter)->score());//(*teiter)->getMeanOutcome(_fitMode,_TRAIN_PHASE,_MEAN_OUT_PROP));
         }
   }
}

/*********************************************************************************************
 * Helper struct for distance comparisons.
 */

struct distanceInstance {
   double distance;
   bool fromArchive;
   distanceInstance(double d,bool a):distance(d),fromArchive(a) {}
} ;

bool compareByDistance(const distanceInstance &a, const distanceInstance &b)
{
   return a.distance < b.distance;
}

/***********************************************************************************************************/
void TPG::finalize()
{
   set < team * , teamIdComp > :: iterator teiter;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      delete *teiter;

   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      delete *leiter; 

   while (_profilePointsFIFO.size() != 0){
      point *p = _profilePointsFIFO.front();
      _profilePointsFIFO.pop_front();
      delete p;
   }
}

/********************************************************************************************/
void TPG::genTeams(long t)
{
   vector < team * > parent;
   team *pm;
   team *cm;

   vector < learner * > lpop;

   set < team *, teamIdComp > :: iterator teiter, teiterend;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      if((*teiter)->root())
         parent.push_back(*teiter);

   updateLearnerProfiles();//synch with FIFO state observation archive
   lpop.insert(lpop.begin(), _L.begin(), _L.end());

   int numNewTeams = 0;

   //for (size_t i = 0; i < parent.size(); i++)
   //{
   //   pm = parent[(int) (drand48() * parent.size())];
   //   genTeams(t, pm, &cm, lpop);
   //   _M.insert(cm);
   //   numNewTeams++;
   //}

   while (setRoots() < _Msize){
      pm = parent[(int) (drand48() * parent.size())];
      genTeams(t, pm, &cm, lpop);
      _M.insert(cm);
      numNewTeams++;
   }

   cout << "TPG::genTeams t " << t  << " Msize " << _M.size() << " Lsize " << _L.size() << " numNewTeams " << numNewTeams << " numRoot " << setRoots() << endl;
}

/********************************************************************************************/
void TPG::genTeams(long t,
      team *pm,
      team **cm,
      vector < learner * > &lpop)
{
   set < learner * > plearners;
   set < learner * > clearners;

   set < learner * > :: iterator leiter;

   vector < learner * > learnervec;

   int i;

   double b;

   learner *lr;

   bool changedL;
   bool changedM;

   size_t lsize;

   lsize = lpop.size();

   pm->members(&plearners);
   *cm = new team(t, team_count++);

   for(leiter = plearners.begin(); leiter != plearners.end(); leiter++)
      (*cm)->addLearner(*leiter);

   learnervec.insert(learnervec.begin(), plearners.begin(), plearners.end());

   vector < team * > teams;
   set < team *, teamIdComp > :: iterator teiter, teiterend;

   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      teams.push_back(*teiter);

   // Remove learners.
   for(b = 1.0; drand48() < b && (*cm)->size() > 2; b = b * _pmd)
   {
      do
      { 
         i = (int) (drand48() * learnervec.size());
      }
      while(learnervec[i]->action() < 0 && (*cm)->numAtomic() < 2);//never remove the fail-safe atomic learner
      (*cm)->removeLearner(learnervec[i]);

      clearners.clear();//this could maybe move into the members function
      (*cm)->members(&clearners);
      learnervec.clear();
      learnervec.insert(learnervec.begin(), clearners.begin(), clearners.end());
   }

   // Add learners.
   //for(b = 1.0; drand48() < b && (*cm)->size() < _omega && (*cm)->size() < lsize; b = b * _pma)
   for(b = 1.0; drand48() < b && (*cm)->size() < lsize; b = b * _pma)
   {
      do
      { 
         i = (int) (drand48() * lsize);
      }
      while(lpop[i]->action() == (*cm)->id() || (*cm)->addLearner(lpop[i]) == false); //avoid loop-backs (impossible anyway since cm is new)
   }

   // Mutate learners.

   clearners.clear();
   (*cm)->members(&clearners);

   changedM = false;

   do
   { 
      for(leiter = clearners.begin(); leiter != clearners.end(); leiter++)
      {
         if(drand48() < _pmm)
         {
            changedM = true;

            (*cm)->removeLearner(*leiter);

            lr = new learner(t, **leiter, learner_count++);

            do
            { 
               changedL = lr->muBid(_pBidDelete, _pBidAdd, _pBidSwap, _pBidMutate, _maxProgSize);
            }
            while(changedL == false);

            genUniqueLearner(lr, _L); /* Make sure bid unique. */

            if(drand48() < _pmn){
               if (t == 1 || (lr->action() < 0 && (*cm)->numAtomic() < 2 ? true : drand48() < _pAtomic)) //first generation is atomic, and always mutate the fail-safe atomic learner to another atomic
                  lr->muAction(-1 - ((long) (drand48() * numAtomicActions()))); //atomic actions are negatives: -1 down to -numAtomicActions()
               else{
                  long a;
                  do{ 
                     a = (long) (drand48() * teams.size()); //meta-actions are between 0 and teams.size() - 1
                  } while(teams[a]->gtime() == t || teams[a]->id() == (*cm)->id());//avoid loop-backs and avoid pointing to a team created this generation
                  lr->muAction(teams[a]->id());
               }
            }
            (*cm)->addLearner(lr);
            _L.insert(lr);
         }
      }
   }
   while(changedM == false);

   /* Increment references. */

   clearners.clear();
   (*cm)->members(&clearners);

   for(leiter = clearners.begin(); leiter != clearners.end(); leiter++)
      (*leiter)->refInc();

   (*cm)->parentScore(pm->fit());
   (*cm)->addAncestor(pm->id());

   //copy all parents ancestors
   vector <long> parentAncestors;
   pm->getAllAncestors(parentAncestors);
   for (size_t i = 0; i < parentAncestors.size(); i++)
      (*cm)->addAncestor(parentAncestors[i]);
}

/********************************************************************************************/
// Keep altering 'lr' as long as its bid profile is the same as the bid
//   profile of one of the learners in 'learners'. 
void TPG::genUniqueLearner(learner *lr,
      set < learner * > learners)
{
   vector < double > profile;
   set < learner * > :: iterator leiter;

   vector < double > state;

   size_t i;

   bool stop = false;

   bool changedL;

   //double *REG = (double *) alloca(REGISTERS * sizeof(double));

#ifdef MYDEBUG
   int n = 0;
   deque<point *>::iterator itp = _profilePointsFIFO.begin();
   while (itp != _profilePointsFIFO.end()){
      (*itp++)->behaviouralState(state);
      cout << "TPG::genUniqueLearner _profilePointFIFO n " << ++n << " | " << vecToStr(state) << endl;
   }

   for(i = 0, leiter = learners.begin(); leiter != learners.end(); leiter++, i++)
   {
      (*leiter)->getProfile(profile);
      cout << "TPG::genUniqueLearner " << lr->id() << " " << i << " " << (*leiter)->id() << " profile " << vecToStr(profile) << endl;
   }
#endif

   while(stop == false)
   {
      profile.clear();

      deque<point *>::iterator it = _profilePointsFIFO.begin();
      while (it != _profilePointsFIFO.end()){
         (*it++)->behaviouralState(state);
         profile.push_back(lr->bid(&state[0]));
      }

#ifdef MYDEBUG
      cout << "TPG::genUniqueLearner " << lr->id() << " profile " << vecToStr(profile);
#endif

      if(profile.size() != _profilePointsFIFO.size())
         die(__FILE__, __FUNCTION__, __LINE__, "bad profile size");

      for(leiter = learners.begin(); leiter != learners.end(); leiter++)
         if((*leiter)->isProfileEqualTo(profile))
            break;

      if(leiter == learners.end())
      {
         //Not a duplicate bidder.

#ifdef MYDEBUG
         cout << " is unique" << endl;
#endif

         stop = true;
      }
      else
      {
         // Duplicate bidder.

#ifdef MYDEBUG
         cout << " is a duplicate of " << (*leiter)->id() << endl;
#endif

         do
         {
            changedL = lr->muBid(_pBidDelete, _pBidAdd, _pBidSwap, _pBidMutate, _maxProgSize);
         }
         while(changedL == false);
      }
   }

   lr->setProfile(profile);
}


/********************************************************************************************/
team * TPG::getBestTeam(int phase){
   set < team * , teamIdComp> :: iterator teiter, teiterend;
   double maxMeanOut = -numeric_limits<double>::max();
   double meanOut = 0;
   currentChampion = (*_M.begin());
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      if ((*teiter)->root()){ 
         meanOut = (*teiter)->getMeanOutcome(phase,_MEAN_OUT_PROP,_fitMode,_fitMode,false);
         //cout << "sbb::getBestTeamAll phase " << phase << " tm " << (*teiter)->id();
         //cout << " score " << meanOut << " numOutcome " << (*teiter)->numOutcomes(phase) << endl;
         if (meanOut > maxMeanOut){
            currentChampion = (*teiter);
            maxMeanOut = meanOut;
         }
      }
   }
   cout << "sbb::getBestTeam " << " id " << currentChampion->id() << " " << currentChampion->getMeanOutcome(phase,_MEAN_OUT_PROP,_fitMode,_fitMode,false) << endl;
   return currentChampion;
}

/********************************************************************************************/
void TPG::setEliteTeams(long t, long phase, const vector <int> &objectives,vector < double > &highScores, vector < double > &lowScores, bool multitask){
   set < team * , teamIdComp> :: iterator teiter, teiterend;
   _eliteTeams.clear();
   team * eliteTeamSG;
   team * eliteTeamMT;
   set <team*> visitedTeams;

   //Find the elite single-task policy relative to each objective
   double maxMeanOut;
   double meanOut;
   for (size_t o = 0; o < objectives.size(); o++){
      maxMeanOut = -numeric_limits<double>::max();
      for(teiter = _M.begin(); teiter != _M.end(); teiter++){
         if ((*teiter)->numOutcomes(phase,objectives[o]) > 0){
            meanOut = (*teiter)->getMeanOutcome(phase, _MEAN_OUT_PROP, objectives[o],objectives[o],false);
            if (meanOut > maxMeanOut){
               eliteTeamSG = (*teiter);
               maxMeanOut = meanOut;
            }
         }
      }
      if (maxMeanOut > -numeric_limits<double>::max()){
         _eliteTeams.push_back(eliteTeamSG);
         cout << "TPG::setEliteTeamsSG t " << t << " phase " << phase << " obj " << objectives[o] << " id " << eliteTeamSG->id() << " ";
         printTeamInfo(t, phase, false, eliteTeamSG->id());
      }
   }
   cout << endl;

   if (multitask){

      //// Find the elite multi-task policy wrt DQN score
      //set <team*, teamScoreLexicalCompare> teamsRanked;
      //for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      //   double score = -numeric_limits<double>::max();
      //   vector <double> normalizedScores;
      //   for (size_t o = 0; o < objectives.size(); o++){
      //      if ((*teiter)->numOutcomes(phase,objectives[o]) > 0){
      //         double rawMeanScore = (*teiter)->getMeanOutcome(phase, _MEAN_OUT_PROP, objectives[o],objectives[o],false);
      //         normalizedScores.push_back(100*(( rawMeanScore - lowScores[objectives[o]])/(highScores[objectives[o]] - lowScores[objectives[o]])));          
      //      }
      //   }
      //   if (normalizedScores.size() == objectives.size()){  
      //      (*teiter)->score(*min_element(normalizedScores.begin(), normalizedScores.end()));
      //      teamsRanked.insert((*teiter));
      //   }  
      //}
      //if (teamsRanked.size() > 0){
      //   eliteTeamMT = *(teamsRanked.begin());
      //   _eliteTeams.push_back(eliteTeamMT);
      //   cout << "TPG::setEliteTeamsMT t " << t << " phase " << phase << " id " << eliteTeamMT->id() << " minThreshold " << eliteTeamMT->score() << " ";
      //   printTeamInfo(t, phase, false, eliteTeamMT->id());
      //}

      //// OR ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      // Find the elite multi-task policy wrt min and max TPG score thus far

      // Use "fitness" to store a team's DQN threshold, used for reporting only
      for(teiter = _M.begin(); teiter != _M.end(); teiter++){
         double score = -numeric_limits<double>::max();
         vector <double> normalizedScores;
         for (size_t o = 0; o < objectives.size(); o++){
            if ((*teiter)->numOutcomes(phase,objectives[o]) > 0){
               double rawMeanScore = (*teiter)->getMeanOutcome(phase, _MEAN_OUT_PROP, objectives[o],objectives[o],false);
               normalizedScores.push_back(100*(( rawMeanScore - lowScores[objectives[o]])/(highScores[objectives[o]] - lowScores[objectives[o]])));          
            }
         }
         if (normalizedScores.size() == objectives.size())  
            (*teiter)->fit(*min_element(normalizedScores.begin(), normalizedScores.end()));
         else  
            (*teiter)->fit(-numeric_limits<double>::max());
      }

      // Get min and max scores for all objectives, use for normalization
      vector <double> minScores;
      for (size_t i = 0; i < objectives.size(); i++) minScores.push_back(numeric_limits<double>::max());
      vector <double> maxScores;
      for (size_t i = 0; i < objectives.size(); i++) maxScores.push_back(-numeric_limits<double>::max());

      for(teiter = _M.begin(); teiter != _M.end(); teiter++)
         for (size_t o = 0; o < objectives.size(); o++){
            double meanOut = (*teiter)->getMeanOutcome(phase, _MEAN_OUT_PROP, objectives[o],objectives[o],false);
            minScores[o] = min(minScores[o], meanOut);
            maxScores[o] = max(maxScores[o], meanOut);
         }

      // Find the team with the max threshold wrt TPG, that is, the team with the 
      // highest minimum normalized score over all objectives
      set <team*, teamScoreLexicalCompare> teamsRanked;
      for(teiter = _M.begin(); teiter != _M.end(); teiter++){
         double score = -numeric_limits<double>::max();
         vector <double> normalizedScores;
         for (size_t o = 0; o < objectives.size(); o++){
            if ((*teiter)->numOutcomes(phase,objectives[o]) > 0){
               double rawMeanScore = (*teiter)->getMeanOutcome(phase, _MEAN_OUT_PROP, objectives[o],objectives[o],false);
               normalizedScores.push_back(100*(( rawMeanScore - minScores[objectives[o]])/(maxScores[objectives[o]] - minScores[objectives[o]])));          
            }
         }
         if (normalizedScores.size() == objectives.size()){  
            (*teiter)->score(*min_element(normalizedScores.begin(), normalizedScores.end()));
            teamsRanked.insert((*teiter));
         }  
      }
      if (teamsRanked.size() > 0){
         eliteTeamMT = *(teamsRanked.begin());
         _eliteTeams.push_back(eliteTeamMT);
         cout << "TPG::setEliteTeamsMT t " << t << " phase " << phase << " id " << eliteTeamMT->id() << " minThreshold " << eliteTeamMT->score() << " ";
         printTeamInfo(t, phase, false, eliteTeamMT->id());
      }
   }
}

/********************************************************************************************/
void TPG::initTeams()
{
   long a1, a2;
   team *m;
   learner *l;

   // Number of teams to initialize.
   int keep = _Msize - floor(_Msize * _Mgap);

   int tc;

   size_t i, j, k;

   set < team * > :: iterator teiter;

   vector < learner * > learnervec;

   size_t size;

   set < learner * > lused;
   set < learner * > :: iterator leiter;

   vector< team* > teams;

   size_t tsize = 10;

   for(tc = 0; tc < keep; tc++)
   {

      // Get two different actions.

      //a1 is the fail-safe atomic
      a1 = -1 - ((long) (drand48() * numAtomicActions())); //atomic actions are negatives -1 to -numAtomicActions()

      do
      {
         //if (_M.size() > 0 ? drand48() < _pAtomic : true)
         a2 = -1 - ((long) (drand48() * numAtomicActions())); //atomic actions are negatives -1 to -numAtomicActions()
         //else{
         //   a2 = (long) (drand48() * _M.size()); //meta-actions are between 0 and _M.size()
         //   a2 = teams[a2]->id();
         //}
      } while (a1 == a2);

      // Create a new team containing two learners.

      m = new team(1, team_count++);

      l = new learner(1, a1, _maxProgSize, _dimB, learner_count++);
      genUniqueLearner(l, _L);  //Make sure bid unique.
      m->addLearner(l);
      l->refInc();
      _L.insert(l);

      l = new learner(1, a2, _maxProgSize, _dimB, learner_count++);
      genUniqueLearner(l, _L); // Make sure bid unique.
      m->addLearner(l);
      l->refInc();
      _L.insert(l);

      _M.insert(m);
      teams.push_back(m);
#ifdef MYDEBUG
      cout << "TPG::initTeams added " << *m << endl;
#endif
   }

   // Mix the learners up.
   learnervec.insert(learnervec.begin(), _L.begin(), _L.end());

   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
   {
      // Get the learners in the team.
      lused.clear();
      (*teiter)->members(&lused);
      // Get the final team size, make sure not too big.
      size = (int) (drand48() * (_omega + 1));
      if(size > learnervec.size() - 2)
         size = learnervec.size() - 2;

      // Build the team up.
      size_t numTries = learnervec.size()*2;
      size_t t = 0;
      while((*teiter)->size() < size && t <= numTries)
      {
         // Get first learner, make sure not in the team or loop-back.

         i = (int) (drand48() * learnervec.size());

         while(lused.find(learnervec[i]) != lused.end() || learnervec[i]->action() == (*teiter)->id())
            i = (i + 1) % learnervec.size();

         for(k = 0; k < tsize; k++)
         {
            // Get another learner, make sure not in the team or loop-back.

            j = (int) (drand48() * learnervec.size());

            while(lused.find(learnervec[j]) != lused.end() || learnervec[i]->action() == (*teiter)->id())
               j = (j + 1) % learnervec.size();

            // Pick second learner if it has fewer refs.
            if(learnervec[j]->refs() < learnervec[i]->refs())
               i = j;

         }
         (*teiter)->addLearner(learnervec[i]);
         lused.insert(learnervec[i]);
         learnervec[i]->refInc();
      }
   }
   cout << "TPG::initTeams numRoot " << setRoots() << endl;
}

/********************************************************************************************
 * Assign each team a score based on multi-objective Pareto ranking with nov 
 * and performance as objectives
 */
void TPG::paretoRanking(long t, int phase){
   vector < team * > teams;
   set < team *, teamIdComp > :: iterator teiter, teiterend;

   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      if ((*teiter)->root()){ //only consider root nodes
         teams.push_back(*teiter);
         (*teiter)->domBy(0);
         (*teiter)->domOf(0);
         (*teiter)->clearDistances();
         (*teiter)->fit((*teiter)->getMeanOutcome(phase,_MEAN_OUT_PROP,_fitMode,_fitMode,false));
      }

   double d;//d0,d1,d2;
   //measure distances w.r.t rest of population
   for(size_t i = 0; i < teams.size(); i++){
      for(size_t j = 0; j < teams.size(); j++)
         if (j > i){//not the same team and haven't considered this pair yet
            //d0 = teams[i]->symbiontUtilityDistance(teams[j]);
            //d1 = teams[i]->ncdBehaviouralDistance(teams[j], phase);//expensive calc for reporting even when its not used :(
            //d2 = teams[i]->policyFeatureDistance(teams[j]);
            //teams[i]->addDistance(0,d0);
            //teams[i]->addDistance(1,d1);
            //teams[i]->addDistance(2,d2);
            //teams[j]->addDistance(0,d0);
            //teams[j]->addDistance(1,d1);
            //teams[j]->addDistance(2,d2);
            if(_distMode == 0)
               d = teams[i]->symbiontUtilityDistance(teams[j]);
            else if (_distMode == 1)
               d = teams[i]->ncdBehaviouralDistance(teams[j], phase);
            else if (_distMode == 2)
               d = teams[i]->policyFeatureDistance(teams[j], true);//only considers active features
            teams[i]->addDistance(_distMode,d);
            teams[j]->addDistance(_distMode,d);
         }
   }

   /* Pareto Scoring */
   vector < team * > dominatedTeams;
   vector < team * > nonDominatedTeams;
   vector < team * > :: iterator itA;
   vector < team * > :: iterator itB;

   dominatedTeams.clear();
   nonDominatedTeams.clear();
   double novA, novB;
   for(itA = teams.begin(); itA != teams.end(); itA++){
      novA = (*itA)->nov(_distMode,_knnNov); 
      for(itB = teams.begin(); itB != teams.end(); itB++){
         novB = (*itB)->nov(_distMode,_knnNov);
         //check if itB dominates itA
         if ((((*itB)->fit()  > (*itA)->fit()  && isEqual((*itA)->fit(), (*itB)->fit(), _paretoEpsilonTeam) == false) || (novB > novA && isEqual(novA, novB, _paretoEpsilonTeam) == false)) &&
               ((*itB)->fit() >= (*itA)->fit() && novB >= novA))
         {
            (*itA)->domBy((*itA)->domBy()+1);
            (*itB)->domOf((*itB)->domOf()+1);
            if(find(dominatedTeams.begin(), dominatedTeams.end(), *itA) == dominatedTeams.end())
               dominatedTeams.push_back(*itA);
         }
      }
      if ((*itA)->domBy() < 1)
         nonDominatedTeams.push_back(*itA);
   }
   for(size_t i = 0; i < teams.size(); i++){
      teams[i]->score(1-((double)teams[i]->domBy()/teams.size()));
      //teams[i]->score((double)teams[i]->domOf()/teams.size());
   }

   //Novelty reporting
   cout.precision(numeric_limits< double >::digits10+1);

   // When all novelty measures are calculated
   //cout << std::fixed << "sbb::paretoRanking t " << t << " _fitMode " << _fitMode << " _distMode " << _distMode << " numFront " << nonDominatedTeams.size() << " numDom " << dominatedTeams.size() << " [id,fit,nov_0,nov_1,nov_2,score] dominated ";
   //for (size_t i = 0; i < dominatedTeams.size(); i++)
   //cout << std::fixed << "[" << dominatedTeams[i]->id() << "," << dominatedTeams[i]->fit() << "," << dominatedTeams[i]->nov(0,_knnNov) << "," << dominatedTeams[i]->nov(1,_knnNov) << "," << dominatedTeams[i]->nov(2,_knnNov) << "," << dominatedTeams[i]->score() << "] ";
   //cout << " nonDominated ";
   //for (size_t i = 0; i < nonDominatedTeams.size(); i++)
   //cout << std::fixed << "[" << nonDominatedTeams[i]->id() << "," << nonDominatedTeams[i]->fit() << "," << nonDominatedTeams[i]->nov(0,_knnNov) << "," << nonDominatedTeams[i]->nov(1,_knnNov) << "," << nonDominatedTeams[i]->nov(2,_knnNov) << "," << nonDominatedTeams[i]->score() << "] ";

   // When only the relevant novelty measure is calculated
   cout << std::fixed << "sbb::paretoRanking t " << t << " _fitMode " << _fitMode << " _distMode " << _distMode << " numFront " << nonDominatedTeams.size() << " numDom " << dominatedTeams.size() << " [id,fit,nov,score] dominated ";
   for (size_t i = 0; i < dominatedTeams.size(); i++)
      cout << std::fixed << "[" << dominatedTeams[i]->id() << "," << dominatedTeams[i]->fit() << "," << dominatedTeams[i]->nov(_distMode,_knnNov) << "," << dominatedTeams[i]->score() << "] ";
   cout << " nonDominated ";
   for (size_t i = 0; i < nonDominatedTeams.size(); i++)
      cout << std::fixed << "[" << nonDominatedTeams[i]->id() << "," << nonDominatedTeams[i]->fit() << "," << nonDominatedTeams[i]->nov(_distMode,_knnNov) << "," << nonDominatedTeams[i]->score() << "] ";

   cout << endl;
   cout.precision(3);
}

/**********************************************************************************************************/
void TPG::policyFeatures(int hostId, set<long> &features, bool active){
   features.clear();
   set < team *, teamIdComp > :: iterator teiter, teiterend;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      if ((*teiter)->id() == hostId){ 
         (*teiter)->policyFeaturesGet(features, active);
         break;
      }
   }
}
/**********************************************************************************************************/
void TPG::printHostGraphs(){
   char outputFilename[80];
   ofstream ofs;
   sprintf(outputFilename, "%s","Nodes");
   if (fileExists(outputFilename)){
      char tmp[80];
      sprintf(tmp,"%s.%d.backup",outputFilename,(int)time(NULL));
      rename(outputFilename,tmp);
   }
   ofs.open(outputFilename, ios::out);
   if (!ofs)
      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");

   set < learner *> :: iterator leiter, leiterEnd;
   set < team *, teamIdComp > :: iterator teiter, teiterend;

   ofs << "id,type" << endl;
   for (int i = 0; i < numAtomicActions(); i++)
      ofs << "a" << -1-i << ",atomic" << endl;
   for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++)
      ofs << "s" << (*leiter)->id() << ",symbiont" << endl;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      ofs << "h" << (*teiter)->id() << ",host" << endl;
   //ofs << "Nodes end" << endl;
   ofs.close();

   sprintf(outputFilename, "%s","Links");
   if (fileExists(outputFilename)){
      char tmp[80];
      sprintf(tmp,"%s.%d.backup",outputFilename,(int)time(NULL));
      rename(outputFilename,tmp);
   }
   ofs.open(outputFilename, ios::out);
   if (!ofs)
      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");

   //cout << "Links start" << endl;
   ofs << "from,to" << endl;
   for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++){
      ofs << "s" << (*leiter)->id() << ",";
      if ((*leiter)->action() < 0) ofs << "a";
      else ofs << "h";
      ofs << (*leiter)->action() << endl;
   }
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      set < learner * > mem;
      (*teiter)->members(&mem);
      set < learner * > :: iterator leiter;
      for(leiter = mem.begin(); leiter != mem.end(); leiter++){
         ofs << "h" << (*teiter)->id() << ",s" << (*leiter)->id() << endl;
      }
   }
   ofs.close();
   //cout << "Links end" << endl;
}
/**********************************************************************************************************/
void TPG::printHostGraphsDFS(long t, bool singleBest, int phase){

   if (singleBest) getBestTeam(phase);

   char outputFilename[80];
   ofstream ofs;

   set < learner *> :: iterator leiterRoot;
   set < team *, teamIdComp > :: iterator teiter, teiterRoot;

   set <learner*> learners;
   set <team*> teams;

   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      if ((!singleBest && (*teiter)->root()) || (singleBest && (*teiter)->id() == currentChampion->id())){
         learners.clear();
         teams.clear();
         (*teiter)->getAllNodes(_teamMap, teams, learners);

         //Save Nodes
         sprintf(outputFilename, "hostGraphs/%s%d%s%d","Nodes-t",(int)t,"-host",(int)(*teiter)->id());
         ofs.open(outputFilename, ios::out);
         if (!ofs)
            die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
         ofs << "id,type" << endl;
         for(leiterRoot = learners.begin(); leiterRoot != learners.end(); leiterRoot++){
            ofs << "s" << (*leiterRoot)->id() << ",symbiont" << endl;
            if ((*leiterRoot)->action() < 0)
               ofs << "a" << (*leiterRoot)->id() <<",atomic" << (*leiterRoot)->action() << endl;
         }
         for(teiterRoot = teams.begin(); teiterRoot != teams.end(); teiterRoot++)
            ofs << "h" << (*teiterRoot)->id() << ",host" << endl;
         ofs.close();

         //Save Links
         sprintf(outputFilename, "hostGraphs/%s%d%s%d","Links-t",(int)t,"-host",(int)(*teiter)->id());
         ofs.open(outputFilename, ios::out);
         if (!ofs)
            die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
         ofs << "from,to" << endl;
         for(leiterRoot = learners.begin(); leiterRoot != learners.end(); leiterRoot++){
            ofs << "s" << (*leiterRoot)->id() << ",";
            if ((*leiterRoot)->action() < 0) ofs << "a" << (*leiterRoot)->id() << endl;
            else ofs << "h" << (*leiterRoot)->action() << endl;
         }
         for(teiterRoot = teams.begin(); teiterRoot != teams.end(); teiterRoot++){
            set < learner * > mem;
            (*teiterRoot)->members(&mem);
            set < learner * > :: iterator leiter;
            for(leiter = mem.begin(); leiter != mem.end(); leiter++)
               ofs << "h" << (*teiterRoot)->id() << ",s" << (*leiter)->id() << endl;
         }
         ofs.close();
      }
   }
}

/**********************************************************************************************************/
void TPG::printLearnerInfo(long t){
   set < learner *> :: iterator leiter, leiterEnd;
   oss.str("");
   oss << "printLearnerInfo start" << endl;
   for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++)
      oss << (*leiter)->checkpoint() << endl;
   oss << "printLearnerInfo end" << endl;
   cout << oss.str();
   oss.str("");
}

/**********************************************************************************************************/
void TPG::printTeamInfo(long t, int phase, bool singleBest, long teamId){
   cout << setprecision(3) << fixed;
   if (singleBest && teamId == -1) getBestTeam(phase);
   set < team *, teamIdComp > :: iterator teiter, teiterend;
   ostringstream tmposs;
   map < point *, double, pointLexicalLessThan > allOutcomes;
   map < point *, double > :: iterator myoiter;
   vector <int> behaviourSequence;
   set <team*> visitedTeams;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      if ((!singleBest && (*teiter)->root() && teamId == -1) || //all root teams
            (!singleBest && (*teiter)->id() == teamId) || //specific team 
            (singleBest && (*teiter)->id() == currentChampion->id())){ //singleBest root team
         //tmposs << "TPG::tminfo_toString t " << t << " ";
         //cout << (*teiter)->toString(tmposs.str(), _teamMap, visitedTeams);
         //tmposs.str("");
         //cout << "TPG::tminfo_teamosop t " << t << " " << *(*teiter) << endl;
         cout << "TPG::tminfo t " << t << " id " << (*teiter)->id() << " gtime " << (*teiter)->gtime();
         cout << " root " << ((*teiter)->root() ? 1 : 0);
         cout << " size " << (*teiter)->size();
         cout << " asize " << (*teiter)->asize();
         cout << " age " << t - (*teiter)->gtime();
         cout << " numEval " << (*teiter)->numEval();
         cout << " numOut " << (*teiter)->numOutcomes(phase);
         cout << " meanOutcomes";
         for (int fm = 0; fm < _numFitMode; fm++)//tmp
            cout << " " << (*teiter)->getMeanOutcome(phase,_MEAN_OUT_PROP,fm,fm,false);//specific fitMode, specific auxDouble, don't skip zero values
         for (int i = _numFitMode; i < _numPointAuxDouble; i++)
            cout << " " << (*teiter)->getMeanOutcome(phase,_MEAN_OUT_PROP,-1,i,true);//all fitMode, specific auxDouble, skip zero values
         cout << " fit " << (*teiter)->fit();
         cout << " nov_0 " << (*teiter)->nov(0,_knnNov);
         cout << " nov_1 " << (*teiter)->nov(1,_knnNov);
         cout << " score " << (*teiter)->score();
         visitedTeams.clear();
         cout << " ePolicyInstructions " << (*teiter)->policyInstructions(_teamMap, visitedTeams, true);
         visitedTeams.clear();
         cout << " policyInstructions " << (*teiter)->policyInstructions(_teamMap, visitedTeams, false);
         set <learner*> learners;
         visitedTeams.clear();
         (*teiter)->getAllNodes(_teamMap, visitedTeams, learners);
         cout << " symCount " << learners.size();
         cout << " hostCount " << visitedTeams.size();
         cout << " members ";
         set < learner * > mem;
         (*teiter)->members(&mem);
         set < learner * > :: iterator leiter;
         for(leiter = mem.begin(); leiter != mem.end(); leiter++)
            cout << " |" << (*leiter)->id() << ":" << (*leiter)->action() << "|";
         cout << " amembers ";
         mem.clear();
         (*teiter)->activeMembers(&mem);
         for(leiter = mem.begin(); leiter != mem.end(); leiter++)
            cout << " |" << (*leiter)->id() << ":" << (*leiter)->action() << ":" << (*leiter)->refs() << "|";

         cout << " childUp ";
         if ((*teiter)->score() > (*teiter)->parentScore())
            cout << "1";
         else
            cout << "0";
         vector <long> ancestors;
         (*teiter)->getAllAncestors(ancestors);
         cout << " ancestors " << vecToStr(ancestors);
         (*teiter)->outcomes(allOutcomes,phase);
         cout << " allOutcomes ";
         if (allOutcomes.size() > 0){
            for(myoiter = allOutcomes.begin(); myoiter != allOutcomes.end(); myoiter++){
               if (myoiter == allOutcomes.begin()) cout << "["; else cout << ",";
               cout << (myoiter->first)->fitMode() << ":" << (myoiter->first)->auxDouble((myoiter->first)->fitMode());
            }
            cout << "]";
         }
         cout << " behaviourProfile ";
         behaviourSequence.clear();
         (*teiter)->getBehaviourSequence(behaviourSequence,phase);
         cout << vecToStrNoSpace(behaviourSequence);
         visitedTeams.clear();
         set < long > features;
         (*teiter)->policyFeatures(_teamMap, visitedTeams, features, false);
         cout << " policyFeatures uniq " << features.size() << " feat";
         set <long >::iterator feiter;
         for(feiter = features.begin(); feiter!=features.end();feiter++)
            cout << " " << (*feiter); 
         visitedTeams.clear();
         features.clear();
         (*teiter)->policyFeatures(_teamMap, visitedTeams, features, true);
         cout << " policyFeaturesActive uniqActive " << features.size() << " featActive";
         for(feiter = features.begin(); feiter!=features.end();feiter++)
            cout << " " << (*feiter);
         cout << endl;
      }
   }
}

/********************************************************************************************/
void TPG::pruneLearners(long &hostToReplay)
{
   set < team *, teamIdComp > :: iterator teiter,teiter2;

   set < learner * > members, active;
   set < learner * > setdiff;
   set < learner * > :: iterator leiter;

   /* Remove inactive team members from the teams. */

   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
   {
#ifdef MYDEBUG
      cout << "TPG::pruneLearners size " << (*teiter)->size();
#endif

      members.clear();
      active.clear();
      setdiff.clear();

      (*teiter)->members(&members); /* Get all members. */
      (*teiter)->activeMembers(&active); /* Get active members. */

      ////If a team has one active learner, bypass the team in every policy graph (it will then be a root node if it isn't already)
      //if (active.size() == 1){
      //   //If the team is already a root node, we bypass by changing hostToReplay to the team 'below' (significant for replay only).
      //   //Degenrate root teams (single active program with atomic action) are ignored by this block.
      //   if ((*teiter)->root() && (*(active.begin()))->action() >= 0)
      //        hostToReplay = _teamMap[(*(active.begin()))->action()]->id();
      //   else{
      //      for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      //         if ((*leiter)->action() >= 0 && _teamMap[(*leiter)->action()]->id() == (*teiter)->id())
      //            (*leiter)->action((*(active.begin()))->action()); 
      //   }
      //}

      /* Inactive members are members \ active. */

      set_difference(members.begin(), members.end(),
            active.begin(), active.end(),
            insert_iterator < set < learner * > > (setdiff, setdiff.begin()));

#ifdef MYDEBUG
      cout << " members";
      for(leiter = members.begin(); leiter != members.end(); leiter++)
         cout << " " << (*leiter)->id();

      cout << " active";
      for(leiter = active.begin(); leiter != active.end(); leiter++)
         cout << " " << (*leiter)->id();

      cout << " inactive";
      for(leiter = setdiff.begin(); leiter != setdiff.end(); leiter++)
         cout << " " << (*leiter)->id();
#endif

      for(leiter = setdiff.begin(); leiter != setdiff.end() && (*teiter)->size() > 2; leiter++)
      {
         if (!((*leiter)->action() < 0 && (*teiter)->numAtomic() < 2)){//don't remove the only atomic
            (*leiter)->refDec();
            (*teiter)->removeLearner(*leiter);
         }
      }

#ifdef MYDEBUG
      cout << " finalsize " << (*teiter)->size() << endl;
#endif
   }
}

/***********************************************************************************************************
 * Read in populations from a checkpoint file. (This whole process needs a rewrite sometime.)
 **********************************************************************************************************/
void TPG::readCheckpoint(long t, int phase){

   ifstream ifs;
   char filename[80];
   sprintf(filename, "%s/%s.%d.%d.%d.%d.rslt","checkpoints","cp",t,_id,_seed,phase);
   ifs.open(filename, ios::in);
   if (!ifs.is_open() || ifs.fail()){
      cerr << "open failed for file: " << filename << " error:" << strerror(errno) << '\n';
      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
   }

   std::pair<std::set<team *, teamIdComp>::iterator,bool> ret;

   set < learner * > testTeamMembers;

   set < team *, teamIdComp > :: iterator teiter, teiterEnd;
   set < learner * > :: iterator leiter, leiterEnd;

   string oneline;
   char delim=':';
   char *token;
   long memberId = 0;
   long max_team_count = -1;
   long max_learner_count = -1;

   vector < string > outcomeFields;

   while (getline(ifs, oneline)){
      outcomeFields.clear();

      split(oneline,delim,outcomeFields);

      if (outcomeFields[0].compare("seed") == 0){
         _seed = atol(outcomeFields[1].c_str());
      }

      else if (outcomeFields[0].compare("learner") == 0){
         long id = 0;
         long gtime = 0;
         long action = 0;;
         long dim = 0;
         int nrefs = 0;
         vector < instruction * > bid;
         instruction *in;
         id = atol(outcomeFields[1].c_str());
         if (id > max_learner_count) max_learner_count = id;
         gtime = atol(outcomeFields[2].c_str());
         action = atol(outcomeFields[3].c_str());
         dim = atol(outcomeFields[4].c_str());
         nrefs = atol(outcomeFields[5].c_str());
         //if (phase < _TEST_PHASE) nrefs = atol(outcomeFields[5].c_str());
         //else nrefs = 0;

         for (size_t ii = 6; ii < outcomeFields.size(); ii++){
            token = (char*)outcomeFields[ii].c_str();
            in = new instruction();
            in->reset();
            for (int j = 22, k = 0; j >= 0; j--, k++){ //there are 23 bits in each instruction
               if (token[j] == '1'){
                  in->flip(k);
               }
            }
            bid.push_back(in);
         }
         learner *l;
         l = new learner(gtime, gtime, action, dim, id, nrefs, bid);
         _L.insert(l);
      }

      else if (outcomeFields[0].compare("team") == 0){
         long id = 0;
         long gtime = 0;
         long numEval = 0;
         bool auxFlag;
         set < learner * > members;
         team *m;
         id = atol(outcomeFields[1].c_str());
         if (id > max_team_count) max_team_count = id;
         gtime = atol(outcomeFields[2].c_str());
         numEval = atol(outcomeFields[3].c_str());
         auxFlag = atoi(outcomeFields[4].c_str())>0?true:false;

         m = new team(gtime, id);
         m->numEval(numEval);
         m->auxFlag(auxFlag);
         for (size_t ii = 5; ii < outcomeFields.size(); ii++){
            memberId = atol(outcomeFields[ii].c_str());
            for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++){
               if ((*leiter)->id() == memberId){
                  m->addLearner(*leiter);
               }
            }
         }
         ret = _M.insert(m);

         //if we get a duplicate team id, scale by 100 until unique
         while (ret.second==false){
            m->id(m->id()*100);
            ret = _M.insert(m);
         }
      }
   }

   ///* In _TEST_PHASE all learner nrefs will be set to zero, so we recalculate them here based on
   // * the single test team.
   // */
   //if (phase == _TEST_PHASE){
   //   teiter = _M.begin();
   //   (*teiter)->members(&testTeamMembers);
   //   for(leiter = testTeamMembers.begin(); leiter != testTeamMembers.end(); leiter++)
   //      (*leiter)->refInc();
   //}

   learner_count = max_learner_count+1;
   team_count = max_team_count+1;

   cout << "TPG::readCheckpoint t " << t  << " Msize " << _M.size() << " Lsize " << _L.size() << " numRoot " << setRoots() << endl;

   //put this in a "sanity check" function
   int sumTeamSizes=0;
   int nrefs = 0;
   int sumNumOutcomes = 0;

   cout << "TPG::readCheckpoint ";
   cout << _L.size() << " learners, " << _M.size() << " teams ";
   for(teiter = _M.begin(), sumTeamSizes = sumNumOutcomes = 0; teiter != _M.end(); teiter++)
   {
      sumTeamSizes += (*teiter)->size();
      sumNumOutcomes += (*teiter)->numOutcomes(_TRAIN_PHASE);
   }

   for(leiter = _L.begin(), nrefs = 0; leiter != _L.end(); leiter++)
      nrefs += (*leiter)->refs();

   cout << ", sumTeamSizes " << sumTeamSizes << ", nrefs " << nrefs << ", sumNumOutcomes " << sumNumOutcomes;
   cout << endl;

   if(sumTeamSizes != nrefs)
      die(__FILE__, __FUNCTION__, __LINE__, "something messed up during readCheckpoint");
   ifs.close();
}

///***********************************************************************************************************
// * ALE HISTORIC TMP Read in populations from a checkpoint file. (This whole process needs a rewrite sometime.)
// **********************************************************************************************************/
//void TPG::readCheckpoint(long t, int phase){
//
//   ifstream ifs;
//   char filename[80];
//   sprintf(filename, "%s/%s.%d.%d.%d.%d.rslt","checkpoints","cp",t,_id,_seed,phase);
//   ifs.open(filename, ios::in);
//   if (!ifs.is_open() || ifs.fail()){
//      cerr << "open failed for file: " << filename << " error:" << strerror(errno) << '\n';
//      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
//   }
//
//   std::pair<std::set<team *, teamIdComp>::iterator,bool> ret;
//
//   set < learner * > testTeamMembers;
//
//   set < team *, teamIdComp > :: iterator teiter, teiterEnd;
//   set < learner * > :: iterator leiter, leiterEnd;
//
//   string oneline;
//   char delim=':';
//   char *token;
//   long memberId = 0;
//   long max_team_count = -1;
//   long max_learner_count = -1;
//
//   vector < string > outcomeFields;
//
//   while (getline(ifs, oneline)){
//      outcomeFields.clear();
//
//      split(oneline,delim,outcomeFields);
//
//      if (outcomeFields[0].compare("seed") == 0){
//         _seed = atol(outcomeFields[1].c_str());
//      }
//
//      else if (outcomeFields[0].compare("learner") == 0){
//         long id = 0;
//         long gtime = 0;
//         long action = 0;;
//         long dim = 0;
//         int nrefs = 0;
//         vector < instruction * > bid;
//         instruction *in;
//         id = atol(outcomeFields[1].c_str());
//         if (id > max_learner_count) max_learner_count = id;
//         gtime = atol(outcomeFields[2].c_str());
//         action = atol(outcomeFields[3].c_str());
//         dim = atol(outcomeFields[4].c_str());
//         nrefs = atol(outcomeFields[5].c_str());
//         //if (phase < _TEST_PHASE) nrefs = atol(outcomeFields[5].c_str());
//         //else nrefs = 0;
//
//         for (size_t ii = 6; ii < outcomeFields.size(); ii++){
//            token = (char*)outcomeFields[ii].c_str();
//            in = new instruction();
//            in->reset();
//            for (int j = 22, k = 0; j >= 0; j--, k++){ //there are 23 bits in each instruction
//               if (token[j] == '1'){
//                  in->flip(k);
//               }
//            }
//            bid.push_back(in);
//         }
//         learner *l;
//         l = new learner(gtime, gtime, action, dim, id, nrefs, bid);
//         _L.insert(l);
//      }
//
//      else if (outcomeFields[0].compare("team") == 0){
//         long id = 0;
//         long gtime = 0;
//         set < learner * > members;
//         team *m;
//         id = atol(outcomeFields[1].c_str());
//         if (id > max_team_count) max_team_count = id;
//         gtime = atol(outcomeFields[2].c_str());
//
//         m = new team(gtime, id);
//         for (size_t ii = 3; ii < outcomeFields.size(); ii++){
//            memberId = atol(outcomeFields[ii].c_str());
//            for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++){
//               if ((*leiter)->id() == memberId){
//                  m->addLearner(*leiter);
//               }
//            }
//         }
//         ret = _M.insert(m);
//
//         //if we get a duplicate team id, scale by 100 until unique
//         while (ret.second==false){
//            m->id(m->id()*100);
//            ret = _M.insert(m);
//         }
//      }
//   }
//
//   ///* In _TEST_PHASE all learner nrefs will be set to zero, so we recalculate them here based on
//   // * the single test team.
//   // */
//   //if (phase == _TEST_PHASE){
//   //   teiter = _M.begin();
//   //   (*teiter)->members(&testTeamMembers);
//   //   for(leiter = testTeamMembers.begin(); leiter != testTeamMembers.end(); leiter++)
//   //      (*leiter)->refInc();
//   //}
//
//   learner_count = max_learner_count+1;
//   team_count = max_team_count+1;
//
//   cout << "TPG::readCheckpoint t " << t  << " Msize " << _M.size() << " Lsize " << _L.size() << " numRoot " << setRoots() << endl;
//
//   //put this in a "sanity check" function
//   int sumTeamSizes=0;
//   int nrefs = 0;
//   int sumNumOutcomes = 0;
//
//   cout << "TPG::readCheckpoint ";
//   cout << _L.size() << " learners, " << _M.size() << " teams ";
//   for(teiter = _M.begin(), sumTeamSizes = sumNumOutcomes = 0; teiter != _M.end(); teiter++)
//   {
//      sumTeamSizes += (*teiter)->size();
//      sumNumOutcomes += (*teiter)->numOutcomes(_TRAIN_PHASE);
//   }
//
//   for(leiter = _L.begin(), nrefs = 0; leiter != _L.end(); leiter++)
//      nrefs += (*leiter)->refs();
//
//   cout << ", sumTeamSizes " << sumTeamSizes << ", nrefs " << nrefs << ", sumNumOutcomes " << sumNumOutcomes;
//   cout << endl;
//
//   if(sumTeamSizes != nrefs)
//      die(__FILE__, __FUNCTION__, __LINE__, "something messed up during readCheckpoint");
//   ifs.close();
//}

/*********************************************************************************************************/
void TPG::recalculateLearnerRefs(){

   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      (*leiter)->setNrefs(0);

   set < team * > :: iterator teiter, teiterEnd;
   set < learner * > mem;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      (*teiter)->members(&mem);
      set < learner * > :: iterator leiter;
      for(leiter = mem.begin(); leiter != mem.end(); leiter++)
         (*leiter)->refInc(); 
      mem.clear();
   }
}

/*********************************************************************************************************/
void TPG::scaleTeamAndLearnerIds(long i){

   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      (*leiter)->id((*leiter)->id()+i);
   set < team * > :: iterator teiter, teiterEnd;
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      (*teiter)->id((*teiter)->id()+i);
}

/********************************************************************************************/
void TPG::selTeams(long t)
{
   vector < team * > teams;
   set < team *, teamIdComp > :: iterator teiter, teiterend;

   // _diversityMoe -1 implies team scores are set in the main method

   if(_diversityMode == 0){
      //this diversityMode runs in here for reporting
      //paretoRanking(t);
      //reset scores to fit only
      diversityMode0(t);
   }
   else if (_diversityMode == 7)
      paretoRanking(t);

   ostringstream tmposs;
   map < point *, double, pointLexicalLessThan > allOutcomes;
   map < point *, double > :: iterator myoiter;

   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      if((*teiter)->root() && !isElite(*teiter))
         teams.push_back(*teiter);

   // Select the top teams.
   for(size_t i = 0; i < teams.size(); i++)
      teams[i]->key(teams[i]->score());
   // Sort the teams by their keys.
   partial_sort(teams.begin(), teams.begin() + floor(teams.size()*_Mgap), teams.end(), lessThan < team > ());
#ifdef MYDEBUG
   for(int i = 0; i < teams.size(); i++)
      cout << "_id " << _id << "selTeams " << t << " sorted " << teams[i]->id() << " key " << teams[i]->key() << endl;
#endif
   // At this point, the order of teams no longer matches the order of the other vectors.
   int numOldDeleted = 0;
   int numDeleted = 0;
   oss << "sbb::selTeams t " << t << " deletedAge";
   for(size_t i = 0; i < floor(teams.size()*_Mgap); i++){
      oss << " " << t - teams[i]->gtime();
      if(teams[i]->gtime() != t){
         numOldDeleted++;
         _mdom++; // The team is old.
      }
#ifdef MYDEBUG
      cout << "sbb::selTeams deleting " << teams[i]->id() << endl;
#endif
      _M.erase(teams[i]);
      delete teams[i]; // Learner refs deleted automatically.
      numDeleted++;
   }
   oss << " numDeleted " << numDeleted << " numOldDeleted " << numOldDeleted << " keptAge";
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      oss << " " << t - (*teiter)->gtime();
   oss << " keptIds";
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      oss << " " << (*teiter)->id();
   oss << " numRoot " << setRoots() << endl;
   cout << oss.str();
   oss.str("");
}

/********************************************************************************************/
void TPG::setParams(point * (*initPoint)(long,long,int)){
   oss.str("");
   oss << "sbb." << _seed << ".arg";

   map < string, string > args;

   readMap(oss.str(), args);

   oss.str("");
   oss << "====== TPG parameters:" << endl << endl;

   map < string, string > :: iterator maiter;

   /* Get arguments. */

   if((maiter = args.find("paretoEpsilonTeam")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg paretoEpsilonTeam");
   _paretoEpsilonTeam = stringTodouble(maiter->second);

   if((maiter = args.find("Msize")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg Msize");
   _Msize = stringToInt(maiter->second);

   if((maiter = args.find("Mgap")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg Mgap");
   _Mgap = stringTodouble(maiter->second);

   if((maiter = args.find("numProfilePoints")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg numProfilePoints");
   _numProfilePoints = stringToInt(maiter->second);

   if((maiter = args.find("pAddProfilePoint")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pAddProfilePoint");
   _pAddProfilePoint = stringTodouble(maiter->second);

   if((maiter = args.find("pAtomic")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pAtomic");
   _pAtomic = stringTodouble(maiter->second);

   if((maiter = args.find("pmd")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pmd");
   _pmd = stringTodouble(maiter->second);

   if((maiter = args.find("pma")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pma");
   _pma = stringTodouble(maiter->second);

   if((maiter = args.find("pmm")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pmm");
   _pmm = stringTodouble(maiter->second);

   if((maiter = args.find("pmn")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pmn");
   _pmn = stringTodouble(maiter->second);

   if((maiter = args.find("omega")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg omega");
   _omega = stringToInt(maiter->second);

   if((maiter = args.find("maxProgSize")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg maxProgSize");
   _maxProgSize = stringToInt(maiter->second);

   if((maiter = args.find("pBidMutate")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pBidMutate");
   _pBidMutate = stringTodouble(maiter->second);

   if((maiter = args.find("pBidSwap")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pBidSwap");
   _pBidSwap = stringTodouble(maiter->second);

   if((maiter = args.find("pBidDelete")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pBidDelete");
   _pBidDelete = stringTodouble(maiter->second);

   if((maiter = args.find("pBidAdd")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pBidAdd");
   _pBidAdd = stringTodouble(maiter->second);

   if((maiter = args.find("episodesPerGeneration")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg episodesPerGeneration");
   _episodesPerGeneration = stringToLong(maiter->second);

   if((maiter = args.find("numStoredOutcomesPerHost_TRAIN")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg numStoredOutcomesPerHost");
   _numStoredOutcomesPerHost[_TRAIN_PHASE] = stringToLong(maiter->second);

   if((maiter = args.find("numStoredOutcomesPerHost_VALIDATION")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg numStoredOutcomesPerHost_VALIDATION");
   _numStoredOutcomesPerHost[_VALIDATION_PHASE] = stringToLong(maiter->second);

   if((maiter = args.find("numStoredOutcomesPerHost_TEST")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg numStoredOutcomesPerHost_TEST");
   _numStoredOutcomesPerHost[_TEST_PHASE] = stringToLong(maiter->second);

   if((maiter = args.find("normalizeFitness")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg normalizeFitness");
   _normalizeFitness = stringToInt(maiter->second) > 0 ? true : false;

   if((maiter = args.find("diversityMode")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg diversityMode");
   _diversityMode = stringToInt(maiter->second);

   if((maiter = args.find("stateDiscretizationSteps")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg stateDiscretizationSteps");
   _stateDiscretizationSteps = stringToInt(maiter->second);

   if((maiter = args.find("knnNovelty")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg knnNovelty");
   _knnNov = stringToInt(maiter->second);   

   if((maiter = args.find("pNoveltyGeno")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pNoveltyGeno");
   _pNoveltyGeno = stringTodouble(maiter->second);

   if((maiter = args.find("pNoveltyPheno")) == args.end()) die(__FILE__, __FUNCTION__, __LINE__, "cannot find arg pNoveltyPheno");
   _pNoveltyPheno = stringTodouble(maiter->second);

   oss << "seed " << _seed << endl;

   oss << "omega " << _omega << endl;
   oss << "pAtomic " << _pAtomic << endl;
   oss << "pmd " << _pmd << endl;
   oss << "pma " << _pma << endl;
   oss << "pmm " << _pmm << endl;
   oss << "pmn " << _pmn << endl;

   oss << "maxProgSize " << _maxProgSize << endl;
   oss << "pBidMutate " << _pBidMutate << endl;
   oss << "pBidSwap " << _pBidSwap << endl;
   oss << "pBidDelete " << _pBidDelete << endl;
   oss << "pBidAdd " << _pBidAdd << endl;

   oss << "Msize " << _Msize << endl;
   oss << "Mgap " << _Mgap << endl;
   oss << "episodesPerGeneration " << _episodesPerGeneration << endl;
   oss << "numStoredOutcomesPerHost_TRAIN_PHASE " << _numStoredOutcomesPerHost[_TRAIN_PHASE] << endl;
   oss << "numStoredOutcomesPerHost_VALIDATION_PHASE " << _numStoredOutcomesPerHost[_VALIDATION_PHASE] << endl;
   oss << "numStoredOutcomesPerHost_TEST_PHASE " << _numStoredOutcomesPerHost[_TEST_PHASE] << endl;
   oss << "numProfilePoints " << _numProfilePoints << endl;
   oss << "pAddProfilePoint " << _pAddProfilePoint << endl;
   oss << "paretoEpsilonTeam " << _paretoEpsilonTeam << endl;

   oss << "normalizeFitness " << _normalizeFitness << endl;

   oss << "diversityMode " << _diversityMode << endl;
   oss << "stateDiscretizationSteps " << _stateDiscretizationSteps << endl;
   oss << "knnNovelty " << _knnNov << endl;

   oss << "pNoveltyGeno " << _pNoveltyGeno << endl;
   oss << "pNoveltyPheno " << _pNoveltyPheno << endl;

   cout << oss.str() << endl;
   oss.str("");

   if(_Msize < 2) die(__FILE__, __FUNCTION__, __LINE__, "bad arg Msize < 2");

   if(_pmd < 0 || _pmd > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pmd < 0 || pmd > 1");

   if(_pma < 0 || _pma > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pma < 0 || pma > 1");

   if(_pmm < 0 || _pmm > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pmm < 0 || pmm > 1");

   if(_pmn < 0 || _pmn > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pmn < 0 || pmn > 1");

   if(_omega < 2) die(__FILE__, __FUNCTION__, __LINE__, "bad arg omega < 2");

   if(_Mgap < 0 || _Mgap > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg Mgap < 0 || Mgap > 1");

   if(_maxProgSize < 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg _maxProgSize < 1");

   if(_pBidDelete < 0 || _pBidDelete > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pBidDelete < 0 || pBidDelete > 1");

   if(_pBidAdd < 0 || _pBidAdd > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pBidAdd < 0 || pBidAdd > 1");

   if(_pBidSwap < 0 || _pBidSwap > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pBidSwap < 0 || pBidSwap > 1");

   if(_pBidMutate < 0 || _pBidMutate > 1) die(__FILE__, __FUNCTION__, __LINE__, "bad arg pBidMutate < 0 || pBidMutate > 1");

   if(_paretoEpsilonTeam < 0) die(__FILE__, __FUNCTION__, __LINE__, "bad arg _paretoEpsilonTeam < 0");

   /* Bidding behaviour. */
   for(int i = 0; i < _numProfilePoints; i++)
      _profilePointsFIFO.push_back(initPoint(-3, point_count++, _VALIDATION_PHASE));
}

/*********************************************************************************************************/
int TPG::setRoots(){

   _teamMap.clear();
   set < team * , teamIdComp > :: iterator teiter;

   for(teiter = _M.begin(); teiter != _M.end(); teiter++){
      (*teiter)->root(true);
      _teamMap[(*teiter)->id()]=*teiter;
   }
   int nroots = _M.size();
   set < learner * > :: iterator leiter;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      if ((*leiter)->action() >= 0 && _teamMap[(*leiter)->action()]->root()){
         _teamMap[(*leiter)->action()]->root(false);
         nroots--;
      }

   // Collect and store all features indexed by root policies for fast lookup
   for(teiter = _M.begin(); teiter != _M.end(); teiter++)
      if ((*teiter)->root()){
         set < team * > visitedTeams;
         set < long > features;

         (*teiter)->policyFeatures(_teamMap, visitedTeams, features, true);//active
         (*teiter)->policyFeaturesSet(features, true);

         features.clear();
         visitedTeams.clear();

         (*teiter)->policyFeatures(_teamMap, visitedTeams, features, false);//all
         (*teiter)->policyFeaturesSet(features, false);
      }
   return nroots;
}

/*********************************************************************************************************/
void TPG::splitLevelPrep(){
   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(); leiter != _L.end(); leiter++)
      (*leiter)->action((*leiter)->action()+1);
}

/**********************************************************************************************************/
void TPG::updateLearnerProfiles(){
   vector < double > state;
   //double *REG = (double *) alloca(REGISTERS * sizeof(double));
   set < learner * > :: iterator leiter, leiterEnd;
   for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++){
      vector < double > profile;
      deque<point *>::iterator it = _profilePointsFIFO.begin();
      while (it != _profilePointsFIFO.end()){
         (*it++)->behaviouralState(state);
         profile.push_back((*leiter)->bid(&state[0]));
      }
      (*leiter)->setProfile(profile);
   }
}

///**********************************************************************************************************/
//void TPG::testSymbiontUtilityDistance(){
//	ostringstream o;
//	vector< team* > teams;
//	set < team * , teamIdComp > :: iterator teiter;
//	for(teiter = _M.begin(); teiter != _M.end(); teiter++)
//		teams.push_back(*teiter);
//	for (size_t i = 0; i < teams.size(); i++)
//		for (size_t j = 0; j < teams.size(); j++){
//			double dist = teams[i]->symbiontUtilityDistance(teams[j]);
//		}
//}

/***********************************************************************************************************/
void TPG::writeCheckpoint(long t, int phase, bool compress) {

   ofstream ofs;
   char filename[80]; sprintf(filename, "%s/%s.%d.%d.%d.%d.rslt","checkpoints","cp",t,_id,_seed,phase);

   if (fileExists(filename)){
      if( remove(filename) != 0 )
         cerr << "error deleting " << filename << endl;
      //char tmp[80];
      //sprintf(tmp,"%s.%d.backup",filename,(int)time(NULL));
      //rename(filename,tmp);
   }
   ofs.open(filename, ios::out);
   if (!ofs.is_open() || ofs.fail()){
      cerr << "open failed for file: " << filename << " error:" << strerror(errno) << '\n';
      die(__FILE__, __FUNCTION__, __LINE__, "Can't open file.");
   }

   set < team *, teamIdComp > :: iterator teiter, itBegin, teiterEnd;
   set < learner * > :: iterator leiter, leiterEnd;

   ofs << "seed:" << _seed << endl;
   ofs << "learnerPop:" << endl; // for compatibility with sbbModule
   for(leiter = _L.begin(), leiterEnd = _L.end(); leiter != leiterEnd; leiter++)
      ofs << (*leiter)->checkpoint();
   ofs << "endLearnerPop" << endl; // for compatibility with sbbModule
   ofs << "teamPop:" << endl; // for compatibility with sbbModule
   for(teiter = _M.begin(), teiterEnd = _M.end(); teiter != teiterEnd; teiter++){
      ofs << (*teiter)->checkpoint(phase);
   }
   ofs << "endTeamPop" << endl; // for compatibility with sbbModule
   ofs.close();

   if (compress){
      //compress this checkpoint
      oss.str("");
      oss << "tar czpf " << filename << ".tgz -C  checkpoints cp." << t << "." << _id << "." << _seed << "." << phase << ".rslt";
      system ( oss.str().c_str());
      if( remove(filename) != 0 )
         cerr << "error deleting " << filename << endl;
      oss.str("");
   }

   ////delete previous compressed checkpoint
   //if (!saveOldCheckpoints){
   //   sprintf(filename, "%s/%s.%d.%d.%d.%d.rslt.tgz","checkpoints","cp",t-1,_id,_seed,phase);
   //   if (ifstream(filename)){
   //      if( remove(filename) != 0 )
   //         cerr << "error deleting " << filename << endl;
   //   }
   //}
}

