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

#include "Track.h"
#include "NotationTypes.h"
#include "Quantizer.h"

#include <iostream>
#include <algorithm>
#include <iterator>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;
    


Track::Track(timeT startIdx) :
    std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0),
    m_id(0),
    m_quantizer(new Quantizer())
{
    invalidateTimeSigAtEnd();
}

Track::~Track()
{
    // delete content
    for (iterator it = begin(); it != end(); ++it) delete (*it);

    delete m_quantizer;
}


void Track::setStartIndex(timeT idx)
{
    int idxDiff = idx - m_startIdx;

    // reset the time of all events
    for (iterator i = begin(); i != end(); ++i)
        (*i)->addAbsoluteTime(idxDiff);
        
    m_startIdx = idx;
}

static bool isTimeSig(const Event* e)
{
    return e->isa(TimeSignature::EventType);
}


TimeSignature Track::getTimeSigAtEnd(timeT &absTimeOfSig)
{
    absTimeOfSig = 0;

    if (m_timeSigTime < 0) findTimeSigAtEnd();
    
    absTimeOfSig = m_timeSigTime;
    return m_timeSigAtEnd;
}


void Track::findTimeSigAtEnd() 
{
    m_timeSigAtEnd = TimeSignature();
    m_timeSigTime = 0;

    const_reverse_iterator sig = std::find_if(rbegin(), rend(), isTimeSig);

    if (sig != rend()) {
	assert(isTimeSig(*sig));
        m_timeSigAtEnd = TimeSignature(**sig);
        m_timeSigTime = (*sig)->getAbsoluteTime();
    }
}


timeT Track::getDuration() const
{
    const_iterator lastEl = end();
    if (lastEl == begin()) return 0;
    --lastEl;
    return (*lastEl)->getAbsoluteTime() +
           (*lastEl)->getDuration() -
           getStartIndex();
}


void Track::setDuration(timeT d)
{
    timeT currentDuration = getDuration();

    cerr << "Track::setDuration() : current = " << currentDuration
         << " - new : " << d << endl;

    if (d == currentDuration) return; // nothing to do
    
    if (d > currentDuration) {

	fillWithRests(d);

    } else { // shrink

        //!!! NOT IMPLEMENTED YET : move an internal marker

//         if (nbBars == 0) { // no fuss
//             erase(begin(), end());
//             return;
//         }

//         unsigned int cutTime = ((nbBars-1) * 384) + getStartIndex();

//         iterator lastElToKeep = std::upper_bound(begin(), end(),
//                                                  cutTime,
//                                                  Event::compareTime2Event);
//         if (lastElToKeep == end()) {
//             cerr << "Track::setNbBars() : upper_bound returned end\n";
//             return;
//         }

//         erase(lastElToKeep, end());
    }
    
}

void Track::erase(iterator pos)
{
    Event *e = *pos;
    if (e->isa(TimeSignature::EventType)) invalidateTimeSigAtEnd();
    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    delete e;
}


Track::iterator Track::insert(Event *e)
{
    if (e->isa(TimeSignature::EventType)) invalidateTimeSigAtEnd();
    m_quantizer->quantizeByNote(e);
    iterator i = std::multiset<Event*, Event::EventCmp>::insert(e);
    notifyAdd(e);
    return i;
}


void Track::erase(iterator from, iterator to)
{
    //!!! not very efficient, but without an observer event for
    //multiple erase we can't do any better:

    // We can't do this :
    //
    //for (Track::iterator i = from; i != to; ++i) erase(i);
    //
    // because erasing an iterator invalidates it

    // So, gather the events which we'll have to delete
    // call notifyRemove() for each of them,
    // and finally call std::multiset<>::erase(from, to)

    for (Track::iterator i = from; i != to; ++i) {
        Event *e = *i;
        if (e->isa(TimeSignature::EventType)) invalidateTimeSigAtEnd();
        notifyRemove(e);
        delete e;
    }
    
    std::multiset<Event*, Event::EventCmp>::erase(from, to);

}


bool Track::eraseSingle(Event* e)
{
    iterator elPos = findSingle(e);

    if (elPos != end()) {

        erase(elPos);
        return true;
            
    } else return false;
    
}


