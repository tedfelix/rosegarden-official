
#ifndef _NOTATION_UNITS_H_
#define _NOTATION_UNITS_H_

#include "Element2.h"

enum Accidental {
  NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
};

enum Clef {
  TrebleClef, TenorClef, AltoClef, BassClef
};


/*
  -- Key

     All we store in a key Event is the name of the key.  A Key object
     can be constructed from such an Event or just from its name, and
     will return all the properties of the key.
*/

class Key {
public:
  static const string ElementPackage;
  static const string ElementType;
  static const string KeyPropertyName;
  static const Key DefaultKey;
  struct BadKeyName { };
  
  Key(Element2 &e)
    throw (Element2::NoData, Element2::BadType, BadKeyName) :
    m_accidentalHeights(0) {
    checkMap();
    if (e.package() != ElementPackage || e.type() != ElementType) {
      throw Element2::BadType();
    }
    m_name = e.get<String>(KeyPropertyName);
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
      throw BadKeyName();
    }
  }

  Key(string name)
    throw (BadKeyName)
    : m_name(name), m_accidentalHeights(0) {
    checkMap();
    if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
      throw BadKeyName();
    }
  }    

  Key(const Key &kc) : m_name(kc.m_name), m_accidentalHeights(0) { }
  virtual ~Key() { delete m_accidentalHeights; }

  Key &operator=(const Key &kc) {
    m_name = kc.m_name;
    m_accidentalHeights = 0;
    return *this;
  }

  bool isMinor() {
    return m_keyDetailMap[m_name].m_minor;
  }

  bool isSharp() {
    return m_keyDetailMap[m_name].m_sharps;
  }

  int getAccidentalCount() {
    return m_keyDetailMap[m_name].m_sharpCount;
  }

  Key getEquivalent() { // e.g. called on C major, return A minor
    return Key(m_keyDetailMap[m_name].m_equivalence);
  }

  string getName() {
    return m_name;
  }

  string getRosegarden2Name() {
    return m_keyDetailMap[m_name].m_rg2name;
  }

  vector<int> getAccidentalHeights() {
    // staff positions of accidentals, if we're in the treble clef
    checkAccidentalHeights();
    return *m_accidentalHeights;
  }

  Element2 getAsElement() {
    Element2 e(ElementPackage, ElementType);
    e.set<String>(KeyPropertyName, m_name);
    return e;
  }

  static vector<Key> getKeys(bool minor = false) {
    checkMap();
    vector<Key> result;
    for (KeyDetailMap::const_iterator i = m_keyDetailMap.begin();
         i != m_keyDetailMap.end(); ++i) {
      if ((*i).second.m_minor == minor) {
        result.push_back(Key((*i).first));
      }
    }
    return result;
  }

private:
  string m_name;
  vector<int> *m_accidentalHeights;

  struct KeyDetails {
    bool   m_sharps;
    bool   m_minor;
    int    m_sharpCount;
    string m_equivalence;
    string m_rg2name;

    KeyDetails() : // ctor needed in order to live in a hash_map
      m_sharps(false), m_minor(false), m_sharpCount(0),
      m_equivalence(""), m_rg2name("") { }

    KeyDetails(bool sharps, bool minor, int sharpCount,
               string equivalence, string rg2name) :
      m_sharps(sharps), m_minor(minor), m_sharpCount(sharpCount),
      m_equivalence(equivalence), m_rg2name(rg2name) { }

    KeyDetails(const KeyDetails &d) :
      m_sharps(d.m_sharps), m_minor(d.m_minor), m_sharpCount(d.m_sharpCount),
      m_equivalence(d.m_equivalence), m_rg2name(d.m_rg2name) { }

    KeyDetails &operator=(const KeyDetails &d) {
      if (&d == this) return *this;
      m_sharps = d.m_sharps; m_minor = d.m_minor;
      m_sharpCount = d.m_sharpCount; m_equivalence = d.m_equivalence;
      m_rg2name = d.m_rg2name;
      return *this;
    }
  };

  typedef hash_map<string, KeyDetails, hashstring, eqstring> KeyDetailMap;
  static KeyDetailMap m_keyDetailMap;
  static void checkMap();
  void checkAccidentalHeights();
};

const string Key::ElementPackage = "core";
const string Key::ElementType = "keychange";
const string Key::KeyPropertyName = "key";
const Key Key::DefaultKey = Key("C major");

