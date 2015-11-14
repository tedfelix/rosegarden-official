/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "base/NotationTypes.h"
#include <QTest>

using namespace Rosegarden;
using std::cout;

// Unit test for resolving accidentals

class TestAccidentals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBInEMinor();
    void testFInBMinor();
    void testInvalidSuggestion();
    void testBbinBb();
    void testDesHeight();
};

static bool assertHasAccidental(Pitch &pitch,
                                const Accidental& accidental,
                                const Key& key)
{
    Accidental calculatedAccidental = pitch.getAccidental(key);

    std::cout << "Got " << calculatedAccidental << " for pitch " << pitch.getPerformancePitch() << " in key " << key.getName() << std::endl;

    if (calculatedAccidental != accidental) {
        std::cout << "Expected " << accidental << std::endl;
        return false;
    }
    return true;
}

void TestAccidentals::testBInEMinor()
{
    // a B, also in E minor, has no accidental
    Pitch testPitch(59 % 12);
    QVERIFY(assertHasAccidental(testPitch, Accidentals::NoAccidental, Key("E minor")));
}

void TestAccidentals::testFInBMinor()
{
    Pitch testPitch(77);
    QVERIFY(assertHasAccidental(testPitch, Accidentals::NoAccidental, Key("B minor")));
}

void TestAccidentals::testInvalidSuggestion()
{
    // If we specify an invalid suggestion,
    // getAccidental() should be robust against that.
    Pitch testPitch = Pitch(59, Accidentals::Sharp);
    QVERIFY(assertHasAccidental(testPitch, Accidentals::NoAccidental, Key("E minor")));
}

void TestAccidentals::testBbinBb()
{
    Pitch testPitch = Pitch(10, Accidentals::NoAccidental);
    Accidental accidental = testPitch.getAccidental(Key("Bb major"));
    QCOMPARE(accidental, Accidentals::Flat);
}

// Verifies that the height on staff for pitch 61 using flats is -1, not -2
void TestAccidentals::testDesHeight()
{
    bool useSharps = false;

    Pitch pitch(61);
    int h = pitch.getHeightOnStaff(Clef(Clef::Treble, 0), useSharps);
    QCOMPARE(h, -1);
}

QTEST_MAIN(TestAccidentals)

#include "accidentals.moc"
