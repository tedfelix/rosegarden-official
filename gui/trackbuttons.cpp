// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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


#include <klocale.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qbuttongroup.h>

#include <cassert>

#include "trackbuttons.h"
#include "Track.h"
#include "Studio.h"
#include "colours.h"
#include "tracklabel.h"
#include "trackvumeter.h"
#include "instrumentlabel.h"

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"

using Rosegarden::TrackId;

TrackButtons::TrackButtons(RosegardenGUIDoc* doc,
                           unsigned int trackCellHeight,
                           unsigned int trackLabelWidth,
                           bool showTrackLabels,
                           int overallHeight,
                           QWidget* parent,
                           const char* name,
                           WFlags f)
    : QFrame(parent, name, f),
      m_doc(doc),
      m_recordButtonGroup(new QButtonGroup(this)),
      m_muteButtonGroup(new QButtonGroup(this)),
      m_layout(new QVBoxLayout(this)),
      m_instrumentPopup(new QPopupMenu(this)),
      m_tracks(doc->getComposition().getNbTracks()),
      m_offset(4),
      m_cellSize(trackCellHeight),
      m_borderGap(1),
      m_lastID(-1),
      m_trackLabelWidth(trackLabelWidth),
      m_popupItem(0)

{
    setFrameStyle(Plain);

    // when we create the widget, what are we looking at?
    if (showTrackLabels)
        m_trackInstrumentLabels = ShowTrack;
    else
        m_trackInstrumentLabels = ShowInstrument;

    // Set the spacing between vertical elements
    //
    m_layout->setSpacing(m_borderGap);

    // Create an exclusive buttongroup for record
    //
    m_recordButtonGroup->setExclusive(true);

    // Create a buttongroup for muting
    //
    m_muteButtonGroup->setExclusive(false);

    // Now draw the buttons and labels and meters
    //
    makeButtons();

    m_layout->addStretch(20);

    connect(m_recordButtonGroup, SIGNAL(released(int)),
            this, SLOT(slotSetRecordTrack(int)));

    connect(m_muteButtonGroup, SIGNAL(released(int)),
            this, SLOT(slotToggleMutedTrack(int)));

    // Populate instrument popup menu just once at start-up
    //
    populateInstrumentPopup();

    // We have to force the height for the moment
    //
    setMinimumHeight(overallHeight);

}

TrackButtons::~TrackButtons()
{
    std::vector<QPopupMenu*>::iterator mIt;
    for (mIt = m_instrumentSubMenu.begin();
         mIt != m_instrumentSubMenu.end(); mIt++)
        delete *mIt;

    m_instrumentSubMenu.clear();
}

// Draw the mute and record buttons, track labels and VU meters
//
void
TrackButtons::makeButtons()
{
    // Create a horizontal box for each track
    // plus the two buttons
    //

    /*
    QFrame *trackHBox = 0;

    // Populate the widgets
    //
    for (TrackId i = 0; i < m_tracks; i++)
    {
        trackHBox = makeButton(i);

        m_layout->addWidget(trackHBox);
        m_trackHBoxes.push_back(trackHBox);
    }
    */

    Rosegarden::Composition::trackcontainer &tracks =
        m_doc->getComposition().getTracks();
    Rosegarden::Composition::trackiterator it;

    for (it = tracks.begin(); it != tracks.end(); ++it)
    {
        QFrame *trackHBox = makeButton((*it).second->getId());

        if (trackHBox)
        {
            m_layout->addWidget(trackHBox);
            m_trackHBoxes.push_back(trackHBox);
        }
    }

    populateButtons();
}

