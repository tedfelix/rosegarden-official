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
//#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"
#include "base/Track.h"
//#include "gui/editors/segment/TrackButtons.h"
//#include "gui/editors/segment/TrackEditor.h"

#include <QCoreApplication>
#include <QEvent>

#include <vector>

namespace Rosegarden
{


KorgNanoKontrol2::KorgNanoKontrol2() :
    m_page(0)
{
}

void KorgNanoKontrol2::init()
{
    // Configure the device.

    // Confirm expected device.

    // Send Inquiry Message Request.
    ExternalController::sendSysExHex("7E7F0601");

#if 0
    SysExBuffer buffer;

    // Get Device Inquiry Reply.
    // We need a synchronous getSysEx() with 500msec(?) timeout.
    // Using QEventLoop might be best since we are in the UI thread
    // and MIDI data is obtained via a polling timer.
    // It also needs to stitch together multiple packets into one until EOX.
    // MappedEvent might already handle that if we are lucky.
    getSysEx(buffer);
    // Special handling for this since the hardware version can vary.
    bool success = checkDeviceInquiryReply(buffer);
    if (!success) {
        RG_WARNING << "init(): Did not receive expected Device Inquiry Reply";
        return;
    }

    // Get a dump of the scene.
    sendSysExRaw(currentSceneDataDumpRequest);
    getSysEx(buffer);

    // Compare with Rosegarden scene, bail if same.
    if (compareSysEx(buffer, rosegardenScene))
        return;

    // If the user appears to have customized the scene, ask if it is
    // ok to clobber it.
    if (!compareSysEx(buffer, defaultScene))
    {
        // Ask user if it is ok to reconfigure the device.
        QMessageBox::StandardButton reply = QMessageBox::warning(
                0,
                tr("Rosegarden"),
                tr("The connected Korg nanoKONTROL2 is not configured optimally for Rosegarden.  Reconfiguring it will lose any custom settings you've made with the nanoKONTROL2 editor.  Reconfigure?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);

        // If not, return.
        if (reply == QMessageBox::No)
            return;
    }

    // Send "Current Scene Data Dump" with Rosegarden scene.
    sendSysExRaw(rosegardenScene);

    // Confirm ACK
    getSysEx(buffer);
    success = compareSysEx(buffer, dataLoadCompleted);
    if (!success) {
        RG_WARNING << "init(): Did not receive expected Data Load Completed ACK";
        return;
    }

    // Send Scene Write Request.
    sendSysExRaw(sceneWriteRequest);

    // Confirm Write Completed.
    getSysEx(buffer);
    success = compareSysEx(buffer, writeCompleted);
    if (!success) {
        RG_WARNING << "init(): Did not receive expected Write Completed";
        return;
    }

    // ??? Do an LED test?  testLEDs() and ask the user to confirm.
#endif

    initLEDs();
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
        if (m_page == 0) {
            // refresh LEDs regardless in case the user needs to get
            // them back in sync.
            refreshLEDs();

            return;
        }

        --m_page;

        refreshLEDs();

        // ??? Would be nice to have some feedback in the UI.  E.g. a
        //     range indicator on the tracks.
        //TrackButtons *trackButtons = RosegardenMainWindow::self()->
        //        getView()->getTrackEditor()->getTrackButtons();
        //trackButtons->setSurfaceRange(m_page * 8, 8);
        //trackButtons->slotUpdateTracks();

        return;
    }

    // Track Right
    if (controlNumber == 59  &&  value == 127) {
        RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
        Composition &comp = doc->getComposition();

        if ((m_page + 1) * 8 >= comp.getTracks().size()) {
            // refresh LEDs regardless in case the user needs to get
            // them back in sync.
            refreshLEDs();

            return;
        }

        ++m_page;

        refreshLEDs();

        return;
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
        // Note: We tried doing this locally, but it crosses threads.
        //       Using the event queue is thread-safe.

        QEvent *event = new ButtonEvent(Rewind, (value == 127));
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);
        return;
    }

    // Fast-forward
    if (controlNumber == 44) {
        // Note: We tried doing this locally, but it crosses threads.
        //       Using the event queue is thread-safe.

        QEvent *event = new ButtonEvent(FastForward, (value == 127));
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);
        return;
    }

    // Cycle (Loop)
    if (controlNumber == 46  &&  value == 127) {
        RosegardenMainWindow::self()->toggleLoop();
        return;
    }

    // "S" solo buttons
    if (32 <= controlNumber  &&  controlNumber <= 39  &&  value == 127) {
        processSolo(controlNumber);
        return;
    }

    // "M" mute buttons
    if (48 <= controlNumber  &&  controlNumber <= 55  &&  value == 127) {
        processMute(controlNumber);
        return;
    }

