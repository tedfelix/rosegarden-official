/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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
#include <sstream>
#include <iostream>

namespace Rosegarden
{

int LilyPondSegmentsContext::m_nextRepeatId = 1;


LilyPondSegmentsContext::LilyPondSegmentsContext(Composition *composition) :
    m_composition(composition),
    m_firstSegmentStartTime(0),
    m_lastSegmentEndTime(0),
    m_automaticVoltaUsable(true),
    m_GSSSegment(nullptr),
    m_repeatWithAlt(false),
    m_currentAltChain(nullptr),
    m_firstAlt(false),
    m_lastAlt(false),
    m_wasRepeatingWithoutAlt(false),
    m_lastWasOK(false)
{
    m_segments.clear();
    m_epsilon = Note(Note::Hemidemisemiquaver).getDuration() / 2;

}

LilyPondSegmentsContext::~LilyPondSegmentsContext()
{
    TrackMap::iterator tit;
    VoiceMap::iterator vit;
    SegmentSet::iterator sit;

    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                if (sit->rawAltChain) {
                    AltChain::iterator i;
                    for (i = sit->rawAltChain->begin();
                            i != sit->rawAltChain->end(); ++i) {
                        delete *i;
                    }
                    delete sit->rawAltChain;
                    // sortedAltChain elements points to rawAltChain ones:
                    // They already have been deleted.
                    delete sit->sortedAltChain;
                }
            }
        }
    }
}


void
LilyPondSegmentsContext::addSegment(Segment *segment)
{
    int trackPos = m_composition->getTrackPositionById(segment->getTrack());
    int voice = m_composition->getSegmentVoiceIndex(segment);
    m_segments[trackPos][voice].insert(SegmentData(segment));
}


bool
LilyPondSegmentsContext::containsNoSegment() const
{
    return m_segments.size() == 0;
}

