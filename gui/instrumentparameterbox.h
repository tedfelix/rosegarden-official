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

#include <qframe.h>
#include <qdial.h>
#include <qlabel.h>

#include "widgets.h"

// Display and allow modification of Instrument parameters
//

#ifndef _INSTRUMENTPARAMETERBOX_H_
#define _INSTRUMENTPARAMETERBOX_H_

namespace Rosegarden { class Instrument; }

class InstrumentParameterBox : public QFrame
{
Q_OBJECT

public:
    InstrumentParameterBox(QWidget *parent=0, const char *name=0, WFlags f=0);
    ~InstrumentParameterBox();

    void useInstrument(Rosegarden::Instrument *instrument);

public slots:
    void slotChangePanLabel(int value);

private:

    void initBox();

    RosegardenComboBox *m_channelValue;
    RosegardenComboBox *m_programValue;
    QDial              *m_panDial;
    QLabel             *m_panValue;
    RosegardenComboBox *m_volumeValue;

};


#endif // _INSTRUMENTPARAMETERBOX_H_
