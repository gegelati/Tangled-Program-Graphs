#ifndef TPG_H
#define TPG_H
#include "point.h"
#include "team.h"
#include "learner.h"

class TPG
{
   public:
      TPG(int seed = 0);
      ~TPG();

      /********************************************************************************************
       * Methods that set or query parameters.
       ********************************************************************************************/

      inline int dimBehavioural(){ return _dimB; }
      inline void dimBehavioural(int dim){ _dimB = dim; }
      inline int diversityMode(){ return _diversityMode; }
      inline void diversityMode(int i){ _diversityMode = i; }
      inline double eliteTeamMeanOutcome(int fitMode, int whichEliteTeam){ return _eliteTeams[whichEliteTeam]->getMeanOutcome(_TRAIN_PHASE,_MEAN_OUT_PROP,fitMode,fitMode,false); }
      inline int episodesPerGeneration() { return _episodesPerGeneration; }
      inline void episodesPerGeneration(int e) { _episodesPerGeneration = e; }
      inline int hostDistanceMode(){ return _distMode; }
      inline void hostDistanceMode(int i){ _distMode = i; }
      inline int hostFitnessMode(){ return _fitMode; }
      inline void hostFitnessMode(int i){ _fitMode = i; }
      inline long id() { return _id; }
      inline void id(long id) { _id = id; }
      inline bool isElite(team * tm){
         for (size_t e = 0; e < _eliteTeams.size(); e++)
            if (tm->id() == _eliteTeams[e]->id()) return true;
         return false;
      }
      inline double numPointAuxDouble() { return _numPointAuxDouble; }
      inline void  numPointAuxDouble(double d) { _numPointAuxDouble = d;}
      inline bool normalizeFitness() { return _normalizeFitness; }
      inline int numFitMode(){ return _numFitMode; }
      inline void numFitMode(int fm){ _numFitMode = fm; }
      inline int numProfilePoints() { return _profilePointsFIFO.size(); }
      inline int numStoredOutcomesPerHost(int phase) { return _numStoredOutcomesPerHost[phase]; }
      inline void numStoredOutcomesPerHost(int phase, long nso) { _numStoredOutcomesPerHost[phase] = nso; }
      inline double pAddProfilePoint() { return _pAddProfilePoint; }
      inline int phase() { return _phase; }
      inline void phase(int p) { _phase = p; }
      inline void rangeBehavioural(double l, double h, bool d){_sensorRangeLow = l; _sensorRangeHigh = h; _sensorsDiscrete = d;}
      inline double rangeBehavioural(int minmax) { return (minmax == 0 ? _sensorRangeLow : _sensorRangeHigh); }
      inline long seed() const { return _seed; }
      inline void seed(long seed) {
         _seed = seed;
         srand48(_seed);
         //readCheckpoint would find the highest team_count and learner_count and start there
         team_count = 1000*seed;
         learner_count = 1000*seed;
         point_count = 1000*seed;
      }
      inline void setupTeamCount(int s) { team_count = 1000*s; }
      inline int stateDiscretizationSteps() { return _stateDiscretizationSteps; }
      inline int tCull(){ return _tCull; }
      inline bool verbose(){ return _verbose; }
      inline void verbose(bool v){ _verbose = v; }

      /********************************************************************************************
       * Methods to implement the TPG algorithm.
       ********************************************************************************************/