void
LilyPondSegmentsContext::precompute()
{
    TrackMap::iterator tit;
    VoiceMap::iterator vit;
    SegmentSet::iterator sit;

    // Look for previous key of each segment.
    // Walk through the segments of a voice and set the previousKey of each
    // segment to the value of the last key found in the previous contiguous
    // segment.
    // This works because segments of a same voice can be contiguous but can't
    // overlap.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            // int voiceIndex = vit->first;
            SegmentSet &segSet = vit->second;
            Rosegarden::Key key;
            bool firstSeg = true;
            timeT lastEndTime;
            Rosegarden::Key lastKey = Rosegarden::Key("undefined");
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                Segment * seg = sit->segment;
                Segment::iterator i;
                key = Rosegarden::Key("undefined");
                for (i = seg->begin(); i != seg->end(); ++i) {
                    Event *e = *i;
                    if (e->isa(Rosegarden::Key::EventType)) {
                        key = Rosegarden::Key(*e);
                    }
                }
                if (firstSeg) {
                    sit->previousKey = lastKey;
                } else {
                    if ((seg->getStartTime() - lastEndTime) < m_epsilon) {
                        sit->previousKey = lastKey;
                    } else {
                        sit->previousKey = Rosegarden::Key("undefined");
                    }
                }
                firstSeg = false;
                lastEndTime = seg->getEndMarkerTime();
                lastKey = key;
            }
        }
    }

    // Set at initialization.
    // If needed, sortAndGatherAlt() method or following "Look for linked
    // segments" loop will clear it.
    m_automaticVoltaUsable = true;

    // Find the start time of the first segment and the end time of the
    // last segment.
    m_firstSegmentStartTime = m_composition->getEndMarker();
    m_lastSegmentEndTime = m_composition->getStartMarker();
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            sit = vit->second.begin();
            if (sit != vit->second.end()) {
                timeT start = sit->segment->getStartTime();
                if (start < m_firstSegmentStartTime) m_firstSegmentStartTime = start;
                timeT end = sit->segment->getEndMarkerTime();
                if (end > m_lastSegmentEndTime) m_lastSegmentEndTime = end;
            }
        }
    }

    // Compute the duration and repeat count of each segment
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        /* int trackPos = tit->first; */
        /* Track * track = m_composition->getTrackByPosition(trackPos); */
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            /* int voiceIndex = vit->first; */
            SegmentSet &segSet = vit->second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                Segment * seg = sit->segment;
                sit->duration = seg->getEndMarkerTime() - seg->getStartTime();
                if (!seg->isRepeating()) {
                    sit->wholeDuration = sit->duration;
                    sit->numberOfRepeats = 0;
                    sit->remainderDuration = 0;
                } else {
                    SegmentSet::iterator next = sit;
                    ++next;
                    timeT endOfRepeat;
                    if (next == segSet.end()) {
                        endOfRepeat = m_composition->getEndMarker();
                    } else {
                        endOfRepeat = (*next).segment->getStartTime();
                    }
                    sit->wholeDuration = endOfRepeat - seg->getStartTime();
                    sit->numberOfRepeats = sit->wholeDuration / sit->duration;
                    sit->remainderDuration = sit->wholeDuration % sit->duration;
                    if (sit->remainderDuration < m_epsilon) {
                        sit->remainderDuration = 0;
                    }
                }
            }
        }
    }

    // Look for synchronous segments.
    // A synchronous segment has no other segment starting or ending while
    // it is running. When a segment is not synchronous, the other segments
    // whose time range overlaps its own time range can't be synchronous.
    // Only synchronous segments may be shown with repeat in LilyPond.

    // For each segment, look at all the other ones and clear the synchronous
    // flag when the segments can't be printed out with repeat bars.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        /* int trackPos = tit->first; */
        /* Track * track = m_composition->getTrackByPosition(trackPos); */
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            /* int voiceIndex = vit->first; */
            SegmentSet &segSet = vit->second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                Segment * seg = sit->segment;
                timeT start = seg->getStartTime();
                timeT end = start + sit->wholeDuration;

                TrackMap::iterator tit2;
                VoiceMap::iterator vit2;
                SegmentSet::iterator sit2;
                for (tit2 = m_segments.begin(); tit2 != m_segments.end(); ++tit2) {
                    /* int trackPos2 = tit2->first; */
                    /* Track * track2 = m_composition->getTrackByPosition(trackPos2); */
                    for (vit2 = tit2->second.begin();
                            vit2 != tit2->second.end(); ++vit2) {
                        SegmentSet &segSet2 = vit2->second;
                        for (sit2 = segSet2.begin(); sit2 != segSet2.end(); ++sit2) {
                            const Segment * seg2 = sit2->segment;
                            if (seg == seg2) continue;
                            timeT start2 = seg2->getStartTime();
                            timeT end2 = start2 + sit2->wholeDuration;

                            // When the two segments have the same bounds,
                            // repeat is possible.
                            if ((start2 == start) && (end2 == end)
                                 && (sit->duration == sit2->duration)) {
                                // Count how many synchronous segments
                                sit->syncCount++;
                                continue;
                            }

                            // If the second segment is starting somewhere inside
                            // the first one, repeat is neither possible for the
                            // first segment nor for the second one.
                            if ((start2 >= start) && (start2 < end)) {
                                sit->synchronous = false;
                                sit2->synchronous = false;
                            }
                        }
                    }
                }
            }
        }
    }


    // Look for linked segments which may be exported as repeat with alternate
    // endings or as simple repeat
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {

        // No LilyPond automatic volta when multiple voices on the same track
        int voiceCount = tit->second.size();
        if (voiceCount > 1) m_automaticVoltaUsable = false;

        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            vit->second.scanForRepeatedLinks();
        }
    }

    // Check linked segment repeat consistency between tracks/voices and
    // mark inconsistent repeats with the noRepeat flag.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                if (sit->repeatId) {
                    const SegmentData * sd;
                    for (sd = getFirstSynchronousSegment(sit->segment);
                                sd; sd = getNextSynchronousSegment()) {
                        if (!sd->repeatId) {
                            sit->noRepeat = true;
                            break;
                        }
                    }
                }
                if (sit->simpleRepeatId) {
                    const SegmentData * sd;
                    for (sd = getFirstSynchronousSegment(sit->segment);
                                sd; sd = getNextSynchronousSegment()) {
                        if (!sd->simpleRepeatId) {
                            sit->noRepeat = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Reset all the repeatId
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                sit->repeatId = 0;
                sit->alt = false;
                sit->ignored = false;
                sit->numberOfAlt = 0;
                sit->simpleRepeatId = 0;
            }
        }
    }

    // Then look again for repeats from linked segments
    // (but without looking at the inconsistent ones)
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            vit->second.scanForRepeatedLinks();
        }
    }


    // On each voice of each track, store the alternate ending of each
    // repeat sequence inside the main segment data
    int currentRepeatId = 0;
    const SegmentData * currentMainSeg = nullptr;
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        // int trackPos = tit->first;
        // int trackId = m_composition->getTrackByPosition(trackPos)->getId();
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            // int voiceIndex = vit->first;

            // First pass: count the repetitions and remember the alternate
            // ending
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                if (sit->repeatId) {
                    if (sit->repeatId != currentRepeatId) {
                        currentRepeatId = sit->repeatId;
                        currentMainSeg = &(*sit);
                        currentMainSeg->numberOfAlt = 1;
                        currentMainSeg->rawAltChain = new AltChain;
                    } else {
                        // Main repeating segment or alternate ending ?
                        if (sit->alt) {
                            // Insert alternate ending in list
                            SegmentData sd = *sit;
                            AlternateEnding * alt = new AlternateEnding(
                                        &(*sit),
                                        currentMainSeg->numberOfAlt);
                            currentMainSeg->rawAltChain->push_back(alt);
                        } else {
                            // Count more one repeat
                            currentMainSeg->numberOfAlt++;
                        }
                    }
                }
            }

            // Second pass: when the repeat count is 1, the repeat sequence is
            // a false one and have to be removed.
            // Note: A repeat count of 1 is probably no more possible with the
            // currently used algorithm.
            // Nevertheless this code is temporarily kept here.
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                if (sit->repeatId) {
                    if (sit->numberOfAlt == 1) {
                        // As numberOfAlt = 1 there is one and only
                        // one alternate endingf
                        AlternateEnding * alt = (*sit->rawAltChain)[0];
                        alt->data->alt = false;
                        alt->data->ignored = false;
                        alt->data->repeatId = 0;
                        delete alt;
                        delete sit->rawAltChain;
                        sit->rawAltChain = nullptr;
                        sit->repeatId = 0;
                        sit->numberOfAlt = 0;
                    }
                }
            }
        }
    }


    // Sort the alternate endings in the print order and gather
    // the duplicate ones

    // Each repeating group of segments has one main repeating segment.
    // First gather the main segments of the synchronous groups of segments
    typedef std::map<timeT, SegmentDataList> RepeatMap;
    RepeatMap repeatMap;
    repeatMap.clear();  // Useful ???
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            for (sit = vit->second.begin(); sit != vit->second.end(); ++sit) {
                if (sit->rawAltChain) {
                    const SegmentData * sd = &(*sit);
                    repeatMap[sit->segment->getStartTime()].push_back(sd);
                }
            }
        }
    }
    // The elements of each SegmentDataList in repeatMap are the data related
    // to a group of synchronous repeated segments.
    // There should be at most one element of each SegmentDataList in each
    // tracks/voices.

    // Now sort and gather the alternate endings in each synchronous group
    // (the grouped alternate endings have to be synchronous)
    for (RepeatMap::iterator i = repeatMap.begin(); i != repeatMap.end(); ++i) {
        sortAndGatherAlt(i->second);
    }


    // Compute the LilyPond start times with all segments unfolded.
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        /* int trackPos = tit->first; */
        /* Track * track = m_composition->getTrackByPosition(trackPos); */
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            // int voiceIndex = vit->first;
            SegmentSet &segSet = vit->second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                const Segment * seg = sit->segment;
                sit->startTime = seg->getStartTime() - m_firstSegmentStartTime;
            }
        }
    }
}

