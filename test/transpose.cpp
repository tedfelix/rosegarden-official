/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
//

#include "base/NotationTypes.h"
#include "gui/dialogs/IntervalDialog.h"
#include "misc/Strings.h"
#include <QTest>

using namespace Rosegarden;
using std::cout;

// Tests for transposition
class TestTranspose : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAisDisplayAccidentalInCmaj();
    void testAisToBis();
    void testGToD();
    void testTransposeBbToF();
    void testIntervalToString();
    void testCisToC();
    void testKeyCMajorToDFlatMajor();
};

/**
 * should be in Pitch eventually
 */
void TestTranspose::testAisDisplayAccidentalInCmaj()
{
    Pitch ais(70, Accidentals::Sharp);
    Key cmaj("C major");
    Accidental accidental = ais.getDisplayAccidental(cmaj);
    QCOMPARE(accidental, Accidentals::Sharp); // "Accidental for A# in Cmaj should be Sharp
}

/** 
 * transpose an C# down by an augmented prime in C# major, should yield a C (in C major)
 */
void TestTranspose::testCisToC()
{
    // Testing transposing C# to C

    Pitch cis(73, Accidentals::Sharp);
    Pitch result = cis.transpose(Key("C# major"), -1, 0);

    Accidental resultAccidental = result.getAccidental(Key("C major"));
    int resultPitch = result.getPerformancePitch();
    QCOMPARE(resultAccidental, Accidentals::NoAccidental);
    QCOMPARE(resultPitch, 72);
}

/** 
 * transpose an A# up by a major second, should 
 * yield a B# (as C would be a minor triad) 
 */
void TestTranspose::testAisToBis()
{
    // Testing transposing A# to B#
    Pitch ais(70, Accidentals::Sharp);
    Key cmaj ("C major");

    Pitch result = ais.transpose(cmaj, 2, 1);

    Accidental resultAccidental = result.getAccidental(cmaj);
    int resultPitch = result.getPerformancePitch();
    QCOMPARE(resultAccidental, Accidentals::Sharp);
    QCOMPARE(resultPitch, 72);
}

/**
 * Transpose G to D in the key of D major.
 */
void TestTranspose::testGToD()
{
    // Testing transposing G to D
    Pitch g(67, Accidentals::Natural);
    Key* dmaj = new Key("D major");

    Pitch result = g.transpose(*dmaj, 7, 4);

    Accidental resultAccidental = result.getAccidental(*dmaj);
    int resultPitch = result.getPerformancePitch();
    QCOMPARE(resultAccidental, Accidentals::NoAccidental);
    QCOMPARE(resultPitch, 74);
}

void TestTranspose::testTransposeBbToF()
{
    Pitch bb(70, Accidentals::Flat);
    Key besmaj("Bb major");
    Pitch result = bb.transpose(besmaj, -5, -3);

    Accidental resultAccidental = result.getAccidental(besmaj);
    int resultPitch = result.getPerformancePitch();
    QCOMPARE(resultAccidental, Accidentals::NoAccidental);
    QCOMPARE(resultPitch, 65);
}

void TestTranspose::testKeyCMajorToDFlatMajor()
{
    Key cmaj("C major");
    Key result = cmaj.transpose(1, 1);

    std::string resultName = result.getName(); 
    int resultPitch = result.getTonicPitch();
    QCOMPARE(resultName, "Db major");
    QCOMPARE(resultPitch, 1);
}

static bool testIntervalString(int steps, int semitones, const QString &expectedString)
{
    QString text = IntervalDialog::getIntervalName(steps, semitones);
    if (text != expectedString) {
        std::cout << "When converting the interval " << steps << "," << semitones << " to string, expected '" << expectedString << "' but got '" << text << "'" << std::endl;
        return false;
    }
    return true;
}

void TestTranspose::testIntervalToString()
{
    QVERIFY(testIntervalString(1,1,"up a minor second"));
    QVERIFY(testIntervalString(0,0,"a perfect unison"));
    QVERIFY(testIntervalString(0,1,"up an augmented unison"));
    QVERIFY(testIntervalString(7,12,"up 1 octave(s)"));
    QVERIFY(testIntervalString(7,13,"up an augmented octave"));
}

QTEST_MAIN(TestTranspose)

#include "transpose.moc"
