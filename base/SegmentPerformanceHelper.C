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

#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include <iostream>

namespace Rosegarden 
{
using std::cerr;
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

    bool tiedBack = false, tiedForward = false;
    e->get<Bool>(TIED_BACKWARD, tiedBack);
    e->get<Bool>(TIED_FORWARD, tiedForward);

    if (tiedBack) return iteratorcontainer();
    else if (!tiedForward) return c;

    timeT t = e->getAbsoluteTime();
    timeT d = e->getDuration();

    if (!e->has(PITCH)) return c;
    int pitch = e->get<Int>(PITCH);

    for (;;) {
        i = segment().findContiguousNext(i);
        if (i == end()) return c;

        e = *i;

        timeT t2 = e->getAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || !e->has(PITCH) ||
                 e->get<Int>(PITCH) != pitch) continue;

        if (!e->get<Bool>(TIED_BACKWARD, tiedBack) ||
            !tiedBack) break;

        d += e->getDuration();
        if (!e->get<Bool>(TIED_FORWARD, tiedForward) ||
            !tiedForward) return c;
    }

    return c;
}


timeT
SegmentPerformanceHelper::getSoundingDuration(iterator i)
{
    if (!(*i)->has(TIED_FORWARD) && !(*i)->has(TIED_BACKWARD)) {
	return (*i)->getDuration();
    }

    iteratorcontainer c(getTiedNotes(i));
    timeT d = 0;

    for (iteratorcontainer::iterator ci = c.begin(); ci != c.end(); ++ci) {
	d += (**ci)->getDuration();
    }

    return d;
}


timeT
SegmentPerformanceHelper::getDurationWithTupling(Event *e)
{
    timeT d = e->getDuration();

/* No, we're now storing the performance duration as the event's
   primary duration.  This is fine for triplets (as our base duration
   is divisible by 3) but may not be suitable for other tuplets where
   our performer's resolution is higher than the Rosegarden base
   resolution.  In such a case, this method could use the tupling data
   and the event's nominal-duration property to calculate a more
   precise duration for the performer than the Rosegarden units will
   permit, but to do so we'd have to know the performer's resolution
   and the point at which this method is currently called (when
   creating a MappedEvent) is not a point at which we know that.

    long tupledLength;
    if (e->get<Int>(BEAMED_GROUP_TUPLED_LENGTH, tupledLength)) {

	long untupledLength;
	if (e->get<Int>(BEAMED_GROUP_UNTUPLED_LENGTH, untupledLength)) {
	    return (d * tupledLength) / untupledLength;
	} else {
	    cerr << "SegmentPerformanceHelper::getDurationWithTupling: WARNING: "
		 << "Found tupled length without untupled length property"
		 << endl;
	}
    }
*/

    return d;
}


//!!! Refine this -- in theory we can do better with tuplets, because
//real time has finer precision than timeT time
RealTime
SegmentPerformanceHelper::getRealAbsoluteTime(iterator i) 
{
    return segment().getComposition()->getElapsedRealTime
	((*i)->getAbsoluteTime());
}


//!!! Refine this -- in theory we can do better with tuplets, because
//real time has finer precision than timeT time.  Perhaps adding
//some abstime->realtime conversion methods that accept double args
//to Composition might be useful
RealTime
SegmentPerformanceHelper::getRealSoundingDuration(iterator i)
{
    return segment().getComposition()->getRealTimeDifference
	((*i)->getAbsoluteTime(),
	 (*i)->getAbsoluteTime() + (*i)->getDuration());
}


}
