// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "audiocommands.h"
#include "rosedebug.h"

#include "AudioFile.h"
#include "Composition.h"
#include "Selection.h"

DistributeAudioCommand::DistributeAudioCommand(
        Rosegarden::Composition *comp,
        Rosegarden::SegmentSelection &inputSelection,
        Rosegarden::Segment *audioSegment):
    KNamedCommand(getGlobalName()),
    m_composition(comp),
    m_selection(inputSelection),
    m_audioFile(0),
    m_audioSegment(audioSegment)
{
}


DistributeAudioCommand::DistributeAudioCommand(
        Rosegarden::Composition *comp,
        Rosegarden::SegmentSelection &inputSelection,
        Rosegarden::AudioFile *audioFile):
    KNamedCommand(getGlobalName()),
    m_composition(comp),
    m_selection(inputSelection),
    m_audioFile(audioFile),
    m_audioSegment(0)
{
}


void 
DistributeAudioCommand::execute()
{
    // Store the insert times in a local vector
    //
    std::vector<Rosegarden::timeT> insertTimes;

    bool addNew = m_newSegments.size() == 0 ? true : false;

    for (Rosegarden::SegmentSelection::iterator i = m_selection.begin();
         i != m_selection.end(); ++i)
    {
        // For MIDI (Internal) Segments only of course
        //
        if ((*i)->getType() == Rosegarden::Segment::Internal)
        {
            if (addNew)
            {
                for (Rosegarden::Segment::iterator it = (*i)->begin(); 
                     it != (*i)->end(); ++it)
                {
                    if ((*it)->isa(Rosegarden::Note::EventType))
                    {
                        Rosegarden::Segment *segment = 
                            new Rosegarden::Segment(
                                Rosegarden::Segment::Audio, 
                                (*it)->getAbsoluteTime());
                        segment->setTrack((*i)->getTrack());

                        // If we've constructed against an AudioFile
                        //
                        if (m_audioFile)
                        {
                            segment->setAudioFileId(m_audioFile->getId());
                            segment->setAudioStartTime(
                                    Rosegarden::RealTime::zeroTime);
                            segment->setAudioEndTime(
                                    m_audioFile->getLength());
                        }
                        else // or an audio Segment
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

    if (!addNew && m_newSegments.size())
    {
        // Reattach new segments
        //
        for (unsigned int i = 0; i < m_newSegments.size(); ++i)
            m_composition->addSegment(m_newSegments[i]);

        return;
    }
}

void 
DistributeAudioCommand::unexecute()
{
    for (unsigned int i = 0; i < m_newSegments.size(); ++i)
        m_composition->detachSegment(m_newSegments[i]);

    for (Rosegarden::SegmentSelection::iterator it = m_selection.begin();
         it != m_selection.end(); ++it)
        m_composition->addSegment(*it);
}

