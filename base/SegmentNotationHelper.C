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

#include "SegmentNotationHelper.h"
#include "NotationTypes.h"
#include "Quantizer.h"
#include "BaseProperties.h"
#include "Composition.h"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <list>

namespace Rosegarden 
{
using std::cerr;
using std::endl;
using std::string;
using std::list;

using namespace BaseProperties;


SegmentNotationHelper::~SegmentNotationHelper() { }


const Quantizer &
SegmentNotationHelper::basicQuantizer() {
    return *(segment().getComposition()->getBasicQuantizer());
}

const Quantizer &
SegmentNotationHelper::noteQuantizer() {
    return *(segment().getComposition()->getNoteQuantizer());
}

const Quantizer &
SegmentNotationHelper::legatoQuantizer() {
    return *(segment().getComposition()->getLegatoQuantizer());
}


timeT
SegmentNotationHelper::getNotationDuration(iterator i)
{
    if ((*i)->has(TUPLET_NOMINAL_DURATION)) {

	return legatoQuantizer().quantizeDuration
	    ((*i)->get<Int>(TUPLET_NOMINAL_DURATION));

    } else if ((*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

	// Code duplicated with SegmentNotationHelper::quantize().
	// This is intended to deal with the case where two edits
	// happen in a row and some events inserted by the first
	// have not been quantized before the second.  We'd be
	// in trouble if the entire segment had not been quantized
	// yet...

	int tcount = (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT);
	int ucount = (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT);
	assert(tcount != 0);
	timeT nominalDuration = ((*i)->getDuration() / tcount) * ucount;
	nominalDuration = legatoQuantizer().quantizeDuration(nominalDuration);
	(*i)->setMaybe<Int>(TUPLET_NOMINAL_DURATION, nominalDuration);
	return nominalDuration;

    } else {
	return legatoQuantizer().getQuantizedDuration(*i);
    }
}


Segment::iterator
SegmentNotationHelper::getNextAdjacentNote(iterator i,
					   bool matchPitch,
					   bool allowOverlap)
{
    iterator j(i);
    if (i == end()) return i;
    if (!(*i)->isa(Note::EventType)) return end();

    timeT iEnd = legatoQuantizer().quantizeAbsoluteTime
	((*i)->getAbsoluteTime() + (*i)->getDuration());
    long ip = 0, jp = 0;
    if (!(*i)->get<Int>(PITCH, ip) && matchPitch) return end();

    while (true) {
	if (j == end() || ++j == end()) return j;
	if (!(*j)->isa(Note::EventType)) continue;

	timeT jStart = legatoQuantizer().quantizeAbsoluteTime
	    ((*j)->getAbsoluteTime());
	if (jStart > iEnd) return end();

	if (matchPitch) {
	    if (!(*j)->get<Int>(PITCH, jp) || (jp != ip)) continue;
	}

	if (allowOverlap || (jStart == iEnd)) return j;
    }
}

   
Segment::iterator
SegmentNotationHelper::getPreviousAdjacentNote(iterator i,
					       timeT rangeStart,
					       bool matchPitch,
					       bool allowOverlap)
{ 
    iterator j(i);
    if (i == end()) return i;
    if (!(*i)->isa(Note::EventType)) return end();

    timeT iStart = legatoQuantizer().quantizeAbsoluteTime
	((*i)->getAbsoluteTime());
    timeT iEnd   = legatoQuantizer().quantizeAbsoluteTime
	((*i)->getAbsoluteTime() + (*i)->getDuration());
    long ip = 0, jp = 0;
    if (!(*i)->get<Int>(PITCH, ip) && matchPitch) return end();

    while (true) {
	if (j == begin()) return end(); else --j;
	if (!(*j)->isa(Note::EventType)) continue;
	if ((*j)->getAbsoluteTime() < rangeStart) return end();

	timeT jEnd = legatoQuantizer().quantizeAbsoluteTime
	    ((*j)->getAbsoluteTime() + (*j)->getDuration());

	// don't consider notes that end after i ends or before i begins

	if (jEnd > iEnd || jEnd < iStart) continue;

	if (matchPitch) {
	    if (!(*j)->get<Int>(PITCH, jp) || (jp != ip)) continue;
	}

	if (allowOverlap || (jEnd == iStart)) return j;
    }
}


bool
SegmentNotationHelper::collapseIfValid(Event* e, bool& collapseForward)
{
    iterator elPos = segment().findSingle(e);
    if (elPos == end()) return false;

    timeT myDuration = getNotationDuration(elPos);

    iterator nextEvent = segment().findContiguousNext(elPos),
	 previousEvent = segment().findContiguousPrevious(elPos);

    //!!! This method fails for notes -- fortunately it's not used for
    // notes at the moment.   (findContiguousXXX is inadequate for
    // notes, we need to check adjacency using e.g. getNextAdjacentNote)

    // collapse to right if (a) not at end...
    if (nextEvent != end() &&
	// ...(b) notes can be merged to a single, valid unit
 	isCollapseValid(getNotationDuration(nextEvent), myDuration) &&
	// ...(c) event is in same bar (no cross-bar collapsing)
	(*nextEvent)->getAbsoluteTime() <
	    segment().getBarEndForTime(e->getAbsoluteTime())) {

        // collapse right is OK; collapse e with nextEvent
	Event *e1(new Event(*e, e->getAbsoluteTime(),
			    e->getDuration() + (*nextEvent)->getDuration()));

        collapseForward = true;
	erase(elPos);
        erase(nextEvent);
	insert(e1);
	return true;
    }

    // logic is exactly backwards from collapse to right logic above
    if (previousEvent != end() &&
	isCollapseValid(getNotationDuration(previousEvent), myDuration) &&
	(*previousEvent)->getAbsoluteTime() >
	    segment().getBarStartForTime(e->getAbsoluteTime())) {
			    
        // collapse left is OK; collapse e with previousEvent
	Event *e1(new Event(**previousEvent,
			    (*previousEvent)->getAbsoluteTime(),
			    e->getDuration() +
			    (*previousEvent)->getDuration()));

        collapseForward = false;
        erase(elPos);
	erase(previousEvent);
	insert(e1);
	return true;
    }
    
    return false;
}


bool
SegmentNotationHelper::isCollapseValid(timeT a, timeT b)
{
    return (isViable(a + b));
}


bool
SegmentNotationHelper::isSplitValid(timeT a, timeT b)
{
    return (isViable(a) && isViable(b));
}

Segment::iterator
SegmentNotationHelper::splitIntoTie(iterator &i, timeT baseDuration)
{
    if (i == end()) return end();
    iterator i2;
    segment().getTimeSlice((*i)->getAbsoluteTime(), i, i2);
    return splitIntoTie(i, i2, baseDuration);
}

Segment::iterator
SegmentNotationHelper::splitIntoTie(iterator &from, iterator to,
				     timeT baseDuration)
{
    // so long as we do the quantization checks for validity before
    // calling this method, we should be fine splitting precise times
    // in this method. only problem is deciding not to split something
    // if its duration is very close to requested duration, but that's
    // probably not a task for this function

    timeT eventDuration = (*from)->getDuration();
    timeT baseTime = (*from)->getAbsoluteTime();

    long firstGroupId = -1;
    (*from)->get<Int>(BEAMED_GROUP_ID, firstGroupId);

    long nextGroupId = -1;
    iterator ni(to);
    if (ni != end() && ++ni != end()) {
	(*ni)->get<Int>(BEAMED_GROUP_ID, nextGroupId);
    }

    list<Event *> toInsert;
    list<iterator> toErase;
          
    // Split all the events in range [from, to[
    //
    for (iterator i = from; i != to; ++i) {

	if ((*i)->getAbsoluteTime() != baseTime) {
	    // no way to really cope with an error, because at this
	    // point we may already have splut some events. Best to
	    // skip this event
	    cerr << "WARNING: SegmentNotationHelper::splitIntoTie(): (*i)->getAbsoluteTime() != baseTime (" << (*i)->getAbsoluteTime() << " vs " << baseTime << "), ignoring this event\n";
	    continue;
	}

        if ((*i)->getDuration() != eventDuration) {
	    if ((*i)->getDuration() == 0) continue;
	    cerr << "WARNING: SegmentNotationHelper::splitIntoTie(): (*i)->getDuration() != eventDuration (" << (*i)->getDuration() << " vs " << eventDuration << "), changing eventDuration to match\n";
            eventDuration = (*i)->getDuration();
        }

        if (baseDuration >= eventDuration) {
            cerr << "SegmentNotationHelper::splitIntoTie() : baseDuration >= eventDuration, ignoring event\n";
            continue;
        }

	// set the initial event's duration to base
	Event *eva = new Event(*(*i), (*i)->getAbsoluteTime(),
				   baseDuration);
            
	// Add 2nd event
	Event* evb = new Event(*(*i), (*i)->getAbsoluteTime() + baseDuration,
			       eventDuration - baseDuration);

	// we only want to tie Note events:

	if (eva->isa(Note::EventType)) {

	    // if the first event was already tied forward, the
	    // second one will now be marked as tied forward
	    // (which is good).  set up the relationship between
	    // the original (now shorter) event and the new one.

	    evb->set<Bool>(TIED_BACKWARD, true);
	    eva->set<Bool>(TIED_FORWARD, true);
	}

	// we may also need to change some group information: if
	// the first event is in a beamed group but the event
	// following the insertion is not or is in a different
	// group, then the new second event should not be in a
	// group.  otherwise, it should inherit the grouping info
	// from the first event (as it already does, because it
	// was created using the copy constructor).

	if (firstGroupId != -1 && nextGroupId != firstGroupId) {
	    evb->unset(BEAMED_GROUP_ID);
	    evb->unset(BEAMED_GROUP_TYPE);
	}

	toInsert.push_back(eva);
        toInsert.push_back(evb);
	toErase.push_back(i);
    }

    // erase the old events
    for (list<iterator>::iterator i = toErase.begin();
	 i != toErase.end(); ++i) {
	segment().erase(*i);
    }

    from = end();
    iterator last = end();

    // now insert the new events
    for (list<Event *>::iterator i = toInsert.begin();
         i != toInsert.end(); ++i) {
        last = insert(*i);
	if (from == end()) from = last;
    }

    return last;
}

bool
SegmentNotationHelper::isViable(timeT duration, int dots)
{
    bool viable;
    duration = basicQuantizer().quantizeDuration(duration);

    if (dots >= 0) {
        viable = (duration == Quantizer(Quantizer::RawEventData,
					Quantizer::DefaultTarget,
					Quantizer::NoteQuantize, 1, dots).
		  quantizeDuration(duration));
    } else {
        viable = (duration == legatoQuantizer().quantizeDuration(duration));
    }

    return viable;
}


void
SegmentNotationHelper::makeRestViable(iterator i)
{
    timeT absTime = (*i)->getAbsoluteTime(); 
    timeT duration = (*i)->getDuration();
    erase(i);
    segment().fillWithRests(absTime, absTime + duration, true);
}


void
SegmentNotationHelper::makeNoteViable(iterator i)
{
    // We don't use quantized values here; we want a precise division.
    // Even if it doesn't look precise on the score (because the score
    // is quantized), we want any playback to produce exactly the same
    // duration of note as was originally recorded

    DurationList dl;

    // Behaviour differs from TimeSignature::getDurationListForInterval


    timeT acc = 0;
    timeT required = (*i)->getDuration();

    while (acc < required) {
        timeT component = Note::getNearestNote(required - acc).getDuration();
        if (component > (required - acc)) dl.push_back(required - acc);
        else dl.push_back(component);
        acc += component;
    }
    
    Event *e = new Event(*(*i));

    bool lastTiedForward;
    e->get<Bool>(TIED_FORWARD, lastTiedForward);

    e->set<Bool>(TIED_FORWARD, true);
    erase(i);
    acc = e->getAbsoluteTime();

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {

        DurationList::iterator j(i);
        if (++j == dl.end() && !lastTiedForward) {
            e->unset(TIED_FORWARD);
        }

        insert(new Event(*e, acc, *i));
	acc += *i;

        e->set<Bool>(TIED_BACKWARD, true);
    }

    delete e;
}


Segment::iterator
SegmentNotationHelper::insertNote(timeT absoluteTime, Note note, int pitch,
				  Accidental explicitAccidental)
{
//    iterator i, j;
//    segment().getTimeSlice(absoluteTime, i, j);

    iterator i = segment().findNearestTime(absoluteTime);

    // If our insertion time doesn't match up precisely with any
    // existing event, and if we're inserting over a rest, split the
    // rest at the insertion time first.
    if (i != end() &&
	(*i)->getAbsoluteTime() < absoluteTime &&
	(*i)->getAbsoluteTime() + (*i)->getDuration() > absoluteTime &&
	(*i)->isa(Note::EventRestType)) {
	i = splitIntoTie(i, absoluteTime - (*i)->getAbsoluteTime());
    }

    timeT duration(note.getDuration());

    if (i != end() && (*i)->has(BEAMED_GROUP_TUPLET_BASE)) {
	duration = duration * (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT) /
	    (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT);
    }

    //!!! Deal with end-of-bar issues!

    return insertSomething(i, duration, pitch, false, false,
			   explicitAccidental);
}


Segment::iterator
SegmentNotationHelper::insertRest(timeT absoluteTime, Note note)
{
    iterator i, j;
    segment().getTimeSlice(absoluteTime, i, j);

    //!!! Deal with end-of-bar issues!

    timeT duration(note.getDuration());

    if (i != end() && (*i)->has(BEAMED_GROUP_TUPLET_BASE)) {
	duration = duration * (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT) /
	    (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT);
    }

    return insertSomething(i, duration, 0, true, false,
			   Accidentals::NoAccidental);
}


// return an iterator pointing to the "same" event as the original
// iterator (which will have been replaced)

Segment::iterator
SegmentNotationHelper::collapseRestsForInsert(iterator i,
					      timeT desiredDuration)
{
    // collapse at most once, then recurse

    if (i == end() || !(*i)->isa(Note::EventRestType)) return i;

    timeT d = (*i)->getDuration();
    iterator j = segment().findContiguousNext(i);
    if (d >= desiredDuration || j == end()) return i;

    Event *e(new Event(**i, (*i)->getAbsoluteTime(), d + (*j)->getDuration()));
    iterator ii(insert(e));
    erase(i);
    erase(j);

    return collapseRestsForInsert(ii, desiredDuration);
}


Segment::iterator
SegmentNotationHelper::insertSomething(iterator i, int duration, int pitch,
				       bool isRest, bool tiedBack,
				       Accidental acc)
{
    // Rules:
    // 
    // 1. If we hit a bar line in the course of the intended inserted
    // note, we should split the note rather than make the bar the
    // wrong length.  (Not implemented yet)
    //
    // 2. If there's nothing at the insertion point but rests (and
    // enough of them to cover the entire duration of the new note),
    // then we should insert the new note/rest literally and remove
    // rests as appropriate.  Rests should never prevent us from
    // inserting what the user asked for.
    // 
    // 3. If there are notes in the way of an inserted note, however,
    // we split whenever "reasonable" and truncate our user's not if
    // not reasonable to split.  We can't always give users the Right
    // Thing here, so to hell with them.

    while (i != end() && (*i)->getDuration() == 0) ++i;

    if (i == end()) {
	return insertSingleSomething
	    (i, duration, pitch, isRest, tiedBack, acc);
    }

    // If there's a rest at the insertion position, merge it with any
    // following rests, if available, until we have at least the
    // duration of the new note.
    i = collapseRestsForInsert(i, duration);

    timeT existingDuration = (*i)->getDuration();

    cerr << "SegmentNotationHelper::insertSomething: asked to insert duration " << duration
	 << " over this event:" << endl;
    (*i)->dump(cerr);

    if (basicQuantizer().quantizeDuration(duration) ==
	basicQuantizer().quantizeDuration(existingDuration)) {

        // 1. If the new note or rest is the same length as an
        // existing note or rest at that position, chord the existing
        // note or delete the existing rest and insert.

	cerr << "Durations match; doing simple insert" << endl;

	return insertSingleSomething
	    (i, duration, pitch, isRest, tiedBack, acc);

    } else if (duration < existingDuration) {

        // 2. If the new note or rest is shorter than an existing one,
        // split the existing one and chord or replace the first part.

	if ((*i)->isa(Note::EventType)) {

	    if (!isSplitValid(getNotationDuration(i), duration)) {

		cerr << "Bad split, coercing new note" << endl;

		// not reasonable to split existing note, so force new one
		// to same duration instead
		duration = (*i)->getDuration();

	    } else {
		cerr << "Good split, splitting old event" << endl;
		splitIntoTie(i, duration);
	    }
	} else {
	    cerr << "Found rest, splitting" << endl;
	    iterator last = splitIntoTie(i, duration);

            // Recover viability for the second half of any split rest

	    makeRestViable(last);
	}

	return insertSingleSomething(i, duration, pitch, isRest, tiedBack,
				     acc);

    } else { // duration > existingDuration

        // 3. If the new note is longer, split the new note so that
        // the first part is the same duration as the existing note or
        // rest, and recurse to step 1 with both the first and the
        // second part in turn.

	bool needToSplit = true;

	// special case: existing event is a rest, and it's at the end
	// of the segment

	if ((*i)->isa(Note::EventRestType)) {
	    iterator j;
	    for (j = i; j != end(); ++j) {
		if ((*j)->isa(Note::EventType)) break;
	    }
	    if (j == end()) needToSplit = false;
	}
	
	if (needToSplit) {

	    //!!! This is not quite right for rests.  Because they
	    //replace (rather than chording with) any events already
	    //present, they don't need to be split in the case where
	    //their duration spans several note-events.  Worry about
	    //that later, I guess.  We're actually getting enough
	    //is-note/is-rest decisions here to make it possibly worth
	    //splitting this method into note and rest versions again

	    cerr << "Need to split new note" << endl;

	    i = insertSingleSomething(i, existingDuration, pitch, isRest,
                                      tiedBack, acc);

	    if (!isRest) (*i)->set<Bool>(TIED_FORWARD, true);

            i = segment().findTime((*i)->getAbsoluteTime() + existingDuration);

	    return insertSomething
		(i, duration - existingDuration, pitch, isRest, true, acc);

	} else {

	    cerr << "No need to split new note" << endl;

	    return insertSingleSomething(i, duration, pitch, isRest,
					 tiedBack, acc);
	}
    }
}

Segment::iterator
SegmentNotationHelper::insertSingleSomething(iterator i, int duration,
					     int pitch, bool isRest,
					     bool tiedBack,
					     Accidental acc)
{
    timeT time;
    bool eraseI = false;
    timeT effectiveDuration(duration);

    if (i == end()) {
	time = segment().getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if (isRest || (*i)->isa(Note::EventRestType)) eraseI = true;
/*!!!
	if ((*i)->has(TUPLET_NOMINAL_DURATION)) {
	    effectiveDuration =
		(effectiveDuration * (*i)->getDuration()) /
		(*i)->get<Int>(TUPLET_NOMINAL_DURATION);
	}
*/
    }

    Event *e = new Event(isRest ? Note::EventRestType : Note::EventType,
			 time, effectiveDuration);

    if (!isRest) {
        e->set<Int>(PITCH, pitch);
        if (acc != Accidentals::NoAccidental) {
            e->set<String>(ACCIDENTAL, acc);
        }
    }

    setInsertedNoteGroup(e, i);

    if (tiedBack && !isRest) {
        e->set<Bool>(TIED_BACKWARD, true);
    }

    if (eraseI) erase(i);

    return insert(e);
}

void
SegmentNotationHelper::setInsertedNoteGroup(Event *e, iterator i)
{
    // Formerly this was posited on the note being inserted between
    // two notes in the same group, but that's quite wrong-headed: we
    // want to place it in the same group as any existing note at the
    // same time, or else the nearest note thereafter if there are no
    // rests in between.  The exception is for tupled groups, where
    // we don't care whether an event is a note or not

    while (i != end()) {
	if ((*i)->has(BEAMED_GROUP_ID)) {

	    string type = (*i)->get<String>(BEAMED_GROUP_TYPE);
	    if (type != GROUP_TYPE_TUPLED && !(*i)->isa(Note::EventType)) {
		if ((*i)->isa(Note::EventRestType)) return;
		else continue;
	    }

	    e->set<Int>(BEAMED_GROUP_ID, (*i)->get<Int>(BEAMED_GROUP_ID));
	    e->set<String>(BEAMED_GROUP_TYPE, type);

	    if ((*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

		e->set<Int>(BEAMED_GROUP_TUPLET_BASE,
			    (*i)->get<Int>(BEAMED_GROUP_TUPLET_BASE));
		e->set<Int>(BEAMED_GROUP_TUPLED_COUNT,
			    (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT));
		e->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT,
			    (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT));
	    }

	    return;

	} else if ((*i)->isa(Note::EventRestType)) return;

	++i;
    }
}


Segment::iterator
SegmentNotationHelper::insertClef(timeT absoluteTime, Clef clef)
{
    return insert(clef.getAsEvent(absoluteTime));
}


Segment::iterator
SegmentNotationHelper::insertKey(timeT absoluteTime, Key key)
{
    return insert(key.getAsEvent(absoluteTime));
}


void
SegmentNotationHelper::deleteNote(Event *e, bool collapseRest)
{
    iterator i = segment().findSingle(e);
    if (i == end()) return;

    if (segment().noteIsInChord(e)) {

	erase(i);

    } else {
	
	// replace with a rest
	Event *newRest = new Event(Note::EventRestType,
				   e->getAbsoluteTime(), e->getDuration());
	insert(newRest);
	erase(i);

	// collapse the new rest
        if (collapseRest) {
            bool dummy;
            collapseIfValid(newRest, dummy);
        }

    }
}

bool
SegmentNotationHelper::deleteRest(Event *e)
{
    bool collapseForward;
    return collapseIfValid(e, collapseForward);
}

bool
SegmentNotationHelper::deleteEvent(Event *e, bool collapseRest)
{
    bool res = true;

    if (e->isa(Note::EventType)) deleteNote(e, collapseRest);
    else if (e->isa(Note::EventRestType)) res = deleteRest(e);
    else {
        // just plain delete
        iterator i = segment().findSingle(e);
	if (i != end()) erase(i);
    }

    return res;    
}


bool
SegmentNotationHelper::hasEffectiveDuration(iterator i)
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


void
SegmentNotationHelper::makeBeamedGroup(timeT from, timeT to, string type)
{
    makeBeamedGroupAux(segment().findTime(from), segment().findTime(to), type);
}

void
SegmentNotationHelper::makeBeamedGroup(iterator from, iterator to, string type)
{
    makeBeamedGroupAux
      ((from == end()) ? from : segment().findTime((*from)->getAbsoluteTime()),
	 (to == end()) ? to   : segment().findTime((*to  )->getAbsoluteTime()),
	 type);
}

void
SegmentNotationHelper::makeBeamedGroupAux(iterator from, iterator to,
					  string type)
{
    int groupId = segment().getNextId();
    
    for (iterator i = from; i != to; ++i) {
        (*i)->set<Int>(BEAMED_GROUP_ID, groupId);
        (*i)->set<String>(BEAMED_GROUP_TYPE, type);
    }
}

void
SegmentNotationHelper::makeTupletGroup(timeT t, int untupled, int tupled,
				       timeT unit)
{
    int groupId = segment().getNextId();

    list<Event *> toInsert;
    list<iterator> toErase;

    for (iterator i = segment().findTime(t); i != end(); ++i) {

	if ((*i)->getAbsoluteTime() >= t + (untupled * unit)) break;

	timeT offset = (*i)->getAbsoluteTime() - t;
	Event *e = new Event(**i,
			     t + (offset * tupled / untupled),
			     (*i)->getDuration() * tupled / untupled);

	e->set<Int>(BEAMED_GROUP_ID, groupId);
	e->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);

	e->set<Int>(BEAMED_GROUP_TUPLET_BASE, unit);
	e->set<Int>(BEAMED_GROUP_TUPLED_COUNT, tupled);
	e->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, untupled);
	e->unset(TUPLET_NOMINAL_DURATION); // should be non-persistent anyway

	toInsert.push_back(e);
	toErase.push_back(i);
    }

