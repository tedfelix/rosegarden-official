/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackButtons.h"

#include "TrackLabel.h"
#include "TrackVUMeter.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Track.h"

#include "commands/segment/RenameTrackCommand.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/widgets/LedButton.h"
#include "sound/AudioFileManager.h"
#include "sound/PluginIdentifier.h"
#include "sequencer/RosegardenSequencer.h"

#include <QLayout>
#include <QMessageBox>
#include <QCursor>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QMenu>
#include <QSignalMapper>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QStackedWidget>
#include <QToolTip>


namespace Rosegarden
{


TrackButtons::TrackButtons(RosegardenDocument* doc,
                           unsigned int trackCellHeight,
                           unsigned int trackLabelWidth,
                           bool showTrackLabels,
                           int overallHeight,
                           QWidget* parent)
        : QFrame(parent), 
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
        m_trackLabelWidth(trackLabelWidth),
        m_popupItem(0),
        m_lastSelected(-1)
{
    m_layout->setMargin(0);
    setFrameStyle(Plain);

    // when we create the widget, what are we looking at?
    if (showTrackLabels) {
        m_trackInstrumentLabels = TrackLabel::ShowTrack;
    } else {
        m_trackInstrumentLabels = TrackLabel::ShowInstrument;
    }

    // Set the spacing between vertical elements
    //
    m_layout->setSpacing(m_borderGap);

    // Now draw the buttons and labels and meters
    //
    makeButtons();

    m_layout->addStretch(20);

    connect(m_recordSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleRecordTrack(int)));

    connect(m_muteSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleMutedTrack(int)));

    // connect signal mappers
    connect(m_instListSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotInstrumentSelection(int)));

    connect(m_clickedSigMapper, SIGNAL(mapped(int)),
            this, SIGNAL(trackSelected(int)));

    // We have to force the height for the moment
    //
    setMinimumHeight(overallHeight);

    m_doc->getComposition().addObserver(this);
}

TrackButtons::~TrackButtons() {
    // CRASH!  Probably m_doc is gone...
    // Probably don't need to disconnect as we only go away when the
    // doc and composition do.  shared_ptr would help here.
//    m_doc->getComposition().removeObserver(this);
}

void
TrackButtons::makeButtons()
{
    if (!m_doc)
        return ;

    // Create a horizontal box for each track
    // plus the two buttons
    //
    unsigned int nbTracks = m_doc->getComposition().getNbTracks();

    for (unsigned int i = 0; i < nbTracks; ++i) {
        Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (track) {
            QFrame *trackHBox = makeButton(track->getId());
            trackHBox->setObjectName("TrackButtonFrame");

            if (trackHBox) {
                m_layout->addWidget(trackHBox);
                m_trackHBoxes.push_back(trackHBox);
            }
        }
    }

    populateButtons();
}

void TrackButtons::setButtonMapping(QObject* obj, TrackId trackId)
{
    m_clickedSigMapper->setMapping(obj, trackId);
    m_instListSigMapper->setMapping(obj, trackId);
}

void
TrackButtons::populateButtons()
{
    Instrument *ins = 0;
    Track *track;

    for (unsigned int i = 0; i < (unsigned int)m_trackLabels.size(); ++i) {
        track = m_doc->getComposition().getTrackByPosition(i);

        if (track) {
            ins = m_doc->getStudio().getInstrumentById(track->getInstrument());

            // Set mute button from track
            //
            if (track->isMuted()) {
                m_muteLeds[i]->off();
            } else {
                m_muteLeds[i]->on();
            }

            // Set record button from track
            //
            bool recording =
                m_doc->getComposition().isTrackRecording(track->getId());
            setRecordTrack(track->getPosition(), recording);

            // reset track tokens
            m_trackLabels[i]->setId(track->getId());
            setButtonMapping(m_trackLabels[i], track->getId());
            m_trackLabels[i]->setPosition(i);
        }

        if (ins) {
            m_trackLabels[i]->getInstrumentLabel()->setText(ins->getLocalizedPresentationName());
            if (ins->sendsProgramChange()) {
                m_trackLabels[i]->setAlternativeLabel(QObject::tr(ins->getProgramName().c_str()));
            }

        } else {
            m_trackLabels[i]->getInstrumentLabel()->setText(tr("<no instrument>"));
        }

        m_trackLabels[i]->update();
    }

}

