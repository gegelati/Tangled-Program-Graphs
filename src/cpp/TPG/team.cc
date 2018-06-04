#include <algorithm>
#include <limits>
#include "team.h"
long team::_count = 0;
/********************************************************************************************/
bool team::addLearner(learner *lr)
{
   bool added;
   added = (_members.insert(lr)).second;
   return added;
}

/********************************************************************************************/
string team::checkpoint(int phase){
   ostringstream oss;
   set < learner * > :: iterator leiter;

   oss << "team:" << _id << ":" << _gtime << ":" << _numEval << ":" << _auxFlag;
   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
      oss << ":" << (*leiter)->id();

   oss << endl;

   return oss.str();
}

/********************************************************************************************/
void team::clearRegisters(map <long, team*> &teamMap, set <team*> &visitedTeams)
{
   visitedTeams.insert(teamMap[_id]);

   set < learner * > :: iterator leiter;

   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
   {
      (*leiter)->clearRegisters();
      if ((*leiter)->action() >= 0 && find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*leiter)->action()]) == visitedTeams.end())
         teamMap[(*leiter)->action()]->clearRegisters(teamMap, visitedTeams);
   }
}

ostream & operator<<(ostream &os, 
      const team &tm)
{
   set < long > features;
   set < long > :: iterator seiter;
   long sumfeat, sumlsize, sumlesize;

   set < learner * > :: iterator leiter;

   tm.features(features);

   for(sumfeat = 0, sumlsize = 0, sumlesize = 0,
         leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
   {
      sumfeat += (*leiter)->numFeatures();
      sumlsize += (*leiter)->size();
      sumlesize += (*leiter)->esize();
   }

   os << "teamout";
   os << " id " << tm.id();
   os << " size " << tm.size();
   os << " active " << tm.asize();
   os << " nodes " << tm.nodes();
   os << " gtime " << tm.gtime();
   os << " feat " << sumfeat << " " << (double) sumfeat / tm.size();
   os << " uniqfeat " << features.size() << " " << (double) features.size() / tm.size();
   os << " lsize " << sumlsize << " " << (double) sumlsize / tm.size();
   os << " lesize " << sumlesize << " " << (double) sumlesize / tm.size();

   os << " id";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++){
      //uintptr_t ptr_val;// = uintptr_t(*leiter);
      os << " " << (*leiter)->id();// << "(" << (long long)(*leiter) << ")";
   }


   os << " size";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->size();

   os << " esize";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->esize();

   os << " gtime";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->gtime();

   os << " refs";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->refs();

   os << " action";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->action();

   os << " numfeat";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
      os << " " << (*leiter)->numFeatures();

   os << " lfeat";
   for(leiter = tm._members.begin(); leiter != tm._members.end(); leiter++)
   {
      os << " lact " << (*leiter)->action() << " lfid";
      features.clear();
      (*leiter)->features(features);
      for(seiter = features.begin(); seiter != features.end(); seiter++)
         os << " " << *seiter;
   }

   features.clear();
   tm.features(features);

   os << " featid";
   for(seiter = features.begin(); seiter != features.end(); seiter++)
      os << " " << *seiter;

   ////output the same info for active members only
   //os << " idA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->id();

   //os << " sizeA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->size();

   //os << " esizeA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->esize();

   //os << " gtimeA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->gtime();

   //os << " refsA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->refs();

   //os << " actionA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->action();

   //os << " numfeatA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //   os << " " << (*leiter)->numFeatures();

   //os << " lfeatA";
   //for(leiter = tm._active.begin(); leiter != tm._active.end(); leiter++)
   //{
   //   os << " lactA " << (*leiter)->action() << " lfidA";
   //   features.clear();
   //   (*leiter)->features(features);
   //   for(seiter = features.begin(); seiter != features.end(); seiter++)
   //      os << " " << *seiter;
   //}

   //features.clear();
   //tm.features(features);

   //os << " featidA";
   //for(seiter = features.begin(); seiter != features.end(); seiter++)
   //   os << " " << *seiter;

   return os;
}

/********************************************************************************************/
long team::collectiveAge(long t){
   set < learner * > :: iterator leiter;
   long age = 0;
   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
      age+= t - (*leiter)->gtime();
   return age;
}

