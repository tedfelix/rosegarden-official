
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
#include "rosedebug.h"
#include "staff.h"
#include "Equation.h"
#include <cstring>

using Rosegarden::Event;
using Rosegarden::String;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Equation;

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
    Event::timeT d(durationOf(i, m_quantized));

    if (d > 0) {
        if (m_longest == m_nel.end() ||
            d > durationOf(m_longest, m_quantized)) {
            m_longest = i;
        }
        if (m_shortest == m_nel.end() ||
            d < durationOf(m_shortest, m_quantized)) {
            m_shortest = i;
        }
    }

    if ((*i)->isNote()) {
        long p = (*i)->event()->get<Int>("pitch");

	kdDebug(KDEBUG_AREA) << "NotationSet::sample: sampling pitch " << p << endl;

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

Event::timeT
NotationSet::durationOf(const NELIterator &i, bool quantized)
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


Chord::Chord(const NotationElementList &nel, NELIterator i, bool quantized) :
    NotationSet(nel, i, quantized),
    m_time((*i)->getAbsoluteTime())
{
    initialise();

    if (size() > 1) {
        std::stable_sort(begin(), end(), PitchGreater());
    }

    kdDebug(KDEBUG_AREA) << "Chord::Chord: pitches are:" << endl;
    for (unsigned int i = 0; i < size(); ++i) {
        try {
            kdDebug(KDEBUG_AREA) << i << ": " << (*(*this)[i])->event()->get<Int>("pitch") << endl;
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << i << ": no pitch property" << endl;
        }
    }
}


NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i, const Clef &clef, const Key &key) :
    NotationSet(nel, i, false),
    //!!! What if the clef and/or key change in the course of the group?
    m_clef(clef),
    m_key(key),
    m_weightAbove(0),
    m_weightBelow(0),
    m_type(Beamed)
{
    if (!(*i)->event()->get<Rosegarden::Int>(P_GROUP_NO, m_groupNo))
        m_groupNo = -1;

    initialise();
    
    if ((i = getInitialElement()) != getList().end()) {

        try {
            std::string t = (*i)->event()->get<String>(P_GROUP_TYPE);
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

    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.  We
    // can calculate the weightings here, as we construct the group.

    if (!(*i)->isNote()) return;

    // The code that uses the Group should not rely on the presence of
    // e.g. P_BEAM_GRADIENT to indicate that a beam should be drawn;
    // it's possible the gradient might be left over from a previous
    // calculation and the group might have changed since.  Instead it
    // should test P_BEAM_NECESSARY, which may be false even if there
    // is a gradient present.
    (*i)->event()->setMaybe<Bool>(P_BEAMED, false);

    int h = height(i);
    if (h > 4) m_weightAbove += h - 4;
    if (h < 4) m_weightBelow += 4 - h;
}

int
NotationGroup::height(const NELIterator &i)
{
    long h;
    if ((*i)->event()->get<Int>(P_HEIGHT_ON_STAFF, h)) return h;
    int pitch = (*i)->event()->get<Int>("pitch");
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(P_HEIGHT_ON_STAFF, h, false);
    return h;
}

NotationGroup::Beam
NotationGroup::calculateBeam(Staff &staff)
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

    Chord initialChord(getList(), initialNote),
            finalChord(getList(),   finalNote);

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
    if (diff > 4) beam.gradient = 30;
    else if (diff > 3) beam.gradient = 15; // was 17
    else if (diff > 0) beam.gradient = 10;
    else beam.gradient = 0;

    if (initialHeight < finalHeight) beam.gradient = -beam.gradient;

    // Now, we need to judge the height of the beam such that the
    // nearest note of the whole group, the nearest note of the first
    // chord and the nearest note of the final chord are all at least
    // two note-body-heights away from it, and at least one of the
    // start and end points is at least the usual note stalk-length
    // away from it.  This is a straight-line equation y = mx + c,
    // where we have m and two x,y pairs and need to find c.
    
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
    if (!(*getShortestElement())->event()->get<Int>(P_NOTE_TYPE,
                                                    shortestNoteType)) {
        kdDebug(KDEBUG_AREA) << "NotationGroup::calculateBeam: WARNING: Shortest element has no note-type; should this be possible?" << endl;
    }
    int nh = staff.getNotePixmapFactory().getNoteBodyHeight();

    if (beam.aboveNotes) {
        beam.startY =
            min(min(c0 - nh*3, c1 - nh*2), c2 - nh*3);
//                min(c2 - nh*2, max(c0 - nh*4, finalY - nh*4)));
        if (shortestNoteType < Note::Quaver)
            beam.startY -= nh * (Note::Quaver - shortestNoteType) / 2;
    } else {
        beam.startY =
            max(max(c0 + nh*3, c1 + nh*2), c2 + nh*3);
//                max(c2 + nh*2, min(c0 + nh*3, finalY + nh*3)));
        if (shortestNoteType < Note::Quaver)
	    beam.startY += nh * (Note::Quaver - shortestNoteType) / 2;
    }  

    Event::timeT crotchet = Note(Note::Crotchet).getDuration();
    beam.necessary =
         (*initialNote)->event()->getDuration() < crotchet
        && (*finalNote)->event()->getDuration() < crotchet;

    kdDebug(KDEBUG_AREA) << "NotationGroup::calculateBeam: beam data:" << endl
                         << "gradient: " << beam.gradient << endl
                         << "startY: " << beam.startY << endl
                         << "aboveNotes: " << beam.aboveNotes << endl
                         << "necessary: " << beam.necessary << endl;

    return beam;
}


void
NotationGroup::applyBeam(Staff &staff)
{
    Beam beam(calculateBeam(staff));
    if (!beam.necessary) return;

    NELIterator initialNote(getInitialNote()),
	          finalNote(  getFinalNote());
    int initialX = (int)(*initialNote)->getLayoutX();

    // For each chord in the group, we nominate the note head furthest
    // from the beam as the one that "owns" the stalk and the section
    // of beam up to the following chord.  For this note, we need to:
    // 
    // * Set the start height, start x-coord and gradient of the beam
    //   (we can't set the stalk length for this note directly, because
    //   we don't know its y-coordinate yet)
    // 
    // * Set width of this section of beam
    // 
    // * Set the number of tails required for the following note (one
    //   slight complication here: a beamed group in which the very
    //   first chord is shorter than the following one.  Here the first
    //   chord needs to know it's the first, or else it can't draw the
    //   part-beams immediately to its right correctly.  We won't deal
    //   with this case just yet...)
    //
    // For the rest of the notes in the chord, we just need to
    // indicate that they aren't part of the beam-drawing process and
    // don't need to draw a stalk.

    NELIterator prev = getList().end();
    double gradient = (double)beam.gradient / 100.0;

    for (NELIterator i = getInitialNote(); i != getList().end(); ++i) {

        if ((*i)->isNote()) {

	    Chord chord(getList(), i);
	    unsigned int j;

	    for (j = 0; j < chord.size(); ++j) {
		NotationElement *el = (*chord[j]);
		el->event()->setMaybe<Bool>(P_STALK_UP, beam.aboveNotes);
		el->event()->setMaybe<Bool>(P_DRAW_TAIL, false);
		el->event()->setMaybe<Bool>(P_BEAMED, true);
		el->event()->setMaybe<Bool>(P_BEAM_PRIMARY_NOTE, false);
	    }

//	    if (beam.aboveNotes) j = 0;
//	    else j = chord.size() - 1;
	    // let's try this the other way around for the moment
	    if (!beam.aboveNotes) j = 0;
	    else j = chord.size() - 1;

	    NotationElement *el = (*chord[j]);
	    int x = (int)el->getLayoutX();

	    int myY = (int)(gradient * (x - initialX)) + beam.startY;

	    if (prev != getList().end()) {
		int secWidth = x - (int)(*prev)->getLayoutX();
//		(*prev)->event()->setMaybe<Int>(P_BEAM_NEXT_Y, myY);
		(*prev)->event()->setMaybe<Int>(P_BEAM_SECTION_WIDTH, secWidth);

		int noteType = el->event()->get<Int>(P_NOTE_TYPE);
		kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam: note type is " << noteType << endl;
		int tailCount = Note(noteType).getTailCount();
		kdDebug(KDEBUG_AREA) << "NotationGroup::applyBeam: setting next tail count to " << tailCount << endl;
		(*prev)->event()->setMaybe<Int>
		    (P_BEAM_NEXT_TAIL_COUNT, tailCount);
	    }

	    el->event()->setMaybe<Bool>(P_BEAM_PRIMARY_NOTE, true);

	    el->event()->setMaybe<Int>(P_BEAM_MY_Y, myY);
	    el->event()->setMaybe<Int>(P_BEAM_GRADIENT, beam.gradient);

	    // until they're set next time around the loop, as (*prev)->...
//	    el->event()->setMaybe<Int>(P_BEAM_NEXT_Y, myY);
	    el->event()->setMaybe<Int>(P_BEAM_SECTION_WIDTH, 0);
	    el->event()->setMaybe<Int>(P_BEAM_NEXT_TAIL_COUNT, 1);

	    prev = chord[j];
	    i = chord.getFinalElement();
        }

        if (i == finalNote) break;
    }
}


