// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qbuttongroup.h>
#include <qsignalmapper.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>

#include "trackbuttons.h"
#include "Track.h"
#include "Studio.h"
#include "colours.h"
#include "tracklabel.h"

#include "segmentcommands.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"

using Rosegarden::TrackId;

/**
 * @author Stefan Schimanski
 * Taken from KMix code,
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 */
KLedButton::KLedButton(const QColor &col, QWidget *parent, const char *name)
   : KLed( col, parent, name )
{	
}

KLedButton::KLedButton(const QColor& col, KLed::State st, KLed::Look look,
		       KLed::Shape shape, QWidget *parent, const char *name)
   : KLed( col, st, look, shape, parent, name )
{
}

KLedButton::~KLedButton()
{
}

void KLedButton::mousePressEvent( QMouseEvent *e )
{
   if (e->button() == LeftButton)
   {
      toggle();
      emit stateChanged( state() );
   }
}
///////////////////////////////////////////////////////////


TrackVUMeter::TrackVUMeter(QWidget *parent,
                           VUMeterType type,
                           int width,
                           int height,
                           int position,
                           const char *name):
    VUMeter(parent, type, false, width, height, VUMeter::Horizontal, name),
    m_position(position), m_textHeight(12)
{
    setAlignment(AlignCenter);
}


void
TrackVUMeter::meterStart()
{
    clear();
    setMinimumHeight(m_originalHeight);
    setMaximumHeight(m_originalHeight);
}


void
TrackVUMeter::meterStop()
{
    setMinimumHeight(m_textHeight);
    setMaximumHeight(m_textHeight);
    setText(QString("%1").arg(m_position + 1));
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
      m_layout(new QVBoxLayout(this)),
      m_recordSigMapper(new QSignalMapper(this)),
      m_muteSigMapper(new QSignalMapper(this)),
      m_clickedSigMapper(new QSignalMapper(this)),
      m_instListSigMapper(new QSignalMapper(this)),
      m_tracks(doc->getComposition().getNbTracks()),
      m_offset(4),
      m_cellSize(trackCellHeight),
      m_borderGap(1),
      m_lastID(-1),
      m_trackLabelWidth(trackLabelWidth),
      m_popupItem(0),
      m_lastSelected(-1)
{
    setFrameStyle(Plain);

    // when we create the widget, what are we looking at?
    if (showTrackLabels)
        m_trackInstrumentLabels = TrackLabel::ShowTrack;
    else
        m_trackInstrumentLabels = TrackLabel::ShowInstrument;

    // Set the spacing between vertical elements
    //
    m_layout->setSpacing(m_borderGap);

    // Now draw the buttons and labels and meters
    //
    makeButtons();

    m_layout->addStretch(20);

    connect(m_recordSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSetRecordTrack(int)));

    connect(m_muteSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleMutedTrack(int)));

    // connect signal mappers
    connect(m_instListSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotInstrumentSelection(int)));

    connect(m_clickedSigMapper, SIGNAL(mapped(int)),
            this, SIGNAL(trackSelected(int)));

//     // Populate instrument popup menu just once at start-up
//     //
//     populateInstrumentPopup();

    // We have to force the height for the moment
    //
    setMinimumHeight(overallHeight);

}

TrackButtons::~TrackButtons()
{
}

