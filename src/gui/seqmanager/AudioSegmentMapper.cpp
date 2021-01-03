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

#include "AudioSegmentMapper.h"

#include "base/Event.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/SegmentMapper.h"
#include "sound/MappedEvent.h"
#include "misc/Debug.h"
#include <QString>


namespace Rosegarden
{

AudioSegmentMapper::AudioSegmentMapper(RosegardenDocument *doc,
                                       Segment *s) :
    SegmentMapper(doc, s)
{
}

void
AudioSegmentMapper::fillBuffer()
{
    Composition &comp = m_doc->getComposition();

    RealTime eventTime;
    Track* track = comp.getTrackById(m_segment->getTrack());

    // Can't write out if no track
    if (!track) {
        RG_WARNING << "AudioSegmentMapper::fillBuffer: ERROR: No track for segment!";
        return;
    }

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    //!!! The repeat count is actually not quite right for audio
    // segments -- it returns one too many for repeating segments,
    // because in midi segments you want that (to deal with partial
    // repeats).  Here we really need to find a better way to deal
    // with partial repeats...

    int repeatCount = getSegmentRepeatCount();
    if (repeatCount > 0)
        repeatEndTime = m_segment->getRepeatEndTime();

    int index = 0;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        timeT playTime = segmentStartTime + repeatNo * segmentDuration;
        if (playTime >= repeatEndTime) break;

        playTime = playTime + m_segment->getDelay();
        eventTime = comp.getElapsedRealTime(playTime);
        eventTime = eventTime + m_segment->getRealTimeDelay();

        RealTime audioStart = m_segment->getAudioStartTime();
        RealTime audioDuration = m_segment->getAudioEndTime() - audioStart;

        MappedEvent e(track->getInstrument(),  // send instrument for audio
                      m_segment->getAudioFileId(),
                      eventTime,
                      audioDuration,
                      audioStart);

        e.setTrackId(track->getId());
        e.setRuntimeSegmentId(m_segment->getRuntimeId());

        // Send the autofade if required
        //
        if (m_segment->isAutoFading()) {
            e.setAutoFade(true);
            e.setFadeInTime(m_segment->getFadeInTime());
            e.setFadeOutTime(m_segment->getFadeOutTime());
            RG_DEBUG << "AudioSegmentMapper::fillBuffer - "
                      << "SETTING AUTOFADE "
                      << "in = " << m_segment->getFadeInTime()
                      << ", out = " << m_segment->getFadeOutTime();
        } else {
            //            RG_DEBUG << "AudioSegmentMapper::fillBuffer - "
            //                      << "NO AUTOFADE SET ON SEGMENT";
        }

        getBuffer()[index] = e;
        ++index;
    }

    resize(index);
    // Instead of calling setStartEnd, we let m_start and m_end remain
    // at their defaults because metaiterator does nothing special for
    // audio segments.
}

int
AudioSegmentMapper::calculateSize()
{
    if (!m_segment) return 0;

    int repeatCount = getSegmentRepeatCount();

    // audio segments don't have events, we just need room for 1 MappedEvent
    return (repeatCount + 1) * 1;
}

bool
AudioSegmentMapper::
shouldPlay(MappedEvent *evt, RealTime sliceStart)
{
    // If it's muted etc it doesn't play.
    if (mutedEtc()) { return false; }

    // Otherwise it should play if it's not already all done sounding.
    // The timeslice logic will have already excluded events that
    // start too late.
    return !evt->EndedBefore(sliceStart);
}

}
