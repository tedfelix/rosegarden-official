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

#include "barbuttons.h"
#include "qvbox.h"


BarButtons::BarButtons(RosegardenGUIDoc* doc,
                       QWidget* parent,
                       QHeader *vHeader,
                       QHeader *hHeader,
                       const char* name,
                       WFlags f): QHBox(parent, name)
{
   m_cellHeight = vHeader->sectionSize(0);
   m_cellWidth = hHeader->sectionSize(0);
   m_bars = hHeader->count();

   cout << "CELL HEIGHT = " << m_cellHeight << endl;
   setMinimumHeight(m_cellHeight);
   setMaximumHeight(m_cellHeight);

   drawButtons();
}

BarButtons::BarButtons(RosegardenGUIDoc* doc,
                       QWidget* parent,
                       const char* name,
                       WFlags f): QHBox(parent, name)
{
   drawButtons();
}



BarButtons::BarButtons(QWidget* parent,
                       const char* name,
                       WFlags f) : QHBox(parent, name)
{
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons()
{

    QVBox *bar;

    for (int i = 0; i < m_bars; i++)
    {
        bar = new QVBox(this);
        bar->setMinimumSize(m_cellWidth, 20);
        bar->setMaximumSize(m_cellWidth, 20);

        // attempt a style
        //
        bar->setFrameStyle(StyledPanel);
        bar->setFrameShape(StyledPanel);
        bar->setFrameShadow(Raised);


        
    }

}




