/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RemapInstrumentDialog.h"

#include "misc/Strings.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/Studio.h"
#include "commands/studio/ModifyDeviceMappingCommand.h"
#include "commands/studio/ModifyInstrumentMappingCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include <QComboBox>
#include "document/Command.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <klocale.h>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
        RosegardenGUIDoc *doc):
        QDialog(parent),
        m_doc(doc)
{
    setModal(true);
    setWindowTitle(i18n("Remap Instrument assigments..."));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QGroupBox *buttonGroup = new QGroupBox(i18n("Device or Instrument"));
    QVBoxLayout *buttonGroupLayout = new QVBoxLayout;
    vBoxLayout->addWidget(buttonGroup);

    buttonGroupLayout->addWidget(new QLabel(i18n("Remap Tracks by all "
                            "Instruments on a Device or by single Instrument")));
    m_deviceButton = new QRadioButton(i18n("Device"));
    buttonGroupLayout->addWidget(m_deviceButton);
    m_instrumentButton = new QRadioButton(i18n("Instrument"));
    buttonGroupLayout->addWidget(m_instrumentButton);
    buttonGroup->setLayout(buttonGroupLayout);

    connect(m_deviceButton, SIGNAL(released()),
            this, SLOT(slotRemapReleased()));
    connect(m_instrumentButton, SIGNAL(released()),
            this, SLOT(slotRemapReleased()));

    QGroupBox *groupBox = new QGroupBox(i18n("Choose Source and Destination"));
    QGridLayout *groupBoxLayout = new QGridLayout;
    vBoxLayout->addWidget(groupBox);

    groupBoxLayout->addWidget(new QLabel(i18n("From")), 0, 0);
    groupBoxLayout->addWidget(new QLabel(i18n("To")), 0, 1);
    m_fromCombo = new QComboBox(groupBox);
    groupBoxLayout->addWidget(m_fromCombo, 1, 0);
    m_toCombo = new QComboBox(groupBox);
    groupBoxLayout->addWidget(m_toCombo, 1, 1);
    groupBox->setLayout(groupBoxLayout);

    vBox->setLayout(vBoxLayout);

    /// m_buttonGroup->setButton(0);
    m_deviceButton->setChecked(true);
    populateCombo();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Apply |
                                                       QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
RemapInstrumentDialog::populateCombo()
{
    m_fromCombo->clear();
    m_toCombo->clear();
    Studio *studio = &m_doc->getStudio();

    if (m_deviceButton->isChecked()) {
        DeviceList *devices = studio->getDevices();
        DeviceListIterator it;
        m_devices.clear();

        for (it = devices->begin(); it != devices->end(); it++) {
            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*it);

            if (md) {
                if (md->getDirection() == MidiDevice::Play) {
                    m_devices.push_back(*it);
                    m_fromCombo->addItem(strtoqstr((*it)->getName()));
                    m_toCombo->addItem(strtoqstr((*it)->getName()));
                }
            } else {
                SoftSynthDevice *sd =
                    dynamic_cast<SoftSynthDevice *>(*it);
                if (sd) {
                    m_devices.push_back(*it);
                    m_fromCombo->addItem(strtoqstr((*it)->getName()));
                    m_toCombo->addItem(strtoqstr((*it)->getName()));
                }
            }
        }

        if (m_devices.size() == 0) {
            m_fromCombo->addItem(i18n("<no devices>"));
            m_toCombo->addItem(i18n("<no devices>"));
        }
    } else {
        m_instruments = studio->getPresentationInstruments();
        InstrumentList::iterator it = m_instruments.begin();

        for (; it != m_instruments.end(); it++) {
            m_fromCombo->addItem(strtoqstr((*it)->getPresentationName()));
            m_toCombo->addItem(strtoqstr((*it)->getPresentationName()));
        }
    }
}

void
RemapInstrumentDialog::slotRemapReleased()
{
    populateCombo();
}

void
RemapInstrumentDialog::slotOk()
{
    slotApply();
    accept();
}

void
RemapInstrumentDialog::slotApply()
{
    if (m_deviceButton->isChecked()) // devices
    {
        ModifyDeviceMappingCommand *command =
            new ModifyDeviceMappingCommand
            (m_doc,
             m_devices[m_fromCombo->currentIndex()]->getId(),
             m_devices[m_toCombo->currentIndex()]->getId());
        addCommandToHistory(command);
    } else // instruments
    {
        ModifyInstrumentMappingCommand *command =
            new ModifyInstrumentMappingCommand
            (m_doc,
             m_instruments[m_fromCombo->currentIndex()]->getId(),
             m_instruments[m_toCombo->currentIndex()]->getId());
        addCommandToHistory(command);
    }

    emit applyClicked();
}

void
RemapInstrumentDialog::addCommandToHistory(Command *command)
{
    getCommandHistory()->addCommand(command);
}

MultiViewCommandHistory*
RemapInstrumentDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

}
#include "RemapInstrumentDialog.moc"
