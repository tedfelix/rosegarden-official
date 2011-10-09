/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Selection.h"
#include "base/Segment.h"
#include "SegmentNotationHelper.h"
#include "base/BaseProperties.h"
#include <limits>

namespace Rosegarden {

EventSelection::EventSelection(Segment& t) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);
}

EventSelection::EventSelection(Segment& t, timeT beginTime, timeT endTime, bool overlap) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);

    Segment::iterator i = t.findTime(beginTime);
    Segment::iterator j = t.findTime(endTime);

    if (i != t.end()) {
	m_beginTime = (*i)->getAbsoluteTime();
	while (i != j) {
	    m_endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
	    m_segmentEvents.insert(*i);
	    ++i;
	}
	m_haveRealStartTime = true;
    }

    // Find events overlapping the beginning
    //
    if (overlap) {
        i = t.findTime(beginTime);

        while (i != t.begin() && i != t.end() && i != j) {

            if ((*i)->getAbsoluteTime() + (*i)->getDuration() > beginTime)
            {
                m_segmentEvents.insert(*i); // duplicates are filtered automatically
                m_beginTime = (*i)->getAbsoluteTime();
            }
            else
                break;

            --i;
        }

    }

}

EventSelection::EventSelection(const EventSelection &sel) :
    SegmentObserver(),
    m_originalSegment(sel.m_originalSegment),
    m_segmentEvents(sel.m_segmentEvents),
    m_beginTime(sel.m_beginTime),
    m_endTime(sel.m_endTime),
    m_haveRealStartTime(sel.m_haveRealStartTime)
{
    m_originalSegment.addObserver(this);
}

EventSelection::~EventSelection()
{
  // Notify observers of deconstruction
    for (ObserverSet::const_iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
	(*i)->eventSelectionDestroyed(this);
    }
    m_originalSegment.removeObserver(this);
}

bool
EventSelection::operator==(const EventSelection &s)
{
    if (&m_originalSegment != &s.m_originalSegment) return false;
    if (m_beginTime != s.m_beginTime) return false;
    if (m_endTime != s.m_endTime) return false;
    if (m_haveRealStartTime != s.m_haveRealStartTime) return false;
    if (m_segmentEvents != s.m_segmentEvents) return false;
    return true;
}

void
EventSelection::insertThisEvent(Event *e)
{
    if (contains(e)) return;
    
    if (e->getAbsoluteTime() < m_beginTime || !m_haveRealStartTime) {
	m_beginTime = e->getAbsoluteTime();
	m_haveRealStartTime = true;
    }

    timeT eventDuration = e->getDuration();
    if (eventDuration == 0) eventDuration = 1;

    timeT eventEndTime = e->getAbsoluteTime() + eventDuration;
    if (eventEndTime > m_endTime) {
	m_endTime = eventEndTime;
    }
    
    m_segmentEvents.insert(e);
    
    // Notify observers of new selected event
    for (ObserverSet::const_iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
	(*i)->eventSelected(this, e);
    }
}

void
EventSelection::eraseThisEvent(Event *e)
{
    
    if (!contains(e)) return;  // This probably not needed.

    std::pair<eventcontainer::iterator, eventcontainer::iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::iterator it = interval.first;
         it != interval.second; it++) {

        if (*it == e) {

	    m_segmentEvents.erase(it);

            // Notify observers of new deselected event
            for (ObserverSet::const_iterator i = m_observers.begin();
                    i != m_observers.end(); ++i) {
                (*i)->eventDeselected(this, e);
                
            }
            // Exit, since we found one good match.  Don't worry about others.
            break;
        }
    }
}

