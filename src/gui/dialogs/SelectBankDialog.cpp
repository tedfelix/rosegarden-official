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

#define RG_MODULE_STRING "[SelectBankDialog]"
//#define RG_NO_DEBUG_PRINT

#include "SelectBankDialog.h"

#include "misc/Debug.h"

#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


SelectBankDialog::SelectBankDialog(
        QWidget *parent,
        const BankList &bankList,
        const MidiBank &originalBank,
        bool allowOriginal) :
    QDialog(parent),
    m_originalBank(originalBank),
    m_allowOriginal(allowOriginal),
    m_bankList(bankList)
{
    // ??? Window is too narrow to see this.
    setWindowTitle(tr("Select Bank"));
#if 0
    resize(350, 250);
#endif

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    int iRow = 0;

    // Percussion
    QLabel *percussionLabel = new QLabel(tr("Percussion"), this);
    layout->addWidget(percussionLabel, iRow, 0);

    m_percussion = new QCheckBox(this);
    m_percussion->setChecked(m_originalBank.isPercussion());
    connect(m_percussion, &QCheckBox::clicked,
            this, &SelectBankDialog::slotPercussionClicked);
    layout->addWidget(m_percussion, iRow, 1);

    ++iRow;

    // MSB
    QLabel *msbLabel = new QLabel(tr("MSB"), this);
    layout->addWidget(msbLabel, iRow, 0);
    m_msb = new QSpinBox(this);
    m_msb->setToolTip(tr("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_msb->setMinimum(0);
    m_msb->setMaximum(127);
    m_msb->setValue(m_originalBank.getMSB());
    connect(m_msb, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SelectBankDialog::slotMSBChanged);
    layout->addWidget(m_msb, iRow, 1);

    ++iRow;

    // LSB
    QLabel *lsbLabel = new QLabel(tr("LSB"), this);
    layout->addWidget(lsbLabel, iRow, 0);
    m_lsb = new QSpinBox(this);
    m_lsb->setToolTip(tr("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_lsb->setMinimum(0);
    m_lsb->setMaximum(127);
    m_lsb->setValue(m_originalBank.getLSB());
    connect(m_lsb, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SelectBankDialog::slotLSBChanged);
    layout->addWidget(m_lsb, iRow, 1);

    ++iRow;

    // ??? We need an "available" indicator.

    // Button Box

    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(m_buttonBox, iRow, 0, 1, 2);

    updateOkButton();
}

void
SelectBankDialog::updateOkButton()
{
    bool conflict{false};

    MidiBank midiBank = getBank();

    RG_DEBUG << "updateOkButton(): original bank " << m_originalBank.isPercussion() << ":" << m_originalBank.getMSB() << ":" << m_originalBank.getLSB();

    // If the original is allowed and this is the original...
    if (m_allowOriginal  &&  midiBank.compareKey(m_originalBank)) {
        // Skip the conflict check.
    } else {  // Do the conflict check.
        // For each MidiBank that is in use...
        for (const MidiBank &midiBankLoop : m_bankList) {
            if (midiBank.compareKey(midiBankLoop)) {
                conflict = true;
                break;
            }
        }
    }

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!conflict);
}

MidiBank
SelectBankDialog::getBank()
{
    const bool percussion = m_percussion->isChecked();
    const MidiByte msb = m_msb->value();
    const MidiByte lsb = m_lsb->value();

    MidiBank midiBank(percussion, msb, lsb, m_originalBank.getName());

    return midiBank;
}


}
