
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MIDIMIXERWINDOW_H_
#define _RG_MIDIMIXERWINDOW_H_

#include "base/MidiDevice.h"
#include "gui/general/ActionFileClient.h"
#include "MixerWindow.h"
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

    void slotControllerDeviceEventReceived(MappedEvent *,
                                           const void *);

    void slotCurrentTabChanged(QWidget *);
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

    // to be redirected to the instrument parameter box if necessary
    void instrumentParametersChanged(InstrumentId);

protected slots:
    void slotUpdateInstrument(InstrumentId);

    //void slotPanChanged(float);
    void slotFaderLevelChanged(float);
    void slotControllerChanged(float);

protected:
    void addTab(QWidget *tab, const QString &title);

    virtual void sendControllerRefresh();

    QTabWidget *m_tabWidget;

    struct FaderStruct {

        FaderStruct():m_id(0), m_vuMeter(0), m_volumeFader(0) {}

        InstrumentId m_id;
        MidiMixerVUMeter *m_vuMeter;
        Fader *m_volumeFader;
        std::vector<std::pair<MidiByte, Rotary*> > m_controllerRotaries;

    };

    typedef std::vector<FaderStruct*>  FaderVector;
    FaderVector m_faders;

    QFrame *m_tabFrame;

    void  setRewFFwdToAutoRepeat();

    // Grab IPB controls and remove Volume.
    ControlList getIPBForMidiMixer(MidiDevice *) const;

};


}

#endif
