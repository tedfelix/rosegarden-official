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

const PropertyName Track::BeamedGroupIdPropertyName   = "BGroupId";
const PropertyName Track::BeamedGroupTypePropertyName = "BGroupType";
    
Track::Track(unsigned int nbTimeSteps, timeT startIdx,
             unsigned int stepsPerBar)
    : std::multiset<Event*, Event::EventCmp>(),
    m_startIdx(startIdx),
    m_instrument(0),
    m_groupId(0),
    m_quantizer(new Quantizer())
{
    unsigned int initialTime = m_startIdx;
    
    //!!! ugh, must ditch this

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
    notifyTrackGone();

    // delete content
    for (iterator it = begin(); it != end(); ++it)
         delete (*it);

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

//    if (sig != rend() ||
//        ((*sig) && (*sig)->isa(TimeSignature::EventType))) {
    if (sig != rend()) {
	assert((*sig)->isa(TimeSignature::EventType));
        absTimeOfSig = (*sig)->getAbsoluteTime();
	timesig = TimeSignature(*(*sig));
    }

    return timesig;
}


// was called getNbTimeSteps, but getDuration is what it is

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
    
    if (d > currentDuration) { // fill up with rests

	fillWithRests(d);

/*!!!
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
             d - currentDuration,  // interval duration
             newElTime - absTimeOfSig);         // start offset

        timeT acc = newElTime;
        for (DurationList::iterator i = dlist.begin(); i != dlist.end(); ++i) {
            Event *e = new Event("rest");
            e->setDuration(*i);
            e->setAbsoluteTime(acc);
            insert(e);
            acc += *i;
        }
*/

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

    m_barPositions.clear();

    bool startNewBar(true);
    bool barCorrect(true);

    timeT absoluteTime = 0;
    timeT thisBarTime = 0;
    timeT barDuration = timeSignature.getBarDuration();

    iterator i(begin());

    for (; i != end(); ++i) {

        Event *e = *i;
        absoluteTime = e->getAbsoluteTime();

        if (startNewBar) {
            addNewBar(absoluteTime, true, barCorrect, timeSignature);
            startNewBar = false;
        }

        if (e->isa(TimeSignature::EventType)) {

            timeSignature = TimeSignature(*e);
            barDuration = timeSignature.getBarDuration();

            if (thisBarTime > 0) {
                addNewBar(absoluteTime, true, true, timeSignature);
                thisBarTime = 0;
            }

        } else if (e->isa(Note::EventType) || e->isa(Note::EventRestType)) {

            bool hasDuration = true;
            timeT d = m_quantizer->quantizeByNote(e);

            if (e->isa(Note::EventType)) {
                iterator i0(i);
                if (++i0 != end() &&
                    (*i0)->getAbsoluteTime() == e->getAbsoluteTime()) {
                    // we're in a chord or something
                    hasDuration = false;
                }
            }

            if (hasDuration) {
                thisBarTime += d;
                cerr << "Track: Quantized duration is " << d
                     << ", current bar now " << thisBarTime << endl;
            }

            if (thisBarTime >= barDuration) {
                barCorrect = (thisBarTime == barDuration);
                thisBarTime = 0;
                startNewBar = true;
            }
        }

        // solely so that absoluteTime is correct after we hit end():
        absoluteTime += e->getDuration();
    }

    if (startNewBar || thisBarTime > 0) {
        addNewBar
            (absoluteTime, false, thisBarTime == barDuration, timeSignature);
    }
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

    return std::distance(m_barPositions.begin(), bpi);
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
//    for (Track::iterator i = from; i != to; ++i)
//        delete *i;
//    
//    std::multiset<Event*, Event::EventCmp>::erase(from, to);

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

bool Track::collapse(Event* e, bool& collapseForward, Event*& collapsedEvent)
{
    collapsedEvent = 0;

    iterator elPos = findSingle(e);

    if (elPos == end()) return false;

    timeT   myDuration = m_quantizer->getNoteQuantizedDuration(*elPos);
    iterator nextEvent = findContiguousNext(elPos),
         previousEvent = findContiguousPrevious(elPos);

    if (nextEvent != end() &&
	isCollapseValid(m_quantizer->getNoteQuantizedDuration(*nextEvent),
			myDuration)) {

        // collapse with next event
        e->setDuration(e->getDuration() + (*nextEvent)->getDuration());


	//!!! no, absolutely not -- we should re-quantize

        Note n = Note::getNearestNote(e->getDuration());
        
        e->set<Int>(Note::NoteType, n.getNoteType());
        e->set<Int>(Note::NoteDots, n.getDots());


        
        collapsedEvent = *nextEvent;
        collapseForward = true;

        erase(nextEvent);

    } else if (previousEvent != end() &&
	       isCollapseValid(m_quantizer->getNoteQuantizedDuration
			       (*previousEvent),
			       myDuration)) {

        // collapse with previous event
        (*previousEvent)->setDuration(e->getDuration() +
                                      (*previousEvent)->getDuration());



	//!!! no, absolutely not -- we should re-quantize

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


//!!! I don't quite understand the logic here.  we can't for example
//collapse a dotted crotchet rest with a dotted minim rest (total
//duration: two minims and one quaver), but this method thinks we can.
//better to add the quantized durations and see if the closest
//possible note has the same duration as we have

bool Track::isCollapseValid(timeT a, timeT b)
{
//    timeT ad = m_quantizer->getQuantizedNoteDuration(*a);
//    timeT bd = m_quantizer->getQuantizedNoteDuration(*b);
    
//    if (ad > bd) {
//        durationMax = ad;
//        durationMin = bd;
//    } else {
//        durationMax = bd;
//        durationMin = ad;
//    }
    timeT durationMax = std::max(a, b);
    timeT durationMin = std::min(a, b);

    return ((durationMax == durationMin) ||
            (durationMax == (2 * durationMin)));
    // TODO : and some test on not fucking up bar count
}


//!!! Not sufficient -- doesn't work for dotted notes (where splitting
//e.g. a dotted crotchet into a crotchet and a quaver is fine) --
//should just check that both durations correspond to decent note
//lengths?  This needs to be relatively permissive, as it's making
//decisions about what notes to allow the user to enter

bool Track::isExpandValid(timeT a, timeT b)
{
    timeT maxDuration = std::max(a, b);
    timeT minDuration = std::min(a, b);
    return ((maxDuration == (2 * minDuration)) ||
            (maxDuration == (4 * minDuration)) ||
            (maxDuration == (4 * minDuration / 3)));
}


Track::iterator Track::expandIntoTie(iterator i, timeT baseDuration)
{
    if (i == end()) return end();
    iterator i2;
    getTimeSlice((*i)->getAbsoluteTime(), i, i2);
    return expandIntoTie(i, i2, baseDuration);
}

Track::iterator Track::expandIntoTie(iterator from, iterator to, timeT baseDuration)
{
    cerr << "Track::expandIntoTie(" << baseDuration << ")\n";


    //!!! not getDuration, and where does baseDuration come from --
    //same quantization problem?


    timeT eventDuration = (*from)->getDuration();
    timeT baseTime = (*from)->getAbsoluteTime();

    if (baseDuration == eventDuration) {
        cerr << "Track::expandIntoTie() : baseDuration == eventDuration\n";
        return end();
    }
    
    if (baseDuration > eventDuration) {
        cerr << "WARNING: Track::expandIntoTie() : baseDuration > eventDuration\n";
        return end();
    }
    
    long firstGroupId = -1;
    (void)(*from)->get<Int>(BeamedGroupIdPropertyName, firstGroupId);

    long nextGroupId = -1;
    iterator ni(to);
    if (ni != end() && ++ni != end()) {
	(void)(*ni)->get<Int>(BeamedGroupIdPropertyName, nextGroupId);
    }

    iterator last = end();
          
    // Expand all the events in range [from, to[
    //
    for (iterator i = from; i != to; ++i) {

	if ((*i)->getAbsoluteTime() != baseTime) {
	    // no way to really cope with an error, because at this
	    // point we may already have expanded some events. Best to
	    // skip this event
	    cerr << "WARNING: Track::expandIntoTie(): (*i)->getAbsoluteTime() != baseTime\n";
	    continue;
	}

	// set the initial event's duration to base
	(*i)->setDuration(baseDuration);
	m_quantizer->quantizeByNote((*i));
            
	// Add 2nd event
	Event* ev = new Event(*(*i));
	ev->setDuration(eventDuration - baseDuration);
	ev->setAbsoluteTime((*i)->getAbsoluteTime() + baseDuration);

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

	m_quantizer->quantizeByNote(ev);
	last = insert(ev);
    }

    return last;
}


/*!!! Commented out while I establish whether we need something like this

bool Track::expandAndInsertEvent(Event *baseEvent, timeT baseDuration,
                                 iterator& lastInsertedEvent)
{
    insert(baseEvent);


    //!!! not getDuration, and where does baseDuration come from --
    //same quantization problem?


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

*/


bool Track::isViable(timeT duration)
{
    duration = m_quantizer->quantizeByUnit(duration);
    return (duration == m_quantizer->quantizeByNote(duration));
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


Track::iterator Track::collapseRestsForInsert(iterator i,
					      timeT desiredDuration)
{
    // collapse at most once, then recurse

    if (i == end() || !(*i)->isa(Note::EventRestType)) return i;

    timeT d = (*i)->getDuration();
    iterator j = findContiguousNext(i);
    if (d >= desiredDuration || j == end()) return ++i;

    (*i)->setDuration(d + (*j)->getDuration());
    m_quantizer->quantizeByNote(*i);
    erase(j);

    return collapseRestsForInsert(i, desiredDuration);
}


void Track::makeRestViable(iterator i)
{
    DurationList dl;
    int barNo = getBarNumber(i);
    TimeSignature tsig = m_barPositions[barNo].timeSignature;
    timeT absTime = (*i)->getAbsoluteTime();

    tsig.getDurationListForInterval
	(dl, (*i)->getDuration(), absTime - m_barPositions[barNo].start);

    cerr << "Track::makeRestViable: Removing rest of duration "
	 << (*i)->getDuration() << " from time " << absTime << endl;

    erase(i);
    
    for (unsigned int i = 0; i < dl.size(); ++i) {
	int duration = dl[i];
	
	cerr << "Track::makeRestViable: Inserting rest of duration "
	     << duration << " at time " << absTime << endl;

	if (duration == 0) {
	    cerr << "WARNING: duration zero; skipping" << endl;
	    continue;
	}

	Event *e = new Event(Note::EventRestType);
	e->setDuration(duration);
	e->setAbsoluteTime(absTime);
	e->setMaybe<String>("Name", "INSERTED_REST"); //!!!
//	m_quantizer->quantizeByNote(e);

	insert(e);
	absTime += duration;
    }
}


void Track::insertNote(timeT absoluteTime, Note note, int pitch)
{

    // Rules:
    // 
    // 1. If we hit a bar line in the course of the intended inserted
    // note, we should split the note rather than make the bar the
    // wrong length.  (Not implemented yet)
    //
    // 2. If there's nothing at the insertion point but rests (and
    // enough of them to cover the entire duration of the new note),
    // then we should insert the new note literally and remove rests
    // as appropriate.  Rests should never prevent us from inserting
    // what the user asked for.
    // 
    // 3. If there are notes in the way, however, we split whenever
    // "reasonable" and truncate our user's not if not reasonable to
    // split.  We can't always give users the Right Thing here, so
    // to hell with them.
    
    // Procedure:
    // 
    // First, if there is a rest at the insertion position, merge it
    // with any following rests, if available, until we have at least
    // the duration of the new note.  Then:
    // 
    // 1. If the new note is the same length as an existing note or
    // rest at that position, chord the existing note or delete the
    // existing rest and insert.
    // 
    // 2. If the new note is shorter than an existing note or rest,
    // split the existing one and chord or replace the first part.
    // 
    // 3. If the new note is longer, split the new note so that the
    // first part is the same duration as the existing note or rest,
    // and recurse (to step 1) with both the first and the second part
    // in turn.

    // 4. Then we somehow need to recover correctness for the
    // second half of any split note or rest...

    //!!! Handle grouping!  (Inserting into the middle of an existing
    // group -- take a cue from NotationView::setupGroup)

    //... 

    iterator i, j;
    getTimeSlice(absoluteTime, i, j);

    int barNo = getBarNumber(i);

//    iterator uncollapsed = collapseRestsForInsert(i, note.getDuration());

    insertSomething(i, note.getDuration(), pitch, false, false);
}


void Track::insertRest(timeT absoluteTime, Note note)
{
    iterator i, j;
    getTimeSlice(absoluteTime, i, j);

    int barNo = getBarNumber(i);

//    iterator uncollapsed = collapseRestsForInsert(i, note.getDuration());

    insertSomething(i, note.getDuration(), 0, true, false);
}


void Track::insertSomething(iterator i, int duration, int pitch,
			    bool isRest, bool tiedBack)
{
    while (i != end() && (*i)->getDuration() == 0) ++i;

    if (i == end()) {
	insertSingleSomething(i, duration, pitch, isRest, tiedBack);
	return;
    }

    collapseRestsForInsert(i, duration);
    timeT existingDuration = (*i)->getDuration();

    cerr << "Track::insertNoteAux: asked to insert duration " << duration
	 << " over this event:" << endl;
    (*i)->dump(cerr);

    if (duration == existingDuration) {

	cerr << "Durations match; doing simple insert" << endl;

	insertSingleSomething(i, duration, pitch, isRest, tiedBack);

    } else if (duration < existingDuration) {

	if ((*i)->isa(Note::EventType)) {

	    if (!isExpandValid((*i)->getDuration(), duration)) {

		cerr << "Bad split, coercing new note" << endl;

		// not reasonable to split existing note, so force new one
		// to same duration instead
		duration = (*i)->getDuration();

	    } else {
		cerr << "Good split, splitting old event" << endl;
		(void)expandIntoTie(i, duration);
	    }
	} else {
	    cerr << "Found rest, splitting" << endl;
	    iterator last = expandIntoTie(i, duration);

	    //!!! In theory, we can do better here -- sometimes the rest
	    // _is_ viable but the duration-list equivalent would still
	    // be better.  Unfortunately I'm not currently quite sure how
	    // to tell.

	    if (last != end() && !isViable(*last)) makeRestViable(last);
	}

	insertSingleSomething(i, duration, pitch, isRest, tiedBack);

    } else { // duration > existingDuration

	bool needToSplit = true;

	// special case: existing event is a rest, and it's at the end
	// of the track

	if ((*i)->isa(Note::EventRestType)) {
	    iterator j;
	    for (j = i; j != end(); ++j) {
		if ((*j)->isa(Note::EventType)) break;
	    }
	    if (j == end()) needToSplit = false;
	}
	
	if (needToSplit) {

	    //!!! This is not quite right for rests.  Because they
	    // replace (rather than chording with) any events already
	    // present, they don't need to be split in the case where
	    // their duration spans several note-events.  Worry about
	    // that later, I guess.

	    cerr << "Need to split new note" << endl;

	    i = insertSingleSomething(i, existingDuration, pitch, isRest, tiedBack);
	    if (!isRest) (*i)->set<Bool>(Note::TiedForwardPropertyName, true);

	    iterator dummy;
	    getTimeSlice((*i)->getAbsoluteTime() + existingDuration, i, dummy);

	    insertSomething(i, duration - existingDuration, pitch, isRest, true);

	} else {

	    cerr << "No need to split new note" << endl;

	    i = insertSingleSomething(i, duration, pitch, isRest, tiedBack);
	}
    }
}

Track::iterator Track::insertSingleSomething(iterator i, int duration, int pitch,
					     bool isRest, bool tiedBack)
{
    timeT time;

    if (i == end()) {
	time = getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if (isRest || (*i)->isa(Note::EventRestType)) erase(i);
    }

    Event *e = new Event(isRest ? Note::EventRestType : Note::EventType);
    e->setAbsoluteTime(time);
    e->setDuration(duration);

    if (!isRest) e->set<Int>("pitch", pitch);
    if (tiedBack && !isRest) e->set<Bool>(Note::TiedBackwardPropertyName, true);

    return insert(e);
}


void Track::deleteNote(Event *e)
{
    iterator i = findSingle(e);

    if (noteIsInChord(e)) {

	erase(i);

    } else {
	
	// replace with a rest
	Event *newRest = new Event(Note::EventRestType);
	newRest->setAbsoluteTime(e->getAbsoluteTime());
	newRest->setDuration(e->getDuration());
	insert(newRest);
	erase(i);
    }
}


void Track::deleteRest(Event *e)
{
    //!!! streamline

    bool collapseForward;
    Event *deletedEvent;

    (void)collapse(e, collapseForward, deletedEvent);
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

 
}
