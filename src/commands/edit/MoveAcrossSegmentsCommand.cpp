/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MoveAcrossSegmentsCommand.h"

#include "base/Clipboard.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "CutCommand.h"
#include "PasteEventsCommand.h"
#include <QString>


namespace Rosegarden
{

MoveAcrossSegmentsCommand::MoveAcrossSegmentsCommand(Segment &,
        Segment &secondSegment,
        timeT newStartTime,
        bool notation,
        EventSelection &selection) :
        MacroCommand(getGlobalName()),
        m_clipboard(new Clipboard())
{
    addCommand(new CutCommand(selection, m_clipboard));

    timeT newEndTime = newStartTime + selection.getEndTime() - selection.getStartTime();
    Segment::iterator i = secondSegment.findTime(newEndTime);
    if (i == secondSegment.end())
        newEndTime = secondSegment.getEndTime();
    else
        newEndTime = (*i)->getAbsoluteTime();

    addCommand(new PasteEventsCommand(secondSegment, m_clipboard,
                                      newStartTime,
                                      newEndTime,
                                      notation ?
                                      PasteEventsCommand::NoteOverlay :
                                      PasteEventsCommand::MatrixOverlay));
}

MoveAcrossSegmentsCommand::~MoveAcrossSegmentsCommand()
{
    delete m_clipboard;
}

QString
MoveAcrossSegmentsCommand::getGlobalName()
{
    return tr("&Move Events to Other Segment");
}

}
