// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "SnapGrid.h"
#include "Composition.h"

namespace Rosegarden {


//////////////////////////////////////////////////////////////////////
// SnapGrid
//////////////////////////////////////////////////////////////////////


const timeT SnapGrid::NoSnap     = -1;
const timeT SnapGrid::SnapToBar  = -2;
const timeT SnapGrid::SnapToBeat = -3;
const timeT SnapGrid::SnapToUnit = -4;

SnapGrid::SnapGrid(RulerScale *rulerScale, int vstep) :
    m_rulerScale(rulerScale),
    m_snapTime(SnapToBeat),
    m_vstep(vstep)
{
    // nothing else 
}

void
SnapGrid::setSnapTime(timeT snap)
{
    assert(snap > 0 ||
	   snap == NoSnap ||
	   snap == SnapToBar ||
	   snap == SnapToBeat ||
	   snap == SnapToUnit);
    m_snapTime = snap;
}

timeT
SnapGrid::getSnapTime(double x) const
{
    timeT time = m_rulerScale->getTimeForX(x);
    return getSnapTime(time);
}

timeT
SnapGrid::getSnapTime(timeT time) const
{
    if (m_snapTime == NoSnap) return 0;

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime == SnapToUnit) {
	snapTime = composition->getTimeSignatureAt(time).getUnitDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    return snapTime;
}

timeT
SnapGrid::snapX(double x, SnapDirection direction) const
{
    return snapTime(m_rulerScale->getTimeForX(x), direction);
}

timeT
SnapGrid::snapTime(timeT time, SnapDirection direction) const
{
    if (m_snapTime == NoSnap) return time;

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime == SnapToUnit) {
	snapTime = composition->getTimeSignatureAt(time).getUnitDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    timeT offset = (time - barRange.first);
    timeT rounded = (offset / snapTime) * snapTime;

    timeT left  = rounded + barRange.first;
    timeT right = left + snapTime;

    if (direction == SnapLeft) return left;
    else if (direction == SnapRight) return right;
    else if ((offset - rounded) > (rounded + snapTime - offset)) {
	return right;
    } else {
	return left;
    }
}

}
