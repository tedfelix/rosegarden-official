// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <qhbox.h>
#include <qvbox.h>
#include <qgroupbox.h>

#include "widgets.h"
#include "commondialogs.h"


RosegardenFloatEdit::RosegardenFloatEdit(QWidget *parent,
                                         const QString &title,
                                         const QString &text,
                                         float min,
                                         float max,
                                         float value,
                                         float step):
        KDialogBase(parent, "rosegardenFloatEdit", true, title, Ok | Cancel, Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox(1, Horizontal, text, vbox);
    QVBox *inVbox = new QVBox(groupBox);

    // Calculate decimal points according to the step size
    //
    double calDP = log10(step);
    int dps = 0;
    if (calDP < 0.0) dps = int(-calDP);
    //std::cout << "CAL DP = " << calDP << ", dps = " << dps << std::endl;

    m_spin = new HSpinBox(inVbox, value, 1, min, max, dps);
    new QLabel(QString("(min: %1, max: %2)").arg(min).arg(max), inVbox);
}

float 
RosegardenFloatEdit::getValue() const
{
    return m_spin->valuef();
}