std::vector<int>
TrackButtons::mutedTracks()
{
    std::vector<int> mutedTracks;

    for (TrackId i = 0; i < m_tracks; i++) {
        if (m_muteLeds[i]->state() == Led::Off)
            mutedTracks.push_back(i);
    }

    return mutedTracks;
}

void
TrackButtons::slotToggleMutedTrack(int mutedTrackPos)
{
    RG_DEBUG << "TrackButtons::slotToggleMutedTrack(" << mutedTrackPos << ")\n";

    if (mutedTrackPos < 0 || mutedTrackPos > (int)m_tracks)
        return ;

    Track *track =
        m_doc->getComposition().getTrackByPosition(mutedTrackPos);

    emit muteButton(track->getId(), !track->isMuted()); // will set the value
}

void
TrackButtons::removeButtons(unsigned int position)
{
    RG_DEBUG << "TrackButtons::removeButtons - "
    << "deleting track button at position "
    << position << endl;

    if (position >= (unsigned int)m_trackHBoxes.size()) {
        RG_DEBUG << "%%%%%%%%% BIG PROBLEM : TrackButtons::removeButtons() was passed a non-existing index\n";
        return ;
    }

    std::vector<TrackLabel*>::iterator tit = m_trackLabels.begin();
    tit += position;
    m_trackLabels.erase(tit);

    std::vector<TrackVUMeter*>::iterator vit = m_trackMeters.begin();
    vit += position;
    m_trackMeters.erase(vit);

    std::vector<LedButton*>::iterator mit = m_muteLeds.begin();
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
#if 0
    RG_DEBUG << "TrackButtons::slotUpdateTracks()";
    static QTime t;
    RG_DEBUG << "  elapsed: " << t.restart();
#endif

    Composition &comp = m_doc->getComposition();
    const unsigned int newNbTracks = comp.getNbTracks();
    Track *track = 0;

    //RG_DEBUG << "TrackButtons::slotUpdateTracks > newNbTracks = " << newNbTracks << endl;

    // If a track or tracks were deleted
    if (newNbTracks < m_tracks) {
        // For each deleted track, remove a button from the end.
        for (unsigned int i = m_tracks; i > newNbTracks; --i)
            removeButtons(i - 1);
    } else if (newNbTracks > m_tracks) {  // if added
        // For each added track
        for (unsigned int i = m_tracks; i < newNbTracks; ++i) {
            track = m_doc->getComposition().getTrackByPosition(i);
            if (track) {
                // Make a new button
                QFrame *trackHBox = makeButton(track->getId());

                if (trackHBox) {
                    trackHBox->show();
                    // Add the new button to the layout.
                    m_layout->insertWidget(i, trackHBox);
                    m_trackHBoxes.push_back(trackHBox);
                }
            } else
                RG_DEBUG << "TrackButtons::slotUpdateTracks - can't find TrackId for position " << i << endl;
        }
    }

    m_tracks = newNbTracks;

    if (m_tracks != m_trackHBoxes.size())
    	RG_DEBUG << "WARNING  TrackButtons::slotUpdateTracks(): m_trackHBoxes.size() != m_tracks";
    if (m_tracks != m_trackLabels.size())
    	RG_DEBUG << "WARNING  TrackButtons::slotUpdateTracks(): m_trackLabels.size() != m_tracks";

    // Set size
    //
    for (unsigned int i = 0; i < m_tracks; ++i) {

        track = comp.getTrackByPosition(i);

        if (track) {
            
            int multiple = m_doc->
                    getComposition().getMaxContemporaneousSegmentsOnTrack(
                            track->getId());

            if (multiple == 0) multiple = 1;

            const int trackHeight = m_cellSize * multiple - m_borderGap;

            // nasty dupe from makeButton

            const int buttonGap = 8;
            const int vuWidth = 20;
            const int vuSpacing = 2;

            const int labelWidth = m_trackLabelWidth -
                ((m_cellSize - buttonGap) * 2 +
                 vuSpacing * 2 + vuWidth);

            m_trackHBoxes[i]->setMinimumSize(labelWidth, trackHeight);

            m_trackHBoxes[i]->setFixedHeight(trackHeight);
        }
    }

    // Renumber all the labels
    //
    for (unsigned int i = 0; i < m_tracks; ++i) {
        track = comp.getTrackByPosition(i);

        if (track) {
            m_trackLabels[i]->setId(track->getId());

            QLabel *trackLabel = m_trackLabels[i]->getTrackLabel();

            if (track->getLabel() == std::string("")) {
                Instrument *ins =
                    m_doc->getStudio().getInstrumentById(track->getInstrument());
                if (ins && ins->getType() == Instrument::Audio) {
                    trackLabel->setText(tr("<untitled audio>"));
                } else {
                    trackLabel->setText(tr("<untitled>"));
                }
            } else {
                trackLabel->setText(track->getLabel().c_str());
            }

            //             RG_DEBUG << "TrackButtons::slotUpdateTracks - set button mapping at pos "
            //                      << i << " to track id " << track->getId() << endl;
            setButtonMapping(m_trackLabels[i], track->getId());
        }
    }

    // Set record status and colour
    // 
    for (unsigned int i = 0; i < m_tracks; ++i) {

        track = comp.getTrackByPosition(i);

        if (track) {

            setRecordTrack(i, comp.isTrackRecording(track->getId()));

            Instrument *ins =
                m_doc->getStudio().getInstrumentById(track->getInstrument());

            m_recordLeds[i]->setColor(getRecordLedColour(ins));
        }

    }

    // repopulate the buttons
    populateButtons();
    
    // This is necessary to update the widgets's sizeHint to reflect any change in child widget sizes
    adjustSize();
}

