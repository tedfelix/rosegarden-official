/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventUnquantizeCommand.h"

#include "base/Quantizer.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"


namespace Rosegarden
{


EventUnquantizeCommand::EventUnquantizeCommand(Segment &segment,
        timeT startTime,
        timeT endTime,
        std::shared_ptr<Quantizer> quantizer) :
    BasicCommand(tr("Unquantize Events"), segment, startTime, endTime,
                 true),  // bruteForceRedo
    m_selection(nullptr),
    m_quantizer(quantizer)
{
}

EventUnquantizeCommand::EventUnquantizeCommand(
        EventSelection &selection,
        std::shared_ptr<Quantizer> quantizer) :
    BasicCommand(tr("Unquantize Events"),
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime(),
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_quantizer(quantizer)
{
}

EventUnquantizeCommand::~EventUnquantizeCommand()
{
}

void
EventUnquantizeCommand::modifySegment()
{
    Segment &segment = getSegment();

    if (m_selection) {

        m_quantizer->unquantize(m_selection);

    } else {
        m_quantizer->unquantize(&segment,
                                segment.findTime(getStartTime()),
                                segment.findTime(getEndTime()));
    }
}

}
