// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "Segment.h"
#include "NotationTypes.h"
#include "Quantizer.h"
#include "BaseProperties.h"

#include <iostream>
#include <algorithm>
#include <iterator>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;
    


Segment::Segment(timeT startIdx) :
    std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0),
    m_id(0),
    m_quantizer(0)
{
    // nothing
}

Segment::~Segment()
{
    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);
}


void Segment::setStartIndex(timeT idx)
{
    int idxDiff = idx - m_startIdx;

    // reset the time of all events
    for (iterator i = begin(); i != end(); ++i)
        (*i)->addAbsoluteTime(idxDiff);
        
    m_startIdx = idx;
}


timeT Segment::getDuration() const
{
    const_iterator lastEl = end();
    if (lastEl == begin()) return 0;
    --lastEl;
    return (*lastEl)->getAbsoluteTime() +
           (*lastEl)->getDuration() -
           getStartIndex();
}


void Segment::setDuration(timeT d)
{
    timeT currentDuration = getDuration();

    cerr << "Segment::setDuration() : current = " << currentDuration
         << " - new : " << d << endl;

    if (d == currentDuration) return; // nothing to do
    
    if (d > currentDuration) {

	fillWithRests(d);

    } else { // shrink

        //!!! TODO : NOT IMPLEMENTED YET : move an internal marker

//         if (nbBars == 0) { // no fuss
//             erase(begin(), end());
//             return;
//         }

//         unsigned int cutTime = ((nbBars-1) * 384) + getStartIndex();

//         iterator lastElToKeep = std::upper_bound(begin(), end(),
//                                                  cutTime,
//                                                  Event::compareTime2Event);
//         if (lastElToKeep == end()) {
//             cerr << "Segment::setNbBars() : upper_bound returned end\n";
//             return;
//         }

//         erase(lastElToKeep, end());
    }
    
}

void Segment::erase(iterator pos)
{
    Event *e = *pos;
    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    delete e;
}


Segment::iterator Segment::insert(Event *e)
{
    iterator i = std::multiset<Event*, Event::EventCmp>::insert(e);
    notifyAdd(e);
    return i;
}


void Segment::erase(iterator from, iterator to)
{
    // Not very efficient, but without an observer event for
    // multiple erase we can't do any better.

    // We can't do this :
    //
    // for (Segment::iterator i = from; i != to; ++i) { ... erase(i); }
    //
    // because erasing an iterator invalidates it.

    // So, gather the events which we'll have to delete
    // call notifyRemove() for each of them,
    // and finally call std::multiset<>::erase(from, to)

    for (Segment::iterator i = from; i != to; ++i) {
        Event *e = *i;
        notifyRemove(e);
        delete e;
    }
    
    std::multiset<Event*, Event::EventCmp>::erase(from, to);

}


bool Segment::eraseSingle(Event* e)
{
    iterator elPos = findSingle(e);

    if (elPos != end()) {

        erase(elPos);
        return true;
            
    } else return false;
    
}


Segment::iterator Segment::findContiguousNext(Segment::iterator el) const
{
    std::string elType = (*el)->getType(),
        reject, accept;
     
    if (elType == Note::EventType) {
        accept = Note::EventType;
        reject = Note::EventRestType;
    } else if (elType == Note::EventRestType) {
        accept = Note::EventRestType;
        reject = Note::EventType;
    } else {
        accept = elType;
        reject = "";
    }

    bool success = false;

    iterator i = ++el;
    
    for(; i != end(); ++i) {
        std::string iType = (*i)->getType();

        if (iType == reject) {
            success = false;
            break;
        }
        if (iType == accept) {
            success = true;
            break;
        }
    }

    if (success) return i;
    else return end();
    
}

Segment::iterator Segment::findContiguousPrevious(Segment::iterator el) const
{
    if (el == begin()) return end();

    std::string elType = (*el)->getType(),
        reject, accept;
     
    if (elType == Note::EventType) {
        accept = Note::EventType;
        reject = Note::EventRestType;
    } else if (elType == Note::EventRestType) {
        accept = Note::EventRestType;
        reject = Note::EventType;
    } else {
        accept = elType;
        reject = "";
    }

    bool success = false;

    iterator i = --el;

    while (true) {
        std::string iType = (*i)->getType();

        if (iType == reject) {
            success = false;
            break;
        }
        if (iType == accept) {
            success = true;
            break;
        }
	if (i == begin()) break;
	--i;
    }

    if (success) return i;
    else return end();
}


Segment::iterator Segment::findSingle(Event* e) const
{
    iterator res = end();

    std::pair<iterator, iterator> interval = equal_range(e);

    for(iterator i = interval.first; i != interval.second; ++i) {
        if (*i == e) {
            res = i;
            break;
        }
    }
    return res;
}


Segment::iterator Segment::findTime(timeT t) const
{
    Event dummy;
    dummy.setAbsoluteTime(t);
    dummy.setSubOrdering(MIN_SUBORDERING);
    return lower_bound(&dummy);
}


Segment::iterator Segment::findBarAt(timeT t) const
{
    const Segment *ref = getReferenceSegment();
    iterator i = ref->findTime(t);
    if (i != ref->begin() &&
	(i == ref->end() || (*i)->getAbsoluteTime() > t)) --i;
    return i;
}


