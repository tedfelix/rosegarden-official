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

#include "rosegardencanvasview.h"

RosegardenCanvasView::RosegardenCanvasView(QScrollBar* hsb, QCanvas* canvas,
                             QWidget* parent,
                             const char* name, WFlags f)
    : QCanvasView(canvas, parent, name, f),
      m_horizontalScrollBar(hsb)
{
    setHScrollBarMode(AlwaysOff);
}


void RosegardenCanvasView::polish()
{
    QCanvasView::polish();
    slotUpdate();
}


void RosegardenCanvasView::slotUpdate()
{
    canvas()->update();

    m_horizontalScrollBar->setRange(horizontalScrollBar()->minValue(),
                                    horizontalScrollBar()->maxValue());

    m_horizontalScrollBar->setSteps(horizontalScrollBar()->lineStep(),
                                    horizontalScrollBar()->pageStep());
}

