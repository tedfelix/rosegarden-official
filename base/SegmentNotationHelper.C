// -*- c-basic-offset: 4 -*-


#include "TrackNotationHelper.h"
#include "NotationTypes.h"
#include "Quantizer.h"

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

const PropertyName TrackNotationHelper::BeamedGroupIdPropertyName   = "BGroupId";
const PropertyName TrackNotationHelper::BeamedGroupTypePropertyName = "BGroupType";

TrackNotationHelper::~TrackNotationHelper() { }


bool TrackNotationHelper::collapse(Event* e, bool& collapseForward)
{
    bool success = false;
    iterator elPos = track().findSingle(e);

    if (elPos == end()) return false;

    timeT   myDuration = quantizer().getNoteQuantizedDuration(*elPos);
    iterator nextEvent = track().findContiguousNext(elPos),
         previousEvent = track().findContiguousPrevious(elPos);

    if (nextEvent != end() &&
	isCollapseValid(quantizer().getNoteQuantizedDuration(*nextEvent),
			myDuration)) {

        // collapse with next event
        e->setDuration(e->getDuration() + (*nextEvent)->getDuration());

        quantizer().unquantize(e);

        success = true;
        collapseForward = true;

        erase(nextEvent);

    } else if (previousEvent != end() &&
	       isCollapseValid(quantizer().getNoteQuantizedDuration
			       (*previousEvent),
			       myDuration)) {

        // collapse with previous event
        (*previousEvent)->setDuration(e->getDuration() +
                                      (*previousEvent)->getDuration());

        quantizer().unquantize(*previousEvent);

        success = true;
        collapseForward = false;

        erase(elPos);
    }
    
    return success;
}


bool TrackNotationHelper::isCollapseValid(timeT a, timeT b)
{
    return (isViable(a + b));
}


bool TrackNotationHelper::isExpandValid(timeT a, timeT b)
{
    return (isViable(a) && isViable(b));
}


Track::iterator TrackNotationHelper::expandIntoTie(iterator i, timeT baseDuration)
{
    if (i == end()) return end();
    iterator i2;
    track().getTimeSlice((*i)->getAbsoluteTime(), i, i2);
    return expandIntoTie(i, i2, baseDuration);
}

