#ifndef team_h
#define team_h

#include <map>
#include "learner.h"
#include "misc.h"
#include "point.h"
#include <set>

#define NUM_TEAM_DISTANCE_MEASURES 3
#define MAX_NCD_PROFILE_SIZE 20000 //arbitrarily large, greater than max ALE frames/episode of 18000

using namespace std;

class team
{
   set < learner * > _active; /* Active member learners, a subset of _members, activated in getAction(). */
   vector <long> _ancestors; /* lineage */
   bool _archived;
   bool _auxFlag;
   static long _count; /* Next id to use. */
   multiset <double> _distances_0;
   multiset <double> _distances_1;
   multiset <double> _distances_2;
   int _domBy; /* Number of other teams dominating this team. */
   int _domOf; /* Number of other teams that this team dominates. */
   double _fit;	
   long _gtime; /* Time step at which generated. */
   long _id; /* Unique id of team. */
   double _key; /* For sorting. */
   int _lastCompareFactor;
   /* Sometimes numOutcomes won't synch with the number of evaluations for this team (e.g. Robocup),
      so maintain a distinct eval counter */
   long _numEval;
   map < point *, double > _margins; /* Maps point->margin. */
   set < learner * > _members; /* The member learners stored in sorted order. */
   int _nodes; /* Number of nodes at all levels assuming this team is the root. */
   map < point *, double, pointLexicalLessThan > _outcomes; /* Maps point->outcome. */
   double _parentFitness;
   double _parentFit;
   set < long > _policyFeatures;
   set < long > _policyFeaturesActive;
   bool _root;
   double _score;

   public:
   inline void activeMembers(set < learner * > *m) { m->insert(_active.begin(), _active.end()); }
   inline void addAncestor(long a) { _ancestors.push_back(a); }
   inline void addDistance(int type, double d){ if (type == 0) _distances_0.insert(d); else if (type == 1) _distances_1.insert(d); else if (type == 2) _distances_2.insert(d);}
   bool addLearner(learner *);
   inline double archived(){ return _archived; }
   inline void archived(bool a) { _archived = a; }
   inline int asize() const { return _active.size(); } /* The number of active learners in this team. */
   inline bool auxFlag() { return _auxFlag; }
   inline void auxFlag(bool af) { _auxFlag = af; }
   string checkpoint(int);
   inline void clearDistances(){ _distances_0.clear(); _distances_1.clear(); _distances_2.clear();}
   void clearRegisters(map <long, team*> &teamMap, set <team*> &);
   void deleteMargin(point *); /* Delete margin. */
   void deleteOutcome(point *); /* Delete outcome. */
   inline int domBy() {return _domBy;}
   inline void domBy(int d){_domBy = d;}
   inline int domOf() {return _domOf;}
   inline void domOf(int d){_domOf = d;}
   void features(set < long > &) const;
   void getAllNodes(map <long, team*> &teamMap, set <team *> &, set <learner *> &);
   int getAction(vector < double > &,map <long, team*> &teamMap, bool, set <learner *, LearnerBidLexicalCompare> &,set <learner *, LearnerBidLexicalCompare> &, long &, vector < set <long> > &, set <team*> &, bool);
   inline void getAllAncestors(vector <long> &a) { a = _ancestors; }
   void getBehaviourSequence(vector <int> &, int);
   inline double fit(){ return _fit; }
   inline void fit(double f) { _fit = f; }
   double getMaxOutcome(int, int fitMode, int auxDouble, bool skipZeros);
   double getMeanOutcome(int,double, int fitMode, int auxDouble, bool skipZeros);
   double getMeanAuxDouble(int phase,int fitMode, int auxDouble);
   double getMedianOutcome(int phase,int fitMode, int auxDouble, bool skipZeros);
   double getMinOutcome(int phase,int fitMode, int auxDouble, bool skipZeros);
   bool getOutcome(point *, double *); /* Get outcome, return true if found. */
   inline long gtime() const { return _gtime; }
   bool hasOutcome(point * p); /* Return true if team has an outcome for this point. */
   inline long id() const { return _id; }
   inline void id(long id) { _id = id; }
   inline double key() { return _key; }
   inline void key(double key) { _key = key; }
   inline int lastCompareFactor() { return _lastCompareFactor; }
   inline void lastCompareFactor(int c) { _lastCompareFactor = c; }
   inline void members(set < learner * > *m) { m->insert(_members.begin(), _members.end()); }
   double ncdBehaviouralDistance(team*, int);
   inline int nodes() const { return _nodes; } /* This is the number of nodes at all levels, assuming this team is the root. */
   double nov(int, int);
   inline long numEval(){ return _numEval; }
   inline void numEval(long e){ _numEval = e; }
   size_t numAtomic();
   inline long numMargins() { return _margins.size(); } /* Number of margins. */
   int numOutcomes(int phase, int fitMode = -1); /* Number of outcomes. */
   void outcomes(map<point*, double, pointLexicalLessThan>&, int); /* Get all outcomes from a particular phase */
   void outcomes(int, int, vector < double >&); /* Get all outcome values of a particular type and from a particular phase.*/
   inline double parentFitness(){ return _parentFitness; }
   inline void parentFitness(double f) { _parentFitness = f; }
   inline double parentFit(){ return _parentFit; }
   inline void parentFit(double n) { _parentFit = n; }
   double policyFeatureDistance(team *, bool);
   inline void policyFeaturesGet(set< long > &f, bool active) { if (active) f = _policyFeaturesActive; else f = _policyFeatures; }
   inline void policyFeaturesSet(set< long > &f, bool active) { if (active) _policyFeaturesActive = f; else _policyFeatures = f;}
   int policyFeatures(map <long, team*> &teamMap, set <team*> &, set < long > &, bool); //populates last set with features and returns number of nodes(learners) in policy
   int policyInstructions(map <long, team*> &teamMap, set <team*> &, bool);
   string printBids(string);
   void removeLearner(learner *);
   void resetOutcomes(int); /* Delete all outcomes from phase. */
   bool root(){ return _root; }
   void root(bool r){ _root = r;}
   void setMargin(point *, double); /* Set margin. */
   inline double score(){ return _score; }
   inline void score(double f) { _score = f; }
   void setOutcome(point *, double, long); /* Set outcome. */
   inline size_t size() const { return _members.size(); } /* This is the team size of the root node. */
   double symbiontUtilityDistance(team*);
   double symbiontUtilityDistance(vector < long > &);

