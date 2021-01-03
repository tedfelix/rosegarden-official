
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

#ifndef RG_MIDIPROGRAMSEDITOR_H
#define RG_MIDIPROGRAMSEDITOR_H

#include "base/MidiProgram.h"
#include "NameSetEditor.h"


class QWidget;
class QString;
class QSpinBox;
class QTreeWidgetItem;
class QCheckBox;
class BankList;


namespace Rosegarden
{

class MidiProgram;
class MidiDevice;
class BankEditorDialog;


class MidiProgramsEditor : public NameSetEditor
{
    Q_OBJECT
public:
    MidiProgramsEditor(BankEditorDialog *bankEditor,
                       QWidget *parent);

    void clearAll();
    void populate(QTreeWidgetItem*);
    void reset();

public slots:

    // Check that any new MSB/LSB combination is unique for this device
    //
    void slotNewMSB(int value);
    void slotNewLSB(int value);
    void slotNewPercussion(); // gets value from checkbox

    void slotNameChanged(const QString &) override;
    void slotKeyMapButtonPressed() override;
    void slotKeyMapMenuItemSelected(QAction *);
    void slotKeyMapMenuItemSelected(int);

protected:

    MidiBank* getCurrentBank();

    int ensureUniqueMSB(int msb, bool ascending);
    int ensureUniqueLSB(int lsb, bool ascending);

    // Does the banklist contain this combination already?
    // Disregard percussion bool, we care only about msb / lsb
    // in these situations.
    //
    bool banklistContains(const MidiBank &);

    ProgramList getBankSubset(const MidiBank &);

    /// Set the currently loaded programs to new MSB and LSB
    void modifyCurrentPrograms(const MidiBank &oldBank,
                               const MidiBank &newBank);
    
    // Get a program (pointer into program list) for modification
    //
    MidiProgram* getProgram(const MidiBank &bank, int program);

    void setBankName(const QString& s);

    virtual QWidget *makeAdditionalWidget(QWidget *parent);

    void blockAllSignals(bool block);

    //--------------- Data members ---------------------------------
    QCheckBox                *m_percussion;
    QSpinBox                 *m_msb;
    QSpinBox                 *m_lsb;

    MidiDevice   *m_device;

    MidiBank     *m_currentBank;
    BankList     &m_bankList;
    ProgramList  &m_programList;

    MidiBank      m_oldBank;

    unsigned int              m_currentMenuProgram;
};


}

#endif
