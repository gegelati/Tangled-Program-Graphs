#include "point.h"
///********************************************************************************************/
//string point::checkPoint(){
//	ostringstream oss;
//
//	oss <<"point:" << _id << ":" << _gtime << ":"  << _phase;
//	oss  << "|" << _pointState.size();
//	for (size_t i = 0; i <_pointState.size(); i++)
//		oss << ":" << _pointState[i];
//	oss  << "|" << _behaviouralState.size();
//	for (size_t i = 0; i <_behaviouralState.size(); i++)
//		oss << ":" << _behaviouralState[i];
//	oss << "|" << _auxDoubles.size();
//	for (size_t i = 0; i <_auxDoubles.size(); i++)
//		oss << ":" << _auxDoubles[i];
//	oss << endl;
//	return oss.str();
//}

/********************************************************************************************/
bool point::isPointUnique(set < point * > &P, bool pointState)
{
   vector < double > state;
   set < point * > :: iterator poiter;

   for(poiter = P.begin(); poiter != P.end(); poiter++)
   {
      if (pointState){
         (*poiter)->pointState(state);
         return !isEqual(_pointState, state, 1e-5);

      }
      else {
         (*poiter)->behaviouralState(state);
         return !isEqual(_behaviouralState, state, 1e-5);
      }
   }
   return true;
}

/********************************************************************************************/
point::point(long gtime,
      int phase,
      long id,
      vector<double> &pState,
      vector<double> &bState,
      vector<double> &auxDoubles,
      int fitMode)
   : _id(id),
   _gtime(gtime),
   _phase(phase),
   _fitMode(fitMode),
   _key(0), _marked(0), _solved(0), _slice(0)
{
   for(int i = 0; i < pState.size(); i++)
      _pointState.push_back(pState[i]);

   for(size_t i = 0; i < bState.size(); i++)
      _behaviouralState.push_back(bState[i]);
   for(size_t i = 0; i < auxDoubles.size(); i++)
      _auxDoubles.push_back(auxDoubles[i]);
}
/********************************************************************************************/
point::point(long gtime,
      int phase,
      long id,
      vector<double> &bState,
      vector<double> &auxDoubles,
      int fitMode)
   : _id(id),
   _gtime(gtime),
   _phase(phase),
   _fitMode(fitMode),
   _key(0), _marked(0), _solved(0), _slice(0)
{
   _pointState.reserve(0);
   _pointState.resize(0);

   for(size_t i = 0; i < bState.size(); i++)
      _behaviouralState.push_back(bState[i]);
   for(size_t i = 0; i < auxDoubles.size(); i++)
      _auxDoubles.push_back(auxDoubles[i]);
}
/********************************************************************************************/
point::point(long gtime,
      int phase,
      long id,
      vector<double> &pState)
   : _id(id),
   _gtime(gtime),
   _phase(phase),
   _key(0), _marked(0), _solved(0), _slice(0)
{
   _behaviouralState.reserve(0);
   _behaviouralState.resize(0);

   for(size_t i = 0; i < pState.size(); i++)
      _pointState.push_back(pState[i]);

   //for(size_t i = 0; i < MAX_AUX_DOUBLE; i++)
   //   _auxDoubles.push_back(0.0);
}
/********************************************************************************************/
void point::setBehaviouralState(vector < double > &bState){
   _behaviouralState.clear();
   for(int i = 0; i < bState.size(); i++)
      _behaviouralState.push_back(bState[i]);
}
/********************************************************************************************/
ostream & operator<<(ostream &os, const point &pt)
{
   os << "(" << pt._id  << ", " << pt._gtime << ")";
   return os;
}
