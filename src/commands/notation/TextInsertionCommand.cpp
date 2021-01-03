/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TextInsertionCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "base/Selection.h"


namespace Rosegarden
{

TextInsertionCommand::TextInsertionCommand(Segment &segment, timeT time,
        Text text) :
        BasicCommand(tr("Insert Text"), segment, time, time + 1),
        m_text(text),
        m_lastInsertedEvent(nullptr)
{
    // nothing
}

TextInsertionCommand::~TextInsertionCommand()
{
    // nothing
}

EventSelection *
TextInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

void
TextInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertText(getStartTime(), m_text);
    if (i != helper.segment().end())
        m_lastInsertedEvent = *i;
}

}
