// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "notationsets.h"
#include "notationproperties.h"
#include "BaseProperties.h"
#include "rosedebug.h"
#include "notationstaff.h"
#include "Equation.h"
#include "Track.h"
#include "Quantizer.h"

#include <cstring>

using Rosegarden::Event;
using Rosegarden::String;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Track;
using Rosegarden::Equation;
using Rosegarden::Quantizer;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

NotationSet::NotationSet(const NotationElementList &nel, NELIterator i) :
    m_nel(nel),
    m_initial(nel.end()),
    m_final(nel.end()),
    m_shortest(nel.end()),
    m_longest(nel.end()),
    m_highest(nel.end()),
    m_lowest(nel.end()),
    m_baseIterator(i)
{
    // ...
}

void
NotationSet::initialise()
{
    if (m_baseIterator == m_nel.end() || !test(m_baseIterator)) return;
    sample(m_baseIterator);

    NELIterator i, j;

    // first scan back to find an element not in the desired set,
    // sampling everything as far back as the one after it

    for (i = j = m_baseIterator; i != m_nel.begin() && test(--j); i = j) {
        sample(j);
    }
    m_initial = i;

    j = m_baseIterator;

    // then scan forwards to find an element not in the desired set,
    // sampling everything as far forward as the one before it

    for (i = j = m_baseIterator; ++j != m_nel.end() && test(j); i = j) {
        sample(j);
    }
    m_final = i;
}

