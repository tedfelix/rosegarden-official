// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <iostream>
#include <klocale.h>

#include <cstdio>

#include <qlayout.h>
#include <qlabel.h>
#include <qhbox.h>

#include "Instrument.h"
#include "MidiDevice.h"
#include "instrumentparameterbox.h"

InstrumentParameterBox::InstrumentParameterBox(QWidget *parent,
                                               const char *name,
                                               WFlags f)
  :QFrame(parent, name, f)
{
    initBox();
}

InstrumentParameterBox::~InstrumentParameterBox()
{
}

void
InstrumentParameterBox::initBox()
{
    QFont plainFont;
    plainFont.setPointSize(10);

    QFont boldFont;
    boldFont.setPointSize(10);
    boldFont.setBold(true);

    QGridLayout *gridLayout = new QGridLayout(this, 2, 2, 5, 1);

    QLabel *title = new QLabel("Instrument Parameters", this);
    title->setFont(boldFont);

    QLabel *channelLabel = new QLabel(i18n("Channel"), this);
    QLabel *programLabel = new QLabel(i18n("Program"), this);
    QLabel *panLabel = new QLabel(i18n("Pan"), this);
    QLabel *volumeLabel = new QLabel(i18n("Volume"), this);

    // reversing motif style read-only combo
    m_channelValue = new RosegardenComboBox(true, false, this);

    // reversing motif style read-only combo
    m_programValue = new RosegardenComboBox(true, false, this);

    // Create an HBox which we insert into the GridLayout
    // for the Pan value
    //
    QHBox *hbox = new QHBox(this);

    // reversing motif style read-only combo
    m_panDial = new QDial(-Rosegarden::MidiMidValue,
                            Rosegarden::MidiMidValue,
                            10, // page step
                            0, // initial value
                            hbox);

    m_panDial->setLineStep(5);
    m_panDial->setMaximumHeight(panLabel->height());

    connect(m_panDial, SIGNAL(valueChanged(int)),
            m_panValue, SLOT(slotChangePanLabel(int)));

    m_panValue = new QLabel("0", hbox);

    // reversing motif style read-only combo
    m_volumeValue = new RosegardenComboBox(true, true, this);

    gridLayout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);

    gridLayout->addWidget(channelLabel, 1, 0, AlignLeft);
    gridLayout->addWidget(m_channelValue, 1, 1, AlignRight);

    gridLayout->addWidget(programLabel, 2, 0, AlignLeft);
    gridLayout->addWidget(m_programValue, 2, 1, AlignRight);

    gridLayout->addWidget(panLabel, 3, 0, AlignLeft);
    gridLayout->addWidget(hbox, 3, 1, AlignRight);

    gridLayout->addWidget(volumeLabel, 4, 0, AlignLeft);
    gridLayout->addWidget(m_volumeValue, 4, 1, AlignRight);

    char buffer[100];

    // Populate channel list
    for (int i = 0; i < 16; i++)
    {
        sprintf(buffer, "%d", i);
        m_channelValue->insertItem(QString(buffer), i);
    }

}

void
InstrumentParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    std::cout << "useInstrument() - populate Instrument" << std::endl;

    if (instrument == 0)
        return;

    // The MIDI channel
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());

    // The program list
    m_programValue->clear();

    Rosegarden::ProgramList list = 
        dynamic_cast<Rosegarden::MidiDevice*>(instrument->getDevice())->getProgramList(0, 0);

    Rosegarden::ProgramList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
    {
        m_programValue->insertItem(QString((*it).data()));
    }

    m_programValue->setCurrentItem((int)instrument->getProgramChange());

    // Set pan
    m_panDial->setValue((int)instrument->getPan());
    m_panValue->setText(QString("%1").arg(instrument->getPan()));

}

void
InstrumentParameterBox::slotChangePanLabel(int value)
{
    m_panValue->setText(QString("%1").arg(value));
}



