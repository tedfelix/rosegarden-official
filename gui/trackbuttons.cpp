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
#include <assert.h>

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           Rosegarden::TrackHeader *vHeader,
                           QHeader *hHeader,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc)
{
    assert(vHeader != 0);
    assert(hHeader != 0);

    m_tracks = vHeader->count();
    //m_offset = vHeader->offset();
    //m_offset = 19;
    m_offset = vHeader->sectionPos(1) - 7;
    m_cellSize = vHeader->sectionSize(0);
    drawButtons();
}

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc), m_tracks(0), m_offset(0), m_cellSize(0)
{
    drawButtons();
}

TrackButtons::TrackButtons(QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(0), m_tracks(0), m_offset(0), m_cellSize(0)
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
    int gap = 8;
    setSpacing(gap);

    // Create a gap at the top of the layout widget
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumHeight(m_offset);
    label->setMaximumHeight(m_offset);

    // Create a horizontal box for each track
    // plus the two buttons
    //
    QHBox *track;
    QPushButton *mute;
    QPushButton *record;


    m_recordButtonGroup = new QButtonGroup();
    m_recordButtonGroup->setExclusive(true);

    m_muteButtonGroup = new QButtonGroup();
    m_muteButtonGroup->setExclusive(false);

    // Populate the widget to our current
    // hardcoded number of tracks
    //
    for (int i = 0; i < m_tracks; i++)
    {
        track = new QHBox(this);
        track->setMinimumWidth(40);
        mute = new QPushButton(track);
        record = new QPushButton(track);

        // insert the button into the group
        //
        m_recordButtonGroup->insert(record, i);
        m_muteButtonGroup->insert(mute, i);

        mute->setToggleButton(true);
        record->setToggleButton(true);

        mute->setText("M");
        record->setText("R"); 

        mute->setMinimumWidth(m_cellSize - gap);
        mute->setMaximumWidth(m_cellSize - gap);

        record->setMinimumWidth(m_cellSize - gap);
        record->setMaximumWidth(m_cellSize - gap);

        mute->setMinimumHeight(m_cellSize - gap);
        mute->setMaximumHeight(m_cellSize - gap);

        record->setMinimumHeight(m_cellSize - gap);
        record->setMaximumHeight(m_cellSize - gap);
    }
}


// Return the track that's currently set for recording
//
//
int
TrackButtons::selectedRecordTrack()
{
   QButton *retButton = m_recordButtonGroup->selected();

   // if none selected
   if (!retButton)
     return -1;

   return m_recordButtonGroup->id(retButton);
}


// Create and return a list of all muted tracks
//
//
list<int>
TrackButtons::mutedTracks()
{
    list<int> mutedTracks;

    for (int i = 0; i < m_tracks; i++)
    {
        if (m_muteButtonGroup->find(i)->isDown())
            mutedTracks.push_back(i);
    }

    return mutedTracks;
}


// Set the record button (button group is exclusive)
//
//
void
TrackButtons::setRecordTrack(const int &recordTrack)
{
    if ( recordTrack < 0 || recordTrack > m_tracks )
        return;

    m_recordButtonGroup->find(recordTrack)->setDown(true);
}


// Set a mute button
//
//
void
TrackButtons::setMutedTrack(const int &mutedTrack)
{
    if (mutedTrack < 0 || mutedTrack > m_tracks )
        return;

    m_muteButtonGroup->find(mutedTrack)->setDown(true);
}