/********************************************************************************************/
void team::deleteOutcome(point *pt)
{
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;

   if((ouiter = _outcomes.find(pt)) == _outcomes.end())
      die(__FILE__, __FUNCTION__, __LINE__, "should not delete outcome that is not set");
   delete ouiter->first;
   _outcomes.erase(ouiter);
}

/********************************************************************************************/
void team::deleteMargin(point *pt)
{
   map < point *, double > :: iterator maiter;

   if((maiter = _margins.find(pt)) == _margins.end())
      die(__FILE__, __FUNCTION__, __LINE__, "should not delete margin that is not set");

   _margins.erase(maiter);
}

/********************************************************************************************/
void team::features(set < long > &F) const
{
   set < learner * > :: iterator leiter;

   if(F.empty() == false)
      die(__FILE__, __FUNCTION__, __LINE__, "feature set not empty");

   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
      (*leiter)->features(F);
}

/********************************************************************************************/
double team::nov(int type, int kNN){       
   multiset<double>::iterator it;
   double nov = 0;
   int i = 0;
   if (type==0){
      for (it=_distances_0.begin(); it!=_distances_0.end() && i<=kNN; ++it, i++)
         nov += *it;
      return nov/i;
   }
   else if (type==1){
      for (it=_distances_1.begin(); it!=_distances_1.end() && i<=kNN; ++it, i++)
         nov += *it;
      return nov/i;
   }
   else if (type==2){
      for (it=_distances_2.begin(); it!=_distances_2.end() && i<=kNN; ++it, i++)
         nov += *it;
      return nov/i;
   }
   else return -1;
}

/********************************************************************************************/
double team::symbiontUtilityDistance(team * t){
   vector<int> symbiontIntersection;
   vector<int> symbiontUnion;
   vector<int>::iterator it;
   int symIntersection;
   int symUnion;
   vector < long > team1Ids;
   vector < long > team2Ids;
   set < learner * > activeMembers;
   t->activeMembers(&activeMembers);
   /* if either team has no active members then return 0 */
   if (_active.size() < 1 || activeMembers.size() < 1)
      return 0.0;
   set < learner * > :: iterator leiter;
   for(leiter = _active.begin(); leiter != _active.end(); leiter++)
      team1Ids.push_back((*leiter)->id());
   for(leiter = activeMembers.begin(); leiter != activeMembers.end(); leiter++)
      team2Ids.push_back((*leiter)->id());
   sort(team1Ids.begin(), team1Ids.end());
   sort(team2Ids.begin(), team2Ids.end());
   set_intersection (team1Ids.begin(), team1Ids.end(), team2Ids.begin(), team2Ids.end(), back_inserter(symbiontIntersection));
   symIntersection = symbiontIntersection.size();
   set_union (team1Ids.begin(), team1Ids.end(), team2Ids.begin(), team2Ids.end(), back_inserter(symbiontUnion));
   symUnion = symbiontUnion.size();
#ifdef MYDEBUG
   cout << "genoDiffa t1Size " << _active.size() << " t1Ids " << vecToStr(team1Ids);
   cout << " allMembersSize " << _members.size();
   cout << " t2Size " << t->asize() << " t2Ids ";
   cout << vecToStr(team2Ids) << " symIntersection " << vecToStr(symbiontIntersection) << " symIntersectionSize " << symIntersection;
   cout << " symUnion " << vecToStr(symbiontUnion) << " symUnionSize " << symUnion << " diff " << 1.0 - ((double)symIntersection / (double)symUnion) << endl;
#endif
   return 1.0 - ((double)symIntersection / (double)symUnion);
}
/********************************************************************************************/
//this version compares with a vector of Ids *assumed sorted*
double team::symbiontUtilityDistance(vector < long > & compareWithThese){
   vector<int> symbiontIntersection;
   vector<int> symbiontUnion;
   vector<int>::iterator it;
   int symIntersection;
   int symUnion;
   vector < long > team1Ids;
   /* if either team has no active members then return 0 */
   if (_active.size() < 1 || compareWithThese.size() < 1)
      return 0.0;
   set < learner * > :: iterator leiter;
   for(leiter = _active.begin(); leiter != _active.end(); leiter++)
      team1Ids.push_back((*leiter)->id());
   sort(team1Ids.begin(), team1Ids.end());
   set_intersection (team1Ids.begin(), team1Ids.end(), compareWithThese.begin(), compareWithThese.end(), back_inserter(symbiontIntersection));
   symIntersection = symbiontIntersection.size();
   set_union (team1Ids.begin(), team1Ids.end(), compareWithThese.begin(), compareWithThese.end(), back_inserter(symbiontUnion));
   symUnion = symbiontUnion.size();
#ifdef MYDEBUG
   cout << "genoDiffb t1Size " << _active.size() << " t1Ids " << vecToStr(team1Ids);
   cout << " allMembersSize " << _members.size();
   cout << " t2Size " << compareWithThese.size() << " t2Ids ";
   cout << vecToStr(compareWithThese) << " symIntersection " << vecToStr(symbiontIntersection) << " symIntersectionSize " << symIntersection;
   cout << " symUnion " << vecToStr(symbiontUnion) << " symUnionSize " << symUnion << " diff " << 1.0 - ((double)symIntersection / (double)symUnion) << endl;
#endif
   return 1.0 - ((double)symIntersection / (double)symUnion);
}

