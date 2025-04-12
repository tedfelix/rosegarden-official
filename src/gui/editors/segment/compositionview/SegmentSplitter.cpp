/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentSplitter]"

#include "SegmentSplitter.h"

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenScrollView.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/TransportStatus.h"
#include "gui/seqmanager/SequenceManager.h"
#include "SegmentTool.h"
#include "document/Command.h"
#include "document/CommandHistory.h"

#include <QPoint>
#include <QRect>
#include <QSettings>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{


QString SegmentSplitter::ToolName() { return "segmentsplitter"; }

SegmentSplitter::SegmentSplitter(CompositionView *c, RosegardenDocument *d) :
    SegmentTool(c, d),
    m_enableEditingDuringPlayback(true),
    m_prevX(0),
    m_prevY(0)
{
    RG_DEBUG << "SegmentSplitter()\n";
}

SegmentSplitter::~SegmentSplitter()
{}

void SegmentSplitter::ready()
{
    m_canvas->viewport()->setCursor(Qt::SplitHCursor);
    setContextHelp(tr("Click on a segment to split it in two; hold Shift to avoid snapping to beat grid"));
}

void
SegmentSplitter::mousePressEvent(QMouseEvent *e)
{
    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_enableEditingDuringPlayback =
            settings.value("enableEditingDuringPlayback", false).toBool();

    // Can't split a segment while playing.
    // ??? What about recording?
    if (!m_enableEditingDuringPlayback  &&
        RosegardenMainWindow::self()->getSequenceManager()->
            getTransportStatus() == PLAYING)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(pos);

    if (item) {
        // Remove cursor and replace with line on a SegmentItem
        // at where the cut will be made
        m_canvas->viewport()->setCursor(Qt::BlankCursor);

        m_prevX = item->rect().x();
        // ??? Clearly, not used.
        m_prevX = item->rect().y();

        drawSplitLine(e);
    }

}

void
SegmentSplitter::mouseReleaseEvent(QMouseEvent *e)
{
    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // Can't split a segment while playing.
    // ??? What about recording?
    if (!m_enableEditingDuringPlayback  &&
        RosegardenMainWindow::self()->getSequenceManager()->
            getTransportStatus() == PLAYING)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(pos);

    if (item) {
        setSnapTime(e, SnapGrid::SnapToBeat);

        Segment* segment = item->getSegment();

        if (segment->getType() == Segment::Audio) {
            AudioSegmentSplitCommand *command =
                new AudioSegmentSplitCommand(segment, m_canvas->grid().snapX(pos.x()));
            if (command->isValid())
                CommandHistory::getInstance()->addCommand(command);
        } else {
            SegmentSplitCommand *command =
                new SegmentSplitCommand(segment, m_canvas->grid().snapX(pos.x()));
            if (command->isValid())
                CommandHistory::getInstance()->addCommand(command);
        }

// 		m_canvas->updateContents(item->rect());
		m_canvas->update(item->rect());
    }

    // Reinstate the cursor
    m_canvas->viewport()->setCursor(Qt::SplitHCursor);
    m_canvas->hideSplitLine();
}

int
SegmentSplitter::mouseMoveEvent(QMouseEvent *e)
{
    // Can't split a segment while playing.
    // ??? What about recording?
    if (!m_enableEditingDuringPlayback  &&
        RosegardenMainWindow::self()->getSequenceManager()->
            getTransportStatus() == PLAYING)
        return NO_FOLLOW;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // ??? Minor bug: Middle button will cause the split line to
    //     appear.  We should be able to query the QMouseEvent for the
    //     button state and bail if left isn't currently pressed.

    ChangingSegmentPtr item = m_canvas->getModel()->getSegmentAt(pos);

    if (item) {
//        m_canvas->viewport()->setCursor(Qt::blankCursor);
        drawSplitLine(e);
        return FOLLOW_HORIZONTAL;
    } else {
        m_canvas->viewport()->setCursor(Qt::SplitHCursor);
        m_canvas->hideSplitLine();
        return NO_FOLLOW;
    }
}

void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{
    setSnapTime(e, SnapGrid::SnapToBeat);

    QPoint pos = m_canvas->viewportToContents(e->pos());

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(pos.x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(pos.y());

    m_canvas->drawSplitLine(x, y);

    m_prevX = x;
    m_prevY = y;
}

/* unused
void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent*)
{
    // DO NOTHING
}
*/

}
