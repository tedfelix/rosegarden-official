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

#include "SequencerDataBlock.h"
#include "MappedComposition.h"

namespace Rosegarden
{
      
SequencerDataBlock::SequencerDataBlock(bool initialise)
{
    if (initialise) {
	m_controlBlock = 0;
	m_positionSec = 0;
	m_positionUsec = 0;
	m_visualEventIndex = 0;
	*((MappedEvent *)&m_visualEvent) = MappedEvent();
	m_haveVisualEvent = false;
	m_recordEventIndex = 0;
	m_recordLevel.level = 0;
	m_recordLevel.levelRight = 0;
	memset(m_trackLevels, 0,
	       CONTROLBLOCK_MAX_NB_TRACKS * sizeof(TrackLevelInfo));
    }
}

bool
SequencerDataBlock::getVisual(MappedEvent &ev) const
{
    static int eventIndex = 0;

    if (!m_haveVisualEvent) {
	return false;
    } else {
	int thisEventIndex = m_visualEventIndex;
	if (thisEventIndex == eventIndex) return false;
	ev = *((MappedEvent *)&m_visualEvent);
	eventIndex = thisEventIndex;
	return true;
    }
}

void
SequencerDataBlock::setVisual(const MappedEvent *ev)
{
    m_haveVisualEvent = false;
    if (ev) {
	*((MappedEvent *)&m_visualEvent) = *ev;
	++m_visualEventIndex;
	m_haveVisualEvent = true;
    }
}

int
SequencerDataBlock::getRecordedEvents(MappedComposition &mC) const
{
    static int readIndex = 0;

    int currentIndex = m_recordEventIndex;
    int count = 0;
    
    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    while (readIndex != currentIndex) {
	mC.insert(new MappedEvent(recordBuffer[readIndex]));
	if (++readIndex == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE) readIndex = 0;
	++count;
    }

    return count;
}

void
SequencerDataBlock::addRecordedEvents(MappedComposition *mC)
{
    // ringbuffer
    int index = m_recordEventIndex;
    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    for (MappedComposition::iterator i = mC->begin(); i != mC->end(); ++i) {
	recordBuffer[index] = **i;
	if (++index == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE) index = 0;
    }

    m_recordEventIndex = index;
}

bool
SequencerDataBlock::getTrackLevel(TrackId track, TrackLevelInfo &info) const
{
    static int lastUpdateIndex[CONTROLBLOCK_MAX_NB_TRACKS];

    int currentIndex = m_trackLevelUpdateIndices[track];
    info = m_trackLevels[track];

    if (lastUpdateIndex[track] != currentIndex) {
	lastUpdateIndex[track]  = currentIndex;
	return true;
    } else {
	return false; // no change
    }
}

void
SequencerDataBlock::setTrackLevel(TrackId track, const TrackLevelInfo &info)
{
    m_trackLevels[track] = info;
    ++m_trackLevelUpdateIndices[track];
}

bool
SequencerDataBlock::getRecordLevel(TrackLevelInfo &level) const
{
    static int lastUpdateIndex = 0;

    int currentIndex = m_recordLevelUpdateIndex;
    level = m_recordLevel;

    if (lastUpdateIndex != currentIndex) {
	lastUpdateIndex  = currentIndex;
	return true;
    } else {
	return false;
    }
}

void
SequencerDataBlock::setRecordLevel(const TrackLevelInfo &level)
{
    m_recordLevel = level;
    ++m_recordLevelUpdateIndex;
}

void
SequencerDataBlock::setTrackLevelsForInstrument(InstrumentId id,
						const TrackLevelInfo &info)
{
    if (m_controlBlock) {
	for (unsigned int track = 0;
	     track < m_controlBlock->getNbTracks(); ++track) {
	    if (m_controlBlock->getInstrumentForTrack(track) == id) {
		setTrackLevel(track, info);
	    }
	}
    }
}

}

