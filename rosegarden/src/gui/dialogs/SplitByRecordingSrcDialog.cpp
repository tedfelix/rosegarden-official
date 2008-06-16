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


#include "SplitByRecordingSrcDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include "document/RosegardenGUIDoc.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

SplitByRecordingSrcDialog::SplitByRecordingSrcDialog(QWidget *parent, RosegardenGUIDoc *doc) :
        KDialogBase(parent, 0, true, i18n("Split by Recording Source"), Ok | Cancel )
{
    QVBox *vBox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Recording Source"), vBox);
    QFrame *frame = new QFrame(groupBox);
    QGridLayout *layout = new QGridLayout(frame, 2, 2, 10, 5);

    layout->addWidget(new QLabel( i18n("Channel:"), frame ), 0, 0);
    m_channel = new KComboBox( frame );
    m_channel->setSizeLimit( 17 );
    layout->addWidget(m_channel, 0, 1);
    QSpacerItem *spacer = new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout->addItem( spacer, 0, 2 );

    m_channel->insertItem(i18n("any"));
    for (int i = 1; i < 17; ++i) {
        m_channel->insertItem(QString::number(i));
    }

    layout->addWidget(new QLabel( i18n("Device:"), frame ), 1, 0);
    m_device = new KComboBox( frame );
    layout->addMultiCellWidget( m_device, 1, 1, 1, 2 );

    m_deviceIds.clear();
    m_deviceIds.push_back( -1);
    m_device->insertItem(i18n("any"));

    DeviceList *devices = doc->getStudio().getDevices();
    DeviceListConstIterator it;
    for (it = devices->begin(); it != devices->end(); it++) {
        MidiDevice *dev =
            dynamic_cast<MidiDevice*>(*it);
        if (dev && dev->getDirection() == MidiDevice::Record) {
            QString label = QString::number(dev->getId());
            label += ": ";
            label += strtoqstr(dev->getName());
            QString connection = strtoqstr(dev->getConnection());
            label += " - ";
            if (connection == "")
                label += i18n("No connection");
            else
                label += connection;
            m_device->insertItem(label);
            m_deviceIds.push_back(dev->getId());
        }
    }

    m_channel->setCurrentItem(0);
    m_device->setCurrentItem(0);
}

int
SplitByRecordingSrcDialog::getChannel()
{
    return m_channel->currentItem() - 1;
}

int
SplitByRecordingSrcDialog::getDevice()
{
    return m_deviceIds[m_device->currentItem()];
}

}
#include "SplitByRecordingSrcDialog.moc"