void
LilyPondSegmentsContext::fixRepeatStartTimes()
{
    TrackMap::iterator tit;
    VoiceMap::iterator vit;
    SegmentSet::iterator sit;

    // precompute() should have been already called and
    // we know what segment may be repeated and what segment may be unfolded.
    // We can compute the start time of each segment in the LilyPond score.

    // Sort the repeating segments into start times
    std::map<timeT, const SegmentData *> repeatedSegments;
    repeatedSegments.clear();
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            SegmentSet &segSet = vit->second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                if (sit->numberOfSimpleRepeats
                    || (sit->numberOfRepeats && sit->synchronous)) {
                    repeatedSegments[sit->startTime] = &(*sit);
                }
            }
        }
    }

    // Then fix all the start times
    std::map<timeT, const SegmentData *>::reverse_iterator it;
    for (it=repeatedSegments.rbegin(); it!=repeatedSegments.rend(); ++it) {
        const SegmentData *segData = it->second;
        timeT deltaT = segData->wholeDuration - segData->duration;
        for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
            for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
                SegmentSet &segSet = vit->second;
                for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                    if (sit->startTime > it->first) {
                        sit->startTime -= deltaT;
                    }
                }
            }
        }
        // Fix the end of composition time
        m_lastSegmentEndTime -= deltaT;
    }
}

void
LilyPondSegmentsContext::fixAltStartTimes()
{
    TrackMap::iterator tit;
    VoiceMap::iterator vit;
    SegmentSet::iterator sit;

    // precompute() should have been called already and
    // we know what segment may be repeated and what segment may be unfolded.
    // We can compute the start time of each segment in the LilyPond score.

    // Validate the output of repeat with alternate endings in LilyPond score
    m_repeatWithAlt = true;

    // Sort the volta/alt sequences by start times
    std::map<timeT, const SegmentData *> repeatedSegments;
    repeatedSegments.clear();
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            SegmentSet &segSet = vit->second;
            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                if (sit->numberOfAlt) {
                    repeatedSegments[sit->startTime] = &(*sit);
                }
            }
        }
    }

    // Then fix all the start times
    std::map<timeT, const SegmentData *>::reverse_iterator it;
    for (it=repeatedSegments.rbegin(); it!=repeatedSegments.rend(); ++it) {
        const SegmentData *segData = it->second;

        // Compute the duration error
        timeT duration = segData->duration;
        timeT wholeDuration = duration * segData->numberOfAlt;
        AltChain::iterator vci;
        for (vci = segData->sortedAltChain->begin();
             vci != segData->sortedAltChain->end(); ++vci) {
            wholeDuration += (*vci)->data->duration * (*vci)->altNumber.size();
            duration += (*vci)->data->duration;
        }
        timeT deltaT = wholeDuration - duration;

        // Fix the segment start time when needed
        for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
            for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
                SegmentSet &segSet = vit->second;
                for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                    if (sit->startTime > it->first) {
                        sit->startTime -= deltaT;
                    }
                }
            }
        }
        // Fix the end of composition time
        m_lastSegmentEndTime -= deltaT;
    }
}

Track *
LilyPondSegmentsContext::useFirstTrack()
{
    m_trackIterator = m_segments.begin();
    if (m_trackIterator == m_segments.end()) return nullptr;
    return m_composition->getTrackByPosition((*m_trackIterator).first);
}

Track *
LilyPondSegmentsContext::useNextTrack()
{
    ++m_trackIterator;
    if (m_trackIterator == m_segments.end()) return nullptr;
    return m_composition->getTrackByPosition((*m_trackIterator).first);
}

int
LilyPondSegmentsContext::getTrackPos() const
{
    if (m_trackIterator == m_segments.end()) return -1;
    return (*m_trackIterator).first;
}

int
LilyPondSegmentsContext::useFirstVoice()
{
    int trackPos = getTrackPos();
    if (trackPos == -1) return -1;

    m_voiceIterator = m_trackIterator->second.begin();
    if (m_voiceIterator == m_trackIterator->second.end()) return -1;

    return m_voiceIterator->first;
}

int
LilyPondSegmentsContext::useNextVoice()
{
    if (m_trackIterator == m_segments.end()) return -1;
    ++m_voiceIterator;
    if (m_voiceIterator == m_trackIterator->second.end()) return -1;
    return m_voiceIterator->first;
}

/* unused
int
LilyPondSegmentsContext::getVoiceIndex()
{
    if (m_trackIterator == m_segments.end()) return -1;
    if (m_voiceIterator == m_trackIterator->second.end()) return -1;
    return m_voiceIterator->first;
}
*/

Segment *
LilyPondSegmentsContext::useFirstSegment()
{
    m_lastWasOK = false;
    m_firstAlt = false;
    m_lastAlt = false;
    m_segIterator = m_voiceIterator->second.begin();
    if (m_segIterator == m_voiceIterator->second.end()) return nullptr;
    if (m_repeatWithAlt && m_segIterator->ignored) return useNextSegment();

    m_wasRepeatingWithoutAlt = false;
    m_lastWasOK = true;
    return m_segIterator->segment;
}

