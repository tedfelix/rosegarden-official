// -*- c-basic-offset: 4 -*-


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


bool SegmentNotationHelper::collapseIfValid(Event* e, bool& collapseForward)
{
    iterator elPos = segment().findSingle(e);
    if (elPos == end()) return false;

    timeT   myDuration = quantizer().getNoteQuantizedDuration(*elPos);
    iterator nextEvent = segment().findContiguousNext(elPos),
         previousEvent = segment().findContiguousPrevious(elPos);

    // collapse to right if (a) not at end...
    if (nextEvent != end() &&
	// ...(b) notes can be merged to a single, valid unit
 	isCollapseValid(quantizer().getNoteQuantizedDuration(*nextEvent),
			myDuration) &&
	// ...(c) event is in same bar (no cross-bar collapsing)
	(*nextEvent)->getAbsoluteTime() <
	    segment().getBarEnd(e->getAbsoluteTime())) {

        // collapse right is OK; collapse e with nextEvent

        e->setDuration(e->getDuration() + (*nextEvent)->getDuration());

        quantizer().unquantize(e);

        collapseForward = true;
        erase(nextEvent);
	return true;
    }

    // logic is exactly backwards from collapse to right logic above
    if (previousEvent != end() &&
	isCollapseValid(quantizer().getNoteQuantizedDuration(*previousEvent),
			myDuration) &&
	(*previousEvent)->getAbsoluteTime() >
	    segment().getBarStart(e->getAbsoluteTime())) {
	
        // collapse left is OK; collapse e with previousEvent
        (*previousEvent)->setDuration(e->getDuration() +
                                      (*previousEvent)->getDuration());

        quantizer().unquantize(*previousEvent);

        collapseForward = false;
        erase(elPos);
	return true;
    }
    
    return false;
}


bool SegmentNotationHelper::isCollapseValid(timeT a, timeT b)
{
    return (isViable(a + b));
}


bool SegmentNotationHelper::isExpandValid(timeT a, timeT b)
{
    return (isViable(a) && isViable(b));
}


Segment::iterator SegmentNotationHelper::expandIntoTie(iterator i, timeT baseDuration)
{
    if (i == end()) return end();
    iterator i2;
    segment().getTimeSlice((*i)->getAbsoluteTime(), i, i2);
    return expandIntoTie(i, i2, baseDuration);
}

Segment::iterator SegmentNotationHelper::expandIntoTie(iterator from, iterator to, timeT baseDuration)
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

    iterator last = end();
    list<Event *> toInsert;
          
    // Expand all the events in range [from, to[
    //
    for (iterator i = from; i != to; ++i) {

	if ((*i)->getAbsoluteTime() != baseTime) {
	    // no way to really cope with an error, because at this
	    // point we may already have expanded some events. Best to
	    // skip this event
	    cerr << "WARNING: SegmentNotationHelper::expandIntoTie(): (*i)->getAbsoluteTime() != baseTime (" << (*i)->getAbsoluteTime() << " vs " << baseTime << "), ignoring this event\n";
	    continue;
	}

        if ((*i)->getDuration() != eventDuration) {
	    cerr << "WARNING: SegmentNotationHelper::expandIntoTie(): (*i)->getDuration() != eventDuration (" << (*i)->getDuration() << " vs " << eventDuration << "), changing eventDuration to match\n";
            eventDuration = (*i)->getDuration();
        }

        if (baseDuration >= eventDuration) {
            cerr << "SegmentNotationHelper::expandIntoTie() : baseDuration >= eventDuration, ignoring event\n";
            continue;
        }

	// set the initial event's duration to base
	(*i)->setDuration(baseDuration);
	quantizer().unquantize(*i);
            
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

  	      ev->set<Bool>(TIED_BACKWARD, true);
	    (*i)->set<Bool>(TIED_FORWARD, true);
	}

	// we may also need to change some group information: if
	// the first event is in a beamed group but the event
	// following the insertion is not or is in a different
	// group, then the new second event should not be in a
	// group.  otherwise, it should inherit the grouping info
	// from the first event (as it already does, because it
	// was created using the copy constructor).

	if (firstGroupId != -1 && nextGroupId != firstGroupId) {
	    ev->unset(BEAMED_GROUP_ID);
	    ev->unset(BEAMED_GROUP_TYPE);
	}

	quantizer().unquantize(ev);

        // We don't want to do the insert yet, as we'd find ourselves
        // iterating over the new event unexpectedly (symptom: we get
        // to see the baseTime WARNING above, even though the "to"
        // iterator was fine when we entered the function).  Not quite
        // clear why this happens though
        toInsert.push_back(ev);
    }

    // now insert the new events
    for (list<Event *>::iterator i = toInsert.begin();
         i != toInsert.end(); ++i) {
        last = insert(*i);
    }

    return last;
}

