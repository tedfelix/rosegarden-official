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

#include "Composition.h"

#include "matrixhlayout.h"
#include "matrixstaff.h"

using Rosegarden::timeT;

MatrixHLayout::MatrixHLayout() :
    m_totalWidth(0.0)
{
}

MatrixHLayout::~MatrixHLayout()
{
}

void MatrixHLayout::reset()
{
    m_barData.clear();
}

void MatrixHLayout::resetStaff(StaffType&)
{
}

void MatrixHLayout::scanStaff(MatrixHLayout::StaffType &staffBase)
{
    // The Matrix layout is not currently designed to be able to lay
    // out more than one staff, because we have no requirement to show
    // more than one at once in the Matrix view.  To make it work for
    // multiple staffs should be straightforward; we just need to bear
    // in mind that they might start and end at different times (hence
    // the total width and bar list can't just be calculated from the
    // last staff scanned as they are now).

    MatrixStaff &staff = dynamic_cast<MatrixStaff &>(staffBase);

    // Do this in two parts: bar lines separately from elements.
    // (We don't need to do all that stuff notationhlayout has to do,
    // scanning the notes bar-by-bar; we can just place the bar lines
    // in the theoretically-correct places and do the same with the
    // notes quite independently.)

    // 1. Bar lines and time signatures

    m_barData.clear();
    Rosegarden::Segment &segment = staff.getSegment();
    Rosegarden::Composition *composition = segment.getComposition();

    timeT from = composition->getBarStart(segment.getStartIndex()),
	    to = composition->getBarEnd  (segment.getEndIndex  ());

    //!!! Deal with time signatures...

    while (from < to) {
	m_barData.push_back(BarData(from * staff.getTimeScaleFactor(), 0));
	from = composition->getBarEnd(from);
    }
    m_barData.push_back(BarData(to * staff.getTimeScaleFactor(), 0));

    // 2. Elements

    m_totalWidth = 0.0;

    MatrixElementList *notes = staff.getViewElementList();
    MatrixElementList::iterator i = notes->begin();

    while (i != notes->end()) {

	(*i)->setLayoutX((*i)->getAbsoluteTime() * staff.getTimeScaleFactor());

	double width = (*i)->getDuration() * staff.getTimeScaleFactor();
	(*i)->setWidth((int)width);
	
	m_totalWidth = (*i)->getLayoutX() + width;
	++i;
    }
}

double MatrixHLayout::getTotalWidth()
{
    return m_totalWidth;
}

unsigned int MatrixHLayout::getBarLineCount(StaffType&)
{
    return m_barData.size();
}

double MatrixHLayout::getBarLineX(StaffType&, unsigned int barNo)
{
    return m_barData[barNo].first;
}

void MatrixHLayout::finishLayout()
{
}