Segment *
LilyPondSegmentsContext::useNextSegment()
{
    if (m_lastWasOK) {
        m_wasRepeatingWithoutAlt = isRepeated();
        if (m_repeatWithAlt) {
            // Process possible alternate ending segment
            if (m_segIterator->numberOfAlt) {
                if (!m_currentAltChain) {
                    m_firstAlt = true;
                    m_currentAltChain = m_segIterator->sortedAltChain;
                    m_altIterator = m_currentAltChain->begin();
                    if (m_altIterator != m_currentAltChain->end()) {
                        if (m_currentAltChain->size() == 1) m_lastAlt = true;
                        return (*m_altIterator)->data->segment;
                    }
                } else {
                    m_firstAlt = false;
                    ++m_altIterator;
                    if (m_altIterator != m_currentAltChain->end()) {
                        AltChain::iterator nextIt = m_altIterator;
                        ++nextIt;
                        if (nextIt == m_currentAltChain->end()) {
                            m_lastAlt = true;
                        }
                        return (*m_altIterator)->data->segment;
                    } else {
                        m_lastAlt = false;
                        m_currentAltChain = nullptr;
                    }
                }
            }
        }
    }
    m_lastWasOK = false;

    ++m_segIterator;
    if (m_segIterator == (*m_voiceIterator).second.end()) return nullptr;
    if (m_repeatWithAlt && (*m_segIterator).ignored) return useNextSegment();

    m_lastWasOK = true;
    return m_segIterator->segment;
}

timeT
LilyPondSegmentsContext::getSegmentStartTime() const
{
    return m_segIterator->startTime;
}

int
LilyPondSegmentsContext::getNumberOfVolta() const
{
    if (m_repeatWithAlt && m_segIterator->repeatId) {
        return m_segIterator->numberOfAlt;
    } else if (m_segIterator->simpleRepeatId) {
        return m_segIterator->numberOfSimpleRepeats;
    } else {
        int n = m_segIterator->numberOfRepeats;
        return n ? n : 1;  // A not repeated segment is seen as played once
    }
}

bool
LilyPondSegmentsContext::isRepeated() const
{
    return m_segIterator->numberOfRepeats
          || m_segIterator->numberOfSimpleRepeats;
}

bool
LilyPondSegmentsContext::isRepeatingSegment() const
{
    return (*m_segIterator).numberOfRepeats
           && !(*m_segIterator).simpleRepeatId;
}

bool
LilyPondSegmentsContext::isSimpleRepeatedLinks() const
{
    return (*m_segIterator).numberOfSimpleRepeats
           && (*m_segIterator).simpleRepeatId;
}

bool
LilyPondSegmentsContext::isRepeatWithAlt() const
{
    if (m_repeatWithAlt) {
        return (*m_segIterator).numberOfAlt;
    } else {
        return false;
    }
}

bool
LilyPondSegmentsContext::isSynchronous() const
{
    return (*m_segIterator).synchronous;
}

bool
LilyPondSegmentsContext::isAlt() const
{
    return m_currentAltChain;
}

bool
LilyPondSegmentsContext::isFirstAlt() const
{
    return m_firstAlt;
}

bool
LilyPondSegmentsContext::isLastAlt() const
{
    return m_lastAlt;
}

const std::set<int> *
LilyPondSegmentsContext::getAltNumbers() const
{
    if (!m_currentAltChain) return nullptr;  // Seg. is not an alt. ending
    if (!(*m_segIterator).sortedAltChain) return nullptr;
    if (m_altIterator == (*m_segIterator).sortedAltChain->end()) return nullptr;
    return &(*m_altIterator)->altNumber;
}

std::string
LilyPondSegmentsContext::getAltText()
{
    std::stringstream out;
    std::set<int>::iterator it;
    int last, current;

    if (!m_currentAltChain) return std::string("?");  // Seg. is not an alt. end.
    if (!(*m_segIterator).sortedAltChain) return std::string("");
    if (m_altIterator == (*m_segIterator).sortedAltChain->end())
        return std::string("");

    it = (*m_altIterator)->altNumber.begin();
    last = *it;
    out << last;
    current = last;

    for (++it; it != (*m_altIterator)->altNumber.end(); ++it) {
        if (*it > current + 1) {
            if (current == last) {
                out << ", " << *it;
                last = current = *it;
            } else if (current == (last + 1)) {
                out << ", " << current << ", " << *it;
                last = current = *it;
            } else {
                out << "-" << current << ", " << *it;
                last = current = *it;
            }
        } else {
            current = *it;
        }
    }

    if (current == (last + 1)) {
        out << ", " << current;
    } else if (current > (last + 1)) {
        out << "-" << current;
    }

    return std::string(out.str());
}

int
LilyPondSegmentsContext::getAltRepeatCount()
{
    if (!m_currentAltChain) return -1;  // Current segment is not an alt. ending
    if (!(*m_segIterator).sortedAltChain) return 0;
    if (m_altIterator == (*m_segIterator).sortedAltChain->end()) return 0;
    return (*m_altIterator)->altNumber.size();
}

Rosegarden::Key
LilyPondSegmentsContext::getPreviousKey() const
{
    if (m_currentAltChain) return (*m_altIterator)->data->previousKey;
    return m_segIterator->previousKey;
}

bool
LilyPondSegmentsContext::wasRepeatingWithoutAlt() const
{
    return m_wasRepeatingWithoutAlt;
}

bool
LilyPondSegmentsContext::SegmentDataCmp::operator()(const SegmentData &s1, const SegmentData &s2) const
{
    // Sort segments according to start time, then end time, then address.
    // Copied from StaffHeader::SegmentCmp::operator()
    if (s1.segment->getStartTime() < s2.segment->getStartTime()) return true;
    if (s1.segment->getStartTime() > s2.segment->getStartTime()) return false;
    if (s1.segment->getEndMarkerTime() < s2.segment->getEndMarkerTime())
        return true;
    if (s1.segment->getEndMarkerTime() > s2.segment->getEndMarkerTime())
        return false;
    return (long) s1.segment < (long) s2.segment;
}

