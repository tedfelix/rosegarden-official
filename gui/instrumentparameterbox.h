// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "MidiProgram.h"

#include "widgets.h"

class QCheckBox;
class QSlider;
class QPushButton;
class QSignalMapper;
class QLabel;
class KComboBox;
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

    // If currently showing this track's instrument then toggle
    // the mute button on the display
    //
    void setMute(bool value);
    void setSolo(bool value);
    void setRecord(bool value);

public slots:

    // To update all InstrumentParameterBoxen for an Instrument.
    //
    void slotUpdateAllBoxes();

signals:

    // Emit a MIDI controller for immediate processing.
    // This is necessary for controlling MIDI devices in
    // real time during playback.
    //
    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);

    void setMute(Rosegarden::InstrumentId, bool value);
    void setSolo(Rosegarden::InstrumentId, bool value);
    void setRecord(Rosegarden::InstrumentId, bool value);

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
    InstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent);

    virtual ~InstrumentParameterPanel() {};

    virtual void setupForInstrument(Rosegarden::Instrument*) = 0;

signals:
    void updateAllBoxes();
    
protected:
    //--------------- Data members ---------------------------------
    QLabel                          *m_instrumentLabel;
    Rosegarden::Instrument          *m_selectedInstrument;
    RosegardenGUIDoc                *m_doc;
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

    void setMute(bool value);

public slots:
    // From AudioFaderWidget
    //
    void slotSelectPlugin(int index);
    void slotSelectAudioLevel(int index);
    void slotAudioChannels(int channels);
    void slotSelectAudioInput(int value);

    // From the parameter box clicks
    void slotMute();
    void slotSolo();
    void slotRecord();

    void slotSetPan(float pan);

    // External things set our button states here
    //
    void slotSetMute(bool value);
    void slotSetSolo(bool value);
    void slotSetRecord(bool value);

    // From Plugin dialog
    //
    void slotPluginSelected(int index, int plugin);
    void slotPluginPortChanged(int pluginIndex, int portIndex, float value);
    void slotBypassed(int pluginIndex, bool bp);
    void slotPluginDialogDestroyed(int index);

signals:
    void muteButton(Rosegarden::InstrumentId, bool state);
    void soloButton(Rosegarden::InstrumentId, bool state);
    void recordButton(Rosegarden::InstrumentId, bool state);

protected:
    //--------------- Data members ---------------------------------

    AudioFaderWidget   *m_audioFader; // audio fader

    std::map<int, Rosegarden::AudioPluginDialog*> m_pluginDialogs;

private:

    QPixmap                                      m_monoPixmap;
    QPixmap                                      m_stereoPixmap;

};

class MIDIInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:

    MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent);

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
    void slotSelectVariation(int index);
    void slotSelectChannel(int index);
    void slotSelectPan(float index);
    void slotSelectVolume(float index);

    void slotSelectChorus(float index);
    void slotSelectReverb(float index);
    void slotSelectHighPass(float index);
    void slotSelectResonance(float index);
    void slotSelectAttack(float index);
    void slotSelectRelease(float index);

    void slotTogglePercussion(bool value);
    void slotToggleProgramChange(bool value);
    void slotToggleBank(bool value);
    void slotToggleVariation(bool value);

protected:

    // fill (or hide) bank combo based on whether the instrument is percussion
    void populateBankList();

    // fill program combo based on current bank
    void populateProgramList();

    // fill (or hide) variation combo based on current bank and program
    void populateVariationList();

    // send the bank and program events relevant to this instrument
    void sendBankAndProgram();

    //--------------- Data members ---------------------------------

    QLabel             *m_connectionLabel;

    KComboBox          *m_bankValue;
    KComboBox          *m_variationValue;
    KComboBox          *m_channelValue;
    KComboBox          *m_programValue;
    RosegardenRotary   *m_panRotary;
    RosegardenRotary   *m_volumeRotary;

    QCheckBox          *m_percussionCheckBox;
    QCheckBox          *m_bankCheckBox;
    QCheckBox          *m_variationCheckBox;
    QCheckBox          *m_programCheckBox;

    QLabel             *m_bankLabel;
    QLabel             *m_variationLabel;

    RosegardenRotary   *m_chorusRotary;
    RosegardenRotary   *m_reverbRotary;
    RosegardenRotary   *m_highPassRotary;
    RosegardenRotary   *m_resonanceRotary;
    RosegardenRotary   *m_attackRotary;
    RosegardenRotary   *m_releaseRotary;

    std::vector<Rosegarden::MidiBank>    m_banks;
    std::vector<Rosegarden::MidiProgram> m_programs;
    Rosegarden::MidiByteList             m_variations;
};


#endif // _INSTRUMENTPARAMETERBOX_H_