bool SegmentNotationHelper::isViable(timeT duration, int dots)
{
    bool viable;
    duration = quantizer().quantizeByUnit(duration);

    if (dots >= 0) {
        viable = (duration == Quantizer(1, dots).quantizeByNote(duration));
    } else {
        viable = (duration == quantizer().quantizeByNote(duration));
    }

    return viable;
}


void SegmentNotationHelper::makeRestViable(iterator i)
{
    DurationList dl;
    timeT absTime = (*i)->getAbsoluteTime(); 

    TimeSignature timeSig;
//!!!    timeT sigTime = segment().findTimeSignatureAt(absTime, timeSig);
    timeT sigTime = segment().getComposition()->getTimeSignatureAt
	(absTime, timeSig);

    timeSig.getDurationListForInterval
	(dl, (*i)->getDuration(), absTime - sigTime);

    cerr << "SegmentNotationHelper::makeRestViable: Removing rest of duration "
	 << (*i)->getDuration() << " from time " << absTime << endl;

    erase(i);
    
    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	int duration = *i;
	
	cerr << "SegmentNotationHelper::makeRestViable: Inserting rest of duration "
	     << duration << " at time " << absTime << endl;

	Event *e = new Event(Note::EventRestType);
	e->setDuration(duration);
	e->setAbsoluteTime(absTime);

	insert(e);
	absTime += duration;
    }
}


void SegmentNotationHelper::makeNoteViable(iterator i)
{
    // We don't use quantized values here; we want a precise division.
    // Even if it doesn't look precise on the score (because the score
    // is quantized), we want any playback to produce exactly the same
    // duration of note as was originally recorded

    DurationList dl;

    timeT acc = 0;
    timeT required = (*i)->getDuration();

    // Behaviour differs from TimeSignature::getDurationListForInterval

    while (acc < required) {
        timeT component = Note::getNearestNote(required - acc).getDuration();
        if (component > (required - acc)) dl.push_back(required - acc);
        else dl.push_back(component);
        acc += component;
    }
    
    Event *e = new Event(*(*i));
    quantizer().unquantize(e);

    bool lastTiedForward;
    e->get<Bool>(TIED_FORWARD, lastTiedForward);

    e->set<Bool>(TIED_FORWARD, true);
    erase(i);

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {

        DurationList::iterator j(i);
        if (++j == dl.end() && !lastTiedForward) {
            e->unset(TIED_FORWARD);
        }

        e->setDuration(*i);
        insert(new Event(*e));
        e->addAbsoluteTime(*i);

        e->set<Bool>(TIED_BACKWARD, true);
    }

    delete e;
}


Segment::iterator
SegmentNotationHelper::insertNote(timeT absoluteTime, Note note, int pitch,
				Accidental explicitAccidental)
{

    //!!! Handle grouping!  (Inserting into the middle of an existing
    // group -- take a cue from NotationView::setupGroup)


    //... 

    iterator i, j;
    segment().getTimeSlice(absoluteTime, i, j);

    //!!! Deal with end-of-bar issues!

//    int barNo = segment().getBarNumber(i);

    return insertSomething(i, note.getDuration(), pitch, false, false,
			   explicitAccidental);
}


