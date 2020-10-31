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

#define RG_MODULE_STRING "[ExternalController]"

#include "ExternalController.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "MappedEvent.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/seqmanager/SequenceManager.h"
#include "misc/Strings.h"

#include <QSettings>

namespace Rosegarden
{


namespace
{
    QSharedPointer<ExternalController> a_self;
}

void ExternalController::create()
{
    a_self.reset(new ExternalController);
}

QSharedPointer<ExternalController> ExternalController::self()
{
    // create() was not called.
    Q_ASSERT(a_self);

    return a_self;
}

ExternalController::ExternalController() :
    activeWindow(Main),
    m_instrumentId(NoInstrument),
    m_playing(false),
    m_recording(false)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_controllerType = static_cast<ControllerType>(
            settings.value("controller_type", CT_RosegardenNative).toInt());
}

void ExternalController::connectRMW(RosegardenMainWindow *rmw)
{
    // If it seems like this isn't connecting, make sure the ExternalController
    // instance was created by the UI thread.  Signals/Slots cannot be
    // connected across threads.

    connect(rmw, &RosegardenMainWindow::documentLoaded,
            this, &ExternalController::slotDocumentLoaded);

    // Might as well hook up for CCs as well.
    connect(Instrument::getStaticSignals().data(),
                &InstrumentStaticSignals::controlChange,
            this,
            &ExternalController::slotControlChange);

    SequenceManager *sequenceManager = rmw->getSequenceManager();

    // And transport changes.
    connect(sequenceManager, &SequenceManager::signalPlaying,
            this, &ExternalController::slotPlaying);
    connect(sequenceManager, &SequenceManager::signalRecording,
            this, &ExternalController::slotRecording);
}

void ExternalController::setType(ControllerType controllerType)
{
    m_controllerType = controllerType;

    // Write to .conf so this becomes the default.
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("controller_type", static_cast<int>(m_controllerType));

    if (m_controllerType == CT_KorgNanoKontrol2)
        korgNanoKontrol2.init();
}

bool ExternalController::isEnabled()
{
    static bool cacheValid = false;
    static bool enabled = false;

    if (cacheValid)
        return enabled;

    // Cache is not valid.  Prime it.

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    // We default to disabling the external controller port because some
    // software synths are very aggressive about making connections and
    // can end up connecting to the external controller port.  When that
    // happens, typically only channel 1 can be heard since
    // RosegardenMainWindow will send volume 0 to all other channels.
    // External controller can be re-enabled via the preferences.  When
    // enabled, be sure to look closely at the connections being made
    // and make sure external controller is only being connected to an
    // appropriate control surface.
    enabled = settings.value("external_controller", false).toBool();

    cacheValid = true;

    return enabled;
}

void ExternalController::processEvent(const MappedEvent *event)
{
    switch (m_controllerType) {
    case CT_RosegardenNative:
        processRGNative(event);
        break;
    case CT_KorgNanoKontrol2:
        korgNanoKontrol2.processEvent(event);
        break;
    }
}

void ExternalController::processRGNative(const MappedEvent *event)
{
    if (event->getType() == MappedEvent::MidiController) {

        // If switch window CC (CONTROLLER_SWITCH_WINDOW)
        if (event->getData1() == 81) {

            const int value = event->getData2();

            if (value < 42) {  // Main Window

                RosegardenMainWindow::self()->openWindow(Main);

                return;

            } else if (value < 84) {  // Audio Mixer

                RosegardenMainWindow::self()->openWindow(AudioMixer);

                return;

            } else {  // MIDI Mixer

                RosegardenMainWindow::self()->openWindow(MidiMixer);

                return;

            }
        }
    }

    // Delegate to the currently active window.
    switch (activeWindow)
    {
    case Main:
        emit externalControllerRMVW(event);
        break;
    case MidiMixer:
        emit externalControllerMMW(event);
        break;
    case AudioMixer:
        emit externalControllerAMW2(event);
        break;
    }
}

void ExternalController::send(
        MidiByte channel, MidiByte controlNumber, MidiByte value)
{
    // Not enabled?  Bail.
    if (!isEnabled())
        return;

    MappedEvent event(NoInstrument,  // instrumentId is ignored
                      MappedEvent::MidiController,
                      controlNumber,
                      MidiByte(value));
    event.setRecordedChannel(channel);
    event.setRecordedDevice(Device::EXTERNAL_CONTROLLER);

    RosegardenSequencer::getInstance()->processMappedEvent(event);
}

void ExternalController::sendAllCCs(
        const Instrument *instrument, MidiByte channel)
{
    if (channel == MidiMaxValue)
        channel = instrument->getNaturalChannel();

    send(channel,
         MIDI_CONTROLLER_VOLUME,
         instrument->getVolumeCC());

    send(channel,
         MIDI_CONTROLLER_PAN,
         instrument->getPanCC());

    // For MIDI instruments...
    if (instrument->getType() == Instrument::Midi) {

        // Also send all the other CCs in case the surface can handle them.

        StaticControllers staticControllers =
                instrument->getStaticControllers();

        // For each Control Change...
        for (const ControllerValuePair &pair : staticControllers) {

            const MidiByte controlNumber = pair.first;

            // Volume and Pan were already covered above.  Skip.
            if (controlNumber == MIDI_CONTROLLER_VOLUME)
                continue;
            if (controlNumber == MIDI_CONTROLLER_PAN)
                continue;

            const MidiByte controlValue = pair.second;

            send(channel, controlNumber, controlValue);

        }

    }
}