const LilyPondSegmentsContext::SegmentData *
LilyPondSegmentsContext::getFirstSynchronousSegment(Segment * seg)
{
    m_GSSSegment = seg;

    m_GSSTrackIterator = m_segments.begin();
    if (m_GSSTrackIterator == m_segments.end()) return nullptr;

    m_GSSVoiceIterator = m_GSSTrackIterator->second.begin();
    if (m_GSSVoiceIterator == m_GSSTrackIterator->second.end()) return nullptr;

    m_GSSSegIterator = m_GSSVoiceIterator->second.begin();
    if (m_GSSSegIterator == m_GSSVoiceIterator->second.end()) return nullptr;

    if (m_GSSSegIterator->synchronous &&
        (m_GSSSegIterator->segment != m_GSSSegment) &&
        (m_GSSSegIterator->segment->getStartTime() == m_GSSSegment->getStartTime()) &&
        (m_GSSSegIterator->segment->getEndMarkerTime() == m_GSSSegment->getEndMarkerTime())) {
            return &(*m_GSSSegIterator);
    }

    return getNextSynchronousSegment();
}

const LilyPondSegmentsContext::SegmentData *
LilyPondSegmentsContext::getNextSynchronousSegment()
{
    for (;;) {
        ++m_GSSSegIterator;
        if (m_GSSSegIterator == m_GSSVoiceIterator->second.end()) {
            ++m_GSSVoiceIterator;
            if (m_GSSVoiceIterator == m_GSSTrackIterator->second.end()) {
                ++m_GSSTrackIterator;
                if (m_GSSTrackIterator == m_segments.end()) return nullptr;
                m_GSSVoiceIterator = m_GSSTrackIterator->second.begin();
            }
            m_GSSSegIterator = m_GSSVoiceIterator->second.begin();
        }

        if (m_GSSSegIterator->synchronous &&
            (m_GSSSegIterator->segment != m_GSSSegment) &&
            (m_GSSSegIterator->segment->getStartTime() == m_GSSSegment->getStartTime()) &&
            (m_GSSSegIterator->segment->getEndMarkerTime() == m_GSSSegment->getEndMarkerTime())) {
                return &(*m_GSSSegIterator);
        }
    }
}

void
LilyPondSegmentsContext::SegmentSet::scanForRepeatedLinks()
{
    SegmentSet::iterator sit;

    // First look for repeats with alternate endings (which have precedence
    // on simple repeats)
    for (sit = begin(); sit != end(); ++sit) {
        setIterators(sit);
        if (isPossibleStartOfRepeatWithAlt()) {
            while (isNextSegmentsOfRepeatWithAlt()) {
                // Nothing to do here
            }

            sit = m_it1; // The last segment known
            m_nextRepeatId++; // One repeat sequence has been found: increment Id

        } else {
            // sit is already the last segment known
        }
    }

    // Then look for simple repeats
    for (sit = begin(); sit != end(); ++sit) {
        setIterators(sit);
        if (isPossibleStartOfSimpleRepeat()) {
            while (isNextSegmentOfSimpleRepeat()) {
                // Nothing to do here
            }

            sit = m_it0; // The last segment known
            m_nextRepeatId++; // One repeat sequence has been found: increment Id

        } else {
            // sit is already the last segment known
        }
    }
}

bool
LilyPondSegmentsContext::SegmentSet::isPossibleStartOfRepeatWithAlt()
{
    // OK if
    // s0 and s2 are valid repeating segs
    // s1 and s3 are valid alternate endings
    //
    // If OK, mark s0, s1, s2 and s3 and return true
    // else keep them unchanged and return false

    // Are still four segments inside iterators window
    if (m_it3 == end()) return false;


    // Is *m_it0 a valid base of a repeating segment with alternate endings ?

    // Such a base is not already registered in a repeat chain
    if (m_it0->repeatId) return false;

    // Such a base can't be a repeating segment
    if (m_it0->numberOfRepeats) return false;

    // Such a base must be a synchronous segment
    if (!m_it0->synchronous) return false;

    // Such a base can't be marked as "no repeat"
    if (m_it0->noRepeat) return false;

    // Such a base must be a plain linked segment
    if (!m_it0->segment->isPlainlyLinked()) return false;


    // Is *m_it2 a repeated *m_it0 ?

    // It must be linked with *m_it0
    if (!m_it2->segment->isLinkedTo(m_it0->segment)) return false;

    // It must have the same "bar offset" as *m_it0
    /// XXXXXXXX  TODO TODO TODO !!!!!!!

    // It should not be already registered in a repeat chain
    if (m_it2->repeatId) return false;

    // It should not be a repeating segment
    if (m_it2->numberOfRepeats) return false;

    // It must be a synchronous segment
    if (!m_it2->synchronous) return false;

    // Repeated segments must have the same number of parallel segments
    if (m_it0->syncCount != m_it2->syncCount) return false;

    // It should not be marked as "no repeat"
    if (m_it2->noRepeat) return false;

    // It must be be a plain linked segment
    if (!m_it2->segment->isPlainlyLinked()) return false;

    // It can't be separated from the previous alternate ending neither
    // overlap it
    if (    m_it2->segment->getStartTime()
         != m_it1->segment->getEndMarkerTime()) return false;


    // Is *m_it1 a valid alternate ending ?

    // A valid alternate ending is not repeating
    if (m_it1->numberOfRepeats) return false;

    // A valid alternate ending can't be the repeated segment
    if (m_it1->segment->isLinkedTo(m_it0->segment)) return false;

    // A valid alternate ending can't be separated from the repeated
    // segment neither overlap it
    if (    m_it1->segment->getStartTime()
         != m_it0->segment->getEndMarkerTime()) return false;

    // An alternate ending which is not the last one must be synchronous
    if (!m_it1->synchronous) return false;

    // An alternate ending which is not the last one must have the same number
    // of parallel segments has the main repeated segment
    if (m_it1->syncCount != m_it0->syncCount) return false;


    // Is *m_it3 a valid alternate ending ?

    // A valid alternate ending is not repeating
    if (m_it3->numberOfRepeats) return false;

    // A valid alternate ending can't be the repeated segment
    if (m_it3->segment->isLinkedTo(m_it0->segment)) return false;

    // A valid alternate ending can't be separated from the repeated
    // segment neither overlap it
    if (    m_it3->segment->getStartTime()
         != m_it2->segment->getEndMarkerTime()) return false;


    // All test succeeded

    // Mark the segments as repeat with alternate ending
    m_it0->repeatId = m_nextRepeatId;
    m_it1->repeatId = m_nextRepeatId;
    m_it1->alt = true;
    m_it1->ignored = true;
    m_it2->repeatId = m_nextRepeatId;
    m_it2->ignored = true;
    m_it3->repeatId = m_nextRepeatId;
    m_it3->alt = true;
    m_it3->ignored = true;

    // set iterators for the isNextSegmentsOfRepeatWithAlt() step
    setIterators(m_it2);

    return true;
}

