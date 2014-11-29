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
#include "gui/widgets/SqueezedLabel.h"

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

    MIDIInstrumentParameterPanel(RosegardenDocument *doc, QWidget *parent);

    /// Update all widgets from the Instrument.
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
     * When the "Receive External" checkbox is checked, this routine takes
     * incoming Bank Select and Program Changes, and updates the instrument's
     * bank and program based on those.  This allows the user to quickly set
     * the bank/program from a MIDI device instead of sifting through the
     * typically very arbitrarily ordered bank and program changes in
     * the comboboxes.
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
    /// Handle m_percussionCheckBox clicked()
    void slotPercussionClicked(bool checked);

    /// Handle m_bankCheckBox clicked()
    void slotBankClicked(bool checked);
    /// Handle m_bankComboBox activated()
    void slotSelectBank(int index);

    /// Handle m_programCheckBox clicked()
    void slotProgramClicked(bool checked);
    /// Handle m_programComboBox activated()
    void slotSelectProgram(int index);

    /// Handle m_variationCheckBox clicked()
    void slotVariationClicked(bool checked);
    /// Handle m_variationComboBox activated()
    void slotSelectVariation(int index);

    /// Handle m_channelValue activated()
    void slotSelectChannel(int index);

    /// Handle a rotary change (m_rotaryMapper mapped())
    void slotControllerChanged(int index);

private:

    QGridLayout        *m_mainGrid;

    // m_instrumentLabel is inherited from InstrumentParameterPanel.

    QLabel             *m_connectionLabel;

    QCheckBox          *m_percussionCheckBox;

    // Bank
    QLabel             *m_bankLabel;
    QCheckBox          *m_bankCheckBox;
    QComboBox          *m_bankComboBox;
    BankList            m_banks;
    /// From the selected instrument.
    void updateBankComboBox();

    // Program
    QLabel             *m_programLabel;
    QCheckBox          *m_programCheckBox;
    QComboBox          *m_programComboBox;
    ProgramList         m_programs;
    /// From the selected instrument.
    void updateProgramComboBox();

    // Variation
    QLabel             *m_variationLabel;
    // ??? See comments in slotVariationClicked() related to getting
    //     rid of this checkbox.
    QCheckBox          *m_variationCheckBox;
    QComboBox          *m_variationComboBox;
    ProgramList         m_variations;
    /// From the selected instrument.
    void updateVariationComboBox();

    // Channel: auto/fixed
    QComboBox          *m_channelValue;

    QLabel             *m_receiveExternalLabel;
    QCheckBox          *m_receiveExternalCheckBox;

    // Rotaries

    QFrame             *m_rotaryFrame;
    QGridLayout        *m_rotaryGrid;

    struct RotaryInfo
    {
        Rotary *rotary;
        SqueezedLabel *label;
        int controller;
    };
    // ??? getValueFromRotary() and setRotaryToValue() would benefit
    //     from this being a std::map indexed by controller.
    typedef std::vector<RotaryInfo> RotaryInfoVector;
    RotaryInfoVector    m_rotaries;

    QSignalMapper      *m_rotaryMapper;

    /// Create or update the rotary controls for each controller.
    void setupControllers(MidiDevice *);

    int getValueFromRotary(int controller);
    void setRotaryToValue(int controller, int value);
};



}

#endif
