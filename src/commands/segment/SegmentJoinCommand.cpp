/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


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


namespace Rosegarden
{

SegmentJoinCommand::SegmentJoinCommand(SegmentSelection &segments) :
        NamedCommand(getGlobalName()),
        m_newSegment(0),
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

void
SegmentJoinCommand::execute()
{
    Composition *composition = m_oldSegments[0]->getComposition();
    if (!composition) {
        RG_DEBUG << "SegmentJoinCommand::execute: ERROR: old segments are not in composition!";
        return ;
    }

    // we normalize rests in any overlapping areas
    timeT overlapStart = 0, overlapEnd = 0;
    bool haveOverlap = false;

    if (!m_newSegment) {

        // Find the leftmost segment's index and the rightmost segment's
        // index.
        timeT t0 = m_oldSegments[0]->getStartTime();
        timeT t1 = m_oldSegments[0]->getEndMarkerTime();
        size_t leftmostIndex = 0;
        size_t rightmostIndex = 0;
        for (size_t i = 1; i < m_oldSegments.size(); ++i) {
            timeT startTime = m_oldSegments[i]->getStartTime();
            if (startTime < t0) {
                t0 = startTime;
                leftmostIndex = i;
            }
            timeT endMarkerTime = m_oldSegments[i]->getEndMarkerTime();
            if (endMarkerTime > t1) {
                t1 = endMarkerTime;
                rightmostIndex = i;
            }
        }

        // Always begin with the leftmost segment to keep in the new segment
        // any clef or key change found at the start of this segment.
        m_newSegment = m_oldSegments[leftmostIndex]->clone(false);

        // drop any events beyond the end marker
        m_newSegment->setEndTime(m_newSegment->getEndMarkerTime());

        // that duplicated the leftmost segment; now do the rest

        for (size_t i = 0; i < m_oldSegments.size(); ++i) {

            // Don't add twice the first old segment
            if (i == leftmostIndex) continue;

            Segment *s = m_oldSegments[i];

            timeT start = s->getStartTime(), end = s->getEndMarkerTime();

            timeT os = 0, oe = 0;
            bool haveOverlapHere = false;

            if (start < m_newSegment->getEndMarkerTime() &&
                end > m_newSegment->getStartTime()) {
                haveOverlapHere = true;
                os = std::max(start, m_newSegment->getStartTime());
                oe = std::min(end, m_newSegment->getEndMarkerTime());
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

            if (start > m_newSegment->getEndMarkerTime()) {
                m_newSegment->setEndMarkerTime(start);
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
                        if (m_newSegment->getClefAtTime
                            ((*si)->getAbsoluteTime() + 1) == newClef) {
                            continue;
                        }
                    } catch (...) { }
                }

                if ((*si)->isa(Key::EventType)) {
                    try {
                        Key newKey(**si);
                        if (m_newSegment->getKeyAtTime
                            ((*si)->getAbsoluteTime() + 1) == newKey) {
                            continue;
                        }
                    } catch (...) { }
                }

                m_newSegment->insert(new Event(**si));
            }

            if (end > m_newSegment->getEndMarkerTime()) {
                m_newSegment->setEndMarkerTime(end);
            }
        }
    }

    composition->addSegment(m_newSegment);

    if (haveOverlap) {
        m_newSegment->normalizeRests(overlapStart, overlapEnd);
    }

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
