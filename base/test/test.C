// -*- c-file-style:  "bsd" -*-

// This does some rather shoddy tests on the Event class.

#include "Event.h"

#include <cstdio>

#include <sys/times.h>

using namespace std;

int main(int argc, char **argv)
{
        clock_t st, et;
        struct tms spare;

        cout << "Testing Event..." << endl
             << "sizeof Event : " << sizeof(Event) << endl;

        Event e("note");
        e.set<Int>("duration", 20);
        cout << "duration is " << e.get<Int>("duration") << endl;

        e.set<Bool>("someBoolProp", true);
        e.set<String>("someStringProp", "foobar");

        cout << "sizeof event after some properties set : "
             << sizeof e << endl;

        cout << "Testing debug dump : " << endl;
        e.dump(cout);
        cout << endl << "dump finished" << endl;

        try {
                cout << "duration is " << e.get<String>("duration") << endl;
        } catch (Event::BadType bt) {
                cout << "Correctly caught BadType when trying to get<String> of duration" << endl;
        }

        string s;
    
        if (!e.get<String>("duration", s)) {
                cout << "Correctly got error when trying to get<String> of duration" << endl;
        } else {
                cerr << "ERROR AT " << __LINE__ << endl;
        }
    
        try {
                cout << "dummy prop is " << e.get<String>("nonexistentprop") << endl;
        } catch (Event::NoData bt) {
                cout << "Correctly caught NoData when trying to get non existent property" << endl;
        }

        if (!e.get<String>("nonexistentprop", s)) {
                cout << "Correctly got error when trying to get<String> of non existent property" << endl;
        } else {
                cerr << "ERROR AT " << __LINE__ << endl;
        }


        e.setFromString<Int>("duration", "30");
        cout << "duration is " << e.get<Int>("duration") << endl;

        e.setFromString<String>("annotation", "This is my house");
        cout << "annotation is " << e.get<String>("annotation") << endl;

        long durationVal;
        if (e.get<Int>("duration", durationVal))
                cout << "duration is " << durationVal << endl;
        else
                cerr << "ERROR AT " << __LINE__ << endl;

        if (e.get<String>("annotation", s))
                cout << "annotation is " << s << endl;
        else
                cerr << "ERROR AT " << __LINE__ << endl;


        cout << "Testing speed of Event..." << endl;
        int i;
        long j;
        char b[20];
        strcpy(b, "test");

        Event e1("note");
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
        cout << "Event: 100 copy ctors of big element: "
             << (et-st)*10 << "ms\n";

        return 0;
};