    for (list<iterator>::iterator i = toErase.begin();
	 i != toErase.end(); ++i) {
	segment().erase(*i);
    }

    for (list<Event *>::iterator i = toInsert.begin();
	 i != toInsert.end(); ++i) {
	segment().insert(*i);
    }

    segment().fillWithRests(t + (tupled * unit), t + (untupled * unit));
}

    


void
SegmentNotationHelper::unbeam(timeT from, timeT to)
{
    unbeamAux(segment().findTime(from), segment().findTime(to));
}

void
SegmentNotationHelper::unbeam(iterator from, iterator to)
{
    unbeamAux
     ((from == end()) ? from : segment().findTime((*from)->getAbsoluteTime()),
        (to == end()) ? to   : segment().findTime((*to  )->getAbsoluteTime()));
}

void
SegmentNotationHelper::unbeamAux(iterator from, iterator to)
{
    for (iterator i = from; i != to; ++i) {
	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
    }
}



/*

  Auto-beaming code derived from Rosegarden 2.1's ItemListAutoBeam
  and ItemListAutoBeamSub in editor/src/ItemList.c.
  
*/

void
SegmentNotationHelper::autoBeam(timeT from, timeT to, string type)
{
    autoBeam(segment().findTime(from), segment().findTime(to), type);
}

