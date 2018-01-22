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


#include "AudioSegmentDistributeCommand.h"

#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "sound/AudioFile.h"
#include <QString>


namespace Rosegarden
{

AudioSegmentDistributeCommand::AudioSegmentDistributeCommand(
    Composition *comp,
    SegmentSelection &inputSelection,
    Segment *audioSegment):
        NamedCommand(getGlobalName()),
        m_composition(comp),
        m_selection(inputSelection),
        m_audioFile(0),
        m_audioSegment(audioSegment),
        m_executed(false)
{}

AudioSegmentDistributeCommand::AudioSegmentDistributeCommand(
    Composition *comp,
    SegmentSelection &inputSelection,
    AudioFile *audioFile):
        NamedCommand(getGlobalName()),
        m_composition(comp),
        m_selection(inputSelection),
        m_audioFile(audioFile),
        m_audioSegment(0),
        m_executed(false)
{}

AudioSegmentDistributeCommand::~AudioSegmentDistributeCommand()
{
    if (m_executed) {
        for (SegmentSelection::iterator i = m_selection.begin();
                i != m_selection.end(); ++i) {
            delete *i;
        }
    } else {
        for (size_t i = 0; i < m_newSegments.size(); ++i)
            delete m_newSegments[i];
    }
}

void
AudioSegmentDistributeCommand::execute()
{
    // Store the insert times in a local vector
    //
    std::vector<timeT> insertTimes;

    bool addNew = m_newSegments.size() == 0 ? true : false;

    for (SegmentSelection::iterator i = m_selection.begin();
            i != m_selection.end(); ++i) {
        // For MIDI (Internal) Segments only of course
        //
        if ((*i)->getType() == Segment::Internal) {
            if (addNew) {
                for (Segment::iterator it = (*i)->begin();
                        it != (*i)->end(); ++it) {
                    if ((*it)->isa(Note::EventType)) {
                        Segment *segment =
                            new Segment(
                                Segment::Audio,
                                (*it)->getAbsoluteTime());
                        segment->setTrack((*i)->getTrack());

                        // If we've constructed against an AudioFile
                        //
                        if (m_audioFile) {
                            segment->setAudioFileId(m_audioFile->getId());
                            segment->setAudioStartTime(
                                RealTime::zeroTime);
                            segment->setAudioEndTime(
                                m_audioFile->getLength());
                        } else // or an audio Segment
                        {
                            segment->setAudioFileId(
                                m_audioSegment->getAudioFileId());
                            segment->setAudioStartTime(
                                m_audioSegment->getAudioStartTime());
                            segment->setAudioEndTime(
                                m_audioSegment->getAudioEndTime());
                        }

                        m_composition->addSegment(segment);
                        m_newSegments.push_back(segment);
                    }
                }
            }

            // Detach original Segment
            //
            m_composition->detachSegment(*i);
        }

    }

    if (!addNew && !m_newSegments.empty()) {
        // Reattach new segments
        //
        for (size_t i = 0; i < m_newSegments.size(); ++i)
            m_composition->addSegment(m_newSegments[i]);
    }

    m_executed = true;
}

void
AudioSegmentDistributeCommand::unexecute()
{
    for (size_t i = 0; i < m_newSegments.size(); ++i)
        m_composition->detachSegment(m_newSegments[i]);

    for (SegmentSelection::iterator it = m_selection.begin();
            it != m_selection.end(); ++it)
        m_composition->addSegment(*it);

    m_executed = false;
}

}
