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
#include <qvbox.h>
#include <qlabel.h>


BarButtons::BarButtons(RosegardenGUIDoc* doc,
                       QWidget* parent,
                       QHeader *vHeader,
                       QHeader *hHeader,
                       const char* name,
                       WFlags f):
    QHBox(parent, name)
{
   m_cellHeight = vHeader->sectionSize(0);
   m_cellWidth = hHeader->sectionSize(0);
   m_bars = hHeader->count();

   m_offset = 4;

   setMinimumHeight(m_cellHeight);
   setMaximumHeight(m_cellHeight);

   drawButtons();
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons()
{

    // Create a horizontal spacing label to jog everything
    // up with the main SegmentCanvas
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumWidth(m_offset);
    label->setMaximumWidth(m_offset);

    int loopBarHeight = 8; // the height of the loop bar
    QVBox *bar;

    // Create a vertical box for the loopBar and the bar buttons
    //
    QVBox *buttonBar = new QVBox(this);
    buttonBar->setMinimumSize(m_cellWidth * m_bars, m_cellHeight);
    buttonBar->setMaximumSize(m_cellWidth * m_bars, m_cellHeight);

    // The loop bar is where we're going to be defining our loops
    //
    QLabel *loopBar = new QLabel(buttonBar);
    loopBar->setMinimumSize(m_cellWidth * m_bars, loopBarHeight);
    loopBar->setMaximumSize(m_cellWidth * m_bars, loopBarHeight);

    // Need another horizontal layout box - makes a bit of a 
    // mockery of what this class is derived from!
    //
    QHBox *hButtonBar = new QHBox(buttonBar);

    for (int i = 0; i < m_bars; i++)
    {
        bar = new QVBox(hButtonBar);
        bar->setSpacing(0);
        bar->setMinimumSize(m_cellWidth, m_cellHeight - loopBarHeight);
        bar->setMaximumSize(m_cellWidth, m_cellHeight - loopBarHeight);

        // attempt a style
        //
        bar->setFrameStyle(StyledPanel);
        bar->setFrameShape(StyledPanel);
        bar->setFrameShadow(Raised);

        label = new QLabel(bar);
        label->setText(QString("%1").arg(i));
        label->setAlignment(AlignLeft|AlignVCenter);
        label->setIndent(4);
        label->setMinimumHeight(m_cellHeight - loopBarHeight - 2);
        label->setMaximumHeight(m_cellHeight - loopBarHeight - 2);

    }

}




