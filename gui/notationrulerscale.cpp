
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

#include "notationrulerscale.h"

#include "notationhlayout.h"
#include "notationstaff.h"

#include "Composition.h"

using Rosegarden::timeT;
using Rosegarden::Composition;
using Rosegarden::TimeSignature;


NotationRulerScale::NotationRulerScale() :
    m_layout(0),
    m_lastFinishingStaff(0),
    m_firstBar(0)
{
    // nothing else
}

NotationRulerScale::~NotationRulerScale()
{
    // nothing to do
}

void
NotationRulerScale::setLayout(NotationHLayout *layout)
{
    m_layout = layout;
}

void
NotationRulerScale::setFirstStartingStaff(NotationStaff *staff)
{
    timeT t = staff->getSegment().getStartTime();
    m_firstBar = staff->getSegment().getComposition()->getBarNumber(t);
}

void
NotationRulerScale::setLastFinishingStaff(NotationStaff *staff)
{
    m_lastFinishingStaff = staff;
}

double
NotationRulerScale::getBarPosition(int n)
{
    assert(m_layout);
    return m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n));
}

double
NotationRulerScale::getBarWidth(int n)
{
    assert(m_layout);
    return
	m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n+1))
      - m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n));
}

double
NotationRulerScale::getBeatWidth(int n)
{
    std::pair<timeT, timeT> barRange = getComposition()->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    bool isNew;
    TimeSignature timeSig = getComposition()->getTimeSignatureInBar(n, isNew);

    // cope with partial bars
    double theoreticalWidth =
	(getBarWidth(n) * timeSig.getBarDuration()) / barDuration;

    return theoreticalWidth / timeSig.getBeatsPerBar();
}

int
NotationRulerScale::getBarForX(double x)
{
    assert(m_layout);

    // Binary search.  I'd really rather like to use the STL generic
    // algorithm for this because I normally get it wrong when I
    // implement binary search, but the NotationHLayout API is not
    // very helpful.  We could do something like generate a vector of
    // bar line numbers and search using a comparator that looks up
    // those numbers in the layout, but that doesn't seem any simpler
    // and would be significantly slower.

    int minBar = 0, maxBar = m_layout->getBarLineCount(*m_lastFinishingStaff);
    
    while (maxBar > minBar) {
	int middle = minBar + (maxBar - minBar) / 2;
	if (x > m_layout->getBarLineX(*m_lastFinishingStaff, middle)) {
	    minBar = middle + 1;
	} else {
	    maxBar = middle;
	}
    }

    // we've just done equivalent of lower_bound -- we're one bar too
    // far into the list

    return m_firstBar + minBar - 1;
}

timeT
NotationRulerScale::getTimeForX(double x)
{
    int n = getBarForX(x);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = getComposition()->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    x -= getBarPosition(n);

    timeT t = barRange.first + (timeT)((x * barDuration) / barWidth);
    return t;
}

double
NotationRulerScale::getXForTime(timeT time)
{
    int n = getComposition()->getBarNumber(time);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = getComposition()->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    time -= barRange.first;

    double x = getBarPosition(n) + (double)(time * barWidth) / barDuration;
    return x;
}

int
NotationRulerScale::getLayoutBarNumber(int n)
{
    assert(m_layout);
    n -= m_firstBar;

    if (n < 0) return 0;
    if (n >= m_layout->getBarLineCount(*m_lastFinishingStaff))
	return m_layout->getBarLineCount(*m_lastFinishingStaff) - 1;

    return n;
}

Composition *
NotationRulerScale::getComposition()
{
    assert(m_lastFinishingStaff);
    return m_lastFinishingStaff->getSegment().getComposition();
}

