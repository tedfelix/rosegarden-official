/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentMapper]"

#include "SegmentMapper.h"

#include "InternalSegmentMapper.h"
#include "AudioSegmentMapper.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/BaseProperties.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "sound/ControlBlock.h"

namespace Rosegarden
{

SegmentMapper::SegmentMapper(RosegardenDocument *doc,
                             Segment *segment) :
    MappedEventBuffer(doc),
    m_segment(segment)
{
    SEQMAN_DEBUG << "SegmentMapper : " << this;
}


void
SegmentMapper::
initSpecial(void)
{
    if (m_segment != nullptr) {
        SEQMAN_DEBUG << "SegmentMapper::initSpecial " 
                     << " for segment " << m_segment->getLabel() << endl;
    }    
};


SegmentMapper::~SegmentMapper()
{
    SEQMAN_DEBUG << "~SegmentMapper : " << this;
}

QSharedPointer<SegmentMapper>
SegmentMapper::makeMapperForSegment(RosegardenDocument *doc,
                                    Segment *segment)
{
    QSharedPointer<SegmentMapper> mapper;

    if (segment == nullptr) {
        RG_DEBUG << "makeMapperForSegment() segment == 0";
        return QSharedPointer<SegmentMapper>();
    }

    switch (segment->getType()) {
    case Segment::Internal :
        mapper = QSharedPointer<SegmentMapper>(new InternalSegmentMapper(doc, segment));
        break;
    case Segment::Audio :
        mapper = QSharedPointer<SegmentMapper>(new AudioSegmentMapper(doc, segment));
        break;
    default:
        RG_DEBUG << "makeMapperForSegment(" << segment << ") : can't map, unknown segment type " << segment->getType();
        mapper = QSharedPointer<SegmentMapper>();
    }

    // ??? InternalSegmentMapper and AudioSegmentMapper's ctors should
    //     call init().
    if (mapper)
        mapper->init();

    return mapper;
}

int
SegmentMapper::getSegmentRepeatCount()
{
    int repeatCount = 0;

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    if (m_segment->isRepeating() && segmentDuration > 0) {
        repeatEndTime = m_segment->getRepeatEndTime();
        repeatCount = 1 + (repeatEndTime - segmentEndTime) / segmentDuration;
    }

    return repeatCount;
}

TrackId
SegmentMapper::getTrackID() const
{
    if (m_segment)
        return m_segment->getTrack();

    return UINT_MAX;
}

bool
SegmentMapper::
mutedEtc(void)
{
    const ControlBlock *controlBlock = ControlBlock::getInstance();
    TrackId trackId = m_segment->getTrack();

    // Archived overrides everything.  Check it first.
    if (controlBlock->isTrackArchived(trackId))
        return true;

    // If we are in solo mode, mute based on whether our track
    // is being soloed.
    if (controlBlock->isAnyTrackInSolo())
        return !controlBlock->isSolo(trackId);

    // Otherwise use the normal muting/archiving logic.
    return controlBlock->isTrackMuted(trackId);
}

}
