// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include <kapp.h>
#include <kglobal.h>
#include <kstddirs.h>

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

QString
TrackButtons::getPresentationName(Rosegarden::Instrument *instr)
{
    if (!instr) {
	return i18n("<no instrument>");
    } else if (instr->getType() == Rosegarden::Instrument::Audio) {
	return strtoqstr(instr->getName());
    } else {
	return strtoqstr(instr->getDevice()->getName() + " " + 
			 instr->getName());
    }
}


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

    connect(m_recordButtonGroup, SIGNAL(clicked(int)),
            this, SLOT(slotSetRecordTrack(int)));

    connect(m_muteButtonGroup, SIGNAL(clicked(int)),
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
                               track->getPosition());

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

    QString instrumentName(getPresentationName(ins));

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

    if (m_trackInstrumentLabels == ShowInstrument)
        trackLabel->hide();
    else
        instrumentLabel->hide();

    connect(trackLabel, SIGNAL(changeToInstrumentList(int)),
            this, SLOT(slotInstrumentSelection(int)));

    connect(instrumentLabel, SIGNAL(changeToInstrumentList(int)),
            this, SLOT(slotInstrumentSelection(int)));

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

        if (track)
        {
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
                setRecordTrack(track->getPosition());

            // reset track tokens
            m_trackLabels[i]->setId(track->getId());
            m_trackLabels[i]->setPosition(i);
            m_instrumentLabels[i]->setId(track->getId());
        }

        if (ins)
        {
            m_instrumentLabels[i]->setText(getPresentationName(ins));
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

    Rosegarden::Track *track = 
        m_doc->getComposition().getTrackByPosition(mutedTrack);

    emit muteButton(track->getId(), !track->isMuted()); // will set the value
}

// Remove buttons from all iterators and delete object
//
void
TrackButtons::removeButtons(unsigned int position)
{
    std::cerr << "TrackButtons::removeButtons - "
              << "deleting track button at position "
              << position << std::endl;

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
    Rosegarden::Composition &comp = m_doc->getComposition();
    unsigned int newNbTracks = comp.getNbTracks();
    Rosegarden::Track *track = 0;

    track = comp.getTrackById(comp.getRecordTrack());
    if (track)
        setRecordTrack(track->getPosition());

    if (newNbTracks == m_tracks)
    {
        populateButtons();
        return;
    }
    else if (newNbTracks < m_tracks)
    {
        for (unsigned int i = m_tracks; i > newNbTracks; --i)
            removeButtons(i - 1);
    }
    else // newNbTracks > m_tracks
    {
        for (unsigned int i = m_tracks; i < newNbTracks; ++i)
        {
            track = m_doc->getComposition().getTrackByPosition(i);
            if (track)
            {
                QFrame *trackHBox = makeButton(track->getId());

                if (trackHBox)
                {
                    trackHBox->show();
                    m_layout->insertWidget(i, trackHBox);
                    m_trackHBoxes.push_back(trackHBox);
                }
            }
            else
                std::cerr << "TrackButtons::slotUpdateTracks - "
                          << "can't find TrackId for position " << i << std::endl;
        }
    }

    // Renumber all the labels
    //
    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
        if (m_trackLabels[i] != (*m_trackLabels.end()))
        {
            track = comp.getTrackByPosition(i);

            if (track)
                m_trackLabels[i]->setId(track->getId());
        }

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
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track = comp.getTrackByPosition(position);

    setRecordTrack(position);
    selectLabel(position);

    comp.setSelectedTrack(track->getId());
    emit newRecordButton();
}

void
TrackButtons::setRecordTrack(int position)
{
    setRecordButtonDown(position);

    // set and update
    m_doc->getComposition().setRecordTrack(m_trackLabels[position]->getId());
}

void
TrackButtons::setRecordButtonDown(int position)
{
    if (position < 0 || position >= (int)m_tracks)
        return;

    if (m_recordButtonGroup->find(position) == 0) return;

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
        {
            m_trackMeters[i]->setLevel(value);
            //cout << "SETTING LEVEL = " << value << " ON INS " << id << endl;
        }
    }
}


void
TrackButtons::slotInstrumentSelection(int trackId)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    int position = comp.getTrackById(trackId)->getPosition();

    QString instrumentName;
    Rosegarden::Track *track = comp.getTrackByPosition(position);

    Rosegarden::Instrument *instrument = 0;
    if (track != 0)
	instrument = studio.getInstrumentById(track->getInstrument());
    instrumentName = getPresentationName(instrument);

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

    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    //
    populateInstrumentPopup(instrument);

    m_instrumentPopup->popup(menuPos);

    // Store the popup item position
    //
    m_popupItem = position;

}

