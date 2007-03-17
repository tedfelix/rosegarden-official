// -*- c-basic-offset: 4 -*-

#include "NotationTypes.h"

using namespace Rosegarden;
using std::cout;

// Unit test-ish tests for transposition.
// 
// Returns -1 (or crashes :)) on error, 0 on success

/** 
 * transpose an A# up by a major second, should 
 * yield a B# (as C would be a minor triad) 
 */
void testAisToBis()
{
    std::cout << "Testing transposing A# to B#... ";
    Pitch ais(70, Accidentals::Sharp);
    Key* cmaj = new Key();

    Pitch result = ais.transpose(*cmaj, 2, 1);

    Accidental resultAccidental = result.getAccidental(*cmaj);
    int resultPitch = result.getPerformancePitch();
    if (resultAccidental != Accidentals::Sharp || resultPitch != 72)
    {
        std::cout << "Transposing A# up by a major second didn't yield B#, but " << result.getNoteName(*cmaj) << resultAccidental << std::endl;
        exit(-1);
    }
    std::cout << "Success" << std::endl;
}

/**
 * Transpose G to D in the key of D major.
 */
void testGToD()
{
    std::cout << "Testing transposing G to D... ";
    Pitch g(67, Accidentals::Natural);
    Key* dmaj = new Key("D major");

    Pitch result = g.transpose(*dmaj, 7, 4);

    Accidental resultAccidental = result.getAccidental(*dmaj);
    int resultPitch = result.getPerformancePitch();
    if (resultAccidental != Accidentals::Natural || resultPitch != 74)
    {
        std::cout << "Transposing G up by a fifth didn't yield D, but " << result.getNoteName(*dmaj) << resultAccidental << std::endl;
        exit(-1);
    }
    std::cout << "Success" << std::endl;
}

void testKeyTransposition()
{

}

int main(int argc, char **argv)
{
    testAisToBis();
    testGToD();
    testKeyTransposition();

    return 0;    
}
