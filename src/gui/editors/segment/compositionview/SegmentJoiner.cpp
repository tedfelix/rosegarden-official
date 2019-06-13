/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentJoiner]"

#include "SegmentJoiner.h"

#include "misc/Debug.h"
#include "CompositionView.h"
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


QString SegmentJoiner::ToolName() { return "segmentjoiner"; }

SegmentJoiner::SegmentJoiner(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentJoiner() - not implemented";
}

#if 0

void
SegmentJoiner::mousePressEvent(QMouseEvent *e)
{
    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    //QPoint pos = m_canvas->viewportToContents(e->pos());

    // ??? not implemented
}

void
SegmentJoiner::mouseReleaseEvent(QMouseEvent *e)
{
    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    //QPoint pos = m_canvas->viewportToContents(e->pos());

    // ??? not implemented
}

int
SegmentJoiner::mouseMoveEvent(QMouseEvent *e)
{
    // No need to propagate.
    e->accept();

    //QPoint pos = m_canvas->viewportToContents(e->pos());

    // ??? not implemented

    return NO_FOLLOW;
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}

#endif

}
