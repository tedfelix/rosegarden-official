// -*- c-file-style:  "bsd" -*-

// Question: is FastVector actually any faster than std::vector
// for random inserts and erases?  Let's test them.

#include "FastVector.h"
#include "CloningWrapper.h"

#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <cstdlib>

#include <sys/time.h>
#include <sys/times.h>

using namespace std;

class BigObject
{
public:
    BigObject(int i) {
	for (int j = 0; j < 1000; ++j) {
	    m_v.push_back(i);
	}
    }
    ~BigObject() { };
    
    BigObject(const BigObject &b) : m_v(b.m_v) { }
    BigObject &operator=(const BigObject &b) {
	if (this != &b) { m_v = b.m_v; }
	return *this;
    }

    bool operator<(const BigObject &b) const { return m_v[0] < b.m_v[0]; }

private:
    vector<int> m_v;
};

template <class Container, class Iterator, class Data>
void test(int outerCount, int innerCount, bool prepend = false, bool dosort = false)
{
    clock_t st, et;
    struct tms spare;
    st = times(&spare);

    int insertions = 0;
    int deletions = 0;

    for (int j = 0; j < outerCount; ++j) {
    
	Container *c = new Container();
	Iterator *pi = new Iterator[40];
	
	for (int i = 0; i < innerCount; ++i) {

	    // always insert, but a tenth of the time delete as well

	    long posn = 0;

	    if (i == 40) {
		for (int k = 0; k < 40; ++k) {
		    pi[k] = c->begin();
		    pi[k] += k;
		}
	    }

	    if (c->begin() != c->end()) {
		if (prepend) posn = 0;
		else posn = rand() % (c->size());
	    }
	    
	    Iterator ci = c->begin();
	    ci += posn;
	    Data d(i);
	    c->insert(ci, d);
	    
	    ++insertions;
	    
	    if (c->begin() != c->end() && rand() < (RAND_MAX/5)) {
		posn = rand() % (c->size());
		ci = c->begin();
		ci += posn;
		c->erase(ci);
		++deletions;
	    }
	}

	if (dosort) {
	    stable_sort(c->begin(), c->end());
	    for (int k = 0; k < c->size()-1; ++k) {
		assert(!((*c)[k+1] < (*c)[k]));
	    }
	}

	delete[] pi;
	delete c;
    }

//    cout << insertions << " insertions, " << deletions << " deletions" << endl;

    et = times(&spare);

    cout << (et-st)*10 << "ms" << endl;
}

int main(int argc, char **argv)
{
    srand(time(0));

    cout << "sizeof vector<int>: " << sizeof(vector<int>) << endl;
    cout << "sizeof FastVector<int>: " << sizeof(FastVector<int>)
	 << endl;
    cout << "sizeof RobustIteratorVector<int>: "
	 << sizeof(RobustIteratorVector<int>) << endl << endl;
    
    int iterations = 10000;
    long t;

    cout  << iterations << " elements in vector<int>: ";
    test<vector<int>, vector<int>::iterator, int>(1, iterations);

    cout  << iterations << " elements in FastVector<int>: ";
    test<FastVector<int>, FastVector<int>::iterator, int>(1, iterations);

    cout  << iterations << " elements in RobustIteratorVector<int> with 40 extant iterators: ";
    test<RobustIteratorVector<int>, RobustIteratorVector<int>::iterator,
	int>(1, iterations);

    iterations = 10000;

    cout  << iterations << " elements in vector<int>, prepending only: ";
    test<vector<int>, vector<int>::iterator, int>(1, iterations, true);

    cout  << iterations << " elements in FastVector<int>, prepending only: ";
    test<FastVector<int>, FastVector<int>::iterator, int>(1, iterations, true);

    cout  << iterations << " elements in RobustIteratorVector<int> with 40 extant iterators, prepending only: ";
    test<RobustIteratorVector<int>,
	RobustIteratorVector<int>::iterator,
	int>(1, iterations, true);

    iterations = 10000;

    cout  << iterations << " elements in vector<int>, with sort: ";
    test<vector<int>, vector<int>::iterator, int>(1, iterations, false, true);

    cout  << iterations << " elements in FastVector<int>, with sort: ";
    test<FastVector<int>, FastVector<int>::iterator, int>(1, iterations, false, true);

    cout  << iterations << " elements in RobustIteratorVector<int> with 40 extant iterators, with sort: ";
    test<RobustIteratorVector<int>,
	RobustIteratorVector<int>::iterator,
	int>(1, iterations, false, true);

    iterations = 1000;

    cout  << iterations << " elements in vector<BigObject>: ";
    test<vector<BigObject>, vector<BigObject>::iterator, BigObject>(1, iterations);

    cout  << iterations << " elements in FastVector<BigObject>: ";
    test<FastVector<BigObject>, FastVector<BigObject>::iterator, BigObject>(1, iterations);

    cout  << iterations << " elements in RobustIteratorVector<BigObject> with 40 extant iterators: ";
    test<RobustIteratorVector<BigObject>,
	RobustIteratorVector<BigObject>::iterator,
	BigObject>(1, iterations);

    iterations = 100;
    int outer = 1000;

    cout  << outer << " repetitions of " << iterations << " elements in vector<int>: ";
    test<vector<int>, vector<int>::iterator, int>(outer, iterations);

    cout  << outer << " repetitions of " << iterations << " elements in FastVector<int>: ";
    test<FastVector<int>, FastVector<int>::iterator, int>(outer, iterations);

    cout  << outer << " repetitions of " << iterations << " elements in RobustIteratorVector<int> with 40 extant iterators: ";
    test<RobustIteratorVector<int>,
	RobustIteratorVector<int>::iterator,
	int>(outer, iterations);

    cout << endl;

}


