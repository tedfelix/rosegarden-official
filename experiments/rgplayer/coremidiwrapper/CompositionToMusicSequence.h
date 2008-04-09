/*
 *  CompositionToMusicSequence.h
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/16/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "MusicPlayerW.h"
#include "MusicTrackW.h"

#include "../base/Composition.h"

class CompositionToMusicSequence {
public:
    MusicSequenceW convertComposition(const Rosegarden::Composition&);


protected:
    unsigned int getSegmentRepeatCount(const Rosegarden::Segment&);
    void convertInternalSegment(const Rosegarden::Segment&);
    void convertEvent(const Rosegarden::Event* event, Rosegarden::timeT playDuration, Rosegarden::timeT playTime);
    
	MusicTrackW getOrCreateTrack(UInt32 trackIndex);
    
    
    MusicSequenceW m_musicSequence;
    MusicTrackW m_currentTrack;
    
    const Rosegarden::Composition* m_composition;
};