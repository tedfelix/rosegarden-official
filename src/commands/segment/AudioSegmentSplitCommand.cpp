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

#define RG_MODULE_STRING "[AudioSegmentSplitCommand]"

#include "AudioSegmentSplitCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/RealTime.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

AudioSegmentSplitCommand::AudioSegmentSplitCommand(Segment *segment,
        timeT splitTime) :
        NamedCommand(tr("Split Audio Segment")),
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

// ??? Or we could have execute() return a bool indicating success, then
//   deal with failure within CommandHistory::addCommand().  That seems
//   like a big project, however.
bool 
AudioSegmentSplitCommand::isValid()
{
    // Can't split before or at the very start of a segment.
    if (m_splitTime <= m_segment->getStartTime())
        return false;

    // Can't split after or at the very end of a segment.
    if (m_splitTime >= m_segment->getEndMarkerTime())
        return false;

    return true;
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

        RG_DEBUG << "AudioSegmentSplitCommand::execute: Set end audio of left segment to " << m_newSegment->getAudioStartTime();


        // Set labels
        //
        std::string label = m_segment->getLabel();
        m_segment->setLabel(appendLabel(label, qstrtostr(tr("(split)"))));
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

    RG_DEBUG << "AudioSegmentSplitCommand::execute: Setting end marker of left segment to " << m_splitTime;

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
        RG_DEBUG << "AudioSegmentSplitCommand::unexecute: Restoring end marker of left segment to " << *m_previousEndMarkerTime;

        m_segment->setEndMarkerTime(*m_previousEndMarkerTime);
        delete m_previousEndMarkerTime;
        m_previousEndMarkerTime = 0;
    } else {
        m_segment->clearEndMarker();
    }

    //    RG_DEBUG << "AudioSegmentSplitCommand::unexecute: Setting audio end time of left segment to " << m_previousEndAudioTime;
    //    m_segment->setAudioEndTime(m_previousEndAudioTime);
    m_segment->getComposition()->detachSegment(m_newSegment);
    m_detached = true;
}

}
