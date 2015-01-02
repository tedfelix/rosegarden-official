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

SegmentJoiner::SegmentJoiner(CompositionView *c, RosegardenDocument *d)
        : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentJoiner() - not implemented\n";
}

SegmentJoiner::~SegmentJoiner()
{}

void
SegmentJoiner::handleMouseButtonPress(QMouseEvent*)
{}

void
SegmentJoiner::handleMouseButtonRelease(QMouseEvent*)
{}

int
SegmentJoiner::handleMouseMove(QMouseEvent*)
{
    return RosegardenScrollView::NoFollow;
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{}

const QString SegmentJoiner::ToolName   = "segmentjoiner";

}
#include "SegmentJoiner.moc"