/********************************************************************************************
 * Returns the index of the action to be taken in the environment.
 * This version populates learnersRankeded with all learners in *this team*, sorted by LearnerBidLexicalCompare
 * winningLeaners is populated with every learner that scores a winning bid per decision.
 */
int team::getAction(vector < double > &state, 
      map <long, team*> &teamMap,
      bool updateActive,
      set <learner *, LearnerBidLexicalCompare> &learnersRanked, //domains like Ms. Pac-Man Need this
      set <learner *, LearnerBidLexicalCompare> &winningLearners, //for reporting 
      long &decisionInstructions, 
      vector < set <long> > &decisionFeatures,
      set <team*> &visitedTeams,
      bool verbose)
{
   //if(_members.size() < 2)
   //   die(__FILE__, __FUNCTION__, __LINE__, "team is too small");

   visitedTeams.insert(teamMap[_id]);

   set < learner * > :: iterator leiter, leiterend;

   //double *REG = (double *) alloca(REGISTERS * sizeof(double));

   set <long> features;
   set <long> featuresSingle;

   for(leiter = _members.begin(), leiterend = _members.end(); /* Go through all the learners. */
         leiter != leiterend; leiter++)
   {
      (*leiter)->bidVal((*leiter)->bid(&state[0]));
      (*leiter)->key((*leiter)->bidVal());
      decisionInstructions += (*leiter)->esize();
      (*leiter)->features(featuresSingle);
      features.insert(featuresSingle.begin(),featuresSingle.end());
   }

   decisionFeatures.push_back(features);

   learnersRanked.clear(); 
   learnersRanked.insert(_members.begin(),_members.end()); //will sort programs by bid

   //if (verbose){
   //for(set<learner*, LearnerBidLexicalCompare> :: iterator it = learnersRanked.begin(); it != learnersRanked.end();++it)
   //   cout << "team::getAction tm " << _id  << " root " << _root  << " size " << _members.size() << " prog " << (*it)->id() << "->" << (*it)->action() << " bid " << (*it)->bidVal() << endl;
   //}

   //Loop through learners highest to lowest bid:
   //1. If the learner is atomic, return the action
   //2. Else if the meta-action has not been visited yet, follow the meta-action
   //Note: Every team has at least 1 "fail-safe" atomic learner.
   long teamIdToFollow = -(numeric_limits<long>::max());
   for(set<learner*, LearnerBidLexicalCompare> :: iterator it = learnersRanked.begin(); it != learnersRanked.end();++it){
      if ( (*it)->action() < 0 ){//atomic
         /* Force champion learner with atomic action to beginning of set,
            followed by every other learner in this team, in order of bid */
         //(*it)->key(numeric_limits<double>::max());
         //if (verbose){ if (_root) {cout << endl << "team::getAction";} cout << " h" << _id << " s" << (*it)->id(); }// cout << "team::getAction tm " << _id  << " root " << _root << " following atomic " << (*it)->id() << endl;
         if(updateActive) _active.insert(*it);
         winningLearners.insert(*it);

         //featuresSingle.clear(); //don't need this
         //(*it)->features(featuresSingle);
         //decisionFeatures.insert(featuresSingle.begin(),featuresSingle.end());

         return (*it)->action();
      } 
      else if (find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*it)->action()]) == visitedTeams.end()){
         teamIdToFollow = (*it)->action();
         //if (verbose){ if (_root) {cout << endl << "team::getAction";} cout << " h" << _id << " s" << (*it)->id(); }// cout << "team::getAction tm " << _id  << " root " << _root << " following meta " << (*it)->id() << endl;
         if(updateActive) _active.insert(*it);
         winningLearners.insert(*it);

         //featuresSingle.clear(); //don't need this
         //(*it)->features(featuresSingle);
         //decisionFeatures.insert(featuresSingle.begin(),featuresSingle.end());

         break;
      }
   }
   if (teamIdToFollow == -(numeric_limits<long>::max())) cerr << "WTF FOUND NO TEAM OR ATOMIC!" << endl;
   /* Repeat at the chosen team in the level below. */
   return teamMap[teamIdToFollow]->getAction(state, teamMap, updateActive, learnersRanked, winningLearners, decisionInstructions, decisionFeatures, visitedTeams, verbose);
}
/********************************************************************************************/
void team::getAllNodes(map <long, team*> &teamMap, set <team*> &visitedTeams, set <learner*> &learners)
{
   visitedTeams.insert(teamMap[_id]);

   set < learner * > :: iterator leiter;

   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
   {
      learners.insert(*leiter);
      if ((*leiter)->action() >= 0 && find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*leiter)->action()]) == visitedTeams.end())
         teamMap[(*leiter)->action()]->getAllNodes(teamMap, visitedTeams, learners);
   }
}
/********************************************************************************************/
double team::policyFeatureDistance(team * t, bool active){
   vector<int> featureIntersection;
   vector<int> featureUnion;
   vector<int>::iterator it;
   int featIntersection;
   int featUnion;
   set < long > tFeatures;
   set <long > pFeatures = active ? _policyFeaturesActive : _policyFeatures;
   t->policyFeaturesGet(tFeatures, active);
   /* if either team indexes no features then return 0 */
   if (pFeatures.size() == 0 || tFeatures.size() == 0)
      return 0.0;
   set < learner * > :: iterator leiter;
   set_intersection (pFeatures.begin(), pFeatures.end(), tFeatures.begin(), tFeatures.end(), back_inserter(featureIntersection));
   featIntersection = featureIntersection.size();
   set_union (pFeatures.begin(), pFeatures.end(), tFeatures.begin(), tFeatures.end(), back_inserter(featureUnion));
   featUnion = featureUnion.size();
#ifdef MYDEBUG
   cout << "team::policyFeatureDistance pf1Size " << pFeatures.size() << " pf1Feats"; 
   set < long > :: iterator iter;
   for(iter=pFeatures.begin(); iter!=pFeatures.end();++iter)
      cout << " " << (*iter);

   cout << " pf2Size " << tFeatures.size() << " pf2Feats";
   for(iter=tFeatures.begin(); iter!=tFeatures.end();++iter)
      cout << " " << (*iter);

   cout << " featureIntersection " << vecToStr(featureIntersection) << " featureIntersectionSize " << featIntersection;
   cout << " featUnion " << vecToStr(featureUnion) << " featureUnionSize " << featUnion << " diff " << 1.0 - ((double)featIntersection / (double)featUnion) << endl;
#endif
   return 1.0 - ((double)featIntersection / (double)featUnion);
}

