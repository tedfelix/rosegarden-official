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


#include "SegmentEraser.h"

#include "misc/Debug.h"
#include "commands/segment/SegmentEraseCommand.h"
#include "CompositionView.h"
#include "CompositionItemImpl.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "SegmentTool.h"
#include <kcommand.h>
#include <qpoint.h>
#include <qstring.h>
#include <klocale.h>


namespace Rosegarden
{

SegmentEraser::SegmentEraser(CompositionView *c, RosegardenGUIDoc *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentEraser()\n";
}

void SegmentEraser::ready()
{
    m_canvas->viewport()->setCursor(Qt::pointingHandCursor);
    setBasicContextHelp();
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    setCurrentItem(m_canvas->getFirstItemAt(e->pos()));
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem) {
        // no need to test the result, we know it's good (see handleMouseButtonPress)
        CompositionItemImpl* item = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)m_currentItem);

        addCommandToHistory(new SegmentEraseCommand(item->getSegment()));
    }

    setCurrentItem(CompositionItem());
}

int SegmentEraser::handleMouseMove(QMouseEvent*)
{
    setBasicContextHelp();
    return RosegardenCanvasView::NoFollow;
}

void SegmentEraser::setBasicContextHelp()
{
    setContextHelp(i18n("Click on a segment to delete it"));
}    

const QString SegmentEraser::ToolName   = "segmenteraser";

}
#include "SegmentEraser.moc"
