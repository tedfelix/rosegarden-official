// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "trackbuttons.h"
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc)
{
    drawButtons();
}

TrackButtons::TrackButtons(QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name)
{
}


TrackButtons::~TrackButtons()
{
}

// Draw the mute and record buttons
//
//
void
TrackButtons::drawButtons()
{
    setSpacing(1);

    // Create a gap at the top of the layout widget
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumHeight(10);
    label->setMaximumHeight(10);

    // Create a horizontal box for each track
    // plus the two buttons
    //
    QHBox *track;
    QPushButton *mute;
    QPushButton *record;

    // Populate the widget to our current
    // hardcoded number of tracks
    //
    for (int i = 0; i < 63; i++)
    {
        track = new QHBox(this);
        track->setMinimumWidth(40);
        mute = new QPushButton(track);
        record = new QPushButton(track);
        mute->setText("M");
        record->setText("R"); 

        mute->setMinimumWidth(15);
        mute->setMaximumWidth(15);

        record->setMinimumWidth(15);
        record->setMaximumWidth(15);

        mute->setMinimumHeight(15);
        mute->setMaximumHeight(15);

        record->setMinimumHeight(15);
        record->setMaximumHeight(15);
    }
}



