/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/SegmentPerformanceHelper.h"
#include "base/BaseProperties.h"
#include <iostream>

namespace Rosegarden 
{
using std::endl;
using std::string;

using namespace BaseProperties;

SegmentPerformanceHelper::~SegmentPerformanceHelper() { }


SegmentPerformanceHelper::iteratorcontainer
SegmentPerformanceHelper::getTiedNotes(iterator i)
{
    iteratorcontainer c;
    c.push_back(i);

    Event *e = *i;
    if (!e->isa(Note::EventType)) return c;
    Segment::iterator j(i);

    bool tiedBack = false, tiedForward = false;
    e->get<Bool>(TIED_BACKWARD, tiedBack);
    e->get<Bool>(TIED_FORWARD, tiedForward);

    timeT d = e->getNotationDuration();
    timeT t = e->getNotationAbsoluteTime();

    if (!e->has(PITCH)) return c;
    int pitch = e->get<Int>(PITCH);

    bool valid = false;

    if (tiedBack) {
	// #1171463: If we can find no preceding TIED_FORWARD event,
	// then we remove this property
	
	while (j != begin()) {

	    --j;
	    if (!(*j)->isa(Note::EventType)) continue;
	    e = *j; // can reuse e because this branch always returns

	    timeT t2 = e->getNotationAbsoluteTime() + e->getNotationDuration();
	    if (t2 < t) break;

	    if (t2 > t || !e->has(PITCH) ||
		e->get<Int>(PITCH) != pitch) continue;

	    bool prevTiedForward = false;
	    if (!e->get<Bool>(TIED_FORWARD, prevTiedForward) ||
		!prevTiedForward) break;

	    valid = true;
	    break;
	}

	if (valid) {
	    return iteratorcontainer();
	} else {
	    (*i)->unset(TIED_BACKWARD);
	    return c;
	}
    }
    else if (!tiedForward) return c;

    for (;;) {
	while (++j != end() && !(*j)->isa(Note::EventType)) {
            // explicit braces around empty while statement to quiet warning
        };
        if (j == end()) return c;

        e = *j;

        timeT t2 = e->getNotationAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || !e->has(PITCH) ||
                 e->get<Int>(PITCH) != pitch) continue;

        if (!e->get<Bool>(TIED_BACKWARD, tiedBack) ||
            !tiedBack) break;

        d += e->getNotationDuration();
	c.push_back(j);
	valid = true;

        if (!e->get<Bool>(TIED_FORWARD, tiedForward) ||
            !tiedForward) return c;
    }

    if (!valid) {
	// Related to #1171463: If we can find no following
	// TIED_BACKWARD event, then we remove this property
	(*i)->unset(TIED_FORWARD);
    }

    return c;
}


bool
SegmentPerformanceHelper::getGraceAndHostNotes(iterator i,
					       iteratorcontainer &graceNotes,
					       iteratorcontainer &hostNotes,
					       bool &isHostNote)
{
    if (i == end() || !(*i)->isa(Note::EventType)) return false;

    Segment::iterator j = i;
    Segment::iterator firstGraceNote = i;
    Segment::iterator firstHostNote = i;

    if ((*i)->has(IS_GRACE_NOTE) && (*i)->get<Bool>(IS_GRACE_NOTE)) {

	// i is a grace note.  Find the first host note following it

	j = i;
	while (++j != end()) {
	    if ((*j)->getNotationAbsoluteTime() >
		(*i)->getNotationAbsoluteTime()) break;
	    if ((*j)->getSubOrdering() < 0) continue;
	    if ((*j)->isa(Note::EventType)) {
		firstHostNote = j;
		break;
	    }
	}

	if (firstHostNote == i) {
	    std::cerr << "SegmentPerformanceHelper::getGraceAndHostNotes: REMARK: Grace note at " << (*i)->getAbsoluteTime() << " has no host note" << std::endl;
	    return false;
	}
    } else {

	// i is a host note, but we need to ensure we have the first
	// one, not just any one

	j = i;

	while (j != begin()) {
	    --j;
	    if ((*j)->getNotationAbsoluteTime() <
		(*i)->getNotationAbsoluteTime()) break;
	    if ((*j)->getSubOrdering() < 
		(*i)->getSubOrdering()) break;
	    if ((*j)->isa(Note::EventType)) {
		firstHostNote = j;
		break;
	    }
	}
    }

    // firstHostNote now points to the first host note, which is
    // either the first non-grace note after i (if i was a grace note)
    // or the first note with the same time and subordering as i (if i
    // was not a grace note).

    if ((*firstHostNote)->getSubOrdering() < 0) {
	std::cerr << "SegmentPerformanceHelper::getGraceAndHostNotes: WARNING: Note at " << (*firstHostNote)->getAbsoluteTime() << " has subordering " << (*i)->getSubOrdering() << " but is not a grace note" << std::endl;
	return false;
    }

    j = firstHostNote;

    while (j != begin()) {
	--j;
	if ((*j)->getNotationAbsoluteTime() <
	    (*firstHostNote)->getNotationAbsoluteTime()) break;
	if ((*j)->getSubOrdering() >= 0) continue;
	if (!(*j)->isa(Note::EventType)) continue;
	if (!(*j)->has(IS_GRACE_NOTE) || !(*j)->get<Bool>(IS_GRACE_NOTE)) {
	    std::cerr << "SegmentPerformanceHelper::getGraceAndHostNotes: WARNING: Note at " << (*j)->getAbsoluteTime() << " (in trackback) has subordering " << (*j)->getSubOrdering() << " but is not a grace note" << std::endl;
	    break;
	}
	firstGraceNote = j;
    }
    
    if (firstGraceNote == firstHostNote) {
	std::cerr << "SegmentPerformanceHelper::getGraceAndHostNotes: REMARK: Note at " << (*firstHostNote)->getAbsoluteTime() << " has no grace notes" << std::endl;
	return false;
    }

    j = firstGraceNote;

    // push all of the grace notes, and notes with the same time as
    // the first host note, onto the container

    isHostNote = false;

    while (j != end()) {
	if ((*j)->isa(Note::EventType)) {
	    if ((*j)->getSubOrdering() < 0) {
		if ((*j)->has(IS_GRACE_NOTE) && (*j)->get<Bool>(IS_GRACE_NOTE)) {
		    graceNotes.push_back(j);
		}
	    } else {
		hostNotes.push_back(j);
		if (j == i) isHostNote = true;
	    }
	}
	if ((*j)->getNotationAbsoluteTime() >
	    (*firstHostNote)->getNotationAbsoluteTime()) break;
	++j;
    }

    return true;
}