/********************************************************************************************/
// Fill F with every feature indexed by every learner in this policy (tree).If we ever build 
// massive policy tress, this should be changed to a more efficient traversal. For now just 
// look at every node.
int team::policyFeatures(map <long, team*> &teamMap, set <team*> &visitedTeams, set < long > &F, bool active)
{
   visitedTeams.insert(teamMap[_id]);

   set < learner * > :: iterator leiter, leiterend;
   set < long > featuresSingle;
   //set < long > :: iterator feiter;
   int numLearnersInPolicy = 0;
   set < learner * > learnersToConsider = active ? _active : _members;
   for(leiter = learnersToConsider.begin(), leiterend = learnersToConsider.end(); leiter != leiterend; leiter++)
   {
      numLearnersInPolicy++;
      featuresSingle.clear();//don't need this
      (*leiter)->features(featuresSingle);
      F.insert(featuresSingle.begin(),featuresSingle.end());

      //cout << "policyFeatures tm " << _id << " numFeatSingle " << featuresSingle.size() << " feat";
      //for(feiter = featuresSingle.begin(); feiter != featuresSingle.end(); feiter++)
      //   cout << " " << *feiter;
      //cout << " numPFeat " << F.size() << " pFeat";
      //for(feiter = F.begin(); feiter != F.end(); feiter++)
      //   cout << " " << *feiter;
      //cout << endl;

      if ((*leiter)->action() >= 0 && find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*leiter)->action()]) == visitedTeams.end())
         numLearnersInPolicy += teamMap[(*leiter)->action()]->policyFeatures(teamMap, visitedTeams, F, active);
   }
   return numLearnersInPolicy;
}