Segment::iterator
SegmentNotationHelper::insertRest(timeT absoluteTime, Note note)
{
    iterator i, j;
    segment().getTimeSlice(absoluteTime, i, j);

    //!!! Deal with end-of-bar issues!

//    int barNo = segment().getBarNumber(i);

    return insertSomething(i, note.getDuration(), 0, true, false,
			   NoAccidental);
}


Segment::iterator
SegmentNotationHelper::collapseRestsForInsert(iterator i,
					    timeT desiredDuration)
{
    // collapse at most once, then recurse

    if (i == end() || !(*i)->isa(Note::EventRestType)) return i;

    timeT d = (*i)->getDuration();
    iterator j = segment().findContiguousNext(i);
    if (d >= desiredDuration || j == end()) return ++i;

    (*i)->setDuration(d + (*j)->getDuration());
    quantizer().unquantize(*i);
    erase(j);

    return collapseRestsForInsert(i, desiredDuration);
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
    collapseRestsForInsert(i, duration);

    timeT existingDuration = (*i)->getDuration();

    cerr << "SegmentNotationHelper::insertSomething: asked to insert duration " << duration
	 << " over this event:" << endl;
    (*i)->dump(cerr);

    if (duration == existingDuration) {

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

	    if (!isExpandValid(quantizer().getNoteQuantizedDuration(*i),
                               duration)) {

		cerr << "Bad split, coercing new note" << endl;

		// not reasonable to split existing note, so force new one
		// to same duration instead
		duration = (*i)->getDuration();

	    } else {
		cerr << "Good split, splitting old event" << endl;
		expandIntoTie(i, duration);
	    }
	} else {
	    cerr << "Found rest, splitting" << endl;
	    iterator last = expandIntoTie(i, duration);

            // Recover viability for the second half of any split rest

            if (last != end() && !isViable(*last, 1)) {
                makeRestViable(last);
            }
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

    if (i == end()) {
	time = segment().getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if (isRest || (*i)->isa(Note::EventRestType)) eraseI = true;
    }

    Event *e = new Event(isRest ? Note::EventRestType : Note::EventType);
    e->setAbsoluteTime(time);
    e->setDuration(duration);

    if (!isRest) {
        e->set<Int>(PITCH, pitch);
        if (acc != NoAccidental) {
            e->set<String>(ACCIDENTAL,
                           NotationDisplayPitch::getAccidentalName(acc));
        }
        setInsertedNoteGroup(e, i);
    }

    if (tiedBack && !isRest) {
        e->set<Bool>(TIED_BACKWARD, true);
    }

    if (eraseI) erase(i);

    return insert(e);
}

void
SegmentNotationHelper::setInsertedNoteGroup(Event *e, iterator i)
{
    if (i == begin() || i == end() || !((*i)->isa(Note::EventType))) return;

    iterator j = segment().findContiguousPrevious(i);
    if (j == end()) return;

    if ((*i)->has(BEAMED_GROUP_ID) &&
        (*j)->has(BEAMED_GROUP_ID) &&
        (*i)->get<Int>(BEAMED_GROUP_ID) ==
        (*j)->get<Int>(BEAMED_GROUP_ID)) {

        e->setMaybe<Int>(BEAMED_GROUP_ID,
                         (*i)->get<Int>(BEAMED_GROUP_ID));
        e->set<String>(BEAMED_GROUP_TYPE,
                       (*i)->get<String>(BEAMED_GROUP_TYPE));
    }
}


Segment::iterator
SegmentNotationHelper::insertClef(timeT absoluteTime, Clef clef)
{
    return insert(clef.getAsEvent(absoluteTime));
}


void SegmentNotationHelper::deleteNote(Event *e, bool collapseRest)
{
    iterator i = segment().findSingle(e);

    if (segment().noteIsInChord(e)) {

	erase(i);

    } else {
	
	// replace with a rest
	Event *newRest = new Event(Note::EventRestType);
	newRest->setAbsoluteTime(e->getAbsoluteTime());
	newRest->setDuration(e->getDuration());
	insert(newRest);
	erase(i);

	// collapse the new rest
        if (collapseRest) {
            bool dummy;
            collapseIfValid(newRest, dummy);
        }

    }
}

bool SegmentNotationHelper::deleteRest(Event *e)
{
    bool collapseForward;
    return collapseIfValid(e, collapseForward);
}

bool SegmentNotationHelper::deleteEvent(Event *e)
{
    bool res = true;

    if (e->isa(Note::EventType)) deleteNote(e);
    else if (e->isa(Note::EventRestType)) res = deleteRest(e);
    else {
        // just plain delete
        iterator i = segment().findSingle(e);
        erase(i);
    }

    return res;    
}


bool SegmentNotationHelper::hasEffectiveDuration(iterator i)
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
	(segment().findTime((*from)->getAbsoluteTime()),
	 (to == end()) ? to : segment().findTime((*to)->getAbsoluteTime()),
	 type);
}

