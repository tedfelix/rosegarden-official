// -*- c-file-style:  "bsd" -*-

// This does some rather shoddy tests on a small selection of core classes.

#include "Event.h"
#include "Track.h"
#include "NotationTypes.h"

#include <cstdio>

#include <sys/times.h>
#include <iostream>

using namespace std;
using namespace Rosegarden;

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

#if 0
        cout << "Testing speed of Event..." << endl;
        int i;
        long j;
        char b[20];
        strcpy(b, "test");

        Event e1("note");
        int gsCount = 20000;

        st = times(&spare);
        for (i = 0; i < gsCount; ++i) {
                if (i%4==0) sprintf(b+4, "%d", i);
                e1.set<Int>(b, i);
        }
        et = times(&spare);
        cout << "Event: " << gsCount << " setInts: " << (et-st)*10 << "ms\n";

        st = times(&spare);
        j = 0;
        for (i = 0; i < gsCount; ++i) {
                if (i%4==0) sprintf(b+4, "%d", i);
                j += e1.get<Int>(b);
        }
        et = times(&spare);
        cout << "Event: " << gsCount << " getInts: " << (et-st)*10 << "ms\n";
        
        st = times(&spare);
        for (i = 0; i < 100; ++i) {
                Event e11(e1);
                (void)e11.get<Int>(b);
        }
        et = times(&spare);
        cout << "Event: 100 copy ctors of " << e1.getStorageSize() << "-byte element: "
             << (et-st)*10 << "ms\n";

        gsCount = 100000;

        st = times(&spare);
        for (i = 0; i < gsCount; ++i) {
                if (i%4==0) sprintf(b+4, "%ds", i);
                e1.set<String>(b, b);
        }
        et = times(&spare);
        cout << "Event: " << gsCount << " setStrings: " << (et-st)*10 << "ms\n";
        
        st = times(&spare);
        j = 0;
        for (i = 0; i < gsCount; ++i) {
                if (i%4==0) sprintf(b+4, "%ds", i);
                j += e1.get<String>(b).size();
        }
        et = times(&spare);
        cout << "Event: " << gsCount << " getStrings: " << (et-st)*10 << "ms\n";
        
        st = times(&spare);
        for (i = 0; i < 100; ++i) {
                Event e11(e1);
                (void)e11.get<String>(b);
        }
        et = times(&spare);
        cout << "Event: 100 copy ctors of " << e1.getStorageSize() << "-byte element: "
             << (et-st)*10 << "ms\n";

        return 0;
#else
        cout << "Skipping test speed of Event\n";
#endif

#ifdef NOT_DEFINED
        cout << "Testing track shrinking\n";
        
        Track track(5, 0);
        unsigned int nbBars = track.getNbBars();

        cout << "Track nbBars : " << nbBars << endl;
        if (nbBars != 5) {
                cerr << "%%%ERROR : track nbBars should be 5\n";
        }

        Track::iterator iter = track.end();
        --iter;
        cout << "Last track el. time : " << (*iter)->getAbsoluteTime() << endl;

        cout << "Shrinking track to 3 bars : \n";
        track.setNbBars(3);
        nbBars = track.getNbBars();

        cout << "Track new nbBars : " << nbBars << endl;
        if (nbBars != 3) {
                cerr << "%%%ERROR : track new nbBars should be 3\n";
        }
#endif

        cout << "Testing duration-list stuff\n";

        cout << "2/4..." << endl;
        TimeSignature ts(2,4);
        DurationList dlist;
        ts.getDurationListForInterval
                (dlist, 1206,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        int acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "6/8..." << endl;
        ts = TimeSignature(6,8);
        dlist = DurationList();
        ts.getDurationListForInterval
                (dlist, 1206,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "3/4..." << endl;
        ts = TimeSignature(3,4);
        dlist = DurationList();
        ts.getDurationListForInterval
                (dlist, 1206,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "4/4..." << endl;
        ts = TimeSignature(4,4);
        dlist = DurationList();
        ts.getDurationListForInterval
                (dlist, 1206,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "3/8..." << endl;
        ts = TimeSignature(3,8);
        dlist = DurationList();
        ts.getDurationListForInterval
                (dlist, 1206,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "Testing Track::expandIntoGroup() - expanding 384 -> 2*192\n";

        Track t;

        Event *ev = new Event("note");
        ev->setAbsoluteTime(0);
        ev->setDuration(384);
        t.insert(ev);

        t.expandIntoGroup(t.begin(), 384/2);

        for(Track::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << endl;
        }

        Track::iterator half2 = t.begin(); ++half2;
        
        cout << "Expanding 192 -> (48 + 144) : \n";

        t.expandIntoGroup(t.begin(), 48);

        for(Track::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << endl;
        }
        
        cout << "Expanding 192 -> (144 + 48) : \n";

        t.expandIntoGroup(half2, 144);

        for(Track::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << endl;
        }
};

