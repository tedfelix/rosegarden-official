// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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

// This file is Copyright 2002 Randall Farmer <rfarme@simons-rock.edu>

// !!!TODO: handle timeslices

#include <list>
#include <utility>

#include "CompositionTimeSliceAdapter.h"
#include "Segment.h"
#include "Composition.h"
#include "Selection.h"

namespace Rosegarden {

using std::list;
using std::pair;

CompositionTimeSliceAdapter::CompositionTimeSliceAdapter(Composition *c,
							 timeT begin,
							 timeT end) :
    m_composition(c),
    m_segments(0),
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }
};

CompositionTimeSliceAdapter::CompositionTimeSliceAdapter(Composition *c,
							 SegmentSelection* s,
							 timeT begin,
							 timeT end) :
    m_composition(c),
    m_segments(s),
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }
};

CompositionTimeSliceAdapter::CompositionTimeSliceAdapter(Composition *c,
							 const TrackSet &trackIDs,
							 timeT begin,
							 timeT end) :
    m_composition(c),
    m_segments(0),
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }

    m_segments = new SegmentSelection();

    for (Composition::iterator ci = m_composition->begin();
         ci != m_composition->end(); ++ci) {
	
	if (trackIDs.find((*ci)->getTrack()) != trackIDs.end()) {
	    m_segments->insert(*ci);
	}
    }
};

CompositionTimeSliceAdapter::iterator
CompositionTimeSliceAdapter::begin() {
    iterator i;

    // Fill m_positionList with segment pointers and segment::iterators.

    // The segment iterators should all point to events starting at or
    // after m_begin.
    for (Composition::iterator ci = m_composition->begin();
         ci != m_composition->end(); ++ci) {

	if (m_segments && m_segments->find(*ci) == m_segments->end()) continue;

        if (!(*ci)->empty() && (*ci)->getEndTime() >= m_begin) {
            Segment::iterator j = (*ci)->findTime(m_begin);

            if (j != (*ci)->end()) {
                i.m_positionList.push_front(iterator::position(*ci, j));
            }

        }

    }

    // Fill m_curEvent and m_end.
    ++i;
    i.m_end = m_end;

    return i;
}

CompositionTimeSliceAdapter::iterator
CompositionTimeSliceAdapter::end() {
    return iterator();
}

CompositionTimeSliceAdapter::iterator&
CompositionTimeSliceAdapter::iterator::operator++() {
    if (m_positionList.empty()) {
        // We're done.
        m_curEvent = 0;
        return *this;
    }

    // these are only "next" at the top of the function:
    Event *nextEvent = 0;
    positionlist::iterator nextPosition;

    for (positionlist::iterator i = m_positionList.begin();
         i != m_positionList.end(); ++i) {

        if (nextEvent == 0 || **i->second < *nextEvent) {
            nextEvent = *i->second;
            nextPosition = i;
        }

    }

    // Check whether we're past the end time, if there is one
    if (m_end > 0 && nextEvent->getAbsoluteTime() > m_end) {
        m_curEvent = 0;
        return *this;
    }

    // nextEvent is now an Event* less than or equal to any that the
    // iterator hasn't already passed over
    m_curEvent = nextEvent;

    // nextPosition->second is a segment::iterator that points to nextEvent
    ++(nextPosition->second);

    if (nextPosition->second == nextPosition->first->end()) {
        m_positionList.erase(nextPosition);
    }

    return *this;
}

bool
CompositionTimeSliceAdapter::iterator::operator!=(const iterator& other) {
    return m_curEvent != other.m_curEvent;
}

Event*
CompositionTimeSliceAdapter::iterator::operator*() {
    return m_curEvent;
}

Event*
CompositionTimeSliceAdapter::iterator::operator->() {
    return m_curEvent;
}


}
