
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

#include "RulerScale.h"
#include "Composition.h"

namespace Rosegarden {


//////////////////////////////////////////////////////////////////////
//                 RulerScale
//////////////////////////////////////////////////////////////////////

RulerScale::RulerScale(Composition *c) :
    m_composition(c)
{ 
    // nothing
}

RulerScale::~RulerScale()
{
    // nothing
}

int
RulerScale::getFirstVisibleBar()
{
    return m_composition->getBarNumber(m_composition->getStartMarker());
}

int
RulerScale::getLastVisibleBar()
{
    return m_composition->getBarNumber(m_composition->getEndMarker());
}

double
RulerScale::getBarWidth(int n)
{
    return getBarPosition(n + 1) - getBarPosition(n);
}

double
RulerScale::getBeatWidth(int n)
{
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    bool isNew;
    TimeSignature timeSig = m_composition->getTimeSignatureInBar(n, isNew);

    // cope with partial bars
    double theoreticalWidth =
	(getBarWidth(n) * timeSig.getBarDuration()) / barDuration;

    return theoreticalWidth / timeSig.getBeatsPerBar();
}

int
RulerScale::getBarForX(double x)
{
    // binary search

    int minBar = getFirstVisibleBar(),
	maxBar = getLastVisibleBar();
    
    while (maxBar > minBar) {
	int middle = minBar + (maxBar - minBar) / 2;
	if (x > getBarPosition(middle)) minBar = middle + 1;
	else maxBar = middle;
    }

    // we've just done equivalent of lower_bound -- we're one bar too
    // far into the list

    return minBar - 1;
}

timeT
RulerScale::getTimeForX(double x)
{
    int n = getBarForX(x);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    x -= getBarPosition(n);

    timeT t = barRange.first + (timeT)((x * barDuration) / barWidth);
    return t;
}

double
RulerScale::getXForTime(timeT time)
{
    int n = m_composition->getBarNumber(time);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    time -= barRange.first;

    double x = getBarPosition(n) + (double)(time * barWidth) / barDuration;
    return x;
}

timeT
RulerScale::getDurationForWidth(double x, double width)
{
    return getTimeForX(x + width) - getTimeForX(x);
}

double
RulerScale::getWidthForDuration(timeT startTime, timeT duration) {
    return getXForTime(startTime + duration) - getXForTime(startTime);
}

double
RulerScale::getTotalWidth()
{
    int n = getLastVisibleBar();
    return getBarPosition(n) + getBarWidth(n);
}




//////////////////////////////////////////////////////////////////////
//                 SimpleRulerScale
//////////////////////////////////////////////////////////////////////


SimpleRulerScale::SimpleRulerScale(Composition *composition,
				   double origin, double ratio) :
    RulerScale(composition),
    m_origin(origin),
    m_ratio(ratio)
{
    // nothing
}

SimpleRulerScale::~SimpleRulerScale()
{
    // nothing
}

double
SimpleRulerScale::getBarPosition(int n)
{
    timeT barStart = m_composition->getBarRange(n).first;
    return getXForTime(barStart);
}

double
SimpleRulerScale::getBarWidth(int n)
{
    std::pair<timeT, timeT> range = m_composition->getBarRange(n);
    return (double)(range.second - range.first) / m_ratio;
}

double
SimpleRulerScale::getBeatWidth(int n)
{
    bool isNew;
    TimeSignature timeSig(m_composition->getTimeSignatureInBar(n, isNew));
    return (double)(timeSig.getBeatDuration()) / m_ratio;
}

int
SimpleRulerScale::getBarForX(double x)
{
    return m_composition->getBarNumber(getTimeForX(x));
}

timeT
SimpleRulerScale::getTimeForX(double x)
{
    timeT t = (timeT)((x - m_origin) * m_ratio);

    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	t += m_composition->getBarRange(firstBar).first;
    }

    return t;
}

double
SimpleRulerScale::getXForTime(timeT time)
{
    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	time -= m_composition->getBarRange(firstBar).first;
    }

    return m_origin + (double)time / m_ratio;
}



//////////////////////////////////////////////////////////////////////
//                 SnapGrid
//////////////////////////////////////////////////////////////////////


const timeT SnapGrid::NoSnap     = -3;
const timeT SnapGrid::SnapToBar  = -2;
const timeT SnapGrid::SnapToBeat = -1;

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
	   snap == SnapToBeat);
    m_snapTime = snap;
}

timeT
SnapGrid::getSnapTime(double x) const
{
    if (m_snapTime == NoSnap) return 0;
    timeT time = m_rulerScale->getTimeForX(x);

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    return snapTime;
}

timeT
SnapGrid::snapX(double x) const
{
    timeT time = m_rulerScale->getTimeForX(x);
    if (m_snapTime == NoSnap) return time;

    Rosegarden::Composition *composition = m_rulerScale->getComposition();
    int barNo = composition->getBarNumber(time);
    std::pair<timeT, timeT> barRange = composition->getBarRange(barNo);

    timeT snapTime = barRange.second - barRange.first;

    if (m_snapTime == SnapToBeat) {
	snapTime = composition->getTimeSignatureAt(time).getBeatDuration();
    } else if (m_snapTime != SnapToBar && m_snapTime < snapTime) {
	snapTime = m_snapTime;
    }

    timeT offset = (time - barRange.first);
    timeT rounded = (offset / snapTime) * snapTime;

    timeT snapped;
    if ((offset - rounded) > (rounded + snapTime - offset)) {
	snapped = rounded + snapTime + barRange.first;
    } else {
	snapped = rounded + barRange.first;
    }

    return snapped;
}

int
SnapGrid::snapY(int y) const
{
    if (m_vstep == 0) return y;
    else return y / m_vstep * m_vstep;
}


}
