/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cmath>
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
RulerScale::getFirstVisibleBar() const
{
    return m_composition->getBarNumber(m_composition->getStartMarker());
}

int
RulerScale::getLastVisibleBar() const
{
    return m_composition->getBarNumber(m_composition->getEndMarker());
}

double
RulerScale::getBarWidth(int n) const
{
    return getBarPosition(n + 1) - getBarPosition(n);
}

double
RulerScale::getBeatWidth(int n) const
{
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;
    if (barDuration == 0) return 0;

    bool isNew;
    TimeSignature timeSig = m_composition->getTimeSignatureInBar(n, isNew);

    // cope with partial bars
    double theoreticalWidth =
	(getBarWidth(n) * timeSig.getBarDuration()) / barDuration;

    return theoreticalWidth / timeSig.getBeatsPerBar();
}

int
RulerScale::getBarForX(double x) const
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

    if (minBar > getFirstVisibleBar()) return minBar - 1;
    else return minBar;
}

timeT
RulerScale::getTimeForX(double x) const
{
    int n = getBarForX(x);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);

    if (barWidth < 1.0) {

	return barRange.first;

    } else {

	timeT barDuration = barRange.second - barRange.first;
	x -= getBarPosition(n);

	return barRange.first + (timeT)nearbyint(((double)(x * barDuration) / barWidth));
    }
}

double
RulerScale::getXForTime(timeT time) const
{
    int n = m_composition->getBarNumber(time);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    if (barDuration == 0) {

	return getBarPosition(n);

    } else {

	time -= barRange.first;
	return getBarPosition(n) + (double)(time * barWidth) / barDuration;
    }
}

timeT
RulerScale::getDurationForWidth(double x, double width) const
{
    return getTimeForX(x + width) - getTimeForX(x);
}

double
RulerScale::getWidthForDuration(timeT startTime, timeT duration) const
{
    return getXForTime(startTime + duration) - getXForTime(startTime);
}

double
RulerScale::getTotalWidth() const
{
    int n = getLastVisibleBar();
    // This line adds extra padding to the the actual width -- JAS
    // return getBarPosition(n) + getBarWidth(n);

    // Return only the actual size instead.
    return getBarPosition(n);
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

SimpleRulerScale::SimpleRulerScale(const SimpleRulerScale &ruler):
    RulerScale(ruler.getComposition()),
    m_origin(ruler.getOrigin()),
    m_ratio(ruler.getUnitsPerPixel())
{
    // nothing
}


SimpleRulerScale::~SimpleRulerScale()
{
    // nothing
}

double
SimpleRulerScale::getBarPosition(int n) const
{
    timeT barStart = m_composition->getBarRange(n).first;
    return getXForTime(barStart);
}

double
SimpleRulerScale::getBarWidth(int n) const
{
    std::pair<timeT, timeT> range = m_composition->getBarRange(n);
    return (double)(range.second - range.first) / m_ratio;
}

double
SimpleRulerScale::getBeatWidth(int n) const
{
    bool isNew;
    TimeSignature timeSig(m_composition->getTimeSignatureInBar(n, isNew));
    return (double)(timeSig.getBeatDuration()) / m_ratio;
}

int
SimpleRulerScale::getBarForX(double x) const
{
    return m_composition->getBarNumber(getTimeForX(x));
}

timeT
SimpleRulerScale::getTimeForX(double x) const
{
    timeT t = (timeT)(nearbyint((double)(x - m_origin) * m_ratio));

    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	t += m_composition->getBarRange(firstBar).first;
    }

    return t;
}

double
SimpleRulerScale::getXForTime(timeT time) const
{
    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	time -= m_composition->getBarRange(firstBar).first;
    }

    return m_origin + (double)time / m_ratio;
}


//////////////////////////////////////////////////////////////////////
//                 SegmentsRulerScale
//////////////////////////////////////////////////////////////////////

SegmentsRulerScale::SegmentsRulerScale(Composition *composition,
                                       SegmentSelection segments,
                                       double origin, double ratio) :
    RulerScale(composition),
    m_origin(origin),
    m_ratio(ratio),
    m_segments(segments)
{
    for (SegmentSelection::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        (*i)->addObserver(this);
    }
}

SegmentsRulerScale::~SegmentsRulerScale()
{
    for (SegmentSelection::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        (*i)->removeObserver(this);
    }
}

void
SegmentsRulerScale::segmentDeleted(const Segment *s)
{
    m_segments.erase((Segment *)s);
}

int
SegmentsRulerScale::getFirstVisibleBar() const
{
    timeT earliest = 0;
    bool have = false;
    for (SegmentSelection::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        if (!have || (*i)->getStartTime() < earliest) {
            earliest = (*i)->getStartTime();
            have = true;
        }
    }
    return m_composition->getBarNumber(earliest);
}

int
SegmentsRulerScale::getLastVisibleBar() const
{
    timeT latest = 0;
    bool have = false;
    for (SegmentSelection::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        if (!have || (*i)->getEndMarkerTime() > latest) {
            latest = (*i)->getEndMarkerTime();
            have = true;
        }
    }
    return m_composition->getBarNumber(latest - 1) + 1;
}

double
SegmentsRulerScale::getBarPosition(int n) const
{
    timeT t = m_composition->getBarRange(n).first;

    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	t -= m_composition->getBarRange(firstBar).first;
    }

    return m_origin + (double)t / m_ratio;
}


//////////////////////////////////////////////////////////////////////
//                 ZoomableRulerScale
//////////////////////////////////////////////////////////////////////

ZoomableRulerScale::ZoomableRulerScale(const RulerScale *reference) :
    RulerScale(reference->getComposition()),
    m_reference(reference),
    m_xfactor(1),
    m_yfactor(1)
{
}

ZoomableRulerScale::~ZoomableRulerScale()
{
}

double
ZoomableRulerScale::getBarPosition(int n) const
{
    return m_reference->getBarPosition(n) * m_xfactor;
}

double
ZoomableRulerScale::getBarWidth(int n) const
{
    return m_reference->getBarWidth(n) * m_xfactor;
}

double
ZoomableRulerScale::getBeatWidth(int n) const
{
    return m_reference->getBeatWidth(n) * m_xfactor;
}

int
ZoomableRulerScale::getBarForX(double x) const
{
    return m_reference->getBarForX(x / m_xfactor);
}

timeT
ZoomableRulerScale::getTimeForX(double x) const
{
    return m_reference->getTimeForX(x / m_xfactor);
}

double
ZoomableRulerScale::getXForTime(timeT time) const
{
    return m_reference->getXForTime(time) * m_xfactor;
}

int
ZoomableRulerScale::getFirstVisibleBar() const
{
    return m_reference->getFirstVisibleBar();
}

int
ZoomableRulerScale::getLastVisibleBar() const
{
    return m_reference->getLastVisibleBar();
}

}
