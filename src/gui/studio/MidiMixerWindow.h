
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

    /// Handle InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

    //void slotPanChanged(float);
    void slotFaderLevelChanged(float);
    void slotControllerChanged(float);

    void slotCurrentTabChanged(int);

    /// Handle events from the external controller port.
    /**
     * @see RosegardenMainViewWidget::slotExternalController()
     * @see AudioMixerWindow2::slotExternalController()
     */
    void slotExternalController(const MappedEvent *event);

    void slotHelpRequested();
    void slotHelpAbout();

    void slotClose()  { close(); }

private:
    /// Setup the tabs on the Mixer according to the Studio
    void setupTabs();

    void changeEvent(QEvent *event) override;

    void addTab(QWidget *tab, const QString &title);

    void sendControllerRefresh() override;

    QTabWidget *m_tabWidget;

    // ??? rename: MidiStrip?  See AudioStrip.
    struct FaderStruct {

        FaderStruct():m_id(0), m_vuMeter(nullptr), m_volumeFader(nullptr) {}

        InstrumentId m_id;
        MidiMixerVUMeter *m_vuMeter;
        Fader *m_volumeFader;
        std::vector<std::pair<MidiByte, Rotary *>> m_controllerRotaries;

    };

    typedef std::vector<std::shared_ptr<FaderStruct>> FaderVector;
    FaderVector m_faders;

    QFrame *m_tabFrame;

    // Grab IPB controls and remove Volume.
    ControlList getIPBForMidiMixer(MidiDevice *) const;

    /**
     * ??? This should not take an Instrument *.  It should update the
     *     widgets for all Instruments.
     */
    void updateWidgets(Instrument *);

};


}

#endif
