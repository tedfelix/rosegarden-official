// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "rosestrings.h"
#include "rosedebug.h"
#include "notationstaff.h"
#include "Equation.h"
#include "Segment.h"
#include "Quantizer.h"
#include "notestyle.h"

#include <cstring>

using Rosegarden::Event;
using Rosegarden::String;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Mark;
using Rosegarden::Marks;
using Rosegarden::Segment;
using Rosegarden::Equation;
using Rosegarden::Quantizer;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;


namespace Rosegarden {

template <>
Event *
AbstractSet<NotationElement, NotationElementList>::getAsEvent(const NotationElementList::iterator &i)
{
    return (*i)->event();
}

}

NotationChord::NotationChord(const NotationElementList &c,
			     NotationElementList::iterator i,
			     const Rosegarden::Quantizer *quantizer,
			     const NotationProperties &properties,
			     const Rosegarden::Clef &clef,
			     const Rosegarden::Key &key) :
    Rosegarden::GenericChord<NotationElement, NotationElementList>(c, i, quantizer,
							    clef, key),
    m_properties(properties)
{
    // nothing else 
}

int
NotationChord::getHeight(const Iterator &i) const
{
    //!!! We use HEIGHT_ON_STAFF in preference to the passed clef/key,
    //but what if the clef/key changed since HEIGHT_ON_STAFF was
    //written?  Who updates the properties then?  Check this.

    long h;
    if (getAsEvent(i)->get<Int>(NotationProperties::HEIGHT_ON_STAFF, h)) {
	return h;
    }

    int pitch = getAsEvent(i)->get<Int>(PITCH);
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();

    // set non-persistent, not setMaybe, as we know the property is absent:
    getAsEvent(i)->set<Int>(NotationProperties::HEIGHT_ON_STAFF, h, false);
    return h;
}

bool
NotationChord::hasStem() const
{
    // true if any of the notes is stemmed
    
    Iterator i(getInitialNote());
    for (;;) {
	Note::Type note = getAsEvent(i)->get<Int>(NOTE_TYPE);
	if (NoteStyleFactory::getStyleForEvent(getAsEvent(i))->hasStem(note))
	    return true;
	if (i == getFinalNote()) return false;
	++i;
    }
    return false;
}

bool
NotationChord::hasStemUp() const
{
    Iterator initialNote(getInitialNote());

    // believe whatever's recorded in the first note, if it's
    // persistent or if it was apparently set by the beaming algorithm

    Event *initialEvent = getAsEvent(initialNote);

    if (initialEvent->has(STEM_UP) &&
	(initialEvent->isPersistent<Bool>(STEM_UP) ||
	 (initialEvent->has(NotationProperties::BEAMED) &&
	  (initialEvent->get<Bool>(NotationProperties::BEAMED))))) {
	return initialEvent->get<Bool>(STEM_UP);
    }

    int high = getHeight(getHighestNote()), low = getHeight(getLowestNote());

    if (high > 4) {
        if (low > 4) return false;
        else return ((high - 4) < (5 - low));
    } else return true;
}

bool
NotationChord::hasNoteHeadShifted() const
{
    int ph = 10000;

    for (unsigned int i = 0; i < size(); ++i) {
        int h = getHeight((*this)[i]);
        if (h == ph + 1) return true;
        ph = h;
    }

    return false;
}

bool
NotationChord::isNoteHeadShifted(const Iterator &itr) const
{
    unsigned int i;
    for (i = 0; i < size(); ++i) {
        if ((*this)[i] == itr) break;
    }

    if (i == size()) {
        std::cerr << "NotationChord::isNoteHeadShifted: Warning: Unable to find note head " << getAsEvent(itr) << std::endl;
        return false;
    }

    int h = getHeight((*this)[i]);

    if (hasStemUp()) {
        if ((i > 0) && (h == getHeight((*this)[i-1]) + 1)) {
            return (!isNoteHeadShifted((*this)[i-1]));
        }
    } else {
        if ((i < size()-1) && (h == getHeight((*this)[i+1]) - 1)) {
            return (!isNoteHeadShifted((*this)[i+1]));
        }
    }

    return false;
}



