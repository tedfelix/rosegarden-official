
#include "notationsets.h"
#include "notationproperties.h"
#include "rosedebug.h"
#include <cstring>

using Rosegarden::Event;
using Rosegarden::String;
using Rosegarden::Int;
using Rosegarden::Clef;
using Rosegarden::Key;

NotationSet::NotationSet(const NotationElementList &nel,
                         NELIterator i, bool quantized) :
    m_nel(nel),
    m_initial(nel.end()),
    m_final(nel.end()),
    m_shortest(nel.end()),
    m_longest(nel.end()),
    m_highest(nel.end()),
    m_lowest(nel.end()),
    m_quantized(quantized),
    m_baseIterator(i)
{
    // ...
}

void NotationSet::initialise()
{
    NELIterator &i(m_baseIterator);
    if (i == m_nel.end() || !test(i)) return;
    sample(i);

    NELIterator j;

    // first scan back to find an element not in the desired set, and
    // leave ipair.first pointing to the one after it

    for (j = i; i != m_nel.begin() && test(--j); i = j) {
        sample(j);
    }
    m_initial = i;

    // then scan forwards to find an element not in the desired set,
    // and leave ipair.second pointing to the one before it

    for (j = i; ++j != m_nel.end() && test(j); i = j) {
        sample(j);
    }
    m_final = i;
}

void NotationSet::sample(const NELIterator &i)
{
    if (m_longest == m_nel.end() ||
        duration(i, m_quantized) > duration(m_longest, m_quantized)) {
        m_longest = i;
    }
    if (m_shortest == m_nel.end() ||
        duration(i, m_quantized) < duration(m_shortest, m_quantized)) {
        m_shortest = i;
    }

    if ((*i)->isNote()) {
        long p = (*i)->event()->get<Int>("pitch");

        if (m_highest == m_nel.end() ||
            p > (*m_highest)->event()->get<Int>("pitch")) {
            m_highest = i;
        }
        if (m_lowest == m_nel.end() ||
            p < (*m_lowest)->event()->get<Int>("pitch")) {
            m_lowest = i;
        }
    }
}

Event::timeT NotationSet::duration(const NELIterator &i, bool quantized)
{
    if (quantized) {
        long d;
        bool done = (*i)->event()->get<Int>(P_QUANTIZED_DURATION, d);
        if (done) {
            return d;
        } else {
            Quantizer().quantize((*i)->event());
            return (*i)->event()->get<Int>(P_QUANTIZED_DURATION);
        }
    } else {
        return (*i)->event()->getDuration();
    }
}



class PitchGreater {
public:
    bool operator()(const NotationElementList::iterator &a,
                    const NotationElementList::iterator &b) {
        try {
            return ((*a)->event()->get<Int>("pitch") <
                    (*b)->event()->get<Int>("pitch"));
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "Bad karma: PitchGreater failed to find one or both pitches" << endl;
            return false;
        }
    }
};


Chord::Chord(const NotationElementList &nel, NELIterator i, bool quantized) :
    NotationSet(nel, i, quantized),
    m_time((*i)->getAbsoluteTime())
{
    initialise();

    if (size() > 1) {
        std::stable_sort(begin(), end(), PitchGreater());
    }

    kdDebug(KDEBUG_AREA) << "Chord::Chord: pitches are:" << endl;
    for (int i = 0; i < size(); ++i) {
        try {
            kdDebug(KDEBUG_AREA) << i << ": " << (*(*this)[i])->event()->get<Int>("pitch") << endl;
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << i << ": no pitch property" << endl;
        }
    }
}


NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i) :
    NotationSet(nel, i, false),
    m_type(Beamed)
{
    if (!(*i)->event()->get<Rosegarden::Int>("GroupNo", m_groupNo))
        m_groupNo = -1;

    initialise();
    
    if ((i = getInitialElement()) != getList().end()) {

        try {
            std::string t = (*i)->event()->get<String>("GroupType");
            if (strcasecmp(t.c_str(), "beamed")) {
                m_type = Beamed;
            } else if (strcasecmp(t.c_str(), "tupled")) {
                m_type = Tupled;
            } else if (strcasecmp(t.c_str(), "grace")) {
                m_type = Grace;
            } else {
                kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: Unknown GroupType \"" << t << "\", defaulting to Beamed" << endl;
            }
        } catch (Rosegarden::Event::NoData) {
            kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: No GroupType in grouped element, defaulting to Beamed" << endl;
        }
    }
}

