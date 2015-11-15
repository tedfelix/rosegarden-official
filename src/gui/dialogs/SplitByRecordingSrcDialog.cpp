/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SplitByRecordingSrcDialog.h"
#include <QLayout>

#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

SplitByRecordingSrcDialog::SplitByRecordingSrcDialog(QWidget *parent, RosegardenDocument *doc) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Split by Recording Source"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QGroupBox *groupBox = new QGroupBox( tr("Recording Source"), vBox );
    groupBox->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(groupBox);
    layout->setSpacing(5);
    vBoxLayout->addWidget(groupBox);
    vBox->setLayout(vBoxLayout);

    layout->addWidget(new QLabel( tr("Channel:"), groupBox ), 0, 0);
    m_channel = new QComboBox( groupBox );
    m_channel->setMaxVisibleItems( 17 );
    layout->addWidget(m_channel, 0, 1);
    QSpacerItem *spacer = new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout->addItem( spacer, 0, 2 );

    m_channel->addItem(tr("any"));
    for (int i = 1; i < 17; ++i) {
        m_channel->addItem(QString::number(i));
    }

    layout->addWidget(new QLabel( tr("Device:"), groupBox ), 1, 0);
    m_device = new QComboBox( groupBox );
    layout->addWidget( m_device, 1, 1, 0+1, 2 - 1+1);

    m_deviceIds.clear();
    m_deviceIds.push_back( -1);
    m_device->addItem(tr("any"));

    DeviceList *devices = doc->getStudio().getDevices();
    DeviceListConstIterator it;
    for (it = devices->begin(); it != devices->end(); it++) {
        MidiDevice *dev =
            dynamic_cast<MidiDevice*>(*it);
        if (dev && dev->getDirection() == MidiDevice::Record) {
            QString label = QString::number(dev->getId());
            label += ": ";
            label += strtoqstr(dev->getName());
            QString connection = RosegardenSequencer::getInstance()->getConnection
                (dev->getId());
            label += " - ";
            if (connection == "")
                label += tr("No connection");
            else
                label += connection;
            m_device->addItem(label);
            m_deviceIds.push_back(dev->getId());
        }
    }

    m_channel->setCurrentIndex(0);
    m_device->setCurrentIndex(0);

    groupBox->setLayout(layout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

int
SplitByRecordingSrcDialog::getChannel()
{
    return m_channel->currentIndex() - 1;
}

int
SplitByRecordingSrcDialog::getDevice()
{
    return m_deviceIds[m_device->currentIndex()];
}

}