timeT
SegmentPerformanceHelper::getSoundingAbsoluteTime(iterator i)
{
    timeT t = 0;

    timeT discard;

//    std::cerr << "SegmentPerformanceHelper::getSoundingAbsoluteTime at " << (*i)->getAbsoluteTime() << std::endl;

    if ((*i)->has(IS_GRACE_NOTE)) {
//	std::cerr << "it's a grace note" << std::endl;
	if (getGraceNoteTimeAndDuration(false, i, t, discard)) return t;
    }
    if ((*i)->has(MAY_HAVE_GRACE_NOTES)) {
//	std::cerr << "it's a candidate host note" << std::endl;
	if (getGraceNoteTimeAndDuration(true, i, t, discard)) return t;
    }

    return (*i)->getAbsoluteTime();
}

timeT
SegmentPerformanceHelper::getSoundingDuration(iterator i)
{
    timeT d = 0;

    timeT discard;

//    std::cerr << "SegmentPerformanceHelper::getSoundingDuration at " << (*i)->getAbsoluteTime() << std::endl;

    if ((*i)->has(IS_GRACE_NOTE)) {
//	std::cerr << "it's a grace note" << std::endl;
	if (getGraceNoteTimeAndDuration(false, i, discard, d)) return d;
    }
    if ((*i)->has(MAY_HAVE_GRACE_NOTES)) {
//	std::cerr << "it's a candidate host note" << std::endl;
	if (getGraceNoteTimeAndDuration(true, i, discard, d)) return d;
    }

    if ((*i)->has(TIED_BACKWARD)) {
	
	// Formerly we just returned d in this case, but now we check
	// with getTiedNotes so as to remove any bogus backward ties
	// that have no corresponding forward tie.  Unfortunately this
	// is quite a bit slower.

	//!!! optimize. at least we should add a marker property to
	//anything we've already processed from this helper this time
	//around.

	iteratorcontainer c(getTiedNotes(i));
	
	if (c.empty()) { // the tie back is valid
	    return 0;
	}
    }

    if (!(*i)->has(TIED_FORWARD) || !(*i)->isa(Note::EventType)) {

	d = (*i)->getDuration();

    } else {

	// tied forward but not back

	iteratorcontainer c(getTiedNotes(i));
	    
	for (iteratorcontainer::iterator ci = c.begin();
	     ci != c.end(); ++ci) {
	    d += (**ci)->getDuration();
	}
    }

    return d;
}


// In theory we can do better with tuplets, because real time has
// finer precision than timeT time.  With a timeT resolution of 960ppq
// however the difference is probably not audible

RealTime
SegmentPerformanceHelper::getRealAbsoluteTime(iterator i) 
{
    return segment().getComposition()->getElapsedRealTime
	(getSoundingAbsoluteTime(i));
}


// In theory we can do better with tuplets, because real time has
// finer precision than timeT time.  With a timeT resolution of 960ppq
// however the difference is probably not audible
// 
// (If we did want to do this, it'd help to have abstime->realtime
// conversion methods that accept double args in Composition)

RealTime
SegmentPerformanceHelper::getRealSoundingDuration(iterator i)
{
    timeT t0 = getSoundingAbsoluteTime(i);
    timeT t1 = t0 + getSoundingDuration(i);

    if (t1 > segment().getEndMarkerTime()) {
	t1 = segment().getEndMarkerTime();
    }

    return segment().getComposition()->getRealTimeDifference(t0, t1);
}


