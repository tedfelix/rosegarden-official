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
    setWindowTitle(tr("Select Bank"));

    QGridLayout *layout = new QGridLayout;
    // Add some extra to the sides.  Otherwise window title is cut off.
    layout->setContentsMargins(30, 15, 30, 15);
    setLayout(layout);

    int row{0};

    // Percussion
    QLabel *percussionLabel = new QLabel(tr("Percussion"), this);
    layout->addWidget(percussionLabel, row, 0);

    m_percussion = new QCheckBox(this);
    m_percussion->setChecked(m_originalBank.isPercussion());
    connect(m_percussion, &QCheckBox::clicked,
            this, &SelectBankDialog::slotPercussionClicked);
    layout->addWidget(m_percussion, row, 1);

    ++row;

    // MSB
    QLabel *msbLabel = new QLabel(tr("MSB Value"), this);
    layout->addWidget(msbLabel, row, 0);
    m_msb = new QSpinBox(this);
    m_msb->setToolTip(tr("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_msb->setMinimum(0);
    m_msb->setMaximum(127);
    m_msb->setValue(m_originalBank.getMSB());
    connect(m_msb, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SelectBankDialog::slotMSBChanged);
    layout->addWidget(m_msb, row, 1);

    ++row;

    // LSB
    QLabel *lsbLabel = new QLabel(tr("LSB Value"), this);
    layout->addWidget(lsbLabel, row, 0);
    m_lsb = new QSpinBox(this);
    m_lsb->setToolTip(tr("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_lsb->setMinimum(0);
    m_lsb->setMaximum(127);
    m_lsb->setValue(m_originalBank.getLSB());
    connect(m_lsb, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SelectBankDialog::slotLSBChanged);
    layout->addWidget(m_lsb, row, 1);

    ++row;

    // Available
    m_available = new QLabel(this);
    m_available->setAlignment(Qt::AlignHCenter);
    m_available->setAutoFillBackground(true);
    layout->addWidget(m_available, row, 0, 1, 2);

    ++row;

    // Button Box

    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(m_buttonBox, row, 0, 1, 2);

    updateWidgets();
}

void
SelectBankDialog::updateWidgets()
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

    m_available->setText(conflict ? tr("IN USE") : tr("available"));
    QPalette palette = m_available->palette();
    if (conflict)
        palette.setColor(QPalette::Window, QColor(0x70, 0x00, 0x00));
    else
        palette.setColor(QPalette::Window, QColor(0x00, 0x70, 0x00));
    m_available->setPalette(palette);
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
