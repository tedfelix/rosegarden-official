/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
// -*- c-file-style:  "bsd" -*-

#include "base/Event.h"

#include <QtGlobal>
#include <QDebug>
#include <QTest>

#include <string>

#include <sys/times.h>

using namespace Rosegarden;

/// Miscellaneous unit tests.
class TestMisc : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testEvent();
    void testEventPerformance();
};

void TestMisc::testEvent() try
{
    qDebug() << "Testing Event...";

    Event e("note", 0);
    static const PropertyName DURATION_PROPERTY = "duration";
    e.set<Int>(DURATION_PROPERTY, 20);
    QCOMPARE(e.get<Int>(DURATION_PROPERTY), 20l);

    static const PropertyName SOME_BOOL_PROPERTY = "someBoolProp";
    static const PropertyName SOME_STRING_PROPERTY = "someStringProp";
    e.set<Bool>(SOME_BOOL_PROPERTY, true);
    e.set<String>(SOME_STRING_PROPERTY, "foobar");

    std::string s;

    bool thrown = false;

    try {
        s = e.get<String>(DURATION_PROPERTY);

        qDebug() << "BadType was not thrown.";
        QVERIFY(false);
    } catch (Event::BadType &bt) {
        thrown = true;
    }

    QVERIFY(thrown);

    QVERIFY(!e.get<String>(DURATION_PROPERTY, s));

    static const PropertyName NONEXISTENT_PROPERTY = "nonexistentprop";

    thrown = false;

    try {
        s = e.get<String>(NONEXISTENT_PROPERTY);

        qDebug() << "NoData was not thrown.";
        QVERIFY(false);
    } catch (Event::NoData &bt) {
        thrown = true;
    }

    QVERIFY(thrown);

    QVERIFY(!e.get<String>(NONEXISTENT_PROPERTY, s));

    e.set<Int>(DURATION_PROPERTY, 30);
    long duration;
    QVERIFY(e.get<Int>(DURATION_PROPERTY, duration));
    QCOMPARE(duration, 30l);

    static const PropertyName ANNOTATION_PROPERTY = "annotation";
    e.set<String>(ANNOTATION_PROPERTY, "This is my house");
    QVERIFY(e.get<String>(ANNOTATION_PROPERTY, s));
    QCOMPARE(s.c_str(), "This is my house");

    qDebug() << "Testing persistence & setMaybe...";

    static const PropertyName SOME_INT_PROPERTY = "someIntProp";
    e.setMaybe<Int>(SOME_INT_PROPERTY, 1);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 1l);

    e.set<Int>(SOME_INT_PROPERTY, 2, false);
    e.setMaybe<Int>(SOME_INT_PROPERTY, 3);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 3l);

    e.set<Int>(SOME_INT_PROPERTY, 4);
    e.setMaybe<Int>(SOME_INT_PROPERTY, 5);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 4l);

    qDebug() << "Testing debug dump : ";
    // ??? Will not compile.
    // testmisc.cpp:113:11: error: cannot bind non-const lvalue reference of
    //                             type ‘QDebug&’ to an rvalue of type ‘QDebug’
    //     Sure enough, this doesn't work either:
    //         QDebug &qd = qDebug();
    //     But why does this work in the app?  What is different there?
    //qDebug() << e;
    //QDebug(QtDebugMsg) << e;
    // This works fine, of course.  It's the temporaries that it doesn't
    // like.  But they are ok in the app.  Hmmm.
    QDebug qd(QtDebugMsg);
    qd << e;
    qDebug() << "dump finished";

} catch(...) {
    qDebug() << "Unexpected exception caught";
    QVERIFY(false);
}

