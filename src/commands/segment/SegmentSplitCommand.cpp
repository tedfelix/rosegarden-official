/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentSplitCommand::SegmentSplitCommand(Segment *segment,
        timeT splitTime, bool keepLabel) :
        KNamedCommand(i18n("Split Segment")),
        m_segment(segment),
        m_newSegmentA(0),
        m_newSegmentB(0),
        m_splitTime(splitTime),
        m_detached(true),
        m_keepLabel(keepLabel)
{}

SegmentSplitCommand::~SegmentSplitCommand()
{
    if (m_detached) {
        delete m_newSegmentA;
        delete m_newSegmentB;
    }
}

void
SegmentSplitCommand::execute()
{
    if (m_newSegmentA) {
        
        m_segment->getComposition()->addSegment(m_newSegmentA);
        m_segment->getComposition()->addSegment(m_newSegmentB);

        m_segment->getComposition()->detachSegment(m_segment);

        m_detached = false; // i.e. new segments are not detached
        return;
    }

    m_newSegmentA = new Segment(*m_segment);
    m_newSegmentB = new Segment();

    m_newSegmentB->setTrack(m_segment->getTrack());
    m_newSegmentB->setStartTime(m_splitTime);

    m_segment->getComposition()->addSegment(m_newSegmentA);
    m_segment->getComposition()->addSegment(m_newSegmentB);

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
        m_newSegmentB->insert(clefEvent);

    if (keyEvent)
        m_newSegmentB->insert(keyEvent);

    // Copy through the Events
    //
    it = m_segment->findTime(m_splitTime);

    if (it != m_segment->end() && (*it)->getAbsoluteTime() > m_splitTime) {
        m_newSegmentB->fillWithRests((*it)->getAbsoluteTime());
    }

    while (it != m_segment->end()) {
        m_newSegmentB->insert(new Event(**it));
        ++it;
    }
    m_newSegmentB->setEndTime(m_segment->getEndTime());
    m_newSegmentB->setEndMarkerTime(m_segment->getEndMarkerTime());

    // Set labels
    //
    std::string label = m_segment->getLabel();
    m_newSegmentA->setLabel(label);
    m_newSegmentB->setLabel(label);
    if (!m_keepLabel) {
        m_newSegmentA->setLabel(appendLabel(label, qstrtostr(i18n("(split)"))));
        m_newSegmentB->setLabel(appendLabel(label, qstrtostr(i18n("(split)"))));
    }

    m_newSegmentB->setColourIndex(m_segment->getColourIndex());
    m_newSegmentB->setTranspose(m_segment->getTranspose());
    m_newSegmentB->setDelay(m_segment->getDelay());

    // Resize left hand Segment
    //
    std::vector<Event *> toErase, toInsert;
    for (Segment::iterator i = m_newSegmentA->findTime(m_splitTime);
         i != m_newSegmentA->end(); ++i) {
        if ((*i)->getAbsoluteTime() >= m_splitTime) break;
        if ((*i)->getAbsoluteTime() + (*i)->getDuration() > m_splitTime) {
            Event *e = new Event(**i, (*i)->getAbsoluteTime(),
                                 m_splitTime - (*i)->getAbsoluteTime());
            toErase.push_back(*i);
            toInsert.push_back(e);
        }
    }

    for (int i = 0; i < toErase.size(); ++i) {
        m_newSegmentA->eraseSingle(toErase[i]);
        delete toErase[i];
    }
    for (int i = 0; i < toInsert.size(); ++i) {
        m_newSegmentA->insert(toInsert[i]);
    }

    m_newSegmentA->setEndTime(m_splitTime);
    m_newSegmentA->setEndMarkerTime(m_splitTime);

    m_segment->getComposition()->detachSegment(m_segment);

    m_detached = false; // i.e. new segments are not detached
}

void
SegmentSplitCommand::unexecute()
{
    m_newSegmentA->getComposition()->addSegment(m_segment);

    m_segment->getComposition()->detachSegment(m_newSegmentA);
    m_segment->getComposition()->detachSegment(m_newSegmentB);

    m_detached = true; // i.e. new segments are not detached
}

}
