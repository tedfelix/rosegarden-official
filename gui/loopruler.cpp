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

#include <qpainter.h>

#include "loopruler.h"
#include "colours.h"
#include "Event.h"
#include "rosedebug.h"

LoopRuler::LoopRuler(RosegardenGUIDoc *doc,
                     const int &bars,
                     const int &barWidth,
                     const int &height,
                     QWidget *parent,
                     const char *name):
    QWidget(parent, name),
    m_bars(bars), m_barWidth(barWidth), m_height(height),
    m_doc(doc), m_loop(false), m_startLoop(0)
{
    setMinimumSize(bars * barWidth, height);
    setMaximumSize(bars * barWidth, height);

    // Load in our bar widths from the Composition
    //
    std::pair<Rosegarden::timeT, Rosegarden::timeT> barMarkers;
    for (int i = 0; i < m_bars; i++)
    {
        barMarkers = m_doc->getComposition().getBarRange(i, false);
        m_barWidthMap[i] = barMarkers.second - barMarkers.first;
    }
}

LoopRuler::~LoopRuler()
{
}

void LoopRuler::paintEvent(QPaintEvent*)
{
    QPainter paint(this);
    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);
}

void LoopRuler::drawBarSections(QPainter* paint)
{
    int subSectionLinePos, barWidth;
    int runningWidth = 0;

    paint->setPen(RosegardenGUIColours::LoopRulerForeground);

    for (int i = 0; i < m_bars; i++)
    {
        // Normalise this bar width to a ratio of the first
        //
        barWidth = m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];

        paint->drawLine(runningWidth, 2 * m_height / 7, runningWidth, m_height);

        int beatsBar = m_doc->getComposition().getTimeSignatureAt(runningWidth).getBeatsPerBar();

        for (int j = 0; j < beatsBar; j++)
        {
            if (j == 0) continue;
            
            subSectionLinePos = (runningWidth) + (barWidth * j/beatsBar);

            paint->drawLine(subSectionLinePos, m_height,
                            subSectionLinePos, 5 * m_height / 7);

        }

        runningWidth += barWidth;

    }

}

void
LoopRuler::drawLoopMarker(QPainter* paint)
{
    int x1 = getXPosition(m_startLoop);
    int x2 = getXPosition(m_endLoop);

    if (x1 > x2) 
    {
        x2 = x1;
        x1 = getXPosition(m_endLoop);
    }

    paint->save();
    paint->setBrush(RosegardenGUIColours::LoopHighlight);
    paint->setPen(RosegardenGUIColours::LoopHighlight);
    paint->drawRect(x1, 0, x2 - x1, m_height);
    paint->restore();

}

void
LoopRuler::mousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        if (m_loop)
            m_endLoop = m_startLoop = getPointerPosition(mE->pos().x());
        else
            emit setPointerPosition(getPointerPosition(mE->pos().x()));
    }
}

void
LoopRuler::mouseReleaseEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton)
    {
        if (m_loop)
        {
            // Cancel the loop if there was no drag
            //
            if (m_endLoop == m_startLoop)
            {
                m_endLoop = m_startLoop = 0;
                update();
            }

            // emit with the args around the right way
            //
            if (m_endLoop < m_startLoop)
                emit setLoop(m_endLoop, m_startLoop);
            else
                emit setLoop(m_startLoop, m_endLoop);
        }
    }
}

void
LoopRuler::mouseDoubleClickEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton && !m_loop)
        emit setPlayPosition(getPointerPosition(position));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (m_loop)
    {
        m_endLoop = getPointerPosition(position);
        update();

    }
    else
        emit setPointerPosition(getPointerPosition(position));
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
    Rosegarden::timeT result = 0;
    Rosegarden::timeT runningPosition = 0;
    int barWidth, runningWidth = 0;

    for (int i = 0; i < m_bars; i++)
    {
        barWidth = m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];

        if (runningWidth + barWidth > xPos)
        {
            result = runningPosition + (m_barWidthMap[i] *
                                        (xPos - runningWidth)/barWidth);
            return result;
        }

        runningPosition += m_barWidthMap[i];
        runningWidth += barWidth;
    }

    return result;
}

// Compute an X position on the LoopRuler from a given pos
//
int
LoopRuler::getXPosition(const Rosegarden::timeT &pos)
{
    int result = 0;
    int thisBar = m_doc->getComposition().getBarNumber(pos, false);

    for (int i = 0; i < thisBar; i++)
    {
        result += m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];
    }

    std::pair<Rosegarden::timeT, Rosegarden::timeT> barRange =
                            m_doc->getComposition().getBarRange(thisBar, false);

    result += m_barWidth *
              (pos - barRange.first) / ( barRange.second - barRange.first)
              * m_barWidthMap[thisBar]/m_barWidthMap[0];

    return result;
}
