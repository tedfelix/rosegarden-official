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

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;

const string Track::BeamedGroupIdPropertyName   = "BGroupId";
const string Track::BeamedGroupTypePropertyName = "BGroupType";
    
Track::Track(unsigned int nbTimeSteps, timeT startIdx,
             unsigned int stepsPerBar)
    : std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0),
    m_groupId(0)
{
    unsigned int initialTime = m_startIdx;
    
    // fill up with whole-note rests
    //
    for (unsigned int i = 0; i < nbTimeSteps; i += stepsPerBar) {
        Event *e = new Event;
        e->setType("rest");
        e->setDuration(384); // TODO : get rid of this magic number
        e->setAbsoluteTime(initialTime);
        insert(e);
        initialTime += 384; // btw, it comes from xmlstorableevent.cpp
    }
}

Track::~Track()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it)
        delete (*it);
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
    static TimeSignature defaultSig44(4,4);

    const_reverse_iterator sig = std::find_if(rbegin(), rend(), isTimeSig);

    if (sig != rend() ||
        ((*sig) && (*sig)->isa(TimeSignature::EventType))) {
        absTimeOfSig = (*sig)->getAbsoluteTime();
        return TimeSignature(*(*sig));
    }

    return defaultSig44;
}

unsigned int Track::getNbTimeSteps() const
{
    const_iterator lastEl = end();
    --lastEl;
    unsigned int nbBars = ((*lastEl)->getAbsoluteTime() +
                           (*lastEl)->getDuration());

    return nbBars;
}


void Track::setNbTimeSteps(unsigned int nbTimeSteps)
{
    unsigned int currentNbTimeSteps = getNbTimeSteps();

    cerr << "Track::setNbBars() : current = " << currentNbTimeSteps
         << " - new : " << nbTimeSteps << endl;

    if (nbTimeSteps == currentNbTimeSteps) return; // nothing to do
    
    if (nbTimeSteps > currentNbTimeSteps) { // fill up with rests

        timeT absTimeOfSig = 0;
        TimeSignature signatureAtEnd = getTimeSigAtEnd(absTimeOfSig);

        cerr << "Found time sig at end : " << signatureAtEnd.getNumerator()
             << "/" << signatureAtEnd.getDenominator() << endl;
        
        iterator lastEl = end();
        --lastEl;
        unsigned int newElTime =
            (*lastEl)->getAbsoluteTime() + (*lastEl)->getDuration();

        // The startOffset argument to getDurationListForInterval is
        // supposed to be the elapsed time since the start of any
        // arbitrary previous bar, provided that the time signature
        // has not changed during that elapsed time.  Since a time
        // signature event always starts a new bar, the elapsed time
        // since the previous time signature event should be fine.

        DurationList dlist;
        signatureAtEnd.getDurationListForInterval
            (dlist,
             nbTimeSteps - currentNbTimeSteps,  // interval duration
             newElTime - absTimeOfSig);         // start offset

        timeT acc = newElTime;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
            Event *e = new Event("rest");
            e->setDuration(*i);
            e->setAbsoluteTime(acc);
            insert(e);
            acc += *i;
        }

    } else { // shrink

        // NOT IMPLEMENTED YET : move an internal marker

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
    Quantizer quantizer;

    m_barPositions.clear();

    bool startNewBar(true);
    bool barCorrect(true);

    timeT absoluteTime = 0;
    timeT thisBarTime = 0;

    iterator i(begin());

    for (; i != end(); ++i) {

        Event *e = *i;
        absoluteTime = e->getAbsoluteTime();

        if (startNewBar) addNewBar(i, true, barCorrect);
        startNewBar = false;

        if (e->isa(TimeSignature::EventType)) {

            if (thisBarTime > 0) {

                thisBarTime = 0;

                // insert the bar line before this event, and also
                // before any preceding clef or key events

                iterator i0(i), i1(i);
                while (i0 == i ||
                       (*i0)->isa(Clef::EventType) ||
                       (*i1)->isa( Key::EventType)) {
                    i1 = i0;
                    if (i0 == begin()) break; // shouldn't happen anyway
                    --i0;
                }
                
                addNewBar(i1, true, true);
            }

            timeSignature = TimeSignature(*e);

        } else if (e->isa(Note::EventType) || e->isa(Note::EventRestType)) {

            bool hasDuration = true;

            quantizer.quantizeByNote(e);

            if (e->isa(Note::EventType)) {
                iterator i0(i);
                if (++i0 != end() &&
                    (*i0)->getAbsoluteTime() == e->getAbsoluteTime()) {
                    hasDuration = false;
                }
            }

            if (hasDuration) {

                // either we're not in a chord or the chord is about
                // to end: update the time accordingly

                long d = e->get<Int>(Quantizer::NoteDurationProperty);
                thisBarTime += d;

                cerr << "Track: Quantized duration is " << d
                     << ", current bar now "
                     << thisBarTime << endl;
            }

            timeT barDuration = timeSignature.getBarDuration();
            if (thisBarTime >= barDuration) {
                barCorrect = (thisBarTime == barDuration);
                thisBarTime = 0;
                startNewBar = true;
            }
        }
    }

    if (startNewBar || thisBarTime > 0) {
        addNewBar(i, false, thisBarTime == timeSignature.getBarDuration());
    }
}


