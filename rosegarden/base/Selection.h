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

#ifndef SELECTION_H
#define SELECTION_H

#include <set>
#include "PropertyName.h"
#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "Composition.h"

namespace Rosegarden {

/**
 * EventSelection records a (possibly non-contiguous) selection
 * of Events in a single Segment, used for cut'n paste operations.
 * It does not take a copy of those Events, it just remembers
 * which ones they are.
 */

class EventSelection : public SegmentObserver
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventcontainer;

    /**
     * Construct an empty EventSelection based on the given Segment.
     */
    EventSelection(Segment &);

    /**
     * Construct an EventSelection selecting all the events in the
     * given range of the given Segment.  Set overlap if you want 
     * to include Events overlapping the selection edges.
     */
    EventSelection(Segment &, timeT beginTime, timeT endTime, bool overlap = false);

    EventSelection(const EventSelection&);

    virtual ~EventSelection();

    /**
     * Add an Event to the selection.  The Event should come from
     * the Segment that was passed to the constructor.  Will
     * silently drop any event that is already in the selection.
     */
    void addEvent(Event* e);

    /**
     * Add all the Events in the given Selection to this one.
     * Will silently drop any events that are already in the
     * selection.
     */
    void addFromSelection(EventSelection *sel);

    /**
     * If the given Event is in the selection, take it out.
     */
    void removeEvent(Event *e);

    /**
     * Test whether a given Event (in the Segment) is part of
     * this selection.
     */
    bool contains(Event *e) const;

    /**
     * Return true if there are any events of the given type in
     * this selection.  Slow.
     */
    bool contains(const std::string &eventType) const;

    /**
     * Return the time at which the first Event in the selection
     * begins.
     */
    timeT getStartTime() const { return m_beginTime; }

    /**
     * Return the time at which the last Event in the selection ends.
     */
    timeT getEndTime() const { return m_endTime; }

    /**
     * Return the total duration spanned by the selection.
     */
    timeT getTotalDuration() const;

    typedef std::vector<std::pair<Segment::iterator,
                                  Segment::iterator> > RangeList;
    /**
     * Return a set of ranges spanned by the selection, such that
     * each range covers only events within the selection.
     */
    RangeList getRanges() const;

    typedef std::vector<std::pair<timeT, timeT> > RangeTimeList;
    /**
     * Return a set of times spanned by the selection, such that
     * each time range covers only events within the selection.
     */
    RangeTimeList getRangeTimes() const;

    /**
     * Return the number of events added to this selection.
     */
    unsigned int getAddedEvents() const { return m_segmentEvents.size(); }

    const eventcontainer &getSegmentEvents() const { return m_segmentEvents; }
    eventcontainer &getSegmentEvents()		   { return m_segmentEvents; }

    const Segment &getSegment() const { return m_originalSegment; }
    Segment &getSegment()             { return m_originalSegment; }

    // SegmentObserver methods
    virtual void eventAdded(const Segment *, Event *) { }
    virtual void eventRemoved(const Segment *, Event *);
    virtual void endMarkerTimeChanged(const Segment *, bool) { }
    virtual void segmentDeleted(const Segment *);
    
private:
    EventSelection &operator=(const EventSelection &);

protected:
    //--------------- Data members ---------------------------------

    Segment& m_originalSegment;

    /// pointers to Events in the original Segment
    eventcontainer m_segmentEvents;

    timeT m_beginTime;
    timeT m_endTime;
    bool m_haveRealStartTime;
};


/**
 * SegmentSelection is much simpler than EventSelection, we don't
 * need to do much with this really
 */

class SegmentSelection : public std::set<Segment *>
{
public:
    bool hasNonAudioSegment() const;
};


/**
 * A selection that includes (only) time signatures.  Unlike
 * EventSelection, this does copy its contents, not just refer to
 * them.
 */
class TimeSignatureSelection
{
public:
    /**
     * Construct an empty TimeSignatureSelection.
     */
    TimeSignatureSelection();

    /**
     * Construct a TimeSignatureSelection containing all the time
     * signatures in the given range of the given Composition.
     */
    TimeSignatureSelection(Composition &, timeT beginTime, timeT endTime);

    virtual ~TimeSignatureSelection();

    /**
     * Add a time signature to the selection.
     */
    void addTimeSignature(timeT t, TimeSignature timeSig);
    
    typedef std::multimap<timeT, TimeSignature> timesigcontainer;

    const timesigcontainer &getTimeSignatures() const { return m_timeSignatures; }
    timesigcontainer::const_iterator begin() const { return m_timeSignatures.begin(); }
    timesigcontainer::const_iterator end() const { return m_timeSignatures.end(); }
    bool empty() const { return begin() == end(); }

protected:
    timesigcontainer m_timeSignatures;
};
 

/**
 * A selection that includes (only) tempo changes.
 */

class TempoSelection
{
public:
    /**
     * Construct an empty TempoSelection.
     */
    TempoSelection();

    /**
     * Construct a TempoSelection containing all the time
     * signatures in the given range of the given Composition.
     */
    TempoSelection(Composition &, timeT beginTime, timeT endTime);

    virtual ~TempoSelection();

    /**
     * Add a time signature to the selection.
     */
    void addTempo(timeT t, tempoT tempo, tempoT targetTempo = -1);

    typedef std::pair<tempoT, tempoT> tempochange;
    typedef std::multimap<timeT, tempochange> tempocontainer;

    const tempocontainer &getTempos() const { return m_tempos; }
    tempocontainer::const_iterator begin() const { return m_tempos.begin(); }
    tempocontainer::const_iterator end() const { return m_tempos.end(); }
    bool empty() const { return begin() == end(); }

protected:
    tempocontainer m_tempos;
};
    


}

#endif
