// -*- c-basic-offset: 4 -*-

#include "NotationTypes.h"

using namespace Rosegarden;
using std::cout;
using std::endl;

static const int verbose = 0;


int testNote(Accidental &acc, Key &key, int octave, int note)
{
    int rv = 0;

    Pitch pitch(note, octave, key, acc);

    static int prevPerformancePitch = -1;
    static Accidental prevAcc = Accidentals::NoAccidental;
    static int prevOctave = -2;

    int p = pitch.getPerformancePitch();
    if (p < prevPerformancePitch && (prevAcc == acc && prevOctave == octave)) {
	cout << "testNote: " << note << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch is " << p << ", should be >= " << prevPerformancePitch << endl;
	rv = 1;
    }

    int nis = pitch.getNoteInScale(key);
    if (nis != note) {
	cout << "testNote: " << note << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "note in scale is " << nis << " (not " << note << ")" << endl;
	rv = 1;
    }

    // can do special checks on C-major etc 'cos it's easy, and stuff like that

    if (key == Key("C major")) {
	if (acc == Accidentals::NoAccidental) {
	    static int pitches[] = { 0, 2, 4, 5, 7, 9, 11 };
	    Pitch comparative(pitches[nis], octave);
	    if (comparative.getPerformancePitch() != p) {
		cout << "testNote: " << note << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
		     << "comparative pitch is " << comparative.getPerformancePitch() << ", should be " << p << endl;
		rv = 1;
	    }
	}
    }

    prevPerformancePitch = p;
    prevOctave = octave;
    prevAcc = acc;

    if (!rv && verbose) {
	cout << "testNote: " << note << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch " << p << endl;
    }
    return rv;
}

int testNoteName(Accidental &acc, Key &key, int octave, char noteName)
{
    int rv = 0;

    Pitch pitch(noteName, octave, key, acc);

    static int prevPerformancePitch = -1;
    static Accidental prevAcc = Accidentals::NoAccidental;
    static int prevOctave = -2;

    int p = pitch.getPerformancePitch();
    if (p < prevPerformancePitch && (prevAcc == acc && prevOctave == octave)) {
	cout << "testNoteName: " << noteName << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch is " << p << ", should be >= " << prevPerformancePitch << endl;
	rv = 1;
    }

    char nn = pitch.getNoteName(key);
    if (nn != noteName) {
	cout << "testNoteName: " << noteName << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "note is " << nn << " (not " << noteName << ") (pitch was " << p << ")" << endl;
	rv = 1;
    }

    prevPerformancePitch = p;
    prevOctave = octave;
    prevAcc = acc;

    if (!rv && verbose) {
	cout << "testNoteName: " << noteName << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch " << p << endl;
    }
    return rv;
}

int testPitchInOctave(Accidental &acc, Key &key, int octave, int pio)
{
    int rv = 0;

    Pitch pitch(pio, octave, acc);
    
    int p = pitch.getPerformancePitch();
    if (p != (octave + 2) * 12 + pio) {
	cout << "testPitchInOctave: " << pio << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch is " << p << ", should be " << ((octave + 2) * 12 + pio) << endl;
	rv = 1;
    }

    if (!rv && verbose) {
	cout << "testNote: " << pio << " " << acc << ", " << key.getName() << ", octave " << octave << ": "
	     << "pitch " << p << endl;
    }
    return rv;
}

int testPitch(Accidental &acc, Key &key, Clef &clef, int pp)
{
    int rv = 0;

    Pitch pitch(pp, acc);
    NotationDisplayPitch ndp(pp, clef, key, acc);

    int h = pitch.getHeightOnStaff(clef, key);
    int nh = ndp.getHeightOnStaff();
    if (h != nh) {
	cout << "testPitch: " << pp << ", " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
	     << "height is " << h << " (ndp returns " << nh << ")" << endl;
	rv = 1;
    }

    Accidental pa = pitch.getDisplayAccidental(key);
    Accidental na = ndp.getAccidental();
    if (pa != na) {
	cout << "testPitch: " << pp << ", " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
	     << "display acc is " << pa << " (ndp returns " << na << ")" << endl;
	rv = 1;
    }

    return rv;
}

int testHeight(Accidental &acc, Key &key, Clef &clef, int height)
{
    int rv = 0;
    
    Pitch pitch(height, clef, key, acc);
    NotationDisplayPitch ndp(height, acc);
    NotationDisplayPitch ndp2(pitch.getPerformancePitch(), clef, key, acc);

    int ppp = pitch.getPerformancePitch();
    int npp = ndp.getPerformancePitch(clef, key);

    if (ppp != npp) {
	cout << "testHeight: " << height << " " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
	     << "pitch " << ppp << " (ndp returns " << npp << ")" << endl;
	rv = 1;
    }

    int h = pitch.getHeightOnStaff(clef, key);
    if (h != ndp.getHeightOnStaff() || h != height) {
	cout << "testHeight: " << height << " " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
	     << "height " << h << " (ndp returns " << ndp.getHeightOnStaff() << ")" << endl;
	rv = 1;
    }

    // for NoAccidental, the Pitch object will acquire the accidental
    // from the current key whereas NotationDisplayPitch will not --
    // hence we skip this test for NoAccidental
    if (acc != Accidentals::NoAccidental) {
	Accidental nacc = ndp2.getAccidental();
	Accidental pacc = pitch.getDisplayAccidental(key);
	if (nacc != pacc) {
	    cout << "testHeight: " << height << " " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
		"acc " << pacc << " (ndp returns " << nacc << ")" << endl;
	    rv = 1;
	}
    }

    if (!rv && verbose) {
	cout << "testHeight: " << height << " " << acc << ", " << key.getName() << ", " << clef.getClefType() << ": "
	     << "pitch " << ppp << endl;
    }
    return rv;
    
}


int main(int argc, char **argv)
{
    Accidentals::AccidentalList accidentals(Accidentals::getStandardAccidentals());
    Clef::ClefList clefs(Clef::getClefs());

    Key::KeyList keys;
    Key::KeyList majorKeys(Key::getKeys(false));
    Key::KeyList minorKeys(Key::getKeys(true));
    keys.insert(keys.end(), majorKeys.begin(), majorKeys.end());
    keys.insert(keys.end(), minorKeys.begin(), minorKeys.end());
    
    for (int a = 0; a < accidentals.size(); ++a) {

	for (int k = 0; k < keys.size(); ++k) {

	    for (int o = -2; o < 9; ++o) {
		for (int n = 0; n < 7; ++n) {
		    testNote(accidentals[a], keys[k], o, n);
		}
	    }

	    for (int o = -2; o < 9; ++o) {
		for (int p = 0; p < 12; ++p) {
		    testPitchInOctave(accidentals[a], keys[k], o, p);
		}
	    }

	    for (int o = -2; o < 9; ++o) {
		for (int p = 0; p < 7; ++p) {
		    testNoteName(accidentals[a], keys[k], o, Pitch::getNoteForIndex(p));
		}
	    }
	    
	    for (int c = 0; c < clefs.size(); ++c) {

		for (int p = 0; p < 128; ++p) {
		    testPitch(accidentals[a], keys[k], clefs[c], p);
		}

		for (int h = -20; h < 30; ++h) {
		    testHeight(accidentals[a], keys[k], clefs[c], h);
		}
	    }
	}
    }

    return 0;
}