QFrame* TrackButtons::makeButton(Rosegarden::TrackId trackId)
{
    // The buttonGap sets up the sizes of the buttons
    //
    static const int buttonGap = 8;

    QFrame *trackHBox = 0;

    QPushButton *mute = 0;
    QPushButton *record = 0;

    TrackVUMeter *vuMeter = 0;
    TrackLabel *trackLabel = 0;
    InstrumentLabel *instrumentLabel = 0;

    int vuWidth = 20;
    int vuSpacing = 2;
    int labelWidth = m_trackLabelWidth - ( (m_cellSize - buttonGap) * 2 +
                                            vuSpacing * 2 + vuWidth );

    // Set the label from the Track object on the Composition
    //
    Rosegarden::Track *track = m_doc->getComposition().getTrackById(trackId);

    if (track == 0) return 0;

    // Create a horizontal box for each track
    //
    trackHBox = new QFrame(this);
    QHBoxLayout *hblayout = new QHBoxLayout(trackHBox);
        
    trackHBox->setMinimumSize(labelWidth, m_cellSize - m_borderGap);
    trackHBox->setFixedHeight(m_cellSize - m_borderGap);

    // Try a style for the box
    //
    trackHBox->setFrameStyle(StyledPanel);
    trackHBox->setFrameShape(StyledPanel);
    trackHBox->setFrameShadow(Raised);

    // Insert a little gap
    hblayout->addSpacing(vuSpacing);

    // Create a VU meter
    vuMeter = new TrackVUMeter(trackHBox,
                               VUMeter::PeakHold,
                               vuWidth,
                               buttonGap,
                               track->getId());

    m_trackMeters.push_back(vuMeter);

    hblayout->addWidget(vuMeter);

    // Create another little gap
    hblayout->addSpacing(vuSpacing);

    // Create buttons
    mute = new QPushButton(trackHBox);
    hblayout->addWidget(mute);
    record = new QPushButton(trackHBox);
    hblayout->addWidget(record);

    mute->setFlat(true);
    record->setFlat(true);

    // Create a label
    //
    trackLabel = new TrackLabel(trackId, track->getPosition(), trackHBox);
    hblayout->addWidget(trackLabel);

    if (track->getLabel() == std::string(""))
        trackLabel->setText(i18n("<untitled>"));
    else
        trackLabel->setText(strtoqstr(track->getLabel()));

    trackLabel->setFixedSize(labelWidth, m_cellSize - buttonGap);
    trackLabel->setFixedHeight(m_cellSize - buttonGap);
    trackLabel->setIndent(7);

    if (m_trackInstrumentLabels == ShowInstrument)
    {
        trackLabel->hide();
    }
    else if (m_trackInstrumentLabels == ShowTrack)
    {
        connect(trackLabel, SIGNAL(changeToInstrumentList(int)),
                this, SLOT(slotInstrumentSelection(int)));
    }

    connect(trackLabel, SIGNAL(renameTrack(QString, int)),
            SLOT(slotRenameTrack(QString, int)));

    // Store the TrackLabel pointer
    //
    m_trackLabels.push_back(trackLabel);

    // Connect it
    connect(trackLabel, SIGNAL(released(int)), SIGNAL(trackSelected(int)));

    // instrument label
    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    QString instrumentName;
    if (ins == 0)
        instrumentName = QString("<no instrument>");
    else
        instrumentName = QString(strtoqstr(ins->getName()));

    instrumentLabel = new InstrumentLabel(instrumentName,
                                          trackId,
                                          trackHBox);

    instrumentLabel->setFixedSize(labelWidth, m_cellSize - buttonGap);
    instrumentLabel->setFixedHeight(m_cellSize - buttonGap);
    instrumentLabel->setIndent(7);
    hblayout->addWidget(instrumentLabel);

    // Set label to program change if it's being sent
    //
    if (ins != 0 && ins->sendsProgramChange())
        instrumentLabel->slotSetAlternativeLabel(
                QString(strtoqstr(ins->getProgramName())));

    // select instrument

    if (m_trackInstrumentLabels == ShowTrack)
    {
        instrumentLabel->hide();
    }
    else
    {
        connect(instrumentLabel, SIGNAL(changeToInstrumentList(int)),
                this, SLOT(slotInstrumentSelection(int)));
    }

    // insert label
    m_instrumentLabels.push_back(instrumentLabel);

    connect(instrumentLabel, SIGNAL(released(int)), SIGNAL(trackSelected(int)));

    // Insert the buttons into groups
    //
    m_recordButtonGroup->insert(record, trackId);
    m_muteButtonGroup->insert(mute, track->getPosition());

    mute->setToggleButton(true);
    record->setToggleButton(true);

    mute->setText("M");
    record->setText("R"); 

    mute->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
    record->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

    // set the mute button
    //
    if (track->isMuted())
        mute->setOn(true);

    // set the record button down
    //
    if (m_doc->getComposition().getRecordTrack() == trackId)
    {
        slotSetRecordTrack(trackId);
        record->setOn(true);
    }

    return trackHBox;
}


