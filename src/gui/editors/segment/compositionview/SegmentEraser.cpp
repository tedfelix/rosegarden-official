/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentEraser]"

#include "SegmentEraser.h"

#include "misc/Debug.h"
#include "commands/segment/SegmentEraseCommand.h"
#include "CompositionView.h"
#include "CompositionItem.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentTool.h"
#include "document/Command.h"
#include <QPoint>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{

SegmentEraser::SegmentEraser(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentEraser()\n";
}

void SegmentEraser::ready()
{
    m_canvas->viewport()->setCursor(Qt::PointingHandCursor);
    setBasicContextHelp();
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    setCurrentIndex(m_canvas->getFirstItemAt(e->pos()));
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentIndex) {
        // no need to test the result, we know it's good (see handleMouseButtonPress)
        CompositionItem* item = m_currentIndex;

        addCommandToHistory(new SegmentEraseCommand(item->getSegment()));
    }

    setCurrentIndex(CompositionItemPtr());
}

int SegmentEraser::handleMouseMove(QMouseEvent*)
{
    setBasicContextHelp();
    return RosegardenScrollView::NoFollow;
}

void SegmentEraser::setBasicContextHelp()
{
    setContextHelp(tr("Click on a segment to delete it"));
}    

const QString SegmentEraser::ToolName   = "segmenteraser";

}
#include "SegmentEraser.moc"