// Draw the mute and record buttons, track labels and VU meters
//
void
TrackButtons::makeButtons()
{
    if (!m_doc) return;

    // Create a horizontal box for each track
    // plus the two buttons
    //
    unsigned int nbTracks = m_doc->getComposition().getNbTracks();

    for (unsigned int i = 0; i < nbTracks; ++i)
    {
        Rosegarden::Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (track)
        {
            QFrame *trackHBox = makeButton(track->getId());

            if (trackHBox)
            {
                m_layout->addWidget(trackHBox);
                m_trackHBoxes.push_back(trackHBox);
            }
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

    KLedButton *mute = 0;
    KLedButton *record = 0;

    TrackVUMeter *vuMeter = 0;
    TrackLabel *trackLabel = 0;

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

    //
    // 'mute' and 'record' leds
    //
    mute = new KLedButton(RosegardenGUIColours::MuteTrackLED, trackHBox);
    QToolTip::add(mute, i18n("Mute track"));
    hblayout->addWidget(mute);
    record = new KLedButton(RosegardenGUIColours::RecordTrackLED, trackHBox);
    QToolTip::add(record, i18n("Record on this track"));
    hblayout->addWidget(record);

    record->setLook(KLed::Sunken);
    mute->setLook(KLed::Sunken);
    record->off();

    // Connect them to their sigmappers
    connect(record, SIGNAL(stateChanged(bool)),
            m_recordSigMapper, SLOT(map()));
    connect(mute, SIGNAL(stateChanged(bool)),
            m_muteSigMapper, SLOT(map()));
    m_recordSigMapper->setMapping(record, trackId);
    m_muteSigMapper->setMapping(mute, trackId);

    // Store the KLedButton
    //
    m_muteLeds.push_back(mute);
    m_recordLeds.push_back(record);


    //
    // Track label
    //
    trackLabel = new TrackLabel(trackId, track->getPosition(), trackHBox);
    hblayout->addWidget(trackLabel);

    if (track->getLabel() == std::string("")) {
	Rosegarden::Instrument *ins =
	    m_doc->getStudio().getInstrumentById(track->getInstrument());
	if (ins && ins->getType() == Rosegarden::Instrument::Audio) {
	    trackLabel->getTrackLabel()->setText(i18n("<untitled audio>"));
	} else {
	    trackLabel->getTrackLabel()->setText(i18n("<untitled>"));
	}
    }
    else
        trackLabel->getTrackLabel()->setText(strtoqstr(track->getLabel()));

    trackLabel->setFixedSize(labelWidth, m_cellSize - buttonGap);
    trackLabel->setFixedHeight(m_cellSize - buttonGap);
    trackLabel->setIndent(7);

    connect(trackLabel, SIGNAL(renameTrack(QString, Rosegarden::TrackId)),
            SLOT(slotRenameTrack(QString, Rosegarden::TrackId)));

    // Store the TrackLabel pointer
    //
    m_trackLabels.push_back(trackLabel);

    // Connect it
    setButtonMapping(trackLabel, trackId);

    connect(trackLabel, SIGNAL(changeToInstrumentList()),
            m_instListSigMapper, SLOT(map()));
    connect(trackLabel, SIGNAL(clicked()),
            m_clickedSigMapper, SLOT(map()));

    //
    // instrument label
    //
    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    QString instrumentName(i18n("<no instrument>"));
    if (ins) instrumentName = strtoqstr(ins->getPresentationName());

    // Set label to program change if it's being sent
    //
    if (ins != 0 && ins->sendsProgramChange())
        trackLabel->setAlternativeLabel(strtoqstr(ins->getProgramName()));

    trackLabel->showLabel(m_trackInstrumentLabels);

    mute->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
    record->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

    // set the mute button
    //
    if (track->isMuted())
        mute->off();

    return trackHBox;
}


void TrackButtons::setButtonMapping(QObject* obj, Rosegarden::TrackId trackId)
{
    m_clickedSigMapper->setMapping(obj, trackId);
    m_instListSigMapper->setMapping(obj, trackId);
}



// Return the track that's currently set for recording
//
//
int TrackButtons::selectedRecordTrack()
{
    return m_lastID;
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
                m_muteLeds[i]->off();
            else
                m_muteLeds[i]->on();

            // Set record button from track
            //
            if (m_doc->getComposition().getRecordTrack() == track->getId())
                setRecordTrack(track->getPosition());

            // reset track tokens
            m_trackLabels[i]->setId(track->getId());
            setButtonMapping(m_trackLabels[i], track->getId());
            m_trackLabels[i]->setPosition(i);
        }

        if (ins)
        {
            m_trackLabels[i]->getInstrumentLabel()->setText
		(strtoqstr(ins->getPresentationName()));
            if (ins->sendsProgramChange())
            {
                m_trackLabels[i]->setAlternativeLabel(strtoqstr(ins->getProgramName()));
            }

        }
        else
        {
            m_trackLabels[i]->getInstrumentLabel()->setText(i18n("<no instrument>"));
        }

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
        if (m_muteLeds[i]->state() == KLed::Off)
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
    RG_DEBUG << "TrackButtons::slotToggleMutedTrack(" << mutedTrack << ")\n";

    if (mutedTrack < 0 || mutedTrack > (int)m_tracks )
        return;

    Rosegarden::Track *track = 
        m_doc->getComposition().getTrackByPosition(mutedTrack);

    RG_DEBUG << "TrackButtons::slotToggleMutedTrack - track = " << mutedTrack
             << " is " << !track->isMuted() << endl;

    emit muteButton(track->getId(), !track->isMuted()); // will set the value
}

// Remove buttons from all iterators and delete object
//
void
TrackButtons::removeButtons(unsigned int position)
{
    RG_DEBUG << "TrackButtons::removeButtons - "
             << "deleting track button at position "
             << position << endl;

    if (position >= m_trackHBoxes.size()) {
        RG_DEBUG << "%%%%%%%%% BIG PROBLEM : TrackButtons::removeButtons() was passed a non-existing index\n";
        return;
    }
        
    std::vector<TrackLabel*>::iterator tit = m_trackLabels.begin();
    tit += position;
    m_trackLabels.erase(tit);

    std::vector<TrackVUMeter*>::iterator vit = m_trackMeters.begin();
    vit += position;
    m_trackMeters.erase(vit);
    
    std::vector<KLedButton*>::iterator mit = m_muteLeds.begin();
    mit += position;
    m_muteLeds.erase(mit);

    mit = m_recordLeds.begin();
    mit += position;
    m_recordLeds.erase(mit);

    delete m_trackHBoxes[position]; // deletes all child widgets (button, led, label...)

    std::vector<QFrame*>::iterator it = m_trackHBoxes.begin();
    it += position;
    m_trackHBoxes.erase(it);

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

    if (newNbTracks < m_tracks)
    {
        for (unsigned int i = m_tracks; i > newNbTracks; --i)
            removeButtons(i - 1);
    }
    else if (newNbTracks > m_tracks)
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
                RG_DEBUG << "TrackButtons::slotUpdateTracks - "
                         << "can't find TrackId for position " << i << endl;
        }
    }

    // Renumber all the labels
    //
    for (unsigned int i = 0; i < m_trackLabels.size(); ++i)
    {
	track = comp.getTrackByPosition(i);

	if (track) {
	    m_trackLabels[i]->setId(track->getId());
	    
	    QLabel *trackLabel = m_trackLabels[i]->getTrackLabel();
	    
	    if (track->getLabel() == std::string("")) {
		Rosegarden::Instrument *ins =
		    m_doc->getStudio().getInstrumentById(track->getInstrument());
		if (ins && ins->getType() == Rosegarden::Instrument::Audio) {
		    trackLabel->setText(i18n("<untitled audio>"));
		} else {
		    trackLabel->setText(i18n("<untitled>"));
		}
	    }
	    else
		trackLabel->setText(strtoqstr(track->getLabel()));
	    
	    setButtonMapping(m_trackLabels[i], track->getId());
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

    KLedButton* led = m_recordLeds[position];
    
    led->on();

    if (m_lastID != position && m_lastID != -1) {
        m_recordLeds[m_lastID]->off();
    }

    m_lastID = position;
}

void
TrackButtons::selectLabel(int position)
{
    if (m_lastSelected >= 0)
        m_trackLabels[m_lastSelected]->setSelected(false);
    m_trackLabels[position]->setSelected(true);

    m_lastSelected = position;
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
TrackButtons::slotRenameTrack(QString newName, Rosegarden::TrackId trackId)
{
    m_doc->getCommandHistory()->addCommand
	(new RenameTrackCommand(&m_doc->getComposition(),
				trackId,
				qstrtostr(newName)));

    changeTrackLabel(trackId, newName);
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
    RG_DEBUG << "TrackButtons::slotInstrumentSelection(" << trackId << ")\n";

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();

    int position = comp.getTrackById(trackId)->getPosition();

    QString instrumentName = i18n("<no instrument>");
    Rosegarden::Track *track = comp.getTrackByPosition(position);

    Rosegarden::Instrument *instrument = 0;
    if (track != 0) {
	instrument = studio.getInstrumentById(track->getInstrument());
	if (instrument)
	    instrumentName = strtoqstr(instrument->getPresentationName());
    }

    //
    // populate this instrument widget
    m_trackLabels[position]->getInstrumentLabel()->setText(instrumentName);

    // Ensure the instrument name is shown
    m_trackLabels[position]->showLabel(TrackLabel::ShowInstrument);
    
    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    //

    QPopupMenu instrumentPopup(this);

    populateInstrumentPopup(instrument, &instrumentPopup);

    // Store the popup item position
    //
    m_popupItem = position;

    instrumentPopup.exec(QCursor::pos());

    // Restore the label back to what it was showing
    m_trackLabels[position]->showLabel(m_trackInstrumentLabels);
}

void
TrackButtons::populateInstrumentPopup(Rosegarden::Instrument *thisTrackInstr, QPopupMenu* instrumentPopup)
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
    instrumentPopup->clear();

    std::vector<QPopupMenu*> instrumentSubMenus;

    // position index
    int i = 0;

    // Get the list
    Rosegarden::InstrumentList list = studio.getPresentationInstruments();
    Rosegarden::InstrumentList::iterator it;
    int currentDevId = -1;
    bool deviceUsedByAnyone = false;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it)) continue; // sanity check

	QString iname(strtoqstr((*it)->getPresentationName()));
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

	    QPopupMenu *subMenu = new QPopupMenu(instrumentPopup);
	    QString deviceName = strtoqstr(device->getName());
	    instrumentPopup->insertItem(iconSet, deviceName, subMenu);
	    instrumentSubMenus.push_back(subMenu);
	    
	    // Connect up the submenu
	    //
	    connect(subMenu, SIGNAL(activated(int)),
		    SLOT(slotInstrumentPopupActivated(int)));
	    
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

	instrumentSubMenus[instrumentSubMenus.size() - 1]->insertItem(iconSet, iname, i++);
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

            m_trackLabels[m_popupItem]->getInstrumentLabel()->
		setText(strtoqstr(inst->getPresentationName()));

            // reset the alternative label
            m_trackLabels[m_popupItem]->clearAlternativeLabel();

            // Now see if the program is being shown for this instrument
            // and if so reset the label
            //
            if (inst->sendsProgramChange())
                m_trackLabels[m_popupItem]->setAlternativeLabel(strtoqstr(inst->getProgramName()));

            // Ensure that we set a record track properly
            //
            if (track->getId() == comp.getRecordTrack())
                slotSetRecordTrack(track->getId());

        }
        else
            RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";
    }
    else
        RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";

}

