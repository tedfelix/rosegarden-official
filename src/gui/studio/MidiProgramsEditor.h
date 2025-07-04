/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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
class QLabel;


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
    void clearAll() override;

    /// Show the programs for the selected bank.
    void populate(const MidiBankTreeWidgetItem *bankItem);

private slots:

    /// Check that any new MSB/LSB combination is unique for this device.
    void slotBankEditClicked(bool checked);


    /// Not used - see slotEditingFinished().
    void slotNameChanged(const QString &) override  { }
    /// Handles a program name change from the base class, NameSetEditor.
    void slotEditingFinished() override;
    void slotKeyMapButtonPressed() override;
    void slotKeyMapMenuItemSelected(QAction *action);

private:

    MidiDevice *m_device{nullptr};

    // Widgets

    QLabel *m_percussion;
    QLabel *m_msb;
    QLabel *m_lsb;

    // Banks

    /// The bank we are editing right now.
    MidiBank m_currentBank;

    /// Find bank and programNo in programList.
    static const MidiProgram *findProgram(const ProgramList &programList,
                                          const MidiBank &bank,
                                          int programNo);
    static MidiProgram *findProgram(ProgramList &programList,
                                    const MidiBank &bank,
                                    int programNo);

    /// Find bank and programNo in programList.
    static ProgramList::iterator findProgramIter(ProgramList &programList,
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
    unsigned int m_keyMapProgramNumber{0};

};


}

#endif
