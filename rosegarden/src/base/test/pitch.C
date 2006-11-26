// -*- c-basic-offset: 4 -*-

#include "NotationTypes.h"

using namespace Rosegarden;
using std::cout;
using std::endl;

static const int verbose = 0;

// This is the old NotationDisplayPitch -- this file was written for
// regression testing when implementing the new Pitch class.  It won't
// compile any more as NotationDisplayPitch needs to be a friend of
// Pitch for this implementation to work.  Add "friend class
// NotationDisplayPitch;" to end of Pitch in ../NotationTypes.h to
// build it

/**
 * NotationDisplayPitch stores a note's pitch in terms of the position
 * of the note on the staff and its associated accidental, and
 * converts these values to and from performance (MIDI) pitches.
 *
 * Rationale: When we insert a note, we need to query the height of the
 * staff line next to which it's being inserted, then translate this
 * back to raw pitch according to the clef in force at the x-coordinate
 * at which the note is inserted.  For display, we translate from raw
 * pitch using both the clef and the key in force.
 *
 * Whether an accidental should be displayed or not depends on the
 * current key, on whether we've already shown the same accidental for
 * that pitch in the same bar, on whether the note event explicitly
 * requests an accidental...  All we calculate here is whether the
 * pitch "should" have an accidental, not whether it really will
 * (e.g. if the accidental has already appeared).
 *
 * (See also docs/discussion/units.txt for explanation of pitch units.)
 */

class NotationDisplayPitch
{
public:
    /**
     * Construct a NotationDisplayPitch containing the given staff
     * height and accidental
     */
    NotationDisplayPitch(int heightOnStaff,
			 const Accidental &accidental);

    /**
     * Construct a NotationDisplayPitch containing the height and
     * accidental to which the given performance pitch corresponds
     * in the given clef and key
     */
    NotationDisplayPitch(int pitch, const Clef &clef, const Key &key,
                         const Accidental &explicitAccidental = 
			 Accidentals::NoAccidental);

    int getHeightOnStaff() const { return m_heightOnStaff; }
    Accidental getAccidental() const { return m_accidental; }

    /**
     * Calculate and return the performance (MIDI) pitch 
     * corresponding to the stored height and accidental, in the
     * given clef and key
     */
    int getPerformancePitch(const Clef &clef, const Key &key) const;

    /**
     * Calculate and return the performance (MIDI) pitch
     * corresponding to the stored height and accidental,
     * interpreting them as Rosegarden-2.1-style values (for
     * backward compatibility use), in the given clef and key
     */
    int getPerformancePitchFromRG21Pitch(const Clef &clef,
					 const Key &key) const;

    /**
     * Return the stored pitch as a string (C4, Bb2, etc...)
     * according to http://www.harmony-central.com/MIDI/Doc/table2.html
     *
     * If inclOctave is false, this will return C, Bb, etc.
     */
    std::string getAsString(const Clef &clef, const Key &key,
			    bool inclOctave = true,
			    int octaveBase = -2) const;

    /**
     * Return the stored pitch as a description of a note in a
     * scale.  Return values are:
     * 
     * -- placeInScale: a number from 0-6 where 0 is C and 6 is B
     * 
     * -- accidentals: a number from -2 to 2 where -2 is double flat,
     *     -1 is flat, 0 is nothing, 1 is sharp, 2 is double sharp
     * 
     * -- octave: MIDI octave in range -2 to 8, where pitch 0 is in
     *     octave -2 and thus middle-C is in octave 3
     * 
     * This function is guaranteed never to return values out of
     * the above ranges.
     */
    void getInScale(const Clef &clef, const Key &key,
		    int &placeInScale, int &accidentals, int &octave) const;
    
private:
    int m_heightOnStaff;
    Accidental m_accidental;

    static void rawPitchToDisplayPitch(int, const Clef &, const Key &,
				       int &, Accidental &);
    static void displayPitchToRawPitch(int, Accidental, const Clef &, const Key &,
				       int &, bool ignoreOffset = false);
};
//////////////////////////////////////////////////////////////////////
// NotationDisplayPitch
//////////////////////////////////////////////////////////////////////

