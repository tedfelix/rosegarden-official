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

    iterator begin();
    iterator end();

    class iterator {
	friend class CompositionTimeSliceAdapter;
    public:
	iterator() : m_curEvent(0), m_end(0) { };
	~iterator() { };
	iterator& operator++();
	bool operator!=(const iterator& other);
	Event* operator*();
	Event* operator->();
	int getTrack();
    private:
	typedef std::pair<Segment*, Segment::iterator> position;
	typedef std::list<position> positionlist;
	positionlist m_positionList;
	Event*	m_curEvent;
	int     m_curTrack;
	timeT 	m_end;
    };


private:
    Composition* m_composition;
    SegmentSelection* m_segments;
    timeT m_begin;
    timeT m_end;
};

}

#endif
