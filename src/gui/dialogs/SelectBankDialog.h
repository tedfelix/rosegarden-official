/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SELECTBANKDIALOG_H
#define SELECTBANKDIALOG_H

#include "base/MidiProgram.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QSpinBox;


namespace Rosegarden
{


class SelectBankDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * allowOriginal - Allows the original value to be accepted.  Set this to
     * true when editing a bank.  Set this to false when pasting a bank in case
     * the original value conflicts.
     */
    SelectBankDialog(
            QWidget *parent,
            const BankList &bankList,
            const MidiBank &originalBank,
            bool allowOriginal);

    MidiBank getBank();

private:

    const MidiBank m_originalBank;
    const bool m_allowOriginal;
    const BankList &m_bankList;

    QCheckBox *m_percussion;
    QSpinBox *m_msb;
    QSpinBox *m_lsb;

    QLabel *m_available;

    QDialogButtonBox *m_buttonBox;
    void updateWidgets();

private slots:

    void slotPercussionClicked()  { updateWidgets(); }
    void slotMSBChanged(int /*value*/)  { updateWidgets(); }
    void slotLSBChanged(int /*value*/)  { updateWidgets(); }

};


}

#endif
