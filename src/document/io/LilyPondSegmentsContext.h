/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_LILYPOND_SEGMENTS_CONTEXT_H
#define RG_LILYPOND_SEGMENTS_CONTEXT_H

/*
 * LilyPondSegmentsContext.h
 *
 * This file defines a class which maintains the context of the segments
 * which have to be exported to LilyPond.
 *
 * This class is used to 
 *      - Hide the segments of the composition which are not exported
 *      - Simplify the access of voices inside a same track
 *      - See when a repeating segment may be printed inside
 *        repeat bars. (i.e. when no other unrepeating segment
 *        coexists at the same time on an other track).
 *      - Find out which linked segments may be printed as repeat with volta.
 *      - Compute the time offset which have to be set in LilyPond for
 *        each segment (with keyword \skip).
 */


#include "base/NotationTypes.h"

#include <set>
#include <map>
#include <list>
#include <string>


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
     * Create an empty segment context
     */
    LilyPondSegmentsContext(LilyPondExporter *exporter, Composition *composition);

    ~LilyPondSegmentsContext();

    /**
     * Add a segment to the context
     */
    void addSegment(Segment *segment);

    /**
     * Return true if the segment context is empty
     */
    bool containsNoSegment();

    /**
     * Walk through all segments, find the repeating ones and compute
     * their start times in the LilyPond score assuming they are synchronous.
     */
    void precompute();

    /**
     * Walk through all segments and fix their start times when some repeating
     * segments are printed with repeat bars in the LilyPond score.
     */
    void fixRepeatStartTimes();

    /**
     * Walk through all segments and fix their start times when some linked
     * segments are printed in the LilyPond score as repeat with volta.
     */
    void fixVoltaStartTimes();

    /**
     * Return the smaller start time of the segments being exported.
     * Only valid after precompute() has been executed.
     */
    timeT getFirstSegmentStartTime() { return m_firstSegmentStartTime; }

    /**
     * Return the larger end time of the segments being exported.
     * Only valid after precompute() has been executed.
     */
    timeT getLastSegmentEndTime() { return m_lastSegmentEndTime; }

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
     * Prepare to get the segments on the first voice of the current track.
     * Return the voice index of the first voice.
     * Return -1 if there is no track.
     */
    int useFirstVoice();

    /**
     * Go to the next voice.
     * Return the voice index of this voice.
     * Return -1 if there is no more voice.
     */
    int useNextVoice();

    /**
     * Return the current voice index or -1
     */
    int getVoiceIndex();

    /**
     * Prepare to get the segments on the current track and for the current
     * voice.
     * Return null if there is no segment on the current track and voice.
     */
    Segment * useFirstSegment();

    /**
     * Go to the next segment.
     * Return null if there is no more segment on the current track and voice.
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
     * Return true if the segment is inside a "repeat with volta" chain
     */
    bool isRepeatWithVolta();

    /**
     * Return true if the segment is synchronous
     */
    bool isSynchronous();

    /**
     * Return true if the segment is a volta
     */
    bool isVolta();

    /**
     * Return true if the segment is the first volta of a chain
     */
    bool isFirstVolta();

    /**
     * Return true if the segment is the last volta of a chain
     */
    bool isLastVolta();

    /**
     * Return the text of the current volta.
     */
    std::string getVoltaText();

    /**
     * Return the number of time the current volta is played.
     */
    int getVoltaRepeatCount();
    
    /**
     * Return the last key signature defined on the last contiguous segment
     * on the same voice.
     * Return an undefined key (or default key) if the previous segment is
     * not contiguous or if there is no previous segment.
     */
    Rosegarden::Key getPreviousKey();
    
    /**
     * Return true if LilyPond automatic volta mode is usable.
     * Valid as soon as precompute() has been executed.
     * 
     * Currently, this is a global flag: automatic and manual repeat/volta
     * are not mixed in the same score.
     */
    bool isAutomaticVoltaUsable() { return m_automaticVoltaUsable; } 

    /// Only for instrumentation while debugging
    void dump();