void ExternalController::sendSysExHex(const QString &hexString)
{
    // Not enabled?  Bail.
    if (!isEnabled())
        return;

    // Translate the hex string to bytes.

    std::string rawString;
    // For each hex byte (two characters)...
    for (int i = 0; i < hexString.size(); i += 2) {
        rawString.push_back(static_cast<char>(
                hexString.mid(i, 2).toInt(nullptr, 16)));
    }

    // Send it out.

    sendSysExRaw(rawString);
}

void
ExternalController::sendSysExRaw(const std::string &rawString)
{
    // Not enabled?  Bail.
    if (!isEnabled())
        return;

    // Assemble the SysEx MappedEvent.

    MappedEvent event(NoInstrument,  // instrumentId is ignored
                      MappedEvent::MidiSystemMessage,
                      MIDI_SYSTEM_EXCLUSIVE);
    event.setRecordedDevice(Device::EXTERNAL_CONTROLLER);
    event.addDataString(rawString);

    // Send it out.

    RosegardenSequencer::getInstance()->processMappedEvent(event);
}

bool ExternalController::getSysEx(std::string & /*rawString*/)
{
#if 0
    // This really didn't work.  We need to do some restructuring of
    // AlsaDriver before we can successfully have conversations with
    // MIDI equipment over the external controller port.

    // ??? Need to do a blocking read from the external controller port
    //     with a 500msec timeout.  How???

    // Copy sysex data from m_sysexEvent to rawString.
    rawString = DataBlockRepository::getDataBlockForEvent(&sysexEvent);

#endif

    return true;
}

void
ExternalController::slotDocumentLoaded(RosegardenDocument *doc)
{
    // If this is never happening, see the comments in connectRMW() above.

    if (!doc)
        return;

    // Connect to the new document for change notifications.
    connect(doc, &RosegardenDocument::documentModified,
            this, &ExternalController::slotDocumentModified);

    // Force an update to make sure we are in sync with the new document.
    m_instrumentId = NoInstrument;
    slotDocumentModified(true);
}

void
ExternalController::slotDocumentModified(bool)
{
    // If this is never happening, see the comments in connectRMW() above.

    if (m_controllerType == CT_RosegardenNative) {

        // We only handle updates for RosegardenMainWindow.
        if (activeWindow != Main)
            return;

        RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

        // Get the selected Track's Instrument.
        InstrumentId instrumentId =
                doc->getComposition().getSelectedInstrumentId();

        if (instrumentId == NoInstrument)
            return;

        // No change to the Track/Instrument?  Bail.
        if (instrumentId == m_instrumentId)
            return;

        m_instrumentId = instrumentId;

        Instrument *instrument = doc->getStudio().getInstrumentById(instrumentId);

        if (!instrument)
            return;

        // Send out the CCs for the current Track on channel 0.
        sendAllCCs(instrument, 0);

        return;
    }

    if (m_controllerType == CT_KorgNanoKontrol2) {
        korgNanoKontrol2.documentModified();
        return;
    }
}

void
ExternalController::slotControlChange(Instrument *instrument, int cc)
{
    if (m_controllerType == CT_RosegardenNative) {
        // We only handle updates for RosegardenMainWindow.
        if (activeWindow != Main)
            return;

        // Not our Instrument?  Bail.
        if (instrument->getId() != m_instrumentId)
            return;

        // Fixed channels only.
        if (!instrument->hasFixedChannel())
            return;

        if (cc == MIDI_CONTROLLER_VOLUME) {
            send(0, MIDI_CONTROLLER_VOLUME, instrument->getVolumeCC());
            return;
        }

        if (cc == MIDI_CONTROLLER_PAN) {
            send(0, MIDI_CONTROLLER_PAN, instrument->getPanCC());
            return;
        }

        // All other controllers can use Instrument::getControllerValue().

        send(0, cc, instrument->getControllerValue(cc));
    }
}

void
ExternalController::slotPlaying(bool checked)
{
    m_playing = checked;

    if (m_controllerType == CT_KorgNanoKontrol2) {
        if (m_playing  &&  !m_recording)
            korgNanoKontrol2.playing();
        else if (!m_playing  &&  !m_recording)
            korgNanoKontrol2.stopped();
        return;
    }
}

void
ExternalController::slotRecording(bool checked)
{
    m_recording = checked;

    if (m_controllerType == CT_KorgNanoKontrol2) {
        if (m_recording)
            korgNanoKontrol2.recording();
        else if (!m_playing  &&  !m_recording)
            korgNanoKontrol2.stopped();
        return;
    }
}


}
