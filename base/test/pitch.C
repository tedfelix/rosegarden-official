
#include "NotationTypes.h"

namespace Rosegarden {

int main(int argc, char **argv)
{
    Accidental acc[] = {
	Accidentals::DoubleFlat,
	Accidentals::Flat,
	Accidentals::NoAccidental,
	Accidentals::Natural,
	Accidentals::Sharp,
	Accidentals::DoubleSharp
    };
    int acount(sizeof(acc)/sizeof(acc[0]));
    
    for (int p = 0; p < 128; ++p) {
	for (int a = 0; a < acount; ++a) {
	    Pitch(p, acc[a]);
	}
    }

    return 0;
}

}