//////////////////////////////////////////////////////////////////////

NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator i, const Quantizer *q,
			     std::pair<timeT, timeT> barRange,
			     const NotationProperties &p,
			     const Clef &clef, const Key &key) :
    Rosegarden::AbstractSet<NotationElement, NotationElementList>(nel, i, q),
    m_barRange(barRange),
    //!!! What if the clef and/or key change in the course of the group?
    m_clef(clef),
    m_key(key),
    m_weightAbove(0),
    m_weightBelow(0),
    m_userSamples(false),
    m_type(Beamed),
    m_properties(p)
{
    if (!(*i)->event()->get<Rosegarden::Int>
        (BEAMED_GROUP_ID, m_groupNo)) m_groupNo = -1;

    initialise();

    /*
    NOTATION_DEBUG << "NotationGroup::NotationGroup: id is " << m_groupNo << endl;
    i = getInitialElement(); 
    while (i != getContainer().end()) {
        long gid = -1;
        (*i)->event()->get<Int>(BEAMED_GROUP_ID, gid);
        NOTATION_DEBUG << "Found element with group id "
                             << gid << endl;
        if (i == getFinalElement()) break;
        ++i;
    }
    */
}

NotationGroup::NotationGroup(const NotationElementList &nel,
			     const Quantizer *q,
			     const NotationProperties &p,
			     const Clef &clef, const Key &key) :
    Rosegarden::AbstractSet<NotationElement, NotationElementList>(nel, nel.end(), q),
    m_barRange(0, 0),
    //!!! What if the clef and/or key change in the course of the group?
    m_clef(clef),
    m_key(key),
    m_weightAbove(0),
    m_weightBelow(0),
    m_userSamples(true),
    m_groupNo(-1),
    m_type(Beamed),
    m_properties(p)
{
    //...
}

NotationGroup::~NotationGroup()
{
}

bool NotationGroup::test(const NELIterator &i)
{
    // An event is a candidate for being within the bounds of the
    // set if it's simply within the same bar as the original event.
    // (Groups may contain other groups, so our bounds may enclose
    // events that aren't members of the group: we reject those in
    // sample rather than test, so as to avoid erroneously ending
    // the group too soon.)
    
    return ((*i)->getViewAbsoluteTime() >= m_barRange.first &&
	    (*i)->getViewAbsoluteTime() <  m_barRange.second);
}

