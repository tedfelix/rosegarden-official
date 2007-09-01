/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixHLayout.h"
#include "MatrixElement.h"
//#include "misc/Debug.h"

#include "base/Composition.h"
#include "scale/LayoutEngine.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Segment.h"
#include "MatrixViewElementManager.h"
#include "MatrixStaff.h"


namespace Rosegarden
{

MatrixHLayout::MatrixHLayout(Composition *c, double timeScaleFactor) :
        HorizontalLayoutEngine(c),
        m_totalWidth(0.0),
        m_firstBar(0)
{}

MatrixHLayout::~MatrixHLayout()
{}

void MatrixHLayout::reset()
{}

void MatrixHLayout::resetStaff(AbstractViewElementManager&, timeT, timeT)
{}

void MatrixHLayout::scanStaff(AbstractViewElementManager &viewElementManager,
                              LinedStaff &staff,
                              timeT startTime, timeT endTime)
{
    Profiler profiler("MatrixHLayout::scanStaff", true);

    // The Matrix layout is not currently designed to be able to lay
    // out more than one staff, because we have no requirement to show
    // more than one at once in the Matrix view.  To make it work for
    // multiple staffs should be straightforward; we just need to bear
    // in mind that they might start and end at different times (hence
    // the total width and bar list can't just be calculated from the
    // last staff scanned as they are now).

    MatrixViewElementManager &matrixViewElementManager = static_cast<MatrixViewElementManager &>(viewElementManager);
    MatrixStaff& matrixStaff = static_cast<MatrixStaff &>(staff);
    
    bool isFullScan = (startTime == endTime);

    MatrixElementList *notes = matrixViewElementManager.getViewElementList();
    MatrixElementList::iterator startItr = notes->begin();
    MatrixElementList::iterator endItr = notes->end();

    if (!isFullScan) {
        startItr = notes->findNearestTime(startTime);
        if (startItr == notes->end())
            startItr = notes->begin();
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

    Segment &segment = matrixViewElementManager.getSegment();
    Composition *composition = segment.getComposition();
    m_firstBar = composition->getBarNumber(segment.getStartTime());
    timeT from = composition->getBarStart(m_firstBar),
                 to = composition->getBarEndForTime(segment.getEndMarkerTime());

    double startPosition = from;

    // 1. Bar lines and time signatures.  We only re-make these on
    // full scans.

    if (isFullScan || m_barData.size() == 0) {

        m_barData.clear();
        int barNo = m_firstBar;

//        MATRIX_DEBUG << "MatrixHLayout::scanStaff() : start time = " << startTime << ", first bar = " << m_firstBar << ", end time = " << endTime << ", end marker time = " << segment.getEndMarkerTime() << ", from = " << from << ", to = " << to << endl;

        // hack for partial bars
        //
        timeT adjTo = to;

        if (composition->getBarStartForTime(segment.getEndMarkerTime())
                != segment.getEndMarkerTime())
            adjTo++;

        while (from < adjTo) {

            bool isNew = false;
            TimeSignature timeSig =
                composition->getTimeSignatureInBar(barNo, isNew);

            if (isNew || barNo == m_firstBar) {
                m_barData.push_back(BarData((from - startPosition) *
                                            matrixStaff.getTimeScaleFactor(),
                                            TimeSigData(true, timeSig)));
            } else {
                m_barData.push_back(BarData((from - startPosition) *
                                            matrixStaff.getTimeScaleFactor(),
                                            TimeSigData(false, timeSig)));
            }

            from = composition->getBarEndForTime(from);
            ++barNo;
        }

        m_barData.push_back(BarData(to * matrixStaff.getTimeScaleFactor(),
                                    TimeSigData(false, TimeSignature())));
    }

    // 2. Elements

    m_totalWidth = 0.0;
    MatrixElementList::iterator i = startItr;

    while (i != endItr) {

        (*i)->setLayoutX(((*i)->getViewAbsoluteTime() - startPosition)
                         * matrixStaff.getTimeScaleFactor());

        double width = (*i)->getViewDuration() * matrixStaff.getTimeScaleFactor();

        // Make sure that very small elements can still be seen
        //
        if (width < 3)
            width = 3;
        else
            width += 1; // fiddle factor

        static_cast<MatrixElement*>((*i))->setWidth((int)width);

        if (isFullScan) {
            m_totalWidth = (*i)->getLayoutX() + width;
        } else {
            m_totalWidth = std::max(m_totalWidth, (*i)->getLayoutX() + width);
        }

        ++i;
    }
}

double MatrixHLayout::getTotalWidth() const
{
    return m_totalWidth;
}

int MatrixHLayout::getFirstVisibleBar() const
{
    return m_firstBar;
}

int MatrixHLayout::getLastVisibleBar() const
{
    int barNo = m_firstBar + m_barData.size() - 2;
    if (barNo < m_firstBar + 1)
        barNo = m_firstBar + 1;

    return barNo;
}

double MatrixHLayout::getBarPosition(int barNo) const
{
    if (barNo < getFirstVisibleBar()) {
        return getBarPosition(getFirstVisibleBar());
    }

    if (barNo > getLastVisibleBar()) {
        return getBarPosition(getLastVisibleBar());
    }

    return m_barData[barNo - m_firstBar].first;
}

bool MatrixHLayout::getTimeSignaturePosition(AbstractViewElementManager &,
        int barNo,
        TimeSignature &timeSig,
        double &timeSigX)
{
    timeSig = m_barData[barNo - m_firstBar].second.second;
    timeSigX = m_barData[barNo - m_firstBar].first;
    return m_barData[barNo - m_firstBar].second.first;
}

void MatrixHLayout::finishLayout(timeT, timeT)
{}

}