/********************************************************************************************/
// Count all effective instructions in every learner in this policy (tree). Make sure to 
// mark introns first.
int team::policyInstructions(map <long, team*> &teamMap, set <team*> &visitedTeams, bool effective)
{
   visitedTeams.insert(teamMap[_id]);

   set < learner * > :: iterator leiter, leiterend;
   int numInstructions = 0;
   for(leiter = _members.begin(), leiterend = _members.end(); leiter != leiterend; leiter++)
   {
      if (effective)
         numInstructions += (*leiter)->esize();
      else
         numInstructions += (*leiter)->size();

      if ((*leiter)->action() >= 0 && find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*leiter)->action()]) == visitedTeams.end())
         numInstructions += teamMap[(*leiter)->action()]->policyInstructions(teamMap, visitedTeams, effective);
   }
   return numInstructions;
}

/********************************************************************************************/
void team::getBehaviourSequence(vector<int>&s, int phase){
   vector <double> singleEpisodeBehaviour;
   map < point *, double >::reverse_iterator rit;
   for (rit=_outcomes.rbegin(); rit!=_outcomes.rend(); rit++){
      if ((rit->first)->phase() == phase){
         (rit->first)->behaviouralState(singleEpisodeBehaviour);
         if (s.size() + singleEpisodeBehaviour.size() <= MAX_NCD_PROFILE_SIZE)
            s.insert(s.end(),singleEpisodeBehaviour.begin(),singleEpisodeBehaviour.end()); //will cast double to int (only discrete values used)
         else
            break;
      }
   }
}

/********************************************************************************************/
bool team::getMargin(point *pt,
      double *margin)
{
   map < point *, double > :: iterator maiter;

   if((maiter = _margins.find(pt)) == _margins.end())
      return false;

   *margin = maiter->second;

   return true;
}

/********************************************************************************************/
double team::getMaxOutcome(int phase, int fitMode, int auxDouble, bool skipZeros)
{
   double maxOut = 0;
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++)
      if (((ouiter->first)->phase() == phase && 
               (fitMode == -1 || (ouiter->first)->fitMode() == fitMode) && 
               (ouiter->first)->auxDouble(auxDouble) > maxOut) &&
            (!skipZeros || (skipZeros && !isEqual((ouiter->first)->auxDouble(auxDouble),0))))
         maxOut = (ouiter->first)->auxDouble(auxDouble);
   return maxOut;
}

/********************************************************************************************/
double team::getMinOutcome(int phase, int fitMode, int auxDouble, bool skipZeros)
{
   double minOut = numeric_limits<double>::max();
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++)
      if (((ouiter->first)->phase() == phase && (fitMode == -1 || (ouiter->first)->fitMode() == fitMode) && (ouiter->first)->auxDouble(auxDouble) < minOut) &&
            (!skipZeros || (skipZeros && !isEqual((ouiter->first)->auxDouble(auxDouble),0))))
         minOut = (ouiter->first)->auxDouble(auxDouble);
   return minOut;
}

