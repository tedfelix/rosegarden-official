
#include "notationgroup.h"
#include "notationproperties.h"
#include "rosedebug.h"
#include <cstring>

NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i) :
    m_nel(nel),
    m_type(Beamed)
{
    if (i == nel.end()) return;

    NotationElementList::IteratorPair pair
        (nel.findContainingSet<GroupMembershipTest>
         (i, GroupMembershipTest(i)));

    for (i = pair.first; i != pair.second; ++i) {
        push_back(i);
    }

    if (size() > 0) {

        i = (*this)[0];

        try {
            string t = (*i)->event()->get<String>("GroupType");
            if (strcasecmp(t.c_str(), "beamed")) {
                m_type = Beamed;
            } else if (strcasecmp(t.c_str(), "tupled")) {
                m_type = Tupled;
            } else if (strcasecmp(t.c_str(), "grace")) {
                m_type = Grace;
            } else {
                kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: Unknown GroupType \"" << t << "\", defaulting to Beamed" << endl;
            }
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: No GroupType in grouped element, defaulting to Beamed" << endl;
        }
    }
}

NotationGroup::~NotationGroup() { }

int
NotationGroup::height(const NELIterator &i, const Clef &clef, const Key &key)
{
    long h;
    if ((*i)->event()->get<Int>(P_HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    NotationDisplayPitch p(pitch, clef, key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(P_HEIGHT_ON_STAFF, h, false);
    return h;
}

NotationGroup::Beam
NotationGroup::calculateBeam(const NotePixmapFactory &npf,
                             const Clef &clef, const Key &key)
{
    iterator i;
    Beam beam;

    // First calculate whether the beam goes above or below the notes.
    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.

    int weightAbove = 0, weightBelow = 0;

    for (i = begin(); i != end(); ++i) {
        if (!(**i)->isNote()) continue;
        int h = height(*i, clef, key);
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

    for (i = begin(); i != end(); ++i) {
        Chord chord(m_nel, *i, true);
        if (chord.size() < 1) continue;

        // strictly perhaps not getLongestNote(), but in practice I
        // doubt if we'll ever really support chords that have notes
        // of differing lengths:

        Event::timeT d = (*chord.getLongestNote())->event()->getDuration();
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
        while (*i != chord.getFinalNote()) ++i;
    }

    

    return beam;
}


    

    
