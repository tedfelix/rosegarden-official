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

#ifndef _COMPOSITION_TIMESLICE_ADAPTER_H_
#define _COMPOSITION_TIMESLICE_ADAPTER_H_

#include <list>
#include <utility>

#include "Segment.h"

namespace Rosegarden {

using std::list;
using std::pair;

class Event;
class Composition;

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

/*!!!
    typedef std::list<Event *> Column;
    class columniterator;
*/

    CompositionTimeSliceAdapter(Composition* c,
				timeT begin = -1, timeT end = -1) :
	m_composition(c),
	m_begin(begin),
	m_end(end) { };
    ~CompositionTimeSliceAdapter() { };

    iterator begin();
    iterator end();

    /**
     * Returns a columniterator referencing the first column of the
     * timeslice.
     */
/*!!!
    columniterator beginColumns();
*/
    /**
     * Returns a past-the-end columniterator.
     */
/*!!!
    columniterator endColumns();
*/

    class iterator {
	friend class CompositionTimeSliceAdapter;
    public:
	iterator() : m_curEvent(0) { };
	~iterator() { };
	iterator& operator++();
	bool operator!=(const iterator& other);
	Event* operator*();
	Event* operator->();
    private:
	typedef std::pair<Segment*, Segment::iterator> position;
	typedef std::list<position> positionlist;
	positionlist m_positionList;
	Event*	m_curEvent;
	timeT 	m_end;
    };


private:
    Composition* m_composition;
    timeT m_begin;
    timeT m_end;
};

}

#endif