void
EventSelection::addRemoveEvent(Event *e, EventFuncPtr insertEraseFn)
{
    const Segment::const_iterator baseSegmentItr = m_originalSegment.find(e);
    
    // Sanity Check
    if (baseSegmentItr == m_originalSegment.end()) {
        std::cerr << "EventSelection::addRemoveEvent Sent event that can not be found "
                  << "in segment.  Exiting method" << std::endl;
    
    }

    timeT eventDuration = e->getDuration();
    if (eventDuration == 0) eventDuration = 1;

    timeT eventStartTime = e->getAbsoluteTime();
    timeT eventEndTime = eventStartTime + eventDuration;
    
    // Always add/remove at least the one Event we were called with.
    (this->*insertEraseFn)(e);

    // Now we handle the tied notes themselves.  If the event we're adding is
    // tied, then we iterate forward and back to try to find all of its linked
    // neighbors, and treat them as though they were one unit.  Musically, they
    // ARE one unit, and having selections treat them that way solves a lot of
    // usability problems.
    //
    // looking AHEAD:
    if (e->has(BaseProperties::TIED_FORWARD)) {

        long oldPitch = 0;
        if (e->has(BaseProperties::PITCH)) e->get<Int>(BaseProperties::PITCH, oldPitch);
        
        // Set iterator to the next element in container after baseSegmentItr;
        Segment::const_iterator si = baseSegmentItr;
        ++si;
        for (; si != m_originalSegment.end(); ++si) {
            if (!(*si)->isa(Note::EventType)) continue;

            if ((*si)->getAbsoluteTime() > eventEndTime) {
                // Break the loop.  There are no more events tied to the original
                // event in this direction
                break;
            }

            long newPitch = 0;
            if ((*si)->has(BaseProperties::PITCH)) (*si)->get<Int>(BaseProperties::PITCH, newPitch);

            // forward from the target, find all notes that are tied backwards,
            // until hitting the end of the segment or the first note at the
            // same pitch that is not tied backwards.
            if (oldPitch == newPitch) {
                if ((*si)->has(BaseProperties::TIED_BACKWARD)) {
                    // add the event
                    (this->*insertEraseFn)(*si);

                    // while looking ahead, we have to keep pushing our
                    // [selection]  search ahead to the end of the most
                    // distant tied note encountered
                    eventDuration = (*si)->getDuration();
                    if (eventDuration == 0) eventDuration = 1;

                    eventEndTime = (*si)->getAbsoluteTime() + eventDuration;
                }
            }
        }
    }
    
    // looking BACK:
    if (e->has(BaseProperties::TIED_BACKWARD) && (m_originalSegment.begin() != m_originalSegment.end())) {

        long oldPitch = 0;
        if (e->has(BaseProperties::PITCH)) e->get<Int>(BaseProperties::PITCH, oldPitch);
        
        for (Segment::const_iterator si = baseSegmentItr;
                si != m_originalSegment.begin();) {

            // Set iterator to the previous element in container
            // First step moves iterator to element prior to baseSegmentItr
            --si;
            if (!(*si)->isa(Note::EventType)) continue;

            if (((*si)->getAbsoluteTime() + (*si)->getDuration()) < eventStartTime) {
                // Break the loop.  There are no more events tied to the original
                // event in this direction
                break;
            }

            long newPitch = 0;
            if ((*si)->has(BaseProperties::PITCH)) (*si)->get<Int>(BaseProperties::PITCH, newPitch);

            // back from the target, find all notes that are tied forward,
            // until hitting the end of the segment or the first note at the
            // same pitch that is not tied forward.
            if (oldPitch == newPitch) {
                if ((*si)->has(BaseProperties::TIED_FORWARD)) {
                    // add the event
                    (this->*insertEraseFn)(*si);

                    // while looking back, we have to keep pushing our
                    // [selection] search back to the end of the most
                    // distant tied note encountered
                    eventStartTime = (*si)->getAbsoluteTime();
                }
            }
        }
    }
}

void
EventSelection::addObserver(EventSelectionObserver *obs) { 
    m_observers.push_back(obs); 
}

void
EventSelection::removeObserver(EventSelectionObserver *obs) { 
    m_observers.remove(obs); 
}


void
EventSelection::addEvent(Event *e)
{
    addRemoveEvent(e, &EventSelection::insertThisEvent);
}

void
EventSelection::addFromSelection(EventSelection *sel)
{
    for (eventcontainer::iterator i = sel->getSegmentEvents().begin();
	 i != sel->getSegmentEvents().end(); ++i) {
	// contains() checked a bit deeper now
	addEvent(*i);
    }
}

