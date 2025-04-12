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


#include "EraseSegmentsStartingInRangeCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"


namespace Rosegarden
{

EraseSegmentsStartingInRangeCommand::EraseSegmentsStartingInRangeCommand(
    Composition *composition,
    timeT begin, timeT end) :
        NamedCommand(tr("Delete Range")),
        m_composition(composition),
        m_beginTime(begin),
        m_endTime(end),
        m_detached(false)
{}

EraseSegmentsStartingInRangeCommand::~EraseSegmentsStartingInRangeCommand()
{
    if (m_detached) {
        for (std::vector<Segment *>::iterator i = m_detaching.begin();
                i != m_detaching.end(); ++i) {
            delete *i;
        }
    }
}

void
EraseSegmentsStartingInRangeCommand::execute()
{
    if (m_detaching.empty()) {

        for (Composition::iterator i = m_composition->begin();
                i != m_composition->end(); ++i) {

            if ((*i)->getStartTime() >= m_beginTime &&
                    (*i)->getStartTime() < m_endTime) {
                m_detaching.push_back(*i);
            }
        }
    }

    for (std::vector<Segment *>::iterator i = m_detaching.begin();
            i != m_detaching.end(); ++i) {
        m_composition->detachSegment(*i);
    }

    m_detached = true;
}

void
EraseSegmentsStartingInRangeCommand::unexecute()
{
    for (std::vector<Segment *>::iterator i = m_detaching.begin();
            i != m_detaching.end(); ++i) {

        m_composition->addSegment(*i);

        //!!! see horrible code in SegmentEraseCommand::unexecute()
        // to restore the audio file ID association in audio file mgr
        // when an audio segment is restored.  Why is this necessary?
        // What is the agency that removed the audio file association
        // in the first place, and why?  Need to investigate that
        // before heedlessly duplicating the same horrors here.

    }

    m_detached = false;
}

}