bool
LilyPondSegmentsContext::SegmentSet::isNextSegmentsOfRepeatWithAlt()
{
    // s0 and s1 are the last found repeated and alt. ending segs
    //
    // OK if
    // s2 and s3 are the next possible repeated and alt. ending segs
    //
    // If OK, mark s2 and s3 and return true
    // else keep them unchanged and return false

    // Are still two segments inside iterators window
    if (m_it3 == end()) return false;


    // Is the previous alternate ending valid as an intermediary one ?

    // It must be synchronous
    if (!m_it1->synchronous) return false;

    // It must have the right number of parallel segments
    if (m_it1->syncCount != m_it0-> syncCount) return false;


    // Is *m_it2 a repeated *m_it0 ?

    // It must be linked with *m_it0
    if (!m_it2->segment->isLinkedTo(m_it0->segment)) return false;

    // It must have the same "bar offset" as *m_it0
    /// XXXXXXXX  TODO TODO TODO !!!!!!!

    // It should not be already registered in a repeat chain
    if (m_it2->repeatId) return false;

    // It should not be a repeating segment
    if (m_it2->numberOfRepeats) return false;

    // It must be a synchronous segment
    if (!m_it2->synchronous) return false;

    // It must have the right number of parallel segments
    if (m_it2->syncCount != m_it0-> syncCount) return false;

    // It should not be marked as "no repeat"
    if (m_it2->noRepeat) return false;

    // It must be be a plain linked segment
    if (!m_it0->segment->isPlainlyLinked()) return false;

    // It can't be separated from the previous alternate ending
    // neither overlap it
    if (    m_it2->segment->getStartTime()
         != m_it1->segment->getEndMarkerTime()) return false;

    // Is *m_sit3 a valid alternate ending ?

    // A valid alternate ending is not repeating
    if (m_it3->numberOfRepeats) return false;

    // A valid alternate ending can't be the repeated segment
    if (m_it3->segment->isLinkedTo(m_it0->segment)) return false;

    // A valid alternate ending can't be separated from the repeated
    // segment neither overlap it
    if (    m_it3->segment->getStartTime()
         != m_it2->segment->getEndMarkerTime()) return false;


    // All test succeeded

    // Mark the segments as repeat with alternate endings
    m_it2->repeatId = m_nextRepeatId;
    m_it2->ignored = true;
    m_it3->repeatId = m_nextRepeatId;
    m_it3->alt = true;
    m_it3->ignored = true;

    // set iterators for the isNextSegmentsOfRepeatWithAlt() step
    setIterators(m_it2);

    return true;
}


bool
LilyPondSegmentsContext::SegmentSet::isPossibleStartOfSimpleRepeat()
{
    // OK if
    // s0 and s1 are valid repeating segs
    //
    // If OK, mark s0 and s1 and return true
    // else keep s0 and s1 unchanged and return false

    // Are still two segments inside iterators window
    if (m_it1 == end()) return false;


    // Is *m_it0 a valid base of a simple repeating segment ?

    // Such a base is not already registered in a repeat chain
    if (m_it0->repeatId) return false;

    // Such a base can't be a repeating segment
    if (m_it0->numberOfRepeats) return false;

    // Such a base must be a synchronous segment
    if (!m_it0->synchronous) return false;

    // Such a base can't be marked as "no repeat"
    if (m_it0->noRepeat) return false;

    // Such a base must be a plain linked segment
    if (!m_it0->segment->isPlainlyLinked()) return false;

    // A segment part of a repeat with alternate endings chain can't be
    // a simple repeated segment (precedence to repeat with alt. endings)
    if (m_it0->repeatId) return false;

    // Is *m_it1 a repeated *m_it0 ?

    // It must be linked with *m_it0
    if (!m_it1->segment->isLinkedTo(m_it0->segment)) return false;

    // It must have the same "bar offset" as *m_it0
    /// XXXXXXXX  TODO TODO TODO !!!!!!!

    // It should not be already registered in a repeat chain
    if (m_it1->repeatId) return false;

    // It should not be a repeating segment
    if (m_it1->numberOfRepeats) return false;

    // It must be a synchronous segment
    if (!m_it1->synchronous) return false;

    // It must have the same number of parallel segments as *m_it0
    if (m_it1->syncCount != m_it0->syncCount) return false;

    // It should not be marked as "no repeat"
    if (m_it1->noRepeat) return false;

    // It must be be a plain linked segment
    if (!m_it1->segment->isPlainlyLinked()) return false;

    // It can't be separated from the previous segment neither overlap it
    if (    m_it1->segment->getStartTime()
         != m_it0->segment->getEndMarkerTime()) return false;

    // A segment part of a repeat with alternate endings chain can't be a
    // simple repeated segment (precedence to repeat with alternate endings)
    if (m_it1->repeatId) return false;

    // All test succeeded

    // Mark the segments as repeat without alternate ending
    m_it0->simpleRepeatId = m_nextRepeatId;
    m_it0->numberOfSimpleRepeats = 2;
    m_it0->wholeDuration = m_it0->duration * 2;
    m_start = m_it0;
    m_it1->simpleRepeatId = m_nextRepeatId;
    m_it1->ignored = true;

    // set iterators for the isNextSegmentsOfSimpleRepeat() step
    setIterators(m_it1);

    return true;
}