void
TrackButtons::slotToggleRecordTrack(int position)
{
    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackByPosition(position);

    bool state = !comp.isTrackRecording(track->getId());

    Instrument *instrument = m_doc->getStudio().getInstrumentById
                             (track->getInstrument());

    bool audio = (instrument &&
                  instrument->getType() == Instrument::Audio);

    if (audio && state) {
        try {
            m_doc->getAudioFileManager().testAudioPath();
        } catch (AudioFileManager::BadAudioPathException e) {
            // ho ho, here was the real culprit: this dialog inherited style
            // from the track button, hence the weird background and black
            // foreground!
            if (QMessageBox::warning(0,
                                     tr("Warning"),
                                     tr("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
                                     QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
              ) == QMessageBox::Yes) {
                RosegardenMainWindow::self()->slotOpenAudioPathSettings();
            }
        }
    }

    // can have any number of audio instruments armed, but only one
    // track armed per instrument.

    // Need to copy this container, as we're implicitly modifying it
    // through calls to comp.setTrackRecording

    Composition::recordtrackcontainer oldRecordTracks =
        comp.getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
            oldRecordTracks.begin();
            i != oldRecordTracks.end(); ++i) {

        if (!comp.isTrackRecording(*i)) {
            // We've already reset this one
            continue;
        }

        Track *otherTrack = comp.getTrackById(*i);

        if (otherTrack &&
                otherTrack != track) {

            if (otherTrack->getInstrument() == track->getInstrument()) {
                // found another record track of the same type (and
                // with the same instrument, if audio): unselect that

                // !!! should we tell the user, particularly for the
                // audio case? might seem odd otherwise

                int otherPos = otherTrack->getPosition();
                setRecordTrack(otherPos, false);
            }
        }
    }

    setRecordTrack(position, state);

    emit recordButton(track->getId(), state);
}

