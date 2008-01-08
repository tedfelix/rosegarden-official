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


#include "SegmentJoinCommand.h"

#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentJoinCommand::SegmentJoinCommand(SegmentSelection &
                                       segments) :
        KNamedCommand(getGlobalName()),
        m_newSegment(0),
        m_detached(false) // true if the old segments are detached, not the new
{
    for (SegmentSelection::iterator i = segments.begin();
            i != segments.end(); ++i)
    {
        m_oldSegments.push_back(*i);
    }
    assert(m_oldSegments.size() > 0);
}

SegmentJoinCommand::~SegmentJoinCommand()
{
    if (m_detached) {
        for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
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
        std::cerr
        << "SegmentJoinCommand::execute: ERROR: old segments are not in composition!"
        << std::endl;
        return ;
    }

    // we normalize rests in any overlapping areas
    timeT overlapStart = 0, overlapEnd = 0;
    bool haveOverlap = false;

    if (!m_newSegment) {

        m_newSegment = new Segment(*m_oldSegments[0]);

        // that duplicated segment 0; now do the rest

        for (unsigned int i = 1; i < m_oldSegments.size(); ++i) {

            Segment *s = m_oldSegments[i];

            timeT start = s->getStartTime(), end = s->getEndMarkerTime();

            timeT os = 0, oe = 0;
            bool haveOverlapHere = false;

            if (start < m_newSegment->getEndMarkerTime() &&
                end > m_newSegment->getStartTime()) {
                haveOverlapHere = true;
                os = std::max(start, m_newSegment->getStartTime());
                oe = std::min(end, m_newSegment->getEndMarkerTime());
                std::cerr << "overlap here, os = " << os << ", oe = " << oe << std::endl;
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

            for (Segment::iterator si = s->begin();
                 s->isBeforeEndMarker(si); ++si) {

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

    for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
        composition->detachSegment(m_oldSegments[i]);
    }

    m_detached = true;
}

void
SegmentJoinCommand::unexecute()
{
    for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
        m_newSegment->getComposition()->addSegment(m_oldSegments[i]);
    }

    m_newSegment->getComposition()->detachSegment(m_newSegment);
    m_detached = false;
}

}
