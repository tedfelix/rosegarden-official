/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/RealTime.h"

#include <QTest>

using namespace Rosegarden;

/// Unit test for RealTime
class TestRealTime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void test();
    void testFrameConversion();
};

void TestRealTime::test()
{
    //qDebug() << "Testing!\n";

    RealTime rt;

    QCOMPARE(rt.sec, 0);
    QCOMPARE(rt.nsec, 0);
    QCOMPARE(rt.toSeconds(), 0.0);
    QCOMPARE(rt, RealTime::zeroTime);
    QCOMPARE(rt.usec(), 0);
    QCOMPARE(rt.msec(), 0);

    RealTime rt2(10, 20);

    QCOMPARE(rt2.sec, 10);
    QCOMPARE(rt2.nsec, 20);
    QCOMPARE(rt2.toSeconds(), 10.00000002);
    QCOMPARE(rt2.usec(), 0);
    QCOMPARE(rt2.msec(), 0);

    rt = RealTime::fromSeconds(1.5);
    QCOMPARE(rt.sec, 1);
    QCOMPARE(rt.nsec, 500000000);
    QCOMPARE(rt.toSeconds(), 1.5);
    QCOMPARE(rt.usec(), 500000);
    QCOMPARE(rt.msec(), 500);

    rt = RealTime::fromSeconds(-1.5);
    QCOMPARE(rt.sec, -1);
    QCOMPARE(rt.nsec, -500000000);
    QCOMPARE(rt.toSeconds(), -1.5);

    rt = RealTime::fromSeconds(40897347.394);
    // A little rounding error is ok.
    QCOMPARE(rt.nsec, 394000001);
    QCOMPARE(rt.toString().c_str(), "40897347.394000001");
    QCOMPARE(rt.toText().c_str(), "11360:22:27.394");

    rt = RealTime::fromMilliseconds(39827393);
    QCOMPARE(rt.sec, 39827);
    QCOMPARE(rt.nsec, 393000000);

    // Math...
    rt = RealTime(45, 783945132);
    rt2 = RealTime(2362, 502457922);
    rt2 = rt + rt2;
    QCOMPARE(rt2.sec, 2408);
    QCOMPARE(rt2.nsec, 286403054);

    rt2 = RealTime(45, 502457922) - RealTime(2362, 783945132);
    QCOMPARE(rt2.sec, -2317);
    QCOMPARE(rt2.nsec, -281487210);

    rt2 = -RealTime(45, 502457922);
    QCOMPARE(rt2.sec, -45);
    QCOMPARE(rt2.nsec, -502457922);

    rt2 = RealTime(23, 500000000) * 2;
    QCOMPARE(rt2.sec, 47);
    QCOMPARE(rt2.nsec, 0);

    rt2 = RealTime(2, 12) / 4;
    QCOMPARE(rt2.sec, 0);
    QCOMPARE(rt2.nsec, 500000003);

    double d = RealTime(5, 0) / RealTime(2, 0);
    QCOMPARE(d, 2.5);

    // Comparison...

    QVERIFY(RealTime(1, 123000000) < RealTime(2, 456000000));
    QVERIFY(RealTime(1, 123000000) <= RealTime(2, 456000000));
    QVERIFY(RealTime(2, 456000000) > RealTime(2, 123000000));
    QVERIFY(RealTime(2, 456000000) >= RealTime(2, 123000000));
    QVERIFY(RealTime(2, 456000000) != RealTime(2, 123000000));
    QVERIFY(RealTime(327834, 120398123) == RealTime(327834, 120398123));
    QVERIFY(RealTime(327834, 120398123) >= RealTime(327834, 120398123));
    QVERIFY(RealTime(327834, 120398123) <= RealTime(327834, 120398123));

}

namespace
{
    bool checkFrameConversion(int frame, int rate)
    {
        RealTime rt = RealTime::frame2RealTime(frame, rate);
        int testframe = RealTime::realTime2Frame(rt, rate);

        return (testframe == frame);
    }
}

// Harvested from older test code
void TestRealTime::testFrameConversion()
{
    int rates[] =
            { 7, 11025, 22050, 44100, 48000, 88200, 192000, 384000, 65521 };

    for (const int &rate : rates) {
        //qDebug() << "rate: " << rate << "\n";
        for (int v = -10; v < 10; ++v) {
            QVERIFY(checkFrameConversion(v, rate));
        }
        for (int v = -10; v < 10; ++v) {
            QVERIFY(checkFrameConversion(v + rate, rate));
        }
        for (int v = -10; v < 10; ++v) {
            QVERIFY(checkFrameConversion(v * rate + v, rate));
        }
    }
}

QTEST_MAIN(TestRealTime)

#include "realtime.moc"
