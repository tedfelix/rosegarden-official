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


#include "SegmentSplitByRecordingSrcCommand.h"

#include "base/BaseProperties.h"
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

SegmentSplitByRecordingSrcCommand::SegmentSplitByRecordingSrcCommand (
    Segment *segment, int channel, int device ) :
        NamedCommand(tr("Split by Recording Source")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_newSegmentA(0),
        m_channel(channel),
        m_device(device),
        m_executed(false)
{}

void
SegmentSplitByRecordingSrcCommand::execute()
{
    if (!m_newSegmentA) {

        m_newSegmentA = new Segment;
        m_newSegmentB = new Segment;

        m_newSegmentA->setTrack(m_segment->getTrack());
        m_newSegmentA->setStartTime(m_segment->getStartTime());

        m_newSegmentB->setTrack(m_segment->getTrack());
        m_newSegmentB->setStartTime(m_segment->getStartTime());

        bool selectedC = false;
        bool selectedD = false;

        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ++i) {

            if ((*i)->isa(Note::EventRestType))
                continue;

            if ( (*i)->isa(Clef::EventType) ||
                    (*i)->isa(Key::EventType) ) {

                m_newSegmentA->insert(new Event(**i));
                m_newSegmentB->insert(new Event(**i));
                continue;
            }

            selectedC = false;
            selectedD = false;

            if ((*i)->has(BaseProperties::RECORDED_CHANNEL)) {
                selectedC = true;
                if (m_channel > -1)
                    selectedC = ( m_channel ==
                                  (*i)->get
                                  <Int>(BaseProperties::RECORDED_CHANNEL) );
            }

            if ((*i)->has(BaseProperties::RECORDED_PORT)) {
                selectedD = true;
                if (m_device > -1)
                    selectedD = ( m_device ==
                                  (*i)->get
                                  <Int>(BaseProperties::RECORDED_PORT) );
            }

            if (selectedC & selectedD) {
                if (m_newSegmentB->empty()) {
                    m_newSegmentB->fillWithRests((*i)->getAbsoluteTime());
                }
                m_newSegmentB->insert(new Event(**i));
            } else {
                if (m_newSegmentA->empty()) {
                    m_newSegmentA->fillWithRests((*i)->getAbsoluteTime());
                }
                m_newSegmentA->insert(new Event(**i));
            }
        }

        m_newSegmentA->normalizeRests(m_segment->getStartTime(),
                                      m_segment->getEndMarkerTime());
        m_newSegmentB->normalizeRests(m_segment->getStartTime(),
                                      m_segment->getEndMarkerTime());

        std::string label = m_segment->getLabel();
        m_newSegmentA->setLabel(appendLabel(label, qstrtostr(tr("(split)"))));
        m_newSegmentB->setLabel(appendLabel(label, qstrtostr(tr("(split)"))));
        m_newSegmentA->setColourIndex(m_segment->getColourIndex());
        m_newSegmentB->setColourIndex(m_segment->getColourIndex());
    }

    m_composition->addSegment(m_newSegmentA);
    m_composition->addSegment(m_newSegmentB);
    m_composition->detachSegment(m_segment);
    m_executed = true;
}

void
SegmentSplitByRecordingSrcCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_composition->detachSegment(m_newSegmentA);
    m_composition->detachSegment(m_newSegmentB);
    m_executed = false;
}

SegmentSplitByRecordingSrcCommand::~SegmentSplitByRecordingSrcCommand()
{
    if (m_executed) {
        delete m_segment;
    } else {
        delete m_newSegmentA;
        delete m_newSegmentB;
    }
}

}
