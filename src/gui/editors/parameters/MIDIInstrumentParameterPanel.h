/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2014 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIINSTRUMENTPARAMETERPANEL_H
#define RG_MIDIINSTRUMENTPARAMETERPANEL_H

#include "base/MidiProgram.h"  // InstrumentId
#include "base/MidiDevice.h"  // MidiByteList

#include "InstrumentParameterPanel.h"

#include <QString>


class QWidget;
class QSignalMapper;
class QLabel;
class QGridLayout;
class QFrame;
class QCheckBox;
class QComboBox;

namespace Rosegarden
{

class RosegardenDocument;
class MidiDevice;
class Instrument;
class Rotary;


class MIDIInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:

    MIDIInstrumentParameterPanel(RosegardenDocument *doc, QWidget* parent);

    /// Change the Instrument that is being displayed.
    /**
     * Called each time the selected track changes.
     */
    void setupForInstrument(Instrument *);

    /// Uncheck the Receive External checkbox.
    /**
     * Called by RosegardenMainViewWidget::slotUpdateInstrumentParameterBox()
     * to clear the "Receive External" checkbox when the user selects a
     * different track.
     */
    void clearReceiveExternal();

    /// Does nothing.
    /**
     * This appears to have provided a way to reduce the number of rotaries
     * displayed on the panel.  However, it now just makes all of them
     * visible.  Recommend removing this routine.  Verify that all rotaries
     * are shown at start.
     */
    void showAdditionalControls(bool showThem);

signals:
    /// Emitted for almost any parameter change.
    /**
     * Connected to InstrumentParameterBox::changeInstrumentLabel().
     *
     * Oddly, never emitted for setupForInstrument(), the one time the
     * instrument and label might actually change.
     */
    void changeInstrumentLabel(InstrumentId id, QString label);

    /// Emitted for almost any parameter change.
    /**
     * Connected to InstrumentParameterBox::instrumentParametersChanged().
     */
    void instrumentParametersChanged(InstrumentId);

public slots:
    /// Handle external Bank Selects and Program Changes.
    /**
     * When a MIDI Bank Select or Program Change comes in from an external
     * MIDI device, this function updates the bank and program in the
     * instrument and then displays the new values.  The "Receive External"
     * checkbox must be checked for this to work.
     *
     * This slot is connected in RosegardenMainWindow's ctor to
     * SequenceManager::signalSelectProgramNoSend().
     *
     * Note: This function's parameters are in reverse order.  They should be:
     *       slotExternalProgramChange(bankMSB, bankLSB, programChange)
     *       This would require changing
     *       SequenceManager::signalSelectProgramNoSend() as well.
     *
     * parameters:
     * programChange : the program to select (triggered by Program
     *                 Change message)
     * bankLSB : the bank to select (-1 if no LSB Bank Select occurred)
     *           (triggered by LSB Bank Select message)
     * bankMSB : the bank to select (-1 if no MSB Bank Select occurred)
     *           (triggered by MSB Bank Select message)
     */
    void slotExternalProgramChange(
            int programChange, int bankLSB, int bankMSB);

private slots:
    void slotTogglePercussion(bool value);
    void slotSelectBank(int index);
    void slotToggleBank(bool value);
    void slotSelectProgram(int index);
    void slotToggleProgramChange(bool value);
    // Channel: auto/fixed
    void slotSetUseChannel(int index);

    void slotSelectVariation(int index);
    void slotToggleVariation(bool value);

    void slotControllerChanged(int index);

private:

    // fill (or hide) bank combo based on whether the instrument is percussion
    void populateBankList();

    // fill program combo based on current bank
    void populateProgramList();

    // fill (or hide) variation combo based on current bank and program
    void populateVariationList();

    // Fill the fixed channel list controls
    void populateChannelList();

    /// Create or update the rotary controls for each controller.
    void setupControllers(MidiDevice *);

    // get value of a specific rotary (keyed by controller value)
    int getValueFromRotary(int rotary);

    // set rotary to value
    void setRotaryToValue(int controller, int value);

    //--------------- Data members ---------------------------------

    QGridLayout        *m_mainGrid;

    // m_instrumentLabel is inherited from InstrumentParameterPanel.

    QLabel             *m_connectionLabel;

    QCheckBox          *m_percussionCheckBox;

    QLabel             *m_bankLabel;
    QCheckBox          *m_bankCheckBox;
    QComboBox          *m_bankValue;
    BankList            m_banks;

    QLabel             *m_programLabel;
    QCheckBox          *m_programCheckBox;
    QComboBox          *m_programValue;
    ProgramList         m_programs;

    QLabel             *m_variationLabel;
    QCheckBox          *m_variationCheckBox;
    QComboBox          *m_variationValue;
    MidiByteList        m_variations;

    // Channel: auto/fixed
    QComboBox          *m_channelValue;

    QLabel             *m_receiveExternalLabel;
    QCheckBox          *m_receiveExternalCheckBox;

    QFrame             *m_rotaryFrame;
    QGridLayout        *m_rotaryGrid;

    struct RotaryInfo
    {
        Rotary *rotary;
        QLabel *label;
        int controller;
    };
    typedef std::vector<RotaryInfo> RotaryInfoVector;
    RotaryInfoVector    m_rotaries;

    QSignalMapper      *m_rotaryMapper;
};



}

#endif
