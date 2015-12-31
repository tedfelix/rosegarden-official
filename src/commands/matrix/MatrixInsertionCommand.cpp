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


#include "MatrixInsertionCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "document/BasicCommand.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"


namespace Rosegarden
{

using namespace BaseProperties;

MatrixInsertionCommand::MatrixInsertionCommand(Segment &segment,
        timeT time,
        timeT endTime,
        Event *event) :
        BasicCommand(tr("Insert Note"), segment, time, endTime),
        m_event(new Event(*event,
                          std::min(time, endTime),
                          (time < endTime) ? endTime - time : time - endTime))
{
    // nothing
}

MatrixInsertionCommand::~MatrixInsertionCommand()
{
    delete m_event;
    // don't want to delete m_lastInsertedEvent, it's just an alias
}

void MatrixInsertionCommand::modifySegment()
{
    MATRIX_DEBUG << "MatrixInsertionCommand::modifySegment()\n";

    if (!m_event->has(VELOCITY)) {
        m_event->set
        <Int>(VELOCITY, 100);
    }

    SegmentMatrixHelper helper(getSegment());
    m_lastInsertedEvent = new Event(*m_event);
    helper.insertNote(m_lastInsertedEvent);
}

}
