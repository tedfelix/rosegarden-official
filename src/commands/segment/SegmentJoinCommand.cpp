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

#define RG_MODULE_STRING "[SegmentJoinCommand]"

#include "SegmentJoinCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/editors/segment/compositionview/CompositionView.h"

#include <QString>
#include <QtGlobal>


namespace Rosegarden
{

SegmentJoinCommand::SegmentJoinCommand(SegmentSelection &segments) :
        NamedCommand(getGlobalName()),
        m_newSegment(nullptr),
        m_detached(false) // true if the old segments are detached, not the new
{
    for (SegmentSelection::iterator i = segments.begin();
            i != segments.end(); ++i)
    {
        m_oldSegments.push_back(*i);
    }

    Q_ASSERT_X(!m_oldSegments.empty(),
            "SegmentJoinCommand::SegmentJoinCommand()",
            "No segments to join");
}

SegmentJoinCommand::~SegmentJoinCommand()
{
    if (m_detached) {
        for (size_t i = 0; i < m_oldSegments.size(); ++i) {
            delete m_oldSegments[i];
        }
    } else {
        delete m_newSegment;
    }
}
Segment *
SegmentJoinCommand::makeSegment(SegmentVec oldSegments)
{
    // We can proceed even if composition is nullptr, just normalize
    // rests will do less.
    Composition *composition = oldSegments[0]->getComposition();

    // Find the leftmost segment's index and the rightmost segment's
    // index.
    timeT t0 = oldSegments[0]->getStartTime();
    timeT t1 = oldSegments[0]->getEndMarkerTime();
    size_t leftmostIndex = 0;
    size_t rightmostIndex = 0;
    for (size_t i = 1; i < oldSegments.size(); ++i) {
        timeT startTime = oldSegments[i]->getStartTime();
        if (startTime < t0) {
            t0 = startTime;
            leftmostIndex = i;
        }
        timeT endMarkerTime = oldSegments[i]->getEndMarkerTime();
        if (endMarkerTime > t1) {
            t1 = endMarkerTime;
            rightmostIndex = i;
        }
    }

    // Always begin with the leftmost segment to keep in the new segment
    // any clef or key change found at the start of this segment.
    Segment *newSegment = oldSegments[leftmostIndex]->clone(false);

    // drop any events beyond the end marker
    newSegment->setEndTime(newSegment->getEndMarkerTime());

    // that duplicated the leftmost segment; now do the rest

    // we normalize rests in any overlapping areas
    timeT overlapStart = 0, overlapEnd = 0;
    bool haveOverlap = false;
    for (size_t i = 0; i < oldSegments.size(); ++i) {

        // Don't add twice the first old segment
        if (i == leftmostIndex) continue;

        Segment *s = oldSegments[i];

        timeT start = s->getStartTime(), end = s->getEndMarkerTime();

        timeT os = 0, oe = 0;
        bool haveOverlapHere = false;

        if (start < newSegment->getEndMarkerTime() &&
            end > newSegment->getStartTime()) {
            haveOverlapHere = true;
            os = std::max(start, newSegment->getStartTime());
            oe = std::min(end, newSegment->getEndMarkerTime());
            RG_DEBUG << "overlap here, os = " << os << ", oe = " << oe;
        }

        if (haveOverlapHere) {
            if (haveOverlap) {
                overlapStart = std::min(overlapStart, os);
                overlapEnd = std::max(overlapEnd, oe);
            } else {
                overlapStart = os;
                overlapEnd = oe;
                haveOverlap = true;
            }
        }

        if (start > newSegment->getEndMarkerTime()) {
            newSegment->setEndMarkerTime(start);
        }

        for (Segment::iterator si = s->begin(); ; ++si) {

            // If we're in the rightmost segment
            if (i == rightmostIndex) {
                // Copy all of the events to the end
                if (si == s->end())
                    break;
            } else {
                // Just copy up to the End Marker of this segment.
                if (!s->isBeforeEndMarker(si))
                    break;
            }

            // weed out duplicate clefs and keys

            if ((*si)->isa(Clef::EventType)) {
                try {
                    Clef newClef(**si);
                    if (newSegment->getClefAtTime
                        ((*si)->getAbsoluteTime() + 1) == newClef) {
                        continue;
                    }
                } catch (...) { }
            }

            if ((*si)->isa(Key::EventType)) {
                try {
                    Key newKey(**si);
                    if (newSegment->getKeyAtTime
                        ((*si)->getAbsoluteTime() + 1) == newKey) {
                        continue;
                    }
                } catch (...) { }
            }

            newSegment->insert(new Event(**si));
        }

        if (end > newSegment->getEndMarkerTime()) {
            newSegment->setEndMarkerTime(end);
        }
    }

    if (haveOverlap) {
        /// normalizeRests doesn't require Composition, but is less
        /// than ideal without it;
        newSegment->setComposition(composition);
        newSegment->normalizeRests(overlapStart, overlapEnd);
        newSegment->setComposition(nullptr);        
    }

    return newSegment;
}

void
SegmentJoinCommand::execute()
{
    Composition *composition = m_oldSegments[0]->getComposition();
    if (!composition) {
        RG_DEBUG << "SegmentJoinCommand::execute: ERROR: old segments are not in composition!";
        return ;
    }

    if (!m_newSegment) {
        m_newSegment = makeSegment(m_oldSegments);
    }

    composition->addSegment(m_newSegment);

    // Select this new joined segment.
    RosegardenMainWindow::self()->getView()->getTrackEditor()->
        getCompositionView()->getModel()->setSelected(m_newSegment);
    
    for (size_t i = 0; i < m_oldSegments.size(); ++i) {
        composition->detachSegment(m_oldSegments[i]);
    }

    m_detached = true;
}

void
SegmentJoinCommand::unexecute()
{
    for (size_t i = 0; i < m_oldSegments.size(); ++i) {
        // Add the segment back into the composition
        m_newSegment->getComposition()->addSegment(m_oldSegments[i]);
        // And select it
        RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getCompositionView()->getModel()->setSelected(m_oldSegments[i]);
    }

    m_newSegment->getComposition()->detachSegment(m_newSegment);
    m_detached = false;
}

}
