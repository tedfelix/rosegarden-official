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


LoopRuler::LoopRuler(QCanvas *canvas,
                     QWidget *parent,
                     const int &bars,
                     const int &barWidth,
                     const int &height,
                     const char *name):
    QCanvasView(canvas, parent, name),
    m_bars(bars), m_barWidth(barWidth), m_height(height),
    m_barSubSections(4), m_canvas(canvas)
{
    setMinimumSize(bars * barWidth, height);
    setMaximumSize(bars * barWidth, height);
    setFrameStyle(NoFrame);

    drawBars();
}

LoopRuler::~LoopRuler()
{
}

void
LoopRuler::drawBars()
{
    QCanvasLine *line;
    int subSectionLinePos;

    for (int i = 0; i < m_bars; i++)
    {
        line = new QCanvasLine(m_canvas);
        line->setPoints(i * m_barWidth, 0, i * m_barWidth, m_height);
        line->setPen(RosegardenGUIColours::LoopRulerForeground);
        line->show();

        for (int j = 0; j < m_barSubSections; j++)
        {
            if (j == 0) continue;
            
            subSectionLinePos = (i * m_barWidth) +
                                (m_barWidth * j/m_barSubSections);

            line = new QCanvasLine(m_canvas);
            line->setPoints(subSectionLinePos, m_height,
                            subSectionLinePos, 3 * m_height / 5);
            line->setPen(RosegardenGUIColours::LoopRulerForeground);
            line->show();

        }

    }

}

void
LoopRuler::contentsMousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        std::cout << "press at " << mE->pos().x() << endl;
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