void
SegmentNotationHelper::autoBeam(iterator from, iterator to, string type)
{
    // This can only manage whole bars at a time, and it will split
    // the from-to range out to encompass the whole bars in which they
    // each occur

    for (;;) {

	timeT t = (*from)->getAbsoluteTime();

	timeT barStartTime = segment().getBarStartForTime(t);
	timeT barEndTime   = segment().getBarEndForTime(t);

	iterator barStart  = segment().findTime(barStartTime);
	iterator barEnd    = segment().findTime(barEndTime);

	TimeSignature timeSig =
	    segment().getComposition()->getTimeSignatureAt(t);

	autoBeamBar(barStart, barEnd, timeSig, type);
	
	if (barEnd == end() ||
	    (to != end() &&
	     ((*barEnd)->getAbsoluteTime() > (*to)->getAbsoluteTime()))) return;

	from = barEnd;
    }
}


/*

  Derived from (and no less mystifying than) Rosegarden 2.1's
  ItemListAutoBeamSub in editor/src/ItemList.c.

  "Today I want to celebrate "Montreal" by Autechre, because of
  its sleep-disturbing aura, because it sounds like the sort of music
  which would be going around in the gunman's head as he trains a laser
  sight into your bedroom through the narrow gap in your curtains and
  dances the little red dot around nervously on your wall."
  
*/