bool
NotationGroup::sample(const NELIterator &i)
{
    if (m_baseIterator == getContainer().end()) {
	m_baseIterator = i;
	if (m_userSamples) m_initial = i;
    }
    if (m_userSamples) m_final = i;

    try {
	std::string t = (*i)->event()->get<String>(BEAMED_GROUP_TYPE);

	if (t == GROUP_TYPE_BEAMED) {
	    m_type = Beamed;
	} else if (t == GROUP_TYPE_TUPLED) {
	    m_type = Tupled;
	} else if (t == GROUP_TYPE_GRACE) {
	    m_type = Grace;
	} else {
	    NOTATION_DEBUG << "NotationGroup::NotationGroup: Warning: Unknown GroupType \"" << t << "\", defaulting to Beamed" << endl;
	}
    } catch (Rosegarden::Event::NoData) {
	NOTATION_DEBUG << "NotationGroup::NotationGroup: Warning: No GroupType in grouped element, defaulting to Beamed" << endl;
    }

    long n;
    if (!(*i)->event()->get<Rosegarden::Int>(BEAMED_GROUP_ID, n)) return false;
    if (m_groupNo == -1) {
	m_groupNo = n;
    } else if (n != m_groupNo) {
	NOTATION_DEBUG << "NotationGroup::NotationGroup: Warning: Rejecting sample() for event with group id " << n << " (mine is " << m_groupNo << ")" << endl;
	return false;
    }

//    NOTATION_DEBUG << "NotationGroup::sample: group id is " << m_groupNo << endl;

    Rosegarden::AbstractSet<NotationElement, NotationElementList>::sample(i);

    // If the sum of the distances from the middle line to the notes
    // above the middle line exceeds the sum of the distances from the
    // middle line to the notes below, then the beam goes below.  We
    // can calculate the weightings here, as we construct the group.

    if (!static_cast<NotationElement*>(*i)->isNote()) return true;
    if (m_userSamples) {
	if (m_initialNote == getContainer().end()) m_initialNote = i;
	m_finalNote = i;
    }

    // The code that uses the Group should not rely on the presence of
    // e.g. BEAM_GRADIENT to indicate that a beam should be drawn;
    // it's possible the gradient might be left over from a previous
    // calculation and the group might have changed since.  Instead it
    // should test BEAM_NECESSARY, which may be false even if there
    // is a gradient present.
    (*i)->event()->setMaybe<Bool>(NotationProperties::BEAMED, false);

    int h = height(i);
    if (h > 4) m_weightAbove += h - 4;
    if (h < 4) m_weightBelow += 4 - h;

    return true;
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
    if ((*i)->event()->get<Int>(NotationProperties::HEIGHT_ON_STAFF, h)) {
	return h;
    }
    int pitch = (*i)->event()->get<Int>(PITCH);
    Rosegarden::NotationDisplayPitch p(pitch, m_clef, m_key);
    h = p.getHeightOnStaff();
    // not setMaybe, as we know the property is absent:
    (*i)->event()->set<Int>(NotationProperties::HEIGHT_ON_STAFF, h, false);
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

    if (initialNote == getContainer().end() ||
        initialNote == finalNote) {
        return beam; // no notes, no case to answer
    }

    if ((*initialNote)->event()->has(STEM_UP) &&
	(*initialNote)->event()->isPersistent<Bool>(STEM_UP)) {
	beam.aboveNotes = (*initialNote)->event()->get<Bool>(STEM_UP);
    }

    timeT crotchet = Note(Note::Crotchet).getDuration();
    beam.necessary =
         (*initialNote)->getViewDuration() < crotchet
        && (*finalNote)->getViewDuration() < crotchet
	&& (*finalNote)->getViewAbsoluteTime() >
	 (*initialNote)->getViewAbsoluteTime();
/*!!!
        && getQuantizer().getQuantizedAbsoluteTime(  (*finalNote)->event()) >
	   getQuantizer().getQuantizedAbsoluteTime((*initialNote)->event());
*/

    // We continue even if the beam is not necessary, because the
    // same data is used to generate the tupling line in tupled
    // groups that do not have beams

    // if (!beam.necessary) return beam;

    NotationChord initialChord(getContainer(), initialNote, &getQuantizer(),
			       m_properties, m_clef, m_key),
                    finalChord(getContainer(),   finalNote, &getQuantizer(),
			       m_properties, m_clef, m_key);

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

    int   finalY  = staff.getLayoutYForHeight(finalHeight);
    int extremeY  = staff.getLayoutYForHeight(extremeHeight);

    int c0 = staff.getLayoutYForHeight(initialHeight), c1, c2;
    double dgrad = (double)beam.gradient / 100.0;

    Equation::solve(Equation::C, extremeY, dgrad, extremeDX, c1);
    Equation::solve(Equation::C, finalY,   dgrad,   finalDX, c2);

    using std::max;
    using std::min;
    long shortestNoteType = Note::Quaver;
    if (!(*getShortestElement())->event()->get<Int>(NOTE_TYPE,
                                                    shortestNoteType)) {
        NOTATION_DEBUG << "NotationGroup::calculateBeam: WARNING: Shortest element has no note-type; should this be possible?" << endl;
	NOTATION_DEBUG << "(Event dump follows)" << endl;
	(*getShortestElement())->event()->dump(std::cerr);
    }
    int nh = staff.getNotePixmapFactory(m_type == Grace).getNoteBodyHeight();
    int sl = staff.getNotePixmapFactory(m_type == Grace).getStemLength();

    if (beam.aboveNotes) {
        beam.startY = min(min(c0 - sl, c1 - nh*2), c2 - sl);
        if (shortestNoteType < Note::Quaver)
            beam.startY -= nh * (Note::Quaver - shortestNoteType);
    } else {
        beam.startY = max(max(c0 + sl, c1 + nh*2), c2 + sl);
        if (shortestNoteType < Note::Quaver)
	    beam.startY += nh * (Note::Quaver - shortestNoteType);
    }  

/*
    NOTATION_DEBUG << "NotationGroup::calculateBeam: beam data:" << endl
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
//    NOTATION_DEBUG << "NotationGroup::applyBeam, group no is " << m_groupNo << endl;
/*!!!
    NOTATION_DEBUG << "\nNotationGroup::applyBeam" << endl;
    NOTATION_DEBUG << "Group id: " << m_groupNo << ", type " << m_type << endl;
    NOTATION_DEBUG << "Coverage:" << endl;
    int i = 0;
    for (NELIterator i = getInitialElement(); i != getContainer().end(); ++i) {
	(*i)->event()->dump(std::cerr);
	if (i == getFinalElement()) break;
    }
    {
	NELIterator i(getInitialNote());
	NOTATION_DEBUG << "Initial note: " << (i == getContainer().end() ? -1 : (*i)->event()->getAbsoluteTime()) << endl;
    }
    {
	NELIterator i(getFinalNote());
	NOTATION_DEBUG << "Final note: " << (i == getContainer().end() ? -1 : (*i)->event()->getAbsoluteTime()) << endl;
    }
    {
	NELIterator i(getHighestNote());
	NOTATION_DEBUG << "Highest note: " << (i == getContainer().end() ? -1 : (*i)->event()->getAbsoluteTime()) << endl;
    }
    {
	NELIterator i(getLowestNote());
	NOTATION_DEBUG << "Lowest note: " << (i == getContainer().end() ? -1 : (*i)->event()->getAbsoluteTime()) << endl;
    }
*/

    Beam beam(calculateBeam(staff));
    if (!beam.necessary) {
	for (NELIterator i = getInitialNote(); i != getContainer().end(); ++i) {
	    (*i)->event()->unset(NotationProperties::BEAMED);
	    if (i == getFinalNote()) break;
	}
	return;
    }

//    NOTATION_DEBUG << "NotationGroup::applyBeam: Beam is necessary" << endl;

    NELIterator initialNote(getInitialNote()),
	          finalNote(  getFinalNote());
    int initialX = (int)(*initialNote)->getLayoutX();
    timeT finalTime = (*finalNote)->getViewAbsoluteTime();

    // For each chord in the group, we nominate the note head furthest
    // from the beam as the primary note, the one that "owns" the stem
    // and the section of beam up to the following chord.  For this
    // note, we need to:
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

    NELIterator prev = getContainer().end(), prevprev = getContainer().end();
    double gradient = (double)beam.gradient / 100.0;

//    NOTATION_DEBUG << "NotationGroup::applyBeam starting for group "<< this << endl;

    for (NELIterator i = getInitialNote(); i != getContainer().end(); ++i) {
        NotationElement* el = static_cast<NotationElement*>(*i);
        
        if (el->isNote() &&
	    el->event()->get<Int>(NOTE_TYPE) < Note::Crotchet &&
	    el->event()->has(BEAMED_GROUP_ID) &&
	    el->event()->get<Int>(BEAMED_GROUP_ID) == m_groupNo) {

	    NotationChord chord(getContainer(), i, &getQuantizer(), 
				m_properties, m_clef, m_key);
	    unsigned int j;

//            NOTATION_DEBUG << "NotationGroup::applyBeam: Found chord" << endl;

	    for (j = 0; j < chord.size(); ++j) {
		NotationElement *el = static_cast<NotationElement*>(*chord[j]);

		el->event()->setMaybe<Bool>
		    (STEM_UP, beam.aboveNotes);

		el->event()->setMaybe<Bool>
		    (m_properties.CHORD_PRIMARY_NOTE, false);

		el->event()->setMaybe<Bool>
		    (m_properties.DRAW_FLAG, false);

		el->event()->setMaybe<Bool>
		    (NotationProperties::BEAMED, true);
	    }

	    if (beam.aboveNotes) j = 0;
	    else j = chord.size() - 1;

	    NotationElement *el = static_cast<NotationElement*>(*chord[j]);
	    el->event()->setMaybe<Bool>(NotationProperties::BEAMED, false); // set later
	    el->event()->setMaybe<Bool>(m_properties.DRAW_FLAG, true); // set later
	    
	    int x = (int)el->getLayoutX();
	    int myY = (int)(gradient * (x - initialX)) + beam.startY;

	    int beamCount =
		NoteStyleFactory::getStyleForEvent(el->event())->
		getFlagCount(el->event()->get<Int>(NOTE_TYPE));

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

	    if (prev != getContainer().end()) {

                NotationElement *prevEl = static_cast<NotationElement*>(*prev);
		int secWidth = x - (int)prevEl->getLayoutX();

//		prevEl->event()->setMaybe<Int>(BEAM_NEXT_Y, myY);

		prevEl->event()->setMaybe<Int>
                    (m_properties.BEAM_SECTION_WIDTH, secWidth);
		prevEl->event()->setMaybe<Int>
		    (m_properties.BEAM_NEXT_BEAM_COUNT, beamCount);

		int prevBeamCount =
		    NoteStyleFactory::getStyleForEvent(prevEl->event())->
		    getFlagCount(prevEl->event()->get<Int>(NOTE_TYPE));

		if ((beamCount > 0) && (prevBeamCount > 0)) {
		    el->event()->setMaybe<Bool>(m_properties.BEAMED, true);
		    el->event()->setMaybe<Bool>(m_properties.DRAW_FLAG, false);
		    prevEl->event()->setMaybe<Bool>(m_properties.BEAMED, true);
		    prevEl->event()->setMaybe<Bool>(m_properties.DRAW_FLAG, false);
		}

		if (beamCount >= prevBeamCount) {
                    prevEl->event()->setMaybe<Bool>
                        (m_properties.BEAM_THIS_PART_BEAMS, false);
                    if (prevprev != getContainer().end()) {
                        (*prevprev)->event()->setMaybe<Bool>
                            (m_properties.BEAM_NEXT_PART_BEAMS, false);
                    }
                }

		if (beamCount > prevBeamCount) {
                    prevEl->event()->setMaybe<Bool>
                        (m_properties.BEAM_NEXT_PART_BEAMS, true);
                }

	    } else {
                el->event()->setMaybe<Bool>(m_properties.BEAM_THIS_PART_BEAMS, true);
            }

	    el->event()->setMaybe<Bool>(m_properties.CHORD_PRIMARY_NOTE, true);

	    el->event()->setMaybe<Int>(m_properties.BEAM_MY_Y, myY);
	    el->event()->setMaybe<Int>(m_properties.BEAM_GRADIENT, beam.gradient);

	    // until they're set next time around the loop, as (*prev)->...
//	    el->event()->setMaybe<Int>(m_properties.BEAM_NEXT_Y, myY);
	    el->event()->setMaybe<Int>(m_properties.BEAM_SECTION_WIDTH, 0);
	    el->event()->setMaybe<Int>(m_properties.BEAM_NEXT_BEAM_COUNT, 1);

            prevprev = prev;
	    prev = chord[j];
	    i = chord.getFinalElement();

        } else if (el->isNote()) {
	    
	    if (i == initialNote || i == finalNote) {
		(*i)->event()->setMaybe<Bool>(STEM_UP, beam.aboveNotes);
	    } else {
		(*i)->event()->setMaybe<Bool>(STEM_UP, !beam.aboveNotes);
	    }
	}

        if (i == finalNote || el->getViewAbsoluteTime() > finalTime) break;
    }
}

