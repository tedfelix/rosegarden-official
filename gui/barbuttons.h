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

#ifndef _BARBUTTONS_H_
#define _BARBUTTONS_H_

#include <qhbox.h>
#include <qheader.h>

#include "rosegardenguidoc.h"


class BarButtons : public QHBox
{
    Q_OBJECT

public:
    BarButtons(RosegardenGUIDoc* doc,
               QWidget* parent = 0,
               QHeader *vHeader = 0,
               QHeader *hHeader = 0,
               const char* name = 0,
               WFlags f=0);

    BarButtons(RosegardenGUIDoc* doc,
               QWidget* parent = 0,
               const char* name = 0,
               WFlags f=0);


    BarButtons(QWidget* parent = 0,
               const char* name = 0,
               WFlags f=0);

    ~BarButtons();

private:
    void drawButtons();

    int m_bars;
    int m_cellHeight;
    int m_cellWidth;
    
};


#endif // _BARBUTTONS_H_