// Hide and show Tracks and Instruments
//
void
TrackButtons::changeTrackInstrumentLabels(TrackLabel::InstrumentTrackLabels label)
{
    // Set new label
    m_trackInstrumentLabels = label;

    // update and reconnect with new value
    for (int i = 0; i < (int)m_tracks; i++) {
        m_trackLabels[i]->showLabel(label);
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
            m_trackLabels[i]->setAlternativeLabel(label);
    }
}


void
TrackButtons::changeTrackLabel(Rosegarden::TrackId id, QString label)
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *track;

    for (int i = 0; i < (int)m_tracks; i++)
    {
        track = comp.getTrackByPosition(i);
	if (track && track->getId() == id) {
	    if (m_trackLabels[i]->getTrackLabel()->text() != label) {
		m_trackLabels[i]->getTrackLabel()->setText(label);
		emit widthChanged();
		emit nameChanged();
	    }
	    return;
	}
    }
}

void
TrackButtons::slotSynchroniseWithComposition()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::Track *track;

    for (int i = 0; i < (int)m_tracks; i++)
    {
        track = comp.getTrackByPosition(i);

        if (track)
        {
            if (track->isMuted())
                m_muteLeds[i]->on();
            else
                m_muteLeds[i]->off();

            Rosegarden::Instrument *ins = studio.
                getInstrumentById(track->getInstrument());

	    QString instrumentName(i18n("<no instrument>"));
	    if (ins) instrumentName = strtoqstr(ins->getPresentationName());

            m_trackLabels[i]->getInstrumentLabel()->setText(instrumentName);
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

    m_muteLeds[trackObj->getPosition()]->setState(value ? KLed::Off : KLed::On);
}




