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


#include "loopruler.h"
#include "colours.h"
#include <iostream>
#include "Event.h"

LoopRuler::LoopRuler(RosegardenGUIDoc *doc,
                     QCanvas *canvas,
                     QWidget *parent,
                     const int &bars,
                     const int &barWidth,
                     const int &height,
                     const char *name):
    QCanvasView(canvas, parent, name),
    m_bars(bars), m_barWidth(barWidth), m_height(height),
    m_barSubSections(4), m_canvas(canvas), m_doc(doc)
{
    setMinimumSize(bars * barWidth, height);
    setMaximumSize(bars * barWidth, height);
    setFrameStyle(NoFrame);

    drawBarSections();
}

LoopRuler::~LoopRuler()
{
}

void
LoopRuler::drawBarSections()
{
    QCanvasLine *line;
    int subSectionLinePos;

    std::pair<Rosegarden::timeT, Rosegarden::timeT> fTimes =
        m_doc->getComposition().getBarRange(1, false);

    int firstBarWidth = fTimes.second - fTimes.first;
    int runningWidth = 0;

    for (int i = 0; i < m_bars; i++)
    {
        std::pair<Rosegarden::timeT, Rosegarden::timeT> times =
            m_doc->getComposition().getBarRange(i, false);

        // Normalise this bar width to a ratio of the first
        //
        int barWidth = m_barWidth * (times.second - times.first)
                          / firstBarWidth;

        line = new QCanvasLine(m_canvas);
        line->setPoints(runningWidth, 0,
                        runningWidth, m_height);
        line->setPen(RosegardenGUIColours::LoopRulerForeground);
        line->show();

        for (int j = 0; j < m_barSubSections; j++)
        {
            if (j == 0) continue;
            
            subSectionLinePos = (runningWidth) +
                                (barWidth * j/m_barSubSections);

            line = new QCanvasLine(m_canvas);
            line->setPoints(subSectionLinePos, m_height,
                            subSectionLinePos, 3 * m_height / 5);
            line->setPen(RosegardenGUIColours::LoopRulerForeground);
            line->show();

        }

        runningWidth += barWidth;

    }

}

void
LoopRuler::contentsMousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        std::cout << "press at " << mE->pos().x() << endl;
        std::cout << "this is " << getPointerPosition(mE->pos().x()) << endl;
    }
    else if (mE->button() == RightButton)
    {
    }
}

void
LoopRuler::contentsMouseReleaseEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
    }
}

void
LoopRuler::contentsMouseDoubleClickEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
    }
}

// From an x position work out the pointer position.
// Loop through all the bars removing a calculated
// x position every pass through until we're left
// in the bar we require.
//
//
Rosegarden::timeT
LoopRuler::getPointerPosition(const int &xPos)
{
    Rosegarden::timeT position;
    std::pair<Rosegarden::timeT, Rosegarden::timeT> fTimes =
        m_doc->getComposition().getBarRange(1, false);

    int firstBarWidth = fTimes.second - fTimes.first;
    int runningWidth;

    Rosegarden::timeT result = 0;

    for (int i = 0; i < m_bars; i++)
    {
        std::pair<Rosegarden::timeT, Rosegarden::timeT> times =
            m_doc->getComposition().getBarRange(i, false);

        int barWidth = m_barWidth * (times.second - times.first)
                          / firstBarWidth;

        if (runningWidth + barWidth > xPos)
        {
            result = position + ((times.second - times.first) *
                                 (xPos - runningWidth)/barWidth);
            break;
        }

        position += times.second - times.first;
        runningWidth += barWidth;
    }

    return result;
}



