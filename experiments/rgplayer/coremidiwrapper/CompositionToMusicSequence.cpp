/*
 *  CompositionToMusicSequence.cpp
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "MusicTrackW.h"
#include "../base/SegmentPerformanceHelper.h"
#include "../base/BaseProperties.h"
#include "../base/Instrument.h"

#include "CompositionToMusicSequence.h"

using Rosegarden::timeT;

MusicSequenceW CompositionToMusicSequence::convertComposition(const Rosegarden::Composition& composition) {

    m_composition = &composition;
    
    Rosegarden::Composition::iterator segIter = composition.begin();
    
    for(; segIter != composition.end(); ++segIter) {
        Rosegarden::Segment* segment = *segIter;
        if (segment->getType() == Rosegarden::Segment::Internal) {
            convertInternalSegment(*segment);
        }
    }
	
	 return m_musicSequence; 
}

unsigned int CompositionToMusicSequence::getSegmentRepeatCount(const Rosegarden::Segment& segment)
{
    int repeatCount = 0;
    
    timeT segmentStartTime = segment.getStartTime();
    timeT segmentEndTime = segment.getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;
    
    if (segment.isRepeating() && segmentDuration > 0) {
        repeatEndTime = segment.getRepeatEndTime();
        repeatCount = 1 + (repeatEndTime - segmentEndTime) / segmentDuration;
    }
    
    return repeatCount;
}


void CompositionToMusicSequence::convertInternalSegment(const Rosegarden::Segment& segment) {
    m_currentTrack = getOrCreateTrack(segment.getTrack());
 
    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;

    timeT segmentStartTime = segment.getStartTime();
    timeT segmentEndTime = segment.getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;
    
    int repeatCount = getSegmentRepeatCount(segment);
    
    if (repeatCount > 0)
        repeatEndTime = segment.getRepeatEndTime();    
    
    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {
        
        for(Rosegarden::Segment::const_iterator it = segment.begin(); it != segment.end(); ++it) {
            Rosegarden::Event* ev = *it; 
            if (!ev->isa(Rosegarden::Note::EventRestType)) {
                
                Rosegarden::SegmentPerformanceHelper helper(const_cast<Rosegarden::Segment&>(segment));
                
                timeT playTime =
                helper.getSoundingAbsoluteTime(it) + repeatNo * segmentDuration;
                if (playTime >= repeatEndTime)
                    break;
                
                timeT playDuration = helper.getSoundingDuration(it);
                
                // Ignore notes without duration -- they're probably in a tied
                // series but not as first note
                //
                if (playDuration > 0 || !(ev)->isa(Rosegarden::Note::EventType)) {
                    
                    if (playTime + playDuration > repeatEndTime)
                        playDuration = repeatEndTime - playTime;
                    
                    playTime = playTime + segment.getDelay();
                    eventTime = m_composition->getElapsedRealTime(playTime);
                    
                    convertEvent(ev, playDuration, playTime);                    
                    
                    // slightly quicker than calling helper.getRealSoundingDuration()
//                    duration =
//                    m_composition->getElapsedRealTime(playTime + playDuration) - eventTime;
//                    
//                    eventTime = eventTime + segment.getRealTimeDelay();
                }
            }
        }
    }
}

void CompositionToMusicSequence::convertEvent(const Rosegarden::Event* event, timeT playDuration, timeT playTime) {

    if (event->isa(Rosegarden::Note::EventType)) {
        MIDINoteMessage msg;
        msg.duration = playDuration / 100.0;
        long tmp = Rosegarden::MidiMaxValue;
        bool hasProperty = event->get<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY, tmp);
        msg.velocity = tmp;        
        hasProperty = event->get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, tmp);
        msg.note = tmp;
        
        MusicTimeStamp timeStamp = playTime / 100.0;
        m_currentTrack.newMIDINoteEvent(timeStamp, &msg);
    }
    
}

MusicTrackW CompositionToMusicSequence::getOrCreateTrack(UInt32 trackIndex) {

    // create tracks if needed
    //
    while (m_musicSequence.getTrackCount() <= trackIndex) {
        m_musicSequence.newTrack();
    }

    return m_musicSequence.getIndTrack(trackIndex);

}