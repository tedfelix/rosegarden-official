

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

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <rosegardenguidoc.h>
#include <qbuttongroup.h>

#include <assert.h>

#include "Track.h"
#include "colours.h"
#include "tracklabel.h"
#include "trackvumeter.h"
#include "instrumentlabel.h"


TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           unsigned int trackCellHeight,
                           unsigned int trackLabelWidth,
                           QWidget* parent,
                           const char* name,
                           WFlags f)
    : QFrame(parent, name, f),
      m_doc(doc),
      m_recordButtonGroup(new QButtonGroup(this)),
      m_muteButtonGroup(new QButtonGroup(this)),
      m_layout(new QVBoxLayout(this)),
      m_tracks(doc->getComposition().getNbTracks()),
      m_offset(4),
      m_cellSize(trackCellHeight),
      m_lastID(-1),
      m_trackLabelWidth(trackLabelWidth)
{
    setFrameStyle(Plain);

    m_instrumentPopup = new QPopupMenu(this);

    // Now draw the buttons and labels and meters
    //
    drawButtons();
}

TrackButtons::~TrackButtons()
{
    // Just doing most of these because they're there - not because
    // they necessarily need to be explicitly free'd

    delete m_instrumentPopup;

    for (std::vector<TrackLabel *>::iterator it = m_trackLabels.begin();
         it != m_trackLabels.end(); it++)
        delete (*it);

    m_trackLabels.erase(m_trackLabels.begin(), m_trackLabels.end());

    for (std::vector<TrackVUMeter *>::iterator it = m_trackMeters.begin();
         it != m_trackMeters.end(); it++)
        delete (*it);

    m_trackMeters.erase(m_trackMeters.begin(), m_trackMeters.end());

    for (std::vector<InstrumentLabel *>::iterator it = 
         m_instrumentLabels.begin(); it != m_instrumentLabels.end(); it++)
        delete (*it);

    m_instrumentLabels.erase(m_instrumentLabels.begin(),
                             m_instrumentLabels.end());

    delete m_recordButtonGroup;
    delete m_muteButtonGroup;
    delete m_layout;

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
    m_layout->setSpacing(borderGap);

    // Create a horizontal box for each track
    // plus the two buttons
    //
    QFrame *trackHBox = 0;
    QPushButton *mute = 0;
    QPushButton *record = 0;

    // Create an exclusive buttongroup for record
    //
    m_recordButtonGroup->setExclusive(true);

    // Create a buttongroup for muting
    //
    m_muteButtonGroup->setExclusive(false);

    TrackVUMeter *vuMeter;
    TrackLabel *trackLabel;
    InstrumentLabel *instrumentLabel;

    // Populate the widgets
    //
    for (int i = 0; i < m_tracks; i++)
    {
        // Set the label from the Track object on the Composition
        //
        Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(i);

        // Create a horizontal box for each track
        //
        trackHBox = new QFrame(this);
        QHBoxLayout *hblayout = new QHBoxLayout(trackHBox);
        
        trackHBox->setMinimumSize(m_trackLabelWidth, m_cellSize - borderGap);
        trackHBox->setFixedHeight(m_cellSize - borderGap);
        //trackHBox->setMaximumSize(m_trackLabelWidth, m_cellSize - borderGap);

        // Try a style for the box
        //
        trackHBox->setFrameStyle(StyledPanel);
        trackHBox->setFrameShape(StyledPanel);
        trackHBox->setFrameShadow(Raised);

        // Insert a little gap
        hblayout->addSpacing(2);

        // Create a VU meter
        vuMeter = new TrackVUMeter(trackHBox,
                                   VUMeter::PeakHold,
                                   25,
                                   buttonGap,
                                   track->getID());

        m_trackMeters.push_back(vuMeter);

        hblayout->addWidget(vuMeter);

        // Create another little gap
        hblayout->addSpacing(2);

        // Create buttons
        mute = new QPushButton(trackHBox);
        hblayout->addWidget(mute);
        record = new QPushButton(trackHBox);
        hblayout->addWidget(record);

        mute->setFlat(true);
        record->setFlat(true);

        // Create a label
        //
        trackLabel = new TrackLabel(i, trackHBox);
        hblayout->addWidget(trackLabel);

        // instrument label
        instrumentLabel = new InstrumentLabel((Rosegarden::InstrumentId)i,
                                               trackHBox);
        hblayout->addWidget(instrumentLabel);
        instrumentLabel->hide();

        // insert label
        m_instrumentLabels.push_back(instrumentLabel);

        connect(trackLabel, SIGNAL(changeToInstrumentList(int)),
                this, SLOT(slotInstrumentSelection(int)));

        // Enforce this
        //
        assert(track != 0);

        trackLabel->setText(QString(track->getLabel().c_str()));

        trackLabel->setMinimumSize(80, m_cellSize - buttonGap);
        //trackLabel->setMaximumSize(80, m_cellSize - buttonGap);
        trackLabel->setFixedHeight(m_cellSize - buttonGap);
        trackLabel->setIndent(7);

        connect(trackLabel, SIGNAL(renameTrack(QString, int)),
                            SLOT(slotRenameTrack(QString, int)));

        // Store the TrackLabel pointer
        //
        m_trackLabels.push_back(trackLabel);

        connect(trackLabel, SIGNAL(released(int)),
                SLOT(slotLabelSelected(int)));

        // Insert the buttons into groups
        //
        m_recordButtonGroup->insert(record, i);
        m_muteButtonGroup->insert(mute, i);

        mute->setToggleButton(true);
        record->setToggleButton(true);

        mute->setText("M");
        record->setText("R"); 

        mute->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
        record->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

        // set the mute button
        //
        if (track->isMuted())
            mute->setDown(true);

        // set the record button down
        //
        if (m_doc->getComposition().getRecordTrack() == i)
        {
            slotSetRecordTrack(i);
            record->setDown(true);
        }

        m_layout->addWidget(trackHBox);
    }

    m_layout->addStretch(20);

    connect(m_recordButtonGroup, SIGNAL(released(int)),
            this, SLOT(slotSetRecordTrack(int)));

    connect(m_muteButtonGroup, SIGNAL(released(int)),
            this, SLOT(slotToggleMutedTrack(int)));

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
TrackButtons::slotToggleMutedTrack(int mutedTrack)
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
TrackButtons::slotSetRecordTrack(int recordTrack)
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
TrackButtons::slotLabelSelected(int position)
{
    std::vector<TrackLabel *>::iterator tlpIt;

    for (tlpIt = m_trackLabels.begin();
         tlpIt != m_trackLabels.end();
         tlpIt++)
    {
        
        if ((*tlpIt)->getPosition() != position &&
            (*tlpIt)->isSelected())
        {
            (*tlpIt)->setLabelHighlight(false);
        }
    }

    // Propagate this message upstairs
    //
    emit(trackSelected(position));

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
            retList.push_back((*tlpIt)->getPosition());
    }

    return retList;
}

