/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SequencerDataBlock.h"
#include "MappedEventList.h"

//#include "misc/Debug.h"

//#include <QThread>
#include <QMutexLocker>

namespace Rosegarden
{

SequencerDataBlock *
SequencerDataBlock::getInstance()
{
    static SequencerDataBlock *instance = 0;
    if (!instance) instance = new SequencerDataBlock();
    return instance;
}

SequencerDataBlock::SequencerDataBlock()
{
    clearTemporaries();
}

bool
SequencerDataBlock::getVisual(MappedEvent &ev)
{
    // If there is no visual event, or setVisual() is working, bail
    if (!m_haveVisualEvent)
        return false;

    // Get the index in case setVisual() changes it.
    int thisEventIndex = m_setVisualIndex;

    // If we've already seen this one, bail.  This prevents reading
    // before setVisual() is finished updating.
    if (thisEventIndex == m_getVisualIndex)
        return false;

    // ??? A call to setVisual() could happen at this point and modify
    //     m_visualEvent while we are reading it.  This isn't as safe as
    //     it appears.  I think we need a mutex for this.  Though since
    //     it is just the MIDI OUT display on playback, it's probably not
    //     worth worrying about.

    // Copy the event to the caller.
    ev = *((MappedEvent *) & m_visualEvent);

    // Remember where we were for next time.
    m_getVisualIndex = thisEventIndex;

    return true;
}

void
SequencerDataBlock::setVisual(const MappedEvent *ev)
{
    // Prevent access by getVisual() while we are changing this.
    m_haveVisualEvent = false;

    if (ev) {
        // Save the visual event
        *((MappedEvent *)&m_visualEvent) = *ev;

        // Indicate that it has changed and it is safe to read now.
        ++m_setVisualIndex;

        // Allow access once again.
        m_haveVisualEvent = true;
    }
}

int
SequencerDataBlock::getRecordedEvents(MappedEventList &mC)
{
    // Grab a copy of the stopping point in case the other thread
    // changes it while we are working.
    int stopIndex = m_recordEventIndex;

    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    // While there are events in the record buffer, copy each event to
    // the user's list.
    while (m_readIndex != stopIndex) {
        mC.insert(new MappedEvent(recordBuffer[m_readIndex]));

        // Increment and wrap around to the beginning if needed.
        if (++m_readIndex == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE)
            m_readIndex = 0;
    }

    return mC.size();
}

void
SequencerDataBlock::addRecordedEvents(MappedEventList *mC)
{
    // Grab a copy of the record position so we don't update it
    // while the other thread is using it.
    int index = m_recordEventIndex;

    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    // Copy each incoming event into the ring buffer.
    for (MappedEventList::iterator i = mC->begin(); i != mC->end(); ++i) {
        recordBuffer[index] = **i;

        // Increment and wrap around to the beginning if needed.
        if (++index == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE)
            index = 0;
    }

    // Once the buffer is in a consistent state, move the record index
    // so that the other thread will read the new events.
    // ??? Is this guaranteed to be atomic and therefore thread safe?
    //     I believe so, and that's why this has always worked.
    m_recordEventIndex = index;
}

int
SequencerDataBlock::instrumentToIndex(InstrumentId id) const
{
    int i;

    for (i = 0; i < m_knownInstrumentCount; ++i) {
        if (m_knownInstruments[i] == id)
            return i;
    }

    return -1;
}

int
SequencerDataBlock::instrumentToIndexCreating(InstrumentId id)
{
    int i;

    for (i = 0; i < m_knownInstrumentCount; ++i) {
        if (m_knownInstruments[i] == id)
            return i;
    }

    if (i == SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS) {
        std::cerr << "ERROR: SequencerDataBlock::instrumentToIndexCreating("
        << id << "): out of instrument index space" << std::endl;
        return -1;
    }

    m_knownInstruments[i] = id;
    ++m_knownInstrumentCount;
    return i;
}

bool
SequencerDataBlock::getInstrumentLevel(InstrumentId id,
                                       LevelInfo &info) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int index = instrumentToIndex(id);
    if (index < 0) {
        info.level = info.levelRight = 0;
        return false;
    }

    int currentUpdateIndex = m_levelUpdateIndices[index];
    info = m_levels[index];

    /*
    std::cout << "SequencerDataBlock::getInstrumentLevel - "
              << "id = " << id
              << ", level = " << info.level << std::endl;
              */

    if (lastUpdateIndex[index] != currentUpdateIndex) {
        lastUpdateIndex[index] = currentUpdateIndex;
        return true;
    } else {
        return false; // no change
    }
}

bool
SequencerDataBlock::getInstrumentLevelForMixer(InstrumentId id,
        LevelInfo &info) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int index = instrumentToIndex(id);
    if (index < 0) {
        info.level = info.levelRight = 0;
        return false;
    }

    int currentUpdateIndex = m_levelUpdateIndices[index];
    info = m_levels[index];

    if (lastUpdateIndex[index] != currentUpdateIndex) {
        lastUpdateIndex[index] = currentUpdateIndex;
        return true;
    } else {
        return false; // no change
    }
}

