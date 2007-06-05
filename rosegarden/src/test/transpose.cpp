// -*- c-basic-offset: 4 -*-
//

#include "NotationTypes.h"

using namespace Rosegarden;
using std::cout;

// Unit test-ish tests for transposition.
// 
// Returns -1 (or crashes :)) on error, 0 on success

/**
 * should be in Pitch eventually
 */
int testAisDisplayAccidentalInCmaj()
{
    Pitch ais(70, Accidentals::Sharp);
    Key cmaj ("C major");
    Accidental accidental = ais.getDisplayAccidental(cmaj);
    if (accidental != Accidentals::Sharp)
    {
        std::cout << "Accidental for A# in Cmaj was " << accidental << " instead of expected Sharp" << std::endl;
        return -1;
    }
    
    return 0;
}

/** 
 * transpose an C# down by an augmented prime in C# major, should yield a C (in C major)
 */
int testCisToC()
{
    std::cout << "Testing transposing C# to C... ";

    Pitch cis(73, Accidentals::Sharp);
    Pitch result = cis.transpose(Key("C# major"), -1, 0);

    Accidental resultAccidental = result.getAccidental(Key("C major"));
    int resultPitch = result.getPerformancePitch();
    if (resultAccidental != Accidentals::NoAccidental || resultPitch != 72)
    {
        std::cout << "Transposing C# down by an augmented prime didn't yield C, but " << result.getNoteName(Key("C major")) << resultAccidental << std::endl;
        return -1;
    }
    
    return 0;
}

/** 
 * transpose an A# up by a major second, should 
 * yield a B# (as C would be a minor triad) 
 */
int testAisToBis()
{
    std::cout << "Testing transposing A# to B#... ";
    Pitch ais(70, Accidentals::Sharp);
    Key cmaj ("C major");

    Pitch result = ais.transpose(cmaj, 2, 1);

    Accidental resultAccidental = result.getAccidental(cmaj);
    int resultPitch = result.getPerformancePitch();
    if (resultAccidental != Accidentals::Sharp || resultPitch != 72)
    {
        std::cout << "Transposing A# up by a major second didn't yield B#, but " << result.getNoteName(cmaj) << resultAccidental << std::endl;
        return -1;
    }
    std::cout << "Success" << std::endl;
    
    return 0;
}

/**
 * Transpose G to D in the key of D major.
 */
int testGToD()
{
    std::cout << "Testing transposing G to D... ";
    Pitch g(67, Accidentals::Natural);
    Key* dmaj = new Key("D major");

    Pitch result = g.transpose(*dmaj, 7, 4);

    Accidental resultAccidental = result.getAccidental(*dmaj);
    int resultPitch = result.getPerformancePitch();
    if (resultAccidental != Accidentals::NoAccidental || resultPitch != 74)
    {
        std::cout << "Transposing G up by a fifth didn't yield D, but " << result.getNoteName(*dmaj) << resultAccidental << std::endl;
        return -1;
    }
    std::cout << "Success" << std::endl;
    return 0;
}

int test_transpose(int argc, char **argv)
{
    return testAisDisplayAccidentalInCmaj() +
        testAisToBis() +
        testGToD() +
        testCisToC();    
}
