// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include "Clipboard.h"
#include "Selection.h"

namespace Rosegarden
{

Clipboard::Clipboard() :
    m_partial(false),
    m_haveTimeSigSelection(false),
    m_haveTempoSelection(false)
{
    // nothing
}

Clipboard::Clipboard(const Clipboard &c) :
    m_partial(false)
{
    copyFrom(&c);
}

Clipboard &
Clipboard::operator=(const Clipboard &c)
{
    copyFrom(&c);
    return *this;
}

Clipboard::~Clipboard()
{
    clear();
}

void
Clipboard::clear()
{
    for (iterator i = begin(); i != end(); ++i) {
	delete *i;
    }
    m_segments.clear();
    clearTimeSignatureSelection();
    clearTempoSelection();
    m_partial = false;
}

bool
Clipboard::isEmpty() const
{
    return (m_segments.size() == 0 &&
	    !m_haveTimeSigSelection &&
	    !m_haveTempoSelection);
}

bool
Clipboard::isSingleSegment() const
{
    return (m_segments.size() == 1 &&
	    !m_haveTimeSigSelection &&
	    !m_haveTempoSelection);
}

Segment *
Clipboard::getSingleSegment() const
{
    if (isSingleSegment()) return *begin();
    else return 0;
}

bool
Clipboard::isPartial() const
{
    return m_partial;
}

Segment *
Clipboard::newSegment()
{
    Segment *s = new Segment();
    m_segments.insert(s);
    // don't change m_partial
    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom)
{
    Segment *s = new Segment(*copyFrom);
    m_segments.insert(s);
    // don't change m_partial
    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom, timeT from, timeT to)
{
    // create with copy ctor so as to inherit track, instrument etc
    Segment *s = new Segment(*copyFrom);

    if (from <= s->getStartTime() && to >= s->getEndTime()) {
	m_segments.insert(s);
	// don't change m_partial
	return s;
    }

    if (s->getType() == Segment::Audio) {
	
	Composition *c = copyFrom->getComposition();
	if (from > s->getStartTime()) {
	    if (c) {
		s->setAudioStartTime
		    (s->getAudioStartTime() +
		     c->getRealTimeDifference(s->getStartTime(),
					      from));
	    }
	    s->setStartTime(from);
	}
	if (to < copyFrom->getEndMarkerTime()) {
	    s->setEndMarkerTime(to);
	    if (c) {
		s->setAudioEndTime
		    (s->getAudioStartTime() +
		     c->getRealTimeDifference(s->getStartTime(),
					      to));
	    }
	}
	m_segments.insert(s);

	return s;
    }

    s->erase(s->begin(), s->end());

    Segment::const_iterator ifrom = copyFrom->findTime(from);
    Segment::const_iterator ito   = copyFrom->findTime(to);

    for (Segment::const_iterator i = ifrom;
	 i != ito && i != copyFrom->end(); ++i) {

	if ((*i)->getAbsoluteTime() + (*i)->getDuration() > to) {
	    s->insert(new Event(**i,
				(*i)->getAbsoluteTime(),
				to - (*i)->getAbsoluteTime()));
	} else {
	    s->insert(new Event(**i));
	}
    }

    // need to call getEndMarkerTime() on copyFrom, not on s, because
    // its return value may depend on the composition it's in
    if (copyFrom->getEndMarkerTime() > to) {
	s->setEndMarkerTime(to);
    }

    m_segments.insert(s);
    m_partial = true;
    return s;
}

Segment *
Clipboard::newSegment(const EventSelection *copyFrom)
{
    // create with copy ctor so as to inherit track, instrument etc
    Segment *s = new Segment(copyFrom->getSegment());
    s->erase(s->begin(), s->end());

    const EventSelection::eventcontainer &events(copyFrom->getSegmentEvents());
    for (EventSelection::eventcontainer::const_iterator i = events.begin();
	 i != events.end(); ++i) {
	s->insert(new Event(**i));
    }

    m_segments.insert(s);
    m_partial = true;
    return s;
}

void
Clipboard::setTimeSignatureSelection(const TimeSignatureSelection &ts)
{
    m_timeSigSelection = ts;
    m_haveTimeSigSelection = true;
}

void
Clipboard::clearTimeSignatureSelection()
{
    m_timeSigSelection = TimeSignatureSelection();
    m_haveTimeSigSelection = false;
}

const TimeSignatureSelection &
Clipboard::getTimeSignatureSelection() const
{
    return m_timeSigSelection;
}
 
void
Clipboard::setTempoSelection(const TempoSelection &ts)
{
    m_tempoSelection = ts;
    m_haveTempoSelection = true;
}

void
Clipboard::clearTempoSelection()
{
    m_tempoSelection = TempoSelection();
    m_haveTempoSelection = false;
}

const TempoSelection &
Clipboard::getTempoSelection() const
{
    return m_tempoSelection;
}
    
void
Clipboard::copyFrom(const Clipboard *c)
{
    if (c == this) return;
    clear();

    for (Clipboard::const_iterator i = c->begin(); i != c->end(); ++i) {
	newSegment(*i);
    }

    m_partial = c->m_partial;

    m_timeSigSelection = c->m_timeSigSelection;
    m_haveTimeSigSelection = c->m_haveTimeSigSelection;

    m_tempoSelection = c->m_tempoSelection;
    m_haveTempoSelection = c->m_haveTempoSelection;
}

timeT
Clipboard::getBaseTime() const
{
    timeT t = 0;
    for (iterator i = begin(); i != end(); ++i) {
	if (i == begin() || (*i)->getStartTime() < t) {
	    t = (*i)->getStartTime();
	}
    }

    if (m_haveTimeSigSelection && !m_timeSigSelection.empty()) {
	if (m_timeSigSelection.begin()->first < t) {
	    t = m_timeSigSelection.begin()->first;
	}
    }

    if (m_haveTempoSelection && !m_tempoSelection.empty()) {
	if (m_tempoSelection.begin()->first < t) {
	    t = m_tempoSelection.begin()->first;
	}
    }
    
    return t;
}

}