void
TrackButtons::populateInstrumentPopup(Rosegarden::Instrument *thisTrackInstr)
{
    static QPixmap     connectedPixmap, unconnectedPixmap,
	           connectedUsedPixmap, unconnectedUsedPixmap,
	       connectedSelectedPixmap, unconnectedSelectedPixmap;
    static bool havePixmaps = false;

    if (!havePixmaps) {

	QString pixmapDir =
	    KGlobal::dirs()->findResource("appdata", "pixmaps/");

	connectedPixmap.load
	    (QString("%1/misc/connected.xpm").arg(pixmapDir));
	connectedUsedPixmap.load
	    (QString("%1/misc/connected-used.xpm").arg(pixmapDir));
	connectedSelectedPixmap.load
	    (QString("%1/misc/connected-selected.xpm").arg(pixmapDir));
	unconnectedPixmap.load
	    (QString("%1/misc/unconnected.xpm").arg(pixmapDir));
	unconnectedUsedPixmap.load
	    (QString("%1/misc/unconnected-used.xpm").arg(pixmapDir));
	unconnectedSelectedPixmap.load
	    (QString("%1/misc/unconnected-selected.xpm").arg(pixmapDir));
	
	havePixmaps = true;
    }
    
    Rosegarden::Composition &comp = m_doc->getComposition();
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
    bool deviceUsedByAnyone = false;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it)) continue; // sanity check

	QString iname(getPresentationName(*it));
	QString pname(strtoqstr((*it)->getProgramName()));
	Rosegarden::Device *device = (*it)->getDevice();
        Rosegarden::DeviceId devId = device->getId();

	bool instrUsedByMe = false;
	bool instrUsedByAnyone = false;

	if (thisTrackInstr && thisTrackInstr->getId() == (*it)->getId()) {
	    instrUsedByMe = true;
	    instrUsedByAnyone = true;
	}

	if (devId != (Rosegarden::DeviceId)(currentDevId)) {
	
	    deviceUsedByAnyone = false;

	    if (instrUsedByMe) deviceUsedByAnyone = true;
	    else {
		for (Rosegarden::Composition::trackcontainer::iterator tit =
			 comp.getTracks().begin();
		     tit != comp.getTracks().end(); ++tit) {

		    if (tit->second->getInstrument() == (*it)->getId()) {
			instrUsedByAnyone = true;
			deviceUsedByAnyone = true;
			break;
		    }

		    Rosegarden::Instrument *instr =
			studio.getInstrumentById(tit->second->getInstrument());
		    if (instr && (instr->getDevice()->getId() == devId)) {
			deviceUsedByAnyone = true;
		    }
		}
	    }

	    QIconSet iconSet
		(device->getConnection() == "" ?
		 (deviceUsedByAnyone ? 
		  unconnectedUsedPixmap : unconnectedPixmap) :
		 (deviceUsedByAnyone ? 
		  connectedUsedPixmap : connectedPixmap));

            currentDevId = int(devId);

	    QPopupMenu *subMenu = new QPopupMenu(this);
	    QString deviceName = strtoqstr(device->getName());
	    m_instrumentPopup->insertItem(iconSet, deviceName, subMenu);
	    m_instrumentSubMenu.push_back(subMenu);
	    
	    // Connect up the submenu
	    //
	    connect(subMenu, SIGNAL(activated(int)),
		    SLOT(slotInstrumentPopupActivated(int)));
	    
	    connect(subMenu, SIGNAL(aboutToHide()),
		    SLOT(slotInstrumentPopupHiding()));

	} else if (!instrUsedByMe) {
	
	    for (Rosegarden::Composition::trackcontainer::iterator tit =
		     comp.getTracks().begin();
		 tit != comp.getTracks().end(); ++tit) {

		if (tit->second->getInstrument() == (*it)->getId()) {
		    instrUsedByAnyone = true;
		    break;
		}
	    }
	}

	QIconSet iconSet
	    (device->getConnection() == "" ?
	     (instrUsedByAnyone ? 
	      instrUsedByMe ?
	      unconnectedSelectedPixmap :
	      unconnectedUsedPixmap : unconnectedPixmap) :
	     (instrUsedByAnyone ? 
	      instrUsedByMe ?
	      connectedSelectedPixmap :
	      connectedUsedPixmap : connectedPixmap));

	if (pname != "") iname += " (" + pname + ")";

	m_instrumentSubMenu[m_instrumentSubMenu.size() - 1]->
	    insertItem(iconSet, iname, i++);
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
                    setText(getPresentationName(inst));

            // reset the alternative label
            m_instrumentLabels[m_popupItem]->clearAlternativeLabel();

            // Now see if the program is being shown for this instrument
            // and if so reset the label
            //
            if (inst->sendsProgramChange())
                m_instrumentLabels[m_popupItem]->slotSetAlternativeLabel(
                             QString(strtoqstr(inst->getProgramName())));

            // Ensure that we set a record track properly
            //
            if (track->getId() == comp.getRecordTrack())
                slotSetRecordTrack(track->getId());

        }
        else
            std::cerr << "slotInstrumentPopupActivated() - can't find item!" << std::endl;
    }
    else
        std::cerr << "slotInstrumentPopupActivated() - can't find item!" << std::endl;

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
    // Set new label
    m_trackInstrumentLabels = label;

    // update and reconnect with new value
    for (int i = 0; i < (int)m_tracks; i++)
    {
        switch(label)
        {
            case ShowInstrument:
                m_trackLabels[i]->hide();
                m_trackLabels[i]->blockSignals(true);
                m_instrumentLabels[i]->show();
                m_instrumentLabels[i]->blockSignals(false);
                break;
    
            case ShowBoth:
                m_trackLabels[i]->show();
                m_trackLabels[i]->blockSignals(false);
                m_instrumentLabels[i]->show();
                m_instrumentLabels[i]->blockSignals(false);
                break;
    
            case ShowTrack:
            default:
                m_trackLabels[i]->show();
                m_trackLabels[i]->blockSignals(false);
                m_instrumentLabels[i]->hide();
                m_instrumentLabels[i]->blockSignals(true);
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

	    instrumentName = getPresentationName(ins);

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


// Set a mute button to a particular state
void
TrackButtons::setMuteButton(TrackId track, bool value)
{
    Rosegarden::Track *trackObj = m_doc->getComposition().getTrackById(track);
    if (trackObj == 0) return;

    dynamic_cast<QPushButton*>(m_muteButtonGroup->find(trackObj->getPosition()))
        ->setOn(value);
}