Track::iterator Track::findContiguousNext(Track::iterator el) const
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

Track::iterator Track::findContiguousPrevious(Track::iterator el) const
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

    for(; i != begin(); --i) {
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


Track::iterator Track::findSingle(Event* e) const
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


Track::iterator Track::findTime(timeT t) const
{
    Event dummy;
    dummy.setAbsoluteTime(t);
    dummy.setSubOrdering(MIN_SUBORDERING);
    return lower_bound(&dummy);
}


Track::iterator Track::findBarAt(timeT t) const
{
    const Track *ref = getReferenceTrack();
    iterator i = ref->findTime(t);
    if (i != ref->begin() &&
	(i == ref->end() || (*i)->getAbsoluteTime() > t)) --i;
    return i;
}


Track::iterator Track::findBarAfter(timeT t) const
{
    const Track *ref = getReferenceTrack();
    iterator i = ref->findTime(t);
    return i;
}


Track::iterator Track::findTimeSignatureAt(timeT t) const
{
    iterator i = findBarAt(t);
    const Track *ref = m_referenceTrack;

    while (i != ref->begin() &&
	   (i == ref->end() || !isTimeSig(*i))) {
	--i;
    }

    if (i == ref->end() || !isTimeSig(*i)) {
	return ref->end();
    }

    return i;
}

timeT Track::findTimeSignatureAt(timeT t, TimeSignature &timeSig) const
{
    iterator i = findTimeSignatureAt(t);

    if (i == m_referenceTrack->end()) {
	timeSig = TimeSignature();
	return 0;
    }

    timeSig = TimeSignature(**i);
    return (*i)->getAbsoluteTime();
}

timeT Track::findBarStartTime(timeT t) const
{
    iterator barItr = findBarAt(t);
    if (barItr == m_referenceTrack->end()) return getEndIndex();
    return (*barItr)->getAbsoluteTime();
}

timeT Track::findBarEndTime(timeT t) const
{
    iterator barItr = findBarAt(t);
    if (barItr == m_referenceTrack->end() ||
	++barItr == m_referenceTrack->end()) return getEndIndex();
    return (*barItr)->getAbsoluteTime();
}

Track::iterator Track::findStartOfBar(timeT t) const
{
    t = findBarStartTime(t);
    return findTime(t);
}

Track::iterator Track::findStartOfNextBar(timeT t) const
{
    t = findBarEndTime(t);
    return findTime(t);
}


int Track::getNextId() const
{
    return m_id++;
}



void Track::fillWithRests(timeT endTime)
{
    timeT sigTime;
    TimeSignature ts(getTimeSigAtEnd(sigTime));
    timeT duration = getDuration();
    
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



void Track::getTimeSlice(timeT absoluteTime, iterator &start, iterator &end)
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


bool Track::noteIsInChord(Event *note) const
{
    std::pair<iterator, iterator> res = equal_range(note);

    int noteCount = 0;
    for (iterator i = res.first; i != res.second; ++i) {
	if ((*i)->isa(Note::EventType)) ++noteCount;
    }
    return noteCount > 1;
}


Track::iterator
Track::getNoteTiedWith(Event *note, bool forwards) const
{
    bool tied = false;

    if (!note->get<Bool>(forwards ?
                         Note::TiedForwardPropertyName :
                         Note::TiedBackwardPropertyName, tied) || !tied) {
        return end();
    }

    timeT myTime = note->getAbsoluteTime();
    timeT myDuration = note->getDuration();
    int myPitch = note->get<Int>("pitch");

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
                             Note::TiedBackwardPropertyName :
                             Note::TiedForwardPropertyName, tied) || !tied) {
            continue;
        }

        int pitch;
        if ((*i)->get<Int>("pitch") == pitch) return i;
    }

    return end();
}

void Track::notifyAdd(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventAdded(this, e);
    }
}

 
void Track::notifyRemove(Event *e) const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventRemoved(this, e);
    }
}


void Track::notifyReferenceTrackRequested() const
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->referenceTrackRequested(this);
    }
}


TrackHelper::~TrackHelper() { }

 
}
