/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIPROGRAMSEDITOR_H
#define RG_MIDIPROGRAMSEDITOR_H

#include "base/MidiProgram.h"  // MidiBank
#include "NameSetEditor.h"

class QWidget;
class QString;
class QSpinBox;
class QTreeWidgetItem;
class QCheckBox;


namespace Rosegarden
{


class MidiDevice;
class BankEditorDialog;
class MidiBankTreeWidgetItem;


class MidiProgramsEditor : public NameSetEditor
{
    Q_OBJECT

public:

    MidiProgramsEditor(BankEditorDialog *bankEditor,
                       QWidget *parent);

    /// Switch to the cleared and disabled state.
    /**
     * Called at the end of BankEditorDialog::updateEditor() if no valid
     * bank or program is selected in the tree.
     */
    void clearAll();

    /// Show the programs for the selected bank.
    void populate(const MidiBankTreeWidgetItem *bankItem);

private slots:

    /// Check that any new MSB/LSB combination is unique for this device.
    void slotNewMSB(int value);
    void slotNewLSB(int value);
    void slotPercussionClicked();

    /// One of the program names was changed by the user.
    void slotNameChanged(const QString &) override;
    void slotEditingFinished() override;
    void slotKeyMapButtonPressed() override;
    void slotKeyMapMenuItemSelected(QAction *action);

private:

    MidiDevice *m_device{nullptr};

    // Widgets

    QCheckBox *m_percussion;
    QSpinBox *m_msb;
    QSpinBox *m_lsb;

    // Banks

    /// The bank we are editing right now.
    /**
     * We edit the bank in place here, then commit the changes in
     * slotEditingFinished().
     */
    MidiBank m_currentBank;

    /// Find bank and programNo in programList.
    const MidiProgram *findProgram(const ProgramList &programList,
                                   const MidiBank &bank,
                                   int programNo);
    /// Find bank and programNo in programList.
    ProgramList::iterator findProgramIter(ProgramList &programList,
                                          const MidiBank &bank,
                                          int programNo);
    /// Within programList, change all programs using oldBank to use newBank.
    void changeBank(ProgramList &programList,
                    const MidiBank &oldBank,
                    const MidiBank &newBank);

    /// For assigning key maps to programs.
    /**
     * Holds the index of the key map button that was pressed.  Set by
     * slotKeyMapButtonPressed().
     *
     * Used by slotKeyMapMenuItemSelected() to make sure the keymap
     * selection ends up associated with the right program.
     */
    unsigned int m_keyMapProgramNumber;

    /// Ensure the msb:lsb:perc combination is unique to m_device.
    void makeUnique(bool &isPercussion,
                    MidiByte &msb,
                    MidiByte &lsb,
                    bool preferLSBChange = true);

};


}

#endif
