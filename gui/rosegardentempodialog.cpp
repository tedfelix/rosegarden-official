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

#include "rosegardentempodialog.h"
#include "rosegardenguidoc.h"


namespace Rosegarden
{

RosegardenTempoDialog::RosegardenTempoDialog(RosegardenGUIDoc *doc,
                                             QWidget *parent,
                                             const char *name):
    RosegardenTempo(parent, name, true), // modal
    m_doc(doc)
{
    resetFonts();

    connect((QObject*)OKButton, SIGNAL(released()),
            this, SLOT(slotOK()));

    connect((QObject*)CancelButton, SIGNAL(released()),
            this, SLOT(slotCancel()));

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::timeT currentPos = comp.getPosition();
    double tempo = comp.getTempoAt(currentPos);
    RealTime currentTime = comp.getElapsedRealTime(currentPos);

    QString tempoString;
    tempoString.sprintf("%4.3f", tempo);

    TempoSpin->setSpecialValueText(tempoString);
    PositionValue->setText(QString("%1.%2 s").arg(currentTime.sec).arg(currentTime.usec));
}

RosegardenTempoDialog::~RosegardenTempoDialog()
{
}

void
RosegardenTempoDialog::slotOK()
{
    slotCancel();
}

void
RosegardenTempoDialog::slotCancel()
{
    delete this;
}

void
RosegardenTempoDialog::resetFont(QWidget *w)
{
    QFont font = w->font();
    font.setPixelSize(10);
    w->setFont(font);
}

void
RosegardenTempoDialog::resetFonts()
{
    resetFont(TempoLabel);
    resetFont(TempoSpin);
    resetFont(PositionLabel);
    resetFont(PositionValue);
}


}
 
