
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <vector>

#include <kdialogbase.h>
#include <qlineedit.h>

#include "Instrument.h"

class RosegardenComboBox;
class QPushButton;
class QFrame;
class QSpinBox;

namespace Rosegarden { class Studio; class MidiDevice; }

#ifndef _BANKEDITOR_H_
#define _BANKEDITOR_H_


class ProgramLine : public QLineEdit
{
    Q_OBJECT

public:
    ProgramLine(QWidget *parent, int id);

    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

public slots:
    void slotNewText(const QString &);

signals:
    void newText(const QString&, int);

protected:
    int m_id;

};

class BankEditorDialog : public KDialogBase
{
    Q_OBJECT

public:
    BankEditorDialog(QWidget *parent,
                     Rosegarden::Studio *studio);

    std::vector<Rosegarden::MidiProgram>
        getBankSubset(Rosegarden::MidiByte msb, Rosegarden::MidiByte lsb);

    std::pair<int, int> getFirstFreeBank(int device);

    int ensureUniqueMSB(int msb, bool ascending);
    int ensureUniqueLSB(int lsb, bool ascending);

    // Does the banklist contain this combination already?
    //
    bool banklistContains(int msb, int lsb);

    void setModified(bool value);

public slots:
    void slotPopulateBank(int bank);
    void slotPopulateDevice(int device);
    void slotPopulateDeviceBank(int device, int bank);

    void slotOK();
    void slotApply();

    void slotAddBank();
    void slotDeleteBank();
    void slotDeleteAllBanks();

    void slotModifyBankName(const QString&);
    void slotModifyDeviceName(const QString&);

    void slotProgramChanged(const QString&, int);

    // Check that any new MSB/LSB combination is unique for this device
    //
    void slotNewMSB(int value);
    void slotNewLSB(int value);

protected:
    // Get a MidiDevice from an index number
    //
    Rosegarden::MidiDevice* getMidiDevice(int number);

    //--------------- Data members ---------------------------------
    Rosegarden::Studio      *m_studio;

    QTabWidget              *m_programTab;
    RosegardenComboBox      *m_deviceCombo;
    RosegardenComboBox      *m_bankCombo;
    QSpinBox                *m_msb;
    QSpinBox                *m_lsb;
    std::vector<ProgramLine*>  m_programNames;

    QPushButton             *m_addBank;
    QPushButton             *m_deleteBank;
    QPushButton             *m_deleteAllBanks;

    QFrame                  *m_mainFrame;

    std::vector<Rosegarden::MidiBank>        m_bankList;
    std::vector<Rosegarden::MidiProgram>     m_programList;

    bool                     m_modified;

};

#endif // _BANKEDITOR_H_