bool
LilyPondSegmentsContext::SegmentSet::isNextSegmentOfSimpleRepeat()
{
    // s0 is the last found simple repeated seg
    //
    // OK if
    // s1 is repeating s0
    //
    // If OK, mark s1 and return true
    // else keep it unchanged and return false

    // Are still two segments inside iterators window
    if (m_it1 == end()) return false;


    // Is *m_it1 a repeated *m_it0 ?

    // It must be linked with *m_it0
    if (!m_it1->segment->isLinkedTo(m_it0->segment)) return false;

    // It must have the same "bar offset" as *m_it0
    /// XXXXXXXX  TODO TODO TODO !!!!!!!

    // It should not be already registered in a repeat chain
    if (m_it1->repeatId) return false;

    // It should not be a repeating segment
    if (m_it1->numberOfRepeats) return false;

    // It must be a synchronous segment
    if (!m_it1->synchronous) return false;

    // It must have the same number of parallel segments as *m_it0
    if (m_it1->syncCount != m_it0->syncCount) return false;

    // It should not be marked as "no repeat"
    if (m_it1->noRepeat) return false;

    // It must be be a plain linked segment
    if (!m_it1->segment->isPlainlyLinked()) return false;

    // It can't be separated from the previous segment neither overlap it
    if (    m_it1->segment->getStartTime()
         != m_it0->segment->getEndMarkerTime()) return false;

    // A segment part of a repeat with alternate endings chain can't be a
    // simple repeated segment (precedence to repeat with alternate endings)
    if (m_it1->repeatId) return false;

    // All test succeeded

    // Mark the segment as repeat without alternate ending
    m_it1->simpleRepeatId = m_nextRepeatId;
    m_it1->ignored = true;

    // Update data of the first segment
    m_start->numberOfSimpleRepeats++;
    m_start->wholeDuration += m_start->duration;

    // set iterators for the next isNextSegmentsOfSimpleRepeat() step
    setIterators(m_it1);

    return true;
}

void
LilyPondSegmentsContext::sortAndGatherAlt(SegmentDataList & repeatList)
{
    int idx;
    SegmentDataList::iterator it;
    SegmentDataList::iterator it1 = repeatList.begin();
    if (it1 == repeatList.end()) return;   // This should not happen

    // Initialize the sorted alternate endings chains with the first
    // raw alternate ending
    for (it = repeatList.begin(); it != repeatList.end(); ++it) {
        (*it)->sortedAltChain = new AltChain;
        if ((*it)->rawAltChain) {
            (*it)->sortedAltChain->push_back((*(*it)->rawAltChain)[0]);
        } else {
            // DON'T CRASH...
            std::cerr << "###############################"
                      << "############################################\n";
            std::cerr << "LilyPondSegmentsContext::sortAndGatherAlt:"
                      << " rawAltChain = 0 : THIS IS A BUG\n";
            std::cerr << "###############################"
                      << "############################################\n";
            return;
        }
    }

    // Add the following alternate endings
    for (idx = 1; idx < (*it1)->numberOfAlt; idx++) {
        // Is the alternate ending indexed by idx similar to a previous one ?
        bool found = false;
        int idx2;
        for (idx2 = 0; idx2 < (int)(*it1)->sortedAltChain->size(); idx2++) {
            bool linked = true;
            for (it = repeatList.begin(); it != repeatList.end(); ++it) {
                const Segment * seg1 =
                    (*(*it)->rawAltChain)[idx]->data->segment;
                Segment * seg2 = (*(*it)->sortedAltChain)[idx2]->data->segment;
                if (!seg1->isPlainlyLinkedTo(seg2)) {
                    linked = false;
                    break;
                }
            }
            if (linked) {
                found = true;
                break;
            }
        }
        if (found) {
            // Add new alternate ending number in existing alternate ending
            for (it = repeatList.begin(); it != repeatList.end(); ++it) {
                (*(*it)->sortedAltChain)[idx2]->altNumber.insert(idx + 1);
            }
            // Automatic volta in LilyPond is not possible when an alternate
            // ending other than the first one is played several time.
            // (i.e. A B B : "1", "2-3")
            if (idx2 != 0) m_automaticVoltaUsable = false;
        } else {
            // Add one more alternate ending
            for (it = repeatList.begin(); it != repeatList.end(); ++it) {
                (*it)->sortedAltChain->push_back((*(*it)->rawAltChain)[idx]);
            }
        }
    }

    // Previous code sets m_automaticVoltaUsable to false when the alternate
    // endings sequence is "A B B" or "A A B B", but fails when sequence is
    // "A A B A".
    // Look again at the alternate endings and set m_automaticVoltaUsable
    // as needed.
    for (it = repeatList.begin(); it != repeatList.end(); ++it) {
        if ((*it)->sortedAltChain) {
            int altIndex = 0;
            for (AltChain::iterator ia = (*it)->sortedAltChain->begin();
                                ia != (*it)->sortedAltChain->end(); ++ia) {
                altIndex++;
                // Iterator to first number
                std::set<int>::iterator is = (*ia)->altNumber.begin();
                // Iterator to last number
                std::set<int>::reverse_iterator ie = (*ia)->altNumber.rbegin();
                int size = (*ia)->altNumber.size();
                // The first alternate ending should start at number 1
                if ((altIndex == 1) && (*is != 1)) m_automaticVoltaUsable = false;
                // The numbers of an alternate ending should de continuous
                if ((*ie - *is) != (size - 1)) m_automaticVoltaUsable = false;
            }
        }
    }
}

