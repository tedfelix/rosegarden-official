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


#include "OpenOrCloseRangeCommand.h"

#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Segment.h"
#include "base/Selection.h"


namespace Rosegarden
{

OpenOrCloseRangeCommand::OpenOrCloseRangeCommand(Composition *composition,
        timeT rangeBegin,
        timeT rangeEnd,
        bool open) :
        NamedCommand(tr("Open or Close Range")),
        m_composition(composition),
        m_beginTime(rangeBegin),
        m_endTime(rangeEnd),
        m_prepared(false),
        m_hasExecuted(false),
        m_opening(open),
        m_loopBegin(0),
        m_loopEnd(0)
{}

OpenOrCloseRangeCommand::~OpenOrCloseRangeCommand()
{
    if (m_prepared) {
        // We own markers that we need to free.  Those are the markers
        // currently not in the composition.
        
        // If we are currently executed, those markers are in "pre",
        // but if we are unexecuted, they are in "post".
        MarkerSelection &unused =
            m_hasExecuted ? m_markersPre : m_markersPost;
            
        for (MarkerSelection::Container::const_iterator i =
                 unused.begin(); i != unused.end(); ++i) {
            delete *i;
        }
    }
}

void
OpenOrCloseRangeCommand::execute()
{
    Profiler profiler("OpenOrCloseRangeCommand::execute()");
    timeT offset = m_endTime - m_beginTime;
    if (!m_opening)
        offset = -offset;

    if (m_opening) {
        if (offset + m_composition->getDuration() >
                m_composition->getEndMarker()) {
            m_composition->setEndMarker(m_composition->getBarEndForTime(
                m_composition->getDuration() + offset));
        }
    }

    if (!m_prepared) {

        timeT movingFrom = m_beginTime;
        if (!m_opening)
            movingFrom = m_endTime;

        for (Composition::iterator i = m_composition->begin();
                i != m_composition->end(); ++i) {

            if ((*i)->getStartTime() >= movingFrom) {
                m_moving.push_back(*i);
            }
        }

        m_timesigsPre = TimeSignatureSelection
                        (*m_composition, movingFrom,
                         m_composition->getEndMarker(),
                         false);

        m_temposPre = TempoSelection
                      (*m_composition, movingFrom,
                       m_composition->getEndMarker(),
                       false);

        m_markersPre = MarkerSelection
            (*m_composition, movingFrom,
             m_composition->getEndMarker());

        for (TimeSignatureSelection::timesigcontainer::const_iterator i =
                    m_timesigsPre.begin(); i != m_timesigsPre.end(); ++i) {

            timeT t = i->first;
            TimeSignature sig = i->second;
            m_timesigsPost.addTimeSignature(t + offset, sig);
        }

        for (TempoSelection::tempocontainer::const_iterator i =
                    m_temposPre.begin(); i != m_temposPre.end(); ++i) {

            timeT t = i->first;
            TempoSelection::tempochange change = i->second;
            m_temposPost.addTempo(t + offset, change.first, change.second);
        }

        for (MarkerSelection::Container::const_iterator i =
                    m_markersPre.begin(); i != m_markersPre.end(); ++i) {

            m_markersPost.addCopyAtOffset(offset, *i);
        }
        
        m_prepared = true;
    }

    // For each segment in the moving list
    for (std::vector<Segment *>::iterator i = m_moving.begin();
            i != m_moving.end(); ++i) {

// RG_DEBUG << "Moving segment on track " << (*i)->getTrack() << " from " << 
//     (*i)->getStartTime() << " to " << ((*i)->getStartTime() + offset) << 
//     " (current end time is " << (*i)->getEndTime() << ", end marker is " << 
//     (*i)->getEndMarkerTime() << ")" << endl;

        // Move the segment
        (*i)->setStartTime((*i)->getStartTime() + offset);
    }


    m_timesigsPre.RemoveFromComposition(m_composition);
    m_timesigsPost.AddToComposition(m_composition);
    m_temposPre.RemoveFromComposition(m_composition);
    m_temposPost.AddToComposition(m_composition);
    m_markersPre.RemoveFromComposition(m_composition);
    m_markersPost.AddToComposition(m_composition);

    // Preserve the loop range for undo
    m_loopBegin = m_composition->getLoopStart();
    m_loopEnd = m_composition->getLoopEnd();

    // If we are opening up a range, try to preserve the loop range.
    if (m_opening) {
        RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

        // If the paste point is prior to the loop range
        if (m_beginTime <= m_loopBegin) {
            // Shift the loop range right.
            doc->setLoop(m_loopBegin + offset, m_loopEnd + offset);
        } else if (m_beginTime < m_loopEnd) {
            // The paste point is within the loop range
            
            // Just shift the end point to expand the loop range
            doc->setLoop(m_loopBegin, m_loopEnd + offset);
        } else {
            // The paste point is after the loop range, so leave it alone.
        }
    }

    // If we are closing the range, the loop range and the range to be removed
    // are the same.  We will leave the loop range alone in case the user wants
    // to Cut Range again (or Delete Range if that is ever added to the UI).
    m_hasExecuted = true;
}

void
OpenOrCloseRangeCommand::unexecute()
{
    timeT offset = m_beginTime - m_endTime;
    if (!m_opening)
        offset = -offset;

    for (std::vector<Segment *>::iterator i = m_moving.begin();
            i != m_moving.end(); ++i) {
        (*i)->setStartTime((*i)->getStartTime() + offset);
    }

    m_timesigsPost.RemoveFromComposition(m_composition);
    m_timesigsPre.AddToComposition(m_composition);
    m_temposPost.RemoveFromComposition(m_composition);
    m_temposPre.AddToComposition(m_composition);
    m_markersPost.RemoveFromComposition(m_composition);
    m_markersPre.AddToComposition(m_composition);
    
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

    // Put back the loop range
    doc->setLoop(m_loopBegin, m_loopEnd);
    m_hasExecuted = false;
}

}