/*
  -- Pitch

     Events store pitch values using the MIDI pitch scale and as
     simple integers.  These are fixed-frequency pitches, independent
     of clef and key.  Adding 12 to a pitch increments it by one
     octave; pitch 60 is the treble-clef middle C.  (Previous rewrites
     have considered using double the MIDI pitch so as to allow
     quarter-tones; this time let's go for the simpler option as if we
     ever want quarter-tones we can always code them using special
     Element2 properties.)
     
     For notation purposes we need a display pitch, which is a
     composite of height on the staff plus accidental.  The
     correspondence between display pitch and raw pitch depends on the
     clef and key.  For height on the staff, we use the RG2.1
     convention of 0 = bottom line, 1 = gap between bottom two lines
     and so on up to 8 = top line.  Negative heights are below the
     staff (i.e. on leger lines), heights over 8 are above.

     When we insert a note, we need to query the height of the staff
     line next to which it's being inserted, then translate this back
     to raw pitch according to the clef in force at the x-coordinate
     at which the note is inserted.  For display, we translate from
     raw pitch using both the clef and the key in force.

     Whether an accidental should be displayed or not depends on the
     current key, on whether we've already shown the same accidental
     for that pitch in the same bar, on whether the note event
     explicitly requests an accidental...  All we calculate here is
     whether the pitch "should" have an accidental, not whether it
     really will (e.g. if the accidental has already appeared).

     If we ever see pitch as a plain integer, we assume it's a raw
     internal pitch.
*/

class NotationDisplayPitch
{
public:
  NotationDisplayPitch(int pitch, Clef clef, Key key) {
    //!!! explicit accidentals in the note event properties?
    rawPitchToDisplayPitch(pitch, clef, key, m_heightOnStaff, m_accidental);
  }

  NotationDisplayPitch(int heightOnStaff, Accidental accidental) :
    m_heightOnStaff(heightOnStaff), m_accidental(accidental) { }

  int getHeightOnStaff() { return m_heightOnStaff; }
  Accidental getAccidental() { return m_accidental; }

  int getPerformancePitch(Clef clef, Key key) {
    int p = 0;
    displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, key, p);
    return p;
  }

private:
  int m_heightOnStaff;
  Accidental m_accidental;

  void rawPitchToDisplayPitch(int, Clef, Key, int &, Accidental &);
  void displayPitchToRawPitch(int, Accidental, Clef, Key, int &);
};


/*
  -- Duration

     Events store duration as simple integers where 6 units = one
     hemidemisemiquaver (and 1 unit = 4 MIDI clocks).

*/     

class Note
{
public:
  typedef int Type; // not an enum, too much arithmetic at stake
  struct BadType { };

  // define both sorts of names; some people prefer the American
  // names, but I just can't remember which of them is which

  static const Type SixtyFourthNote, ThirtySecondNote, SixteenthNote,
    EighthNote, QuarterNote, HalfNote, WholeNote;

  static const Type Hemidemisemiquaver, Demisemiquaver, Semiquaver,
    Quaver, Crotchet, Minim, Semibreve, Breve;

  static const Type Shortest, Longest;

  Note(Type type, bool dotted = false) throw (BadType) :
    m_type(type), m_dotted(dotted) {
    if (m_type < Shortest || m_type > Longest) throw BadType();
  }

  Note(const Note &n) : m_type(n.m_type), m_dotted(n.m_dotted) { }
  virtual ~Note() { }

  Note &operator=(const Note &n) {
    if (&n == this) return *this;
    m_type = n.m_type;
    m_dotted = n.m_dotted;
    return *this;
  }

  bool isFilled()     { return m_type <= Crotchet; }
  bool isDotted()     { return m_dotted; }
  bool isStalked()    { return m_type <= Minim; }
  bool getTailCount() { return m_type >= Crotchet ? 0 : Crotchet - m_type; }

  int  getDuration()  {
    int d = 6;
    for (int t = m_type; t > Hemidemisemiquaver; --t) d *= 2;
    return (m_dotted ? (d + d/2) : d);
  }

  string getEnglishName();
  string getAmericanName();
  string getShortName();
  
private:
  Type m_type;
  bool m_dotted;
};

const Note::Type Note::SixtyFourthNote     = 0;
const Note::Type Note::ThirtySecondNote    = 1;
const Note::Type Note::SixteenthNote       = 2;
const Note::Type Note::EighthNote          = 3;
const Note::Type Note::QuarterNote         = 4;
const Note::Type Note::HalfNote            = 5;
const Note::Type Note::WholeNote           = 6;

const Note::Type Note::Hemidemisemiquaver  = 0;
const Note::Type Note::Demisemiquaver      = 1;
const Note::Type Note::Semiquaver          = 2;
const Note::Type Note::Quaver              = 3;
const Note::Type Note::Crotchet            = 4;
const Note::Type Note::Minim               = 5;
const Note::Type Note::Semibreve           = 6;
const Note::Type Note::Breve               = 7;

const Note::Type Note::Shortest = 0;
const Note::Type Note::Longest  = 7;

#endif
