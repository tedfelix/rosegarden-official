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

#include "Segment.h"
#include "NotationTypes.h"
#include "Quantizer.h"
#include "BaseProperties.h"
#include "Composition.h"

#include <iostream>
#include <algorithm>
#include <iterator>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;
    


Segment::Segment(SegmentType segmentType, timeT startIdx) :
    std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_track(0),
    m_id(0),
    m_composition(0),
    m_type(segmentType),
    m_audioFileID(0)
{
    // nothing
}

Segment::~Segment()
{
    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);
}


const Quantizer *
Segment::getQuantizer() const
{
    return getComposition()->getQuantizer();
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

    assert(e);

    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    delete e;
}


Segment::iterator Segment::insert(Event *e)
{
    assert(e);

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


timeT
Segment::getBarStart(timeT t) const
{
    return getComposition()->getBarStart(t);
}


timeT
Segment::getBarEnd(timeT t) const
{
    return getComposition()->getBarEnd(t);
}


int Segment::getNextId() const
{
    return m_id++;
}



void Segment::fillWithRests(timeT endTime, bool permitQuantize,
			    timeT startTime)
{
    if (startTime < 0) startTime = getDuration();
    TimeSignature ts;
    timeT sigTime = 0;

    if (getComposition()) {
	sigTime = getComposition()->getTimeSignatureAt(startTime, ts);
    }

    //!!! Since the rests are only used for notation, it may well
    // make sense to quantize the fill-with-rests duration before
    // splitting it up -- we may find it rounds quite nicely.
    // Perhaps only when explicitly called as fillWithRests rather
    // than called from setDuration?  Let's try this, and see how
    // it looks

    timeT restDuration = endTime - startTime;

    if (permitQuantize) {
	restDuration = getQuantizer()->quantizeByUnit(restDuration);
    }

    cerr << "Segment(" << this << ")::fillWithRests: endTime "
	 << endTime << ", startTime " << startTime << ", composition "
	 << (getComposition() ? "exists" : "does not exist") << ", sigTime "
	 << sigTime << ", timeSig duration " << ts.getBarDuration() << endl;
    
    DurationList dl;
    ts.getDurationListForInterval(dl, restDuration, startTime - sigTime);

    timeT acc = getStartIndex() + startTime;

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


SegmentHelper::~SegmentHelper() { }

 
}
