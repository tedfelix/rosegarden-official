
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


	//!!! no, absolutely not -- we should re-quantize
/*
        Note n = Note::getNearestNote(e->getDuration());
        
        e->set<Int>(Note::NoteType, n.getNoteType());
        e->set<Int>(Note::NoteDots, n.getDots());
*/
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



	//!!! no, absolutely not -- we should re-quantize
/*
        Note n = Note::getNearestNote((*previousEvent)->getDuration());
        
        (*previousEvent)->set<Int>(Note::NoteType, n.getNoteType());
        (*previousEvent)->set<Int>(Note::NoteDots, n.getDots());
*/
        quantizer().unquantize(*previousEvent);


        success = true;
        collapseForward = false;

        erase(elPos);
    }
    
    return success;
}

//!!! I don't quite understand the logic here.  we can't for example
//collapse a dotted crotchet rest with a dotted minim rest (total
//duration: two minims and one quaver), but this method thinks we can.
//better to add the quantized durations and see if the closest
//possible note has the same duration as we have

bool TrackNotationHelper::isCollapseValid(timeT a, timeT b)
{
    // experimental:
    return (isViable(a + b));

/*
    timeT durationMax = std::max(a, b);
    timeT durationMin = std::min(a, b);

    return ((durationMax == durationMin) ||
            (durationMax == (2 * durationMin)));
*/
    // TODO : and some test on not fucking up bar count
}


//!!! Not sufficient -- doesn't work for dotted notes (where splitting
//e.g. a dotted crotchet into a crotchet and a quaver is fine) --
//should just check that both durations correspond to decent note
//lengths?  This needs to be relatively permissive, as it's making
//decisions about what notes to allow the user to enter