void TestMisc::testEventPerformance()
{
    // Performance testing probably belongs in the code itself.

    qDebug() << "Testing speed of Event...";

    constexpr int NAME_COUNT = 500;

    char b[20];
    strcpy(b, "test");

    int i;
    PropertyName names[NAME_COUNT];
    for (i = 0; i < NAME_COUNT; ++i) {
        sprintf(b+4, "%d", i);
        names[i] = b;
    }

    Event e1("note", 0);
    int gsCount = 200000;

    struct tms spare;
    clock_t st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
            e1.set<Int>(names[i % NAME_COUNT], i);
    }
    clock_t et = times(&spare);
    qDebug() << "Event: " << gsCount << " setInts: " << (et-st)*10 << "ms";

    st = times(&spare);
    long j = 0;
    for (i = 0; i < gsCount; ++i) {
            if (i%4==0) sprintf(b+4, "%d", i);
            j += e1.get<Int>(names[i % NAME_COUNT]);
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " getInts: " << (et-st)*10 << "ms (result: " << j << ")";

    st = times(&spare);
    for (i = 0; i < 1000; ++i) {
            Event e11(e1);
            (void)e11.get<Int>(names[i % NAME_COUNT]);
    }
    et = times(&spare);
    qDebug() << "Event: 1000 copy ctors of " << e1.getStorageSize() << "-byte element: "
         << (et-st)*10 << "ms";

    for (i = 0; i < NAME_COUNT; ++i) {
        sprintf(b+4, "%ds", i);
        names[i] = b;
    }

    st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
            e1.set<String>(names[i % NAME_COUNT], b);
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " setStrings: " << (et-st)*10 << "ms";

    st = times(&spare);
    j = 0;
    for (i = 0; i < gsCount; ++i) {
            if (i%4==0) sprintf(b+4, "%ds", i);
            j += e1.get<String>(names[i % NAME_COUNT]).size();
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " getStrings: " << (et-st)*10 << "ms (result: " << j << ")";

    st = times(&spare);
    for (i = 0; i < 1000; ++i) {
            Event e11(e1);
            (void)e11.get<String>(names[i % NAME_COUNT]);
    }
    et = times(&spare);
    qDebug() << "Event: 1000 copy ctors of " << e1.getStorageSize() << "-byte element: "
         << (et-st)*10 << "ms";

    st = times(&spare);
    for (i = 0; i < 1000; ++i) {
            Event e11(e1);
            (void)e11.get<String>(names[i % NAME_COUNT]);
            (void)e11.set<String>(names[i % NAME_COUNT], "blah");
    }
    et = times(&spare);
    qDebug() << "Event: 1000 copy ctors plus set<String> of " << e1.getStorageSize() << "-byte element: "
         << (et-st)*10 << "ms";

    // ??? Is this correct?
    constexpr short MIN_SUBORDERING = 0;

    st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
            Event e21("dummy", i, 0, MIN_SUBORDERING);
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " event ctors alone: "
         << (et-st)*10 << "ms";

    st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
        std::string s0("dummy");
        std::string s1 = s0;
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " string ctors+assignents: "
         << (et-st)*10 << "ms";

    st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
            Event e21("dummy", i, 0, MIN_SUBORDERING);
            (void)e21.getAbsoluteTime();
            (void)e21.getDuration();
            (void)e21.getSubOrdering();
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " event ctors plus getAbsTime/Duration/SubOrdering: "
         << (et-st)*10 << "ms";

    st = times(&spare);
    for (i = 0; i < gsCount; ++i) {
            Event e21("dummy", i, 0, MIN_SUBORDERING);
            (void)e21.getAbsoluteTime();
            (void)e21.getDuration();
            (void)e21.getSubOrdering();
            e21.set<Int>(names[0], 40);
            (void)e21.get<Int>(names[0]);
    }
    et = times(&spare);
    qDebug() << "Event: " << gsCount << " event ctors plus one get/set and getAbsTime/Duration/SubOrdering: "
         << (et-st)*10 << "ms";
}

#if 0

#include "base/Segment.h"
#include "base/Composition.h"

#define TEST_NOTATION_TYPES 1
#define TEST_SPEED 1

#ifdef TEST_NOTATION_TYPES
#include "base/NotationTypes.h"
#include "base/SegmentNotationHelper.h"
#include "base/SegmentPerformanceHelper.h"
#endif

#include "base/MidiTypes.h"

#include <cstdio>

#include <iostream>

using namespace std;
using namespace Rosegarden;



int main(int argc, char **argv)
{

#ifdef NOT_DEFINED
        cout << "Testing segment shrinking\n";
        
        Segment segment(5, 0);
        unsigned int nbBars = segment.getNbBars();

        cout << "Segment nbBars : " << nbBars << endl;
        if (nbBars != 5) {
                cerr << "%%%ERROR : segment nbBars should be 5\n";
        }

        Segment::iterator iter = segment.end();
        --iter;
        cout << "Last segment el. time : " << (*iter)->getAbsoluteTime() << endl;

        cout << "Shrinking segment to 3 bars : \n";
        segment.setNbBars(3);
        nbBars = segment.getNbBars();

        cout << "Segment new nbBars : " << nbBars << endl;
        if (nbBars != 3) {
                cerr << "%%%ERROR : segment new nbBars should be 3\n";
        }
#endif // NOT_DEFINED

#ifdef TEST_NOTATION_TYPES
        cout << "Testing duration-list stuff\n";

        cout << "2/4..." << endl;
        TimeSignature ts(2,4);
        DurationList dlist;
        ts.getDurationListForInterval
                (dlist, 1209,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        int acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;



        cout << "4/4 96/96..." << endl;
        ts = TimeSignature(4,4);
        dlist = DurationList();
        ts.getDurationListForInterval(dlist, 96, 96);
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;
        


        cout << "6/8..." << endl;
        ts = TimeSignature(6,8);
        dlist = DurationList();
        ts.getDurationListForInterval
                (dlist, 1209,
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
                (dlist, 1209,
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
                (dlist, 1209,
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
                (dlist, 1209,
                 ts.getBarDuration() - Note(Note::Semiquaver, true).getDuration());
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "4/4 wacky placement..." << endl;
        ts = TimeSignature(4,4);
        dlist = DurationList();
        ts.getDurationListForInterval(dlist, 160, 1280);
        acc = 0;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
                cout << "duration: " << *i << endl;
                acc += *i;
        }
        cout << "total: " << acc << " (on bar duration of " << ts.getBarDuration() << ")" << endl;

        cout << "Testing Segment::splitIntoTie() - splitting 384 -> 2*192\n";

        Composition c;
        Segment *ht = new Segment();
        c.addSegment(ht);
        Segment &t(*ht);
        SegmentNotationHelper nh(t);
        SegmentPerformanceHelper ph(t);

        Event *ev = new Event("note", 0, 384);
        ev->set<Int>("pitch", 60);
        t.insert(ev);

        Segment::iterator sb(t.begin());
        nh.splitIntoTie(sb, 384/2);

        for(Segment::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << endl;
        }

        Segment::iterator half2 = t.begin(); ++half2;
        
        cout << "Splitting 192 -> (48 + 144) : \n";

        sb = t.begin();
        nh.splitIntoTie(sb, 48);

        for(Segment::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << endl;
        }
        
        cout << "Splitting 192 -> (144 + 48) : \n";

        nh.splitIntoTie(half2, 144);


        for(Segment::iterator i = t.begin(); i != t.end(); ++i) {
                cout << "Event at " << (*i)->getAbsoluteTime()
                     << " - duration : " << (*i)->getDuration()
                     << " - performance duration : " <<
                    ph.getSoundingDuration(i) << endl;

                cout << endl;
                qDebug() << (*i);
                cout << endl;
        }

        nh.autoBeam(t.begin(), t.end(), "beamed");

#endif // TEST_NOTATION_TYPES
};

#endif

#if 0
        basic_string<wchar_t> widestring(L"This is a test");
        widestring += L" of wide character strings";
        for (size_t i = 0; i < widestring.length(); ++i) {
            if (widestring[i] == L'w' ||
                widestring[i] == L'c') {
                widestring[i] = toupper(widestring[i]);
            }
        }
        cout << "Testing wide string: string value is \"" << widestring << "\""
             << endl;
        cout << "String's length is " << widestring.length() << endl;
        cout << "and storage space is "
             << (widestring.length() * sizeof(widestring[0]))
             << endl;
        cout << "Characters are: ";
        for (size_t i = 0; i < widestring.length(); ++i) {
            cout << widestring[i];
            if (i < widestring.length()-1) cout << " ";
            else cout << endl;
        }
#endif

#if 0
// Some attempts at reproducing the func-template-within-template problem
//
enum FooType {A, B, C};

class Foo
{
public:
    template<FooType T> void func();
};

template<class T>
void Foo::func()
{
    // dummy code
    T j = 0;
    for(T i = 0; i < 100; ++i) j += i;
}

//template void Foo::func<int>();

template <class R>
class FooR
{
public:
    void rfunc();
};

template<class R>
void FooR<R>::rfunc()
{
    // this won't compile
    Foo* foo;
    foo->func<A>();
}

void templateTest()
{
    Foo foo;
    foo.func<A>();

//     FooR<float> foor;
//     foor.rfunc();
}


template <class Element, class Container>
class GenericSet // abstract base
{
public:
    typedef typename Container::iterator Iterator;

    /// Return true if this element, known to test() true, is a set member
    virtual bool sample(const Iterator &i);
};


template <class Element, class Container>
bool
GenericSet<Element, Container>::sample(const Iterator &i)
{
    Event *e;
    long p = e->get<Int>("blah");
}

#endif

QTEST_MAIN(TestMisc)

#include "testmisc.moc"