    // "R" record buttons
    if (64 <= controlNumber  &&  controlNumber <= 71  &&  value == 127) {
        processRecord(controlNumber);
        return;
    }

    // Marker Set
    if (controlNumber == 60  &&  value == 127) {
        QEvent *event = new QEvent(AddMarker);
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);
        return;
    }

    // Marker Left
    if (controlNumber == 61  &&  value == 127) {
        QEvent *event = new QEvent(PreviousMarker);
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);
        return;
    }

    // Marker Right
    if (controlNumber == 62  &&  value == 127) {
        QEvent *event = new QEvent(NextMarker);
        QCoreApplication::postEvent(
                RosegardenMainWindow::self(), event);
        return;
    }

}

void KorgNanoKontrol2::processFader(MidiByte controlNumber, MidiByte value)
{
    const int trackNumber = controlNumber + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

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
    const int trackNumber = controlNumber - 16 + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

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

    instrument->setControllerValue(
            MIDI_CONTROLLER_PAN,
            AudioLevel::AudioPanI(value));
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_PAN);
    doc->setModified();
}

void KorgNanoKontrol2::processSolo(MidiByte controlNumber)
{
    const int trackNumber = controlNumber - 32 + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    Track *track = comp.getTrackByPosition(trackNumber);
    if (!track)
        return;

    // Toggle solo
    track->setSolo(!track->isSolo());
    comp.notifyTrackChanged(track);

    doc->setModified();
}

void KorgNanoKontrol2::processMute(MidiByte controlNumber)
{
    const int trackNumber = controlNumber - 48 + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    Track *track = comp.getTrackByPosition(trackNumber);
    if (!track)
        return;

    // Toggle mute
    const bool muted = !track->isMuted();

    // Update the Track
    track->setMuted(muted);
    comp.notifyTrackChanged(track);

    doc->setModified();
}

void KorgNanoKontrol2::processRecord(MidiByte controlNumber)
{
    const int trackNumber = controlNumber - 64 + m_page*8;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    Track *track = comp.getTrackByPosition(trackNumber);
    if (!track)
        return;

    // Toggle
    bool state = !comp.isTrackRecording(track->getId());

    // Update the Track
    comp.setTrackRecording(track->getId(), state);
    comp.notifyTrackChanged(track);

    doc->checkAudioPath(track);

    doc->setModified();
}

void KorgNanoKontrol2::testLEDs(bool on)
{
    const MidiByte value = on ? 127 : 0;

    for (int i = 0; i < 8; ++i) {
        // Record
        ExternalController::send(
                0, // channel
                64+i, // controlNumber
                value);  // value
        // Mute
        ExternalController::send(
                0, // channel
                48+i, // controlNumber
                value);  // value
        // Solo
        ExternalController::send(
                0, // channel
                32+i, // controlNumber
                value);  // value
    }

    ExternalController::send(
            0, // channel
            41, // controlNumber
            value);  // value
    ExternalController::send(
            0, // channel
            42, // controlNumber
            value);  // value
    ExternalController::send(
            0, // channel
            43, // controlNumber
            value);  // value
    ExternalController::send(
            0, // channel
            44, // controlNumber
            value);  // value
    ExternalController::send(
            0, // channel
            45, // controlNumber
            value);  // value
    ExternalController::send(
            0, // channel
            46, // controlNumber
            value);  // value

    // The following buttons have no LED:
    // - Track Left (58)
    // - Track Right (59)
    // - Marker Set (60)
    // - Marker Left (61)
    // - Marker Left (62)
}

void KorgNanoKontrol2::initLEDs()
{
    // Turn off all the LEDs and update the cache to match.
}

void KorgNanoKontrol2::refreshLEDs()
{
#if 0
    // Do another init for testing purposes.  We should be able to
    // connect to aseqdump and watch the messages go back and forth.
    init();
#endif
#if 0
    static bool on = false;
    on = !on;
    testLEDs(on);
#endif

#if 0
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Composition &comp = doc->getComposition();

    // For each Track
    for (int i = 0; i < 8; ++i) {
        const int trackNumber = i + m_page*8;

        Track *track = comp.getTrackByPosition(trackNumber);
        if (!track)
            return;

        // Solo
        ExternalController::send(
                0, // channel
                32+i, // controlNumber
                track->isSolo() ? 127 : 0);  // value

        // Mute
        ExternalController::send(
                0, // channel
                48+i, // controlNumber
                track->isMuted() ? 0 : 127);  // value

        // Record
        ExternalController::send(
                0, // channel
                64+i, // controlNumber
                comp.isTrackRecording(track->getId()) ? 127 : 0);  // value
    }
#endif

    // Also update the local state cache.
}


}
