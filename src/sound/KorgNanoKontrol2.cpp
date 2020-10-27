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
#include "base/QEvents.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"
#include "base/Track.h"

#include <QCoreApplication>
#include <QEvent>
#include <QObject>

namespace Rosegarden
{


KorgNanoKontrol2::KorgNanoKontrol2() :
    m_page(0)
{
    QObject::connect(
            &m_rewindTypematic, &Typematic::click,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewind);

    QObject::connect(
            &m_fastForwardTypematic, &Typematic::click,
            RosegardenMainWindow::self(),
                &RosegardenMainWindow::slotFastforward);
}

void KorgNanoKontrol2::processEvent(const MappedEvent *event)
{
    // Not a CC?  Bail.
    if (event->getType() != MappedEvent::MidiController)
        return;

    const MidiByte controlNumber = event->getData1();
    const MidiByte value = event->getData2();

    // Record
    // Handle this first for "speed".
    if (controlNumber == 45  &&  value == 127) {
        RosegardenMainWindow::self()->slotRecord();
        return;
    }

    // Play
    // Handle this second for "speed".
    if (controlNumber == 41  &&  value == 127) {
        RosegardenMainWindow::self()->slotPlay();
        return;
    }

    // Volume Faders
    if (controlNumber <= 7) {
        processFader(controlNumber, value);
        return;
    }

    // Pan Knobs
    if (16 <= controlNumber  &&  controlNumber <= 23) {
        processKnob(controlNumber, value);
        return;
    }

    // Track Left
    if (controlNumber == 58  &&  value == 127) {
        if (m_page == 0)
            return;

        --m_page;

        // ??? Would be nice to have some feedback in the UI.  E.g. a
        //     range indicator on the tracks.
    }

    // Track Right
    if (controlNumber == 59  &&  value == 127) {
        RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
        Composition &comp = doc->getComposition();

        if ((m_page + 1) * 8 >= comp.getTracks().size())
            return;

        ++m_page;
    }

    // Stop
    if (controlNumber == 42  &&  value == 127) {
        // We cannot call this in the middle of processing incoming MIDI.
        // The system is not ready for recording to stop.
        //RosegardenMainWindow::self()->slotStop();

        // Instead, we queue up a request for
        // RosegardenMainWindow::customEvent().
        QEvent *event = new QEvent(Stop);
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);

        return;
    }

    // Rewind
    if (controlNumber == 43) {
        m_rewindTypematic.press(value == 127);
        return;
    }

    // Fast-forward
    if (controlNumber == 44) {
        m_fastForwardTypematic.press(value == 127);
        return;
    }

    // Cycle (Loop)
    if (controlNumber == 46  &&  value == 127) {
        RosegardenMainWindow::self()->toggleLoop();
        return;
    }

}

void KorgNanoKontrol2::processFader(MidiByte controlNumber, MidiByte value)
{
    const int trackNumber = controlNumber + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    // ??? Should we use the selected Track to indicate track 1 on the
    //     surface?  That would provide some potentially helpful feedback.
    //     OTOH it would cause the surface's position to jump around if
    //     the user is doing some editing.
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

    const float dB = AudioLevel::fader_to_dB(
            value, 127, AudioLevel::ShortFader);

    instrument->setLevel(dB);
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);
    doc->setModified();
}

void KorgNanoKontrol2::processKnob(MidiByte controlNumber, MidiByte value)
{
    //RG_DEBUG << "processKnob(): Knob " << controlNumber - 15 << " value " << value;

    const int trackNumber = controlNumber - 16 + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    // ??? Should we use the selected Track to indicate track 1 on the
    //     surface?  That would provide some potentially helpful feedback.
    //     OTOH it would cause the surface's position to jump around if
    //     the user is doing some editing.
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
        if (instrument->hasController(MIDI_CONTROLLER_PAN)) {
            // Adjust the Instrument and update everyone.
            instrument->setControllerValue(MIDI_CONTROLLER_PAN, value);
            Instrument::emitControlChange(instrument, MIDI_CONTROLLER_PAN);
            doc->setModified();
        }

        return;
    }

    // We have an audio instrument or a softsynth...

    //RG_DEBUG << "  Setting pan for instrument " << instrument->getId() << " to " << value;

    instrument->setControllerValue(
            MIDI_CONTROLLER_PAN,
            AudioLevel::AudioPanI(value));
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_PAN);
    doc->setModified();
}


}
