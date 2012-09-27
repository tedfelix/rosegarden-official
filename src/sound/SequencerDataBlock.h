/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEQUENCER_DATA_BLOCK_H
#define RG_SEQUENCER_DATA_BLOCK_H

#include "ControlBlock.h"
#include "base/RealTime.h"
#include "MappedEvent.h"

#include <QMutex>

namespace Rosegarden
{
        
/**
 * ONLY PUT PLAIN DATA HERE - NO POINTERS EVER
 * (and this struct mustn't have a constructor)
 *
 * Since we no longer use shared memory, it might be safe to lift
 * the POD/no pointer/no ctor restrictions.
 */
struct LevelInfo
{
    int level;
    int levelRight; // if stereo audio
};

class MappedEventList;


#define SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS 512 // can't be a symbol
#define SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS   64 // can't be a symbol
#define SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE 1024 // MIDI events

/// Holds MIDI data going from RosegardenSequencer to RosegardenMainWindow
/**
 * This class contains recorded data that is being passed from sequencer
 * threads (RosegardenSequencer::processRecordedMidi()) to GUI threads
 * (RosegardenMainWindow::processRecordedEvents()).
 *
 * This class needs to be reviewed for thread safety.  A coarse-grained
 * lock has been added (m_recordLock) for MIDI events.  Other parts of
 * this class probably need locks as well.
 *
 * This used to be mapped into a shared memory
 * backed file, which had to be of fixed size and layout.  The design
 * reflects that history, though nowadays it is a simple singleton
 * class.
 *
 * Examples of some of the shared memory related design decisions:
 * Position is represented as two ints (m_positionSec and m_positionNsec)
 * rather than a RealTime, as the RealTime default ctor
 * initialises the space & so can't be used from the GUI's
 * placement-new ctor (which has no write access and doesn't want
 * it anyway).  Likewise we use char[] instead of MappedEvents
 * for m_visualEvent and m_recordBuffer.
 *
 * Since shared memory is no longer used,
 * it should be possible to change this from being a fixed-layout
 * C-style struct to something more "C++" (e.g. std::vector<MappedEvent>
 * instead of char[sizeof(MappedEvent*CAPACITY_MAX)], RealTime instead of
 * ints, etc...).  We would need to investigate each of the users to make
 * sure things like placement new were replaced with conventional new.
 * From a maintenance standpoint, this should improve the code significantly.
 */
class SequencerDataBlock
{
public:
    // Singleton.
    static SequencerDataBlock *getInstance();

    RealTime getPositionPointer() const {
        return RealTime(m_positionSec, m_positionNsec);
    }
    void setPositionPointer(const RealTime &rt) {
        m_positionSec = rt.sec;
        m_positionNsec = rt.nsec;
    }
    
    bool getVisual(MappedEvent &ev) const;
    void setVisual(const MappedEvent *ev);

    /// Add events to the record ring buffer (m_recordBuffer).
    /**
     * Called by RosegardenSequencer::processRecordedMidi().
     */
    void addRecordedEvents(MappedEventList *);
    /// Get events from the record ring buffer (m_recordBuffer).
    /**
     * Called by RosegardenMainWindow::processRecordedEvents().
     */
    int getRecordedEvents(MappedEventList &) const;

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

    // Reset the temporaries on (for example) GUI restart
    //
    void clearTemporaries();
    
protected:
    SequencerDataBlock();

    int instrumentToIndex(InstrumentId id) const;
    int instrumentToIndexCreating(InstrumentId id);

    int m_positionSec;
    int m_positionNsec;

    int m_visualEventIndex;
    bool m_haveVisualEvent;
    char m_visualEvent[sizeof(MappedEvent)];
    
    /// Index of the next available position in m_recordBuffer.
    int m_recordEventIndex;
    /// Ring buffer of recorded MIDI events.
    char m_recordBuffer[sizeof(MappedEvent) *
                        SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE];
    mutable QMutex m_recordMutex;

    // ??? Mutex?
    InstrumentId m_knownInstruments[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    int m_knownInstrumentCount;

    // ??? Mutex?
    int m_levelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_levels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    // ??? Mutex?
    int m_recordLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_recordLevels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    // ??? Mutex?
    int m_submasterLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];
    LevelInfo m_submasterLevels[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];

    // ??? Mutex?
    int m_masterLevelUpdateIndex;
    LevelInfo m_masterLevel;
};

}

#endif
