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

#ifndef _COMPOSITION_TIMESLICE_ADAPTER_H_
#define _COMPOSITION_TIMESLICE_ADAPTER_H_

#include <list>
#include <utility>

#include "Segment.h"

namespace Rosegarden {

class Event;
class Composition;
class SegmentSelection;

/**
 * CompositionTimeSliceAdapter makes a Composition act like a sorted
 * list of all the events in a timeslice. Unlike with Segment, you can
 * only iterate from begin() to end() -- there's no random access or
 * decrement.
 */

class CompositionTimeSliceAdapter
{
public:
    class iterator;
    typedef std::set<TrackId> TrackSet;

    /**
     * Construct a CompositionTimeSliceAdapter that operates on the
     * given section in time of the given composition.  If begin and
     * end are equal, the whole composition will be used.
     */
    CompositionTimeSliceAdapter(Composition* c,
				timeT begin = 0,
				timeT end = 0);

    /**
     * Construct a CompositionTimeSliceAdapter that operates on the
     * given section in time of the given set of segments within the
     * given composition.  If begin and end are equal, the whole
     * duration of the composition will be used.
     */
    CompositionTimeSliceAdapter(Composition* c,
				SegmentSelection* s,
				timeT begin = 0,
				timeT end = 0);

    /**
     * Construct a CompositionTimeSliceAdapter that operates on the
     * given section in time of all the segments in the given set of
     * tracks within the given composition.  If begin and end are
     * equal, the whole duration of the composition will be used.
     */
    CompositionTimeSliceAdapter(Composition *c,
				const TrackSet &trackIDs,
				timeT begin = 0,
				timeT end = 0);

    ~CompositionTimeSliceAdapter() { };

    // bit sloppy -- we don't have a const_iterator
    iterator begin() const;
    iterator end() const;

    typedef std::vector<Segment *> segmentlist;
    typedef std::vector<Segment::iterator> segmentitrlist;

    Composition *getComposition() { return m_composition; }

    class iterator {
	friend class CompositionTimeSliceAdapter;

    public:
	iterator() :
	    m_a(0), m_curEvent(0), m_curTrack(-1), m_needFill(true) { }
	iterator(const CompositionTimeSliceAdapter *a) :
	    m_a(a), m_curEvent(0), m_curTrack(-1), m_needFill(true) { }
	iterator(const iterator &);
	iterator &operator=(const iterator &);
	~iterator() { };

	iterator &operator++();
	iterator &operator--();

	bool operator==(const iterator& other) const;
	bool operator!=(const iterator& other) const;

	Event *operator*() const;
	Event &operator->() const;

	int getTrack() const;

    private:
	segmentitrlist m_segmentItrList;
	const CompositionTimeSliceAdapter *m_a;
	Event*	m_curEvent;
	int     m_curTrack;
	bool    m_needFill;

	static bool strictLessThan(Event *, Event *);
    };


private:
    friend class iterator;

    Composition* m_composition;
    mutable iterator m_beginItr;
    timeT m_begin;
    timeT m_end;

    segmentlist m_segmentList;

    void fill(iterator &, bool atEnd) const;
};

}

#endif