/********************************************************************************************/
double team::getMeanOutcome(int phase, double topPortion, int fitMode, int auxDouble, bool skipZeros)
{
   vector < double > outcomes;
   double sum = 0;
   if (_outcomes.size() == 0)
      return sum;

   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++){
      if (((ouiter->first)->phase() == phase && (fitMode == -1 || (ouiter->first)->fitMode() == fitMode)) && 
            (!skipZeros || (skipZeros && !isEqual((ouiter->first)->auxDouble(auxDouble),0))))
         outcomes.push_back((ouiter->first)->auxDouble(auxDouble));

   }
   sort (outcomes.begin(), outcomes.end(), greater<double>());
   return accumulate(outcomes.begin(),outcomes.begin()+(int)(topPortion*outcomes.size()),0.0)/(int)(topPortion*outcomes.size());
}

/********************************************************************************************/
double team::getMedianOutcome(int phase, int fitMode, int auxDouble, bool skipZeros)
{
   vector <double> outcomes;
   if (_outcomes.size() == 0)
      return 0;
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++){
      if ((ouiter->first)->auxDouble(auxDouble) > -numeric_limits<double>::max() && //min double indicates unused value
            ((ouiter->first)->phase() == phase && (fitMode == -1 || (ouiter->first)->fitMode() == fitMode)) &&
            (!skipZeros || (skipZeros && !isEqual((ouiter->first)->auxDouble(auxDouble),0))))
         outcomes.push_back((ouiter->first)->auxDouble(auxDouble));
   }
   return vecMedian(outcomes);
}

/********************************************************************************************/
bool team::getOutcome(point *pt,
      double *out)
{
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;

   if((ouiter = _outcomes.find(pt)) == _outcomes.end())
      return false;

   *out = ouiter->second;

   return true;
}
/********************************************************************************************/
bool team::hasOutcome(point *pt)
{
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;

   if((ouiter = _outcomes.find(pt)) == _outcomes.end())
      return false;

   return true;
}

/********************************************************************************************/
/* Calculate normalized compression distance w.r.t another team. */
double team::ncdBehaviouralDistance(team * t, int phase){
   ostringstream oss;
   vector <int> theirBehaviourSequence;
   t->getBehaviourSequence(theirBehaviourSequence, phase);
   vector <int> myBehaviourSequence;
   getBehaviourSequence(myBehaviourSequence, phase);
   if (myBehaviourSequence.size() == 0 || theirBehaviourSequence.size() == 0)
      return -1;
   return normalizedCompressionDistance(myBehaviourSequence,theirBehaviourSequence);
}

/********************************************************************************************/
size_t team::numAtomic(){
   int numAtomic = 0;
   set < learner * > :: iterator leiter, leiterend;

   for(leiter = _members.begin(), leiterend = _members.end();
         leiter != leiterend; leiter++)
   {
      if ((*leiter)->action() < 0)
         numAtomic++;
   }

   return numAtomic;
}

/********************************************************************************************/
int team::numOutcomes(int phase, int fitMode)
{
   int numOut = 0;
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++)
      if (ouiter->first->phase() == phase && (fitMode == -1 || ouiter->first->fitMode() == fitMode))
         numOut++;
   return numOut;
}

/********************************************************************************************/
void team::outcomes(int i, int phase, vector < double > &outcomes){
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++)
      if ((ouiter->first)->phase() == phase)
         outcomes.push_back((ouiter->first)->auxDouble(i));
}

/********************************************************************************************/
void team::outcomes(map < point*, double, pointLexicalLessThan > &outcomes,int phase){
   outcomes = _outcomes;
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for(ouiter = _outcomes.begin(); ouiter != _outcomes.end(); ouiter++)
      if ((ouiter->first)->phase() == phase)
         outcomes.insert(*ouiter);
}

/********************************************************************************************/
string team::printBids(string prefix)
{
   set < learner * > :: iterator leiter;

   ostringstream oss, ossout;
   int i;

   for(i = 0, leiter = _members.begin(); leiter != _members.end(); i++, leiter++)
   {
      oss.str("");
      oss << prefix << " " << i << " lid " << (*leiter)->id() << " act " << (*leiter)->action();
      oss << " size " << (*leiter)->size() << " esize " << (*leiter)->esize();
      ossout << (*leiter)->printBid(oss.str());
   }

   return ossout.str();
}

