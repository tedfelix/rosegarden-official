/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#include "LilyPondSegmentsContext.h"
#include "base/Composition.h"
#include "base/Segment.h"

#include <map>
#include <set>


namespace Rosegarden
{


LilyPondSegmentsContext::LilyPondSegmentsContext(LilyPondExporter *exporter,
                                                 Composition *composition) :
    m_exporter(exporter),
    m_composition(composition),
    m_firstSegmentStartTime(0)
{
    m_segments.clear();
    m_epsilon = Note(Note::Hemidemisemiquaver).getDuration() / 2;

}

LilyPondSegmentsContext::~LilyPondSegmentsContext()
{
}

void
LilyPondSegmentsContext::addSegment(Segment *segment)
{
    int trackId = segment->getTrack();
    m_segments[trackId].insert(SegmentData(segment));
}

void
LilyPondSegmentsContext::precompute()
{
    TrackMap::iterator tit;
    SegmentSet::iterator sit;

    // Find the start time of the first segment
    m_firstSegmentStartTime = m_composition->getEndMarker();
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        sit = (*tit).second.begin();
        if (sit != (*tit).second.end()) {
            timeT start = (*sit).segment->getStartTime();
            if (start < m_firstSegmentStartTime) m_firstSegmentStartTime = start;
        }
    }

    // Compute the duration and repeat count of each segment
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        int trackPos = (*tit).first;
        Track * track = m_composition->getTrackByPosition(trackPos);
        SegmentSet &segSet = (*tit).second;
        for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
            Segment * seg = (*sit).segment;
            (*sit).duration = seg->getEndMarkerTime() - seg->getStartTime();
            if (!seg->isRepeating()) {
                (*sit).wholeDuration = (*sit).duration;
                (*sit).numberOfRepeats = 0;
                (*sit).remainderDuration = 0;
            } else {
                SegmentSet::iterator next = sit;
                ++next;
                timeT endOfRepeat;
                if (next == segSet.end()) {
                    endOfRepeat = m_composition->getEndMarker();
                } else {
                    endOfRepeat = (*next).segment->getStartTime();
                }
                (*sit).wholeDuration = endOfRepeat - seg->getStartTime();
                (*sit).numberOfRepeats = (*sit).wholeDuration / (*sit).duration;
                (*sit).remainderDuration = (*sit).wholeDuration % (*sit).duration;
                if ((*sit).remainderDuration < m_epsilon) {
                    (*sit).remainderDuration = 0;
                }
            }
        }
    }

    // Don't allow segment to be shown as a repeat in LilyPond if another
    // segment is nor repeating in the same way in the same time.
    // For each segment, look at all the other ones and set the unfolded
    // flag when the segments can't be printed out with repeat bars.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        int trackPos = (*tit).first;
        Track * track = m_composition->getTrackByPosition(trackPos);
        SegmentSet &segSet = (*tit).second;
        for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
            Segment * seg = (*sit).segment;
            timeT start = seg->getStartTime();
            timeT end = start + (*sit).wholeDuration;

            TrackMap::iterator tit2;
            SegmentSet::iterator sit2;
            for (tit2 = m_segments.begin(); tit2 != m_segments.end(); ++tit2) {
                int trackPos2 = (*tit2).first;
                Track * track2 = m_composition->getTrackByPosition(trackPos2);
                SegmentSet &segSet2 = (*tit2).second;
                for (sit2 = segSet2.begin(); sit2 != segSet2.end(); ++sit2) {
                    Segment * seg2 = (*sit2).segment;
                    if (seg == seg2) continue;
                    timeT start2 = seg2->getStartTime();
                    timeT end2 = start2 + (*sit2).wholeDuration;

                    // When the two segments have the same bounds,
                    // repeat is possible.
                    if ((start2 == start) && (end2 == end)
                        && ((*sit).duration == (*sit2).duration)) continue;

                    // If the second segment is starting somewhere inside
                    // the first one, repeat is not possible for either
                    // of the two segments.
                    if ((start2 >= start) && (start2 < end)) {
                        (*sit).unfolded = true;
                        (*sit2).unfolded = true;
                    }
                }
            }
        }
    }

    // Compute the LilyPond start times with all segments unfolded.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        int trackPos = (*tit).first;
        Track * track = m_composition->getTrackByPosition(trackPos);
        SegmentSet &segSet = (*tit).second;
        for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
            Segment * seg = (*sit).segment;
            (*sit).startTime = seg->getStartTime() - m_firstSegmentStartTime;
        }
    }
}