void
NotationGroup::sample(const NELIterator &i)
{
    NotationSet::sample(i);
    
}

int
NotationGroup::height(const NELIterator &i, const Clef &clef, const Key &key)
{
    long h;
    if ((*i)->event()->get<Int>(P_HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    Rosegarden::NotationDisplayPitch p(pitch, clef, key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(P_HEIGHT_ON_STAFF, h, false);
    return h;
}

NotationGroup::Beam
NotationGroup::calculateBeam(const NotePixmapFactory &npf,
                             const Clef &clef, const Key &key,
                             int width)
{
    Beam beam;

    // Should be doing above/below sums in sample() so as to avoid
    // this first loop...

    NELIterator i;
    NELIterator begin = getInitialElement();
    NELIterator end = getFinalElement();
    ++end;

    // First calculate whether the beam goes above or below the notes.
    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.

    int weightAbove = 0, weightBelow = 0;

    for (i = begin; i != end; ++i) {
        if (!(*i)->isNote()) continue;
        int h = height(i, clef, key);
        if (h < 4) weightBelow += 4 - h;
        if (h > 4) weightAbove += h - 4;
    }

    beam.aboveNotes = !(weightAbove > weightBelow);
    beam.gradient = 0.0;
    beam.startHeight = npf.getStalkLength();

    // Now collect some pitch and duration data

    Event::timeT shortestDuration = 1000000;
    Event::timeT longestDuration = 0, totalDuration = 0;

    int topHeight = -1000, bottomHeight = 1000;
    int prevHeight, firstHeight = -1000, lastHeight;

    int angle = -2; // -2 = don't know, -1 = down, 0 = no direction, 1 = up

    for (i = begin; i != end; ++i) {
        Chord chord(getList(), i, true);
        if (chord.size() < 1) continue;

        // strictly perhaps not getLongestNote(), but in practice I
        // doubt if we'll ever really support chords that have notes
        // of differing lengths:
        Event *e = (*chord.getLongestElement())->event();

        Event::timeT d = e->getDuration();
        totalDuration += d;
        if (d < shortestDuration) shortestDuration = d;
        if (d >  longestDuration)  longestDuration = d;

        int h = (beam.aboveNotes ? 
                 height(chord.getHighestNote(), clef, key) :
                 height(chord.getLowestNote(),  clef, key));

        if (firstHeight == -1000) {
            firstHeight = prevHeight = h;
        }
        lastHeight = h;

        if (h >    topHeight)    topHeight = h;
        if (h < bottomHeight) bottomHeight = h;

        if (h < prevHeight) {
            if      (angle == -2) angle = -1;
            else if (angle ==  1) angle =  0;
        } else if (h > prevHeight) {
            if      (angle == -2) angle =  1;
            else if (angle == -1) angle =  0;
        }
        
        prevHeight = h;
        while (i != chord.getFinalElement()) ++i;
    }

    // shortestWidth is the screen width of shortest note in group,
    // used as a multiplier for widths; shortestDuration is the length
    // of that note (already calculated)
    int shortestWidth;
    if (totalDuration == 0 || width == 0) {
        shortestWidth = npf.getNoteBodyWidth() + 2;
    } else {
        shortestWidth = (width * shortestDuration) / totalDuration;
    }

    static double gradients[] = { 0.1, 0.17, 0.3 };
    int diff = firstHeight - lastHeight;
    if (diff < 0) diff = 0;

    if (angle == 0 || angle == -2) {    // nonincreasing, nondecreasing group
        if (diff > 2)       beam.gradient = gradients[0];
        else                beam.gradient = 0.0;
    } else {                                        // some overall direction
        if (diff > 4)       beam.gradient = gradients[2];
        else if (diff > 3)  beam.gradient = gradients[1];
        else                beam.gradient = gradients[0];
    }
    if (lastHeight > firstHeight) beam.gradient = -beam.gradient;

    if (beam.aboveNotes) {
        int nearestHeight = topHeight + 1;
        //???
    } //... else

    return beam;
}

