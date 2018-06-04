#ifndef point_h
#define point_h

#include <vector>
#include <map>
#include <set>

#include "misc.h"

using namespace std;

class point
{
	vector <double> _auxDoubles;
	vector < double > _behaviouralState;
        int _fitMode;
	long _gtime;
	long _id;
	double _key; /* For sorting. */
	bool _marked;
	int _phase; /* Phase: TRAIN_MODE, VALIDATION_MODE, or TEST_MODE. */
	vector < double > _pointState;
	double _slice; /* For roulette wheel. */
	bool _solved; /* If this point has been solved at some time. */

public:
	inline double auxDouble(int i){ return _auxDoubles[i]; }
	inline void auxDouble(int i, double d) { _auxDoubles[i] = d; }
	inline void behaviouralState(vector < double > &v){ v = _behaviouralState; }
	//string checkPoint();
	inline int dimPoint(){ return _pointState.size(); }
	inline int dimBehavioural(){ return _behaviouralState.size(); }
	inline int fitMode() { return _fitMode; }
        inline long gtime(){ return _gtime; }
	inline long id(){ return _id; }
	bool isPointUnique(set < point * > &, bool); /* Return true if the point is unique w.r.t. another point. */
	inline double key(){ return _key; }
	inline void key(double key){ _key = key; }
	inline void mark(){ _marked = 1; }
	inline bool marked(){ return _marked; }
	inline int phase(){ return _phase; }
	inline void phase(int p){ _phase = p; }
	point(long,  int, long, vector< double > &,  vector< double > &, vector < double > &, int auxInt=0);
	point(long,  int, long, vector< double > &,  vector< double > &, int auxInt=0);
	point(long,  int, long, vector< double > &);
	inline void pointState(vector < double > &v){ v = _pointState; }
	void setBehaviouralState(vector < double > &);
	inline double slice(){ return _slice; }
	inline void slice(double slice){ _slice = slice; }
	inline bool solved(){ return _solved; }
	inline void solved(bool s){ _solved = s; }

	friend ostream & operator<<(ostream &, const point &);
};

struct pointLexicalLessThan {
	bool operator()( point* p1 , point* p2) const
	{
		//if (p1->auxDouble(_TRAIN_REWARD) != p2->auxDouble(_TRAIN_REWARD)){ return p1->auxDouble(_TRAIN_REWARD) < p2->auxDouble(_TRAIN_REWARD); } //reward, smaller implies less
		//else if (p1->gtime() != p2->gtime()){return p1->gtime() < p2->gtime();} //age, older implies less
		return  p1->id() < p2->id(); //correlated to age but technically arbitrary, id is guaranteed to be unique and thus ensures deterministic comparison
	}
};
#endif