Track::iterator TrackNotationHelper::expandIntoTie(iterator from, iterator to, timeT baseDuration)
{
    // so long as we do the quantization checks for validity before
    // calling this method, we should be fine splitting precise times
    // in this method. only problem is deciding not to split something
    // if its duration is very close to requested duration, but that's
    // probably not a task for this function

    timeT eventDuration = (*from)->getDuration();
    timeT baseTime = (*from)->getAbsoluteTime();

    long firstGroupId = -1;
    (*from)->get<Int>(BeamedGroupIdPropertyName, firstGroupId);

    long nextGroupId = -1;
    iterator ni(to);
    if (ni != end() && ++ni != end()) {
	(*ni)->get<Int>(BeamedGroupIdPropertyName, nextGroupId);
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
	    cerr << "WARNING: TrackNotationHelper::expandIntoTie(): (*i)->getAbsoluteTime() != baseTime (" << (*i)->getAbsoluteTime() << " vs " << baseTime << "), ignoring this event\n";
	    continue;
	}

        if ((*i)->getDuration() != eventDuration) {
	    cerr << "WARNING: TrackNotationHelper::expandIntoTie(): (*i)->getDuration() != eventDuration (" << (*i)->getAbsoluteTime() << " vs " << eventDuration << "), changing eventDuration to match\n";
            eventDuration = (*i)->getDuration();
        }

        if (baseDuration >= eventDuration) {
            cerr << "TrackNotationHelper::expandIntoTie() : baseDuration >= eventDuration, ignoring event\n";
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

bool TrackNotationHelper::isViable(timeT duration, int dots)
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


void TrackNotationHelper::makeRestViable(iterator i)
{
    DurationList dl;
    timeT absTime = (*i)->getAbsoluteTime(); 

    TimeSignature timeSig;
    timeT sigTime = track().findTimeSignatureAt(absTime, timeSig);

    timeSig.getDurationListForInterval
	(dl, (*i)->getDuration(), absTime - sigTime);

    cerr << "TrackNotationHelper::makeRestViable: Removing rest of duration "
	 << (*i)->getDuration() << " from time " << absTime << endl;

    erase(i);
    
    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {
	int duration = *i;
	
	cerr << "TrackNotationHelper::makeRestViable: Inserting rest of duration "
	     << duration << " at time " << absTime << endl;

	Event *e = new Event(Note::EventRestType);
	e->setDuration(duration);
	e->setAbsoluteTime(absTime);
	e->setMaybe<String>("Name", "INSERTED_REST"); //!!!

	insert(e);
	absTime += duration;
    }
}


void TrackNotationHelper::makeNoteViable(iterator i)
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
    e->get<Bool>(Note::TiedForwardPropertyName, lastTiedForward);

    e->set<Bool>(Note::TiedForwardPropertyName, true);
    erase(i);

    for (DurationList::iterator i = dl.begin(); i != dl.end(); ++i) {

        DurationList::iterator j(i);
        if (++j == dl.end() && !lastTiedForward) {
            e->unset(Note::TiedForwardPropertyName);
        }

        e->setDuration(*i);
        insert(new Event(*e));
        e->addAbsoluteTime(*i);

        e->set<Bool>(Note::TiedBackwardPropertyName, true);
    }

    delete e;
}


void TrackNotationHelper::insertNote(timeT absoluteTime, Note note, int pitch,
                                     Accidental explicitAccidental)
{

    //!!! Handle grouping!  (Inserting into the middle of an existing
    // group -- take a cue from NotationView::setupGroup)


    //... 

    iterator i, j;
    track().getTimeSlice(absoluteTime, i, j);

    //!!! Deal with end-of-bar issues!

//    int barNo = track().getBarNumber(i);

    insertSomething(i, note.getDuration(), pitch, false, false,
                    explicitAccidental);
}


void TrackNotationHelper::insertRest(timeT absoluteTime, Note note)
{
    iterator i, j;
    track().getTimeSlice(absoluteTime, i, j);

    //!!! Deal with end-of-bar issues!

//    int barNo = track().getBarNumber(i);

    insertSomething(i, note.getDuration(), 0, true, false, NoAccidental);
}


Track::iterator TrackNotationHelper::collapseRestsForInsert(iterator i,
					      timeT desiredDuration)
{
    // collapse at most once, then recurse

    if (i == end() || !(*i)->isa(Note::EventRestType)) return i;

    timeT d = (*i)->getDuration();
    iterator j = track().findContiguousNext(i);
    if (d >= desiredDuration || j == end()) return ++i;

    (*i)->setDuration(d + (*j)->getDuration());
    quantizer().unquantize(*i);
    erase(j);

    return collapseRestsForInsert(i, desiredDuration);
}


void TrackNotationHelper::insertSomething(iterator i, int duration, int pitch,
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
	insertSingleSomething(i, duration, pitch, isRest, tiedBack, acc);
	return;
    }

    // If there's a rest at the insertion position, merge it with any
    // following rests, if available, until we have at least the
    // duration of the new note.
    collapseRestsForInsert(i, duration);

    timeT existingDuration = (*i)->getDuration();

    cerr << "TrackNotationHelper::insertSomething: asked to insert duration " << duration
	 << " over this event:" << endl;
    (*i)->dump(cerr);

    if (duration == existingDuration) {

        // 1. If the new note or rest is the same length as an
        // existing note or rest at that position, chord the existing
        // note or delete the existing rest and insert.

	cerr << "Durations match; doing simple insert" << endl;

	insertSingleSomething(i, duration, pitch, isRest, tiedBack, acc);

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

	insertSingleSomething(i, duration, pitch, isRest, tiedBack,
                              acc);

    } else { // duration > existingDuration

        // 3. If the new note is longer, split the new note so that
        // the first part is the same duration as the existing note or
        // rest, and recurse to step 1 with both the first and the
        // second part in turn.

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
	    //replace (rather than chording with) any events already
	    //present, they don't need to be split in the case where
	    //their duration spans several note-events.  Worry about
	    //that later, I guess.  We're actually getting enough
	    //is-note/is-rest decisions here to make it possibly worth
	    //splitting this method into note and rest versions again

	    cerr << "Need to split new note" << endl;

	    i = insertSingleSomething(i, existingDuration, pitch, isRest,
                                      tiedBack, acc);

	    if (!isRest) (*i)->set<Bool>(Note::TiedForwardPropertyName, true);

            i = track().findTime((*i)->getAbsoluteTime() + existingDuration);

	    insertSomething(i, duration - existingDuration, pitch, isRest,
                            true, acc);

	} else {

	    cerr << "No need to split new note" << endl;

	    i = insertSingleSomething(i, duration, pitch, isRest,
                                      tiedBack, acc);
	}
    }
}

