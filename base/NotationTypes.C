
#include "NotationTypes.h"

void Key::checkAccidentalHeights() {

    if (m_accidentalHeights) return;
    m_accidentalHeights = new vector<int>;
  
    bool sharp = isSharp();
    int accidentals = getAccidentalCount();
    int accPitch = sharp ? 8 : 4;
  
    for (int i = 0; i < accidentals; ++i) {
        m_accidentalHeights->push_back(accPitch % 7);
        accPitch += (sharp ? -3 : 3);
    }
}

void Key::checkMap() {
    if (!m_keyDetailMap.empty()) return;

    m_keyDetailMap["A major" ] = KeyDetails(true,  false, 3, "F# minor", "A  maj / F# min");
    m_keyDetailMap["F# minor"] = KeyDetails(true,  true,  3, "A major",  "A  maj / F# min");
    m_keyDetailMap["Ab major"] = KeyDetails(false, false, 4, "F minor",  "Ab maj / F  min");
    m_keyDetailMap["F minor" ] = KeyDetails(false, true,  4, "Ab major", "Ab maj / F  min");
    m_keyDetailMap["B major" ] = KeyDetails(true,  false, 5, "G# minor", "B  maj / G# min");
    m_keyDetailMap["G# minor"] = KeyDetails(true,  true,  5, "B major",  "B  maj / G# min");
    m_keyDetailMap["Bb major"] = KeyDetails(false, false, 2, "G minor",  "Bb maj / G  min");
    m_keyDetailMap["G minor" ] = KeyDetails(false, true,  2, "Bb major", "Bb maj / G  min");
    m_keyDetailMap["C major" ] = KeyDetails(true,  false, 0, "A minor",  "C  maj / A  min");
    m_keyDetailMap["A minor" ] = KeyDetails(true,  true,  0, "C major",  "C  maj / A  min");
    m_keyDetailMap["Cb major"] = KeyDetails(false, false, 7, "Ab minor", "Cb maj / Ab min");
    m_keyDetailMap["Ab minor"] = KeyDetails(false, true,  7, "Cb major", "Cb maj / Ab min");
    m_keyDetailMap["C# major"] = KeyDetails(true,  false, 7, "A# minor", "C# maj / A# min");
    m_keyDetailMap["A# minor"] = KeyDetails(true,  true,  7, "C# major", "C# maj / A# min");
    m_keyDetailMap["D major" ] = KeyDetails(true,  false, 2, "B minor",  "D  maj / B  min");
    m_keyDetailMap["B minor" ] = KeyDetails(true,  true,  2, "D major",  "D  maj / B  min");
    m_keyDetailMap["Db major"] = KeyDetails(false, false, 5, "Bb minor", "Db maj / Bb min");
    m_keyDetailMap["Bb minor"] = KeyDetails(false, true,  5, "Db major", "Db maj / Bb min");
    m_keyDetailMap["E major" ] = KeyDetails(true,  false, 4, "C# minor", "E  maj / C# min");
    m_keyDetailMap["C# minor"] = KeyDetails(true,  true,  4, "E major",  "E  maj / C# min");
    m_keyDetailMap["Eb major"] = KeyDetails(false, false, 3, "C minor",  "Eb maj / C  min");
    m_keyDetailMap["C minor" ] = KeyDetails(false, true,  3, "Eb major", "Eb maj / C  min");
    m_keyDetailMap["F major" ] = KeyDetails(false, false, 1, "D minor",  "F  maj / D  min");
    m_keyDetailMap["D minor" ] = KeyDetails(false, true,  1, "F major",  "F  maj / D  min");
    m_keyDetailMap["F# major"] = KeyDetails(true,  false, 6, "D# minor", "F# maj / D# min");
    m_keyDetailMap["D# minor"] = KeyDetails(true,  true,  6, "F# major", "F# maj / D# min");
    m_keyDetailMap["G major" ] = KeyDetails(true,  false, 1, "E minor",  "G  maj / E  min");
    m_keyDetailMap["E minor" ] = KeyDetails(true,  true,  1, "G major",  "G  maj / E  min");
    m_keyDetailMap["Gb major"] = KeyDetails(false, false, 6, "Eb minor", "Gb maj / Eb min");
    m_keyDetailMap["Eb minor"] = KeyDetails(false, true,  6, "Gb major", "Gb maj / Eb min");
}


// Derived from RG2.1's MidiPitchToVoice in editor/src/Methods.c,
// InitialiseAccidentalTable in Format.c, and PutItemListInClef in
// MidiIn.c.  Converts performance pitch to height on staff + correct
// accidentals for current key.

