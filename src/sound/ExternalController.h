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

#pragma once

#include "base/Instrument.h"
#include "base/MidiProgram.h"  // For MidiByte

#include <QObject>
#include <QSharedPointer>


namespace Rosegarden
{


class MappedEvent;
class RosegardenDocument;
class RosegardenMainWindow;


/// Support for the "external controller" port.
/**
 * The external controller port allows MIDI control surfaces to control
 * the main window, MIDI Mixer window, and Audio Mixer window.
 *
 * To use, be sure to enable the external controller port in the preferences:
 *
 *   Edit > Preferences... > MIDI > General tab > External controller port
 *   checkbox.
 *
 * After a restart of Rosegarden, an "external controller" port will
 * appear for MIDI control surfaces to connect to.
 *
 * The current window can be selected via CC81.  See processEvent().
 *
 * Depending on the current active window, the behavior of the external
 * controller port will change.  For the main window, the following features
 * are available via control changes on *channel 1*:
 *
 *   CC 82: select Track
 *   CC  7: Adjust volume on current Track
 *   CC 10: Adjust pan on current track
 *   CC  x: Adjust that CC on current track
 *
 * See RosegardenMainViewWidget::slotExternalController() for details.
 *
 * For the MIDI Mixer window and Audio Mixer window, CCs can be sent on
 * any of the 16 channels to control those channels on the mixer.  See
 * MidiMixerWindow::slotExternalController() and
 * AudioMixerWindow2::slotExternalController() for details.
 *
 * CCs are also sent out the external controller port to allow motorized
 * control surfaces to stay in sync with the current state of the CCs.
 *
 */
class ExternalController : public QObject
{
    Q_OBJECT

public:
    /// The global instance.
    static QSharedPointer<ExternalController> self();

    static bool isEnabled();

    /// Call this from RosegardenMainWindow's ctor.
    /**
     * This has to be called at the right moment, before the autoload
     * occurs.  Otherwise the very first RMW::documentLoaded() will not get
     * in here.
     */
    void connectRMW(RosegardenMainWindow *rmw);

    /// The three windows that currently handle external controller events.
    enum Window { Main, AudioMixer, MidiMixer };
    /// The currently active window for external controller events.
    /**
     * External controller events are forwarded to the currently active
     * window.
     *
     * Set by the three windows that can handle external controller events.
     */
    Window activeWindow;

    /// Handle MappedEvent's from the external controller port.
    /**
     * This routine handles remote control events received from a
     * device connected to the "external controller" port.
     *
     * This routine handles controller 81 which opens or
     * brings to the top various windows based on the value.
     *
     * All other events are forwarded to the active window.
     * See m_activeWindow.
     */
    void processEvent(const MappedEvent *event);

    /// Send a control change message out the external controller port.
    static void send(MidiByte channel, MidiByte controlNumber, MidiByte value);
    static void sendAllCCs(
            const Instrument *instrument, MidiByte channel = MidiMaxValue);

signals:

    /// Connected to RosegardenMainViewWidget::slotExternalController().
    void externalControllerRMVW(const MappedEvent *event);
    /// Connected to MidiMixerWindow::slotExternalController().
    void externalControllerMMW(const MappedEvent *event);
    /// Connected to AudioMixerWindow::slotExternalController().
    void externalControllerAMW2(const MappedEvent *event);

private slots:

    /// Connected to RMW::documentLoaded()
    void slotDocumentLoaded(RosegardenDocument *doc);
    /// Connected to RD::documentModified()
    void slotDocumentModified(bool);

    /// Connected to InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

private:

    // Access through self() only.
    ExternalController();

    /// Cache of the last Instrument we were tracking for RosegardenMainWindow.
    InstrumentId m_instrumentId;

};


}