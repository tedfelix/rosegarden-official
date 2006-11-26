/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include <kcombobox.h>
#include <kcommand.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <qbuttongroup.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
        RosegardenGUIDoc *doc):
        KDialogBase(parent, "", true, i18n("Remap Instrument assigments..."),
                    Ok | Apply | Cancel),
        m_doc(doc)
{
    QVBox *vBox = makeVBoxMainWidget();

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Device or Instrument"),
                                     vBox);

    new QLabel(i18n("Remap Tracks by all Instruments on a Device or by single Instrument"), m_buttonGroup);
    m_deviceButton = new QRadioButton(i18n("Device"), m_buttonGroup);
    m_instrumentButton = new QRadioButton(i18n("Instrument"), m_buttonGroup);


    connect(m_buttonGroup, SIGNAL(released(int)),
            this, SLOT(slotRemapReleased(int)));

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Choose Source and Destination"),
                                        vBox);

    new QLabel(i18n("From"), groupBox);
    new QLabel(i18n("To"), groupBox);
    m_fromCombo = new KComboBox(groupBox);
    m_toCombo = new KComboBox(groupBox);

    m_buttonGroup->setButton(0);
    populateCombo(0);
}

void
RemapInstrumentDialog::populateCombo(int id)
{
    m_fromCombo->clear();
    m_toCombo->clear();
    Studio *studio = &m_doc->getStudio();

    if (id == 0) {
        DeviceList *devices = studio->getDevices();
        DeviceListIterator it;
        m_devices.clear();

        for (it = devices->begin(); it != devices->end(); it++) {
            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*it);

            if (md) {
                if (md->getDirection() == MidiDevice::Play) {
                    m_devices.push_back(*it);
                    m_fromCombo->insertItem(strtoqstr((*it)->getName()));
                    m_toCombo->insertItem(strtoqstr((*it)->getName()));
                }
            } else {
                SoftSynthDevice *sd =
                    dynamic_cast<SoftSynthDevice *>(*it);
                if (sd) {
                    m_devices.push_back(*it);
                    m_fromCombo->insertItem(strtoqstr((*it)->getName()));
                    m_toCombo->insertItem(strtoqstr((*it)->getName()));
                }
            }
        }

        if (m_devices.size() == 0) {
            m_fromCombo->insertItem(i18n("<no devices>"));
            m_toCombo->insertItem(i18n("<no devices>"));
        }
    } else {
        m_instruments = studio->getPresentationInstruments();
        InstrumentList::iterator it = m_instruments.begin();

        for (; it != m_instruments.end(); it++) {
            m_fromCombo->insertItem(strtoqstr((*it)->getPresentationName()));
            m_toCombo->insertItem(strtoqstr((*it)->getPresentationName()));
        }
    }
}

void
RemapInstrumentDialog::slotRemapReleased(int id)
{
    populateCombo(id);
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
    if (m_buttonGroup->id(m_buttonGroup->selected()) == 0) // devices
    {
        ModifyDeviceMappingCommand *command =
            new ModifyDeviceMappingCommand
            (m_doc,
             m_devices[m_fromCombo->currentItem()]->getId(),
             m_devices[m_toCombo->currentItem()]->getId());
        addCommandToHistory(command);
    } else // instruments
    {
        ModifyInstrumentMappingCommand *command =
            new ModifyInstrumentMappingCommand
            (m_doc,
             m_instruments[m_fromCombo->currentItem()]->getId(),
             m_instruments[m_toCombo->currentItem()]->getId());
        addCommandToHistory(command);
    }

    emit applyClicked();
}

void
RemapInstrumentDialog::addCommandToHistory(KCommand *command)
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
