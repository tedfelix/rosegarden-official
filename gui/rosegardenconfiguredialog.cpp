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

#include "rosegardenconfiguredialog.h"
#include "rosegardenguidoc.h"
#include "Composition.h"

namespace Rosegarden
{

RosegardenConfigureDialog::RosegardenConfigureDialog(RosegardenGUIDoc *doc,
                                                     QWidget *parent,
                                                     const char *name):
    RosegardenConfigure(parent, name, true), // modal
    m_doc(doc)
{

    connect((QObject *)CancelButton, SIGNAL(released()),
            this, SLOT(slotClose()));

    connect((QObject *)ApplyButton, SIGNAL(released()),
            this, SLOT(slotApply()));

    // Ok, populate the fields
    //
    Rosegarden::Composition &comp = m_doc->getComposition();

    // count-in bars
    //
    CountInSpin->setValue(comp.getCountInBars());
    CountInSpin->setMaxValue(10);
    CountInSpin->setMinValue(0);

    connect(CountInSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotActivateApply()));


    // latency
    connect((QObject *)PlaybackSlider, SIGNAL(valueChanged(int)),
            (QObject *)PlaybackValue, SLOT(setNum(int)));

    connect((QObject *)ReadAheadSlider, SIGNAL(valueChanged(int)),
            (QObject *)ReadAheadValue, SLOT(setNum(int)));

    //ReadAheadSlider->setValue(comp.

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

    int countIn = CountInSpin->text().toInt();
    comp.setCountInBars(countIn);

    slotClose();
}


void
RosegardenConfigureDialog::slotActivateApply()
{
    ApplyButton->setDisabled(false);
}

}
 