// Return the track that's currently set for recording
//
//
int TrackButtons::selectedRecordTrack()
{
   QButton *retButton = m_recordButtonGroup->selected();

   // if none selected
   if (!retButton)
     return -1;

   return m_recordButtonGroup->id(retButton);
}

// Fill out the buttons with Instrument information
//
void
TrackButtons::populateButtons()
{
    Rosegarden::Instrument *ins = 0;
    Rosegarden::Track *track;

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        track = m_doc->getComposition().getTrackByPosition(i);

        cout << "TRACK LABEL i = " << i 
             << " ID = " << track->getId() << " ";

        if (track)
        {
            cout << " GOT TRACK (" << track->getId() << ") ";

            ins = m_doc->getStudio().getInstrumentById(track->getInstrument());

            // Set mute button from track
            //
            if (track->isMuted())
                m_muteButtonGroup->find(i)->setDown(true);
            else
                m_muteButtonGroup->find(i)->setDown(false);

            // Set record button from track
            //
            if (m_doc->getComposition().getRecordTrack() == track->getId())
                slotSetRecordTrack(track->getId());

            // reset track tokens
            m_trackLabels[i]->setId(track->getId());
            m_instrumentLabels[i]->setId(track->getId());
        }

        if (ins)
        {
            cout << " GOT INSTRUMENT (" << ins->getId() << ")" ;
            m_instrumentLabels[i]->setText(strtoqstr(ins->getName()));
            if (ins->sendsProgramChange())
            {
                m_instrumentLabels[i]->
                    slotSetAlternativeLabel(strtoqstr(ins->getProgramName()));
            }

        }
        else
        {
            m_instrumentLabels[i]->setText(i18n("<no instrument>"));
        }
        m_instrumentLabels[i]->update();
        m_trackLabels[i]->update();

        cout << endl;
    }

}




// Create and return a vector of all muted tracks
//
//
std::vector<int>
TrackButtons::mutedTracks()
{
    std::vector<int> mutedTracks;

    for (TrackId i = 0; i < m_tracks; i++)
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
    if (mutedTrack < 0 || mutedTrack > (int)m_tracks )
        return;

    bool set = true;

    if (m_doc->getComposition().getTrackById(mutedTrack)->isMuted())
        set = false;

    m_doc->getComposition().getTrackById(mutedTrack)->setMuted(set);

    emit modified();
}

// Remove buttons from all iterators and delete object
//
void
TrackButtons::removeButtons(unsigned int position)
{
    cout << "DELETING TRACK BOX " << position << endl;

    unsigned int i = 0;
    std::vector<QFrame*>::iterator it;
    for (it = m_trackHBoxes.begin(); it != m_trackHBoxes.end(); ++it)
    {
        if (i == position) break;
        i++;
    }

    if (it != m_trackHBoxes.end())
        m_trackHBoxes.erase(it);

    i = 0;
    std::vector<InstrumentLabel*>::iterator iit;
    for (iit = m_instrumentLabels.begin();
         iit != m_instrumentLabels.end(); ++iit)
    {
        if (i == position) break;
        i++;
    }

    if (iit != m_instrumentLabels.end())
    {
        //delete (*iit);
        m_instrumentLabels.erase(iit);
    }

    i = 0;
    std::vector<TrackLabel*>::iterator tit;
    for (tit = m_trackLabels.begin(); tit != m_trackLabels.end(); ++tit)
    {
        if (i == position) break;
        i++;
    }

    if (tit != m_trackLabels.end()) 
    {
        //delete (*iit);
        m_trackLabels.erase(tit);
    }

    i = 0;
    std::vector<TrackVUMeter*>::iterator vit;
    for (vit = m_trackMeters.begin(); vit != m_trackMeters.end(); ++vit)
    {
        if (i == position) break;
        i++;
    }

    if (vit != m_trackMeters.end())
    {
        //delete (*vit);
        m_trackMeters.erase(vit);
    }

    // Get rid of the buttons
    //
    QButton *button = m_muteButtonGroup->find(position);
    m_muteButtonGroup->remove(button);
    //delete button;

    button = m_recordButtonGroup->find(position);
    m_recordButtonGroup->remove(button);
    //delete button;

    delete m_trackHBoxes[position];

}