void
SegmentNotationHelper::autoBeamBar(iterator from, iterator to,
				   TimeSignature tsig, string type)
{
    int num = tsig.getNumerator();
    int denom = tsig.getDenominator();

    timeT average;
    timeT minimum = 0;

    // If the denominator is 2 or 4, beam in twos (3/4, 6/2 etc).
    
    if (denom == 2 || denom == 4) {

        if (num % 3) {
            average = Note(Note::Quaver).getDuration();
        } else {
            average = Note(Note::Semiquaver).getDuration();
            minimum = average;
        }

    } else {

        if (num == 6 && denom == 8) { // special hack for 6/8
            average = 3 * Note(Note::Quaver).getDuration();

        } else {
            // find a divisor (at least 2) for the numerator
            int n = 2;
            while (num >= n && num % n != 0) ++n;
            average = n * Note(Note::Semiquaver).getDuration();
        }
    }

    if (minimum == 0) minimum = average / 2;
    if (denom > 4) average /= 2;

    autoBeamBar(from, to, average, minimum, average * 4, type);
}


void
SegmentNotationHelper::autoBeamBar(iterator from, iterator to,
				   timeT average, timeT minimum,
				   timeT maximum, string type)
{
    timeT accumulator = 0;
    timeT crotchet    = Note(Note::Crotchet).getDuration();
    timeT semiquaver  = Note(Note::Semiquaver).getDuration();

    for (iterator i = from; i != to; ++i) {

        // only look at one note in each chord, and at rests
        if (!hasEffectiveDuration(i)) continue;
        timeT idur = getNotationDuration(i);

	if (accumulator % average == 0 &&  // "beamable duration" threshold
	    idur < crotchet) {

	    // This could be the start of a beamed group.  We maintain
	    // two sorts of state as we scan along here: data about
	    // the best group we've found so far (beamDuration,
	    // prospective, k etc), and data about the items we're
	    // looking at (count, beamable, longerThanDemi etc) just
	    // in case we find a better candidate group before the
	    // eight-line conditional further down makes us give up
	    // the search, beam our best shot, and start again.

	    // I hope this is clear.

	    iterator k = end(); // best-so-far last item in group;
				// end() indicates that we've found nothing

	    timeT tmin         = minimum;
	    timeT count        = 0;
	    timeT prospective  = 0;
	    timeT beamDuration = 0;

	    int beamable       = 0;
	    int longerThanDemi = 0;

	    for (iterator j = i; j != to; ++j) {

		if (!hasEffectiveDuration(j)) continue;
                timeT jdur = getNotationDuration(j);

		if ((*j)->isa(Note::EventType)) {
		    if (jdur < crotchet) ++beamable;
		    if (jdur >= semiquaver) ++longerThanDemi;
		}

		count += jdur;

		if (count % tmin == 0) {

		    k = j;
		    beamDuration = count;
		    prospective = accumulator + count;

		    // found a group; now accept only double this
		    // group's length for a better one
		    tmin *= 2;
		}

		// Stop scanning and make the group if our scan has
		// reached the maximum length of beamed group, we have
		// more than 4 semis or quavers, we're at the end of
		// our run, the next chord is longer than the current
		// one, or there's a rest ahead.

		iterator jnext(j);

		if ((count > maximum)
		    || (longerThanDemi > 4)
		    || (++jnext == to)     
		    || ((*j    )->isa(Note::EventType) &&
			(*jnext)->isa(Note::EventType) &&
			getNotationDuration(jnext) > jdur)
		    || ((*jnext)->isa(Note::EventRestType))) {

		    if (k != end() && beamable >= 2) {

			iterator knext(k);
			++knext;

			makeBeamedGroup(i, knext, type);
		    }

		    // If this group is at least as long as the check
		    // threshold ("average"), its length must be a
		    // multiple of the threshold and hence we can
		    // continue scanning from the end of the group
		    // without losing the modulo properties of the
		    // accumulator.

		    if (k != end() && beamDuration >= average) {

			i = k;
			accumulator = prospective;

		    } else {

			// Otherwise, we continue from where we were.
			// (This must be safe because we can't get
			// another group starting half-way through, as
			// we know the last group is shorter than the
			// check threshold.)

			accumulator += idur;
		    }

		    break;
		}
	    }
	} else {

	    accumulator += idur;
	}
    }
}


