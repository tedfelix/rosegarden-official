
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


NotationRulerScale::NotationRulerScale(NotationHLayout *layout) :
    m_layout(layout),
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
NotationRulerScale::setFirstStartingStaff(NotationStaff *staff)
{
    timeT t = staff->getSegment().getStartIndex();
    m_firstBar = staff->getSegment().getComposition()->getBarNumber(t, false);
}

void
NotationRulerScale::setLastFinishingStaff(NotationStaff *staff)
{
    m_lastFinishingStaff = staff;
}

double
NotationRulerScale::getBarPosition(int n)
{
    return m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n));
}

double
NotationRulerScale::getBarWidth(int n)
{
    return
	m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n+1))
      - m_layout->getBarLineX(*m_lastFinishingStaff, getLayoutBarNumber(n));
}

double
NotationRulerScale::getBeatWidth(int n)
{
    //!!! implement
    return 10;
}

int
NotationRulerScale::getBarForX(double x)
{
    //!!! implement
    return 1;
}

timeT
NotationRulerScale::getTimeForX(double x)
{
    //!!! implement
    return 0;
}

double
NotationRulerScale::getXForTime(timeT time)
{
    //!!! implement
    return 0;
}

int
NotationRulerScale::getLayoutBarNumber(int n)
{
    n -= m_firstBar;

    if (n < 0) return 0;
    if (n >= m_layout->getBarLineCount(*m_lastFinishingStaff))
	return m_layout->getBarLineCount(*m_lastFinishingStaff);

    return n;
}