void
TrackButtons::setRecordTrack(int position, bool state)
{
    setRecordButton(position, state);
    m_doc->getComposition().setTrackRecording(
            m_trackLabels[position]->getId(), state);
}

void
TrackButtons::setRecordButton(int position, bool state)
{
    if (position < 0 || position >= (int)m_tracks)
        return ;

    LedButton* led = m_recordLeds[position];

    led->setState(state ? Led::On : Led::Off);
}

void
TrackButtons::selectLabel(int position)
{
    if (m_lastSelected >= 0 && m_lastSelected < (int)m_trackLabels.size()) {
        m_trackLabels[m_lastSelected]->setSelected(false);
    }

    if (position >= 0 && position < (int)m_trackLabels.size()) {
        m_trackLabels[position]->setSelected(true);
        m_lastSelected = position;
    }
}

std::vector<int>
TrackButtons::getHighlightedTracks()
{
    std::vector<int> retList;

    for (unsigned int i = 0; i < (unsigned int)m_trackLabels.size(); ++i) {
        if (m_trackLabels[i]->isSelected())
            retList.push_back(i);
    }

    return retList;
}

void
TrackButtons::slotRenameTrack(QString newName, TrackId trackId)
{
    CommandHistory::getInstance()->addCommand
        (new RenameTrackCommand(&m_doc->getComposition(),
                                trackId,
                                qstrtostr(newName)));

    changeTrackLabel(trackId, newName);
}

void
TrackButtons::slotSetTrackMeter(float value, int position)
{
    //Composition &comp = m_doc->getComposition();
    //Studio &studio = m_doc->getStudio();
    //Track *track;

    // ??? Couldn't we just do this:
    //   m_trackMeters[position]->setLevel(value);
    //     This loop may have been leftover from a version of this using
    //     track ID.
    for (unsigned int i = 0; i < (unsigned int)m_trackMeters.size(); ++i) {
        if (i == ((unsigned int)position)) {
            m_trackMeters[i]->setLevel(value);
            return ;
        }
    }
}

void
TrackButtons::slotSetMetersByInstrument(float value,
                                        InstrumentId id)
{
    Composition &comp = m_doc->getComposition();
    //Studio &studio = m_doc->getStudio();
    Track *track;

    for (unsigned int i = 0; i < (unsigned int)m_trackMeters.size(); ++i) {
        track = comp.getTrackByPosition(i);

        if (track != 0 && track->getInstrument() == id) {
            m_trackMeters[i]->setLevel(value);
        }
    }
}

void
TrackButtons::slotInstrumentSelection(int trackId)
{
    RG_DEBUG << "TrackButtons::slotInstrumentSelection(" << trackId << ")\n";

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    int position = comp.getTrackById(trackId)->getPosition();

    QString instrumentName = tr("<no instrument>");
    Track *track = comp.getTrackByPosition(position);

    Instrument *instrument = 0;
    if (track != 0) {
        instrument = studio.getInstrumentById(track->getInstrument());
        if (instrument)
            instrumentName = instrument->getLocalizedPresentationName();
    }

    //
    // populate this instrument widget
    m_trackLabels[position]->getInstrumentLabel()->setText(instrumentName);

    // Ensure the instrument name is shown
    m_trackLabels[position]->showLabel(TrackLabel::ShowInstrument);

    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    QMenu instrumentPopup(this);

    populateInstrumentPopup(instrument, &instrumentPopup);

    // Store the popup item position
    //
    m_popupItem = position;

    instrumentPopup.exec(QCursor::pos());

    // Restore the label back to what it was showing
    m_trackLabels[position]->showLabel(m_trackInstrumentLabels);

    // Do this here as well as in slotInstrumentPopupActivated, so as
    // to restore the correct alternative label even if no other
    // program was selected from the menu
    if (track != 0) {
        instrument = studio.getInstrumentById(track->getInstrument());
        if (instrument) {
            m_trackLabels[position]->getInstrumentLabel()->
                setText(instrument->getLocalizedPresentationName());
            m_trackLabels[position]->clearAlternativeLabel();
            if (instrument->sendsProgramChange()) {
                m_trackLabels[position]->setAlternativeLabel
                    (QObject::tr(instrument->getProgramName().c_str()));
            }
        }
    }
}

