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

#include "Composition.h"
#include "Profiler.h"

#include "matrixhlayout.h"
#include "matrixstaff.h"

using Rosegarden::timeT;

MatrixHLayout::MatrixHLayout(Rosegarden::Composition *c) :
    Rosegarden::HorizontalLayoutEngine<MatrixElement>(c),
    m_totalWidth(0.0),
    m_firstBar(0)
{
}

MatrixHLayout::~MatrixHLayout()
{
}

void MatrixHLayout::reset()
{
}

void MatrixHLayout::resetStaff(StaffType&, timeT, timeT)
{
}

void MatrixHLayout::scanStaff(MatrixHLayout::StaffType &staffBase,
			      timeT startTime, timeT endTime)
{
    Rosegarden::Profiler profiler("MatrixHLayout::scanStaff", true);

    // The Matrix layout is not currently designed to be able to lay
    // out more than one staff, because we have no requirement to show
    // more than one at once in the Matrix view.  To make it work for
    // multiple staffs should be straightforward; we just need to bear
    // in mind that they might start and end at different times (hence
    // the total width and bar list can't just be calculated from the
    // last staff scanned as they are now).

    MatrixStaff &staff = dynamic_cast<MatrixStaff &>(staffBase);
    bool isFullScan = (startTime == endTime);

    MatrixElementList *notes = staff.getViewElementList();
    MatrixElementList::iterator startItr = notes->begin();
    MatrixElementList::iterator endItr = notes->end();

    if (!isFullScan) {
	startItr = notes->findNearestTime(startTime);
	if (startItr == notes->end()) startItr = notes->begin();
	endItr = notes->findTime(endTime);
    }

    if (endItr == notes->end() && startItr == notes->begin()) {
	isFullScan = true;
    }

    // Do this in two parts: bar lines separately from elements.
    // (We don't need to do all that stuff notationhlayout has to do,
    // scanning the notes bar-by-bar; we can just place the bar lines
    // in the theoretically-correct places and do the same with the
    // notes quite independently.)
	
    Rosegarden::Segment &segment = staff.getSegment();
    Rosegarden::Composition *composition = segment.getComposition();
    m_firstBar = composition->getBarNumber(segment.getStartTime());
    timeT from = composition->getBarStart(m_firstBar),
	    to = composition->getBarEndForTime(segment.getEndMarkerTime());

    double startPosition = from;

    // 1. Bar lines and time signatures.  We only re-make these on
    // full scans.

    if (isFullScan || m_barData.size() == 0) {
    
	for (BarDataList::iterator i = m_barData.begin();
	     i != m_barData.end(); ++i) delete i->second;

	m_barData.clear();
	int barNo = m_firstBar;
	
	MATRIX_DEBUG << "MatrixHLayout::scanStaff() : start time = " << startTime << ", first bar = " << m_firstBar << ", end time = " << endTime << ", end marker time = "  << segment.getEndMarkerTime() << ", from = " << from << ", to = " << to << endl;
	
        // hack for partial bars
        //
        timeT adjTo = to;

        if (composition->getBarStartForTime(segment.getEndMarkerTime())
            != segment.getEndMarkerTime())
            adjTo++;

	while (from < adjTo) {
	    
	    bool isNew = false;
	    Rosegarden::TimeSignature timeSig =
		composition->getTimeSignatureInBar(barNo, isNew);
	    
	    if (isNew || barNo == m_firstBar) {
		m_barData.push_back(BarData((from - startPosition) *
					    staff.getTimeScaleFactor(),
					    timeSig.getAsEvent(from)));
	    } else {
		m_barData.push_back(BarData((from - startPosition) *
					    staff.getTimeScaleFactor(),
					    0));
	    }

	    from = composition->getBarEndForTime(from);
	    ++barNo;
	}

	m_barData.push_back(BarData(to * staff.getTimeScaleFactor(), 0));
    }

    // 2. Elements

    m_totalWidth = 0.0;
    MatrixElementList::iterator i = startItr;

    while (i != endItr) {

	(*i)->setLayoutX(((*i)->getViewAbsoluteTime() - startPosition)
                          * staff.getTimeScaleFactor());

	double width = (*i)->getViewDuration() * staff.getTimeScaleFactor();
	(*i)->setWidth((int)width + 2); // fiddle factor
	
	if (isFullScan) {
	    m_totalWidth = (*i)->getLayoutX() + width;
	} else {
	    m_totalWidth = std::max(m_totalWidth, (*i)->getLayoutX() + width);
	}
	    
	++i;
    }
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

int MatrixHLayout::getFirstVisibleBar()
{
    return m_firstBar;
}

int MatrixHLayout::getLastVisibleBar()
{
    int barNo = m_firstBar + m_barData.size() - 2;
    if (barNo < m_firstBar + 1) barNo = m_firstBar + 1;

    return barNo;
}

double MatrixHLayout::getBarPosition(int barNo)
{
    if (barNo < getFirstVisibleBar()) {
        return getBarPosition(getFirstVisibleBar());
    }

    if (barNo > getLastVisibleBar()) {
        return getBarPosition(getLastVisibleBar());
    }
    return m_barData[barNo - m_firstBar].first;
}

Rosegarden::Event *MatrixHLayout::getTimeSignaturePosition(StaffType &,
							   int barNo,
							   double &timeSigX)
{
    timeSigX = m_barData[barNo - m_firstBar].first;
    return m_barData[barNo - m_firstBar].second;
}

void MatrixHLayout::finishLayout(timeT, timeT)
{
}
