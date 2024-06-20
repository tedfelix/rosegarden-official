
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

#include "base/MidiProgram.h"  // BankList, ProgramList
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


class MidiProgramsEditor : public NameSetEditor
{
    Q_OBJECT

public:

    MidiProgramsEditor(BankEditorDialog *bankEditor,
                       QWidget *parent);

    /// Switch to the cleared and disabled state.
    /**
     * Called at the end of populateDeviceEditors() if no valid
     * bank or program is selected in the tree.
     */
    void clearAll();

    /// Show the programs for the selected bank.
    void populate(QTreeWidgetItem *);

    /// Limited undo in response to the Reset button in the lower left.
    /**
     * This appears to be a very limited undo for just the MSB/LSB,
     * and the percussion setting.
     */
    void reset();

private slots:

    /// Check that any new MSB/LSB combination is unique for this device.
    void slotNewMSB(int value);
    void slotNewLSB(int value);
    void slotPercussionClicked();

    /// One of the program names was changed by the user.
    void slotNameChanged(const QString &) override;
    void slotKeyMapButtonPressed() override;
    void slotKeyMapMenuItemSelected(QAction *action);
    void slotKeyMapMenuItemSelected(int programNumber);

private:

    MidiDevice *m_device{nullptr};

    // Widgets

    QCheckBox *m_percussion;
    QSpinBox *m_msb;
    QSpinBox *m_lsb;
    QFrame *initWidgets(QWidget *parent);

    // Banks

    /// The BankList we are editing.
    /**
     * We use this to check for dupes and make changes to the banks.
     */
    BankList &m_bankList;
    // Does m_bankList contain this combination already?
    // Disregard percussion bool, we care only about msb / lsb
    // in these situations.
    bool banklistContains(const MidiBank &);
    int ensureUniqueMSB(int msb, bool ascending);
    int ensureUniqueLSB(int lsb, bool ascending);

    /// The bank we are editing right now.
    MidiBank *m_currentBank{nullptr};
    /// Used by reset() to restore the original MSB/LSB/Percussion.
    MidiBank m_oldBank{false, 0, 0};

    // Programs

    /// Programs for the device being edited.
    ProgramList &m_programList;
    /// Get a pointer into the program list.
    MidiProgram *getProgram(const MidiBank &bank, int programNo);
    /// Get an iterator which is more flexible.
    ProgramList::iterator getProgramIter(const MidiBank &bank, int programNo);
    /// Get a ProgramList containing only the programs in a bank.
    ProgramList getBankSubset(const MidiBank &);
    /// Set the currently loaded programs to new MSB and LSB
    void modifyCurrentPrograms(const MidiBank &oldBank,
                               const MidiBank &newBank);

    /// For assigning keymaps to programs.
    /**
     * Holds the index of the keymap button that was pressed.  Set by
     * slotKeyMapButtonPressed().
     *
     * Used by slotKeyMapMenuItemSelected() to make sure the keymap
     * selection ends up associated with the right program.
     */
    unsigned int m_keyMapProgramNumber;
};


}

#endif
