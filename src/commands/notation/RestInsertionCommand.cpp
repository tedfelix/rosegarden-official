/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RestInsertionCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "NoteInsertionCommand.h"


namespace Rosegarden
{

RestInsertionCommand::RestInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(segment, time, endTime, note, 0,
                         Accidentals::NoAccidental,
                         AutoBeamOff, AutoTieBarlinesOn, MatrixModeOff, GraceModeOff, 0,
                         NoteStyleFactory::DefaultStyle)
{
    setName("Insert Rest");
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertRest(m_insertionTime, m_note);
    if (i != helper.segment().end())
        m_lastInsertedEvent = *i;
    
    if (m_autoTieBarlines) {

        // Note: if m_lastInsertedEvent is null then no note was inserted
        if (m_lastInsertedEvent) {

            // Do the split
            m_lastInsertedEvent = helper.makeThisNoteViable(i);
        }
    }

}

}