void
SequencerDataBlock::setInstrumentLevel(InstrumentId id, const LevelInfo &info)
{
    int index = instrumentToIndexCreating(id);
    if (index < 0)
        return ;

    m_levels[index] = info;
    ++m_levelUpdateIndices[index];
}

bool
SequencerDataBlock::getInstrumentRecordLevel(InstrumentId id, LevelInfo &info) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int index = instrumentToIndex(id);
    if (index < 0) {
        info.level = info.levelRight = 0;
        return false;
    }

    int currentUpdateIndex = m_recordLevelUpdateIndices[index];
    info = m_recordLevels[index];

    if (lastUpdateIndex[index] != currentUpdateIndex) {
        lastUpdateIndex[index] = currentUpdateIndex;
        return true;
    } else {
        return false; // no change
    }
}

bool
SequencerDataBlock::getInstrumentRecordLevelForMixer(InstrumentId id, LevelInfo &info) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int index = instrumentToIndex(id);
    if (index < 0) {
        info.level = info.levelRight = 0;
        return false;
    }

    int currentUpdateIndex = m_recordLevelUpdateIndices[index];
    info = m_recordLevels[index];

    if (lastUpdateIndex[index] != currentUpdateIndex) {
        lastUpdateIndex[index] = currentUpdateIndex;
        return true;
    } else {
        return false; // no change
    }
}

void
SequencerDataBlock::setInstrumentRecordLevel(InstrumentId id, const LevelInfo &info)
{
    int index = instrumentToIndexCreating(id);
    if (index < 0)
        return ;

    m_recordLevels[index] = info;
    ++m_recordLevelUpdateIndices[index];
}

void
SequencerDataBlock::setTrackLevel(TrackId id, const LevelInfo &info)
{
    setInstrumentLevel
	(ControlBlock::getInstance()->getInstrumentForTrack(id), info);
}

bool
SequencerDataBlock::getTrackLevel(TrackId id, LevelInfo &info) const
{
    info.level = info.levelRight = 0;

    return getInstrumentLevel
	(ControlBlock::getInstance()->getInstrumentForTrack(id), info);

    return false;
}

bool
SequencerDataBlock::getSubmasterLevel(int submaster, LevelInfo &info) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS];

    if (submaster < 0 || submaster > SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS) {
        info.level = info.levelRight = 0;
        return false;
    }

    int currentUpdateIndex = m_submasterLevelUpdateIndices[submaster];
    info = m_submasterLevels[submaster];

    if (lastUpdateIndex[submaster] != currentUpdateIndex) {
        lastUpdateIndex[submaster] = currentUpdateIndex;
        return true;
    } else {
        return false; // no change
    }
}

void
SequencerDataBlock::setSubmasterLevel(int submaster, const LevelInfo &info)
{
    if (submaster < 0 || submaster > SEQUENCER_DATABLOCK_MAX_NB_SUBMASTERS) {
        return ;
    }

    m_submasterLevels[submaster] = info;
    ++m_submasterLevelUpdateIndices[submaster];
}

bool
SequencerDataBlock::getMasterLevel(LevelInfo &level) const
{
    // ??? Move statics to class scope non-static and clear them appropriately
    //     in clearTemporaries().
    static int lastUpdateIndex = 0;

    int currentIndex = m_masterLevelUpdateIndex;
    level = m_masterLevel;

    if (lastUpdateIndex != currentIndex) {
        lastUpdateIndex = currentIndex;
        return true;
    } else {
        return false;
    }
}

void
SequencerDataBlock::setMasterLevel(const LevelInfo &info)
{
    m_masterLevel = info;
    ++m_masterLevelUpdateIndex;
}

void
SequencerDataBlock::clearTemporaries()
{
    m_positionSec = 0;
    m_positionNsec = 0;

    m_setVisualIndex = 0;
    m_getVisualIndex = 0;
    m_haveVisualEvent = false;
    *((MappedEvent *)&m_visualEvent) = MappedEvent();

    m_recordEventIndex = 0;
    m_readIndex = 0;
    memset(m_recordBuffer, 0, sizeof(m_recordBuffer));

    memset(m_knownInstruments, 0, sizeof(m_knownInstruments));
    m_knownInstrumentCount = 0;

    memset(m_levelUpdateIndices, 0, sizeof(m_levelUpdateIndices));
    memset(m_levels, 0, sizeof(m_levels));

    memset(m_recordLevelUpdateIndices, 0, sizeof(m_recordLevelUpdateIndices));
    memset(m_recordLevels, 0, sizeof(m_recordLevels));

    memset(m_submasterLevelUpdateIndices, 0,
            sizeof(m_submasterLevelUpdateIndices));
    memset(m_submasterLevels, 0, sizeof(m_submasterLevels));

    m_masterLevelUpdateIndex = 0;
    m_masterLevel.level = 0;
    m_masterLevel.levelRight = 0;
}

}
