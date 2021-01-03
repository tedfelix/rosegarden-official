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
 *
 *
 * Here is an example:
 *
 *    // Create a LilyPondSegmentsContext object and insert into it segments
 *    // to export
 *    LilyPondSegmentsContext lsc(m_composition);
 *    for (...) {
 *        Segment * seg = ...
 *        lsc.addSegment(seg);
 *    }
 *
 *    // Prepare everything inside the LilyPondSegmentsContext
 *    lsc.precompute();
 *    lsc.fixRepeatStartTimes();
 *    lsc.fixVoltaStartTimes();
 *
 *    // Then get from it the segments sorted and ready to use
 *    Track *track;
 *    for (track = lsc.useFirstTrack(); track; track = lsc.useNextTrack()) {
 *        int voiceIndex;
 *        for (voiceIndex = lsc.useFirstVoice();
 *                voiceIndex != -1; voiceIndex = lsc.useNextVoice()) {
 *            Segment *seg;
 *            for (seg = lsc.useFirstSegment();
 *                    seg; seg = lsc.useNextSegment()) {
 *
 *                // Write here the LilyPond stuff related to seg
 *                ...
 *                // Getting data from context when needed
 *                int trackPos = lsc.getTrackPos();
 *                bool repeated = lsc.isRepeating();
 *                timeT lilyPondStartTime = lsc.getSegmentStartTime();
 *                etc...
 *
 *            }
 *        }
 *    }
 *
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
    LilyPondSegmentsContext(Composition *composition);

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
     * Return true if the segment is repeated (a repeating segment or
     * simple repeated links) without volta
     */
    bool isRepeated();

    /**
     * Return true if the segment is a repeating segment (not linked segments)
     */
    bool isRepeatingSegment();

    /**
     * Return true if the repetition is made of several linked segments
     * without alternate ends
     */
    bool isSimpleRepeatedLinks();

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
     * Return true if the previous segment (on the same voice) was repeating
     * without volta.
     */
    bool wasRepeatingWithoutVolta();

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

    static int m_nextRepeatId;

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
        mutable int syncCount;                // Number of parallel synchronous
                                              // segments on the other voices and
                                              // tracks
        mutable bool noRepeat;                // Repeat is forbidden
        mutable bool ignored;                 // Mark the segments inserted in
                                              // a repeat chain (with or without
                                              // volta) except the first one.

        mutable int repeatId;                 // Identify a repeat with volta chain
        mutable int numberOfVolta;            // How many repeat in the chain
        mutable bool startOfRepeatChain;
        mutable bool volta;                   // Mark a volta
        mutable VoltaChain * rawVoltaChain;
        mutable VoltaChain * sortedVoltaChain;

        mutable timeT startTime;              // In LilyPond output
        mutable timeT endTime;                // In LilyPond output

        mutable Rosegarden::Key previousKey;  // Last key in the previous segment

        mutable int simpleRepeatId;           // Identify a repeat without volta chain
        mutable int numberOfSimpleRepeats;    // How many segments in the chain

        SegmentData(Segment * seg)
        {
            segment = seg;
            duration = 0;
            wholeDuration = 0;
            numberOfRepeats = 0;
            remainderDuration = 0;
            synchronous = true;
            syncCount = 0;
            noRepeat = false;
            ignored = false;
            repeatId = 0;
            numberOfVolta = 0;
            startOfRepeatChain = false;
            volta = false;
            rawVoltaChain = nullptr;
            sortedVoltaChain = nullptr;
            startTime = 0;
            endTime = 0;
            previousKey = Rosegarden::Key("undefined");
            simpleRepeatId = 0;
            numberOfSimpleRepeats = 0;
         }
    };

    struct SegmentDataCmp {
        bool operator()(const SegmentData &s1, const SegmentData &s2) const;
    };

    class SegmentSet :
                public std::multiset<SegmentData,
                                     LilyPondSegmentsContext::SegmentDataCmp>
    {
    public:
        void scanForRepeatedLinks();

    private:
        void setIterators(iterator it);

        bool isPossibleStartOfSimpleRepeat();
        bool isNextSegmentOfSimpleRepeat();

        bool isPossibleStartOfRepeatWithVolta();
        bool isNextSegmentsOfRepeatWithVolta();

        bool isValidSimpleRepeatingSegment(iterator base, iterator target);
        bool isValidRepeatingSegment(iterator base, iterator baseVolta, iterator target);
        bool isValidVoltaSegment(iterator repeating, iterator target);

    private:
        iterator m_it0;
        iterator m_it1;
        iterator m_it2;
        iterator m_it3;
        iterator m_it4;

        iterator m_start;
        int m_count;
    };

    typedef std::map<int, SegmentSet> VoiceMap;
    typedef std::map<int, VoiceMap> TrackMap;

    class SegmentDataList : public std::list<const SegmentData *>
    {
    public:

        // Only for instrumentation while debugging
        void dump();
    };

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
    void lookForRepeatedLinksWithVolta(SegmentSet &segSet);

    /**
     * Look in the specified voice of the specified track for linked segments
     * which may be exported as simple repeat and mark them accordingly.
     * The concerned segments are gathered in the set passed as argument.
     *
     * The previously found repeat with volta sequences are ignored
     * when pass=1, but not when pass=2.
     */
    void lookForSimpleRepeatedLinks(SegmentSet &segSet, int pass = 1);

    /**
    * Look for similar segments in the raw volta chain (on all tracks
    * simultaneously) and fill the sorted volta chain accordingly.
    * The argument is the list of the associated synchronous main repeat
    * segment data from all the tracks.
    */
    void sortAndGatherVolta(SegmentDataList &);


    TrackMap m_segments;

    Composition * m_composition;

    timeT m_epsilon;
    timeT m_firstSegmentStartTime;
    timeT m_lastSegmentEndTime;
    bool m_automaticVoltaUsable;

    TrackMap::iterator m_trackIterator;
    VoiceMap::iterator m_voiceIterator;
    SegmentSet::iterator m_segIterator;
    VoltaChain::iterator m_voltaIterator;

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

    bool m_wasRepeatingWithoutVolta; // Remember the type of the previous
                                     // segment (which was the current one
                                     // before the last call of useNextSegment())

    bool m_lastWasOK;  // To deal with recursion in useNextSegment()
};


}

#endif // RG_LILYPOND_SEGMENTS_CONTEXT_H
