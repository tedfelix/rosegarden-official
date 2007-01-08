/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "AudioSegmentResizeFromStartCommand.h"

#include <klocale.h>
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"


namespace Rosegarden
{

AudioSegmentResizeFromStartCommand::AudioSegmentResizeFromStartCommand(Segment *segment,
        timeT newStartTime) :
        KNamedCommand(i18n("Resize Segment")),
        m_segment(segment),
        m_newSegment(0),
        m_detached(false),
        m_oldStartTime(segment->getStartTime()),
        m_newStartTime(newStartTime)
{}

AudioSegmentResizeFromStartCommand::~AudioSegmentResizeFromStartCommand()
{
    if (!m_detached)
        delete m_segment;
    else
        delete m_newSegment;
}

void
AudioSegmentResizeFromStartCommand::execute()
{
    Composition *c = m_segment->getComposition();

    if (!m_newSegment) {
        RealTime oldRT = c->getElapsedRealTime(m_oldStartTime);
        RealTime newRT = c->getElapsedRealTime(m_newStartTime);

        m_newSegment = new Segment(*m_segment);
        m_newSegment->setStartTime(m_newStartTime);
        m_newSegment->setAudioStartTime(m_segment->getAudioStartTime() -
                                        (oldRT - newRT));
    }

    c->addSegment(m_newSegment);
    m_newSegment->setEndMarkerTime(m_segment->getEndMarkerTime());
    c->detachSegment(m_segment);

    m_detached = false;
}

void
AudioSegmentResizeFromStartCommand::unexecute()
{
    Composition *c = m_newSegment->getComposition();
    c->addSegment(m_segment);
    c->detachSegment(m_newSegment);

    m_detached = true;
}

}