void 
NotationGroup::applyTuplingLine(NotationStaff &staff)
{
//    NOTATION_DEBUG << "NotationGroup::applyTuplingLine, group no is " << m_groupNo << ", group type is " << m_type << endl;

    if (m_type != Tupled) return;

//    NOTATION_DEBUG << "NotationGroup::applyTuplingLine: line is necessary" << endl;

    Beam beam(calculateBeam(staff));

    NELIterator initialNote(getInitialNote()),
	          finalNote(  getFinalNote()),

   	     initialElement(getInitialElement()),
	       finalElement(  getFinalElement());

    NELIterator initialNoteOrRest(initialElement);
    NotationElement* initialNoteOrRestEl = static_cast<NotationElement*>(*initialNoteOrRest);

    while (initialNoteOrRest != finalElement &&
	   !(initialNoteOrRestEl->isNote() || 
	     initialNoteOrRestEl->isRest())) {
	++initialNoteOrRest;
        initialNoteOrRestEl = static_cast<NotationElement*>(*initialNoteOrRest);
    }

    if (!initialNoteOrRestEl->isRest()) {
        initialNoteOrRest = initialNote;
        initialNoteOrRestEl = static_cast<NotationElement*>(*initialNoteOrRest);
    }
    
    if (initialNoteOrRest == staff.getViewElementList()->end()) return;

    NOTATION_DEBUG << "NotationGroup::applyTuplingLine: first element is " << (initialNoteOrRestEl->isNote() ? "Note" : "Non-Note") << ", last is " << (static_cast<NotationElement*>(*finalElement)->isNote() ? "Note" : "Non-Note") << endl;

    int initialX = (int)(*initialNoteOrRest)->getLayoutX();
    int   finalX = (int)(*finalElement)->getLayoutX();

    if (initialNote == staff.getViewElementList()->end() &&
	  finalNote == staff.getViewElementList()->end()) {

	Event *e = (*initialNoteOrRest)->event();
	e->setMaybe<Int>(m_properties.TUPLING_LINE_MY_Y,
			 staff.getLayoutYForHeight(12));
	e->setMaybe<Int>(m_properties.TUPLING_LINE_WIDTH, finalX - initialX);
	e->setMaybe<Int>(m_properties.TUPLING_LINE_GRADIENT, 0);

    } else {
    
	// only notes have height
	int initialY = staff.getLayoutYForHeight(height(initialNote));
	int   finalY = staff.getLayoutYForHeight(height(  finalNote));

	int   startY = initialY - (beam.startY - initialY);
	int     endY =   startY + (int)((finalX - initialX) *
					((double)beam.gradient / 100.0));

	int nh = staff.getNotePixmapFactory(m_type == Grace).getNoteBodyHeight();
	if (startY < initialY) {
	    if (initialY - startY > nh * 3) startY  = initialY - nh * 3;
	    if (  finalY -   endY < nh * 2) startY -= nh * 2 - (finalY - endY);
	} else {
	    if (startY - initialY > nh * 3) startY  = initialY + nh * 3;
	    if (  endY -   finalY < nh * 2) startY += nh * 2 - (endY - finalY);
	}	
	
	Event *e = (*initialNoteOrRest)->event();
	e->setMaybe<Int>(m_properties.TUPLING_LINE_MY_Y, startY);
	e->setMaybe<Int>(m_properties.TUPLING_LINE_WIDTH, finalX - initialX);
	e->setMaybe<Int>(m_properties.TUPLING_LINE_GRADIENT, beam.gradient);
    }
}

