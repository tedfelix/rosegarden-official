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

#include "trackbuttons.h"

#include <klocale.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qbuttongroup.h>

#include <cassert>

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
    QFrame *trackHBox = 0;

    // Populate the widgets
    //
    for (TrackId i = 0; i < m_tracks; i++)
    {
        trackHBox = makeButton(i);

        m_layout->addWidget(trackHBox);
        m_trackHBoxes.push_back(trackHBox);
    }
}

QFrame* TrackButtons::makeButton(unsigned int trackId)
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
    Rosegarden::Track *track = m_doc->getComposition().getTrackByIndex(trackId);

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
    trackLabel = new TrackLabel(trackId, trackHBox);
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
    connect(trackLabel, SIGNAL(released(int)),
            SIGNAL(trackSelected(int)));

    // instrument label
    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    QString instrumentName;
    if (ins == 0)
        instrumentName = QString("<no instrument>");
    else
        instrumentName = QString(strtoqstr(ins->getName()));

    instrumentLabel = new InstrumentLabel(instrumentName,
                                          Rosegarden::InstrumentId(trackId),
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

    connect(instrumentLabel, SIGNAL(released(int)),
            SIGNAL(trackSelected(int)));


    // Insert the buttons into groups
    //
    m_recordButtonGroup->insert(record, trackId);
    m_muteButtonGroup->insert(mute, trackId);

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

    if (m_doc->getComposition().getTrackByIndex(mutedTrack)->isMuted())
        set = false;

    m_doc->getComposition().getTrackByIndex(mutedTrack)->setMuted(set);

    emit modified();
}

void
TrackButtons::slotUpdateTracks()
{
    TrackId newNbTracks = m_doc->getComposition().getNbTracks();

    if (newNbTracks == m_tracks) return; // nothing to do

    if (newNbTracks > m_tracks) {
        
        for(unsigned int i = m_tracks; i < newNbTracks; ++i) {
            QFrame *trackHBox = makeButton(i);
            if (trackHBox)
            {
                trackHBox->show();
                m_layout->insertWidget(i, trackHBox);
                m_trackHBoxes.push_back(trackHBox);
            }
        }

    } else {

        for(unsigned int i = m_tracks; i > newNbTracks; --i) {
            QFrame *trackHBox = m_trackHBoxes.back();
            delete trackHBox;
            m_trackHBoxes.pop_back();
        }

    }    
    
    m_tracks = newNbTracks;
}

// Set a newly selected record button to a shocking palette and
// unset the palette on the record buttons we're jumping from.
//
//
void
TrackButtons::slotSetRecordTrack(int recordTrack)
{
    setRecordButtonDown(recordTrack);

    // set and update
    m_doc->getComposition().setRecordTrack(recordTrack);
    emit newRecordButton();
}

void
TrackButtons::setRecordButtonDown(int recordTrack)
{
    if (recordTrack < 0 || recordTrack > (int)m_tracks )
        return;

    // Unset the palette if we're jumping to another button
    if (m_lastID != recordTrack && m_lastID != -1)
    {
       m_recordButtonGroup->find(m_lastID)->unsetPalette();
       dynamic_cast<QPushButton*>(
               m_recordButtonGroup->find(m_lastID))->setOn(false);
    }

    m_recordButtonGroup->find(recordTrack)->setPalette
	(QPalette(RosegardenGUIColours::ActiveRecordTrack));
    dynamic_cast<QPushButton*>(
            m_recordButtonGroup->find(recordTrack))->setOn(true);

    m_lastID = recordTrack;
}

void
TrackButtons::slotLabelSelected(int position)
{
    std::vector<TrackLabel *>::iterator tlpIt;
    bool changed;

    for (tlpIt = m_trackLabels.begin();
         tlpIt != m_trackLabels.end();
         tlpIt++)
    {
        if ((*tlpIt)->getPosition() != position)
        {
            changed = (*tlpIt)->isSelected();
            (*tlpIt)->setLabelHighlight(false);
        }
        else
        {
            (*tlpIt)->setLabelHighlight(true);
            changed = true;
        }

        if (changed) (*tlpIt)->update();
    }

    std::vector<InstrumentLabel *>::iterator it;

    for (it = m_instrumentLabels.begin();
         it != m_instrumentLabels.end();
         it++)
    {
        if ((*it)->getPosition() != position)
        {
            changed = (*it)->isSelected();
            (*it)->setLabelHighlight(false);
        }
        else
        {
            (*it)->setLabelHighlight(true);
            changed = true;
        }

        if (changed) (*it)->update();
    }
}



// Return a vector of highlighted tracks by querying the TrackLabels
// for highlight state.
//
std::vector<int>
TrackButtons::getHighlightedTracks()
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
    track->setLabel(qstrtostr(newName));

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

    std::vector<TrackVUMeter*>::iterator it = m_trackMeters.begin();
    
    for (; it != m_trackMeters.end(); it++)
    {
        track = comp.getTrackByPosition((*it)->getPosition());

        if (track !=0 && track->getInstrument() == id)
            (*it)->setLevel(value);
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
            int subOrderDepth = studio.getDevice(devId)->getSubOrderDepth();

            if (subOrderDepth)
            {
                QPopupMenu *groupMenu = new QPopupMenu(this);

                m_instrumentPopup->
                    insertItem(strtoqstr((*it)->getDevice()->getName()),
                               groupMenu);

                // store the first sub menu position
                groupBase = int(m_instrumentSubMenu.size());

                // Add a number of groups
                //
                for (int j = 0; j < subOrderDepth + 1; ++j)
                {
                    QPopupMenu *subMenu = new QPopupMenu(this);

                    /*
                    // A bit wide
                    QString label = strtoqstr((*it)->getDevice()->getName()) +
                        i18n(" (sub-group ") + QString("%1)").arg(j+1);
                    */

                    QString label = i18n("sub-group ") + QString("%1").arg(j+1);

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
	    m_instrumentSubMenu[groupBase + (*it)->getSubOrdering()]->
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
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    Rosegarden::Instrument *inst = studio.getInstrumentFromList(item);

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


