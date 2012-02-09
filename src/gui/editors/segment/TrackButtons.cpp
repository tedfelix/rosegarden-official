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


// Constants
const int TrackButtons::buttonGap = 8;
const int TrackButtons::vuWidth = 20;
const int TrackButtons::vuSpacing = 2;


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
            this, SLOT(slotInstrumentMenu(int)));

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

// New routine intended to standardize updating the UI from the Track.
void
TrackButtons::updateUI(Track *track)
{
    if (!track)
        return;

    unsigned pos = track->getPosition();

    if (pos >= m_tracks)
        return;


    // *** Mute LED

    if (track->isMuted()) {
        m_muteLeds[pos]->off();
    } else {
        m_muteLeds[pos]->on();
    }


    // *** Record LED

    Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());
    m_recordLeds[pos]->setColor(getRecordLedColour(ins));

    // Note: setRecordTrack() used to be used to do this.  But that would
    //       set the track in the composition to record as well as setting
    //       the button on the UI.  This seems better and works fine.
    bool recording =
        m_doc->getComposition().isTrackRecording(track->getId());
    setRecordButton(pos, recording);


    // *** Track Label

    TrackLabel *label = m_trackLabels[pos];

    if (!label)
        return;

    // My guess is that we shouldn't set the position and ID in this routine.
    // They should be set up by the creator of the TrackLabel object.  But is
    // there a possibility that these will change?  Yes.  When tracks are
    // shuffled due to add/delete.  That's probably a special case that
    // shouldn't be handled here.
    //label->setId(track->getId());
    //setButtonMapping(label, track->getId());
    //label->setPosition(pos);

    if (track->getLabel() == std::string("")) {
        if (ins && ins->getType() == Instrument::Audio) {
            label->setTrackName(tr("<untitled audio>"));
        } else {
            label->setTrackName(tr("<untitled>"));
        }
    } else {
        label->setTrackName(track->getLabel().c_str());
    }

    initInstrumentLabel(ins, label);

    label->updateLabel();
}

void
TrackButtons::makeButtons()
{
    if (!m_doc)
        return;

    // Create a horizontal box for each track

    for (unsigned int i = 0; i < m_tracks; ++i) {
        Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (track) {
            QFrame *trackHBox = makeButton(track);

            if (trackHBox) {
                trackHBox->setObjectName("TrackButtonFrame");
                m_layout->addWidget(trackHBox);
                m_trackHBoxes.push_back(trackHBox);
            }
        }
    }

    populateButtons();
}

void
TrackButtons::setButtonMapping(QObject* obj, TrackId trackId)
{
    m_clickedSigMapper->setMapping(obj, trackId);
    m_instListSigMapper->setMapping(obj, trackId);
}

void
TrackButtons::initInstrumentLabel(Instrument *ins, TrackLabel *label)
{
    // Initializes a label's instrument name(s) from a track

    if (!label)
        return;

    if (ins) {
        label->setPresentationName(ins->getLocalizedPresentationName());

        if (ins->sendsProgramChange()) {
            label->setProgramChangeName(
                    QObject::tr(ins->getProgramName().c_str()));
        } else {
            label->setProgramChangeName("");
        }
    } else {
        label->setPresentationName(tr("<no instrument>"));
    }

    // All callers take care of this on their own.  Not needed.
    //label->updateLabel();
}

