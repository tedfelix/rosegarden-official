// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Randall Farmer      <rfarme@simons-rock.edu>
	with additional work by Chris Cannam.

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

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
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }

    for (Composition::iterator ci = m_composition->begin();
         ci != m_composition->end(); ++ci) {
	m_segmentList.push_back(*ci);
    }
};

CompositionTimeSliceAdapter::CompositionTimeSliceAdapter(Composition *c,
							 SegmentSelection* s,
							 timeT begin,
							 timeT end) :
    m_composition(c),
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }

    for (Composition::iterator ci = m_composition->begin();
         ci != m_composition->end(); ++ci) {
	if (!s || s->find(*ci) != s->end()) {
	    m_segmentList.push_back(*ci);
	}
    }
};

CompositionTimeSliceAdapter::CompositionTimeSliceAdapter(Composition *c,
							 const TrackSet &trackIDs,
							 timeT begin,
							 timeT end) :
    m_composition(c),
    m_begin(begin),
    m_end(end)
{
    if (begin == end) {
	m_begin = 0;
	m_end = c->getDuration();
    }

    for (Composition::iterator ci = m_composition->begin();
         ci != m_composition->end(); ++ci) {
	if (trackIDs.find((*ci)->getTrack()) != trackIDs.end()) {
	    m_segmentList.push_back(*ci);
	}
    }
};

CompositionTimeSliceAdapter::iterator
CompositionTimeSliceAdapter::begin() const
{
    if (m_beginItr.m_a == 0) {
	m_beginItr = iterator(this);
	fill(m_beginItr, false);
    }
    return m_beginItr;
}

CompositionTimeSliceAdapter::iterator
CompositionTimeSliceAdapter::end() const
{
    return iterator(this);
}

void
CompositionTimeSliceAdapter::fill(iterator &i, bool atEnd) const
{
    // The segment iterators should all point to events starting at or
    // after m_begin (if atEnd false) or at or before m_end (if atEnd true).

    for (unsigned int k = 0; k < m_segmentList.size(); ++k) {
	Segment::iterator j = m_segmentList[k]->findTime(atEnd ? m_end : m_begin);
	i.m_segmentItrList.push_back(j);
    }

    i.m_needFill = false;

    // fill m_curEvent & m_curTrack
    ++i;
}

CompositionTimeSliceAdapter::iterator&
CompositionTimeSliceAdapter::iterator::operator++()
{
    assert(m_a != 0);

    // needFill is only set true for iterators created at end()
    if (m_needFill) m_a->fill(*this, true);

    Event *e = 0;
    unsigned int pos = 0;

    for (unsigned int i = 0; i < m_a->m_segmentList.size(); ++i) {

	if (!m_a->m_segmentList[i]->isBeforeEndMarker(m_segmentItrList[i])) continue;

        if (!e || **m_segmentItrList[i] < *e) {
            e = *m_segmentItrList[i];
	    m_curTrack = m_a->m_segmentList[i]->getTrack();
            pos = i;
        }
    }

    // Check whether we're past the end time, if there is one
    if (!e || e->getAbsoluteTime() >= m_a->m_end) {
        m_curEvent = 0;
	m_curTrack = -1;
        return *this;
    }

    // e is now an Event* less than or equal to any that the iterator
    // hasn't already passed over
    m_curEvent = e;

    // m_segmentItrList[pos] is a segment::iterator that points to e
    ++m_segmentItrList[pos];

    return *this;
}

CompositionTimeSliceAdapter::iterator&
CompositionTimeSliceAdapter::iterator::operator--()
{
    assert(m_a != 0);

    // needFill is only set true for iterators created at end()
    if (m_needFill) m_a->fill(*this, true);

    Event *e = 0;
    unsigned int pos = 0;

    for (unsigned int i = 0; i < m_a->m_segmentList.size(); ++i) {

	if (m_segmentItrList[i] == m_a->m_segmentList[i]->begin()) continue;

	Segment::iterator si(m_segmentItrList[i]);
	--si;

	if (!e || **si < *e) {
	    e = *si;
	    m_curTrack = m_a->m_segmentList[i]->getTrack();
	    pos = i;
	}
    }

    // Check whether we're past the begin time, if there is one
    if (!e || e->getAbsoluteTime() < m_a->m_begin) {
        m_curEvent = 0;
	m_curTrack = -1;
        return *this;
    }

    // e is now an Event* greater than or equal to any that the iterator
    // hasn't already passed over in this direction
    m_curEvent = e;

    // m_segmentItrList[pos] is a segment::iterator that points to e
    --m_segmentItrList[pos];

    return *this;
}

bool
CompositionTimeSliceAdapter::iterator::operator==(const iterator& other) const {
    return m_a == other.m_a && m_curEvent == other.m_curEvent;
}

bool
CompositionTimeSliceAdapter::iterator::operator!=(const iterator& other) const {
    return !operator==(other);
}

Event *
CompositionTimeSliceAdapter::iterator::operator*() const {
    return m_curEvent;
}

Event &
CompositionTimeSliceAdapter::iterator::operator->() const {
    return *m_curEvent;
}

int
CompositionTimeSliceAdapter::iterator::getTrack() const {
    return m_curTrack;
}

}