private :
    
    struct SegmentData;

    struct Volta {
        const SegmentData * data;
        std::set<int> voltaNumber;

        Volta(const SegmentData *sd, int number)
        {
            data = sd;
            voltaNumber.insert(number);
        }
    };
    
    typedef std::vector<Volta *> VoltaChain;

    struct SegmentData
    {
        Segment * segment;

        mutable timeT duration;               // Duration without repeat
        mutable timeT wholeDuration;          // Duration with repeat
        mutable int numberOfRepeats;          // 0 if not repeating
        mutable timeT remainderDuration;

        mutable bool synchronous;             // Multitrack repeat is possible
        mutable bool noRepeat;                // Repeat is forbidden
        mutable int repeatId;                 // Identify a repeat chain
        mutable int numberOfRepeatLinks;      // How many repeat in a chain

        mutable bool startOfRepeatChain;
        mutable bool volta;                   // Mark a volta
        mutable bool ignored;                 // Mark a segment inserted
                                              // in a repeat chain.
        mutable VoltaChain * rawVoltaChain;
        mutable VoltaChain * sortedVoltaChain;

        mutable timeT startTime;              // In LilyPond output
        mutable timeT endTime;                // In LilyPond output

        mutable Rosegarden::Key previousKey;  // Last key in the previous segment

        SegmentData(Segment * seg)
        {
            segment = seg;
            duration = 0;
            wholeDuration = 0;
            numberOfRepeats = 0;
            remainderDuration = 0;
            synchronous = true;
            noRepeat = false;
            repeatId = 0;
            numberOfRepeatLinks = 0;
            startOfRepeatChain = false;
            volta = false;
            ignored = false;
            rawVoltaChain = 0;
            sortedVoltaChain = 0;
            startTime = 0;
            endTime = 0;
            previousKey = Rosegarden::Key("undefined");
        }
    };

    struct SegmentDataCmp {
        bool operator()(const SegmentData &s1, const SegmentData &s2) const;
    };
    typedef std::multiset<SegmentData, LilyPondSegmentsContext::SegmentDataCmp> SegmentSet;
    typedef std::map<int, SegmentSet> VoiceMap;
    typedef std::map<int, VoiceMap> TrackMap;

    typedef std::list<const SegmentData *> SegmentDataList;


   /**
    * Begin to look on all tracks/voices for all segments synchronous of the
    * given one.
    * Return null if no segment found.
    */
    const SegmentData * getFirstSynchronousSegment(Segment * seg);

   /**
    * Get the next segment synchronous of the one passed as argument of
    * the last call of getFirstSynchronousSegment().
    * Return null if no more segment found.
    */
    const SegmentData * getNextSynchronousSegment();

    /**
     * Look in the specified voice of the specified track for linked segments
     * which may be exported as repeat with volta and mark them accordingly.
     * The concerned segments are gathered in the set passed as argument.
     */
    void lookForRepeatedLinks(SegmentSet &segSet);

    /**
    * Look for similar segments in the raw volta chain (on all tracks
    * simultaneously) and fill the sorted volta chain accordingly.
    * The argument is the list of the associated synchronous main repeat
    * segment data from all the tracks.
    */
    void sortAndGatherVolta(SegmentDataList &);


    // Only for instrumentation while debugging
    void dumpSDL(SegmentDataList & l);


    TrackMap m_segments;

    LilyPondExporter * m_exporter;
    Composition * m_composition;
    
    timeT m_epsilon;
    timeT m_firstSegmentStartTime;
    timeT m_lastSegmentEndTime;
    bool m_automaticVoltaUsable;

    TrackMap::iterator m_trackIterator;
    VoiceMap::iterator m_voiceIterator;
    SegmentSet::iterator m_segIterator;
    VoltaChain::iterator m_voltaIterator;

    int m_nextRepeatId;

    // Used by "Get Synchronous Segment" (GSS) methods
    // getFirstSynchronousSegment() and getNextSynchronousSegment() to remember
    // the current position in maps and set.
    Segment * m_GSSSegment;
    TrackMap::iterator m_GSSTrackIterator;
    VoiceMap::iterator m_GSSVoiceIterator;
    SegmentSet::iterator m_GSSSegIterator;

    bool m_repeatWithVolta; // Repeat with volta is usable in LilyPondExporter
    
    // Values associated with segment returned by useNextSegment()
    VoltaChain * m_currentVoltaChain;
    bool m_firstVolta;
    bool m_lastVolta;
    
    bool m_lastWasOK;  // To deal with recursivity in useNextSegment()
};


}

#endif // RG_LILYPOND_SEGMENTS_CONTEXT_H
