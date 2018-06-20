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

#define RG_MODULE_STRING "[AudioSegmentAutoSplitCommand]"

#include "AudioSegmentAutoSplitCommand.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "sound/AudioFileManager.h"
#include "sound/PeakFileManager.h"
#include <QString>


namespace Rosegarden
{

struct AutoSplitPoint
{
    timeT time;
    timeT lastSoundTime;
    Clef clef;
    Rosegarden::Key key;
    AutoSplitPoint(timeT t, timeT lst, Clef c, Rosegarden::Key k) :
	time(t), lastSoundTime(lst), clef(c), key(k) { }
};

AudioSegmentAutoSplitCommand::AudioSegmentAutoSplitCommand(
    RosegardenDocument *doc,
    Segment *segment,
    int threshold) :
        NamedCommand(getGlobalName()),
        m_segment(segment),
        m_composition(segment->getComposition()),
        m_audioFileManager(&(doc->getAudioFileManager())),
        m_detached(false),
        m_threshold(threshold)
{}

AudioSegmentAutoSplitCommand::~AudioSegmentAutoSplitCommand()
{
    if (m_detached) {
        delete m_segment;
    } else {
        for (size_t i = 0; i < m_newSegments.size(); ++i) {
            delete m_newSegments[i];
        }
    }
}

void
AudioSegmentAutoSplitCommand::execute()
{
    if (m_newSegments.empty()) {

        std::vector<AutoSplitPoint> splitPoints;

        if (m_segment->getType() != Segment::Audio)
            return ;

        // Auto split the audio file - we ask for a minimum
        // result file size of 0.2secs - that's probably fair
        // enough.
        //
        std::vector<SplitPointPair> rtSplitPoints;

        try {
            rtSplitPoints =
                m_audioFileManager->
                getSplitPoints(m_segment->getAudioFileId(),
                               m_segment->getAudioStartTime(),
                               m_segment->getAudioEndTime(),
                               m_threshold,
                               RealTime(0, 200000000));
        } catch (AudioFileManager::BadAudioPathException e) {
            RG_WARNING << "ERROR: AudioSegmentAutoSplitCommand: Bad audio path: " << e.getMessage();
        } catch (PeakFileManager::BadPeakFileException e) {
            RG_WARNING << "ERROR: AudioSegmentAutoSplitCommand: Bad peak file: " << e.getMessage();
        }

        std::vector<SplitPointPair>::iterator it;
        timeT absStartTime, absEndTime;

        int splitCount = 0;

        timeT origStartTime = m_segment->getStartTime();
        RealTime audioStart = m_segment->getAudioStartTime();
        RealTime origStartRT = m_composition->getElapsedRealTime(origStartTime);

        for (it = rtSplitPoints.begin(); it != rtSplitPoints.end(); ++it) {
            // The start time for the segment is the original
            // segment's start time, plus whatever it->first translates
            // into as an offset from the original segment's start
            // time

            RG_DEBUG << "AudioSegmentAutoSplitCommand::execute: range " << it->first << " -> " << it->second;

            absStartTime = m_composition->getElapsedTimeForRealTime
                           (origStartRT - audioStart + it->first);

            absEndTime = m_composition->getElapsedTimeForRealTime
                         (origStartRT - audioStart + it->second);

            //	    absStartTime = m_segment->getStartTime() +
            //		m_composition->getElapsedTimeForRealTime(it->first - audioStart);

            //	    absEndTime = m_segment->getStartTime() +
            //		m_composition->getElapsedTimeForRealTime(it->second - audioStart);

            Segment *newSegment = m_segment->clone(false);

            newSegment->setStartTime(absStartTime);
            newSegment->setAudioFileId(m_segment->getAudioFileId());
            newSegment->setAudioStartTime(it->first);
            newSegment->setAudioEndTime(it->second);
            newSegment->setEndMarkerTime(absEndTime);

            // label
            ++splitCount;
            std::string label = m_segment->getLabel();
            newSegment->setLabel(appendLabel(label, qstrtostr(
                    tr("(part %1)").arg(splitCount))));

            newSegment->setColourIndex(m_segment->getColourIndex());

            RG_DEBUG << "AudioSegmentAutoSplitCommand::execute "
            << "abs start = " << absStartTime
            << ", abs end = " << absEndTime
            << ", seg start = " << newSegment->getStartTime()
            << ", seg end = " << newSegment->getEndMarkerTime()
            << ", audio start = " << newSegment->getAudioStartTime()
            << ", audio end = " << newSegment->getAudioEndTime()
            << endl;

            m_newSegments.push_back(newSegment);
        }
    }

    RG_DEBUG << "AudioSegmentAutoSplitCommand::execute: have " << m_newSegments.size() << " new segments";

    for (size_t i = 0; i < m_newSegments.size(); ++i) {
        m_composition->addSegment(m_newSegments[i]);
    }

    if (!m_newSegments.empty()) {
        m_composition->detachSegment(m_segment);
    }

    m_detached = true;
}

void
AudioSegmentAutoSplitCommand::unexecute()
{
    for (size_t i = 0; i < m_newSegments.size(); ++i) {
        m_composition->detachSegment(m_newSegments[i]);
    }
    if (!m_newSegments.empty()) { // otherwise it was never detached
        m_composition->addSegment(m_segment);
    }
    m_detached = false;
}

}
