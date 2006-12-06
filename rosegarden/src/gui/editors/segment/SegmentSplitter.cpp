/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "SegmentSplitter.h"

#include "misc/Debug.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "SegmentTool.h"
#include <kcommand.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>
#include <klocale.h>


namespace Rosegarden
{

SegmentSplitter::SegmentSplitter(CompositionView *c, RosegardenGUIDoc *d)
        : SegmentTool(c, d),
        m_prevX(0),
        m_prevY(0)
{
    RG_DEBUG << "SegmentSplitter()\n";
}

SegmentSplitter::~SegmentSplitter()
{}

void SegmentSplitter::ready()
{
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
    setBasicContextHelp();
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        m_canvas->viewport()->setCursor(Qt::blankCursor);
        m_prevX = item->rect().x();
        m_prevX = item->rect().y();
        drawSplitLine(e);
        delete item;
    }

}

void
SegmentSplitter::handleMouseButtonRelease(QMouseEvent *e)
{
    setBasicContextHelp();

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        m_canvas->setSnapGrain(true);
        Segment* segment = CompositionItemHelper::getSegment(item);

        if (segment->getType() == Segment::Audio) {
            AudioSegmentSplitCommand *command =
                new AudioSegmentSplitCommand(segment, m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        } else {
            SegmentSplitCommand *command =
                new SegmentSplitCommand(segment, m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        }

        m_canvas->updateContents(item->rect());
        delete item;
    }

    // Reinstate the cursor
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}

int
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    setBasicContextHelp();

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
//        m_canvas->viewport()->setCursor(Qt::blankCursor);
        drawSplitLine(e);
        delete item;
        return RosegardenCanvasView::FollowHorizontal;
    } else {
        m_canvas->viewport()->setCursor(Qt::splitHCursor);
        m_canvas->slotHideSplitLine();
        return RosegardenCanvasView::NoFollow;
    }
}

void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{
    m_canvas->setSnapGrain(true);

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

    m_canvas->slotShowSplitLine(x, y);

    QRect updateRect(std::max(0, std::min(x, m_prevX) - 5), y,
                     std::max(m_prevX, x) + 5, m_prevY + m_canvas->grid().getYSnap());
    m_canvas->updateContents(updateRect);
    m_prevX = x;
    m_prevY = y;
}

void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent*)
{
    // DO NOTHING
}

void
SegmentSplitter::setBasicContextHelp()
{
    setContextHelp(i18n("Click on a segment to split it in two; hold Shift to avoid snapping to beat grid"));
}

const QString SegmentSplitter::ToolName = "segmentsplitter";

}
#include "SegmentSplitter.moc"
