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

#include "rosestrings.h"
#include "rosegardencanvasview.h"

#include "rosedebug.h"

RosegardenCanvasView::RosegardenCanvasView(QScrollBar* hsb, QCanvas* canvas,
                             QWidget* parent,
                             const char* name, WFlags f)
    : QCanvasView(canvas, parent, name, f),
      m_horizontalScrollBar(hsb),
      m_canvasCurrentWidth(canvas->width())
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
    CanvasItemGC::gc();

    canvas()->update();

    if (canvas()->width() != m_canvasCurrentWidth) {
        
        // Ugly hack needed for Qt3 : otherwise we get dummy min/max
        // values from horizontalScrollBar()
        //
        setHScrollBarMode(Auto);
        updateScrollBars();
        /// end of ugly hack

        m_horizontalScrollBar->setRange(horizontalScrollBar()->minValue(),
                                        horizontalScrollBar()->maxValue());

        m_horizontalScrollBar->setSteps(horizontalScrollBar()->lineStep(),
                                        horizontalScrollBar()->pageStep());

        setHScrollBarMode(AlwaysOff);

        m_canvasCurrentWidth = canvas()->width();
    }
    
}

void CanvasItemGC::mark(QCanvasItem* item)
{
    if (!item) return;

    item->hide();
//     RG_DEBUG << "CanvasItemGC::mark() : "
//                          << item << std::endl;
    m_garbage.push_back(item);
}

void CanvasItemGC::gc()
{
    for(unsigned int i = 0; i < m_garbage.size(); ++i) {
//         RG_DEBUG << "CanvasItemGC::gc() : delete "
//                              << m_garbage[i] << "\n";
        delete m_garbage[i];
    }

    m_garbage.clear();
}

void CanvasItemGC::flush()
{
    m_garbage.clear();
}

std::vector<QCanvasItem*> CanvasItemGC::m_garbage;