void
SegmentNotationHelper::getClefAndKeyAt(timeT time, Clef &clef, Key &key)
{
    Event *clefEvent = 0;
    Event *keyEvent = 0;

    iterator i = segment().findTime(time);
    while (i != end() && (*i)->getAbsoluteTime() == time) ++i;

    while (i != segment().begin()) {

	--i;

	if (!clefEvent && (*i)->isa(Rosegarden::Clef::EventType)) {
	    clefEvent = *i;
	}

	if (!keyEvent && (*i)->isa(Rosegarden::Key::EventType)) {
	    keyEvent = *i;
	}
	
	if (clefEvent && keyEvent) break;
    }

    if (clefEvent) {
	clef = Clef(*clefEvent);
    } else {
	clef = Clef();
    }

    if (keyEvent) {
	key = Key(*keyEvent);
    } else {
	key = Key();
    }
}


// based on Rosegarden 2.1's GuessItemListClef in editor/src/MidiIn.c

Clef
SegmentNotationHelper::guessClef(iterator from, iterator to)
{
    long totalHeight = 0;
    int noteCount = 0;

    // just the defaults:
    Clef clef;
    Key key;

    for (iterator i = from; i != to; ++i) {
        if ((*i)->isa(Note::EventType)) {
            ++noteCount;
            NotationDisplayPitch p((*i)->get<Int>(PITCH), clef, key);
            totalHeight += p.getHeightOnStaff();
        }
    }

    if    (noteCount == 0) return Clef(Clef::Treble);

    int average = totalHeight / noteCount;

    if      (average < -6) return Clef(Clef::Bass);
    else if (average < -3) return Clef(Clef::Tenor);
    else if (average <  1) return Clef(Clef::Alto);
    else                   return Clef(Clef::Treble);
}


