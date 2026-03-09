/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEQUENCERDATABLOCK_H
#define RG_SEQUENCERDATABLOCK_H

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
 * (RosegardenMainWindow::processRecordedEvents()).  It is an important
 * link in the chain from AlsaDriver::getMappedEventList() to
 * RosegardenDocument::insertRecordedMidi().
 *
 * This class needs to be reviewed for thread safety.  See the comments
 * in addRecordedEvents().
 *
 * This used to be mapped into a shared memory
 * backed file, which had to be of fixed size and layout.  The design
 * reflects that history, though nowadays it is a simple singleton
 * class.
 *
 * The position is still represented as two ints (m_positionSec and
 * m_positionNsec) rather than a RealTime, as a leftover from that history.
 *
 * RosegardenMainWindow::slotHandleInputs() supports communication between
 * the Sequencer and GUI threads using RosegardenSequencer::m_transportRequests.
 *
 * AlsaDriver::handleTransportCCs() talks across threads to
 * RosegardenMainWindow::customEvent() using QCoreApplication::postEvent().
 *
 * @see ControlBlock
 */
class SequencerDataBlock
{
public:
    // Singleton.
    static SequencerDataBlock *getInstance();

    /// Called by the UI.
    RealTime getPositionPointer() const;

    /// Called by the sequencer.
    void setPositionPointer(const RealTime &rt);

    /// Get the MIDI OUT event to show on the transport during playback.
    bool getVisual(MappedEvent &ev);
    /// Set the MIDI OUT event to show on the transport during playback.
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
    int getRecordedEvents(MappedEventList &);

    // unused bool getTrackLevel(TrackId track, LevelInfo &) const;
    // unused void setTrackLevel(TrackId track, const LevelInfo &);

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

    // Reset this class on (for example) GUI restart
    // rename: reset()
    void clearTemporaries();

protected:
    SequencerDataBlock();

    int instrumentToIndex(InstrumentId id) const;
    int instrumentToIndexCreating(InstrumentId id);

    // ??? Thread-safe?  Probably not.  Seems like the worst-case is that
    //     the pointer might jump forward about one second momentarily.
    int m_positionSec;
    int m_positionNsec;

    int m_setVisualIndex;
    int m_getVisualIndex;
    bool m_haveVisualEvent;
    /// MIDI OUT event for display on the transport during playback.
    MappedEvent m_visualEvent{};

    /// Index of the next available position in m_recordBuffer.
    /**
     * volatile is needed here (and probably other places) since this is used
     * across threads.  volatile prevents compiler optimizations (caching)
     * that might render the data useless.  The chances that the compiler
     * might optimize in this fashion are very slim given the code that uses
     * this variable.  However, better safe than sorry.
     */
    volatile int m_recordEventIndex;
    /// Read position in m_recordBuffer.
    /**
     * It's iffy as to whether "volatile" is actually needed here.  The two
     * functions that use this may or may not be on different threads.
     */
    int m_readIndex;
    /// Ring buffer of recorded MIDI events.
    MappedEvent m_recordBuffer[SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE]{};

    // ??? Thread-safe?
    InstrumentId m_knownInstruments[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    int m_knownInstrumentCount;

    // ??? Thread-safe?
    int m_levelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_levels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    // ??? Thread-safe?
    int m_recordLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];
    LevelInfo m_recordLevels[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    // ??? Thread-safe?
    int m_submasterLevelUpdateIndices[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];
    LevelInfo m_submasterLevels[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];

    // ??? Thread-safe?
    int m_masterLevelUpdateIndex;
    LevelInfo m_masterLevel;

private:
    mutable QMutex m_mutex;

};


}

#endif
