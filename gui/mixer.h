// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }
class SequencerMapper;

class MixerWindow: public KMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenGUIDoc *document);

signals:
    void closing();

protected:
    virtual void closeEvent(QCloseEvent *);

    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;
    Rosegarden::InstrumentId m_currentId;
};

class AudioMixerWindow : public MixerWindow
{
    Q_OBJECT

public:
    AudioMixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    ~AudioMixerWindow();

    void updateMeters(SequencerMapper *mapper);

signals:
    void selectPlugin(QWidget *, Rosegarden::InstrumentId id, int index);

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();

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
    void slotClose();
    
    // to be called if something changes in an instrument parameter box
    void slotUpdateInstrument(Rosegarden::InstrumentId);

    void slotTrackAssignmentsChanged();

    // from Plugin dialog
    void slotPluginSelected(Rosegarden::InstrumentId id, int index, int plugin);
    void slotPluginBypassed(Rosegarden::InstrumentId id, int pluginIndex, bool bp);

    void slotSetInputCountFromAction();
    void slotSetSubmasterCountFromAction();

    void slotShowFaders();
    void slotShowSubmasters();
    void slotShowPluginButtons();
    void slotShowUnassignedFaders();

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

class MidiMixerWindow : public MixerWindow
{
    Q_OBJECT

public:
    MidiMixerWindow(QWidget *parent, RosegardenGUIDoc *document);

    void addTab(QWidget *tab, const QString &title);

protected:

    QTabWidget    *m_tabWidget;

};

#endif