void
LilyPondSegmentsContext::fixStartTimes()
{
    TrackMap::iterator tit;
    SegmentSet::iterator sit;

    // precompute() should have been called already and
    // we know what segment may be repeated and what segment may be unfolded.
    // We can compute the start time of each segment in the LilyPond score.

    // Sort the repeating segments into start times
    std::map<timeT, const SegmentData *> repeatedSegments;
    repeatedSegments.clear();
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        SegmentSet &segSet = (*tit).second;
        for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
            if ((*sit).numberOfRepeats && !(*sit).unfolded) {
                repeatedSegments[(*sit).startTime] = &(*sit);
            }
        }
    }

    // Then fix all the start times
    std::map<timeT, const SegmentData *>::reverse_iterator it;
    for (it=repeatedSegments.rbegin(); it!=repeatedSegments.rend(); ++it) {
        const SegmentData *segData = (*it).second;
        timeT deltaT = segData->wholeDuration - segData->duration;
        for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
            SegmentSet &segSet = (*tit).second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                if ((*sit).startTime > (*it).first) {
                    (*sit).startTime -= deltaT;
                }
            }
        }
    }
}

Track *
LilyPondSegmentsContext::useFirstTrack()
{
    m_trackIterator = m_segments.begin();
    if (m_trackIterator == m_segments.end()) return 0;
    return m_composition->getTrackByPosition((*m_trackIterator).first);
}

Track *
LilyPondSegmentsContext::useNextTrack()
{
    ++m_trackIterator;
    if (m_trackIterator == m_segments.end()) return 0;
    return m_composition->getTrackByPosition((*m_trackIterator).first);
}

int
LilyPondSegmentsContext::getTrackPos()
{
    if (m_trackIterator == m_segments.end()) return -1;
    return (*m_trackIterator).first;
}

Segment *
LilyPondSegmentsContext::useFirstSegment()
{
    m_segIterator = (*m_trackIterator).second.begin();
    if (m_segIterator == (*m_trackIterator).second.end()) return 0;
    return (*m_segIterator).segment;
}

Segment *
LilyPondSegmentsContext::useNextSegment()
{
    ++m_segIterator;
    if (m_segIterator == (*m_trackIterator).second.end()) return 0;
    return (*m_segIterator).segment;
}

timeT
LilyPondSegmentsContext::getSegmentStartTime()
{
    return (*m_segIterator).startTime;
}

int
LilyPondSegmentsContext::getNumberOfRepeats()
{
    return (*m_segIterator).numberOfRepeats;
}

bool
LilyPondSegmentsContext::isRepeated()
{
    return (*m_segIterator).numberOfRepeats;
}

bool
LilyPondSegmentsContext::isUnfolded()
{
    return (*m_segIterator).unfolded;
}

bool
LilyPondSegmentsContext::SegmentDataCmp::operator()(const SegmentData &s1, const SegmentData &s2) const
{
    // Sort segments according to start time, then end time, then address.
    // Copied from StaffHeader::SegmentCmp::operator()
    if (s1.segment->getStartTime() < s2.segment->getStartTime()) return true;
    if (s1.segment->getStartTime() > s2.segment->getStartTime()) return false;
    if (s1.segment->getEndMarkerTime() < s2.segment->getEndMarkerTime()) return true;
    if (s1.segment->getEndMarkerTime() > s2.segment->getEndMarkerTime()) return false;
    return (long) s1.segment < (long) s2.segment;
}

void
LilyPondSegmentsContext::dump()
{

    TrackMap::iterator tit;
    SegmentSet::iterator sit;

    std::cout << std::endl;
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        int trackPos = (*tit).first;
        Track * track = m_composition->getTrackByPosition(trackPos);
        SegmentSet &segSet = (*tit).second;

        std::cout << "Track pos=" << trackPos << " id=" << track->getId()
                  << "   \"" << track->getLabel() << "\"" << std::endl;
        for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
            Segment * seg = (*sit).segment;

            std::cout << "     Segment \"" << seg->getLabel()
                      << "  start=" << seg->getStartTime()
                      << " duration=" << (*sit).duration
                      << " wholeDuration=" <<  (*sit).wholeDuration
                      << std::endl;
            std::cout << "               numRepeat=" << (*sit).numberOfRepeats
                      << " remainder=" << (*sit).remainderDuration
                      << " unfolded=" << (*sit).unfolded
                      << " lilyStart=" << (*sit).startTime << std::endl;
        }
    }
    std::cout << std::endl;
}

}


