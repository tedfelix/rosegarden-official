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


#include "SegmentSplitCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentSplitCommand::SegmentSplitCommand(Segment *segment,
        timeT splitTime) :
        KNamedCommand(i18n("Split Segment")),
        m_segment(segment),
        m_newSegment(0),
        m_splitTime(splitTime),
        m_previousEndMarkerTime(0),
        m_detached(false)
{}

SegmentSplitCommand::~SegmentSplitCommand()
{
    if (m_detached) {
        delete m_newSegment;
    }
    delete m_previousEndMarkerTime;
}

void
SegmentSplitCommand::execute()
{
    if (!m_newSegment) {

        m_newSegment = new Segment;

        m_newSegment->setTrack(m_segment->getTrack());
        m_newSegment->setStartTime(m_splitTime);
        m_segment->getComposition()->addSegment(m_newSegment);

        Event *clefEvent = 0;
        Event *keyEvent = 0;

        // Copy the last occurrence of clef and key
        // from the left hand side of the split (nb. timesig events
        // don't appear in segments, only in composition)
        //
        Segment::iterator it = m_segment->findTime(m_splitTime);

        while (it != m_segment->begin()) {

            --it;

            if (!clefEvent && (*it)->isa(Clef::EventType)) {
                clefEvent = new Event(**it, m_splitTime);
            }

            if (!keyEvent && (*it)->isa(Key::EventType)) {
                keyEvent = new Event(**it, m_splitTime);
            }

            if (clefEvent && keyEvent)
                break;
        }

        // Insert relevant meta info if we've found some
        //
        if (clefEvent)
            m_newSegment->insert(clefEvent);

        if (keyEvent)
            m_newSegment->insert(keyEvent);

        // Copy through the Events
        //
        it = m_segment->findTime(m_splitTime);

        if (it != m_segment->end() && (*it)->getAbsoluteTime() > m_splitTime) {
            m_newSegment->fillWithRests((*it)->getAbsoluteTime());
        }

        while (it != m_segment->end()) {
            m_newSegment->insert(new Event(**it));
            ++it;
        }
        m_newSegment->setEndTime(m_segment->getEndTime());
        m_newSegment->setEndMarkerTime(m_segment->getEndMarkerTime());

        // Set labels
        //
        m_segmentLabel = m_segment->getLabel();
        QString newLabel = strtoqstr(m_segmentLabel);
        if (!newLabel.endsWith(i18n(" (split)"))) {
            newLabel = i18n("%1 (split)").arg(newLabel);
        }
        m_segment->setLabel(newLabel);
        m_newSegment->setLabel(m_segment->getLabel());
        m_newSegment->setColourIndex(m_segment->getColourIndex());
        m_newSegment->setTranspose(m_segment->getTranspose());
        m_newSegment->setDelay(m_segment->getDelay());
    }

    // Resize left hand Segment
    //
    const timeT *emt = m_segment->getRawEndMarkerTime();
    if (emt) {
        m_previousEndMarkerTime = new timeT(*emt);
    } else {
        m_previousEndMarkerTime = 0;
    }

    m_segment->setEndMarkerTime(m_splitTime);

    if (!m_newSegment->getComposition()) {
        m_segment->getComposition()->addSegment(m_newSegment);
    }

    m_detached = false;

}

void
SegmentSplitCommand::unexecute()
{
    if (m_previousEndMarkerTime) {
        m_segment->setEndMarkerTime(*m_previousEndMarkerTime);
        delete m_previousEndMarkerTime;
        m_previousEndMarkerTime = 0;
    } else {
        m_segment->clearEndMarker();
    }

    m_segment->setLabel(m_segmentLabel);
    m_segment->getComposition()->detachSegment(m_newSegment);
    m_detached = true;
}

}