void
NotationSet::sample(const NELIterator &i)
{
    timeT d(durationOf(i));

    if (d > 0) {
        if (m_longest == m_nel.end() ||
            d > durationOf(m_longest)) {
            m_longest = i;
        }
        if (m_shortest == m_nel.end() ||
            d < durationOf(m_shortest)) {
            m_shortest = i;
        }
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

timeT
NotationSet::durationOf(const NELIterator &i)
{
    return (*i)->event()->get<Int>(Quantizer::LegatoDurationProperty);
}

NotationSet::NELIterator
NotationSet::getInitialNote() const
{
    NELIterator i(getInitialElement());
    if ((*i)->isNote()) return i;

    NELIterator j(getFinalElement());
    ++j;
    while (i != j) {
        if ((*i)->isNote()) return i;
        ++i;
    }

    return getList().end();
}

NotationSet::NELIterator
NotationSet::getFinalNote() const
{
    NELIterator i(getFinalElement());
    if ((*i)->isNote()) return i;

    NELIterator j(getInitialElement());
    --j;
    while (i != j) {
        if ((*i)->isNote()) return i;
        --i;
    }

    return getList().end();
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

//////////////////////////////////////////////////////////////////////

Chord::Chord(const NotationElementList &nel, NELIterator i,
             const Clef &clef, const Key &key) :
    NotationSet(nel, i),
    m_clef(clef),
    m_key(key),
    m_time((*i)->getAbsoluteTime())
{
    initialise();

    if (size() > 1) {
        std::stable_sort(begin(), end(), PitchGreater());
    }

/*
    kdDebug(KDEBUG_AREA) << "Chord::Chord: pitches are:" << endl;
    for (unsigned int i = 0; i < size(); ++i) {
        try {
            kdDebug(KDEBUG_AREA) << i << ": " << (*(*this)[i])->event()->get<Int>("pitch") << endl;
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << i << ": no pitch property" << endl;
        }
    }
*/
}

Chord::~Chord()
{
}

bool Chord::test(const NELIterator &i)
{
    return ((*i)->isNote() && ((*i)->getAbsoluteTime() == m_time));
}

void Chord::sample(const NELIterator &i)
{
    NotationSet::sample(i);
    push_back(i);
}

int
Chord::height(const NELIterator &i) const
{
    long h;
    if ((*i)->event()->get<Int>(HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(HEIGHT_ON_STAFF, h, false);
    return h;
}

bool Chord::hasStemUp() const
{
    int high = height(getHighestNote()), low = height(getLowestNote());

    if (high > 4) {
        if (low > 4) return false;
        else return ((high - 4) < (5 - low));
    } else return true;
}

bool Chord::hasShiftedNoteHeads() const
{
    int prevH = 10000;
    for (unsigned int i = 0; i < size(); ++i) {
        int h = height((*this)[i]);
        if (h == prevH + 1) return true;
        prevH = h;
    }
    return false;
}

bool Chord::contains(const NELIterator &itr) const
{
    for (const_iterator i = begin(); i != end(); ++i) {
        if (*i == itr) return true;
    }
    return false;
}

bool Chord::hasNoteHeadShifted() const
{
    int ph = 999;

    for (unsigned int i = 0; i < size(); ++i) {
        int h = height((*this)[i]);
        if (h == ph + 1) return true;
        ph = h;
    }

    return false;
}

bool Chord::isNoteHeadShifted(const NELIterator &itr) const
{
    unsigned int i;
    for (i = 0; i < size(); ++i) {
        if ((*this)[i] == itr) break;
    }
    if (i == size()) {
        kdDebug(KDEBUG_AREA) << "Chord::isNoteHeadShifted: Warning: Unable to find note head " << (*itr) << endl;
        return false;
    }

    int h = height((*this)[i]);

    if (hasStemUp()) {
        if ((i > 0) && (h == height((*this)[i-1]) + 1)) {
            return (!isNoteHeadShifted((*this)[i-1]));
        }
    } else {
        if ((i < size()-1) && (h == height((*this)[i+1]) - 1)) {
            return (!isNoteHeadShifted((*this)[i+1]));
        }
    }

    return false;
}


//////////////////////////////////////////////////////////////////////

NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i, const Clef &clef, const Key &key) :
    NotationSet(nel, i),
    //!!! What if the clef and/or key change in the course of the group?
    m_clef(clef),
    m_key(key),
    m_weightAbove(0),
    m_weightBelow(0),
    m_type(Beamed)
{
    if (!(*i)->event()->get<Rosegarden::Int>
        (BEAMED_GROUP_ID, m_groupNo)) m_groupNo = -1;

    initialise();
    
    if ((i = getInitialElement()) != getList().end()) {

        try {
            std::string t = (*i)->event()->get<String>(BEAMED_GROUP_TYPE);
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

    kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: id is " << m_groupNo << endl;
    i = getInitialElement(); 
    while (i != getList().end()) {
        long gid = -1;
        (*i)->event()->get<Int>(BEAMED_GROUP_ID, gid);
        kdDebug(KDEBUG_AREA) << "Found element with group id "
                             << gid << endl;
        if (i == getFinalElement()) break;
        ++i;
    }
}

NotationGroup::~NotationGroup()
{
}

bool NotationGroup::test(const NELIterator &i)
{
    long n;
    return ((*i)->event()->get<Rosegarden::Int>
            (BEAMED_GROUP_ID, n) && n == m_groupNo);
}

void
NotationGroup::sample(const NELIterator &i)
{
    NotationSet::sample(i);

    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.  We
    // can calculate the weightings here, as we construct the group.

    if (!(*i)->isNote()) return;

    // The code that uses the Group should not rely on the presence of
    // e.g. BEAM_GRADIENT to indicate that a beam should be drawn;
    // it's possible the gradient might be left over from a previous
    // calculation and the group might have changed since.  Instead it
    // should test BEAM_NECESSARY, which may be false even if there
    // is a gradient present.
    (*i)->event()->setMaybe<Bool>(BEAMED, false);

    int h = height(i);
    if (h > 4) m_weightAbove += h - 4;
    if (h < 4) m_weightBelow += 4 - h;
}

bool
NotationGroup::contains(const NELIterator &i) const
{
    NELIterator j(getInitialElement()),
                k(  getFinalElement());

    for (;;) {
        if (j == i) return true;
        if (j == k) return false;
        ++j;
    }
}

int
NotationGroup::height(const NELIterator &i) const
{
    long h;
    if ((*i)->event()->get<Int>(HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(HEIGHT_ON_STAFF, h, false);
    return h;
}

NotationGroup::Beam
NotationGroup::calculateBeam(NotationStaff &staff)
{
    Beam beam;
    beam.aboveNotes = !(m_weightAbove > m_weightBelow);
    beam.gradient = 0;
    beam.startY = 0;
    beam.necessary = false;
    
    NELIterator initialNote(getInitialNote()),
                  finalNote(  getFinalNote());

    if (initialNote == getList().end() ||
        initialNote == finalNote) {
        return beam; // no notes, no case to answer
    }

    timeT crotchet = Note(Note::Crotchet).getDuration();
    beam.necessary =
         (*initialNote)->event()->getDuration() < crotchet
        && (*finalNote)->event()->getDuration() < crotchet
        && (*finalNote)->getAbsoluteTime() > (*initialNote)->getAbsoluteTime();

    // We continue even if the beam is not necessary, because the
    // same data is used to generate the tupling line in tupled
    // groups that do not have beams

    // if (!beam.necessary) return beam;

    Chord initialChord(getList(), initialNote, m_clef, m_key),
            finalChord(getList(),   finalNote, m_clef, m_key);

    if (initialChord.getInitialElement() == finalChord.getInitialElement()) {
        return beam;
    }

    int initialHeight, finalHeight, extremeHeight;
    NELIterator extremeNote;

    if (beam.aboveNotes) {
        initialHeight = height(initialChord.getHighestNote());
          finalHeight = height(  finalChord.getHighestNote());
        extremeHeight = height(             getHighestNote());
        extremeNote = getHighestNote();

    } else {
        initialHeight = height(initialChord.getLowestNote());
          finalHeight = height(  finalChord.getLowestNote());
        extremeHeight = height(             getLowestNote());
        extremeNote = getLowestNote();
    }

    int diff = initialHeight - finalHeight;
    if (diff < 0) diff = -diff;

    bool linear =
        (beam.aboveNotes ?
         (extremeHeight <= std::max(initialHeight, finalHeight)) :
         (extremeHeight >= std::min(initialHeight, finalHeight)));

    if (!linear) {
        if (diff > 2) diff = 1;
        else diff = 0;
    }

    // some magic numbers
/*    if (diff > 4) beam.gradient = 25;
      else*/ if (diff > 3) beam.gradient = 18;
    else if (diff > 1) beam.gradient = 10;
    else beam.gradient = 0;

    if (initialHeight < finalHeight) beam.gradient = -beam.gradient;

    // Now, we need to judge the height of the beam such that the
    // nearest note of the whole group, the nearest note of the first
    // chord and the nearest note of the final chord are all at least
    // two note-body-heights away from it, and at least one of the
    // start and end points is at least the usual note stem-length
    // away from it.  This is a straight-line equation y = mx + c,
    // where we have m and two x,y pairs and need to find c.
    
    //!!! If we find that making one extreme a sensible distance from
    //the note head makes the other extreme way too far away from it
    //in the direction of the gradient, then we should flatten the
    //gradient.  There may be a better heuristic for this.

    int  initialX = (int)(*initialNote)->getLayoutX();
    int   finalDX = (int)  (*finalNote)->getLayoutX() - initialX;
    int extremeDX = (int)(*extremeNote)->getLayoutX() - initialX;

    int   finalY  = staff.yCoordOfHeight(finalHeight);
    int extremeY  = staff.yCoordOfHeight(extremeHeight);

    int c0 = staff.yCoordOfHeight(initialHeight), c1, c2;
    double dgrad = (double)beam.gradient / 100.0;

    Equation::solve(Equation::C, extremeY, dgrad, extremeDX, c1);
    Equation::solve(Equation::C, finalY,   dgrad,   finalDX, c2);

    using std::max;
    using std::min;
    long shortestNoteType = Note::Quaver;
    if (!(*getShortestElement())->event()->get<Int>(NOTE_TYPE,
                                                    shortestNoteType)) {
        kdDebug(KDEBUG_AREA) << "NotationGroup::calculateBeam: WARNING: Shortest element has no note-type; should this be possible?" << endl;
    }
    int nh = staff.getNotePixmapFactory().getNoteBodyHeight();
    int sl = staff.getNotePixmapFactory().getStemLength();

    if (beam.aboveNotes) {
        beam.startY = min(min(c0 - sl, c1 - nh*2), c2 - sl);
        if (shortestNoteType < Note::Quaver)
            beam.startY -= nh * (Note::Quaver - shortestNoteType) / 2;
    } else {
        beam.startY = max(max(c0 + sl, c1 + nh*2), c2 + sl);
        if (shortestNoteType < Note::Quaver)
	    beam.startY += nh * (Note::Quaver - shortestNoteType) / 2;
    }  

/*
    kdDebug(KDEBUG_AREA) << "NotationGroup::calculateBeam: beam data:" << endl
                         << "gradient: " << beam.gradient << endl
                         << "startY: " << beam.startY << endl
                         << "aboveNotes: " << beam.aboveNotes << endl
                         << "necessary: " << beam.necessary << endl;
*/
    return beam;
}


void
NotationGroup::applyBeam(NotationStaff &staff)
{
    kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam, group no is " << m_groupNo << endl;

    Beam beam(calculateBeam(staff));
    if (!beam.necessary) return;

    kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam: Beam is necessary" << endl;

    NELIterator initialNote(getInitialNote()),
	          finalNote(  getFinalNote());
    int initialX = (int)(*initialNote)->getLayoutX();

    // For each chord in the group, we nominate the note head furthest
    // from the beam as the one that "owns" the stem and the section
    // of beam up to the following chord.  For this note, we need to:
    // 
    // * Set the start height, start x-coord and gradient of the beam
    //   (we can't set the stem length for this note directly, because
    //   we don't know its y-coordinate yet)
    // 
    // * Set width of this section of beam
    // 
    // * Set the number of beams required for the following note (one
    //   slight complication here: a beamed group in which the very
    //   first chord is shorter than the following one.  Here the first
    //   chord needs to know it's the first, or else it can't draw the
    //   part-beams immediately to its right correctly.)
    //
    // For the rest of the notes in the chord, we just need to
    // indicate that they aren't part of the beam-drawing process and
    // don't need to draw a stem.

    NELIterator prev = getList().end(), prevprev = getList().end();
    double gradient = (double)beam.gradient / 100.0;

    for (NELIterator i = getInitialNote(); i != getList().end(); ++i) {

        if ((*i)->isNote()) {

	    Chord chord(getList(), i, m_clef, m_key);
	    unsigned int j;

            kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam: Found chord" << endl;

	    for (j = 0; j < chord.size(); ++j) {
		NotationElement *el = (*chord[j]);
		el->event()->setMaybe<Bool>(STEM_UP, beam.aboveNotes);
		el->event()->setMaybe<Bool>(DRAW_FLAG, false);
		el->event()->setMaybe<Bool>(BEAMED, true);
		el->event()->setMaybe<Bool>(BEAM_PRIMARY_NOTE, false);
	    }

	    if (!beam.aboveNotes) j = 0;
	    else j = chord.size() - 1;

	    NotationElement *el = (*chord[j]);
	    int x = (int)el->getLayoutX();

	    int myY = (int)(gradient * (x - initialX)) + beam.startY;

            // If THIS_PART_BEAMS is true, then when drawing the
            // chord, if it requires more beams than the following
            // chord then they should be added as partial beams to the
            // right of the stem.

            // If NEXT_PART_BEAMS is true, then when drawing the
            // chord, if it requires fewer beams than the following
            // chord then the difference should be added as partial
            // beams to the left of the following chord's stem.

            // Procedure for setting these: If we have more beams than
            // the preceding chord, then the preceding chord should
            // have NEXT_PART_BEAMS set, until possibly unset again on
            // the next iteration.  If we have at least as many beams
            // as the preceding chord, then the preceding chord should
            // have THIS_PART_BEAMS unset and the one before it should
            // have NEXT_PART_BEAMS unset.  The first chord should
            // have THIS_PART_BEAMS set, until possibly unset again on
            // the next iteration.

            int beamCount = Note(el->event()->get<Int>
                                 (NOTE_TYPE)).getFlagCount();

	    if (prev != getList().end()) {

                NotationElement *prevEl = (*prev);
		int secWidth = x - (int)prevEl->getLayoutX();

//		prevEl->event()->setMaybe<Int>(BEAM_NEXT_Y, myY);

		prevEl->event()->setMaybe<Int>
                    (BEAM_SECTION_WIDTH, secWidth);
		prevEl->event()->setMaybe<Int>
		    (BEAM_NEXT_BEAM_COUNT, beamCount);

                int prevBeamCount = Note(prevEl->event()->get<Int>
                                         (NOTE_TYPE)).getFlagCount();
                if (beamCount >= prevBeamCount) {
                    prevEl->event()->setMaybe<Bool>
                        (BEAM_THIS_PART_BEAMS, false);
                    if (prevprev != getList().end()) {
                        (*prevprev)->event()->setMaybe<Bool>
                            (BEAM_NEXT_PART_BEAMS, false);
                    }
                }

                if (beamCount > prevBeamCount) {
                    prevEl->event()->setMaybe<Bool>
                        (BEAM_NEXT_PART_BEAMS, true);
                }                    
	    } else {
                el->event()->setMaybe<Bool>(BEAM_THIS_PART_BEAMS, true);
            }

	    el->event()->setMaybe<Bool>(BEAM_PRIMARY_NOTE, true);

	    el->event()->setMaybe<Int>(BEAM_MY_Y, myY);
	    el->event()->setMaybe<Int>(BEAM_GRADIENT, beam.gradient);

	    // until they're set next time around the loop, as (*prev)->...
//	    el->event()->setMaybe<Int>(BEAM_NEXT_Y, myY);
	    el->event()->setMaybe<Int>(BEAM_SECTION_WIDTH, 0);
	    el->event()->setMaybe<Int>(BEAM_NEXT_BEAM_COUNT, 1);

            prevprev = prev;
	    prev = chord[j];
	    i = chord.getFinalElement();
        }

        if (i == finalNote) break;

        // We could miss the final note, if it was actually in the
        // middle of a chord (slightly pathological, but it happens
        // easily enough).  So let's check the group id too:
        long gid = -1;
        if (!(*i)->event()->get<Int>(BEAMED_GROUP_ID, gid)
            || gid != m_groupNo) break;
    }
}

void 
NotationGroup::applyTuplingLine(NotationStaff &staff)
{
    kdDebug(KDEBUG_AREA) << "NotationGroup::applyTuplingLine, group no is " << m_groupNo << endl;

    if (m_type != Tupled) return;

    Beam beam(calculateBeam(staff));

    NELIterator initialNote(getInitialNote()),
	          finalNote(  getFinalNote());
    int initialX = (int)(*initialNote)->getLayoutX();

    //!!!
    //...

}