      void addProfilePoint(vector < double > &bState, vector <double> &rewards, int phase, long gtime){
         _profilePointsFIFO.push_back(new point(gtime,phase,point_count++, bState, rewards));
         while (_profilePointsFIFO.size() > _numProfilePoints){
            point *p = _profilePointsFIFO.front();
            _profilePointsFIFO.pop_front();
            delete p;
         }
      }
      void adjustIds(int);
      void checkRefCounts(const char *);
      void cleanup (long,bool,long &);
      void cleanup (long t,bool prune){ long dummy; cleanup(t,prune,dummy); }
      void clearNumEvals(){
         set < team * , teamIdComp > :: iterator teiter;
         for(teiter = _M.begin(); teiter != _M.end(); teiter++)
            (*teiter)->numEval(0);
      }
      void clearRegisters(team* tm){
         set <team*> visitedTeams;
         tm->clearRegisters(_teamMap,visitedTeams);
      }
      inline int currentChampId(){ return currentChampion->id();}
      inline int currentChampNumOutcomes(int phase){ return currentChampion->numOutcomes(phase,true);}
      void debugRefs(long);
      void diversityMode0(long);
      void finalize();
      void genTeams(long);
      void genTeams(long, team *, team **, vector < learner * > &);
      void genUniqueLearner(learner *, set < learner * >);
      inline int getAction(team* tm, vector < double > &state, bool updateActive, set <learner *, LearnerBidLexicalCompare> &learnersRanked, set <learner *, LearnerBidLexicalCompare> &winningLearners,long &decisionInstructions, vector < set <long> > &decisionFeatures, bool verbose)
      {
         set <team*> visitedTeams;
         return tm->getAction(state, _teamMap, updateActive, learnersRanked, winningLearners, decisionInstructions, decisionFeatures, visitedTeams, verbose);
      }
      inline int getAction(team* tm, vector < double > &state, bool updateActive, set <learner *, LearnerBidLexicalCompare> &learnersRanked, set <learner *, LearnerBidLexicalCompare> &winningLearners, long &decisionInstructions, vector < set <long> > &decisionFeatures, set < team * > &visitedTeams, bool verbose)
      {
         learnersRanked.clear();
         winningLearners.clear();
         decisionInstructions = 0;
         decisionFeatures.clear();
         visitedTeams.clear();
         return tm->getAction(state, _teamMap, updateActive, learnersRanked, winningLearners, decisionInstructions, decisionFeatures, visitedTeams, verbose);
      }
      inline void getAllNodes(team* tm, set < team * > &teams, set < learner * > &learners){
         teams.clear();
         learners.clear(); 
         tm->getAllNodes(_teamMap, teams, learners);
      }
      team * getBestTeam( int );
      inline void getLearnerPop(set <learner *> &lp){ lp = _L; }
      inline void getTeams(vector <team*> &t, bool roots) {
         t.clear();
         set < team * , teamIdComp > :: iterator teiter;
         for(teiter = _M.begin(); teiter != _M.end(); teiter++)
            if(!roots || roots && (*teiter)->root())
               t.push_back(*teiter);
      }
      inline void getFlaggedTeams(vector <team*> &t) {
         set < team * , teamIdComp > :: iterator teiter;
         for(teiter = _M.begin(); teiter != _M.end(); teiter++)
            if((*teiter)->auxFlag())
               t.push_back(*teiter);
      }
      void initTeams();
      inline int lSize(){ return _L.size(); }
      inline int mSize(){ return _M.size(); }
      void paretoRanking(long, int phase = 0);
      inline int numAtomicActions()
      {  return _numActions; }
      inline void numAtomicActions(int a) { _numActions = a; }
      void policyFeatures(int,set<long> &, bool);
      inline long pointCount(){ return point_count; }
      inline void pointCount(long pc){ point_count = pc; }
      void printEvaluationVector(long t);
      void printHostGraphs();
      void printHostGraphsDFS(long, bool, int);
      void printLearnerInfo(long);
      void printTeamInfo(long, int, bool, long team=-1);
      /* Remove inactive learners. */
      void pruneLearners(long &);
      void readCheckpoint (long,int);
      void recalculateLearnerRefs();
      inline void replaceLearnerPop(set <learner *> lp){ _L = lp; }
      inline void resetOutcomes(int phase){
         set < team * , teamIdComp > :: iterator teiter;
         for(teiter = _M.begin(); teiter != _M.end(); teiter++)
            (*teiter)->resetOutcomes(phase);
      }
      void scaleTeamAndLearnerIds(long);
      void selTeams(long);
      void setEliteTeams(long, long, const vector <int> &, vector <double> &, vector <double> &, bool);
      /* Team tm will store a clone of the point with the same id. */
      void setOutcome(team* tm, vector < double > &bState, vector<double> &rewards, int phase, long gtime){
         tm->setOutcome(new point(gtime, phase, point_count++, bState, rewards, _fitMode) , rewards[_fitMode], _numStoredOutcomesPerHost[phase]);
         tm->numEval(tm->numEval()+1);
      }
      void setOutcome(team* tm, point* pt, vector < double > &bState, vector<double> &rewards, int phase, long gtime){
         vector<double> pState;
         pt->pointState(pState);	
         point * teamPoint = new point(gtime, phase, pt->id(), pState, bState, rewards, _fitMode);
         tm->setOutcome(teamPoint , rewards[_fitMode], _numStoredOutcomesPerHost[phase]);
         tm->numEval(tm->numEval()+1);
      }
      void setParams(point * (*initPoint)(long,long,int));
      int setRoots();
      inline long simTime() { return _simTime; }
      void splitLevelPrep();
      void testSymbiontUtilityDistance();
      void updateLearnerProfiles();
      void writeCheckpoint(long, int, bool);