void Track::erase(iterator pos)
{
    delete *pos;
    std::multiset<Event*, Event::EventCmp>::erase(pos);
}

void Track::erase(iterator from, iterator to)
{
    for(Track::iterator i = from; i != to; ++i)
        delete *i;
    
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

bool Track::collapse(Event* e, bool& collapseForward, Event*& collapsedEvent)
{
    collapsedEvent = 0;

    iterator elPos = findSingle(e);

    if (elPos == end()) return false;

    iterator nextEvent = findContiguousNext(elPos),
        previousEvent  = findContiguousPrevious(elPos);

    if (nextEvent != end() &&
        canCollapse(nextEvent, elPos)) {

        // collapse with next event
        e->setDuration(e->getDuration() + (*nextEvent)->getDuration());

        Note n = Note::getNearestNote(e->getDuration());
        
        e->set<Int>(Note::NoteType, n.getNoteType());
        e->set<Int>(Note::NoteDots, n.getDots());
        
        collapsedEvent = *nextEvent;
        collapseForward = true;

        erase(nextEvent);

    } else if (previousEvent != end() &&
               canCollapse(previousEvent, elPos)) {

        // collapse with previous event
        (*previousEvent)->setDuration(e->getDuration() +
                                      (*previousEvent)->getDuration());

        Note n = Note::getNearestNote((*previousEvent)->getDuration());
        
        (*previousEvent)->set<Int>(Note::NoteType, n.getNoteType());
        (*previousEvent)->set<Int>(Note::NoteDots, n.getDots());

        collapsedEvent = e;
        erase(elPos);
        collapseForward = false;
    }
    
    
    return collapsedEvent != 0;
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

bool Track::canCollapse(Track::iterator a, Track::iterator b)
{
    time_t durationMax, durationMin;

    if ((*a)->getDuration() > (*b)->getDuration()) {
        durationMax = (*a)->getDuration();
        durationMin = (*b)->getDuration();
    } else {
        durationMax = (*b)->getDuration();
        durationMin = (*a)->getDuration();
    }

    return ((durationMax == durationMin) ||
            (durationMax == (2 * durationMin)));
    // TODO : and some test on not fucking up bar count
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



int Track::getNextGroupId() const
{
    return m_groupId++;
}

bool Track::expandIntoTie(iterator i,
                          timeT baseDuration,
                          iterator& lastInsertedEvent)
{
    if (i == end()) return false;

    iterator i2 = i;
    ++i2;
    
    return expandIntoTie(i, i2, baseDuration, lastInsertedEvent);
}

bool Track::expandIntoTie(iterator from, iterator to,
                          timeT baseDuration,
                          iterator& lastInsertedEvent)
{
    cerr << "Track::expandIntoTie(" << baseDuration << ")\n";

    timeT eventDuration = (*from)->getDuration();
    timeT baseTime = (*from)->getAbsoluteTime();

    if (baseDuration == eventDuration) {
        cerr << "Track::expandIntoTie() : baseDuration == eventDuration\n";
        return true;
    }
    
    timeT maxDuration = 0,
          minDuration = 0;
    
    if (baseDuration > eventDuration) {
        maxDuration = baseDuration;
        minDuration = eventDuration;
    } else {
        maxDuration = eventDuration;
        minDuration = baseDuration;
    }

    // Check if we can perform the operation
    //
    if (checkExpansionValid(maxDuration, minDuration)) {
  
        long firstGroupId = -1;
        (void)(*from)->get<Int>(BeamedGroupIdPropertyName, firstGroupId);

        long nextGroupId = -1;
        iterator ni(to);
        if (ni != end() && ++ni != end()) {
            (void)(*ni)->get<Int>(BeamedGroupIdPropertyName, nextGroupId);
        }
          
        // Expand all the events in range [from, to[
        //
        for (iterator i = from; i != to; ++i) {

            if ((*i)->getAbsoluteTime() != baseTime) {
                // there's no way to really report an error,
                // because at this point we may already have
                // expanded some events. So since returning false
                // means "no change was made at all", it's better
                // to silently skip this event
                continue;
            }

            // set the initial event's duration to base
            (*i)->setDuration(minDuration);
            
            // Add 2nd event
            Event* ev = new Event(*(*i));
            ev->setDuration(maxDuration - minDuration);
            ev->setAbsoluteTime((*i)->getAbsoluteTime() + minDuration);       

            // we only want to tie Note events:

            if ((*i)->isa(Note::EventType)) {

                // if the first event was already tied forward, the
                // second one will now be marked as tied forward
                // (which is good).  set up the relationship between
                // the original (now shorter) event and the new one.

                    ev->set<Bool>(Note::TiedBackwardPropertyName, true);
                  (*i)->set<Bool>(Note:: TiedForwardPropertyName, true);
            }

            // we may also need to change some group information: if
            // the first event is in a beamed group but the event
            // following the insertion is not or is in a different
            // group, then the new second event should not be in a
            // group.  otherwise, it should inherit the grouping info
            // from the first event (as it already does, because it
            // was created using the copy constructor).

            //!!! Note that the whole division principle collapses if
            //tuplets are involved.  That might be an acceptable
            //behaviour, though, as the user can hardly expect an
            //exact division where tuplets are present.

            if (firstGroupId != -1 && nextGroupId != firstGroupId) {
                ev->unset(BeamedGroupIdPropertyName);
                ev->unset(BeamedGroupTypePropertyName);
            }

            lastInsertedEvent = insert(ev);
        }
    
        cerr << "Track::expandIntoTie() returning true\n";
        return true;

    } else { // expansion is not possible
        
        cerr << "Track::expandIntoTie() returning false\n";
        return false;
    }


}

bool Track::expandAndInsertEvent(Event *baseEvent, timeT baseDuration,
                                 iterator& lastInsertedEvent)
{
    insert(baseEvent);

    timeT eventDuration = baseEvent->getDuration();
    timeT baseTime = baseEvent->getAbsoluteTime();

    if (baseDuration == eventDuration) {
        return true;
    }
    
    timeT maxDuration = 0,
        minDuration = 0;
    
    if (baseDuration > eventDuration) {
        maxDuration = baseDuration;
        minDuration = eventDuration;
    } else {
        maxDuration = eventDuration;
        minDuration = baseDuration;
    }

    if (checkExpansionValid(maxDuration, minDuration)) {

        baseEvent->setDuration(minDuration);

        // Add 2nd event
        Event* ev = new Event(*baseEvent);
        ev->setDuration(maxDuration - minDuration);
        ev->setAbsoluteTime(baseTime + minDuration);       

        // we only want to tie Note events:

        if (baseEvent->isa(Note::EventType)) {

            // if the first event was already tied forward, the second
            // one will now be marked as tied forward (which is good).
            // set up the relationship between the original (now
            // shorter) event and the new one.
        
                   ev->set<Bool>(Note::TiedBackwardPropertyName, true);
            baseEvent->set<Bool>(Note:: TiedForwardPropertyName, true);
        }

        // we won't bother with the group tests that we do in
        // expandIntoTie, because there is no "following" event to
        // compare with yet.  (In theory we could do the tests, but
        // we're lazy -- let's wait and see whether the behaviour
        // seems okay in practice first)

        lastInsertedEvent = insert(ev);

        return true;
    } else {
        // expansion is not possible - only the base event has been inserted
        return false;
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

    int dist = distance(res.first,res.second);

    return dist > 1;
}


bool Track::checkExpansionValid(timeT maxDuration, timeT minDuration)
{
    return ((maxDuration == (2 * minDuration)) ||
            (maxDuration == (4 * minDuration)) ||
            (maxDuration == (4 * minDuration / 3)));
}

 
}