Track::iterator
TrackNotationHelper::insertSingleSomething(iterator i, int duration,
                                           int pitch, bool isRest,
                                           bool tiedBack,
                                           Accidental acc)
{
    timeT time;
    bool eraseI = false;

    if (i == end()) {
	time = track().getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if (isRest || (*i)->isa(Note::EventRestType)) eraseI = true;
    }

    Event *e = new Event(isRest ? Note::EventRestType : Note::EventType);
    e->setAbsoluteTime(time);
    e->setDuration(duration);

    if (!isRest) {
        e->set<Int>("pitch", pitch);
        if (acc != NoAccidental) {
            e->set<String>("accidental",
                           NotationDisplayPitch::getAccidentalName(acc));
        }
        setInsertedNoteGroup(e, i);
    }

    if (tiedBack && !isRest) {
        e->set<Bool>(Note::TiedBackwardPropertyName, true);
    }

    if (eraseI) erase(i);

    return insert(e);
}

void
TrackNotationHelper::setInsertedNoteGroup(Event *e, iterator i)
{
    if (i == begin() || i == end() || !((*i)->isa(Note::EventType))) return;

    iterator j = track().findContiguousPrevious(i);
    if (j == end()) return;

    if ((*i)->has(BeamedGroupIdPropertyName) &&
        (*j)->has(BeamedGroupIdPropertyName) &&
        (*i)->get<Int>(BeamedGroupIdPropertyName) ==
        (*j)->get<Int>(BeamedGroupIdPropertyName)) {

        e->set<Int>(BeamedGroupIdPropertyName,
                    (*i)->get<Int>(BeamedGroupIdPropertyName));
        e->set<String>(BeamedGroupTypePropertyName,
                       (*i)->get<String>(BeamedGroupTypePropertyName));
    }
}


void TrackNotationHelper::insertClef(timeT absoluteTime, Clef clef)
{
    insert(clef.getAsEvent(absoluteTime));
}


void TrackNotationHelper::deleteNote(Event *e)
{
    iterator i = track().findSingle(e);

    if (track().noteIsInChord(e)) {

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

bool TrackNotationHelper::deleteRest(Event *e)
{
    //!!! can we do anything useful with collapseForward? should we return it?

    bool collapseForward;
    return collapse(e, collapseForward);
}

bool TrackNotationHelper::deleteEvent(Event *e)
{
    bool res = true;

    if (e->isa(Note::EventType)) deleteNote(e);
    else if (e->isa(Note::EventRestType)) res = deleteRest(e);
    else {
        // just plain delete
        iterator i = track().findSingle(e);
        erase(i);
    }

    return res;    
}


bool TrackNotationHelper::hasEffectiveDuration(iterator i)
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
TrackNotationHelper::makeBeamedGroup(iterator from, iterator to, string type)
{
    int groupId = track().getNextId();
    
    from = track().findTime((*from)->getAbsoluteTime());
    if (to != end()) to = track().findTime((*to)->getAbsoluteTime());

    for (iterator i = from; i != to; ++i) {
        (*i)->set<Int>(BeamedGroupIdPropertyName, groupId);
        (*i)->set<String>(BeamedGroupTypePropertyName, type);
    }
}



/*

  Auto-beaming code derived from Rosegarden 2.1's ItemListAutoBeam
  and ItemListAutoBeamSub in editor/src/ItemList.c.
  
*/

void TrackNotationHelper::autoBeam(iterator from, iterator to, string type)
{
    // This can only manage whole bars at a time, and it will expand
    // the from-to range out to encompass the whole bars in which they
    // each occur

    for (;;) {

	timeT t = (*from)->getAbsoluteTime();

	iterator barStart = track().findStartOfBar(t);
	iterator barEnd = track().findStartOfNextBar(t);

	TimeSignature timeSig;
	track().findTimeSignatureAt(t, timeSig);

//	cerr << "TrackNotationHelper::autoBeam: t is " << t << "; from time " <<
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

void TrackNotationHelper::autoBeamBar(iterator from, iterator to,
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


void TrackNotationHelper::autoBeamBar(iterator from, iterator to,
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

Clef TrackNotationHelper::guessClef(iterator from, iterator to)
{
    long totalHeight = 0;
    int noteCount = 0;

    // just the defaults:
    Clef clef;
    Key key;

    for (iterator i = from; i != to; ++i) {
        if ((*i)->isa(Note::EventType)) {
            ++noteCount;
            NotationDisplayPitch p((*i)->get<Int>("pitch"), clef, key);
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

}