void
TrackButtons::slotUpdateTracks()
{
    unsigned int newNbTracks = m_doc->getComposition().getNbTracks();

    if (newNbTracks == m_tracks) return; // nothing to do

    Rosegarden::Composition::trackcontainer &tracks =
        m_doc->getComposition().getTracks();
    Rosegarden::Composition::trackiterator it;
    bool match = false;

    // Delete any extra tracks based on position calculations -
    // move around TrackIds if we find a mismatch.  The important
    // thing is making sure we have valid and contiguous positions
    // and then juggle TrackIds accordingly.
    //
    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        match = false;
        for (it = tracks.begin(); it != tracks.end(); ++it)
        {
            // Map track positions to ids
            //
            if (i == ((unsigned int)(*it).second->getPosition()))
            {
                m_trackLabels[i]->setId((*it).second->getId());
                match = true;
                break;
            }
        }

        if (!match)
        {
            removeButtons(i);
        }
    }

    // Check for adding tracks
    //
    if (m_trackHBoxes.size() != newNbTracks)
    {
        for (it = tracks.begin(); it != tracks.end(); ++it)
        {
            cout << "TRACK ID = " << (*it).second->getId()
                 << " POSITION = " <<  (*it).second->getPosition() << endl;
            match = false;
            int j = 0;
            for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
            {
                if (m_trackLabels[i]->getId() == (*it).second->getId() &&
                    i == ((unsigned int)(*it).second->getPosition()))
                {
                    match = true;
                    break;
                }
            }

            if (!match)
            {
                QFrame *trackHBox = makeButton((*it).second->getId());

                if (trackHBox)
                {
                    cout << "MAKE ID = " << (*it).second->getId()
                         << " @ j = " << j << endl;
                    trackHBox->show();
                    m_layout->insertWidget((*it).second->getPosition(),
                                           trackHBox);
                    m_trackHBoxes.push_back(trackHBox);
                }
            }
            j++;
        }
    }

    // ensure that positioning is correct
    //
    Rosegarden::Track *track;
    Rosegarden::Instrument *inst;

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        if (m_trackLabels[i] != (*m_trackLabels.end()))
        {
            track = m_doc->getComposition().getTrackByPosition(i);

            if (track)
                m_trackLabels[i]->setId(track->getId());
        }

        cout << "SU : " << i
             << " : TRACK " << m_trackLabels[i]->getId()
             << endl;

        if (m_instrumentLabels[i] != (*m_instrumentLabels.end()))
        {
            if (track) m_instrumentLabels[i]->setId(track->getId());

        }

    }
    m_tracks = newNbTracks;

    // repopulate the buttons
    populateButtons();
}

// Set a newly selected record button to a shocking palette and
// unset the palette on the record buttons we're jumping from.
//
//
void
TrackButtons::slotSetRecordTrack(int position)
{
    setRecordButtonDown(position);

    // set and update
    m_doc->getComposition().setRecordTrack(m_trackLabels[position]->getId());
    emit newRecordButton();
}

void
TrackButtons::setRecordButtonDown(int position)
{
    if (position < 0 || position >= (int)m_tracks)
        return;

    // Unset the palette if we're jumping to another button
    if (m_lastID != position && m_lastID != -1)
    {
       m_recordButtonGroup->find(m_lastID)->unsetPalette();
       dynamic_cast<QPushButton*>(
               m_recordButtonGroup->find(m_lastID))->setOn(false);
    }

    m_recordButtonGroup->find(position)->setPalette
	(QPalette(RosegardenGUIColours::ActiveRecordTrack));
    dynamic_cast<QPushButton*>(
            m_recordButtonGroup->find(position))->setOn(true);

    m_lastID = position;
}

