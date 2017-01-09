/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SegmentMatrixHelper.h"
#include "base/BaseProperties.h"

namespace Rosegarden
{

Segment::iterator SegmentMatrixHelper::insertNote(Event* e)
{
    Segment::iterator i = segment().insert(e);
    segment().normalizeRests(e->getAbsoluteTime(),
			     e->getAbsoluteTime() + e->getDuration());
    return i;
}

bool
SegmentMatrixHelper::isDrumColliding(Event* e)
{
    long pitch = 0;
    if (!e->get<Int>(BaseProperties::PITCH, pitch))
        return false;

    timeT evTime = e->getAbsoluteTime();

    Segment::iterator it;
    for (it = segment().findTime(evTime); it != end(); ++it) {
        if ((*it) == e) continue;
        if ((*it)->getAbsoluteTime() != evTime) break;
        long p = 0;
        if (!(*it)->get<Int>(BaseProperties::PITCH, p)) continue;
        if (p == pitch) return true;
    }
    return false;
}

}
