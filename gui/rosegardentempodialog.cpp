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

#include <qstring.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qvalidator.h>
#include <qaccel.h>

#include "segmentcommands.h"
#include "rosegardentempodialog.h"
#include "rosegardenguidoc.h"
#include "widgets.h"


namespace Rosegarden
{

RosegardenTempoDialog::RosegardenTempoDialog(RosegardenGUIDoc *doc,
                                             QWidget *parent,
                                             const char *name):
    RosegardenTempo(parent, name, true), // modal
    m_doc(doc)
{
    connect((QObject*)OKButton, SIGNAL(released()),
            this, SLOT(slotOK()));

    connect((QObject*)CancelButton, SIGNAL(released()),
            this, SLOT(slotCancel()));

    // bind Return key to OK button
    //
    QAccel *a = new QAccel(this);
    a->connectItem(a->insertItem(Key_Return),
                   this,
                   SLOT(slotOK()));

    //TempoSpin->setFrameShadow(QFrame::Plain);
    TempoSpin->setMinValue(1);
    TempoSpin->setMaxValue(1000);

    // Create a 3 d.p. validator for the spin box
    //
    QDoubleValidator *validator = new QDoubleValidator(1.0, 1000.0, 3, this);
    TempoSpin->setValidator(validator);

    showTempo();
    showPosition();

}

RosegardenTempoDialog::~RosegardenTempoDialog()
{
}

void
RosegardenTempoDialog::showTempo()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::timeT currentPos = comp.getPosition();
    double tempo = comp.getTempoAt(currentPos);

    QString tempoString;
    tempoString.sprintf("%4.3f", tempo);
    TempoSpin->setValue((int)tempo);
}

void
RosegardenTempoDialog::showPosition()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::timeT currentPos = comp.getPosition();
    RealTime currentTime = comp.getElapsedRealTime(currentPos);

    QString milliSeconds;
    milliSeconds.sprintf("%03ld", currentTime.usec/1000);

    PositionValue->setText(QString("%1.%2 s").
            arg(currentTime.sec).
            arg(milliSeconds));
}


void
RosegardenTempoDialog::slotOK()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    AddTempoChangeCommand *command = 
        new AddTempoChangeCommand(&comp,
                                  comp.getPosition(),
                                  TempoSpin->getDoubleValue());

    addCommandToHistory(command);

    slotCancel();
}

void
RosegardenTempoDialog::slotCancel()
{
    delete this;
}

void 
RosegardenTempoDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}

MultiViewCommandHistory*
RosegardenTempoDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

}
 