void
TrackButtons::populateButtons()
{
    //RG_DEBUG << "TrackButtons::populateButtons()";

    // For each track, copy info from Track object to the widgets
    for (unsigned int i = 0; i < m_tracks; ++i) {
        Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (!track)
            continue;

        // Configure the track label's ID and position.
        m_trackLabels[i]->setId(track->getId());
        setButtonMapping(m_trackLabels[i], track->getId());
        m_trackLabels[i]->setPosition(i);

        updateUI(track);

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
    //RG_DEBUG << "TrackButtons::slotToggleMutedTrack( position =" << mutedTrackPos << ")";

    if (mutedTrackPos < 0 || mutedTrackPos >= (int)m_tracks)
        return ;

    Track *track =
        m_doc->getComposition().getTrackByPosition(mutedTrackPos);

    emit muteButton(track->getId(), !track->isMuted()); // will set the value
}

void
TrackButtons::removeButtons(unsigned int position)
{
    //RG_DEBUG << "TrackButtons::removeButtons - deleting track button at position " << position;

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

    //RG_DEBUG << "TrackButtons::slotUpdateTracks > newNbTracks = " << newNbTracks;

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
                QFrame *trackHBox = makeButton(track);

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

    // For each track
    for (unsigned int i = 0; i < m_tracks; ++i) {

        track = comp.getTrackByPosition(i);

        if (!track)
            continue;


        // *** Set Track Size ***

        // Track height can change when the user moves segments around and
        // they overlap.

        m_trackHBoxes[i]->setMinimumSize(labelWidth(), trackHeight(track->getId()));
        m_trackHBoxes[i]->setFixedHeight(trackHeight(track->getId()));

    }

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
    // Copy this potentially new state to the UI.
    setRecordButton(position, state);
    // Copy this potentially new state to the Track.
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
TrackButtons::slotSetTrackMeter(float value, unsigned position)
{
    if (position >= m_tracks)
        return;

    m_trackMeters[position]->setLevel(value);
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
TrackButtons::slotInstrumentMenu(int trackId)
{
    //RG_DEBUG << "TrackButtons::slotInstrumentMenu( trackId =" << trackId << ")";


    // *** Force The Track Label To Show The Presentation Name ***

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

    // Force the track label to show the "presentation name".
    // E.g. "General MIDI Device  #1"
    m_trackLabels[position]->forcePresentationName(true);
    m_trackLabels[position]->updateLabel();


    // *** Launch The Popup ***

    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    QMenu instrumentPopup(this);

    populateInstrumentPopup(instrument, &instrumentPopup);

    // Store the popup item position
    //
    m_popupItem = position;

    instrumentPopup.exec(QCursor::pos());


    // *** Restore The Track Label ***

    // Turn off the presentation name
    m_trackLabels[position]->forcePresentationName(false);
    m_trackLabels[position]->updateLabel();
}

// ??? Break this stuff off into an InstrumentPopup class.  This class is too
//     big.
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

    // For each instrument
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
            connect(subMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotInstrumentSelected(QAction*)));

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
TrackButtons::slotInstrumentSelected(QAction* act)
{
    slotInstrumentSelected(act->data().toInt());
}

void
TrackButtons::slotInstrumentSelected(int item)
{
    // This is called when the user selects a new instrument via the popup.

    //RG_DEBUG << "TrackButtons::slotInstrumentSelected item =" << item;

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    Instrument *inst = studio.getInstrumentFromList(item);

    // debug dump
//    for (int n = 0; n < 100; n++) {
//        inst = studio.getInstrumentFromList(n);
//        RG_DEBUG << "Studio returned instrument \"" << inst->getPresentationName() << "\" for index " << n;
//    }
//    inst = studio.getInstrumentFromList(item);

    //RG_DEBUG << "TrackButtons::slotInstrumentSelected: instrument " << inst;

    if (inst != 0) {
        Track *track = comp.getTrackByPosition(m_popupItem);

        if (track != 0) {
            // select instrument
            track->setInstrument(inst->getId());
            emit instrumentSelected((int)inst->getId());

            // Set the labels
            initInstrumentLabel(inst, m_trackLabels[m_popupItem]);
            m_trackLabels[m_popupItem]->updateLabel();

            m_recordLeds[m_popupItem]->setColor(getRecordLedColour(inst));

        } else {
            RG_DEBUG << "slotInstrumentSelected() - can't find track!\n";
        }

    } else {
        RG_DEBUG << "slotInstrumentSelected() - can't find item!\n";
    }

}

void
TrackButtons::changeTrackInstrumentLabels(TrackLabel::DisplayMode mode)
{
    // Set new label
    m_trackInstrumentLabels = mode;

    // update and reconnect with new value
    for (int i = 0; i < (int)m_tracks; i++) {
        m_trackLabels[i]->setDisplayMode(mode);
        m_trackLabels[i]->updateLabel();
    }
}

void
TrackButtons::changeInstrumentLabel(InstrumentId id, QString programChangeName)
{
    //RG_DEBUG << "TrackButtons::changeInstrumentLabel( id =" << id << ", programChangeName = " << programChangeName << ")";

    Composition &comp = m_doc->getComposition();
    Track *track;

    // for each track, search for the one with this instrument id
    // This is essentially a Composition::getTrackByInstrumentId().
    for (int i = 0; i < (int)m_tracks; i++) {
        track = comp.getTrackByPosition(i);

        if (track && track->getInstrument() == id) {

            // Set the program change name.
            m_trackLabels[i]->setProgramChangeName(programChangeName);
            m_trackLabels[i]->updateLabel();

        }
    }
}

void
TrackButtons::changeTrackLabel(TrackId id, QString name)
{
    Track *track = m_doc->getComposition().getTrackById(id);

    if (track) {
        unsigned pos = track->getPosition();
        TrackLabel *label = m_trackLabels[pos];

        // If the name is actually changing
        if (label->getTrackName() != name) {
            label->setTrackName(name);
            label->updateLabel();
            emit widthChanged();
            emit nameChanged();
        }
    }
}

void
TrackButtons::slotSynchroniseWithComposition()
{
    //RG_DEBUG << "TrackButtons::slotSynchroniseWithComposition()";

    Composition &comp = m_doc->getComposition();

    for (int i = 0; i < (int)m_tracks; i++) {
        updateUI(comp.getTrackByPosition(i));
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

    //RG_DEBUG << "TrackButtons::setMuteButton() trackId = " << track << ", pos = " << pos;

    m_muteLeds[pos]->setState(value ? Led::Off : Led::On);
}

void
TrackButtons::slotTPBInstrumentSelected(TrackId trackId, int item)
{
    //RG_DEBUG << "TrackButtons::slotTPBInstrumentSelected( trackId =" << trackId << ", item =" << item << ")";

    Composition &comp = m_doc->getComposition();
    int position = comp.getTrackById(trackId)->getPosition();
    m_popupItem = position;
    slotInstrumentSelected(item);
}

int
TrackButtons::labelWidth()
{
    return m_trackLabelWidth -
           ((m_cellSize - buttonGap) * 2 + vuSpacing * 2 + vuWidth);
}

int
TrackButtons::trackHeight(TrackId trackId)
{
    int multiple = m_doc->
            getComposition().getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple == 0) multiple = 1;

    return m_cellSize * multiple - m_borderGap;
}

QFrame*
TrackButtons::makeButton(Track *track)
{
    if (track == 0) return 0;

    TrackId trackId = track->getId();


    // *** Horizontal Box ***

    QFrame *trackHBox = new QFrame(this);
    QHBoxLayout *hblayout = new QHBoxLayout(trackHBox);
    trackHBox->setLayout(hblayout);
    hblayout->setMargin(0);
    hblayout->setSpacing(0);

    trackHBox->setMinimumSize(labelWidth(), trackHeight(trackId));
    trackHBox->setFixedHeight(trackHeight(trackId));

    // Try a style for the box
    trackHBox->setFrameStyle(StyledPanel);
    trackHBox->setFrameShape(StyledPanel);
    trackHBox->setFrameShadow(Raised);

    // Insert a little gap
    hblayout->addSpacing(vuSpacing);


    // *** VU Meter ***

    TrackVUMeter *vuMeter = new TrackVUMeter(trackHBox,
                                             VUMeter::PeakHold,
                                             vuWidth,
                                             buttonGap,
                                             track->getPosition());

    m_trackMeters.push_back(vuMeter);

    hblayout->addWidget(vuMeter);

    // Insert a little gap
    hblayout->addSpacing(vuSpacing);


    // *** Mute LED ***

    LedButton *mute = new LedButton(
            GUIPalette::getColour(GUIPalette::MuteTrackLED), trackHBox);
    mute->setToolTip(tr("Mute track"));
    hblayout->addWidget(mute);

    if (track->isMuted()) mute->off();

    connect(mute, SIGNAL(stateChanged(bool)),
            m_muteSigMapper, SLOT(map()));
    m_muteSigMapper->setMapping(mute, track->getPosition());

    m_muteLeds.push_back(mute);

    mute->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);


    // *** Record LED ***

    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    LedButton *record = new LedButton(getRecordLedColour(ins), trackHBox);
    record->setToolTip(tr("Record on this track"));
    hblayout->addWidget(record);

    record->off();

    connect(record, SIGNAL(stateChanged(bool)),
            m_recordSigMapper, SLOT(map()));
    m_recordSigMapper->setMapping(record, track->getPosition());

    m_recordLeds.push_back(record);

    record->setFixedSize(m_cellSize - buttonGap, m_cellSize - buttonGap);


    // *** Track Label ***

    TrackLabel *trackLabel =
            new TrackLabel(trackId, track->getPosition(), trackHBox);
    hblayout->addWidget(trackLabel);

    hblayout->addSpacing(vuSpacing);

    trackLabel->setDisplayMode(m_trackInstrumentLabels);

    trackLabel->setFixedSize(labelWidth(), m_cellSize - buttonGap);
    trackLabel->setFixedHeight(m_cellSize - buttonGap);
    trackLabel->setIndent(7);

    connect(trackLabel, SIGNAL(renameTrack(QString, TrackId)),
            SLOT(slotRenameTrack(QString, TrackId)));

    m_trackLabels.push_back(trackLabel);

    // Connect it
    setButtonMapping(trackLabel, trackId);

    connect(trackLabel, SIGNAL(changeToInstrumentList()),
            m_instListSigMapper, SLOT(map()));
    connect(trackLabel, SIGNAL(clicked()),
            m_clickedSigMapper, SLOT(map()));


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
        RG_DEBUG << "TrackButtons::slotUpdateTracks() - invalid instrument type, this is probably a BUG!";
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