      /********************************************************************************************
       *  TPG member variables and data structures.
       ********************************************************************************************/

   protected:
      /* Populations */
      set < team *, teamIdComp > _M; /* Teams */
      map < long, team*> _teamMap;
      set < learner * > _L; /* Learners. */

      //vector < double > bid1; /* First highest bid, reporting only */
      //vector < double > bid2; /* Second highest bid, reporting only */
      team * currentChampion;
      double _compatibilityThresholdGeno;
      double _compatibilityThresholdIncrement;
      double _compatibilityThresholdPheno;
      int _dimB;
      int _diversityMode;
      long _episodesPerGeneration; /* How many games should each team get to play before calculating fit and performing selection. */
      int _distMode;
      vector < team * > _eliteTeams;
      int _fitMode;
      //	void getDistinctions(set < point * > &, map < point *, vector < short > * > &);
      int _id;
      int _keepawayStateCode;
      int _knnNov;
      long learner_count;
      int _numPointAuxDouble;
      size_t _maxProgSize;
      long _mdom; /* Number of individuals pushed out of the population by younger individuals. */
      double _Mgap; /* Team generation gap as percentage. */
      int _minOutcomesForNoveltyArchive;
      int    m_lastAction;
      size_t _Msize; /* Team population size. */
      bool _normalizeFitness;
      int _numActions; /* Number of actions, actions 0, 1, ..., _numActions - 1 are assumed. */
      int _numExpectedAdditionsToArchive;
      int _numFitMode;
      int _numProfilePoints;
      long _numStoredOutcomesPerHost[_NUM_PHASE];
      size_t _omega; /* Initial team size. */
      ostringstream oss; /* logging, reporting */
      double _paretoEpsilonTeam;
      double _pAddProfilePoint;
      double _pAtomic;
      double _pBidAdd;
      double _pBidDelete;
      double _pBidMutate;
      double _pBidSwap;
      long _pdom; /* Number of individuals pushed out of the population by younger individuals. */
      int _phase; // 0:train 1:validation 2:test
      long point_count;
      double _pma; /* Probability of learner addition. */
      double _pmd; /* Probability of learner deletion.*/
      double _pmm; /* Probability of learner mutation. */
      double _pmn; /* Probability of learner action mutation. */
      double _pNoveltyGeno; //percentage of nov relative to fit in score
      double _pNoveltyPheno; //percentage of nov relative to fit in score
      deque < point * > _profilePointsFIFO; /* Points used to build the learner profiles. */
      int _seed;
      bool _sensorsDiscrete;
      double _sensorRangeHigh;
      double _sensorRangeLow;
      long _simTime; //simulator time
      int _stateDiscretizationSteps;
      int _tCull; /* what generation to test for restart. */
      long team_count;
      bool _verbose;
};

#endif
