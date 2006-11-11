// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
struct LevelInfo
{
    int level;
    int levelRight; // if stereo audio
};

class MappedComposition;


#define SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS 512 // can't be a symbol
#define SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS   64 // can't be a symbol
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

    bool getTrackLevel(TrackId track, LevelInfo &) const;
    void setTrackLevel(TrackId track, const LevelInfo &);

    // Two of these to rather hamfistedly get around the fact
    // we need to fetch this value twice - once from IPB, 
    // and again for the Mixer.
    //
    bool getInstrumentLevel(InstrumentId id, LevelInfo &) const;
    bool getInstrumentLevelForMixer(InstrumentId id, LevelInfo &) const;

    void setInstrumentLevel(InstrumentId id, const LevelInfo &);

    bool getInstrumentRecordLevel(InstrumentId id, LevelInfo &) const;
    bool getInstrumentRecordLevelForMixer(InstrumentId id, LevelInfo &) const;

    void setInstrumentRecordLevel(InstrumentId id, const LevelInfo &);

    bool getSubmasterLevel(int submaster, LevelInfo &) const;
    void setSubmasterLevel(int submaster, const LevelInfo &);

    bool getMasterLevel(LevelInfo &) const;
    void setMasterLevel(const LevelInfo &);

    void setControlBlock(ControlBlock *cb) { m_controlBlock = cb; }
    ControlBlock *getControlBlock() { return m_controlBlock; }

    // Reset the temporaries on (for example) GUI restart
    //
    void clearTemporaries();
    
protected:
    int instrumentToIndex(InstrumentId id) const;
    int instrumentToIndexCreating(InstrumentId id);
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

    InstrumentId m_knownInstruments[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    int m_knownInstrumentCount;

    int m_levelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_levels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int m_recordLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_recordLevels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int m_submasterLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];
    LevelInfo m_submasterLevels[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];

    int m_masterLevelUpdateIndex;
    LevelInfo m_masterLevel;
};

}

#endif
