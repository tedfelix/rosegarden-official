/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
//

#include "base/Composition.h"
#include <QTest>
#include <sstream>

using namespace Rosegarden;

// Tests for ReferenceSegment
class TestReferenceSegment : public QObject,
                             public Composition // ReferenceSegment is protected

{
    Q_OBJECT

private Q_SLOTS:
    void testEventType();
    void testInsert();
    void testErase();

private:
    Composition::ReferenceSegment* setup_rs();
    std::string toString(const Composition::ReferenceSegment& rs);
};

/**
 * test EventType
 */
void TestReferenceSegment::testEventType()
{
    ReferenceSegment rs(TempoEventType);
    std::string et = rs.getEventType();
    QCOMPARE(et, TempoEventType);
}

/**
 * test insert
 */
void TestReferenceSegment::testInsert()
{
    ReferenceSegment rs(TempoEventType);

    timeT ttime1 = 0;
    timeT ttime2 = 1000;
    tempoT tempo1 = 10000;
    tempoT tempo2 = 20000;
    tempoT tempo3 = 40000;
    Event *tempoEvent1 = new Event(TempoEventType, ttime1);
    tempoEvent1->set<Int>(TempoProperty, tempo1);
    rs.insertEvent(tempoEvent1);
    QCOMPARE(rs.size(), 1l);

    Event *badEvent = new Event("xxx", 100);
    try {
        rs.insertEvent(badEvent);
    } catch (const Event::BadType& b) {
        std::string msg = b.getMessage();
        QVERIFY(msg == "Bad type for event in ReferenceSegment (expected tempo, found xxx)");
    }

    Event *tempoEvent2 = new Event(TempoEventType, ttime2);
    tempoEvent2->set<Int>(TempoProperty, tempo2);
    rs.insertEvent(tempoEvent2);
    QCOMPARE(rs.size(), 2l);

    Event *tempoEvent3 = new Event(TempoEventType, ttime1);
    tempoEvent3->set<Int>(TempoProperty, tempo3);
    rs.insertEvent(tempoEvent3);
    QCOMPARE(rs.size(), 2l); // only one event per time
}

void TestReferenceSegment::testErase()
{
    ReferenceSegment* rs = setup_rs();

    rs->erase(rs->begin());

    QCOMPARE(rs->size(), 4l);
    std::string rss = toString(*rs);
    QVERIFY(rss == "test:10/20/50/100/");

    // get the second event
    auto it = rs->begin();
    ++it;
    Event* e = *it;
    rs->eraseEvent(e);

    QCOMPARE(rs->size(), 3l);
    rss = toString(*rs);
    QVERIFY(rss == "test:10/50/100/");

    auto it1 = rs->findNearestTime(60);
    rs->erase(it1);
    QCOMPARE(rs->size(), 2l);
    rss = toString(*rs);
    QVERIFY(rss == "test:10/100/");

    rs->clear();
    QCOMPARE(rs->size(), 0l);
    rss = toString(*rs);
    QVERIFY(rss == "test:");
}

Composition::ReferenceSegment* TestReferenceSegment::setup_rs()
{
    // setup a ReferenceSegment with some data
    ReferenceSegment* rs = new ReferenceSegment("test");

    Event* e1 = new Event("test", 0);
    e1->set<Int>("test_property", 10);
    rs->insertEvent(e1);

    Event* e2 = new Event("test", 10);
    e2->set<Int>("test_property", 1);
    rs->insertEvent(e2);

    Event* e3 = new Event("test", 20);
    e3->set<Int>("test_property", 11);
    rs->insertEvent(e3);

    Event* e4 = new Event("test", 50);
    e4->set<Int>("test_property", 45);
    rs->insertEvent(e4);

    Event* e5 = new Event("test", 100);
    e5->set<Int>("test_property", 78);
    rs->insertEvent(e5);

    return rs;
}

std::string TestReferenceSegment::toString(const Composition::ReferenceSegment& rs)
{
    std::ostringstream oss;
    std::string et = rs.getEventType();
    oss << et << ":";
    for (auto it : rs)
        {
            Event& e = (*it);
            timeT t = e.getAbsoluteTime();
            oss << t << "/";
        }
    return oss.str();
}

QTEST_MAIN(TestReferenceSegment)

#include "reference_segment.moc"