void
EventSelection::removeEvent(Event *e) 
{
    addRemoveEvent(e, &EventSelection::eraseThisEvent);
}

bool
EventSelection::contains(Event *e) const
{
    std::pair<eventcontainer::const_iterator, eventcontainer::const_iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::const_iterator it = interval.first;
         it != interval.second; ++it)
    {
        if (*it == e) return true;
    }

    return false;
}

bool
EventSelection::contains(const std::string &type) const
{
    for (eventcontainer::const_iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	if ((*i)->isa(type)) return true;
    }
    return false;
}

timeT
EventSelection::getTotalDuration() const
{
    return getEndTime() - getStartTime();
}

timeT
EventSelection::getNotationStartTime() const
{
    timeT start = 0;
    bool first = true;
    // inefficient, but the simplest way to be sure (since events are
    // not ordered in notation time)
    for (eventcontainer::const_iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	timeT t = (*i)->getNotationAbsoluteTime();
	if (first || t < start) start = t;
	first = false;
    }
    return start;
}

timeT
EventSelection::getNotationEndTime() const
{
    timeT end = 0;
    bool first = true;
    // inefficient, but the simplest way to be sure (since events are
    // not ordered in notation time)
    for (eventcontainer::const_iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	timeT t = (*i)->getNotationAbsoluteTime() + (*i)->getNotationDuration();
	if (first || t > end) end = t;
	first = false;
    }
    return end;
}

timeT
EventSelection::getTotalNotationDuration() const
{
    timeT start = 0, end = 0;
    bool first = true;
    // inefficient, but the simplest way to be sure (since events are
    // not ordered in notation time)
    for (eventcontainer::const_iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	timeT t = (*i)->getNotationAbsoluteTime();
	if (first || t < start) start = t;
	t += (*i)->getNotationDuration();
	if (first || t > end) end = t;
	first = false;
    }
    return end - start;
}


// Return the average value of an integer-valued property for events
// in the selection.
int
EventSelection::getAverageProperty(PropertyName property) const
{
    float total = 0;
    int count = 0;
    for (eventcontainer::iterator i = getSegmentEvents().begin();
         i != getSegmentEvents().end();
         ++i) {
        if ((*i)->has(property)) {
            total += (*i)->get<Int>(property);
            ++count;
        }
    }

    if (count > 0) { return (total / count) + 0.5; }
    else { return 0; }
}


// Return the maximum and minimum values of an integer-valued property
// for events in the selection.
std::pair<int,int>
EventSelection::getMinMaxProperty(PropertyName property) const
{
    // Start with values such that anything we find will supersede
    // them.
    int min = std::numeric_limits<int>::max(),
        max = std::numeric_limits<int>::min();
    for (eventcontainer::iterator i = getSegmentEvents().begin();
         i != getSegmentEvents().end();
         ++i) {
        if ((*i)->has(property)) {
            int value = (*i)->get<Int>(property);
            if (max < value) { max = value; }
            if (value < min) { min = value; }
        }
    }
    return std::pair<int,int>(min,max);
}

EventSelection::RangeList
EventSelection::getRanges() const
{
    RangeList ranges;

    Segment::iterator i = m_originalSegment.findTime(getStartTime());
    Segment::iterator j = i;
    Segment::iterator k = m_originalSegment.findTime(getEndTime());

    while (j != k) {

        for (j = i; j != k && contains(*j); ++j) { }

        if (j != i) {
            ranges.push_back(RangeList::value_type(i, j));
	}

	for (i = j; i != k && !contains(*i); ++i) { }
	j = i;
    }

    return ranges;
}

EventSelection::RangeTimeList
EventSelection::getRangeTimes() const
{
    RangeList ranges(getRanges());
    RangeTimeList rangeTimes;

    for (RangeList::iterator i = ranges.begin(); i != ranges.end(); ++i) {
	timeT startTime = m_originalSegment.getEndTime();
	timeT   endTime = m_originalSegment.getEndTime();
	if (i->first != m_originalSegment.end()) {
	    startTime = (*i->first)->getAbsoluteTime();
	}
	if (i->second != m_originalSegment.end()) {
	    endTime = (*i->second)->getAbsoluteTime();
	}
	rangeTimes.push_back(RangeTimeList::value_type(startTime, endTime));
    }

    return rangeTimes;
}

