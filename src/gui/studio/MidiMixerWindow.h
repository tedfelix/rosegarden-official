
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIMIXERWINDOW_H
#define RG_MIDIMIXERWINDOW_H

#include "MixerWindow.h"

#include "base/Controllable.h"
#include "gui/general/ActionFileClient.h"

#include <memory>
#include <utility>
#include <vector>

class QWidget;
class QTabWidget;
class QString;
class QFrame;


namespace Rosegarden
{


class Fader;
class MappedEvent;
class MidiDevice;
class MidiMixerVUMeter;
class RosegardenDocument;
class Rotary;


/// The MIDI Mixer.
class MidiMixerWindow : public MixerWindow, public ActionFileClient
{
    Q_OBJECT

public:

    MidiMixerWindow(QWidget *parent, RosegardenDocument *document);

    /// Called by RosegardenMainWindow::slotUpdateUI()
    /**
     * ??? AudioMixerWindow2 has no such routine.  Why not?  Looks like
     *     AudioStrip has its own timer.  Need to move in that direction.
     */
    void updateMeters();
    /// Called by RosegardenMainWindow::slotUpdateMonitoring()
    /**
     * ??? AudioMixerWindow2 has no such routine.  Why not?  Looks like
     *     AudioStrip has its own timer.  Need to move in that direction.
     */
    void updateMonitorMeter();

signals:

    // ??? Get rid of these and do what AudioMixerWindow does.  Connect the
    //     actions directly to RMW.  See AudioMixerWindow2's ctor.
    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void record();
    void panic();

public slots:

    /// Used by DeviceManagerDialog to update Device names.
    /**
     * ??? rename: slotUpdateDeviceNames()?
     * ??? Actually this routine is EMPTY!!!  Can we try the use case it
     *     appears to be related to and see if there is still a problem?
     */
    void slotSynchronise();

private slots:

    /// Update the strip to match changes to the Instrument.
    /**
     * Connected to InstrumentStaticSignals::controlChange().
     */
    void slotControlChange(Instrument *instrument, int cc);

    void slotFaderLevelChanged(float value);
    void slotControllerChanged(float value);

    /// Calls sendControllerRefresh().
    void slotCurrentTabChanged(int);

    /// Handle events from the external controller port.
    /**
     * @see RosegardenMainViewWidget::slotExternalController()
     * @see AudioMixerWindow2::slotExternalController()
     */
    void slotExternalController(const MappedEvent *event);

    /// Help > Help
    void slotHelpRequested();
    /// Help > About Rosegarden
    void slotHelpAbout();

    /// File > Close
    void slotClose()  { close(); }

private:

    QTabWidget *m_tabWidget;
    QFrame *m_tabFrame;

    /// Setup the tabs on the Mixer according to the Studio
    void setupTabs();
    void updateWidgets(const Instrument *i_instrument);

    // QWidget override.
    /// Calls sendControllerRefresh() in response to window activation.
    void changeEvent(QEvent *event) override;

    /// Send MIDI volume and pan messages to the "external controller" port.
    /**
     * This is called when the window is activated or the tab is changed.  It
     * allows the device connected to the "external controller" port to stay in
     * sync with whichever Mixer window is active.
     */
    void sendControllerRefresh();

    struct MidiStrip {
        InstrumentId m_id{0};
        MidiMixerVUMeter *m_vuMeter{nullptr};
        Fader *m_volumeFader{nullptr};
        // ??? STRUCT!!!
        std::vector<std::pair<MidiByte /*controllerNumber*/, Rotary *>>
                m_controllerRotaries;
    };

    typedef std::vector<std::shared_ptr<MidiStrip>> MidiStripVector;
    MidiStripVector m_midiStrips;

    /// Get InstrumentParameterBox controllers and remove volume.
    ControlList getIPBControlParameters(const MidiDevice *) const;

};


}

#endif