void
TrackButtons::selectLabel(int position)
{
    bool update = false;
    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        update = false;

        if (i == ((unsigned int)position))
        {
            if (!m_instrumentLabels[i]->isSelected())
            {
                update = true;
                m_trackLabels[i]->setSelected(true);
            }
        }
        else
        {
            if (m_instrumentLabels[i]->isSelected())
            {
                update = true;
                m_trackLabels[i]->setSelected(false);
            }
        }

        if (update) m_trackLabels[i]->update();
    }

    for (unsigned int i = 0; i < m_instrumentLabels.size(); ++i)
    {
        update = false;

        if (i == ((unsigned int)position))
        {
            if (!m_instrumentLabels[i]->isSelected())
            {
                update = true;
                m_instrumentLabels[i]->setSelected(true);
            }
        }
        else
        {
            if (m_instrumentLabels[i]->isSelected())
            {
                update = true;
                m_instrumentLabels[i]->setSelected(false);
            }
        }

        if (update) m_instrumentLabels[i]->update();
    }
}



// Return a vector of highlighted tracks by querying the TrackLabels
// for highlight state.
//
std::vector<int>
TrackButtons::getHighlightedTracks()
{
    std::vector<int> retList;

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        if (m_trackLabels[i]->isSelected())
            retList.push_back(i);
    }

    return retList;
}

void
TrackButtons::slotRenameTrack(QString newName, int trackNumber)
{
    Rosegarden::Track *track = m_doc->getComposition().getTrackById(trackNumber);
    track->setLabel(qstrtostr(newName));

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        if (i == ((unsigned int)trackNumber))
        {
            m_trackLabels[i]->setText(newName);
            emit widthChanged();
            return;
        }
    }
}


void
TrackButtons::slotSetTrackMeter(double value, int position)
{
    for (unsigned int i = 0; i < m_trackMeters.size(); ++i)
    {
        if (i == ((unsigned int)position))
        {
            m_trackMeters[i]->setLevel(value);
            return;
        }
    }
}

// For all Track meters get their position and from that
// value work out their Track and hence Instrument and
// equate that to the Instrument we're setting.
//
void
TrackButtons::slotSetMetersByInstrument(double value,
                                        Rosegarden::InstrumentId id)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track;

    for (unsigned int i = 0; i < m_trackMeters.size(); ++i)
    {
        track = comp.getTrackByPosition(i);

        if (track !=0 && track->getInstrument() == id)
            m_trackMeters[i]->setLevel(value);
    }
}


void
TrackButtons::slotInstrumentSelection(int position)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    QString instrumentName;
    Rosegarden::Track *track = comp.getTrackByPosition(position);

    if (track != 0)
    {
        Rosegarden::Instrument *ins = studio.
                getInstrumentById(track->getInstrument());

        if (ins == 0)
            instrumentName = QString("<no instrument>");
        else
            instrumentName = QString(strtoqstr(ins->getName()));
    }
    else
        instrumentName = QString("<no instrument>");
    //
    // populate this instrument widget
    m_instrumentLabels[position]->setText(instrumentName);

    // Hide the track label if we're in Track only mode
    if (m_trackInstrumentLabels == ShowTrack)
    {
       m_trackLabels[position]->hide();
       m_instrumentLabels[position]->show();
    }

    // Show the popup at the mouse click positon stored in
    // the track label
    //
    QPoint menuPos;

    // Get the correct menu position according to what's switched on
    //
    switch(m_trackInstrumentLabels)
    {
        case ShowInstrument:
            menuPos = m_instrumentLabels[position]->getPressPosition();
            break;

        case ShowTrack:
        case ShowBoth:
        default:
            menuPos = m_trackLabels[position]->getPressPosition();
            break;
    }

    //!!!
    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    //
    populateInstrumentPopup();

    m_instrumentPopup->popup(menuPos);

    // Store the popup item position
    //
    m_popupItem = position;

}

