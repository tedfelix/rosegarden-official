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
#include "rulerscale.h"

LoopRuler::LoopRuler(RosegardenGUIDoc *doc,
                     RulerScale *rulerScale,
                     int height,
                     QWidget *parent,
                     const char *name)
    : QWidget(parent, name),
      m_height(height),
      m_snap(0),
      m_doc(doc),
      m_rulerScale(rulerScale),
      m_loop(false),
      m_startLoop(0), m_endLoop(0)
{
    calculateExtents();
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
    if (!m_doc) return;
    Rosegarden::Composition &comp = m_doc->getComposition();

    int x = 0;

    paint->setPen(RosegardenGUIColours::LoopRulerForeground);

    for (int i = m_firstBar; i <= m_lastBar; i++)
    {
	paint->drawLine(x, 2 * m_height / 7, x, m_height);

	double width = m_rulerScale->getBarWidth(i);
	double beatAccumulator = 0;
	
	for (beatAccumulator  = m_rulerScale->getBeatWidth(i);
	     beatAccumulator < width;
	     beatAccumulator += m_rulerScale->getBeatWidth(i)) {

	    paint->drawLine(x + beatAccumulator, 5 * m_height / 7,
			    x + beatAccumulator, m_height);
	}

	x += (int)width;
    }
	
/*!!!
        // Normalise this bar width to a ratio of the first
        //
        barWidth = m_baseBarWidth * m_barWidthMap[i] / m_barWidthMap[0];

        paint->drawLine(runningWidth, 2 * m_height / 7, runningWidth, m_height);

	Rosegarden::TimeSignature timeSig =
	    comp.getTimeSignatureAt(runningTime);

        int beatsBar = timeSig.getBeatsPerBar();
	Rosegarden::timeT beatDuration = timeSig.getBeatDuration();
	float beatSize = float(m_baseBarWidth * beatDuration) / m_barWidthMap[0];

        for (int j = 0; j < beatsBar; j++)
        {
            if (j == 0) continue;
            
//            subSectionLinePos = (runningWidth) + (barWidth * j/beatsBar);
	    subSectionLinePos = (int)(runningWidth + j * beatSize);
	    if (subSectionLinePos - runningWidth >= barWidth) break;

            paint->drawLine(subSectionLinePos, m_height,
                            subSectionLinePos, 5 * m_height / 7);

        }

        runningWidth += barWidth;
	runningTime += m_barWidthMap[i];
    }
*/
}

void
LoopRuler::drawLoopMarker(QPainter* paint)
{
    int x1 = m_rulerScale->getXForTime(m_startLoop);
    int x2 = m_rulerScale->getXForTime(m_endLoop);

    if (x1 > x2) 
    {
        x2 = x1;
        x1 = m_rulerScale->getXForTime(m_endLoop);
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
            m_endLoop = m_startLoop = m_rulerScale->getTimeForX(mE->pos().x());
        else
            emit setPointerPosition(m_rulerScale->getTimeForX(mE->pos().x()));
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
        emit setPlayPosition(m_rulerScale->getTimeForX(position));
}

void
LoopRuler::mouseMoveEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (m_loop)
    {
        m_endLoop = m_rulerScale->getTimeForX(position);
        update();

    }
    else
        emit setPointerPosition(m_rulerScale->getTimeForX(position));
}


// From an x position work out the pointer position.
// Loop through all the bars removing a calculated
// x position every pass through until we're left
// in the bar we require.
//
//
/*!!!
Rosegarden::timeT
LoopRuler::getPointerPosition(int xPos)
{
    Rosegarden::timeT result = 0;
    Rosegarden::timeT runningPosition = 0;
    int barWidth, runningWidth = 0;

    for (int i = 0; i < m_bars; i++)
    {
        barWidth = m_baseBarWidth * m_barWidthMap[i] / m_barWidthMap[0];

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
LoopRuler::getXPosition(Rosegarden::timeT pos)
{
    int result = 0;
    int thisBar = m_doc->getComposition().getBarNumber(pos, false);

    for (int i = 0; i < thisBar; i++)
    {
        result += m_baseBarWidth * m_barWidthMap[i] / m_barWidthMap[0];
    }

    std::pair<Rosegarden::timeT, Rosegarden::timeT> barRange =
                            m_doc->getComposition().getBarRange(thisBar, false);

    result += m_baseBarWidth *
              (pos - barRange.first) / ( barRange.second - barRange.first)
              * m_barWidthMap[thisBar]/m_barWidthMap[0];

    return result;
}
*/
void
LoopRuler::setLoopMarker(Rosegarden::timeT startLoop, Rosegarden::timeT endLoop)
{
    if (startLoop == endLoop) 
        return;

    m_startLoop = startLoop;
    m_endLoop = endLoop;

    QPainter paint(this);
    paint.setBrush(colorGroup().foreground());
    drawBarSections(&paint);
    drawLoopMarker(&paint);

    update();
}

void
LoopRuler::calculateExtents()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    m_firstBar = comp.getBarNumber(comp.getStartMarker(), false);
    m_lastBar = comp.getBarNumber(comp.getEndMarker(), false);



/*!!!
    // Load in our bar widths from the Composition
    //
    std::pair<Rosegarden::timeT, Rosegarden::timeT> barMarkers;

    Rosegarden::Composition &comp = m_doc->getComposition();
    m_bars = comp.getBarNumber(comp.getEndMarker() -
                                   comp.getStartMarker(), false);
 
    for (int i = 0; i < m_bars; i++)
    {
        barMarkers = m_doc->getComposition().getBarRange(i, false);
        m_barWidthMap.push_back(barMarkers.second - barMarkers.first);
    }
*/
}

