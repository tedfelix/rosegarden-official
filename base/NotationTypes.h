
#ifndef _NOTATION_UNITS_H_
#define _NOTATION_UNITS_H_

#include "Event.h"

enum Accidental {
    NoAccidental, Sharp, Flat, Natural, DoubleSharp, DoubleFlat
};


// somewhat mechanical:

class Clef {
public:
    static const string EventPackage;
    static const string EventType;
    static const string ClefPropertyName;
    static const Clef DefaultClef;
    struct BadClefName { };

    static const string Treble;
    static const string Tenor;
    static const string Alto;
    static const string Bass;

    Clef() : m_clef(DefaultClef.m_clef) { }

    Clef(const Event &e) throw (Event::NoData, Event::BadType, BadClefName) {
        if (e.package() != EventPackage || e.type() != EventType) {
            throw Event::BadType();
        }
        string s = e.get<String>(ClefPropertyName);
        if (s != Treble && s != Tenor && s != Alto && s != Bass) {
            throw BadClefName();
        }
        m_clef = s;
    }        

    Clef(const string &s) throw (BadClefName) {
        if (s != Treble && s != Tenor && s != Alto && s != Bass) {
            throw BadClefName();
        }
        m_clef = s;
    }

    Clef(const Clef &c) : m_clef(c.m_clef) { }
    Clef &operator=(const Clef &c) {
        if (this != &c) m_clef = c.m_clef;
        return *this;
    }
    virtual ~Clef() { }

    string getName() const { return m_clef; }
    int getOctave() const {
        if (m_clef == Treble) return 0;
        else if (m_clef == Bass) return -2;
        else return -1;
    }

private:
    string m_clef;
};


/*
  -- Key

     All we store in a key Event is the name of the key.  A Key object
     can be constructed from such an Event or just from its name, and
     will return all the properties of the key.
*/

class Key {
public:
    static const string EventPackage;
    static const string EventType;
    static const string KeyPropertyName;
    static const Key DefaultKey;
    struct BadKeyName { };

    Key() : m_name(DefaultKey.m_name), m_accidentalHeights(0) { checkMap(); }
  
    Key(const Event &e)
        throw (Event::NoData, Event::BadType, BadKeyName) :
        m_accidentalHeights(0) {
        checkMap();
        if (e.package() != EventPackage || e.type() != EventType) {
            throw Event::BadType();
        }
        m_name = e.get<String>(KeyPropertyName);
        if (m_keyDetailMap.find(m_name) == m_keyDetailMap.end()) {
            throw BadKeyName();
        }
    }

    Key(const string &name)
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

    bool isMinor() const {
        return m_keyDetailMap[m_name].m_minor;
    }

    bool isSharp() const {
        return m_keyDetailMap[m_name].m_sharps;
    }

    int getAccidentalCount() const {
        return m_keyDetailMap[m_name].m_sharpCount;
    }

    Key getEquivalent() const { // e.g. called on C major, return A minor
        return Key(m_keyDetailMap[m_name].m_equivalence);
    }

    string getName() const {
        return m_name;
    }

    string getRosegarden2Name() const {
        return m_keyDetailMap[m_name].m_rg2name;
    }

    vector<int> getAccidentalHeights() const {
        // staff positions of accidentals, if we're in the treble clef
        checkAccidentalHeights();
        return *m_accidentalHeights;
    }

    Event getAsEvent() const {
        Event e(EventPackage, EventType);
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
    mutable vector<int> *m_accidentalHeights;

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
            m_sharps(d.m_sharps), m_minor(d.m_minor),
            m_sharpCount(d.m_sharpCount), m_equivalence(d.m_equivalence),
            m_rg2name(d.m_rg2name) { }

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
    void checkAccidentalHeights() const;
};

/*
  -- Pitch

     Events store pitch values using the MIDI pitch scale and as
     simple integers.  These are fixed-frequency pitches, independent
     of clef and key.  Adding 12 to a pitch increments it by one
     octave; pitch 60 is the treble-clef middle C.  (Previous rewrites
     have considered using double the MIDI pitch so as to allow
     quarter-tones; this time let's go for the simpler option as if we
     ever want quarter-tones we can always code them using special
     Event properties.)
     
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
    NotationDisplayPitch(int pitch, const Clef &clef, const Key &key) {
        //!!! explicit accidentals in the note event properties?
        rawPitchToDisplayPitch(pitch, clef, key, m_heightOnStaff, m_accidental);
    }

    NotationDisplayPitch(int heightOnStaff, Accidental accidental) :
        m_heightOnStaff(heightOnStaff), m_accidental(accidental) { }

    int getHeightOnStaff() const { return m_heightOnStaff; }
    Accidental getAccidental() const { return m_accidental; }

    int getPerformancePitch(const Clef &clef, const Key &key) const {
        int p = 0;
        displayPitchToRawPitch(m_heightOnStaff, m_accidental, clef, key, p);
        return p;
    }

private:
    int m_heightOnStaff;
    Accidental m_accidental;

    void rawPitchToDisplayPitch(int, const Clef &, const Key &,
                                int &, Accidental &) const;
    void displayPitchToRawPitch(int, Accidental, const Clef &, const Key &,
                                int &) const;
};


/*
  -- Duration

     Events store duration as simple integers where 6 units = one
     hemidemisemiquaver (and 1 unit = 4 MIDI clocks).

*/     

class TimeSignature;

class Note
{
public:
    static const string EventPackage;
    static const string EventType;
    static const string NotePropertyName;
    
