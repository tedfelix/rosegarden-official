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
	while (++i != end() && !(*i)->isa(Note::EventType));
//!!!        i = segment().findContiguousNext(i);
        if (i == end()) return c;

        e = *i;

        timeT t2 = e->getAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || !e->has(PITCH) ||
                 e->get<Int>(PITCH) != pitch) continue;

        if (!e->get<Bool>(TIED_BACKWARD, tiedBack) ||
            !tiedBack) break;

        d += e->getDuration();
	c.push_back(i);

        if (!e->get<Bool>(TIED_FORWARD, tiedForward) ||
            !tiedForward) return c;
    }

    return c;
}


timeT
SegmentPerformanceHelper::getSoundingAbsoluteTime(iterator i)
{
    return (*i)->getAbsoluteTime();
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
    return segment().getComposition()->getRealTimeDifference
	(getSoundingAbsoluteTime(i),
	 getSoundingAbsoluteTime(i) + getSoundingDuration(i));
}


}
