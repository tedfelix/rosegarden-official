/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NAMESETEDITOR_H
#define RG_NAMESETEDITOR_H

#include <QGroupBox>
#include <QString>
#include <QStringList>

class QFrame;
class QGridLayout;
class QLabel;
class QPushButton;
class QToolButton;
class QTreeWidgetItem;

#include <vector>


namespace Rosegarden
{


class BankEditorDialog;
class LineEdit;

/// A tabbed editor for large lists of names.
/**
 * MidiProgramsEditor and MidiKeyMappingEditor derive from this to provide
 * the program and key mapping lists in the BankEditorDialog.
 *
 * Other common parts of MidiProgramsEditor and MidiKeyMappingEditor have
 * been factored up into the baseclass.  Those parts should be pushed
 * back down into the derived classes, and this should become focused
 * solely on providing the name list.  Then this should be switched
 * from derivation to containment.
 */
class NameSetEditor : public QGroupBox
{
    Q_OBJECT

public slots:
    /// Handler for changes to text in any of the line edit widgets.
    /**
     * Connected to LineEdit::textChanged() for all line edits.
     *
     * This is virtual because NameSetEditor does the connect and wants
     * the derived version.
     */
    virtual void slotNameChanged(const QString &) = 0;

    /// Handler for presses of any of the key map buttons.
    /**
     * Connected to QToolButton::clicked() for all key map buttons.
     *
     * This is virtual because NameSetEditor does the connect and wants
     * the derived version.
     */
    virtual void slotKeyMapButtonPressed() = 0;

    /// Handler for presses of the numbering base (0/1) button.
    /**
     * Connected to QPushButton::clicked() for the numbering base button.
     */
    void slotToggleNumberingBase();

protected:
    NameSetEditor(BankEditorDialog *bankEditor,
                  QString title,  // Gets overridden by the bank/map name.
                  QWidget *parent,
                  bool showKeyMapButtons = false);

    QToolButton *getKeyMapButton(int n) { return m_keyMapButtons[n]; }
    const QToolButton *getKeyMapButton(int n) const { return m_keyMapButtons[n]; }

    /// Parent
    /**
     * ??? Arbitrary factoring.  Push back down into derived classes.
     */
    BankEditorDialog *m_bankEditor;

    /// Area above the tabbed widget.
    /**
     * This area contains the "Provided by" groupbox and the
     * Percussion/MSB/LSB fields in the MidiProgramsEditor.
     *
     * ??? "Provided by" should be moved to a new panel that appears
     *     when the root node on the tree is selected.  Once that's done,
     *     the top frame is only needed by MidiProgramsEditor and can be
     *     pushed down into there.
     */
    QFrame *m_topFrame;
    QGridLayout *m_topLayout;

    QLabel *m_librarian;
    QLabel *m_librarianEmail;

    std::vector<LineEdit *> m_names;
    QStringList m_completions;

private:
    QPushButton *m_numberingBaseButton;
    unsigned m_numberingBase;

    /// Numbers to the left of each line edit.
    std::vector<QLabel *> m_labels;
    void updateLabels();

    /// Key map buttons between the labels and the line edits.
    /**
     * Instead of key map buttons, we should have comboboxes that
     * allow direct selection of the keymap (along with viewing of the
     * current value which is currently impossible).
     */
    std::vector<QToolButton *> m_keyMapButtons;
};


}

#endif