void
TrackButtons::populateInstrumentPopup(Instrument *thisTrackInstr, QMenu* instrumentPopup)
{
    static QPixmap connectedPixmap, unconnectedPixmap,
    connectedUsedPixmap, unconnectedUsedPixmap,
    connectedSelectedPixmap, unconnectedSelectedPixmap;
    static bool havePixmaps = false;
        
    // pixmaps to show connection states as variously colored boxes
    if (!havePixmaps) {

        IconLoader il;
        
        connectedPixmap = il.loadPixmap("connected");
        connectedUsedPixmap = il.loadPixmap("connected-used");
        connectedSelectedPixmap = il.loadPixmap("connected-selected");
        unconnectedPixmap = il.loadPixmap("unconnected");
        unconnectedUsedPixmap = il.loadPixmap("unconnected-used");
        unconnectedSelectedPixmap = il.loadPixmap("unconnected-selected");

        havePixmaps = true;
    }

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    // clear the popup
    instrumentPopup->clear();

    std::vector<QMenu*> instrumentSubMenus;

    // position index
    int count = 0;

    // Get the list
    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;
    int currentDevId = -1;
    bool deviceUsedByAnyone = false;
    QAction* tempMenu = 0;

    for (it = list.begin(); it != list.end(); it++) {

        if (!(*it)) continue; // sanity check

        // get the Localized instrument name, with the string hackery performed
        // in Instrument
        QString iname((*it)->getLocalizedPresentationName());

        // translate the program name
        //
        // Note we are converting the string from std to Q back to std then to
        // C.  This is obviously ridiculous, but the fact that we have pname
        // here at all makes me think it exists as some kind of necessary hack
        // to coax tr() into behaving nicely.  I decided to change it as little
        // as possible to get it to compile, and not refactor this down to the
        // simplest way to call tr() on a C string.
        QString pname(strtoqstr((*it)->getProgramName()));
        pname = QObject::tr(pname.toStdString().c_str());

        Device *device = (*it)->getDevice();
        DeviceId devId = device->getId();
        bool connected = false;

        if ((*it)->getType() == Instrument::SoftSynth) {
            pname = "";
            AudioPluginInstance *plugin = (*it)->getPlugin
                (Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                // we don't translate any plugin program names or other texts
                pname = strtoqstr(plugin->getProgram());
                QString identifier = strtoqstr(plugin->getIdentifier());
                if (identifier != "") {
                    connected = true;
                    QString type, soName, label;
                    PluginIdentifier::parseIdentifier
                        (identifier, type, soName, label);
                    if (pname == "") {
                        pname = strtoqstr(plugin->getDistinctiveConfigurationText());
                    }
                    if (pname != "") {
                        pname = QString("%1: %2").arg(label).arg(pname);
                    } else {
                        pname = label;
                    }
                } else {
                    connected = false;
                }
            }
        } else if ((*it)->getType() == Instrument::Audio) {
            connected = true;
        } else {
            QString conn = RosegardenSequencer::getInstance()->getConnection(devId);
            connected = (conn != "");
        }

        bool instrUsedByMe = false;
        bool instrUsedByAnyone = false;

        if (thisTrackInstr && thisTrackInstr->getId() == (*it)->getId()) {
            instrUsedByMe = true;
            instrUsedByAnyone = true;
        }

        if (devId != (DeviceId)(currentDevId)) {

            deviceUsedByAnyone = false;

            if (instrUsedByMe)
                deviceUsedByAnyone = true;
            else {
                for (Composition::trackcontainer::iterator tit =
                         comp.getTracks().begin();
                     tit != comp.getTracks().end(); ++tit) {

                    if (tit->second->getInstrument() == (*it)->getId()) {
                        instrUsedByAnyone = true;
                        deviceUsedByAnyone = true;
                        break;
                    }

                    Instrument *instr =
                        studio.getInstrumentById(tit->second->getInstrument());
                    if (instr && (instr->getDevice()->getId() == devId)) {
                        deviceUsedByAnyone = true;
                    }
                }
            }

            QIcon iconSet
                (connected ?
                 (deviceUsedByAnyone ?
                  connectedUsedPixmap : connectedPixmap) :
                 (deviceUsedByAnyone ?
                  unconnectedUsedPixmap : unconnectedPixmap));

            currentDevId = int(devId);

            QMenu *subMenu = new QMenu(instrumentPopup);
            QString deviceName = QObject::tr(device->getName().c_str());

            subMenu->setObjectName(deviceName);
            subMenu->setTitle(deviceName);
            subMenu->setIcon(iconSet);
                        
            instrumentPopup->addMenu(subMenu);
            instrumentSubMenus.push_back(subMenu);

            // Connect up the submenu
            connect(subMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotInstrumentPopupActivated(QAction*)));

        } else if (!instrUsedByMe) {

            for (Composition::trackcontainer::iterator tit =
                     comp.getTracks().begin();
                 tit != comp.getTracks().end(); ++tit) {

                if (tit->second->getInstrument() == (*it)->getId()) {
                    instrUsedByAnyone = true;
                    break;
                }
            }
        }

        QIcon iconSet
            (connected ?
             (instrUsedByAnyone ?
              instrUsedByMe ?
              connectedSelectedPixmap :
              connectedUsedPixmap : connectedPixmap) :
             (instrUsedByAnyone ?
              instrUsedByMe ?
              unconnectedSelectedPixmap :
              unconnectedUsedPixmap : unconnectedPixmap));

        if (pname != "") iname += " (" + pname + ")";
            
        tempMenu = new QAction(instrumentPopup);
        tempMenu->setIcon(iconSet);
        tempMenu->setText(iname);    // for QAction
        tempMenu->setData(QVariant(count));
        tempMenu->setObjectName(iname + QString(count));
        
        count++;
        
        instrumentSubMenus[instrumentSubMenus.size() - 1]->addAction(tempMenu);
    }

}

