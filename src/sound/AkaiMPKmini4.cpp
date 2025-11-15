/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2020-2025 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/**
 * Support for the Akai MPK Mini IV control surface (MIDI port).
 * Christophe MALHAIRE 26/10/2025
 * Inspired by: KorgNanoKontrol2.cpp
 */

#define RG_MODULE_STRING "[AkaiMPKmini4]"

#include "AkaiMPKmini4.h"

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

#include <vector>


#include <iostream>
#include <chrono>
#include <thread>
using namespace std;

namespace Rosegarden
{


AkaiMPKmini4::AkaiMPKmini4()
{
}

void AkaiMPKmini4::init()
{
    // Configure the device.

#if 0

    // Confirm expected device.

    // Send Inquiry Message Request.
    ExternalController::sendSysExHex("F07E7F0601F7"); // Identity Request

    std::string buffer;

    // Get Device Inquiry Reply.
    // We need a synchronous getSysEx() with 500msec(?) timeout.
    // It also needs to stitch together multiple packets into one until EOX.
    ExternalController::self()->getSysEx(buffer);

    QByteArray byteArray(buffer.c_str());
    RG_DEBUG << "Response: " << byteArray.toHex();

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
                QObject::tr("Rosegarden"),
                QObject::tr("The connected Korg nanoKONTROL2 is not configured optimally for Rosegarden.  Reconfiguring it will lose any custom settings you've made with the nanoKONTROL2 editor.  Reconfigure?"),
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

    initLEDs();

#endif

}

void AkaiMPKmini4::documentModified()
{
    //int dummy = 0;
    //refreshLEDs();
}

void AkaiMPKmini4::stopped()
{
    setPlayStopLED(false);
    setRecordLED(false);
}

void AkaiMPKmini4::playing()
{
    setPlayStopLED(true);
}



// Does not work without adding a delay
void AkaiMPKmini4::recording()
{
    setPlayStopLED(true);
    setRecordLED(true);
    std::this_thread::sleep_for(10ms); // ugly workaround!
}


void AkaiMPKmini4::processEvent(const MappedEvent *i_event)
{
    // Not a CC?  Bail.
    if (i_event->getType() != MappedEvent::MidiController)
        return;

    // ??? See RosegardenMainWindow::customEvent().  That would be a
    //     more generic, albeit slower (message queue), way to do this.

    const MidiByte controlNumber = i_event->getData1();
    const MidiByte value = i_event->getData2();

    printf("CC, value = 0x%02X 0x%02X\n", controlNumber, value); // debug

    // Record
    // Handle this first for "speed".
    if (controlNumber == 77  &&  value == 127) {
        printf("Process event Record -> m_record = %s\n", m_record ? "true" : "false"); // debug
        if (m_record == false) {
            //m_record = true; // not needed
            printf("lancement de recording()\n"); // debug
            recording(); // does not work...
            std::this_thread::sleep_for(10ms); // ugly workaround!
            printf("fin recording()\n"); // debug
            printf("lancement de RosegardenMainWindow::self()->slotRecord()\n"); // debug
            RosegardenMainWindow::self()->slotRecord();
            printf("fin de RosegardenMainWindow::self()->slotRecord()\n"); // debug
        }
        else {
            printf("else\n"); // debug
            m_record = false;
            //stopped();
        }
        //RosegardenMainWindow::self()->slotRecord();
        return;
    }

    // Play/Stop
    // Handle this second for "speed".
    // There is only one button for Play/Stop
    if (controlNumber == 76  &&  value == 127) {
        //printf("Process event Record -> m_play = %s\n", m_play ? "true" : "false"); // debug
        if (m_play == false) {
            RosegardenMainWindow::self()->slotPlay();
        }
        else {
            // We cannot call this in the middle of processing incoming MIDI.
            // The system is not ready for recording to stop.
            // RosegardenMainWindow::self()->slotStop();

            // Instead, we queue up a request for
            // RosegardenMainWindow::customEvent().
            QEvent *event = new QEvent(Stop);
            QCoreApplication::postEvent(
                    RosegardenMainWindow::self(), event);
        }
        return;
    }

    // Cycle (Loop)
    if (controlNumber == 74  &&  value == 127) {
        RosegardenMainWindow::self()->toggleLoop();
        return;
    }

    // Knobs
    if (6 <= controlNumber  &&  controlNumber <= 97) {
        processKnob(controlNumber, value);
        return;
    }

}



void AkaiMPKmini4::processKnob(MidiByte controlNumber, MidiByte value) const
{
    
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition &comp = doc->getComposition();
    
    TrackId m_selectedTrackId;
    m_selectedTrackId = comp.getSelectedTrack();

    const Track *track = comp.getTrackById(m_selectedTrackId);
    if (!track)
        return;

    Studio &studio = doc->getStudio();
    Instrument *instrument = studio.getInstrumentById(track->getInstrument());
    if (!instrument)
        return;

    if (instrument->getType() == Instrument::Midi) {
        // If the Instrument has volume...
        if (instrument->hasController(controlNumber)) {
            // Adjust the Instrument and update everyone.
            instrument->setControllerValue(controlNumber, value);
            Instrument::emitControlChange(instrument, controlNumber);
            doc->setModified();
        }

        return;
    }

    // We have an audio instrument or a softsynth...

    //instrument->setControllerValue(
    //        controlNumber,
    //        AudioLevel::AudioPanI(value));
    //Instrument::emitControlChange(instrument, controlNumber);
    //doc->setModified();
}


/**
 * OK, works as expected
 */
void AkaiMPKmini4::setPlayStopLED(bool play)
{
    ExternalController::sendNoteOn(
          play ? 9 : 0, // channel=behavior: 9=Pulsing 1/4, 0 = On 10% brightness
          0x4C, // noteNumber
          1);  // velocity (0=Off, 1=On)
    m_play = play;
}


/**
 * Does not work without adding a delay
 * 
 * (see KorgNanoKontrol2.cpp)
 */

void AkaiMPKmini4::setRecordLED(bool record)
{
    ExternalController::sendNoteOn(
          record ? 9 : 0, // channel=behavior: 9=Pulsing 1/4, 0 = On 10% brightness
          0x4D, // noteNumber
          1);  // velocity (0=Off, 1=On)
    m_record = record;
}


//void AkaiMPKmini4::setLoopLED(bool loop)
//{
//    ExternalController::sendNoteOn(
//          record ? 2 : 0, // channel=behavior: 2 = On 50% brightness, 0 = On 10% brightness
//          0x4A, // noteNumber
//          1);  // velocity (0=Off, 1=On)
//    m_record = record;
//}

}
