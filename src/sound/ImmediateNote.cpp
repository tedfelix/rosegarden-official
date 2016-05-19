/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ImmediateNote.h"

#include "base/Instrument.h"
#include "base/RealTime.h"
#include "misc/Debug.h"
#include "sound/MappedEvent.h"
#include "sound/MappedEventInserter.h"

#define DEBUG_PREVIEW_NOTES 1

namespace Rosegarden
{

static bool
canPreviewAnotherNote()
{
    // NB, this is C's time_t, not our timeT.
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second or so, to
            // avoid gungeing up the sound drivers
            return false;
        }
    }

    return true;
}

// Fill mC with a corresponding note and its appropriate setup events.
// @author Tom Breton (Tehom) 
void
ImmediateNote::
fillWithNote(MappedEventList &mC, Instrument *instrument,
             int pitch, int velocity, RealTime duration, bool oneshot)
{
  if (!instrument) { return; }
#ifdef DEBUG_PREVIEW_NOTES
    SEQUENCER_DEBUG
        << "ImmediateNote::fillWithNote on"
        << (instrument->isPercussion() ? "percussion" : "non-percussion")
        << instrument->getName() << instrument->getId()
        << endl;
#endif
  if (!canPreviewAnotherNote()) { return; }
  if ((pitch < 0) || (pitch > 127)) { return; }
  if (velocity < 0) { velocity = 100; }

  MappedEvent::MappedEventType type =
    oneshot ?
    MappedEvent::MidiNoteOneShot :
    MappedEvent::MidiNote;

  // Make the event.
  MappedEvent mE(instrument->getId(),
                 type,
                 pitch,
                 velocity,
                 RealTime::zeroTime,
                 duration,
                 RealTime::zeroTime);
  // Since we're not going thru MappedBufMetaIterator::acceptEvent
  // which checks tracks for muting, we needn't set a track.

  // Set up channel manager.
  m_channelManager.setInstrument(instrument);
  m_channelManager.reallocateEternalChannel();

  // Set up channel.
  ChannelManager::SimpleCallbacks callbacks;
  MappedEventInserter inserter(mC);

  // Insert the event.
  m_channelManager.doInsert(inserter, mE, RealTime::zeroTime,
                            &callbacks, true, NO_TRACK);
}


}
