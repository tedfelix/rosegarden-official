

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

#include "trackbuttons.h"
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <assert.h>
#include "Track.h"
#include "colours.h"
#include "tracklabel.h"
#include "vumeter.h"

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           QWidget* parent,
                           Rosegarden::TrackHeader *vHeader,
                           QHeader *hHeader,
                           const int &trackLabelWidth,
                           const char* name,
                           WFlags):
   QVBox(parent, name), m_doc(doc), m_lastID(-1),
   m_trackLabelWidth(trackLabelWidth)
{
    assert(vHeader != 0);
    assert(hHeader != 0);

    // Number of tracks on our view
    //
    m_tracks = vHeader->count();

    // The pixel offset from the top - just to overcome
    // the borders
    //
    m_offset = 4;
 
    // The height of the cells
    //
    m_cellSize = vHeader->sectionSize(0);

    // Now draw the buttons and labels and meters
    //
    drawButtons();
}

TrackButtons::~TrackButtons()
{
    std::vector<TrackVUMeter*>::iterator vuit = m_trackMeters.begin();

    for (; vuit != m_trackMeters.end(); vuit++)
    {
        delete(*vuit);
    }

    m_trackMeters.erase(m_trackMeters.begin(), m_trackMeters.end());

    std::vector<TrackLabel*>::iterator lit = m_trackLabels.begin();
    for (; lit != m_trackLabels.end(); lit++)
    {
        delete(*lit);
    }

    m_trackLabels.erase(m_trackLabels.begin(), m_trackLabels.end());
}

// Draw the mute and record buttons, track labels and VU meters
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

    TrackVUMeter *vuMeter;
    TrackLabel *trackLabel;

    // Populate the widgets
    //
    for (int i = 0; i < m_tracks; i++)
    {
        // Create a horizontal box for each track
        //
        track = new QHBox(this);
        track->setMinimumSize(m_trackLabelWidth, m_cellSize - borderGap);
        track->setMaximumSize(m_trackLabelWidth, m_cellSize - borderGap);

        // Try a style for the box
        //
        track->setFrameStyle(StyledPanel);
        track->setFrameShape(StyledPanel);
        track->setFrameShadow(Raised);

        // Insert a little gap
        label = new QLabel(track);
        label->setMinimumWidth(2);
        label->setMaximumWidth(2);

        // Create a VU meter
        vuMeter = new TrackVUMeter(track,
                              VUMeter::PeakHold,
                              25,
                              buttonGap,
                              i);

        m_trackMeters.push_back(vuMeter);

        // set an initial level to show the meter off
        //vuMeter->setLevel(1.0);

        // Create another little gap
        label = new QLabel(track);
        label->setMinimumWidth(2);
        label->setMaximumWidth(2);

        // Create buttons
        mute = new QPushButton(track);
        record = new QPushButton(track);

        mute->setFlat(true);
        record->setFlat(true);

        // Create a label
        //
        trackLabel = new TrackLabel(i, track);

        // Set the label from the Track object on the Composition
        //
        Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(i);

        // Enforce this
        //
        assert(track != 0);

        trackLabel->setText(QString(track->getLabel().c_str()));
        //label->setText(QString(m_doc->getComposition().getTracks()[i].getLabel().c_str()));
        //label->setText(QString("Track %1").arg(i));

        trackLabel->setMinimumSize(80, m_cellSize - buttonGap);
        trackLabel->setMaximumSize(80, m_cellSize - buttonGap);
        trackLabel->setIndent(7);

        connect(trackLabel, SIGNAL(renameTrack(QString, int)),
                            SLOT(renameTrack(QString, int)));

        // Store the TrackLabel pointer
        //
        m_trackLabels.push_back(trackLabel);

        connect(trackLabel, SIGNAL(released(int)),
                SLOT(labelSelected(int)));

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


// Create and return a vector of all muted tracks
//
//
std::vector<int>
TrackButtons::mutedTracks()
{
    std::vector<int> mutedTracks;

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


// Connected to the released(int) callback of the TrackLabels
//
void
TrackButtons::labelSelected(int trackNum)
{
    std::vector<TrackLabel *>::iterator tlpIt;

    for (tlpIt = m_trackLabels.begin();
         tlpIt != m_trackLabels.end();
         tlpIt++)
    {
        
        if ((*tlpIt)->trackNum() != trackNum &&
            (*tlpIt)->isSelected())
        {
            (*tlpIt)->setLabelHighlight(false);
        }
    }

    // Propagate this message upstairs
    //
    emit(trackSelected(trackNum));

}

// Return a vector of highlighted tracks by querying the TrackLabels
// for highlight state.
//
std::vector<int>
TrackButtons::getHighLightedTracks()
{
    std::vector<int> retList;
    std::vector<TrackLabel *>::iterator tlpIt;

    for (tlpIt = m_trackLabels.begin();
         tlpIt != m_trackLabels.end();
         tlpIt++)
    {
        if ((*tlpIt)->isSelected())
            retList.push_back((*tlpIt)->trackNum());
    }

    return retList;
}

void
TrackButtons::renameTrack(QString newName, int trackNum)
{
    Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(trackNum);
    track->setLabel(std::string(newName.data()));

    std::vector<TrackLabel*>::iterator it = m_trackLabels.begin();
    for (; it != m_trackLabels.end(); it++)
    {
        if ((*it)->trackNum() == trackNum)
        {
            (*it)->setText(newName);
            return;
        }
    }

}


void
TrackButtons::setTrackMeter(double value, int trackNum)
{
    std::vector<TrackVUMeter*>::iterator it = m_trackMeters.begin();
    
    for (; it != m_trackMeters.end(); it++)
    {
        if ((*it)->trackNum() == trackNum)
        {
            (*it)->setLevel(value);
            return;
        }
    }
}






