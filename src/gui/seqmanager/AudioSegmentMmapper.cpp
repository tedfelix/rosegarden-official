/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioSegmentMmapper.h"

#include "base/Event.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenGUIDoc.h"
#include "SegmentMmapper.h"
#include "sound/MappedEvent.h"
#include <qstring.h>


namespace Rosegarden
{

AudioSegmentMmapper::AudioSegmentMmapper(RosegardenGUIDoc* doc, Segment* s,
        const QString& fileName)
        : SegmentMmapper(doc, s, fileName)
{}

void AudioSegmentMmapper::dump()
{
    Composition &comp = m_doc->getComposition();

    RealTime eventTime;
    Track* track = comp.getTrackById(m_segment->getTrack());

    // Can't write out if no track
    if (!track) {
        std::cerr << "AudioSegmentMmapper::dump: ERROR: No track for segment!"
        << std::endl;
        return ;
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

    MappedEvent* bufPos = m_mmappedEventBuffer;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        timeT playTime =
            segmentStartTime + repeatNo * segmentDuration;
        if (playTime >= repeatEndTime)
            break;

        playTime = playTime + m_segment->getDelay();
        eventTime = comp.getElapsedRealTime(playTime);
        eventTime = eventTime + m_segment->getRealTimeDelay();

        RealTime audioStart = m_segment->getAudioStartTime();
        RealTime audioDuration = m_segment->getAudioEndTime() - audioStart;
        MappedEvent *mE =
            new (bufPos) MappedEvent(track->getInstrument(),  // send instrument for audio
                                     m_segment->getAudioFileId(),
                                     eventTime,
                                     audioDuration,
                                     audioStart);
        mE->setTrackId(track->getId());
        mE->setRuntimeSegmentId(m_segment->getRuntimeId());

        // Send the autofade if required
        //
        if (m_segment->isAutoFading()) {
            mE->setAutoFade(true);
            mE->setFadeInTime(m_segment->getFadeInTime());
            mE->setFadeOutTime(m_segment->getFadeOutTime());
            std::cout << "AudioSegmentMmapper::dump - "
            << "SETTING AUTOFADE "
            << "in = " << m_segment->getFadeInTime()
            << ", out = " << m_segment->getFadeOutTime()
            << std::endl;
        } else {
            //            std::cout << "AudioSegmentMmapper::dump - "
            //                      << "NO AUTOFADE SET ON SEGMENT" << std::endl;
        }

        ++bufPos;
    }

    *(size_t *)m_mmappedRegion = repeatCount + 1;
}

size_t AudioSegmentMmapper::computeMmappedSize()
{
    if (!m_segment) return 0;

    int repeatCount = getSegmentRepeatCount();

    return (repeatCount + 1) * 1 * sizeof(MappedEvent);
    // audio segments don't have events, we just need room for 1 MappedEvent
}

}

