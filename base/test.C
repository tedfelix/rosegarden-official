// -*- c-file-style:  "bsd" -*-

// This does some rather shoddy tests on a few classes.
// The classes included here are exercised:

#include "Element.h"
#include "CloningWrapper.h"
#include "FastVector.h"

#include "Event.h"

// (end of exercised classes)


#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <sys/times.h>

using namespace std;

struct A
{
    A(int i) : m_i(i) { }

    virtual void show() const { cout << "A " << m_i << endl; }
    virtual A *clone() const { return new A(*this); }

    int m_i;
};

struct B : A
{
    B(int i, int j) : A(i), m_j(j) { }

    virtual void show() const { cout << "B " << m_j << " in "; A::show(); }
    virtual A *clone() const { return new B(*this); }

    int m_j;
};

template <class T>
class ShowWrappedObject // functor
{
public:
    void operator()(const CloningWrapper<T *> &cw) { cw->show(); }
};


int main(int argc, char **argv)
{
    clock_t st, et;
    struct tms spare;

    cout << "Testing Element2..." << endl;

    Event e("sys", "note");
    e.set<Int>("duration", 20);
    cout << "duration is " << e.get<Int>("duration") << endl;

    e.set<Bool>("someBoolProp", true);
    e.set<String>("someStringProp", "foobar");


    cout << "Testing debug dump : " << endl;
    e.dump(cout);
    cout << endl << "dump finished" << endl;

    try {
	cout << "duration is " << e.get<String>("duration") << endl;
    } catch (Event::BadType bt) {
	cout << "Correctly caught BadType when trying to get<String> of duration" << endl;
    }

    try {
	cout << "dummy prop is " << e.get<String>("nonexistentprop") << endl;
    } catch (Event::NoData bt) {
	cout << "Correctly caught NoData when trying to get non existent property" << endl;
    }

    e.setFromString<Int>("duration", "30");
    cout << "duration is " << e.get<Int>("duration") << endl;

    e.setFromString<String>("annotation", "This is my house");
    cout << "annotation is " << e.get<String>("annotation") << endl;

    cout << "Testing Element..." << endl;

    try {
	Rosegarden::Element ed("(sys::note (duration 1) (label \"My Note (may contain brackets)\") (pitch 128) (some-tag-or-other))");
	string rf = ed.flatten();
	cout << "Parsed ElementData:: reflattened as:" << endl << rf << endl;
	cout << "(size is " << ed.getSize() << ")" << endl;
    } catch (Rosegarden::Element::BadFormat &bf) {
	cout << "Caught BadFormat: reason is:" << endl << bf.reason << endl;
    }

    cout << "Testing relative speeds of Element and Event..." << endl;
    int i;
    long j;
    char b[20];
    strcpy(b, "test");

    Rosegarden::Element e0("sys", "note");
    st = times(&spare);
    for (i = 0; i < 10000; ++i) {
	if (i%4==0) sprintf(b+4, "%d", i);
	e0.setInt(b, i);
    }
    et = times(&spare);
    cout << "Element: 10000 setInts: " << (et-st)*10 << "ms\n";
    st = times(&spare);
    j = 0;
    for (i = 0; i < 10000; ++i) {
	if (i%4==0) sprintf(b+4, "%d", i);
	j += e0.getInt(b);
    }
    et = times(&spare);
    cout << "Element: 10000 getInts: " << (et-st)*10 << "ms\n";
    st = times(&spare);
    for (i = 0; i < 10; ++i) {
	Rosegarden::Element e01(e0);
	(void)e01.getInt(b);
    }
    et = times(&spare);
    cout << "Element: 10 copy ctors of big element: " << (et-st)*10 << "ms\n";

    Event e1("sys", "note");
    st = times(&spare);
    for (i = 0; i < 10000; ++i) {
	if (i%4==0) sprintf(b+4, "%d", i);
	e1.set<Int>(b, i);
    }
    et = times(&spare);
    cout << "Event: 10000 setInts: " << (et-st)*10 << "ms\n";
    st = times(&spare);
    j = 0;
    for (i = 0; i < 10000; ++i) {
	if (i%4==0) sprintf(b+4, "%d", i);
	j += e1.get<Int>(b);
    }
    et = times(&spare);
    cout << "Event: 10000 getInts: " << (et-st)*10 << "ms\n";
    st = times(&spare);
    for (i = 0; i < 100; ++i) {
	Event e11(e1);
	(void)e11.get<Int>(b);
    }
    et = times(&spare);
    cout << "Event: 100 copy ctors of big element: " << (et-st)*10 << "ms\n";

    cout << "Testing RobustIteratorVector of CloningWrappers..." << endl;

    typedef CloningWrapper<A *> AWrapper;

    RobustIteratorVector<AWrapper> ril;

    {
	vector<AWrapper> v;

	A a(3);
        v.push_back(AWrapper(&a));
	a = A(4);
	v.push_back(AWrapper(&a));
	B b(5,6);
	v.push_back(AWrapper(&b));
	b = B(7,8);
	v.push_back(AWrapper(&b));
 
	cout << "Showing list (" << v.size() << " elements):" << endl;
	for_each(v.begin(), v.end(), ShowWrappedObject<A>());
	cout << "Done" << endl;

	b = B(9,10);
        v[2] = AWrapper(&b); // was B(5,6)
	v[3]->m_i = 200; // was B(7,8), should now be B(200,8)

//	vector<AWrapper>::iterator vi(v.begin());
//	++vi; ++vi;
//	(**vi).show();
	//cout << "deleting item 0" << endl;
	//v.erase(v.begin());
	//if (vi == v.end()) {
	 //   cout << "vi is now at the end" << endl;
	//} else {
	 //   AWrapper aw =*vi;
	 //   (*aw).show();
	//}

	cout << "Showing list (" << v.size() << " elements):" << endl;
	for_each(v.begin(), v.end(), ShowWrappedObject<A>());
	cout << "Done" << endl;

	FastVector<AWrapper> ril0(v.begin(), v.end());
	FastVector<AWrapper> ril1 = FastVector<AWrapper>(v.begin(), v.end());
	ril = RobustIteratorVector<AWrapper>(v.begin(), v.end());
    }

    RobustIteratorVector<AWrapper>::iterator rili(ril.begin());

    rili++;
    ++rili;

    cout << "iterator is at (should be third element): ";
  
    (*rili)->show();

    cout << "in list (" << ril.size() << " elements):" << endl;
    for_each(ril.begin(), ril.end(), ShowWrappedObject<A>());
    A a(1);

    while (ril.begin() != ril.end()) {

	cout << "deleting last object (of " << ril.size() << ")... ";
	ril.erase(--ril.end());
	cout << "done" << endl;

	cout << "iterator is now at ";
	
	if (rili == ril.end()) {
	    cout << "end of list" << endl;
	} else {
	    (*rili)->show();
	    cout << "in" << endl;
	    for_each(ril.begin(), ril.end(), ShowWrappedObject<A>());
	}

	if (rand() < RAND_MAX/2) continue;
	cout << "inserting at start..." << endl;
	ril.insert(ril.begin(), AWrapper(&a));
	cout << "done" << endl;

	cout << "iterator is now at ";
	
	if (rili == ril.end()) {
	    cout << "end of list" << endl;
	} else {
	    (*rili)->show();
	    cout << "in" << endl;
	    for_each(ril.begin(), ril.end(), ShowWrappedObject<A>());
	}
    }

    return 0;
};