void NotationDisplayPitch::rawPitchToDisplayPitch
(int pitch, Clef clef, Key key, int &height, Accidental &accidental)
{
    int octave;
    bool modified = false;
    height = 0;
    accidental = NoAccidental;

    // 1. Calculate with plain pitches, disregarding clef and key

    octave = pitch / 12;
    pitch  = pitch % 12;

    switch (pitch) {
    case  0: height = -2; break;	                  // C  
    case  1: height = -2; modified = true; break;   // C# 
    case  2: height = -1; break;                    // D  
    case  3: height = -1; modified = true; break;   // D# 
    case  4: height =  0; break;                    // E  
    case  5: height =  1; break;                    // F  
    case  6: height =  1; modified = true; break;   // F# 
    case  7: height =  2; break;                    // G  
    case  8: height =  2; modified = true; break;   // G# 
    case  9: height =  3; break;                    // A  
    case 10: height =  3; modified = true; break;   // A# 
    case 11: height =  4; break;                    // B  
    }

    height += (octave - 5) * 7;

    // 2. Adjust accidentals for the current key

    bool sharp = key.isSharp();

    accidental = modified ? (sharp ? Sharp : Flat) : NoAccidental;
    if (modified && !sharp) ++height; // because the modifier has become a flat

    for (vector<int>::const_iterator i = key.getAccidentalHeights().begin();
         i != key.getAccidentalHeights().end(); ++i) {
        if (*i == (height % 7)) {
            // the key has an accidental at the same height as this note, so
            // undo the note's accidental if there is one, or make explicit
            // if there isn't
            if (modified) accidental = NoAccidental;
            else accidental = Natural;
            break;
        }
    }

    // 3. Transpose up or down for the clef
  
    switch (clef) {
    case  TrebleClef: break;
    case   TenorClef: height += 7;  break;
    case    AltoClef: height += 7;  break; //???
    case    BassClef: height += 14; break;
    }
}


void displayPitchToRawPitch
(int height, Accidental accidental, Clef clef, Key key, int &pitch)
{
    int octave = 5;

    // 1. Get canonical pitch and correct octave

    while (height < 0) { octave -= 1; height += 7; }
    while (height > 7) { octave += 1; height -= 7; }

    if (height > 4) ++octave;

    switch (height) {

    case 0: pitch =  4; break;	/* bottom line, treble clef: E */
    case 1: pitch =  5; break;	/* F */
    case 2: pitch =  7; break;	/* G */
    case 3: pitch =  9; break;	/* A, in next octave */
    case 4: pitch = 11; break;	/* B, likewise*/
    case 5: pitch =  0; break;	/* C, moved up an octave (see above) */
    case 6: pitch =  2; break;	/* D, likewise */
    case 7: pitch =  4; break;	/* E, likewise */
    }

    // 2. Make any implicit accidentals from key explicit, and adjust pitch

    bool sharp = key.isSharp();

    for (vector<int>::const_iterator i = key.getAccidentalHeights().begin();
         i != key.getAccidentalHeights().end(); ++i) {
        if (*i == (height % 7)) {
            // the key has an accidental at the same height as this note
            if (accidental == Natural) accidental = NoAccidental;
            else if (accidental == NoAccidental) accidental = sharp ? Sharp : Flat;
            break;
        }
    }

    switch (accidental) {
    case DoubleSharp: pitch += 2; break;
    case       Sharp: pitch += 1; break;
    case        Flat: pitch -= 1; break;
    case  DoubleFlat: pitch -= 2; break;
    }

    // 3. Adjust for clef

    switch (clef) {
    case  TrebleClef: break;
    case   TenorClef: octave -= 1; break;
    case    AltoClef: octave -= 1; break;
    case    BassClef: octave -= 2; break;
    }

    pitch += 12 * octave;
}


Note::Note(const string &n)
    throw (BadType) :
    m_type(-1), m_dotted(false)
{
    string name(n);
    if (name.length() > 7 && name.substr(0, 7) == "dotted ") {
        m_dotted = true;
        name = name.substr(8);
    }
    Type t;
    for (t = Shortest; t <= Longest; ++t) {
        if (name == getEnglishName(t) ||
            name == getAmericanName(t) ||
            name == getShortName(t)) {
            m_type = t;
            break;
        }
    }
    if (m_type == -1) throw BadType();
}

string Note::getEnglishName(Type type, bool dotted) {
    static const string names[] = {
        "hemidemisemiquaver", "demisemiquaver", "semiquaver",
            "quaver", "crotchet", "minim", "semibreve", "breve"
            };
    if (type < 0) { type = m_type; dotted = m_dotted; }
    return dotted ? ("dotted " + names[type]) : names[type];
}

string Note::getAmericanName(Type type, bool dotted) {
    static const string names[] = {
        "sixty-fourth note", "thirty-second note", "sixteenth note",
            "eighth note", "quarter note", "half note", "whole note", "breve"
            };
    if (type < 0) { type = m_type; dotted = m_dotted; }
    return dotted ? ("dotted " + names[type]) : names[type];
}

string Note::getShortName(Type type, bool dotted) {
    static const string names[] = {
        "64th", "32nd", "16th", "8th", "quarter", "half", "whole", "breve"
            };
    if (type < 0) { type = m_type; dotted = m_dotted; }
    return dotted ? ("dotted " + names[type]) : names[type];
}

