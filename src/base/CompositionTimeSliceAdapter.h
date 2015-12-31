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

#ifndef RG_COMPOSITION_TIMESLICE_ADAPTER_H
#define RG_COMPOSITION_TIMESLICE_ADAPTER_H

#include <list>
#include <utility>

#include "base/Segment.h"
#include "base/Selection.h"

namespace Rosegarden {


class Event;
class Composition;


/**
 * CompositionTimeSliceAdapter provides the ability to iterate through
 * all the events in a Composition in time order, across many segments
 * at once.
 *
 * The CompositionTimeSliceAdapter is suitable for use as the backing
 * container for the Set classes, notably GenericChord (see Sets.h).
 * This combination enables you to iterate through a Composition as a
 * sequence of chords composed of all Events on a set of Segments that
 * lie within a particular quantize range of one another.
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
        Event*  m_curEvent;
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
