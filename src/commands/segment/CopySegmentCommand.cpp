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

#define RG_MODULE_STRING "[CopySegmentCommand]"
#define RG_NO_DEBUG_PRINT

#include "CopySegmentCommand.h"

#include "misc/Debug.h"
#include "misc/Strings.h"  // qstrtostr()
#include "misc/AppendLabel.h"  // appendLabel()
#include "base/Composition.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/Track.h"

#include <QString>

namespace Rosegarden
{

CopySegmentCommand::CopySegmentCommand(Composition *composition,
                                       Segment* segment,
                                       timeT startTime,
                                       TrackId track,
                                       bool copyAsLink) :
    NamedCommand(getGlobalName()),
    m_composition(composition),
    m_segment(segment),
    m_startTime(startTime),
    m_track(track),
    m_copyAsLink(copyAsLink),
    m_detached(false),
    m_oldEndTime(m_composition->getEndMarker()),
    m_addedSegment(nullptr),
    m_originalSegmentIsLinked(segment->isTrulyLinked())
{
    RG_DEBUG << "ctor" << startTime << track;
}

CopySegmentCommand::~CopySegmentCommand()
{
    RG_DEBUG << "dtor";
    if (m_detached && m_addedSegment) {
        delete m_addedSegment;
    }
}

void
CopySegmentCommand::execute()
{
    RG_DEBUG << "execute";
    if (m_addedSegment) {
        // been here before
        RG_DEBUG << "execute add stored segment";
        m_composition->addSegment(m_addedSegment);
        return ;
    }

    Segment *segment;
    if (m_copyAsLink || m_originalSegmentIsLinked) {
        RG_DEBUG << "execute linking segment" << m_startTime << m_track;
        segment = SegmentLinker::createLinkedSegment(m_segment);
    } else {
        RG_DEBUG << "execute cloning segment" << m_startTime << m_track;
        segment = m_segment->clone(true);
        // make sure it´s not a link
        SegmentLinker* linker = segment->getLinker();
        if (linker) {
            linker->removeLinkedSegment(m_segment);
        }
    }

    const std::string originalLabel = m_segment->getLabel();
    if (m_copyAsLink || m_originalSegmentIsLinked) {
        RG_DEBUG << "execute set label linked";
        segment->setLabel(appendLabel
                          (originalLabel, qstrtostr(tr("(linked)"))));
    } else {
        RG_DEBUG << "execute set label copied";
        segment->setLabel(appendLabel
                          (originalLabel, qstrtostr(tr("(copied)"))));
    }
    segment->setStartTime(m_startTime);
    segment->setTrack(m_track);
    m_composition->addSegment(segment);
    m_addedSegment = segment;

    if (m_composition->autoExpandEnabled()) {
        // If the composition needs expanding, do so...
        timeT endTime = segment->getEndTime();
        if (endTime > m_composition->getEndMarker())
            m_composition->setEndMarker
                (m_composition->getBarEndForTime(endTime));
    }

    m_detached = false;
}

void
CopySegmentCommand::unexecute()
{
    RG_DEBUG << "unexecute";
    m_composition->detachSegment(m_addedSegment);
    m_detached = true;

    // Restore the original composition end in case it was changed
    m_composition->setEndMarker(m_oldEndTime);
}

}