NotationDisplayPitch::NotationDisplayPitch(int heightOnStaff,
                                           const Accidental &accidental)
    : m_heightOnStaff(heightOnStaff),
      m_accidental(accidental)
{
}

NotationDisplayPitch::NotationDisplayPitch(int pitch, const Clef &clef,
                                           const Key &key,
                                           const Accidental &explicitAccidental) :
    m_accidental(explicitAccidental)
{
    rawPitchToDisplayPitch(pitch, clef, key, m_heightOnStaff, m_accidental);
}

int
NotationDisplayPitch::getPerformancePitch(const Clef &clef, const Key &key) const
{
    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, key, p);
    return p;
}

int
NotationDisplayPitch::getPerformancePitchFromRG21Pitch(const Clef &clef,
                                                       const Key &) const
{
    // Rosegarden 2.1 pitches are a bit weird; see
    // docs/data_struct/units.txt

    // We pass the accidental and clef, a faked key of C major, and a
    // flag telling displayPitchToRawPitch to ignore the clef offset
    // and take only its octave into account

    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, Key(), p, true);
    return p;
}


void
NotationDisplayPitch::rawPitchToDisplayPitch(int rawpitch,
					     const Clef &clef,
					     const Key &key,
					     int &height,
					     Accidental &accidental)
{
    Pitch::rawPitchToDisplayPitch(rawpitch, clef, key, height, accidental);
}

void
NotationDisplayPitch::displayPitchToRawPitch(int height,
                                             Accidental accidental,
                                             const Clef &clef,
                                             const Key &key,
                                             int &pitch,
                                             bool ignoreOffset)
{
    Pitch::displayPitchToRawPitch(height, accidental, clef, key, pitch,
				  ignoreOffset);
}
string
NotationDisplayPitch::getAsString(const Clef &clef, const Key &key,
                                  bool inclOctave, int octaveBase) const
{
    static const string noteNamesSharps[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    static const string noteNamesFlats[]  = {
        "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
    };
    
    int performancePitch = getPerformancePitch(clef, key);

    // highly unlikely, but fatal if it happened:
    if (performancePitch < 0) performancePitch = 0;

    int pitch  = performancePitch % 12;
    int octave = performancePitch / 12;

    if (!inclOctave)
        return key.isSharp() ? noteNamesSharps[pitch] : noteNamesFlats[pitch];

    char tmp[1024];

    if (key.isSharp())
        sprintf(tmp, "%s%d", noteNamesSharps[pitch].c_str(),
                octave + octaveBase);
    else
        sprintf(tmp, "%s%d", noteNamesFlats[pitch].c_str(),
                octave + octaveBase);
    
    return string(tmp);
}

void
NotationDisplayPitch::getInScale(const Clef &clef, const Key &key,
				 int &placeInScale, int &accidentals, int &octave) const
{
    //!!! Maybe we should bring the logic from rawPitchToDisplayPitch down
    // into this method, and make rawPitchToDisplayPitch wrap this

    static int pitches[2][12] = {
	{ 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 },
	{ 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 },
    };
    static int accidentalsForPitches[2][12] = {
	{ 0,  1, 0,  1, 0, 0,  1, 0,  1, 0,  1, 0 },
	{ 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0 },
    };
    
    int performancePitch = getPerformancePitch(clef, key);

    // highly unlikely, but fatal if it happened:
    if (performancePitch < 0) performancePitch = 0;
    if (performancePitch > 127) performancePitch = 127;

    int pitch  = performancePitch % 12;
    octave = performancePitch / 12 - 2;

    if (key.isSharp()) { //!!! need to [optionally?] handle minor keys (similarly in getAsString?)
	placeInScale = pitches[0][pitch];
	accidentals = accidentalsForPitches[0][pitch];
    } else {
	placeInScale = pitches[1][pitch];
	accidentals = accidentalsForPitches[1][pitch];
    }
}



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