bool
SegmentNotationHelper::removeRests(timeT time, timeT &duration, bool testOnly)
{
    Event dummy("dummy", time, 0, MIN_SUBORDERING);
    
    cerr << "SegmentNotationHelper::removeRests(" << time
         << ", " << duration << ")\n";

    iterator from = segment().lower_bound(&dummy);

    if (from == segment().end()) return false;
    
    iterator to = from;

    cerr << "SegmentNotationHelper::removeRests : start at "
         << (*to)->getAbsoluteTime() << endl;

    timeT eventTime = time;
    timeT finalTime = time + duration;

    //!!! We should probably not use an accumulator, but instead
    // calculate based on each event's absolute time + duration --
    // in case we've somehow ended up with overlapping rests

    // Iterate on events, checking if all are rests
    //
    while ((eventTime < finalTime) && (to != end())) {

        cerr << "SegmentNotationHelper::removeRests : eventTime : "
             << eventTime << " finalTime : " << finalTime << endl;
        
        if (!(*to)->isa(Note::EventRestType)) {
            // a non-rest was found
            cerr << "SegmentNotationHelper::removeRests : an event of type "
                 << (*to)->getType() << " was found - abort\n";
	    duration = (*to)->getAbsoluteTime() - time;
            return false;
        }

        timeT nextEventDuration = (*to)->getDuration();

        cerr << "SegmentNotationHelper::removeRests : nextEventDuration : "
             << nextEventDuration << endl;

        if ((eventTime + nextEventDuration) <= finalTime) {
            eventTime += nextEventDuration;
	    duration = eventTime - time;
	} else break;

        ++to;
    }

    bool checkLastRest = false;
    iterator lastEvent = to;
    
    if (eventTime < finalTime) {
        // shorten last event's duration, if possible


        if (lastEvent == end()) {
            cerr << "SegmentNotationHelper::removeRests : not enough rest space\n";
	    duration = segment().getDuration() - time;
            return false;
        }

        cerr << "SegmentNotationHelper::removeRests : shorten last event duration from "
             << (*lastEvent)->getDuration() << " to "
             << (*lastEvent)->getDuration() - (finalTime - eventTime)
             << endl;

	if (!testOnly) {
	    // can't safely change the absolute time of an event in a segment
	    Event *newEvent = new Event(**lastEvent, finalTime,
					(*lastEvent)->getDuration() -
					(finalTime - eventTime));
	    duration = finalTime + (*lastEvent)->getDuration() - time;
	    segment().erase(lastEvent);
	    lastEvent = segment().insert(newEvent);
	    checkLastRest = true;
	}
    }

    if (testOnly) return true;

    segment().erase(from, to);

    // we must defer calling makeRestViable() until after erase,
    // because it will invalidate 'to'
    //
    if (checkLastRest) makeRestViable(lastEvent);

    return true;
}

