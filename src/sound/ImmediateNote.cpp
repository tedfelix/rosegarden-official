/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ImmediateNote]"

#include "ImmediateNote.h"

#include "base/Instrument.h"
#include "base/RealTime.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"
#include "sound/MappedEventInserter.h"

//#define DEBUG_PREVIEW_NOTES 1

namespace Rosegarden
{

// @author Tom Breton (Tehom) 
void
ImmediateNote::fillWithNote(
        MappedEventList &mappedEventList, Instrument *instrument,
        int pitch, int velocity, RealTime duration, bool oneshot)
{
    if (!instrument)
        return;

#ifdef DEBUG_PREVIEW_NOTES
    RG_DEBUG << "fillWithNote() on" << (instrument->isPercussion() ? "percussion" : "non-percussion") << instrument->getName() << instrument->getId();
#endif

    if ((pitch < 0) || (pitch > 127))
        return;

    if (velocity < 0)
        velocity = 100;

    MappedEvent::MappedEventType type =
            oneshot ? MappedEvent::MidiNoteOneShot : MappedEvent::MidiNote;

    // Make the event.
    MappedEvent mappedEvent(
            instrument->getId(),
            type,
            pitch,
            velocity,
            RealTime::zeroTime,  // absTime
            duration,
            RealTime::zeroTime);  // audioStartMarker

    // Since we're not going thru MappedBufMetaIterator::acceptEvent()
    // which checks tracks for muting, we needn't set a track.

    // Set up channel manager.
    m_channelManager.setInstrument(instrument);
    m_channelManager.reallocate(false);

    ChannelManager::SimpleCallbacks callbacks;
    MappedEventInserter inserter(mappedEventList);

    // Insert the event.
    // Setting firstOutput to true indicates that we want a channel
    // setup.
    m_channelManager.doInsert(
            inserter,
            mappedEvent,
            RealTime::zeroTime,  // refTime
            &callbacks,
            true,  // firstOutput
            NO_TRACK);
}


}