void
TrackButtons::slotRenameTrack(QString newName, int trackNumber)
{
    Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(trackNumber);
    track->setLabel(std::string(newName.data()));

    std::vector<TrackLabel*>::iterator it = m_trackLabels.begin();
    for (; it != m_trackLabels.end(); it++)
    {
        if ((*it)->getPosition() == trackNumber)
        {
            (*it)->setText(newName);
            emit widthChanged();
            return;
        }
    }
}


void
TrackButtons::slotSetTrackMeter(double value, int position)
{
    std::vector<TrackVUMeter*>::iterator it = m_trackMeters.begin();
    
    for (; it != m_trackMeters.end(); it++)
    {
        if ((*it)->getPosition() == position)
        {
            (*it)->setLevel(value);
            return;
        }
    }
}


void
TrackButtons::slotInstrumentSelection(int position)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    // populate this instrument widget
    m_instrumentLabels[position]->setText(QString("Instrument"));

    Rosegarden::Composition::instrumentiterator it;
    for (it = comp.getInstruments()->begin();
         it != comp.getInstruments()->end(); it++)
    {
        m_instrumentPopup->
            insertItem(QString((*it).second->getName().c_str()));
    }

    // Hide the track label
    m_trackLabels[position]->hide();

    // Show the instrument label
    m_instrumentPopup->show();

}





