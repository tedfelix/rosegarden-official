// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _SEQUENCER_DATA_BLOCK_H_
#define _SEQUENCER_DATA_BLOCK_H_

#include "ControlBlock.h"
#include "RealTime.h"
#include "MappedEvent.h"

namespace Rosegarden
{
	
/**
 * ONLY PUT PLAIN DATA HERE - NO POINTERS EVER
 * (and this struct mustn't have a constructor)
 */
struct TrackLevelInfo
{
    int level;
    int levelRight; // if stereo audio
};

class MappedComposition;


#define SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE 1024 // MIDI events

class SequencerDataBlock
{
public:
    /**
     * Constructor only initialises memory if initialise is true
     */
    SequencerDataBlock(bool initialise);

    RealTime getPositionPointer() const {
	return RealTime(m_positionSec, m_positionNsec);
    }
    void setPositionPointer(const RealTime &rt) {
	m_positionSec = rt.sec;
	m_positionNsec = rt.nsec;
    }
    
    bool getVisual(MappedEvent &ev) const;
    void setVisual(const MappedEvent *ev);

    int getRecordedEvents(MappedComposition &) const;
    void addRecordedEvents(MappedComposition *);

    bool getRecordLevel(TrackLevelInfo &) const;
    void setRecordLevel(const TrackLevelInfo &);

    bool getTrackLevel(TrackId track, TrackLevelInfo &) const;
    void setTrackLevel(TrackId track, const TrackLevelInfo &);

    void setTrackLevelsForInstrument(InstrumentId instrument,
				     const TrackLevelInfo &);

    void setControlBlock(ControlBlock *cb) { m_controlBlock = cb; }
    
protected:
    ControlBlock *m_controlBlock;

    // Two ints rather than a RealTime, as the RealTime default ctor
    // initialises the space & so can't be used from the GUI's
    // placement-new ctor (which has no write access and doesn't want
    // it anyway).  Likewise we use char[] instead of MappedEvents

    int m_positionSec;
    int m_positionNsec;

    int m_visualEventIndex;
    bool m_haveVisualEvent;
    char m_visualEvent[sizeof(MappedEvent)];
    
    int m_recordEventIndex;
    char m_recordBuffer[sizeof(MappedEvent) *
			SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE];

    int m_recordLevelUpdateIndex;
    TrackLevelInfo m_recordLevel;

    int m_trackLevelUpdateIndices[CONTROLBLOCK_MAX_NB_TRACKS];
    TrackLevelInfo m_trackLevels[CONTROLBLOCK_MAX_NB_TRACKS];
};

}

#endif
