

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
#include "Track.h"
#include "colours.h"

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           Rosegarden::TrackHeader *vHeader,
                           QHeader *hHeader,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc), m_lastID(-1)
{
    assert(vHeader != 0);
    assert(hHeader != 0);

    // Number of tracks on our view
    //
    m_tracks = vHeader->count();

    // The pixel offset from the top minus a fiddle factor
    //
    m_offset = vHeader->sectionPos(1) - 3;
 
    // The height of the cells
    //
    m_cellSize = vHeader->sectionSize(0);

    // Now draw the buttons and labels
    //
    drawButtons();
}

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc), m_tracks(0), m_offset(0), m_cellSize(0),
   m_lastID(-1)
{
    drawButtons();
}

TrackButtons::TrackButtons(QWidget* parent,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(0), m_tracks(0), m_offset(0), m_cellSize(0),
   m_lastID(-1)
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
    // The buttonGap sets up the sizes of the buttons
    //
    int buttonGap = 8;

    // The borderGap sets the gaps between elements
    //
    int borderGap = 1;

    // Set the spacing between vertical elements
    //
    setSpacing(borderGap);

    // Create a buttonGap at the top of the layout widget
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

    // Create an exclusive buttongroup for record
    //
    m_recordButtonGroup = new QButtonGroup();
    m_recordButtonGroup->setExclusive(true);

    // Create a buttongroup for muting
    //
    m_muteButtonGroup = new QButtonGroup();
    m_muteButtonGroup->setExclusive(false);

    // Populate the widgets
    //
    for (int i = 0; i < m_tracks; i++)
    {
        // Create a horizontal box for each track
        //
        track = new QHBox(this);
        track->setMinimumSize(120, m_cellSize - borderGap);
        track->setMaximumSize(120, m_cellSize - borderGap);

        // Try a style for the box
        //
        track->setFrameStyle(StyledPanel);
        track->setFrameShape(StyledPanel);
        track->setFrameShadow(Raised);

        // Create buttons
        mute = new QPushButton(track);
        record = new QPushButton(track);

        mute->setFlat(true);
        record->setFlat(true);

        // Create a label
        //
        label = new QLabel(track);


        // Set the label from the Track object on the Composition
        //
        Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(i);

        // Enforce this
        //
        assert(track != 0);

        label->setText(QString(track->getLabel().c_str()));
        //label->setText(QString(m_doc->getComposition().getTracks()[i].getLabel().c_str()));
        //label->setText(QString("Track %1").arg(i));

        label->setMinimumSize(80, m_cellSize - buttonGap);
        label->setMaximumSize(80, m_cellSize - buttonGap);
        label->setIndent(7);

        // Insert the buttons into groups
        //
        m_recordButtonGroup->insert(record, i);
        m_muteButtonGroup->insert(mute, i);

        mute->setToggleButton(true);
        record->setToggleButton(true);

        mute->setText("M");
        record->setText("R"); 

        mute->setMinimumSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
        mute->setMaximumSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

        record->setMinimumSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
        record->setMaximumSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

        // set the mute button
        //
        if (track->isMuted())
            mute->setDown(true);

        // set the record button down
        //
        if (m_doc->getComposition().getRecordTrack() == i)
        {
            setRecordTrack(i);
            record->setDown(true);
        }

    }

    // Create a blank label at the bottom just to keep
    // the scrolling in step
    //
    label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumHeight(40);
    label->setMaximumHeight(40);


    connect(m_recordButtonGroup, SIGNAL(released(int)),
            this, SLOT(setRecordTrack(int)));

    connect(m_muteButtonGroup, SIGNAL(released(int)),
            this, SLOT(toggleMutedTrack(int)));

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


// Toggle a mute button
//
//
void
TrackButtons::toggleMutedTrack(int mutedTrack)
{
    if (mutedTrack < 0 || mutedTrack > m_tracks )
        return;

    bool set = true;

    if (m_doc->getComposition().getTrackByIndex(mutedTrack)->isMuted())
        set = false;

    m_doc->getComposition().getTrackByIndex(mutedTrack)->setMuted(set);
}


// Set a newly selected record button to a shocking palette and
// unset the palette on the record buttons we're jumping from.
//
//
void
TrackButtons::setRecordTrack(int recordTrack)
{
    if (recordTrack < 0 || recordTrack > m_tracks )
        return;

    // Unset the palette if we're jumping to another button
    if (m_lastID != recordTrack && m_lastID != -1)
    {
       m_recordButtonGroup->find(m_lastID)->unsetPalette();
       m_recordButtonGroup->find(m_lastID)->setDown(false);
    }

    m_doc->getComposition().setRecordTrack(recordTrack);
    m_recordButtonGroup->find(recordTrack)->setPalette
	(QPalette(RosegardenGUIColours::ActiveRecordTrack));
    m_lastID = recordTrack;
}




