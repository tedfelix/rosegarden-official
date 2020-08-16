
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#include "base/MidiDevice.h"
#include "gui/general/ActionFileClient.h"
#include "MixerWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"

#include <QSharedPointer>
#include <vector>


class QWidget;
class QTabWidget;
class QString;
class QFrame;


namespace Rosegarden
{

class Rotary;
class RosegardenDocument;
class MidiMixerVUMeter;
class MappedEvent;
class Fader;
class InstrumentStaticSignals;


class MidiMixerWindow : public MixerWindow, public ActionFileClient
{
    Q_OBJECT

public:
    MidiMixerWindow(QWidget *parent, RosegardenDocument *document);

    /**
     * Setup the tabs on the Mixer according to the Studio
     */
    void setupTabs();

    /* 
     * Update the VU meters
     */
    void updateMeters();
    void updateMonitorMeter();

public slots:
    void slotSynchronise(); // synchronise with updated studio

    /// Handle events from the external controller port.
    /**
     * @see RosegardenMainViewWidget::slotExternalController()
     * @see AudioMixerWindow2::slotExternalController()
     */
    void slotExternalController(MappedEvent *,
            RosegardenMainViewWidget::ExternalControllerWindow window);

    void slotCurrentTabChanged(int);
    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void record();
    void panic();

protected slots:
    /// Handle InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

    //void slotPanChanged(float);
    void slotFaderLevelChanged(float);
    void slotControllerChanged(float);

protected:
    void changeEvent(QEvent *event) override;

    void addTab(QWidget *tab, const QString &title);

    void sendControllerRefresh() override;

    QTabWidget *m_tabWidget;

    struct FaderStruct {

        FaderStruct():m_id(0), m_vuMeter(nullptr), m_volumeFader(nullptr) {}

        InstrumentId m_id;
        MidiMixerVUMeter *m_vuMeter;
        Fader *m_volumeFader;
        std::vector<std::pair<MidiByte, Rotary*> > m_controllerRotaries;

    };

    typedef std::vector<FaderStruct*>  FaderVector;
    FaderVector m_faders;

    QFrame *m_tabFrame;

    // Grab IPB controls and remove Volume.
    ControlList getIPBForMidiMixer(MidiDevice *) const;

private:
    /**
     * ??? This should not take an Instrument *.  It should update the
     *     widgets for all Instruments.
     */
    void updateWidgets(Instrument *);

};


}

#endif