void
TrackButtons::populateInstrumentPopup()
{
    Rosegarden::Studio &studio = m_doc->getStudio();

    // clear the popup
    m_instrumentPopup->clear();

    // clear submenus
    std::vector<QPopupMenu*>::iterator mIt;
    for (mIt = m_instrumentSubMenu.begin();
         mIt != m_instrumentSubMenu.end(); mIt++)
        delete *mIt;
    m_instrumentSubMenu.clear();

    // position index
    int i = 0;

    // Get the list
    Rosegarden::InstrumentList list = studio.getPresentationInstruments();
    Rosegarden::InstrumentList::iterator it;
    int currentDevId = -1;
    int groupBase = -1;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it)) continue; // sanity check

	std::string iname((*it)->getName());
	std::string pname((*it)->getProgramName());
        Rosegarden::DeviceId devId = (*it)->getDevice()->getId();

	if (devId != (Rosegarden::DeviceId)(currentDevId)) {

            currentDevId = int(devId);

            // Check for sub-ordering
            //
#ifdef EXPERIMENTAL_ALSA_DRIVER
	    int subOrderDepth = 0;
#else
            int subOrderDepth =
                studio.getDevice(devId)->getPortNumbers().size();
#endif

            if (subOrderDepth > 1)
            {
                QPopupMenu *groupMenu = new QPopupMenu(this);

                m_instrumentPopup->
                    insertItem(strtoqstr((*it)->getDevice()->getName()),
                               groupMenu);

                // store the first sub menu position
                groupBase = int(m_instrumentSubMenu.size());

                // Add a number of groups
                //
                for (int j = 0; j < subOrderDepth; ++j)
                {
                    QPopupMenu *subMenu = new QPopupMenu(this);

                    /*
                    // A bit wide
                    QString label = strtoqstr((*it)->getDevice()->getName()) +
                        i18n(" (sub-group ") + QString("%1)").arg(j+1);
                    */

                    QString label = i18n("port ") + QString("%1").arg(j+1);

                    groupMenu->insertItem(label, subMenu);

                    // insert sub-menu
                    m_instrumentSubMenu.push_back(subMenu);

                    // connect it
                    connect(subMenu, SIGNAL(activated(int)),
                            SLOT(slotInstrumentPopupActivated(int)));

                    connect(subMenu, SIGNAL(aboutToHide()),
                            SLOT(slotInstrumentPopupHiding()));
                }
            }
            else // for no sub-ordering
            {
                QPopupMenu *subMenu = new QPopupMenu(this);
                m_instrumentPopup->
                    insertItem(strtoqstr((*it)->getDevice()->getName()),
                               subMenu);

                m_instrumentSubMenu.push_back(subMenu);

                // Connect up the submenu
                //
                connect(subMenu, SIGNAL(activated(int)),
                        SLOT(slotInstrumentPopupActivated(int)));

                connect(subMenu, SIGNAL(aboutToHide()),
                        SLOT(slotInstrumentPopupHiding()));
                groupBase = -1;
            }
	}

	if (pname != "") iname += " (" + pname + ")";

        if (groupBase == -1)
        {
	    m_instrumentSubMenu[m_instrumentSubMenu.size() - 1]->
                insertItem(strtoqstr(iname), i++);
        }
        else
        {
#ifndef EXPERIMENTAL_ALSA_DRIVER
            int position =  (*it)->getDevice()->getPortNumberPosition(
                                     (*it)->getPort());

            if (position == -1) position = 0;
#else
	    int position = 0;
#endif

	    m_instrumentSubMenu[groupBase + position]->
                insertItem(strtoqstr(iname), i++);
        }

    }

}

// Set the relevant Instrument for the Track
// according to popup position.
//
void
TrackButtons::slotInstrumentPopupActivated(int item)
{
    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated " << item << endl;
    
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    Rosegarden::Instrument *inst = studio.getInstrumentFromList(item);

    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated: instrument " << inst << endl;
    
    if (inst != 0)
    {
        Rosegarden::Track *track = comp.getTrackByPosition(m_popupItem);

        if (track != 0)
        {
            track->setInstrument(inst->getId());

            // select instrument
            emit instrumentSelected((int)inst->getId());

            m_instrumentLabels[m_popupItem]->
                    setText(strtoqstr(inst->getName()));

            // reset the alternative label
            m_instrumentLabels[m_popupItem]->clearAlternativeLabel();

            // Now see if the program is being shown for this instrument
            // and if so reset the label
            //
            if (inst->sendsProgramChange())
                m_instrumentLabels[m_popupItem]->slotSetAlternativeLabel(
                             QString(strtoqstr(inst->getProgramName())));

        }
        else
            cerr << "slotInstrumentPopupActivated() - can't find item!" << endl;
    }
    else
        cerr << "slotInstrumentPopupActivated() - can't find item!" << endl;

}