void
TrackButtons::slotInstrumentPopupActivated(QAction* act)
{
    slotInstrumentPopupActivated(act->data().toInt());
}

void
TrackButtons::slotInstrumentPopupActivated(int item)
{
    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated " << item << endl;

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    Instrument *inst = studio.getInstrumentFromList(item);

    // debug dump
//    for (int n = 0; n < 100; n++) {
//        inst = studio.getInstrumentFromList(n);
//        std::cout << "Studio returned instrument \"" << inst->getPresentationName() << "\" for index " << n << std::endl;
//    }
//    inst = studio.getInstrumentFromList(item);

    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated: instrument " << inst << endl;

    if (inst != 0) {
        Track *track = comp.getTrackByPosition(m_popupItem);

        if (track != 0) {
            track->setInstrument(inst->getId());

            // select instrument
            emit instrumentSelected((int)inst->getId());

            m_trackLabels[m_popupItem]->getInstrumentLabel()->
                setText(inst->getLocalizedPresentationName());

            // reset the alternative label
            m_trackLabels[m_popupItem]->clearAlternativeLabel();

            // Now see if the program is being shown for this instrument
            // and if so reset the label
            //
            if (inst->sendsProgramChange())
                m_trackLabels[m_popupItem]->setAlternativeLabel(QObject::tr(inst->getProgramName().c_str()));

            m_recordLeds[m_popupItem]->setColor(getRecordLedColour(inst));

        } else {
            RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";
        }

    } else {
        RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";
    }

}

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

