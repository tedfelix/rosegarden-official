// -*- c-basic-offset: 4 -*-

#include "NotationTypes.h"

using namespace Rosegarden;

int main(int argc, char **argv)
{
    Accidentals::AccidentalList accidentals(Accidentals::getStandardAccidentals());
    Clef::ClefList clefs(Clef::getClefs());
    Key::KeyList majorKeys(Key::getKeys(false));
    Key::KeyList minorKeys(Key::getKeys(true));
    
    for (int p = 0; p < 128; ++p) {
	for (int a = 0; a < accidentals.size(); ++a) {
	    Pitch pitch(p, accidentals[a]);

	    for (int k = 0; k < majorKeys.size(); ++k) {
		//!!! + minor

		for (int c = 0; c < clefs.size(); ++c) {

		}
	    }
	}
    }

    return 0;
}