void
SegmentNotationHelper::quantize()
{
    legatoQuantizer().quantize(&segment(), begin(), end());

    for (iterator i = begin(); i != end(); ++i) {

	timeT duration = legatoQuantizer().getQuantizedDuration(*i);

	if ((*i)->has(BEAMED_GROUP_TUPLET_BASE)) {
	    int tcount = (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT);
	    int ucount = (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT);
	    assert(tcount != 0);
	    timeT nominalDuration = ((*i)->getDuration() / tcount) * ucount;
	    duration = legatoQuantizer().quantizeDuration(nominalDuration);
	    (*i)->setMaybe<Int>(TUPLET_NOMINAL_DURATION, duration);
	}

	if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {

	    Note n(Note::getNearestNote(duration));
	    (*i)->setMaybe<Int>(NOTE_TYPE, n.getNoteType());
	    (*i)->setMaybe<Int>(NOTE_DOTS, n.getDots());
	}
    }
}

/*!!!
void
SegmentNotationHelper::normalizeRests(timeT startTime, timeT endTime)
{
    //!!! This method should also check for places where rests are
    // absent but necessary (i.e. no notes are sounding -- trickier
    // than it looks), or present but unwanted, or present but simply
    // wrong.  In fact it should probably regenerate the rests
    // completely within the given range...

    reorganizeRests(startTime, endTime,
		    &SegmentNotationHelper::normalizeContiguousRests);
}
*/