void
EventSelection::eventRemoved(const Segment *s, Event *e)
{
    // contains() checked a bit deeper now.
    if (s == &m_originalSegment) {
        removeEvent(e);
    }
}

void
EventSelection::segmentDeleted(const Segment *)
{
    /*
    std::cerr << "WARNING: EventSelection notified of segment deletion: this is probably a bug "
	      << "(selection should have been deleted before segment)" << std::endl;
              */
}

bool SegmentSelection::hasNonAudioSegment() const
{
    for (const_iterator i = begin(); i != end(); ++i) {
        if ((*i)->getType() == Segment::Internal)
            return true;
    }
    return false;
}


TimeSignatureSelection::TimeSignatureSelection() { }

TimeSignatureSelection::TimeSignatureSelection(Composition &composition,
					       timeT beginTime,
					       timeT endTime,
					       bool includeOpeningTimeSig)
{
    int n = composition.getTimeSignatureNumberAt(endTime);

    for (int i = composition.getTimeSignatureNumberAt(beginTime);
	 i <= n;
	 ++i) {

	if (i < 0) continue;

	std::pair<timeT, TimeSignature> sig =
	    composition.getTimeSignatureChange(i);

	if (sig.first < endTime) {
	    if (sig.first < beginTime) {
		if (includeOpeningTimeSig) {
		    sig.first = beginTime;
		} else {
		    continue;
		}
	    }
	    addTimeSignature(sig.first, sig.second);
	}
    }
}

TimeSignatureSelection::~TimeSignatureSelection() { }

void
TimeSignatureSelection::addTimeSignature(timeT t, TimeSignature timeSig)
{
    m_timeSignatures.insert(timesigcontainer::value_type(t, timeSig));
}
void
TimeSignatureSelection::RemoveFromComposition(Composition *composition)
{
    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                begin(); i != end(); ++i) {
        int n = composition->getTimeSignatureNumberAt(i->first);
        if (n >= 0)
            { composition->removeTimeSignature(n); }
    }
}
void
TimeSignatureSelection::AddToComposition(Composition *composition)
{
    for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                begin(); i != end(); ++i) {
        composition->addTimeSignature(i->first, i->second);
    }    
}

TempoSelection::TempoSelection() { }

TempoSelection::TempoSelection(Composition &composition,
			       timeT beginTime,
			       timeT endTime,
			       bool includeOpeningTempo)
{
    int n = composition.getTempoChangeNumberAt(endTime);

    for (int i = composition.getTempoChangeNumberAt(beginTime);
	 i <= n;
	 ++i) {

	if (i < 0) continue;

	std::pair<timeT, tempoT> change = composition.getTempoChange(i);

	if (change.first < endTime) {
	    if (change.first < beginTime) {
		if (includeOpeningTempo) {
		    change.first = beginTime;
		} else {
		    continue;
		}
	    }
	    std::pair<bool, tempoT> ramping =
		composition.getTempoRamping(i, false);
	    addTempo(change.first, change.second,
		     ramping.first ? ramping.second : -1);
	}
    }
}

TempoSelection::~TempoSelection() { }

void
TempoSelection::addTempo(timeT t, tempoT tempo, tempoT targetTempo)
{
    m_tempos.insert(tempocontainer::value_type
		    (t, tempochange(tempo, targetTempo)));
}

void
TempoSelection::RemoveFromComposition(Composition *composition)
{

    for (TempoSelection::tempocontainer::const_iterator i = begin();
         i != end();
         ++i) {
        int n = composition->getTempoChangeNumberAt(i->first);
        if (n >= 0)
            { composition->removeTempoChange(n); }
    }

}    
    
void
TempoSelection::AddToComposition(Composition *composition)
{

    for (TempoSelection::tempocontainer::const_iterator i = begin();
         i != end();
         ++i) {
        composition->addTempoAtTime(i->first,
                                    i->second.first,
                                    i->second.second);
    }
}

}
