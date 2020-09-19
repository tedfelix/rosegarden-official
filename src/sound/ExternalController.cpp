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
#include "gui/studio/StudioControl.h"

#include <QSettings>

namespace Rosegarden
{


QSharedPointer<ExternalController> ExternalController::self()
{
    static QSharedPointer<ExternalController> self;

    if (!self)
        self.reset(new ExternalController);

    return self;
}

ExternalController::ExternalController() :
    m_activeWindow(Main),
    m_instrumentId(NoInstrument)
{
}

void ExternalController::connectRMW(RosegardenMainWindow *rmw)
{
    // If it seems like this isn't connecting, it's possible that self()
    // was called before Qt was up.  That will make it impossible for this
    // object to receive signals.

    connect(rmw, &RosegardenMainWindow::documentLoaded,
            this, &ExternalController::slotDocumentLoaded);
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
    //RG_DEBUG << "processEvent(): Active Window:" << m_activeWindow;

    // -- external controller that sends e.g. volume control for each
    // of a number of channels -> if mixer present, use control to adjust
    // tracks on mixer

    // -- external controller that sends e.g. separate controllers on
    // the same channel for adjusting various parameters -> if IPB
    // visible, adjust it.  Should we use the channel to select the
    // track? maybe as an option

    // do we actually need the last active main window for either of
    // these? -- yes, to determine whether to send to mixer or to IPB
    // in the first place.  Send to audio mixer if active, midi mixer
    // if active, plugin dialog if active, otherwise keep it for
    // ourselves for the IPB.  But, we'll do that by having the edit
    // views pass it back to us.

    // -- then we need to send back out to device.

    // CC 81: select window
    // CC 82: select track (see slotExternalController())
    // CC  7: Adjust volume on current track (see slotExternalController())
    // CC 10: Adjust pan on current track (see slotExternalController())
    // CC  x: Adjust that CC on current track (see slotExternalController())

    // These obviously should be configurable.

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

    // Delegate to the currently active track-related window.
    // The idea here is that the user can remotely switch between the
    // three track-related windows (rg main, MIDI Mixer, and Audio Mixer)
    // via controller 81 (above).  Then they can control the tracks
    // on that window via other controllers.

    // The behavior is slightly different between the three windows.
    // The main window uses controller 82 to select a track while
    // the other two use the incoming MIDI channel to select which
    // track will be modified.
    emit externalController(event, m_activeWindow);
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

    StudioControl::sendMappedEvent(event);
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

            ExternalController::send(
                    channel,
                    controlNumber,
                    controlValue);

        }

    }
}

void
ExternalController::slotDocumentLoaded(RosegardenDocument *doc)
{
    if (!doc)
        return;

    // If this is never happening, see the comments in connectRMW() above.

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
    // We only handle updates for RosegardenMainWindow.
    if (m_activeWindow != Main)
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

    //RG_DEBUG << "slotDocumentModified(): Track change detected.";

    // Send out the CCs for the current Track on channel 0.
    sendAllCCs(instrument, 0);
}


}
