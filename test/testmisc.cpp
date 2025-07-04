/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
// -*- c-file-style:  "bsd" -*-

#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/SegmentNotationHelper.h"
#include "base/SegmentPerformanceHelper.h"

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
    void testNotationTypes();
};

void TestMisc::testEvent() try
{
    qDebug() << "Testing Event...";

    Event e("note", 0);
    static const PropertyName DURATION_PROPERTY("duration");
    e.set<Int>(DURATION_PROPERTY, 20);
    QCOMPARE(e.get<Int>(DURATION_PROPERTY), 20l);

    static const PropertyName SOME_BOOL_PROPERTY("someBoolProp");
    static const PropertyName SOME_STRING_PROPERTY("someStringProp");
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

    static const PropertyName NONEXISTENT_PROPERTY("nonexistentprop");

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

    static const PropertyName ANNOTATION_PROPERTY("annotation");
    e.set<String>(ANNOTATION_PROPERTY, "This is my house");
    QVERIFY(e.get<String>(ANNOTATION_PROPERTY, s));
    QCOMPARE(s.c_str(), "This is my house");

    qDebug() << "Testing persistence & setMaybe...";

    static const PropertyName SOME_INT_PROPERTY("someIntProp");
    e.setMaybe<Int>(SOME_INT_PROPERTY, 1);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 1l);

    e.set<Int>(SOME_INT_PROPERTY, 2, false);
    e.setMaybe<Int>(SOME_INT_PROPERTY, 3);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 3l);

    e.set<Int>(SOME_INT_PROPERTY, 4);
    e.setMaybe<Int>(SOME_INT_PROPERTY, 5);
    QCOMPARE(e.get<Int>(SOME_INT_PROPERTY), 4l);

    qDebug() << "Testing debug dump : ";
    qDebug() << e;
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

void TestMisc::testNotationTypes() {
    qDebug() << "Testing duration-list stuff";

    Note note(Note::Semiquaver, 1);
    QCOMPARE((int)note.getNoteType(), (int)Note::Semiquaver);
    QCOMPARE(note.getDots(), 1);

    qDebug() << "2/4...";
    TimeSignature ts(2,4);
    QCOMPARE(ts.getBarDuration(), 1920l);

    DurationList dlist;
    ts.getDurationListForInterval(
            dlist, 1209,
            ts.getBarDuration() - Note(Note::Semiquaver, 1).getDuration());

    DurationList::const_iterator durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 480);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 9);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "4/4 96/96...";
    ts = TimeSignature(4,4);
    QCOMPARE(ts.getBarDuration(), 3840l);

    dlist = DurationList();
    ts.getDurationListForInterval(dlist, 96, 96);

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 96);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "6/8...";
    ts = TimeSignature(6,8);
    QCOMPARE(ts.getBarDuration(), 2880l);

    dlist = DurationList();
    ts.getDurationListForInterval
            (dlist, 1209,
             ts.getBarDuration() - Note(Note::Semiquaver, 1).getDuration());

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 480);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 9);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "3/4...";
    ts = TimeSignature(3,4);
    QCOMPARE(ts.getBarDuration(), 2880l);

    dlist = DurationList();
    ts.getDurationListForInterval
            (dlist, 1209,
             ts.getBarDuration() - Note(Note::Semiquaver, 1).getDuration());

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 480);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 9);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "4/4...";
    ts = TimeSignature(4,4);
    QCOMPARE(ts.getBarDuration(), 3840l);

    dlist = DurationList();
    ts.getDurationListForInterval
            (dlist, 1209,
             ts.getBarDuration() - Note(Note::Semiquaver, 1).getDuration());

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 480);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 9);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "3/8...";
    ts = TimeSignature(3,8);
    QCOMPARE(ts.getBarDuration(), 1440l);

    dlist = DurationList();
    ts.getDurationListForInterval
            (dlist, 1209,
             ts.getBarDuration() - Note(Note::Semiquaver, 1).getDuration());

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 480);
    QCOMPARE(*durationIter++, 240);
    QCOMPARE(*durationIter++, 120);
    QCOMPARE(*durationIter++, 9);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "4/4 wacky placement...";
    ts = TimeSignature(4,4);

    dlist = DurationList();
    ts.getDurationListForInterval(dlist, 160, 1280);

    durationIter = dlist.begin();
    QCOMPARE(*durationIter++, 160);
    QVERIFY(durationIter == dlist.end());


    qDebug() << "Testing Segment::splitIntoTie() - splitting 384 -> 2*192";

    Composition c;
    Segment *ht = new Segment();
    c.addSegment(ht);
    Segment &t(*ht);
    SegmentNotationHelper nh(t);
    SegmentPerformanceHelper ph(t);

    Event *ev = new Event("note", 0, 384);
    ev->set<Int>(PropertyName("pitch"), 60);
    t.insert(ev);

    Segment::iterator sb(t.begin());
    nh.splitIntoTie(sb, 384/2);

    Segment::const_iterator eventIter = t.cbegin();
    QCOMPARE((*eventIter)->getAbsoluteTime(), 0l);
    QCOMPARE((*eventIter)->getDuration(), 192l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 192l);
    QCOMPARE((*eventIter)->getDuration(), 192l);
    ++eventIter;
    QVERIFY(eventIter == t.cend());


    qDebug() << "Splitting 192 -> (48 + 144)";

    Segment::iterator half2 = t.begin();
    ++half2;

    sb = t.begin();
    nh.splitIntoTie(sb, 48);

    eventIter = t.cbegin();
    QCOMPARE((*eventIter)->getAbsoluteTime(), 0l);
    QCOMPARE((*eventIter)->getDuration(), 48l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 48l);
    QCOMPARE((*eventIter)->getDuration(), 144l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 192l);
    QCOMPARE((*eventIter)->getDuration(), 192l);
    ++eventIter;
    QVERIFY(eventIter == t.cend());


    qDebug() << "Splitting 192 -> (144 + 48)";

    nh.splitIntoTie(half2, 144);

    eventIter = t.cbegin();
    QCOMPARE((*eventIter)->getAbsoluteTime(), 0l);
    QCOMPARE((*eventIter)->getDuration(), 48l);
    QCOMPARE(ph.getSoundingDuration(eventIter), 384l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 48l);
    QCOMPARE((*eventIter)->getDuration(), 144l);
    QCOMPARE(ph.getSoundingDuration(eventIter), 0l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 192l);
    QCOMPARE((*eventIter)->getDuration(), 144l);
    QCOMPARE(ph.getSoundingDuration(eventIter), 0l);
    ++eventIter;
    QCOMPARE((*eventIter)->getAbsoluteTime(), 336l);
    QCOMPARE((*eventIter)->getDuration(), 48l);
    QCOMPARE(ph.getSoundingDuration(eventIter), 0l);
    ++eventIter;
    QVERIFY(eventIter == t.cend());

    nh.autoBeam(t.begin(), t.end(), "beamed");

    // ??? Do we need to test something here?
}

QTEST_MAIN(TestMisc)

#include "testmisc.moc"
