// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MIXER_H_
#define _MIXER_H_

#include <kmainwindow.h>
#include "studiowidgets.h"
#include "Instrument.h"
#include "MappedEvent.h"

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }
class SequencerMapper;
class QAccel;

class MixerWindow: public KMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    QAccel* getAccelerators() { return m_accelerators; }

signals:
    void closing();
    void windowActivated();

protected slots:
    void slotClose();

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual void windowActivationChange(bool);

    virtual void sendControllerRefresh() = 0;

    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;
    Rosegarden::InstrumentId m_currentId;

    QAccel *m_accelerators;

};

class AudioMixerWindow : public MixerWindow
{
    Q_OBJECT

public:
    AudioMixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    ~AudioMixerWindow();

    void updateMeters(SequencerMapper *mapper);
    void updateMonitorMeters(SequencerMapper *mapper);

public slots:
    void slotControllerDeviceEventReceived(Rosegarden::MappedEvent *,
					   const void *);

signals:
    void selectPlugin(QWidget *, Rosegarden::InstrumentId id, int index);

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void record();
    void panic();

    // to be redirected to the instrument parameter box if necessary
    void instrumentParametersChanged(Rosegarden::InstrumentId);

protected slots:
    void slotFaderLevelChanged(float level);
    void slotPanChanged(float value);
    void slotInputChanged();
    void slotOutputChanged();
    void slotChannelsChanged();
    void slotSoloChanged();
    void slotMuteChanged();
    void slotRecordChanged();
    void slotSelectPlugin();
    
    // to be called if something changes in an instrument parameter box
    void slotUpdateInstrument(Rosegarden::InstrumentId);

    void slotTrackAssignmentsChanged();

    // from Plugin dialog
    void slotPluginSelected(Rosegarden::InstrumentId id, int index, int plugin);
    void slotPluginBypassed(Rosegarden::InstrumentId id, int pluginIndex, bool bp);

    void slotSetInputCountFromAction();
    void slotSetSubmasterCountFromAction();

    void slotToggleFaders();
    void slotToggleSynthFaders();
    void slotToggleSubmasters();
    void slotTogglePluginButtons();
    void slotToggleUnassignedFaders();

    void slotUpdateFaderVisibility();
    void slotUpdateSynthFaderVisibility();
    void slotUpdateSubmasterVisibility();
    void slotUpdatePluginButtonVisibility();

protected:
    virtual void sendControllerRefresh();

private:

    void toggleNamedWidgets(bool show, const char* const);
    

    // manage the various bits of it in horizontal/vertical slices
    // with other faders:

    struct FaderRec {

	FaderRec() :
	    m_populated(false),
	    m_input(0), m_output(0), m_pan(0), m_fader(0), m_meter(0),
	    m_muteButton(0), m_soloButton(0), m_recordButton(0),
	    m_stereoButton(0), m_stereoness(false), m_pluginBox(0)
	{ }

        void setVisible(bool);
        void setPluginButtonsVisible(bool);
        
	bool m_populated;

	AudioRouteMenu *m_input;
	AudioRouteMenu *m_output;

	RosegardenRotary *m_pan;
	RosegardenFader *m_fader;
	AudioVUMeter *m_meter;

	QPushButton *m_muteButton;
	QPushButton *m_soloButton;
	QPushButton *m_recordButton;
	QPushButton *m_stereoButton;
	bool m_stereoness;

	QVBox *m_pluginBox;
	std::vector<QPushButton *> m_plugins;
    };

    QHBox *m_surroundBox;
    QFrame *m_mainBox;

    typedef std::map<Rosegarden::InstrumentId, FaderRec> FaderMap;
    FaderMap m_faders;

    typedef std::vector<FaderRec> FaderVector;
    FaderVector m_submasters;
    FaderRec m_monitor;
    FaderRec m_master;

    void depopulate();
    void populate();

    bool isInstrumentAssigned(Rosegarden::InstrumentId id);

    void updateFader(int id); // instrument id if large enough, monitor if -1, master/sub otherwise
    void updateRouteButtons(int id);
    void updateStereoButton(int id);
    void updatePluginButtons(int id);
    void updateMiscButtons(int id);

    QPixmap m_monoPixmap;
    QPixmap m_stereoPixmap;
};


class MidiMixerVUMeter : public VUMeter
{
public:
    MidiMixerVUMeter(QWidget *parent = 0,
                     VUMeterType type = Plain,
                     int width = 0,
                     int height = 0,
                     const char *name = 0);

protected:
     virtual void meterStart();
     virtual void meterStop();

private:
     int m_textHeight;

}; 

class MidiMixerWindow : public MixerWindow
{
    Q_OBJECT

public:
    MidiMixerWindow(QWidget *parent, RosegardenGUIDoc *document);

    /**
     * Setup the tabs on the Mixer according to the Studio
     */
    void setupTabs();

    /* 
     * Update the VU meters
     */
    void updateMeters(SequencerMapper *mapper);
    void updateMonitorMeter(SequencerMapper *mapper);

public slots:
    void slotSynchronise(); // synchronise with updated studio

    void slotControllerDeviceEventReceived(Rosegarden::MappedEvent *,
					   const void *);

    void slotCurrentTabChanged(QWidget *);

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
    void instrumentParametersChanged(Rosegarden::InstrumentId);

protected slots:
    void slotUpdateInstrument(Rosegarden::InstrumentId);

    //void slotPanChanged(float);
    void slotFaderLevelChanged(float);
    void slotControllerChanged(float);

protected:
    void addTab(QWidget *tab, const QString &title);

    virtual void sendControllerRefresh();

    QTabWidget                        *m_tabWidget;

    struct FaderStruct {

        FaderStruct():m_id(0), m_vuMeter(0), m_volumeFader(0) {}

        Rosegarden::InstrumentId       m_id;
        MidiMixerVUMeter              *m_vuMeter;
        RosegardenFader               *m_volumeFader;
        std::vector<std::pair<Rosegarden::MidiByte, RosegardenRotary*> > m_controllerRotaries;

    };

    typedef std::vector<FaderStruct*>  FaderVector;
    FaderVector                        m_faders;

    QFrame                            *m_tabFrame;

};

#endif