bool TrackNotationHelper::isExpandValid(timeT a, timeT b)
{
    // experimental:
    return (isViable(a) && isViable(b));

/*
    timeT maxDuration = std::max(a, b);
    timeT minDuration = std::min(a, b);
    return ((maxDuration == (2 * minDuration)) ||
            (maxDuration == (4 * minDuration)) ||
            (maxDuration == (4 * minDuration / 3)));
*/
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
    cerr << "TrackNotationHelper::expandIntoTie(" << baseDuration << ")\n";


    //!!! not getDuration, and where does baseDuration come from --
    //same quantization problem?

    // actually no, this could be okay -- so long as we do the
    // quantization checks for validity before calling this method, we
    // should be fine splitting precise times in this method. only
    // problem is deciding not to split something if its duration is
    // very close to requested duration

    timeT eventDuration = (*from)->getDuration();
    timeT baseTime = (*from)->getAbsoluteTime();

    long firstGroupId = -1;
    (void)(*from)->get<Int>(BeamedGroupIdPropertyName, firstGroupId);

    long nextGroupId = -1;
    iterator ni(to);
    if (ni != end() && ++ni != end()) {
	(void)(*ni)->get<Int>(BeamedGroupIdPropertyName, nextGroupId);
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


bool TrackNotationHelper::isViable(timeT duration, int dots)
{
    cerr << "TrackNotationHelper::isViable: timeT " << duration << ", dots " << dots << endl;
    bool viable;
    duration = quantizer().quantizeByUnit(duration);

    if (dots >= 0) {
        viable = (duration == Quantizer(1, dots).quantizeByNote(duration));
    } else {
        viable = (duration == quantizer().quantizeByNote(duration));
    }

    cerr << "TrackNotationHelper::isViable: returning " << viable << endl;
    return viable;
}


void TrackNotationHelper::makeRestViable(iterator i)
{
    DurationList dl;
    int barNo = track().getBarNumber(i);

    cerr << "TrackNotationHelper::makeRestViable: bar number is " << barNo
         << ", start time is " << track().getBarPositions()[barNo].start
         << ", absTime is " << (*i)->getAbsoluteTime() << endl;

    TimeSignature tsig = track().getBarPositions()[barNo].timeSignature;
    timeT absTime = (*i)->getAbsoluteTime();

    tsig.getDurationListForInterval
	(dl, (*i)->getDuration(), absTime - track().getBarPositions()[barNo].start);

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
    (void)e->get<Bool>(Note::TiedForwardPropertyName, lastTiedForward);

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

    //!!! Put these comments in the right bit of the code!
    
    //!!! Deal with end-of-bar issues!

    //... 

    iterator i, j;
    track().getTimeSlice(absoluteTime, i, j);

    int barNo = track().getBarNumber(i);

//    iterator uncollapsed = collapseRestsForInsert(i, note.getDuration());

    insertSomething(i, note.getDuration(), pitch, false, false,
                    explicitAccidental);
}

// need to deal with groups, kinda like this code?:
/*
void NotationView::setupGroup(NotationElementList::iterator closestNote,
                              NotationElement* newNotationElement)
{
    long groupNo = 0;
    if ((*closestNote)->event()->get<Int>(Track::BeamedGroupIdPropertyName,
                                          groupNo)) {

        newNotationElement->event()->setMaybe<Int>(Track::BeamedGroupIdPropertyName, groupNo);

        newNotationElement->event()->setMaybe<String>(Track::BeamedGroupTypePropertyName,
                                                      (*closestNote)->event()->get<String>
                                                      (Track::BeamedGroupTypePropertyName));
    }
}
*/

void TrackNotationHelper::insertRest(timeT absoluteTime, Note note)
{
    iterator i, j;
    track().getTimeSlice(absoluteTime, i, j);

    int barNo = track().getBarNumber(i);

//    iterator uncollapsed = collapseRestsForInsert(i, note.getDuration());

    insertSomething(i, note.getDuration(), 0, true, false, NoAccidental);
}


void TrackNotationHelper::insertSomething(iterator i, int duration, int pitch,
                                          bool isRest, bool tiedBack,
                                          Accidental acc)
{
    while (i != end() && (*i)->getDuration() == 0) ++i;

    if (i == end()) {
	insertSingleSomething(i, duration, pitch, isRest, tiedBack, acc);
	return;
    }

    collapseRestsForInsert(i, duration);
    timeT existingDuration = (*i)->getDuration();

    cerr << "TrackNotationHelper::insertSomething: asked to insert duration " << duration
	 << " over this event:" << endl;
    (*i)->dump(cerr);

    if (duration == existingDuration) {

	cerr << "Durations match; doing simple insert" << endl;

	insertSingleSomething(i, duration, pitch, isRest, tiedBack, acc);

    } else if (duration < existingDuration) {

	if ((*i)->isa(Note::EventType)) {

            //!!! should be quantized durations

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

            if (last != end() && !isViable(*last, 1)) {
                makeRestViable(last);
            }
	}

	insertSingleSomething(i, duration, pitch, isRest, tiedBack,
                              acc);

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

    if (i == end()) {
	time = track().getDuration();
    } else {
	time = (*i)->getAbsoluteTime();
	if (isRest || (*i)->isa(Note::EventRestType)) erase(i);
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
    }

    if (tiedBack && !isRest) e->set<Bool>(Note::TiedBackwardPropertyName, true);

    return insert(e);
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

    Track::BarPositionList &bpl(track().getBarPositions());

    int fn = track().getBarNumber(from);
    int tn = track().getBarNumber(to);

    if (tn > fn &&
        (to != end() &&
         (*to)->getAbsoluteTime() == track().getBarPositions()[tn].start))
        --tn;

    for (int i = fn; i <= tn; ++i) {

        from = track().findTime(bpl[i].start);

        if (i < (int)bpl.size() - 1) {
            to = track().findTime(bpl[i+1].start);
        } else {
            to = end();
        }

        autoBeamBar(from, to, bpl[i].timeSignature, type);
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




void TrackNotationHelper::autoBeamAuxOld(iterator from, iterator to,
                        timeT average, timeT minimum, timeT maximum,
                        TimeSignature tsig, string type)
{
    //!!! This won't (currently) work right if the time signature
    //changes during the auto-beamed section, and it won't work right
    //if started on an off-beat.  Ideally "from" should be the start
    //of a bar.

    //!!! should reimplement to use existing bar positions, then it'd
    //recover much better from getting out of phase because of bizarre
    //note-lengths and incorrect-duration bars

    timeT accumulator = 0;
    timeT crotchet = Note(Note::Crotchet).getDuration();
    timeT semiquaver = Note(Note::Semiquaver).getDuration();

    for (iterator i = from; i != to; ++i) {

	if ((*i)->getDuration() == 0) continue; // not a note or rest

	// start of new bar -- reset accumulator
	if (accumulator % tsig.getBarDuration() == 0) accumulator = 0;

	if (accumulator % average == 0 &&  // "beamable duration" threshold
	    quantizer().getNoteQuantizedDuration(*i) < crotchet) {

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
		timeT current = quantizer().getNoteQuantizedDuration(*j);

		if ((*i)->isa(Note::EventType)) {
		    if (current < crotchet) ++beamable;
		    if (current >= semiquaver) ++longerThanDemi;
		}

		count += current;

		if (count % tmin == 0) {

		    k = j;
		    beamDuration = count;

		    // increment prospective accumulator.  and then --
		    // will this group have crossed a barline?  if so,
		    // wrap around the prospective accumulator from
		    // the bar mark

		    prospective = (accumulator + count) % tsig.getBarDuration();

		    // found a group; now accept only double this
		    // group's length for a better one
		    tmin *= 2;
		}

		// Stop scanning and make the group if our scan has
		// reached the maximum length of beamed group, we have
		// more than 4 semis or quavers, we're at the end of
		// our run or of a bar, if the next chord is longer
		// than the current one, or if there's a rest ahead.

		iterator jnext(j);

		if ((count > maximum)
		    || (longerThanDemi > 4)
		    || (++jnext == to)     
		    || (prospective == 0 && k != end())
		    || ((*j    )->isa(Note::EventType) &&
			(*jnext)->isa(Note::EventType) &&
			quantizer().getNoteQuantizedDuration(*jnext) > current)
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

			accumulator += quantizer().getNoteQuantizedDuration(*i);
		    }

		    break;
		}
	    }
	} else {

	    accumulator += quantizer().getNoteQuantizedDuration(*i);
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

