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
    
Track::Track(timeT duration, timeT startIdx) :
    std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0),
    m_id(0),
    m_quantizer(new Quantizer())
{
    setDuration(duration);
}

Track::~Track()
{
    notifyTrackGone();

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


TimeSignature Track::getTimeSigAtEnd(timeT &absTimeOfSig) const
{
    TimeSignature timesig;
    absTimeOfSig = 0;

    const_reverse_iterator sig = std::find_if(rbegin(), rend(), isTimeSig);

    if (sig != rend()) {
	assert((*sig)->isa(TimeSignature::EventType));
        absTimeOfSig = (*sig)->getAbsoluteTime();
	timesig = TimeSignature(*(*sig));
    }

    return timesig;
}


timeT Track::getDuration() const
{
    const_iterator lastEl = end();
    if (lastEl == begin()) return 0;
    --lastEl;
    return ((*lastEl)->getAbsoluteTime() + (*lastEl)->getDuration());
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


void Track::calculateBarPositions()
{
    TimeSignature timeSignature;

    m_barPositions.clear();
    addNewBar(0, false, 0, timeSignature);

    timeT absoluteTime = 0;
    timeT barStartTime = 0;
    timeT barDuration = timeSignature.getBarDuration();

    iterator i(begin());

    for (; i != end(); ++i) {

        Event *e = *i;
        absoluteTime = m_quantizer->quantizeByUnit(e->getAbsoluteTime());

        if (absoluteTime - barStartTime >= barDuration) {
            addNewBar(absoluteTime, false, barStartTime, timeSignature);
            barStartTime += barDuration;  //= absoluteTime;
        }

        if (e->isa(TimeSignature::EventType)) {

            if (absoluteTime > barStartTime) {
                addNewBar(absoluteTime, true, barStartTime, timeSignature);
                barStartTime = absoluteTime;
            }

            timeSignature = TimeSignature(*e);
            barDuration = timeSignature.getBarDuration();
        }

        // solely so that absoluteTime is correct after we hit end():
        absoluteTime += m_quantizer->getNoteQuantizedDuration(e);
    }

    if (absoluteTime > barStartTime) {
        addNewBar(absoluteTime, false, barStartTime, timeSignature);
    }
}


void Track::addNewBar(timeT start, bool fixed, timeT previousStart,
		      TimeSignature timesig)
{
    bool correct =
        fixed || (start == previousStart + timesig.getBarDuration());
    m_barPositions.push_back(BarPosition(start, fixed, correct, timesig));
}


int Track::getBarNumber(const iterator &i) const
{
    if (i == end()) return m_barPositions.size() - 1;
    else return getBarNumber(*i);
}


int Track::getBarNumber(const Event *e) const
{
    BarPosition pos(e->getAbsoluteTime(), true, true, TimeSignature());

    BarPositionList::const_iterator bpi
        (std::lower_bound(m_barPositions.begin(), m_barPositions.end(), pos));

    int barNo = std::distance(m_barPositions.begin(), bpi);

    if (bpi->start > e->getAbsoluteTime() && barNo > 0) --barNo;
    return barNo;
}


void Track::erase(iterator pos)
{
    Event *e = *pos;
    std::multiset<Event*, Event::EventCmp>::erase(pos);
    notifyRemove(e);
    delete e;
}


Track::iterator Track::insert(Event *e)
{
    iterator i = std::multiset<Event*, Event::EventCmp>::insert(e);
    notifyAdd(e);
    return i;
}


void Track::erase(iterator from, iterator to)
{
    //!!! not very efficient, but without an observer event for
    //multiple erase we can't do any better:

    for (Track::iterator i = from; i != to; ++i) erase(i);
}


bool Track::eraseSingle(Event* e)
{
    iterator elPos = findSingle(e);

    if (elPos != end()) {

        erase(elPos);
        return true;
            
    } else return false;
    
}


Track::iterator Track::findContiguousNext(Track::iterator el)
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

Track::iterator Track::findContiguousPrevious(Track::iterator el)
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


Track::iterator Track::findSingle(Event* e)
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


Track::iterator Track::findTime(timeT t)
{
    Event dummy;
    dummy.setAbsoluteTime(t);
    return lower_bound(&dummy);
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

    timeT acc = duration;

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	Event *e = new Event(Note::EventRestType);
	e->setDuration(*i);
	e->setAbsoluteTime(acc);
	insert(e);
	acc += *i;
    }
}



void Track::getTimeSlice(timeT absoluteTime, iterator &start, iterator &end)
{
    Event dummy;
    
    dummy.setAbsoluteTime(absoluteTime);
    
    std::pair<iterator, iterator> res = equal_range(&dummy);

    start = res.first;
    end = res.second;
}


bool Track::noteIsInChord(Event *note)
{
    std::pair<iterator, iterator> res = equal_range(note);

    int noteCount = 0;
    for (iterator i = res.first; i != res.second; ++i) {
	if ((*i)->isa(Note::EventType)) ++noteCount;
    }
    return noteCount > 1;
}

bool Track::hasEffectiveDuration(iterator i)
{
    bool hasDuration = ((*i)->getDuration() > 0);

    if ((*i)->isa(Note::EventType)) {
	iterator i0(i);
	if (++i0 != end() &&
	    (*i0)->getAbsoluteTime() == (*i)->getAbsoluteTime()) {
	    // we're in a chord or something
	    hasDuration = false;
	}
    }
    
    return hasDuration;
}



void Track::notifyAdd(Event *e)
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventAdded(this, e);
    }
}

 
void Track::notifyRemove(Event *e)
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->eventRemoved(this, e);
    }
}


void Track::notifyTrackGone()
{
    for (ObserverSet::iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->trackDeleted(this);
    }    
}


TrackHelper::~TrackHelper() { }

 
}