   //this constructor used for checkpointing
   team(long gtime, long id){
      _archived = false;
      _domBy = -1;
      _domOf = -1;
      _fit = 0.0;
      _gtime = gtime;
      _id = id;
      _key = 0;
      _lastCompareFactor = -1;
      _nodes = 1;
      _numEval = 0;
      _parentFitness = 0.0;
      _parentFit = 0.0;
      _root = false;
      _score = 0;
   };

   ~team(); /* Affects learner refs, unlike addLearner() and removeLearner(). */
   string toString(string,map<long,team*> &, set <team*> &);
   void updateActiveMembersFromIds( vector < long > &);

   friend ostream & operator<<(ostream &, const team &);  
};

struct teamIdComp
{
   bool operator() (team* t1, team* t2) const
   {
      return t1->id() < t2->id();
   }
};

struct teamScoreLexicalCompare {
   bool operator()(team* t1, team* t2) {
      //      set < long > policyFeaturesT1;
      //      set <team*> teams;
      //      int pf_t1 = t1->policyFeatures(teams,policyFeaturesT1);
      //      set < long > policyFeaturesT2;
      //      teams.clear();
      //      int pf_t2 = t2->policyFeatures(teams,policyFeaturesT2);
      if (t1->score() != t2->score()){ t1->lastCompareFactor(0); t2->lastCompareFactor(0); return t1->score() > t2->score();} //score, higher is better
      else if (t1->asize() != t2->asize()){ t1->lastCompareFactor(1); t2->lastCompareFactor(1);  return t1->asize() < t2->asize();} //team size post intron removal (active members), smaller is better
      //else if (pf_t1 != pf_t2){ t1->lastCompareFactor(3); t2->lastCompareFactor(3); return pf_t1 < pf_t2;} //total number of learners (nodes), less is better
      //else if (policyFeaturesT1.size()  != policyFeaturesT2.size()){ t1->lastCompareFactor(4); t2->lastCompareFactor(4); return policyFeaturesT1.size() < policyFeaturesT2.size();} //total number of features, less is better
      else if (t1->gtime() != t2->gtime()){ t1->lastCompareFactor(5); t2->lastCompareFactor(5); return t1->gtime() > t2->gtime();} //age, younger is better
      else { t1->lastCompareFactor(6); t2->lastCompareFactor(6); return t1->id() < t2->id();} //correlated to age but technically arbirary, id is guaranteed to be unique and thus ensures deterministic comparison
   }
};

#endif
