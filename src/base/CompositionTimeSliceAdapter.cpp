/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file is Copyright 2002
        Randall Farmer      <rfarme@simons-rock.edu>
	with additional work by Chris Cannam.

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
#include "base/Segment.h"
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
}

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
}

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
}

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

    for (size_t k = 0; k < m_segmentList.size(); ++k) {
	Segment::iterator j = m_segmentList[k]->findTime(atEnd ? m_end : m_begin);
	i.m_segmentItrList.push_back(j);
    }

    // fill m_curEvent & m_curTrack
    if (!atEnd) ++i;
}

CompositionTimeSliceAdapter::iterator&
CompositionTimeSliceAdapter::iterator::operator=(const iterator &i)
{
    if (&i == this) return *this;
    m_segmentItrList.clear();

    for (segmentitrlist::const_iterator j = i.m_segmentItrList.begin(); 
	 j != i.m_segmentItrList.end(); ++j) {
	m_segmentItrList.push_back(Segment::iterator(*j));
    }

    m_a = i.m_a;
    m_curTrack = i.m_curTrack;
    m_curEvent = i.m_curEvent;
    m_needFill = i.m_needFill;
    return *this;
}

CompositionTimeSliceAdapter::iterator::iterator(const iterator &i) :
    m_a(i.m_a),
    m_curEvent(i.m_curEvent),
    m_curTrack(i.m_curTrack),
    m_needFill(i.m_needFill)
{
    for (segmentitrlist::const_iterator j = i.m_segmentItrList.begin(); 
	 j != i.m_segmentItrList.end(); ++j) {
	m_segmentItrList.push_back(Segment::iterator(*j));
    }
}

CompositionTimeSliceAdapter::iterator&
CompositionTimeSliceAdapter::iterator::operator++()
{
    assert(m_a != 0);

    // needFill is only set true for iterators created at end()
    if (m_needFill) {
	m_a->fill(*this, true);
	m_needFill = false;
    }

    Event *e = 0;
    size_t pos = 0;

    for (size_t i = 0; i < m_a->m_segmentList.size(); ++i) {

	if (!m_a->m_segmentList[i]->isBeforeEndMarker(m_segmentItrList[i])) continue;

        if (!e || strictLessThan(*m_segmentItrList[i], e)) {
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
    if (m_needFill) {
	m_a->fill(*this, true);
	m_needFill = false;
    }

    Event *e = 0;
    int pos = -1;

    // Decrement is more subtle than increment.  We have to scan the
    // iterators available, and decrement the one that points to
    // m_curEvent.  Then to fill m_curEvent we need to find the next
    // greatest event back that is not itself m_curEvent.

    for (size_t i = 0; i < m_a->m_segmentList.size(); ++i) {

	if (m_segmentItrList[i] == m_a->m_segmentList[i]->begin()) continue;

	Segment::iterator si(m_segmentItrList[i]);
	--si;

	if (*si == m_curEvent) {
	    pos = i;
	} else if (!e || !strictLessThan(*si, e)) {
	    e = *si;
	    m_curTrack = m_a->m_segmentList[i]->getTrack();
	}
    }

    if (e) m_curEvent = e;
    if (pos >= 0) {
	--m_segmentItrList[pos];
    }

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

bool
CompositionTimeSliceAdapter::iterator::strictLessThan(Event *e1, Event *e2) {
    // We need a complete ordering of events -- we can't cope with two events
    // comparing equal.  i.e. one of e1 < e2 and e2 < e1 must be true.  The
    // ordering can be arbitrary -- we just compare addresses for events the
    // event comparator doesn't distinguish between.  We know we're always
    // dealing with event pointers, not copies of events.
    if (*e1 < *e2) return true;
    else if (*e2 < *e1) return false;
    else return e1 < e2;
}

}
