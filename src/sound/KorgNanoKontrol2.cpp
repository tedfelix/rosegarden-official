/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2020 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[KorgNanoKontrol2]"

#include "KorgNanoKontrol2.h"

#include "base/AudioLevel.h"
#include "base/Composition.h"
#include "misc/Debug.h"
#include "base/Instrument.h"
#include "MappedEvent.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"
#include "base/Track.h"


namespace Rosegarden
{


void KorgNanoKontrol2::processEvent(const MappedEvent *event)
{
    // Not a CC?  Bail.
    if (event->getType() != MappedEvent::MidiController)
        return;

    const MidiByte controlNumber = event->getData1();
    const MidiByte value = event->getData2();

    // ??? Probably should handle "play" first to maximize responsiveness.

    // Volume Faders
    if (controlNumber <= 7)
    {
        processFader(controlNumber, value);
        return;
    }

    // Pan Knobs
    if (16 <= controlNumber  &&  controlNumber <= 23)
    {
        RG_DEBUG << "processEvent(): Received pan knob CC.  Knob " << controlNumber - 15 << " value " << value;
        //processKnob(controlNumber, value);
        return;
    }
}

void KorgNanoKontrol2::processFader(MidiByte controlNumber, MidiByte value)
{
    RG_DEBUG << "processFader(): Fader " << controlNumber + 1 << " value " << value;

    const int trackNumber = controlNumber;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    // ??? Should we use the selected Track to indicate track 1 on the
    //     surface?  That would provide some potentially helpful feedback.
    //const Track *track = comp.getTrackById(comp.getSelectedTrack());

    const Track *track = comp.getTrackByPosition(trackNumber);
    if (!track)
        return;

    Studio &studio = doc->getStudio();
    Instrument *instrument = studio.getInstrumentById(track->getInstrument());
    if (!instrument)
        return;

    if (instrument->getType() == Instrument::Midi) {
        // If the Instrument has volume...
        if (instrument->hasController(MIDI_CONTROLLER_VOLUME)) {
            // Adjust the Instrument and update everyone.
            instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, value);
            Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);
            doc->setModified();
        }

        return;
    }

    // We have an audio instrument or a softsynth...

    //RG_DEBUG << "  Setting volume for instrument " << instrument->getId() << " to " << value;

    const float dB = AudioLevel::fader_to_dB(
            value, 127, AudioLevel::ShortFader);

    instrument->setLevel(dB);
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);
    doc->setModified();
}


}
