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

#ifndef _INSTRUMENTPARAMETERBOX_H_
#define _INSTRUMENTPARAMETERBOX_H_

#include <vector>

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
class QVBoxLayout;
class KComboBox;
class RosegardenGUIDoc;
class QWidgetStack;
class AudioVUMeter;
class AudioFaderBox;

namespace Rosegarden
{
    class AudioPluginManager;
    class AudioPluginDialog;
}

class AudioInstrumentParameterPanel;
class MIDIInstrumentParameterPanel;

typedef std::pair<RosegardenRotary *, QLabel *> RotaryPair;
typedef std::vector<std::pair<int, RotaryPair> > RotaryMap;

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

    void setAudioMeter(float dBleft, float dBright,
		       float recDBleft, float recDBright);

    void setDocument(RosegardenGUIDoc* doc);

public slots:

    // To update all InstrumentParameterBoxen for an Instrument.  Called
    // from one of the parameter panels when something changes.
    //
    void slotUpdateAllBoxes();

    // Update InstrumentParameterBoxes that are showing a given instrument.
    // Called from the Outside.
    //
    void slotInstrumentParametersChanged(Rosegarden::InstrumentId id);

    // From Plugin dialog
    //
    void slotPluginSelected(Rosegarden::InstrumentId id, int index, int plugin);
    void slotPluginBypassed(Rosegarden::InstrumentId id, int pluginIndex, bool bp);

signals:

    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);

    void selectPlugin(QWidget*, Rosegarden::InstrumentId id, int index);
    void showPluginGUI(Rosegarden::InstrumentId id, int index);

    void instrumentParametersChanged(Rosegarden::InstrumentId);

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

    void setDocument(RosegardenGUIDoc* doc);

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
    void setAudioMeter(float dBleft, float dBright,
		       float recDBleft, float recDBright);

    // Set the button colour
    //
    void setButtonColour(int pluginIndex, bool bypassState, 
                         const QColor &color);

public slots:
    // From AudioFaderBox
    //
    void slotSelectAudioLevel(float dB);
    void slotSelectAudioRecordLevel(float dB);
    void slotAudioChannels(int channels);
    void slotAudioRoutingChanged();
    void slotSelectPlugin(int index);

    // From the parameter box clicks
    void slotSetPan(float pan);

    // From Plugin dialog
    //
    void slotPluginSelected(Rosegarden::InstrumentId id, int index, int plugin);
    void slotPluginBypassed(Rosegarden::InstrumentId id, int pluginIndex, bool bp);

    void slotSynthButtonClicked();
    void slotSynthGUIButtonClicked();

signals:
    void selectPlugin(QWidget *, Rosegarden::InstrumentId, int index);
    void instrumentParametersChanged(Rosegarden::InstrumentId);
    void showPluginGUI(Rosegarden::InstrumentId, int index);

protected:
    //--------------- Data members ---------------------------------

    AudioFaderBox   *m_audioFader;

private:

    QPixmap                                      m_monoPixmap;
    QPixmap                                      m_stereoPixmap;

};

class MIDIInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:

    MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent);

    void setupControllers(Rosegarden::MidiDevice *); // setup ControlParameters on box

    virtual void setupForInstrument(Rosegarden::Instrument*);

signals:
    void changeInstrumentLabel(Rosegarden::InstrumentId id, QString label);
    void instrumentParametersChanged(Rosegarden::InstrumentId);

public slots:
    void slotSelectProgram(int index);
    void slotSelectBank(int index);
    void slotSelectVariation(int index);
    void slotSelectChannel(int index);

    void slotControllerChanged(int index);

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

    // get value of a specific rotary (keyed by controller value)
    int getValueFromRotary(int rotary);

    // set rotary to value
    void setRotaryToValue(int controller, int value);

    //--------------- Data members ---------------------------------

    QLabel             *m_connectionLabel;

    KComboBox          *m_bankValue;
    KComboBox          *m_variationValue;
    KComboBox          *m_channelValue;
    KComboBox          *m_programValue;

    QCheckBox          *m_percussionCheckBox;
    QCheckBox          *m_bankCheckBox;
    QCheckBox          *m_variationCheckBox;
    QCheckBox          *m_programCheckBox;

    QLabel             *m_bankLabel;
    QLabel             *m_variationLabel;
    QLabel             *m_programLabel;

    QGridLayout        *m_mainGrid;
    QFrame             *m_rotaryFrame;
    QGridLayout        *m_rotaryGrid;
    RotaryMap           m_rotaries;
    QSignalMapper      *m_rotaryMapper;

    Rosegarden::BankList       m_banks;
    Rosegarden::ProgramList    m_programs;
    Rosegarden::MidiByteList   m_variations;
};


#endif // _INSTRUMENTPARAMETERBOX_H_
