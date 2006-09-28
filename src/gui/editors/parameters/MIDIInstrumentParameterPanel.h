
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MIDIINSTRUMENTPARAMETERPANEL_H_
#define _RG_MIDIINSTRUMENTPARAMETERPANEL_H_

#include "base/MidiProgram.h"
#include "InstrumentParameterPanel.h"
#include <qstring.h>


class QWidget;
class QSignalMapper;
class QLabel;
class QGridLayout;
class QFrame;
class QCheckBox;
class KComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;
class MidiDevice;
class Instrument;


class MIDIInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:

    MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent);

    void setupControllers(MidiDevice *); // setup ControlParameters on box

    virtual void setupForInstrument(Instrument*);

    void showAdditionalControls(bool showThem);

signals:
    void changeInstrumentLabel(InstrumentId id, QString label);
    void instrumentParametersChanged(InstrumentId);

public slots:
    void slotSelectProgram(int index);
    void slotSelectBank(int index);
    void slotSelectVariation(int index);
    void slotSelectChannel(int index);
    //void slotSelectInputChannel(int index);

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
    //KComboBox          *m_channelInValue;

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

    BankList       m_banks;
    ProgramList    m_programs;
    MidiByteList   m_variations;
};



}

#endif