void
TrackButtons::changeInstrumentLabel(InstrumentId id, QString label)
{
    Composition &comp = m_doc->getComposition();
    Track *track;

    for (int i = 0; i < (int)m_tracks; i++) {
        track = comp.getTrackByPosition(i);

        if (track && track->getInstrument() == id) {

            m_trackLabels[i]->setAlternativeLabel(label);

            Instrument *ins = m_doc->getStudio().
                              getInstrumentById(track->getInstrument());

            m_recordLeds[i]->setColor(getRecordLedColour(ins));

        }
    }
}

void
TrackButtons::changeTrackLabel(TrackId id, QString label)
{
    Composition &comp = m_doc->getComposition();
    Track *track;

    // ??? Wouldn't it be better to use Composition::getTrackById()?
    for (int i = 0; i < (int)m_tracks; i++) {
        track = comp.getTrackByPosition(i);
        if (track && track->getId() == id) {
            if (m_trackLabels[i]->getTrackLabel()->text() != label) {
                m_trackLabels[i]->getTrackLabel()->setText(label);
                emit widthChanged();
                emit nameChanged();
            }
            return ;
        }
    }
}

void
TrackButtons::slotSynchroniseWithComposition()
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();
    Track *track;

    for (int i = 0; i < (int)m_tracks; i++) {
        track = comp.getTrackByPosition(i);

        if (track) {
            if (track->isMuted())
                m_muteLeds[i]->off();
            else
                m_muteLeds[i]->on();

            Instrument *ins = studio.
                              getInstrumentById(track->getInstrument());

            QString instrumentName(tr("<no instrument>"));
            if (ins)
                instrumentName = ins->getLocalizedPresentationName();

            m_trackLabels[i]->getInstrumentLabel()->setText(instrumentName);

            setRecordButton(i, comp.isTrackRecording(track->getId()));

            m_recordLeds[i]->setColor(getRecordLedColour(ins));
        }
    }
}

void
TrackButtons::slotLabelSelected(int position)
{
    Track *track =
        m_doc->getComposition().getTrackByPosition(position);

    if (track) {
        emit trackSelected(track->getId());
    }
}

void
TrackButtons::setMuteButton(TrackId track, bool value)
{
    Track *trackObj = m_doc->getComposition().getTrackById(track);
    if (trackObj == 0)
        return ;

    int pos = trackObj->getPosition();

    RG_DEBUG << "TrackButtons::setMuteButton() trackId = "
    << track << ", pos = " << pos << endl;

    m_muteLeds[pos]->setState(value ? Led::Off : Led::On);
}

void
TrackButtons::slotTrackInstrumentSelection(TrackId trackId, int item)
{
    RG_DEBUG << "TrackButtons::slotTrackInstrumentSelection(" << trackId << ")\n";

    Composition &comp = m_doc->getComposition();
    int position = comp.getTrackById(trackId)->getPosition();
    m_popupItem = position;
    slotInstrumentPopupActivated(item);
}