Segment::iterator Segment::findBarAfter(timeT t) const
{
    const Segment *ref = getReferenceSegment();
    iterator i = ref->findTime(t);
    return i;
}


static bool isTimeSig(const Event* e)
{
    return e->isa(TimeSignature::EventType);
}

Segment::iterator Segment::findTimeSignatureAt(timeT t) const
{
    // We explicitly want to avoid calling getReferenceSegment here,
    // because we don't need the bars to be recalculated -- we only
    // want to look at the time signatures

    const Segment *ref = m_referenceSegment;
    iterator i = ref->findTime(t);
    
    while (i != ref->begin() &&
	   (i == ref->end() ||
	    ((*i)->getAbsoluteTime() > t) ||
	    !isTimeSig(*i))) {
	--i;
    }

    if (i == ref->end() || !isTimeSig(*i)) {
	return ref->end();
    }

    return i;
}

timeT Segment::findTimeSignatureAt(timeT t, TimeSignature &timeSig) const
{
    timeSig = TimeSignature();
    if (!m_referenceSegment) return 0;

    iterator i = findTimeSignatureAt(t);
    if (i == m_referenceSegment->end()) return 0;

    timeSig = TimeSignature(**i);
    return (*i)->getAbsoluteTime();
}

timeT Segment::findBarStartTime(timeT t) const
{
    iterator barItr = findBarAt(t);
    if (barItr == m_referenceSegment->end()) return getEndIndex();
    return (*barItr)->getAbsoluteTime();
}

timeT Segment::findBarEndTime(timeT t) const
{
    iterator barItr = findBarAt(t);
    if (barItr == m_referenceSegment->end() ||
	++barItr == m_referenceSegment->end()) return getEndIndex();
    return (*barItr)->getAbsoluteTime();
}

Segment::iterator Segment::findStartOfBar(timeT t) const
{
    t = findBarStartTime(t);
    return findTime(t);
}

Segment::iterator Segment::findStartOfNextBar(timeT t) const
{
    t = findBarEndTime(t);
    return findTime(t);
}


int Segment::getNextId() const
{
    return m_id++;
}



void Segment::fillWithRests(timeT endTime)
{
    timeT duration = getDuration();
    TimeSignature ts;
    timeT sigTime = findTimeSignatureAt(duration, ts);
    
    DurationList dl;
    ts.getDurationListForInterval(dl, endTime - duration, sigTime);

    timeT acc = getStartIndex() + duration;

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	Event *e = new Event(Note::EventRestType);
	e->setDuration(*i);
	e->setAbsoluteTime(acc);
	insert(e);
	acc += *i;
    }
}



void Segment::getTimeSlice(timeT absoluteTime, iterator &start, iterator &end)
    const
{
    Event dummy;
    
    dummy.setAbsoluteTime(absoluteTime);
  
    // No, this won't work -- we need to include things that don't
    // compare equal because they have different suborderings, as long
    // as they have the same times
  
//    std::pair<iterator, iterator> res = equal_range(&dummy);

//    start = res.first;
//    end = res.second;

    // Got to do this instead:

    dummy.setSubOrdering(MIN_SUBORDERING);
    start = end = lower_bound(&dummy);

    while (end != this->end() &&
	   (*end)->getAbsoluteTime() == (*start)->getAbsoluteTime())
	++end;
}


bool Segment::noteIsInChord(Event *note) const
{
    std::pair<iterator, iterator> res = equal_range(note);

    int noteCount = 0;
    for (iterator i = res.first; i != res.second; ++i) {
	if ((*i)->isa(Note::EventType)) ++noteCount;
    }
    return noteCount > 1;
}


Segment::iterator
Segment::getNoteTiedWith(Event *note, bool forwards) const
{
    bool tied = false;

    if (!note->get<Bool>(forwards ?
			 BaseProperties::TIED_FORWARD :
                         BaseProperties::TIED_BACKWARD, tied) || !tied) {
        return end();
    }

    timeT myTime = note->getAbsoluteTime();
    timeT myDuration = note->getDuration();
    int myPitch = note->get<Int>(BaseProperties::PITCH);

    iterator i = findSingle(note);
    if (i == end()) return end();

    for (;;) {
        i = forwards ? findContiguousNext(i) : findContiguousPrevious(i);

        if (i == end()) return end();
        if ((*i)->getAbsoluteTime() == myTime) continue;

        if (forwards && ((*i)->getAbsoluteTime() != myTime + myDuration)) {
            return end();
        }
        if (!forwards &&
            (((*i)->getAbsoluteTime() + (*i)->getDuration()) != myTime)) {
            return end();
        }
        
        if (!(*i)->get<Bool>(forwards ?
                             BaseProperties::TIED_BACKWARD :
                             BaseProperties::TIED_FORWARD, tied) || !tied) {
            continue;
        }

        if ((*i)->get<Int>(BaseProperties::PITCH) == myPitch) return i;
    }

    return end();
}

void Segment::notifyAdd(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventAdded(this, e);
    }
}

 
void Segment::notifyRemove(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventRemoved(this, e);
    }
}


void Segment::notifyReferenceSegmentRequested() const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->referenceSegmentRequested(this);
    }
}


SegmentHelper::~SegmentHelper() { }

 
}