// Swap back the labels
void
TrackButtons::slotInstrumentPopupHiding()
{
    changeTrackInstrumentLabels(m_trackInstrumentLabels);
}


// Hide and show Tracks and Instruments
//
void
TrackButtons::changeTrackInstrumentLabels(InstrumentTrackLabels label)
{
    // Disconnect where we're coming from
    //
    if (m_trackInstrumentLabels != label)
    {
        for (int i = 0; i < (int)m_tracks; i++)
        {
            switch(m_trackInstrumentLabels)
            {
                case ShowTrack:
                    disconnect(m_trackLabels[i],
                               SIGNAL(changeToInstrumentList(int)),
                               this, SLOT(slotInstrumentSelection(int)));
                    break;

                case ShowBoth:
                case ShowInstrument:
                    disconnect(m_instrumentLabels[i],
                               SIGNAL(changeToInstrumentList(int)),
                               this, SLOT(slotInstrumentSelection(int)));
                    break;

                default:
                    break;
            }
        }
    }

    // Set new label
    m_trackInstrumentLabels = label;

    // update and reconnect with new value
    for (int i = 0; i < (int)m_tracks; i++)
    {
        switch(label)
        {
            case ShowInstrument:
                m_trackLabels[i]->hide();
                m_instrumentLabels[i]->show();
                connect(m_instrumentLabels[i],
                        SIGNAL(changeToInstrumentList(int)),
                        this, SLOT(slotInstrumentSelection(int)));
                break;
    
            case ShowBoth:
                m_trackLabels[i]->show();
                m_instrumentLabels[i]->show();
                connect(m_instrumentLabels[i],
                        SIGNAL(changeToInstrumentList(int)),
                        this, SLOT(slotInstrumentSelection(int)));
                break;
    
            case ShowTrack:
            default:
                m_trackLabels[i]->show();
                m_instrumentLabels[i]->hide();
                connect(m_trackLabels[i],
                        SIGNAL(changeToInstrumentList(int)),
                        this, SLOT(slotInstrumentSelection(int)));
                break;
        }
    }
}

// Set all the labels that are showing InstrumentId to show a 
// new label - this is usually driven by enabling program change
// sending for this instrument
//
void
TrackButtons::changeInstrumentLabel(Rosegarden::InstrumentId id, QString label)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track;

    for (int i = 0; i < (int)m_tracks; i++)
    {
        track = comp.getTrackByPosition(i);

        if(track && track->getInstrument() == id)
            m_instrumentLabels[i]->slotSetAlternativeLabel(label);
    }
}


void
TrackButtons::slotSynchroniseWithComposition()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::Track *track;
    QString instrumentName;

    for (int i = 0; i < (int)m_tracks; i++)
    {
        track = comp.getTrackByPosition(i);

        if (track)
        {
            if (track->isMuted())
                dynamic_cast<QPushButton*>(
                        m_muteButtonGroup->find(i))->setOn(true);
            else
                dynamic_cast<QPushButton*>(
                        m_muteButtonGroup->find(i))->setOn(false);

            Rosegarden::Instrument *ins = studio.
                getInstrumentById(track->getInstrument());

            if (ins == 0)
                instrumentName = QString("<no instrument>");
            else
                instrumentName = QString(strtoqstr(ins->getName()));

            m_instrumentLabels[i]->setText(instrumentName);
        }
    }

    setRecordButtonDown(comp.getRecordTrack());
}

void
TrackButtons::slotLabelSelected(int position)
{
    Rosegarden::Track *track =
        m_doc->getComposition().getTrackByPosition(position);

    if (track)
    {
        emit trackSelected(track->getId());
    }
}