QFrame* TrackButtons::makeButton(Rosegarden::TrackId trackId)
{
    // The buttonGap sets up the sizes of the buttons
    //
    static const int buttonGap = 8;

    QFrame *trackHBox = 0;

    LedButton *mute = 0;
    LedButton *record = 0;

    TrackVUMeter *vuMeter = 0;
    TrackLabel *trackLabel = 0;

    int vuWidth = 20;
    int vuSpacing = 2;
    int multiple = m_doc->getComposition()
        .getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple == 0) multiple = 1;
    int labelWidth = m_trackLabelWidth - ((m_cellSize - buttonGap) * 2 +
                                           vuSpacing * 2 + vuWidth);

    // Set the label from the Track object on the Composition
    //
    Rosegarden::Track *track = m_doc->getComposition().getTrackById(trackId);

    if (track == 0) return 0;

    // Create a horizontal box for each track
    //
    trackHBox = new QFrame(this);
    QHBoxLayout *hblayout = new QHBoxLayout(trackHBox);
    trackHBox->setLayout(hblayout);
    hblayout->setMargin(0);
    hblayout->setSpacing(0);

    trackHBox->setMinimumSize(labelWidth, m_cellSize * multiple - m_borderGap);
    trackHBox->setFixedHeight(m_cellSize * multiple - m_borderGap);

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
    mute = new LedButton(Rosegarden::GUIPalette::getColour
              (Rosegarden::GUIPalette::MuteTrackLED), trackHBox);
    mute->setToolTip(tr("Mute track"));
    hblayout->addWidget(mute);

    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    record = new LedButton(getRecordLedColour(ins), trackHBox);
    record->setToolTip(tr("Record on this track"));
    hblayout->addWidget(record);

    record->off();

    // Connect them to their sigmappers
    connect(record, SIGNAL(stateChanged(bool)),
            m_recordSigMapper, SLOT(map()));
    connect(mute, SIGNAL(stateChanged(bool)),
            m_muteSigMapper, SLOT(map()));
    m_recordSigMapper->setMapping(record, track->getPosition());
    m_muteSigMapper->setMapping(mute, track->getPosition());

    // Store the LedButton
    //
    m_muteLeds.push_back(mute);
    m_recordLeds.push_back(record);

    //
    // Track label
    //
    trackLabel = new TrackLabel(trackId, track->getPosition(), trackHBox);
    hblayout->addWidget(trackLabel);
    hblayout->addSpacing(vuSpacing);

    if (track->getLabel() == std::string("")) {
        if (ins && ins->getType() == Rosegarden::Instrument::Audio) {
            trackLabel->getTrackLabel()->setText(tr("<untitled audio>"));
        } else {
            trackLabel->getTrackLabel()->setText(tr("<untitled>"));
        }
    } else {
        trackLabel->getTrackLabel()->setText(strtoqstr(track->getLabel()));
    }

    trackLabel->setFixedSize(labelWidth, m_cellSize - buttonGap);
    trackLabel->setFixedHeight(m_cellSize - buttonGap);
    trackLabel->setIndent(7);

    connect(trackLabel, SIGNAL(renameTrack(QString, TrackId)),
            SLOT(slotRenameTrack(QString, TrackId)));

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
    QString instrumentName(tr("<no instrument>"));
    if (ins) instrumentName = ins->getLocalizedPresentationName();

    // Set label to program change if it's being sent
    //
    if (ins != 0 && ins->sendsProgramChange()) {
        trackLabel->setAlternativeLabel(QObject::tr(ins->getProgramName().c_str()));
    }

    trackLabel->showLabel(m_trackInstrumentLabels);

    mute->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);
    record->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);

    // set the mute button
    //
    if (track->isMuted()) mute->off();

    return trackHBox;
}

QColor
TrackButtons::getRecordLedColour(Instrument *ins)
{
    if (!ins) return Qt::white;

    switch (ins->getType()) {

    case Instrument::Audio:
        return GUIPalette::getColour(GUIPalette::RecordAudioTrackLED);

    case Instrument::SoftSynth:
        return GUIPalette::getColour(GUIPalette::RecordSoftSynthTrackLED);

    case Instrument::Midi:
        return GUIPalette::getColour(GUIPalette::RecordMIDITrackLED);
            
    default:
        std::cerr << "TrackButtons::slotUpdateTracks() - invalid instrument type, this is probably a BUG!" 
                  << std::endl;
        return Qt::green;

    }

}

void
TrackButtons::tracksAdded(const Composition *, std::vector<TrackId> &/*trackIds*/)
{
    //RG_DEBUG << "TrackButtons::tracksAdded()";

    slotUpdateTracks();
}

#if 0
// Definitely not ready for primetime.
void
TrackButtons::trackChanged(const Composition *, Track*)
{
    RG_DEBUG << "TrackButtons::trackChanged()";

//    updateTrack(track);
}
#endif

void
TrackButtons::tracksDeleted(const Composition *, std::vector<TrackId> &/*trackIds*/)
{
    //RG_DEBUG << "TrackButtons::tracksDeleted()";

    slotUpdateTracks();
}

}
#include "TrackButtons.moc"
