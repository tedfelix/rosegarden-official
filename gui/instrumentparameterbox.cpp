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

#include <qlayout.h>
#include <qlabel.h>

#include "instrumentparameterbox.h"

InstrumentParameterBox::InstrumentParameterBox(QWidget *parent,
                                               const char *name,
                                               WFlags f)
  :QFrame(parent, name, f)
{
    setMinimumSize(130, 100);
    setMaximumSize(130, 100);

    initBox();
}

InstrumentParameterBox::~InstrumentParameterBox()
{
}

void
InstrumentParameterBox::initBox()
{
    QFont font ("lucidasanstypewriter", 8);
    font.setPixelSize(10);

    QGridLayout *gridLayout = new QGridLayout(this, 2, 2, 8, 1);

    QLabel *title = new QLabel("Instrument Parameters", this);
    title->setFont(font);

    gridLayout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);

}