bool
SegmentPerformanceHelper::getGraceNoteTimeAndDuration(bool /* host */, iterator i,
						      timeT &t, timeT &d)
{
    // [This code currently assumes appoggiatura.  Acciaccatura later.]

    // For our present purposes, we will assume that grace notes start
    // at the same time as their host note was intended to, and
    // "steal" a proportion of the duration of their host note.  This
    // causes the host note to start later, and be shorter, by that
    // same proportion.

    // If a host note has more than one (consecutive) grace note, they
    // should take a single cut from the grace note and divide it
    // amongst themselves.

    // To begin with we will set the proportion to 1/4, but we will
    // probably want it to be (a) something different [because I don't
    // really know what I'm doing], (b) adaptive [e.g. shorter host
    // note or more grace notes = longer proportion], (c)
    // configurable, or (d) all of the above.

    // Of course we also ought to be taking into account the notated
    // duration of the grace notes -- though in my working examples it
    // generally doesn't seem to be the case that we can always just
    // follow those.  I wonder if we can always use the grace notes'
    // notated duration if the ratio of grace note duration to host
    // note duration is less than some value?  Whatever we do, we
    // should be dividing the grace note duration up in proportion to
    // the durations of the grace notes, in situations where we have
    // more than one grace note consecutively of different durations;
    // that isn't handled at all here.

    if (i == end()) return false;

    iteratorcontainer graceNotes, hostNotes;
    bool isHostNote;

    if (!getGraceAndHostNotes(i, graceNotes, hostNotes, isHostNote)) {
	std::cerr << "SegmentPerformanceHelper::getGraceNoteTimeAndDuration: REMARK: Note at " << (*i)->getAbsoluteTime() << " is not a grace note, or has no grace notes" << std::endl;
	return false;
    }

    if (!isHostNote) {

	if (!(*i)->has(IS_GRACE_NOTE) || !(*i)->get<Bool>(IS_GRACE_NOTE)) {
	    std::cerr << "SegmentPerformanceHelper::getGraceNoteTimeAndDuration: WARNING: Note at " << (*i)->getAbsoluteTime() << " is neither grace nor host note, but was reported as suitable by getGraceAndHostNotes" << std::endl;
	    return false;
	}
    }

    if (hostNotes.empty()) {
	std::cerr << "SegmentPerformanceHelper::getGraceNoteTimeAndDuration: REMARK: Grace note at " << (*i)->getAbsoluteTime() << " has no host note" << std::endl;
	return false;
    }

    if (graceNotes.empty()) {
	std::cerr << "SegmentPerformanceHelper::getGraceNoteTimeAndDuration: REMARK: Note at " << (*i)->getAbsoluteTime() << " has no grace notes" << std::endl;
	return false;
    }

    timeT hostNoteEarliestTime = 0;
    timeT hostNoteShortestDuration = 0;
    timeT hostNoteNotationDuration = 0;

    for (iteratorcontainer::iterator j = hostNotes.begin();
	 j != hostNotes.end(); ++j) {

	if (j == hostNotes.begin() ||
	    (**j)->getAbsoluteTime() < hostNoteEarliestTime) {
	    hostNoteEarliestTime = (**j)->getAbsoluteTime();
	}
	if (j == hostNotes.begin() ||
	    (**j)->getDuration() < hostNoteShortestDuration) {
	    hostNoteShortestDuration = (**j)->getDuration();
	}
	if (j == hostNotes.begin() ||
	    (**j)->getNotationDuration() > hostNoteNotationDuration) {
	    hostNoteNotationDuration = (**j)->getNotationDuration();
	}
	(**j)->set<Bool>(MAY_HAVE_GRACE_NOTES, true);
    }

    timeT graceNoteTime = hostNoteEarliestTime;
    timeT graceNoteDuration = hostNoteNotationDuration / 4;
    if (graceNoteDuration > hostNoteShortestDuration / 2) {
	graceNoteDuration = hostNoteShortestDuration / 2;
    }

    if (isHostNote) {
	t = (*i)->getAbsoluteTime() + graceNoteDuration;
	d = (*i)->getDuration() - graceNoteDuration;
    } else {

	int count = 0, index = 0;
	bool found = false;
	int prevSubOrdering = 0;

	for (iteratorcontainer::iterator j = graceNotes.begin();
	     j != graceNotes.end(); ++j) {

	    bool newChord = false;

	    if ((**j)->getSubOrdering() != prevSubOrdering) {
		newChord = true;
		prevSubOrdering = (**j)->getSubOrdering();
	    }

	    if (newChord) ++count;

	    if (*j == i) found = true;

	    if (!found) {
		if (newChord) ++index;
	    }
	}

	if (index == count) index = 0;
	if (count == 0) count = 1; // should not happen

	d = graceNoteDuration / count;
	t = graceNoteTime + d * index;
    }

    return true;


}


}
