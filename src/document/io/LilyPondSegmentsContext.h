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


#ifndef _LILYPOND_SEGMENTS_CONTEXT_H_
#define _LILYPOND_SEGMENTS_CONTEXT_H_

/*
 * LilyPondSegmentsContext.h
 *
 * This file defines a class which maintains the context of the segments
 * which have to be exported to LilyPond.
 *
 * This context is used to 
 *      - See when a repeating segment may be cam ne print inside
 *        repeat bars. (i.e. when no other unrepeating segment
 *        coexists at the same time on an other track).
 *      - Compute the time offset which have to be set il LilyPond for
 *        each segment (with keyword \skip).
 */


#include "base/NotationTypes.h"

#include <set>
#include <map>


namespace Rosegarden
{

class Composition;
class Track;
class Segment;
class LilyPondExporter;

class LilyPondSegmentsContext
{

public:
    /**
     * Create an empty context
     */
    LilyPondSegmentsContext(LilyPondExporter *exporter, Composition *composition);

    ~LilyPondSegmentsContext();

    /**
     * Add a segment to the context
     */
    void addSegment(Segment *segment);

    /**
     * Walk through all segments, find the repeating ones and compute
     * their start times in the LilyPond score assuming they are unfolded.
     */
    void precompute();

    /**
     * Walk through all segments and fix their start times when some of them
     * are printed with repeat bars in the LilyPond score.
     */
    void fixStartTimes();

    /**
     * Prepare to get the segments on the first track.
     * Return null if there is no track.
     */
    Track * useFirstTrack();

    /**
     * Go to the next track
     * Return null if there is no more track.
     */
    Track * useNextTrack();

    /**
     * Return the position of the current track or -1
     */
    int getTrackPos();

    /**
     * Prepare to get the segments on the current track.
     * Return null if there is no segment on the current track.
     */
    Segment * useFirstSegment();

    /**
     * Go to the next segment.
     * Return null if there is no more segment on the current track.
     */
    Segment * useNextSegment();

    /**
     * Return the start time of the current segment in LilyPond
     */
    timeT getSegmentStartTime();

    /**
     * Return how many time the current segment is repeated in LilyPond
     */
    int getNumberOfRepeats();

    /**
     * Return true if the segment is repeated
     */
    bool isRepeated();

    /**
     * Return true if the segment is unfolded
     */
    bool isUnfolded();

    /// Only for debug
    void dump();


private :

    struct SegmentData
    {
        Segment * segment;
        
        mutable timeT duration;               // Duration without repeat
        mutable timeT wholeDuration;          // Duration with repeat
        mutable int numberOfRepeats;          // 0 if not repeating
        mutable timeT remainderDuration;
        
        mutable bool unfolded;                // repeat not allowed

        mutable timeT startTime;       // In LilyPond output
        mutable timeT endTime;         // In LilyPond output

        SegmentData(Segment * seg)
        {
            segment = seg;
            duration = 0;
            wholeDuration = 0;
            numberOfRepeats = 0;
            remainderDuration = 0;
            unfolded = false;
            startTime = 0;
            endTime = 0;
        }
    };

    struct SegmentDataCmp {
        bool operator()(const SegmentData &s1, const SegmentData &s2) const;
    };
    typedef std::multiset<SegmentData, LilyPondSegmentsContext::SegmentDataCmp> SegmentSet;
    typedef std::map<int, SegmentSet> TrackMap;

    TrackMap m_segments;

    LilyPondExporter * m_exporter;
    Composition * m_composition;
    
    timeT m_epsilon;
    timeT m_firstSegmentStartTime;

    TrackMap::iterator m_trackIterator;
    SegmentSet::iterator m_segIterator;

};


}

#endif // _LILYPOND_SEGMENTS_CONTEXT_H_
