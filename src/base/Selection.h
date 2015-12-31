/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SELECTION_H
#define SELECTION_H

#include <set>
#include "Event.h"
#include "base/Segment.h"
#include "base/NotationTypes.h"
#include "Composition.h"

namespace Rosegarden {

class EventSelection;	
	
class EventSelectionObserver {
public:
    virtual ~EventSelectionObserver();
    virtual void eventSelected(EventSelection *e,Event *)=0;
    virtual void eventDeselected(EventSelection *e,Event *)=0; 
    virtual void eventSelectionDestroyed(EventSelection *e)=0;
};
	
/**
 * EventSelection records a (possibly non-contiguous) selection of the Events
 * that are contained in a single Segment, used for cut'n paste operations.  It
 * does not take a copy of those Events, it just remembers which ones they are.
 */

class ROSEGARDENPRIVATE_EXPORT EventSelection : public SegmentObserver
{
public:
    typedef EventContainer eventcontainer;

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

    bool operator==(const EventSelection &);

    /**
     *
     */
    void addObserver(EventSelectionObserver *obs);

    void removeObserver(EventSelectionObserver *obs);

    /**
     * Add an Event to the selection.  The Event should come from
     * the Segment that was passed to the constructor.  Will
     * silently drop any event that is already in the selection.
     */
    void addEvent(Event* e, bool ties = true);

    /**
     * Add all the Events in the given Selection to this one.
     * Will silently drop any events that are already in the
     * selection.
     */
    void addFromSelection(EventSelection *sel, bool ties = true);

    /**
     * If the given Event is in the selection, take it out.
     */
    void removeEvent(Event *e, bool ties = true);

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

    /**
     * Return the earliest notation absolute time of any event in the
     * selection.
     */
    timeT getNotationStartTime() const;

    /**
     * Return the latest notation absolute time plus duration of any
     * event in the selection.
     */
    timeT getNotationEndTime() const;

    /**
     * Return the total notation duration spanned by the selection.
     */
    timeT getTotalNotationDuration() const;

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

    const EventContainer &getSegmentEvents() const { return m_segmentEvents; }
    EventContainer &getSegmentEvents()             { return m_segmentEvents; }

    const Segment &getSegment() const { return m_originalSegment; }
    Segment &getSegment()             { return m_originalSegment; }

    // SegmentObserver methods
    virtual void eventAdded(const Segment *, Event *) { }
    virtual void eventRemoved(const Segment *, Event *);
    virtual void endMarkerTimeChanged(const Segment *, bool) { }
    virtual void segmentDeleted(const Segment *);
    
    // Debug
    void dump() const;

private:
    EventSelection &operator=(const EventSelection &);
  
    /**
     * Function Pointer to allow insertion or erasure of Event from Selection..
     */
    typedef void (EventSelection::*EventFuncPtr)(Event *e);
    
    /**
     * Inserts the Event into the selection set and calls the observers.
     */
    void insertThisEvent(Event *e);
    
    /**
     * Erases the Event from the selection container and calls the observers.
     */
    void eraseThisEvent(Event *e);
        
    /**
     * This method encapsulates all of the logic needed to add and remove events
     * from the selection set.
     */
    void addRemoveEvent(Event *e, EventFuncPtr insertEraseFn,
                        bool ties = true);
        
    typedef std::list<EventSelectionObserver *> ObserverSet;
    ObserverSet m_observers;
    
protected:
    //--------------- Data members ---------------------------------

    Segment& m_originalSegment;

    /// pointers to Events in the original Segment
    EventContainer m_segmentEvents;

    timeT m_beginTime;
    timeT m_endTime;
    bool m_haveRealStartTime;
};


/**
 * SegmentSelection is much simpler than EventSelection, we don't
 * need to do much with this really
 */
typedef std::set<Segment *> SegmentSelection;

/**
 * Template for a selection that includes only elements of type
 * ElementInfo::value_type, indexed by time.  Unlike EventSelection,
 * this does copy its contents, not just refer to them.
 *
 */
template <typename ElementInfo>
class TimewiseSelection
{
public:
    typedef typename ElementInfo::value_type Element;
    typedef std::multiset<Element> Container;

    virtual ~TimewiseSelection() {}

    /**
     * Add an element to the selection
     */
    void addRaw(const Element & element)
    { m_contents.insert(element); }

    /**
     * Add a copy of an element to the selection, offset by time t.
     */
    void addCopyAtOffset(timeT offset, const Element &element) {
        timeT t = ElementInfo::getTime(element);
        addRaw(ElementInfo::copyAtTime(t + offset, element));
    }
    
    const Container &getContents() const { return m_contents; }
    typename Container::const_iterator begin() const
        { return m_contents.begin(); }
    typename Container::const_iterator end() const
        { return m_contents.end(); }
    bool empty() const { return m_contents.empty(); }
    void RemoveFromComposition(Composition *composition);
    void AddToComposition(Composition *composition);

protected:
    Container m_contents;
};

// TimeSignatureSelection and TempoSelection could be realized as
// descendants of TimewiseSelection, reducing their code a great deal.

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
     *
     * If includeOpeningTimeSig is true, the selection will start with
     * a duplicate of the time signature (if any) that is already in
     * force at beginTime.  Otherwise the selection will only start
     * with a time signature at beginTime if there is an explicit
     * signature there in the source composition.
     */
    TimeSignatureSelection(Composition &, timeT beginTime, timeT endTime,
                           bool includeOpeningTimeSig);

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
    void RemoveFromComposition(Composition *composition);
    void AddToComposition(Composition *composition);

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
     *
     * If includeOpeningTempo is true, the selection will start with a
     * duplicate of the tempo (if any) that is already in force at
     * beginTime.  Otherwise the selection will only start with a
     * tempo at beginTime if there is an explicit tempo change there
     * in the source composition.
     */
    TempoSelection(Composition &, timeT beginTime, timeT endTime,
                   bool includeOpeningTempo);

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
    void RemoveFromComposition(Composition *composition);
    void AddToComposition(Composition *composition);

protected:
    tempocontainer m_tempos;
};

/**
 * A selection that includes (only) markers.
 */

// A helper class containing static information to guide the template.
class MarkerElementInfo
{
    friend class TimewiseSelection<MarkerElementInfo>;
    typedef Marker* value_type;
    
    static void RemoveFromComposition(Composition *composition,
                                      const value_type& element);
    static void AddToComposition(Composition *composition,
                                 const value_type& element);
    static value_type copyAtTime(timeT t, const value_type& element)
    { return new Marker(*element, t); }
    static timeT getTime(const value_type& element)
    { return element->getTime(); }
};
    
// The marker selection class itself
class MarkerSelection : public TimewiseSelection<MarkerElementInfo>
{
public:
   MarkerSelection(void) : TimewiseSelection<MarkerElementInfo>() {};
   MarkerSelection(Composition &, timeT beginTime, timeT endTime);

};

}

#endif
