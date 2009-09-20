/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "NotationTypes.h"

using namespace Rosegarden;
using std::cout;

// Unit test-ish tests for resolving accidentals
// 
// Returns -1 (or crashes :)) on error, 0 on success
int assertHasAccidental(Pitch &pitch, 
    const Accidental& accidental, const Key& key)
{
      Accidental calculatedAccidental = 
        pitch.getAccidental(key);

    std::cout << "Got " << calculatedAccidental << " for pitch " << pitch.getPerformancePitch() << " in key " << key.getName() << std::endl;

    if (calculatedAccidental != accidental) {
        std::cout << "Expected " << accidental << std::endl;
        return -1;
    }
    return 0;
}

int testBInEMinor()
{
    // a B, also in E minor, has no accidental
    Pitch testPitch(59 % 12);
    return assertHasAccidental(testPitch,
        Accidentals::NoAccidental, Key("E minor"));
}

/**
 *
 */
int testFInBMinor()
{
    Pitch testPitch(77);
    return assertHasAccidental(testPitch,
        Accidentals::NoAccidental, Key("B minor"));
}

int testInvalidSuggestion()
{
    // If we specify an invalid suggestion,
    // getAccidental() should be robust against that.
    Pitch testPitch = Pitch(59, Accidentals::Sharp);
    return assertHasAccidental(testPitch,
        Accidentals::NoAccidental, Key("E minor"));
}

int testBbinBb()
{
    Pitch testPitch = Pitch(10, Accidentals::NoAccidental);
    Accidental accidental = testPitch.getAccidental(Key("Bb major"));
    std::cout << "Bb accidental: " << accidental << std::endl;
    if (accidental != Accidentals::Flat)
    {
        return -1;
    }
    return 0;
}

// Verifies that the height on staff for pitch 61 using flats is -1, not -2
int testDesHeight()
{
    bool useSharps = false;

    Pitch pitch(61);
    int h = pitch.getHeightOnStaff(Clef(Clef::Treble, 0), useSharps);

    if (h != -1)
    {
        std::cerr << "Error in testDesHeight: expected height -1, got " << h << std::endl;
        return -1;
    } 
    return 0;
}

int test_accidentals(int argc, char **argv)
{
  return testBInEMinor() +
      testFInBMinor() +
      testInvalidSuggestion() +
      testBbinBb() +
      testDesHeight();
}
