/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentSplitCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/editors/segment/compositionview/CompositionView.h"

#include <QString>


namespace Rosegarden
{

SegmentSplitCommand::SegmentSplitCommand(Segment *segment,
        timeT splitTime, bool keepLabel) :
        NamedCommand(tr("Split Segment")),
        m_segment(segment),
        m_newSegmentA(nullptr),
        m_newSegmentB(nullptr),
        m_splitTime(splitTime),
        m_detached(true),
        m_keepLabel(keepLabel),
        m_wasSelected(false)
{}

SegmentSplitCommand::~SegmentSplitCommand()
{
    if (m_detached) {
        delete m_newSegmentA;
        delete m_newSegmentB;
    }
}

// ??? Or we could have execute() return a bool indicating success, then
//   deal with failure within CommandHistory::addCommand().  That seems
//   like a big project, however.
bool 
SegmentSplitCommand::isValid(Segment * segment, timeT splitTime)
{
    // Can't split before or at the very start of a segment.
    if (splitTime <= segment->getStartTime())
        return false;

    // Can't split after or at the very end of a segment.
    if (splitTime >= segment->getEndMarkerTime())
        return false;

    return true;
}

SegmentSplitCommand::SegmentVec
SegmentSplitCommand::
getNewSegments(Segment *segment, timeT splitTime, bool keepLabel)
{
    Segment * newSegmentA = segment->clone(false);
    Segment * newSegmentB = new Segment();

    newSegmentB->setTrack(segment->getTrack());
    newSegmentB->setStartTime(splitTime);

    // !!! Set Composition?
    
    Event *clefEvent = nullptr;
    Event *keyEvent = nullptr;

    // Copy the last occurrence of clef and key
    // from the left hand side of the split (nb. timesig events
    // don't appear in segments, only in composition)
    //
    Segment::iterator it = segment->findTime(splitTime);

    while (it != segment->begin()) {

        --it;

        if (!clefEvent && (*it)->isa(Clef::EventType)) {
            clefEvent = new Event(**it, splitTime);
        }

        if (!keyEvent && (*it)->isa(Key::EventType)) {
            keyEvent = new Event(**it, splitTime);
        }

        if (clefEvent && keyEvent)
            break;
    }

    // Insert relevant meta info if we've found some
    //
    if (clefEvent)
        newSegmentB->insert(clefEvent);

    if (keyEvent)
        newSegmentB->insert(keyEvent);

    // Copy through the Events
    //
    it = segment->findTime(splitTime);

    if (it != segment->end() && (*it)->getAbsoluteTime() > splitTime) {
        newSegmentB->fillWithRests((*it)->getAbsoluteTime());
    }

    while (it != segment->end()) {
        newSegmentB->insert(new Event(**it));
        ++it;
    }
    newSegmentB->setEndTime(segment->getEndTime());
    newSegmentB->setEndMarkerTime(segment->getEndMarkerTime());

    // Set labels
    //
    std::string label = segment->getLabel();
    newSegmentA->setLabel(label);
    newSegmentB->setLabel(label);
    if (!keepLabel) {
        newSegmentA->setLabel(appendLabel(label, qstrtostr(tr("(split)"))));
        newSegmentB->setLabel(appendLabel(label, qstrtostr(tr("(split)"))));
    }

    newSegmentB->setColourIndex(segment->getColourIndex());
    newSegmentB->setTranspose(segment->getTranspose());
    newSegmentB->setDelay(segment->getDelay());

    // Resize left hand Segment
    //
    std::vector<Event *> toErase, toInsert;
    for (Segment::iterator i = newSegmentA->findTime(splitTime);
         i != newSegmentA->end(); ++i) {
        if ((*i)->getAbsoluteTime() >= splitTime) break;
        if ((*i)->getAbsoluteTime() + (*i)->getDuration() > splitTime) {
            Event *e = new Event(**i, (*i)->getAbsoluteTime(),
                                 splitTime - (*i)->getAbsoluteTime());
            toErase.push_back(*i);
            toInsert.push_back(e);
        }
    }

    for (size_t i = 0; i < toErase.size(); ++i) {
        newSegmentA->eraseSingle(toErase[i]);
        delete toErase[i];
    }
    for (size_t i = 0; i < toInsert.size(); ++i) {
        newSegmentA->insert(toInsert[i]);
    }

    newSegmentA->setEndTime(splitTime);
    newSegmentA->setEndMarkerTime(splitTime);
    SegmentVec segmentVec;
    segmentVec.reserve(2);
    segmentVec.push_back(newSegmentA);
    segmentVec.push_back(newSegmentB);
    return segmentVec;
}

void
SegmentSplitCommand::execute()
{
    m_wasSelected =
        RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getCompositionView()->getModel()->isSelected(m_segment);

    if (!m_newSegmentA) {
        SegmentVec splitSegments =
            getNewSegments(m_segment, m_splitTime, m_keepLabel);
        m_newSegmentA = splitSegments[0];
        m_newSegmentB = splitSegments[1];
    }
    
    m_segment->getComposition()->addSegment(m_newSegmentA);
    m_segment->getComposition()->addSegment(m_newSegmentB);
    m_segment->getComposition()->detachSegment(m_segment);

    m_detached = false; // i.e. new segments are not detached

    // If the original segment was selected
    if (m_wasSelected) {
        // Select the two split segments.
        RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getCompositionView()->getModel()->setSelected(m_newSegmentA);
        RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getCompositionView()->getModel()->setSelected(m_newSegmentB);
    }

}

void
SegmentSplitCommand::unexecute()
{
    m_newSegmentA->getComposition()->addSegment(m_segment);

    // If the original segment was selected, select it
    if (m_wasSelected) {
        RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getCompositionView()->getModel()->setSelected(m_segment);
    }

    m_segment->getComposition()->detachSegment(m_newSegmentA);
    m_segment->getComposition()->detachSegment(m_newSegmentB);

    m_detached = true; // i.e. new segments are not detached
}

}