void
SegmentNotationHelper::collapseRestsAggressively(timeT startTime,
						 timeT endTime)
{
    reorganizeRests(startTime, endTime,
		    &SegmentNotationHelper::mergeContiguousRests);
}


void
SegmentNotationHelper::reorganizeRests(timeT startTime, timeT endTime,
				       Reorganizer reorganizer)
{
    iterator ia = segment().findTime(startTime);
    iterator ib = segment().findTime(endTime);
    
    if (ia == end()) return;

    std::vector<iterator> erasable;
    std::vector<Event *> insertable;

    cerr << "SegmentNotationHelper::reorganizeRests (" << startTime << ","
	 << endTime << ")" << endl;

    cerr << "ia is at " << (*ia)->getAbsoluteTime() << endl;
    if (ib == end()) cerr << "ib is end()" << endl;
    else cerr << "ib is at " << (*ib)->getAbsoluteTime() << endl;
    

    for (iterator i = ia; i != ib; ++i) {

	cerr << "SegmentNotationHelper::reorganizeRests: looking at i, it's at "
	     << (*i)->getAbsoluteTime() << " and has type " << (*i)->getType()
	     << endl;

	if ((*i)->isa(Note::EventRestType)) {

	    timeT startTime = (*i)->getAbsoluteTime();
	    timeT duration = 0;
	    iterator j = i;

	    for ( ; j != ib; ++j) {

		cerr << "SegmentNotationHelper::reorganizeRests: looking at j, it's at "
		     << (*j)->getAbsoluteTime() << " and has type " << (*j)->getType()
		     << endl;

		if ((*j)->isa(Note::EventRestType)) {
		    duration += (*j)->getDuration();
		    erasable.push_back(j);
		} else break;
	    }

	    (this->*reorganizer)(startTime, duration, insertable);
	    if (j == ib) break;
	    i = j;
	}
    }

    for (unsigned int ei = 0; ei < erasable.size(); ++ei)
	segment().erase(erasable[ei]);

    for (unsigned int ii = 0; ii < insertable.size(); ++ii)
	segment().insert(insertable[ii]);
}


void
SegmentNotationHelper::normalizeContiguousRests(timeT startTime,
						timeT duration,
						std::vector<Event *> &toInsert)
{
    TimeSignature ts;
    timeT sigTime =
	segment().getComposition()->getTimeSignatureAt(startTime, ts);

    cerr << "SegmentNotationHelper::normalizeContiguousRests:"
	 << " startTime = " << startTime << ", duration = "
	 << duration << endl;

    DurationList dl;
    ts.getDurationListForInterval(dl, duration, startTime - sigTime);

    timeT acc = startTime;

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	Event *e = new Event(Note::EventRestType, acc, *i);
	toInsert.push_back(e);
	acc += *i;
    }
}


void
SegmentNotationHelper::mergeContiguousRests(timeT startTime,
					    timeT duration,
					    std::vector<Event *> &toInsert)
{
    while (duration > 0) {

	timeT d = Note::getNearestNote(duration).getDuration();

	Event *e = new Event(Note::EventRestType, startTime, d);
	toInsert.push_back(e);

	startTime += d;
	duration -= d;
    }
}


bool
SegmentNotationHelper::collapseNoteAggressively(Event *note,
						timeT rangeEnd)
{
    iterator i = segment().findSingle(note);
    if (i == end()) return false;

    iterator j = getNextAdjacentNote(i, true, true);
    if (j == end() || (*j)->getAbsoluteTime() >= rangeEnd) return false;

    timeT iEnd = (*i)->getAbsoluteTime() + (*i)->getDuration();
    timeT jEnd = (*j)->getAbsoluteTime() + (*j)->getDuration();

    Event *newEvent = new Event
	(**i, (*i)->getAbsoluteTime(),
	 (std::max(iEnd, jEnd) - (*i)->getAbsoluteTime()));

    newEvent->unset(TIED_BACKWARD);
    newEvent->unset(TIED_FORWARD);

    segment().erase(i);
    segment().erase(j);
    segment().insert(newEvent);
    return true;
}


} // end of namespace