Segment *
LilyPondSegmentsContext::getArbitrarySegment(int trackPos)
{
    return m_segments[trackPos][0].begin()->segment;
}


void
LilyPondSegmentsContext::dump()
{

    TrackMap::iterator tit;
    VoiceMap::iterator vit;
    SegmentSet::iterator sit;

    std::cout << std::endl;
    for (tit = m_segments.begin(); tit != m_segments.end(); ++tit) {
        int trackPos = (*tit).first;
        Track * track = m_composition->getTrackByPosition(trackPos);
        std::cout << "Track pos=" << trackPos << " id=" << track->getId()
            << "   \"" << track->getLabel() << "\"" << std::endl;

        for (vit = tit->second.begin(); vit != tit->second.end(); ++vit) {
            std::cout << "  Voice index = " << vit->first << std::endl;
            SegmentSet &segSet = vit->second;

            for (sit = segSet.begin(); sit != segSet.end(); ++sit) {
                const Segment * seg = (*sit).segment;

                std::cout << "     Segment \"" << seg->getLabel() << "\""
                    << " voice=" << m_composition->getSegmentVoiceIndex(seg)
                    << " start=" << seg->getStartTime()
                    << " duration=" << (*sit).duration
                    << " wholeDuration=" <<  (*sit).wholeDuration
                    << " previousKey = " << (*sit).previousKey.getName()
                    << std::endl;
                std::cout << "               numRepeat=" << (*sit).numberOfRepeats
                    << " remainder=" << (*sit).remainderDuration
                    << " synchronous=" << (*sit).synchronous
                    << " (" << (*sit).syncCount << ")"
                    << " lilyStart=" << (*sit).startTime
                    << std::endl;
                std::cout << "               noRepeat=" << (*sit).noRepeat
                    << " repeatId=" << (*sit).repeatId
                    << " numberOfAlt=" << (*sit).numberOfAlt
                    << " rawAltChain=" << (*sit).rawAltChain
                    << std::endl;
                if (sit->rawAltChain) {
                    AltChain::iterator i;
                    for (i = sit->rawAltChain->begin(); i != sit->rawAltChain->end(); ++i) {
                        std::cout << "                 --> \"" << (*i)->data->segment->getLabel()
                            << "\": ";
                        std::set<int>::iterator j;
                        for (j = (*i)->altNumber.begin(); j != (*i)->altNumber.end(); ++j) {
                            std::cout << (*j) << " ";
                        }
                        std::cout << "\n";
                    }
                }
                std::cout << "               sortedAltChain=" << (*sit).sortedAltChain
                        << std::endl;
                if (sit->sortedAltChain) {
                    AltChain::iterator i;
                    for (i = sit->sortedAltChain->begin(); i != sit->sortedAltChain->end(); ++i) {
                        std::cout << "                 --> \"" << (*i)->data->segment->getLabel()
                                << "\"  [" << (*i)->data->previousKey.getName()
                                << "] : ";
                        std::set<int>::iterator j;
                        for (j = (*i)->altNumber.begin(); j != (*i)->altNumber.end(); ++j) {
                            std::cout << (*j) << " ";
                        }
                        std::cout << "\n";
                    }
                }
                std::cout << "               ignored=" << (*sit).ignored
                          << " simpleRepeatId="
                          << (*sit).simpleRepeatId << std::endl;
            }
        }
    }
    std::cout << std::endl;
}





void
LilyPondSegmentsContext::SegmentSet::setIterators(iterator it)
{
    m_it0 = it;

    m_it1 = m_it0;
    if (m_it1 != end()) ++m_it1;

    m_it2 = m_it1;
    if (m_it2 != end()) ++m_it2;

    m_it3 = m_it2;
    if (m_it3 != end()) ++m_it3;

    m_it4 = m_it3;
    if (m_it3 != end()) ++m_it4;
}



void
LilyPondSegmentsContext::SegmentDataList::dump()
{
    iterator it;
    std::cout << "------->\n";
    for (it = begin(); it != end(); ++it) {
        std::cout << " \"" << (*it)->segment->getLabel() << "\"" << std::endl;

        if ((*it)->rawAltChain) {
            std::cout << "raw:" << std::endl;
            AltChain::iterator ivc;
            for (ivc=(*it)->rawAltChain->begin();
                     ivc!=(*it)->rawAltChain->end(); ++ivc) {
                std::cout << "   \"" << (*ivc)->data->segment->getLabel() << "\" :";
                for (std::set<int>::iterator u=(*ivc)->altNumber.begin();
                        u!=(*ivc)->altNumber.end(); ++u) {
                    std::cout << " " << (*u);
                }
            }
        }

        if ((*it)->sortedAltChain) {
            std::cout << std::endl << "sorted:" << std::endl;
            AltChain::iterator ivc;
            for (ivc=(*it)->sortedAltChain->begin();
                     ivc!=(*it)->sortedAltChain->end(); ++ivc) {
                std::cout << "   \"" << (*ivc)->data->segment->getLabel() << "\" :";
                for (std::set<int>::iterator u=(*ivc)->altNumber.begin();
                        u!=(*ivc)->altNumber.end(); ++u) {
                    std::cout << " " << (*u);
                }
            }
        }
    }
    std::cout << std::endl << "<--------" << std::endl;
}

}
