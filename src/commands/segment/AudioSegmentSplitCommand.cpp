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


#include "AudioSegmentSplitCommand.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/RealTime.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

AudioSegmentSplitCommand::AudioSegmentSplitCommand(Segment *segment,
        timeT splitTime) :
        KNamedCommand(i18n("Split Audio Segment")),
        m_segment(segment),
        m_newSegment(0),
        m_splitTime(splitTime),
        m_previousEndMarkerTime(0),
        m_detached(false)
{}

AudioSegmentSplitCommand::~AudioSegmentSplitCommand()
{
    if (m_detached) {
        delete m_newSegment;
    }
    delete m_previousEndMarkerTime;
}

void
AudioSegmentSplitCommand::execute()
{
    if (!m_newSegment) {

        m_newSegment = new Segment(Segment::Audio);

        // Basics
        //
        m_newSegment->setAudioFileId(m_segment->getAudioFileId());
        m_newSegment->setTrack(m_segment->getTrack());

        // Get the RealTime split time
        //
        RealTime splitDiff =
            m_segment->getComposition()->getRealTimeDifference(
                m_segment->getStartTime(), m_splitTime);

        // Set audio start and end
        //
        m_newSegment->setAudioStartTime
        (m_segment->getAudioStartTime() + splitDiff);
        m_newSegment->setAudioEndTime(m_segment->getAudioEndTime());

        // Insert into composition before setting end time
        //
        m_segment->getComposition()->addSegment(m_newSegment);

        // Set start and end times
        //
        m_newSegment->setStartTime(m_splitTime);
        m_newSegment->setEndTime(m_segment->getEndTime());

        // Set original end time
        //
        //        m_previousEndAudioTime = m_segment->getAudioEndTime();
        //        m_segment->setAudioEndTime(m_newSegment->getAudioStartTime());

        RG_DEBUG << "AudioSegmentSplitCommand::execute: Set end audio of left segment to " << m_newSegment->getAudioStartTime() << endl;


        // Set labels
        //
        m_segmentLabel = m_segment->getLabel();
        QString newLabel = strtoqstr(m_segmentLabel);
        if (!newLabel.endsWith(i18n(" (split)"))) {
            newLabel = i18n("%1 (split)").arg(newLabel);
        }
        m_segment->setLabel(qstrtostr(newLabel));
        m_newSegment->setLabel(m_segment->getLabel());

        // Set color
        //
        m_newSegment->setColourIndex(m_segment->getColourIndex());
    }

    // Resize left hand Segment
    //
    const timeT *emt = m_segment->getRawEndMarkerTime();
    if (emt) {
        m_previousEndMarkerTime = new timeT(*emt);
    } else {
        m_previousEndMarkerTime = 0;
    }

    RG_DEBUG << "AudioSegmentSplitCommand::execute: Setting end marker of left segment to " << m_splitTime << endl;

    m_segment->setEndMarkerTime(m_splitTime);

    if (!m_newSegment->getComposition()) {
        m_segment->getComposition()->addSegment(m_newSegment);
    }

    m_detached = false;

}

void
AudioSegmentSplitCommand::unexecute()
{
    if (m_previousEndMarkerTime) {
        RG_DEBUG << "AudioSegmentSplitCommand::unexecute: Restoring end marker of left segment to " << *m_previousEndMarkerTime << endl;

        m_segment->setEndMarkerTime(*m_previousEndMarkerTime);
        delete m_previousEndMarkerTime;
        m_previousEndMarkerTime = 0;
    } else {
        m_segment->clearEndMarker();
    }

    m_segment->setLabel(m_segmentLabel);
    //    RG_DEBUG << "AudioSegmentSplitCommand::unexecute: Setting audio end time of left segment to " << m_previousEndAudioTime << endl;
    //    m_segment->setAudioEndTime(m_previousEndAudioTime);
    m_segment->getComposition()->detachSegment(m_newSegment);
    m_detached = true;
}

}