/********************************************************************************************/
team::~team()
{
   set < learner * > :: iterator leiter, leiterend;

   for(leiter = _members.begin(), leiterend = _members.end();
         leiter != leiterend; leiter++)
      (*leiter)->refDec();

   map < point *, double, pointLexicalLessThan > :: iterator ouiter;

   for (ouiter = _outcomes.begin(); ouiter != _outcomes.end();){
      delete ouiter->first;
      _outcomes.erase(ouiter++);
   }
}

/********************************************************************************************/
void team::removeLearner(learner *lr)
{
   set < learner * > :: iterator leiter;

   if((leiter = _members.find(lr)) == _members.end())
      die(__FILE__, __FUNCTION__, __LINE__, "should not remove learner that is not there");

   _members.erase(leiter);

   if((leiter = _active.find(lr)) != _active.end())
      _active.erase(leiter);
}

/********************************************************************************************/
void team::resetOutcomes(int phase)
{
   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   for (ouiter = _outcomes.begin(); ouiter != _outcomes.end();)
   {
      if ((ouiter->first)->phase() == phase){
         delete ouiter->first;
         _outcomes.erase(ouiter++);
      }
      else
         ouiter++;
   }
}

/********************************************************************************************/
void team::setMargin(point *pt,
      double margin)
{
   if((_margins.insert(map < point *, double >::value_type(pt, margin))).second == false)
      die(__FILE__, __FUNCTION__, __LINE__, "could not set margin, duplicate point?");
}

/********************************************************************************************/
void team::setOutcome(point *pt, double out, long nso)
{
   if((_outcomes.insert(map < point *, double >::value_type(pt, out))).second == false)
      die(__FILE__, __FUNCTION__, __LINE__, "could not set outcome, duplicate point?");

   //if (numOutcomes(pt->phase()) > nso){
   //   map < point *, double, pointLexicalLessThan > :: iterator ouiter;
   //   ouiter = _outcomes.begin();
   //   while (ouiter->first->phase() != pt->phase()) ouiter++;
   //   if (ouiter != _outcomes.end()){
   //      delete ouiter->first;
   //      _outcomes.erase(ouiter);
   //   }
   //}
}

/********************************************************************************************/
string team::toString(string prefix, map<long,team*> &teamMap, set <team*> &visitedTeams)
{
   visitedTeams.insert(teamMap[_id]);
   set < learner * > :: iterator leiter;

   ostringstream oss;
   prefix = "   " + prefix;
   oss << prefix << " team::toString id " << _id;
   oss << " gtime " << _gtime;
   oss << " nodes " << _nodes;
   oss << " numOut " << _outcomes.size();//ignoring phase and fitMode
   oss << " size " << _members.size();
   oss << " asize " << _active.size();
   oss << endl;

   oss << prefix << " learners";
   bool metaActionsPresent = false;
   for(leiter = _members.begin(); leiter != _members.end(); leiter++){
      vector < instruction * > bid;
      (*leiter)->getBid(bid);
      oss << " " << (*leiter)->id() << "(" << (long long)(*leiter) << ")->" << (*leiter)->action();
      //oss << " bid:";
      //for (int i = 0; i < bid.size(); i++) oss << " " << bid[i]->to_string();
      if ((*leiter)->action() < 0)
         oss << "(a)";
      else{
         metaActionsPresent = true;
         oss << "(m)";
      }
   }
   if (metaActionsPresent == true){
      oss << endl << "meta-actions:" << endl;
      for(leiter = _members.begin(); leiter != _members.end(); leiter++){
         if ((*leiter)->action() >= 0 && find(visitedTeams.begin(), visitedTeams.end(), teamMap[(*leiter)->action()]) == visitedTeams.end()){
            oss << prefix << (*leiter)->id() <<"->" << (*leiter)->action() << ":" << endl;
            oss << teamMap[(*leiter)->action()]->toString(prefix, teamMap, visitedTeams);
         }
      }
   }
   else
      oss << endl;
   return oss.str();
}

/********************************************************************************************/
void team::updateActiveMembersFromIds( vector < long > &activeMemberIds){
   sort(activeMemberIds.begin(), activeMemberIds.end()); //for binary search
   set < learner * > :: iterator leiter;
   for(leiter = _members.begin(); leiter != _members.end(); leiter++)
      if (binary_search(activeMemberIds.begin(), activeMemberIds.end(), (*leiter)->id()))
         _active.insert(*leiter);
}