    typedef int Type; // not an enum, too much arithmetic at stake
    struct BadType {
        string type;
        BadType(string t = "") : type(t) { }
    };


    // define both sorts of names; some people prefer the American
    // names, but I just can't remember which of them is which

    static const Type

        SixtyFourthNote     = 0,
        ThirtySecondNote    = 1,
        SixteenthNote       = 2,
        EighthNote          = 3,
        QuarterNote         = 4,
        HalfNote            = 5,
        WholeNote           = 6,

        Hemidemisemiquaver  = 0,
        Demisemiquaver      = 1,
        Semiquaver          = 2,
        Quaver              = 3,
        Crotchet            = 4,
        Minim               = 5,
        Semibreve           = 6,
        Breve               = 7,

        Shortest            = 0,
        Longest             = 7;


    Note(Type type, bool dotted = false) throw (BadType) :
        m_type(type), m_dotted(dotted) {
        // this may really bugger up compiler optimisations for simple
        // uses of Note (e.g. "int d = Note(Crotchet, true).getDuration()"):
        if (m_type < Shortest || m_type > Longest) throw BadType();
    }

    Note(const string &s) throw (BadType);

    Note(const Note &n) : m_type(n.m_type), m_dotted(n.m_dotted) { }
    virtual ~Note() { }

    Note &operator=(const Note &n) {
        if (&n == this) return *this;
        m_type = n.m_type;
        m_dotted = n.m_dotted;
        return *this;
    }

    Type getType()      const { return m_type; }

    bool isFilled()     const { return m_type <= Crotchet; }
    bool isDotted()     const { return m_dotted; }
    bool isStalked()    const { return m_type <= Minim; }
    bool getTailCount() const {
        return m_type >= Crotchet ? 0 : Crotchet - m_type;
    }
    int  getDuration()  const {
        return (m_dotted ? m_dottedShortestTime : m_shortestTime) *
            (1 << m_type);
    }

    // these default to whatever I am:
    string getEnglishName(Type type = -1, bool dotted = false)  const;
    string getAmericanName(Type type = -1, bool dotted = false) const;
    string getShortName(Type type = -1, bool dotted = false)    const;

    static Note getNearestNote(int duration);
    static vector<int> getNoteDurationList(int start, int duration,
                                           const TimeSignature &ts);
  
private:
    Type m_type;
    bool m_dotted;
    static void makeTimeListSub(int time, bool dottedTime,
                                vector<int> &timeList);

    // a time & effort saving device
    static const int m_shortestTime;
    static const int m_dottedShortestTime;
    static const int m_crotchetTime;
    static const int m_dottedCrotchetTime;
};


class TimeSignature
{
public:
    static const string EventPackage;
    static const string EventType;
    static const string NumeratorPropertyName;
    static const string DenominatorPropertyName;
    static const TimeSignature DefaultTimeSignature;
    struct BadTimeSignature { };

    TimeSignature() : m_numerator(DefaultTimeSignature.m_numerator) ,
                      m_denominator(DefaultTimeSignature.m_denominator) { }

    TimeSignature(int numerator, int denominator)
        throw (BadTimeSignature) :
        m_numerator(numerator), m_denominator(denominator) {
        //!!! check, and throw BadTimeSignature if appropriate
    }

    TimeSignature(const Event &e)
        throw (Event::NoData, Event::BadType, BadTimeSignature) {
        if (e.package() != EventPackage || e.type() != EventType) {
            throw Event::BadType();
        }
        m_numerator = e.get<Int>(NumeratorPropertyName);
        m_denominator = e.get<Int>(DenominatorPropertyName);
        //!!! check, and throw BadTimeSignature if appropriate
    }

    TimeSignature(const TimeSignature &ts) :
        m_numerator(ts.m_numerator), m_denominator(ts.m_denominator) { }

    virtual ~TimeSignature() { }

    TimeSignature &operator=(const TimeSignature &ts) {
        if (&ts == this) return *this;
        m_numerator = ts.m_numerator;
        m_denominator = ts.m_denominator;
        return *this;
    }

    int getNumerator()    const { return m_numerator; }
    int getDenominator()  const { return m_denominator; }
    int getBarDuration()  const { return m_numerator * getUnitDuration(); }

    // We say the "unit" of the time is the duration of the note
    // implied by the denominator.  For example, the unit of 4/4 time
    // is the crotchet, and that of 6/8 is the quaver.  The numerator
    // of the time signature gives the number of units per bar.

    Note::Type getUnit()  const {
        int c, d;
        for (c = 0, d = m_denominator; d > 1; d /= 2) ++c;
        return Note::Semibreve - c;
    }
    int getUnitDuration() const { return 6 * (64 / m_denominator); }

    // The "beat" of the time depends on whether the signature implies
    // dotted or undotted time.  The beat of 4/4 time is the crotchet,
    // the same as its unit, but that of 6/8 is the dotted crotchet
    // (there are only two beats in a 6/8 bar).  We don't worry
    // ourselves with more complex times (7/16 anyone?) at the moment

    bool isDotted() const {
        return (m_numerator % 3 == 0 &&
                getBarDuration() >= Note(Note::Crotchet, true).getDuration());
    }
    int getBeatDuration() const {
        if (isDotted()) {
            return (getUnitDuration() * 3) / 2; //!!! is this always correct?
        } else {
            return  getUnitDuration();
        }
    }
    int getBeatsPerBar() const {
        return getBarDuration() / getBeatDuration();
    }

private:
    int m_numerator;
    int m_denominator;
};

#endif
