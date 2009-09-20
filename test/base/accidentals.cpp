/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "NotationTypes.h"
#include <cstdlib>

using namespace Rosegarden;
using std::cout;

// Unit test-ish tests for resolving accidentals
// 
// Returns -1 (or crashes :)) on error, 0 on success
void assertHasAccidental(Pitch &pitch, 
	const Accidental& accidental, const Key& key)
{
  	Accidental calculatedAccidental = 
		pitch.getAccidental(key);

	std::cout << "Got " << calculatedAccidental << " for pitch " << pitch.getPerformancePitch() << " in key " << key.getName() << std::endl;

	if (calculatedAccidental != accidental)
	{
		std::cout << "Expected " << accidental << std::endl;
		exit(-1);
	}
}

void testBInEMinor()
{
	// a B, also in E minor, has no accidental
  	Pitch testPitch(59 % 12);
	assertHasAccidental(testPitch,
		Accidentals::NoAccidental, Key("E minor"));
}

/**
 *
 */
void testFInBMinor()
{
  	Pitch testPitch(77);
	assertHasAccidental(testPitch,
		Accidentals::NoAccidental, Key("B minor"));
}

void testInvalidSuggestion()
{
	// If we specify an invalid suggestion,
	// getAccidental() should be robust against that.
  	Pitch testPitch = Pitch(59, Accidentals::Sharp);
	assertHasAccidental(testPitch,
		Accidentals::NoAccidental, Key("E minor"));
}

int main(int argc, char **argv)
{
  testBInEMinor();
  testFInBMinor();
  testInvalidSuggestion();
  std::cout << "Success" << std::endl;
  exit(0);
}
