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

#include <qconnect.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcombobox.h>

#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenguidoc.h"
#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"

namespace Rosegarden
{

RosegardenConfigureDialog::RosegardenConfigureDialog(RosegardenGUIDoc *doc,
                                                     QWidget *parent,
                                                     const char *name):
    RosegardenConfigure(parent, name, true), // modal
    m_doc(doc)
{

    // buttons 
    //
    connect((QObject *)CancelButton, SIGNAL(released()),
            this, SLOT(slotClose()));

    connect((QObject *)ApplyButton, SIGNAL(released()),
            this, SLOT(slotApply()));

    connect((QObject *)OKButton, SIGNAL(released()),
            this, SLOT(slotOK()));

    // Ok, populate the fields
    //
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Configuration &config = m_doc->getConfiguration();

    // count-in bars
    //
    CountInSpin->setValue(comp.getCountInBars());
    CountInSpin->setMaxValue(10);
    CountInSpin->setMinValue(0);

    connect(CountInSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotActivateApply()));

    // latency sliders
    connect((QObject *)PlaybackSlider, SIGNAL(valueChanged(int)),
            (QObject *)PlaybackValue, SLOT(setNum(int)));

    connect((QObject *)ReadAheadSlider, SIGNAL(valueChanged(int)),
            (QObject *)ReadAheadValue, SLOT(setNum(int)));

    connect((QObject *)PlaybackSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotActivateApply()));

    connect((QObject *)ReadAheadSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotActivateApply()));

    ReadAheadSlider->setMinValue(20);
    ReadAheadSlider->setMaxValue(80);
    ReadAheadSlider->setValue(config.getReadAhead().usec / 1000);

    PlaybackSlider->setMinValue(20);
    PlaybackSlider->setMaxValue(500);
    PlaybackSlider->setValue(config.getPlaybackLatency().usec / 1000);

    // set client combo
    //
    ClientCombo->setCurrentItem(config.getDoubleClickClient());

    connect((QObject*)ClientCombo, SIGNAL(activated(int)),
            this, SLOT(slotActivateApply()));

    // Disable apply until something changes
    //
    ApplyButton->setDisabled(true);

}

RosegardenConfigureDialog::~RosegardenConfigureDialog()
{
}

void
RosegardenConfigureDialog::slotClose()
{
    delete this;
}

void
RosegardenConfigureDialog::slotApply()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Configuration &config = m_doc->getConfiguration();

    int countIn = CountInSpin->text().toInt();
    comp.setCountInBars(countIn);

    int readAhead = ReadAheadValue->text().toInt();
    config.setReadAhead((RealTime(0, (readAhead * 1000))));

    int playback = PlaybackValue->text().toInt();
    config.setPlaybackLatency((RealTime(0, (playback * 1000))));

    int client = ClientCombo->currentItem();
    config.setDoubleClickClient(
            (Rosegarden::Configuration::DoubleClickClient)client);

    ApplyButton->setDisabled(true);
}


void
RosegardenConfigureDialog::slotActivateApply()
{
    ApplyButton->setDisabled(false);
}

void
RosegardenConfigureDialog::slotOK()
{
    slotApply();
    slotClose();
}

}
 
