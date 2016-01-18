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

#define RG_MODULE_STRING "[SegmentInsertCommand]"

#include "SegmentInsertCommand.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"


#include <QApplication>

namespace Rosegarden
{

SegmentInsertCommand::SegmentInsertCommand(RosegardenDocument *doc,
        TrackId track,
        timeT startTime,
        timeT endTime):
        NamedCommand(tr("Create Segment")),
        m_composition(&(doc->getComposition())),
        m_studio(&(doc->getStudio())),
        m_segment(0),
        m_track(track),
        m_startTime(startTime),
        m_endTime(endTime),
        m_detached(false)
{}

SegmentInsertCommand::SegmentInsertCommand(Composition *composition,
        Segment *segment,
        TrackId track):
        NamedCommand(tr("Create Segment")),
        m_composition(composition),
        m_studio(0),
        m_segment(segment),
        m_track(track),
        m_startTime(0),
        m_endTime(0),
        m_detached(false)
{}

SegmentInsertCommand::~SegmentInsertCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

Segment *
SegmentInsertCommand::getSegment() const
{
    return m_segment;
}

void
SegmentInsertCommand::execute()
{
    if (!m_segment) {
        // Create and insert Segment
        //
        m_segment = new Segment();
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
        m_composition->addSegment(m_segment);
        m_segment->setEndTime(m_endTime);

        // Do our best to label the Segment with whatever is currently
        // showing against it.
        //
        Track *track = m_composition->getTrackById(m_track);
        std::string label;

        if (track) {
            // try to get a reasonable Segment name by Instrument
            //
            label = m_studio->getSegmentName(track->getInstrument());

            // if not use the track label
            //
            if (label == "")
                label = track->getLabel();

            m_segment->setLabel(label);
        }
    } else {
        m_segment->setTrack(m_track);
        m_composition->addSegment(m_segment);
    }

    m_detached = false;
}

void
SegmentInsertCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

}