void
SegmentNotationHelper::makeBeamedGroupAux(iterator from, iterator to,
					  string type)
{
    int groupId = segment().getNextId();
    
    for (iterator i = from; i != to; ++i) {
        (*i)->setMaybe<Int>(BEAMED_GROUP_ID, groupId);
        (*i)->set<String>(BEAMED_GROUP_TYPE, type);
    }
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
	(segment().findTime((*from)->getAbsoluteTime()),
	 (to == end()) ? to : segment().findTime((*to)->getAbsoluteTime()));
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

void SegmentNotationHelper::autoBeam(timeT from, timeT to, string type)
{
    autoBeam(segment().findTime(from), segment().findTime(to), type);
}

void SegmentNotationHelper::autoBeam(iterator from, iterator to, string type)
{
    // This can only manage whole bars at a time, and it will expand
    // the from-to range out to encompass the whole bars in which they
    // each occur

    for (;;) {

	timeT t = (*from)->getAbsoluteTime();

/*!!!
	iterator barStart = segment().findStartOfBar(t);
	iterator barEnd = segment().findStartOfNextBar(t);
*/
	iterator barStart =
	    segment().findTime(segment().getBarStart(t));
	iterator barEnd = 
	    segment().findTime(segment().getBarEnd(t));

	TimeSignature timeSig =
	    segment().getComposition()->getTimeSignatureAt(t);
//!!!	segment().findTimeSignatureAt(t, timeSig);

//	cerr << "SegmentNotationHelper::autoBeam: t is " << t << "; from time " <<
//	    (*barStart)->getAbsoluteTime() << " to time " <<
//	    (barEnd == end() ? -1 : (*barEnd)->getAbsoluteTime()) << endl;
 
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

void SegmentNotationHelper::autoBeamBar(iterator from, iterator to,
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
            while (num % n != 0) ++n;
            average = n * Note(Note::Semiquaver).getDuration();
        }
    }

    if (minimum == 0) minimum = average / 2;
    if (denom > 4) average /= 2;

    autoBeamBar(from, to, average, minimum, average * 4, type);
}


void SegmentNotationHelper::autoBeamBar(iterator from, iterator to,
                                      timeT average, timeT minimum,
                                      timeT maximum, string type)
{
    timeT accumulator = 0;
    timeT crotchet    = Note(Note::Crotchet).getDuration();
    timeT semiquaver  = Note(Note::Semiquaver).getDuration();

    for (iterator i = from; i != to; ++i) {

        // only look at one note in each chord, and at rests
        if (!hasEffectiveDuration(i)) continue;
        timeT idur = quantizer().getNoteQuantizedDuration(*i);

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
                timeT jdur = quantizer().getNoteQuantizedDuration(*j);

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
			quantizer().getNoteQuantizedDuration(*jnext) > jdur)
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


// based on Rosegarden 2.1's GuessItemListClef in editor/src/MidiIn.c

Clef SegmentNotationHelper::guessClef(iterator from, iterator to)
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

bool SegmentNotationHelper::removeRests(timeT time, timeT duration)
{
    Event dummy;
    
    dummy.setAbsoluteTime(time);

    cerr << "SegmentNotationHelper::removeRests(" << time
         << ", " << duration << ")\n";

    iterator from = segment().lower_bound(&dummy);

    if (from == segment().end()) return false;
    
    iterator to = from;

    cerr << "SegmentNotationHelper::removeRests : start at "
         << (*to)->getAbsoluteTime() << endl;

    timeT eventTime = time;
    timeT finalTime = time + duration;

    // Iterate on events, checking if all are rests
    //
    while ((eventTime < finalTime) && (to != end())) {

        cerr << "SegmentNotationHelper::removeRests : eventTime : "
             << eventTime << " finalTime : " << finalTime << endl;
        
        if (!(*to)->isa(Note::EventRestType)) {
            // a non-rest was found
            cerr << "SegmentNotationHelper::removeRests : an event of type "
                 << (*to)->getType() << " was found - abort\n";
            return false;
        }

        timeT nextEventDuration = (*to)->getDuration();

        cerr << "SegmentNotationHelper::removeRests : nextEventDuration : "
             << nextEventDuration << endl;

        if ((eventTime + nextEventDuration) <= finalTime)
            eventTime += nextEventDuration;
        else
            break;

        ++to;
    }

    bool checkLastRest = false;
    iterator lastEvent = to;
    
    if (eventTime < finalTime) {
        // shorten last event's duration, if possible


        if (lastEvent == end()) {
            cerr << "SegmentNotationHelper::removeRests : not enough rest space\n";
            return false;
        }

        cerr << "SegmentNotationHelper::removeRests : shorten last event duration from "
             << (*lastEvent)->getDuration() << " to "
             << (*lastEvent)->getDuration() - (finalTime - eventTime)
             << endl;

        (*lastEvent)->setAbsoluteTime(finalTime);
        (*lastEvent)->setDuration((*lastEvent)->getDuration() - (finalTime - eventTime));
        checkLastRest = true;
    }

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
    quantizer().quantizeLegato(begin(), end());

    for (iterator i = begin(); i != end(); ++i) {

	if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {

	    timeT duration = (*i)->get<Int>(Quantizer::LegatoDurationProperty);
	    Note n(Note::getNearestNote(duration));
	    (*i)->setMaybe<Int>(NOTE_TYPE, n.getNoteType());
	    (*i)->setMaybe<Int>(NOTE_DOTS, n.getDots());
	}
    }
}


void SegmentNotationHelper::normalizeRests(timeT startTime, timeT endTime)
{
    reorganizeRests(startTime, endTime,
		    &SegmentNotationHelper::normalizeContiguousRests);
}

void SegmentNotationHelper::collapseRestsAggressively(timeT startTime,
						      timeT endTime)
{
    reorganizeRests(startTime, endTime,
		    &SegmentNotationHelper::mergeContiguousRests);
}


void SegmentNotationHelper::reorganizeRests(timeT startTime, timeT endTime,
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


void SegmentNotationHelper::normalizeContiguousRests(timeT startTime,
						     timeT duration,
						     std::vector<Event *> &
						         toInsert)
{
    TimeSignature ts;
    timeT sigTime =
	segment().getComposition()->getTimeSignatureAt(startTime, ts);

    cerr << "SegmentNotationHelper::normalizeContiguousRests:"
	 << " startTime = " << startTime << ", duration = "
	 << duration << endl;

/*!!!
    iterator tsi = segment().findTimeSignatureAt(startTime);
    if (tsi != segment().getReferenceSegment()->end()) {
	ts = TimeSignature(**tsi);
	sigTime = (*tsi)->getAbsoluteTime();
    }
*/

    DurationList dl;
    ts.getDurationListForInterval(dl, duration, sigTime);

    timeT acc = startTime;

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	Event *e = new Event(Note::EventRestType);
	e->setDuration(*i);
	e->setAbsoluteTime(acc);
	toInsert.push_back(e);
	acc += *i;
    }
}


void SegmentNotationHelper::mergeContiguousRests(timeT startTime,
						 timeT duration,
						 std::vector<Event *> &
						     toInsert)
{
    while (duration > 0) {

	timeT d = Note::getNearestNote(duration).getDuration();

	Event *e = new Event(Note::EventRestType);
	e->setDuration(d);
	e->setAbsoluteTime(startTime);
	toInsert.push_back(e);

	startTime += d;
	duration -= d;
    }
}


} // end of namespace

