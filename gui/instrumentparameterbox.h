// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef _INSTRUMENTPARAMETERBOX_H_
#define _INSTRUMENTPARAMETERBOX_H_

#include "Instrument.h"
#include "MappedEvent.h"
#include "MappedInstrument.h"
#include "MappedCommon.h"

#include "widgets.h"

class QCheckBox;
class QSlider;
class QPushButton;
class QSignalMapper;
class QLabel;
class RosegardenComboBox;
class RosegardenGUIDoc;
class QWidgetStack;
class AudioVUMeter;
class AudioFaderWidget;

namespace Rosegarden
{
    class AudioPluginManager;
    class AudioPluginDialog;
}

class AudioInstrumentParameterPanel;
class MIDIInstrumentParameterPanel;

/**
 * Display and allow modification of Instrument parameters
 */
class InstrumentParameterBox : public RosegardenParameterBox
{
Q_OBJECT

public:
    InstrumentParameterBox(RosegardenGUIDoc *doc,
                           QWidget *parent = 0);
    ~InstrumentParameterBox();

    void useInstrument(Rosegarden::Instrument *instrument);

    Rosegarden::Instrument* getSelectedInstrument()
        { return m_selectedInstrument; }

    void setAudioMeter(double ch1, double ch2);

public slots:

    // To update all InstrumentParamterBoxen for an Instrument.
    //
    void slotUpdateAllBoxes();


signals:

    // Emit a MIDI controller for immediate processing.
    // This is necessary for controlling MIDI devices in
    // real time during playback.
    //
    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);

protected:

    //--------------- Data members ---------------------------------
    QWidgetStack                    *m_widgetStack;
    QFrame                          *m_noInstrumentParameters;
    MIDIInstrumentParameterPanel    *m_midiInstrumentParameters;
    AudioInstrumentParameterPanel   *m_audioInstrumentParameters;

    Rosegarden::Instrument          *m_selectedInstrument;

    // So we can setModified()
    //
    RosegardenGUIDoc                *m_doc;

};

// Global references
//
static std::vector<InstrumentParameterBox*> instrumentParamBoxes;

////////////////////////////////////////////////////////////////////////

class InstrumentParameterPanel : public QFrame
{
    Q_OBJECT
public:
    InstrumentParameterPanel(QWidget* parent);

    virtual ~InstrumentParameterPanel() {};

    virtual void setupForInstrument(Rosegarden::Instrument*) = 0;

signals:
    void updateAllBoxes();
    
protected:
    //--------------- Data members ---------------------------------
    QLabel                          *m_instrumentLabel;
    Rosegarden::Instrument          *m_selectedInstrument;
};


class AudioInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:
    AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent);

    virtual void setupForInstrument(Rosegarden::Instrument*);

    // Set the audio meter to a given level for a maximum of
    // two channels.
    //
    void setAudioMeter(double ch1, double ch2);

    // Set the button colour
    //
    void setBypassButtonColour(int pluginIndex, bool bypassState);

public slots:
    // From AudioFaderWidget
    //
    void slotSelectPlugin(int index);
    void slotSelectAudioLevel(int index);
    void slotAudioChannels(int channels);
    void slotMute();
    void slotSolo();
    void slotSetPan(float pan);

    // From Plugin dialog
    //
    void slotPluginSelected(int index, int plugin);
    void slotPluginPortChanged(int pluginIndex, int portIndex, float value);
    void slotBypassed(int pluginIndex, bool bp);
    void slotPluginDialogDestroyed(int index);

protected:
    //--------------- Data members ---------------------------------

    AudioFaderWidget   *m_audioFader; // audio fader

    std::map<int, Rosegarden::AudioPluginDialog*> m_pluginDialogs;

    Rosegarden::AudioPluginManager               *m_pluginManager;

private:

    QPixmap                                      m_monoPixmap;
    QPixmap                                      m_stereoPixmap;

};

class MIDIInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:

    MIDIInstrumentParameterPanel(QWidget* parent);

    virtual void setupForInstrument(Rosegarden::Instrument*);

signals:

    // Emit a MIDI controller for immediate processing.
    // This is necessary for controlling MIDI devices in
    // real time during playback.
    //
    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);

public slots:
    void slotSelectProgram(int index);
    void slotSelectBank(int index);
    void slotSelectChannel(int index);
    void slotSelectPan(float index);
    void slotSelectVolume(float index);

    void slotSelectChorus(float index);
    void slotSelectReverb(float index);
    void slotSelectHighPass(float index);
    void slotSelectResonance(float index);
    void slotSelectAttack(float index);
    void slotSelectRelease(float index);

    void slotActivateProgramChange(bool value);
    void slotActivateBank(bool value);

protected:

    void populateProgramList();

    //--------------- Data members ---------------------------------

    QLabel             *m_deviceLabel;

    RosegardenComboBox *m_bankValue;
    RosegardenComboBox *m_channelValue; 
    RosegardenComboBox *m_programValue;
    RosegardenRotary   *m_panRotary;
    RosegardenRotary   *m_volumeRotary;

    QCheckBox          *m_bankCheckBox;
    QCheckBox          *m_programCheckBox;

    RosegardenRotary   *m_chorusRotary;
    RosegardenRotary   *m_reverbRotary;
    RosegardenRotary   *m_highPassRotary;
    RosegardenRotary   *m_resonanceRotary;
    RosegardenRotary   *m_attackRotary;
    RosegardenRotary   *m_releaseRotary;
};


#endif // _INSTRUMENTPARAMETERBOX_H_
