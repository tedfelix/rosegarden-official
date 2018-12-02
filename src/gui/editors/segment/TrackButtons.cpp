/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TrackButtons]"

#include "TrackButtons.h"

#include "TrackLabel.h"
#include "TrackVUMeter.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Track.h"

#include "commands/segment/RenameTrackCommand.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/widgets/LedButton.h"
#include "sound/AudioFileManager.h"
#include "sound/ControlBlock.h"
#include "sound/PluginIdentifier.h"
#include "sequencer/RosegardenSequencer.h"

#include <QApplication>
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
const int TrackButtons::m_borderGap = 1;
const int TrackButtons::m_buttonGap = 8;
const int TrackButtons::m_vuWidth = 20;
const int TrackButtons::m_vuSpacing = 2;


TrackButtons::TrackButtons(RosegardenDocument* doc,
                           int trackCellHeight,
                           int trackLabelWidth,
                           bool showTrackLabels,
                           int overallHeight,
                           QWidget* parent) :
        QFrame(parent),
        m_doc(doc),
        m_layout(new QVBoxLayout(this)),
        m_recordSigMapper(new QSignalMapper(this)),
        m_muteSigMapper(new QSignalMapper(this)),
        m_soloSigMapper(new QSignalMapper(this)),
        m_clickedSigMapper(new QSignalMapper(this)),
        m_instListSigMapper(new QSignalMapper(this)),
        m_tracks(doc->getComposition().getNbTracks()),
//        m_offset(4),
        m_cellSize(trackCellHeight),
        m_trackLabelWidth(trackLabelWidth),
        m_popupTrackPos(0),
        m_lastSelected(-1)
{
    setFrameStyle(Plain);

    QPalette pal = palette();
    pal.setColor(backgroundRole(), QColor(0xDD, 0xDD, 0xDD));
    pal.setColor(foregroundRole(), Qt::black);
    setPalette(pal);

    // when we create the widget, what are we looking at?
    if (showTrackLabels) {
        m_labelDisplayMode = TrackLabel::ShowTrack;
    } else {
        m_labelDisplayMode = TrackLabel::ShowInstrument;
    }

    m_layout->setMargin(0);
    // Set the spacing between vertical elements
    m_layout->setSpacing(m_borderGap);

    // Now draw the buttons and labels and meters
    //
    makeButtons();

    m_layout->addStretch(20);

    connect(m_recordSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleRecord(int)));

    connect(m_muteSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleMute(int)));

    connect(m_soloSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleSolo(int)));

    // connect signal mappers
    connect(m_instListSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotInstrumentMenu(int)));

    connect(m_clickedSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotTrackSelected(int)));

    // We have to force the height for the moment
    //
    setMinimumHeight(overallHeight);

    m_doc->getComposition().addObserver(this);

    // We do not care about documentChanged() because if the
    // document is changing, we are going away.  A new TrackButtons
    // is created for each new document.
    //connect(RosegardenMainWindow::self(),
    //            SIGNAL(documentChanged(RosegardenDocument *)),
    //        SLOT(slotNewDocument(RosegardenDocument *)));
}

TrackButtons::~TrackButtons() {
    // CRASH!  Probably m_doc is gone...
    // Probably don't need to disconnect as we only go away when the
    // doc and composition do.  shared_ptr would help here.
//    m_doc->getComposition().removeObserver(this);
}

void
TrackButtons::updateUI(Track *track)
{
    if (!track)
        return;

    int pos = track->getPosition();

    if (pos < 0  ||  pos >= m_tracks)
        return;


    // *** Archive Background

    QFrame *hbox = m_trackHBoxes.at(pos);
    if (track->isArchived()) {
        // Go with the dark gray background.
        QPalette palette = hbox->palette();
        palette.setColor(hbox->backgroundRole(), QColor(0x88, 0x88, 0x88));
        hbox->setPalette(palette);
    } else {
        // Go with the parent's background color.
        QColor parentBackground = palette().color(backgroundRole());
        QPalette palette = hbox->palette();
        palette.setColor(hbox->backgroundRole(), parentBackground);
        hbox->setPalette(palette);
    }


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

    // Note: setRecord() used to be used to do this.  But that would
    //       set the track in the composition to record as well as setting
    //       the button on the UI.  This seems better and works fine.
    bool recording =
        m_doc->getComposition().isTrackRecording(track->getId());
    setRecordButton(pos, recording);


    // *** Solo LED

    // ??? An Led::setState(bool) would be handy.
    m_soloLeds[pos]->setState(track->isSolo() ? Led::On : Led::Off);


    // *** Track Label

    TrackLabel *label = m_trackLabels[pos];

    if (!label)
        return;

    // In case the tracks have been moved around, update the mapping.
    label->setId(track->getId());
    setButtonMapping(label, track->getId());
    label->setPosition(pos);

    if (track->getLabel() == "") {
        if (ins && ins->getType() == Instrument::Audio) {
            label->setTrackName(tr("<untitled audio>"));
        } else {
            label->setTrackName(tr("<untitled>"));
        }
    } else {
        label->setTrackName(strtoqstr(track->getLabel()));
        label->setShortName(strtoqstr(track->getShortLabel()));
    }

    initInstrumentNames(ins, label);

    label->updateLabel();
}

void
TrackButtons::makeButtons()
{
    if (!m_doc)
        return;

    //RG_DEBUG << "makeButtons()";

    // Create a horizontal box filled with widgets for each track

    for (int i = 0; i < m_tracks; ++i) {
        Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (!track)
            continue;

        QFrame *trackHBox = makeButton(track);

        if (trackHBox) {
            trackHBox->setObjectName("TrackButtonFrame");
            m_layout->addWidget(trackHBox);
            m_trackHBoxes.push_back(trackHBox);
        }
    }

    populateButtons();
}

void
TrackButtons::setButtonMapping(TrackLabel* trackLabel, TrackId trackId)
{
    m_clickedSigMapper->setMapping(trackLabel, trackId);
    m_instListSigMapper->setMapping(trackLabel, trackId);
}

void
TrackButtons::initInstrumentNames(Instrument *ins, TrackLabel *label)
{
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
}

void
TrackButtons::populateButtons()
{
    //RG_DEBUG << "populateButtons()";

    // For each track, copy info from Track object to the widgets
    for (int i = 0; i < m_tracks; ++i) {
        Track *track = m_doc->getComposition().getTrackByPosition(i);

        if (!track)
            continue;

        updateUI(track);

    }
}

void
TrackButtons::slotToggleMute(int pos)
{
    //RG_DEBUG << "TrackButtons::slotToggleMute( position =" << pos << ")";

    if (!m_doc)
        return;

    if (pos < 0  ||  pos >= m_tracks)
        return;

    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackByPosition(pos);

    if (!track)
        return;

    // Toggle the mute state
    track->setMuted(!track->isMuted());

    // Notify observers
    comp.notifyTrackChanged(track);
    m_doc->slotDocumentModified();
}

void TrackButtons::toggleSolo()
{
    if (!m_doc)
        return;

    Composition &comp = m_doc->getComposition();
    int pos = comp.getTrackPositionById(comp.getSelectedTrack());

    if (pos == -1)
        return;

    slotToggleSolo(pos);
}

void
TrackButtons::slotToggleSolo(int pos)
{
    //RG_DEBUG << "slotToggleSolo( position =" << pos << ")";

    if (!m_doc)
        return;

    if (pos < 0  ||  pos >= m_tracks)
        return;

    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackByPosition(pos);

    if (!track)
        return;

    bool state = !track->isSolo();

    // If we're setting solo on this track and shift isn't being held down,
    // clear solo on all tracks (canceling mode).  If shift is being held
    // down, multiple tracks can be put into solo (latching mode).
    if (state  &&
        QApplication::keyboardModifiers() != Qt::ShiftModifier) {
        // For each track
        for (int i = 0; i < m_tracks; ++i) {
            // Except the one that is being toggled.
            if (i == pos)
                continue;

            Track *track2 = comp.getTrackByPosition(i);

            if (!track2)
                continue;

            if (track2->isSolo()) {
                // Clear solo
                track2->setSolo(false);
                comp.notifyTrackChanged(track2);
            }
        }
    }

    // Toggle the solo state
    track->setSolo(state);

    // Notify observers
    comp.notifyTrackChanged(track);
    m_doc->slotDocumentModified();
}

void
TrackButtons::removeButtons(int position)
{
    //RG_DEBUG << "removeButtons() - deleting track button at position:" << position;

    if (position < 0  ||  position >= m_tracks) {
        RG_DEBUG << "%%%%%%%%% BIG PROBLEM : TrackButtons::removeButtons() was passed a non-existing index\n";
        return;
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

    m_soloLeds.erase(m_soloLeds.begin() + position);

    // Delete all child widgets (button, led, label...)
    delete m_trackHBoxes[position];
    m_trackHBoxes[position] = nullptr;

    std::vector<QFrame*>::iterator it = m_trackHBoxes.begin();
    it += position;
    m_trackHBoxes.erase(it);

}

void
TrackButtons::slotUpdateTracks()
{
    //RG_DEBUG << "slotUpdateTracks()";

#if 0
    static QTime t;
    RG_DEBUG << "  elapsed: " << t.restart();
#endif

    if (!m_doc)
        return;

    Composition &comp = m_doc->getComposition();
    const int newNbTracks = comp.getNbTracks();

    if (newNbTracks < 0) {
        RG_WARNING << "slotUpdateTracks(): WARNING: New number of tracks was negative:" << newNbTracks;
        return;
    }

    //RG_DEBUG << "TrackButtons::slotUpdateTracks > newNbTracks = " << newNbTracks;

    // If a track or tracks were deleted
    if (newNbTracks < m_tracks) {
        // For each deleted track, remove a button from the end.
        for (int i = m_tracks; i > newNbTracks; --i)
            removeButtons(i - 1);
    } else if (newNbTracks > m_tracks) {  // if added
        // For each added track
        for (int i = m_tracks; i < newNbTracks; ++i) {
            Track *track = m_doc->getComposition().getTrackByPosition(i);
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
                RG_DEBUG << "TrackButtons::slotUpdateTracks - can't find TrackId for position " << i;
        }
    }

    m_tracks = newNbTracks;

    if (m_tracks != (int)m_trackHBoxes.size())
        RG_DEBUG << "WARNING  TrackButtons::slotUpdateTracks(): m_trackHBoxes.size() != m_tracks";
    if (m_tracks != (int)m_trackLabels.size())
        RG_DEBUG << "WARNING  TrackButtons::slotUpdateTracks(): m_trackLabels.size() != m_tracks";

    // For each track
    for (int i = 0; i < m_tracks; ++i) {

        Track *track = comp.getTrackByPosition(i);

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
    // Make the TrackButtons QFrame big enough to hold all the track buttons.
    // Some may have grown taller due to segments that overlap.
    // Note: This appears to no longer be needed.  But it doesn't hurt.
    adjustSize();
}

void
TrackButtons::slotToggleRecord(int position)
{
    //RG_DEBUG << "TrackButtons::slotToggleRecord(" << position << ")";

    if (position < 0  ||  position >= m_tracks)
        return;
    if (!m_doc)
        return;

    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackByPosition(position);

    if (!track)
        return;

    // Toggle
    bool state = !comp.isTrackRecording(track->getId());

    // Update the Track
    comp.setTrackRecording(track->getId(), state);
    comp.notifyTrackChanged(track);

    m_doc->checkAudioPath(track);
}

void
TrackButtons::setRecordButton(int position, bool record)
{
    if (position < 0  ||  position >= m_tracks)
        return;

    m_recordLeds[position]->setState(record ? Led::On : Led::Off);
}

void
TrackButtons::selectTrack(int position)
{
    if (position < 0  ||  position >= m_tracks)
        return;

    // No sense doing anything if the selection isn't changing
    if (position == m_lastSelected)
        return;

    // Unselect the previously selected
    if (m_lastSelected >= 0  &&  m_lastSelected < m_tracks) {
        m_trackLabels[m_lastSelected]->setSelected(false);
    }

    // Select the newly selected
    m_trackLabels[position]->setSelected(true);
    m_lastSelected = position;
}

#if 0
// unused
std::vector<int>
TrackButtons::getHighlightedTracks()
{
    std::vector<int> retList;

    for (int i = 0; i < m_trackLabels.size(); ++i) {
        if (m_trackLabels[i]->isSelected())
            retList.push_back(i);
    }

    return retList;
}
#endif

void
TrackButtons::slotRenameTrack(QString longLabel, QString shortLabel, TrackId trackId)
{
    if (!m_doc) return;

    Track *track = m_doc->getComposition().getTrackById(trackId);

    if (!track) return;

    TrackLabel *label = m_trackLabels[track->getPosition()];

    // If neither label is changing, skip it
    if (label->getTrackName() == longLabel &&
        QString::fromStdString(track->getShortLabel()) == shortLabel) return;

    // Rename the track
    CommandHistory::getInstance()->addCommand(
            new RenameTrackCommand(&m_doc->getComposition(),
                                   trackId,
                                   longLabel,
                                   shortLabel));
}

void
TrackButtons::slotSetTrackMeter(float value, int position)
{
    if (position < 0  ||  position >= m_tracks)
        return;

    m_trackMeters[position]->setLevel(value);
}

void
TrackButtons::slotSetMetersByInstrument(float value,
                                        InstrumentId id)
{
    Composition &comp = m_doc->getComposition();

    for (int i = 0; i < m_tracks; ++i) {
        Track *track = comp.getTrackByPosition(i);

        if (track  &&  track->getInstrument() == id) {
            m_trackMeters[i]->setLevel(value);
        }
    }
}

void
TrackButtons::slotInstrumentMenu(int trackId)
{
    //RG_DEBUG << "TrackButtons::slotInstrumentMenu( trackId =" << trackId << ")";

    Composition &comp = m_doc->getComposition();
    const int position = comp.getTrackById(trackId)->getPosition();
    Track *track = comp.getTrackByPosition(position);

    Instrument *instrument = nullptr;

    if (track != nullptr) {
        instrument = m_doc->getStudio().getInstrumentById(
                track->getInstrument());
    }


    // *** Force The Track Label To Show The Presentation Name ***

    // E.g. "General MIDI Device  #1"
    m_trackLabels[position]->forcePresentationName(true);
    m_trackLabels[position]->updateLabel();


    // *** Launch The Popup ***

    // Yes, well as we might've changed the Device name in the
    // Device/Bank dialog then we reload the whole menu here.
    QMenu instrumentPopup(this);
    populateInstrumentPopup(instrument, &instrumentPopup);

    // Store the popup item position for slotInstrumentSelected().
    m_popupTrackPos = position;

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
    // pixmaps for icons to show connection states as variously colored boxes
    // ??? Factor out the icon-related stuff to make this routine clearer.
    //     getIcon(Instrument *) would be ideal, but might not be easy.
    //     getIcon(Device *) would also be needed.
    static QPixmap connectedPixmap, unconnectedPixmap,
                   connectedUsedPixmap, unconnectedUsedPixmap,
                   connectedSelectedPixmap, unconnectedSelectedPixmap;

    static bool havePixmaps = false;
        
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

    // clear the popup
    instrumentPopup->clear();

    QMenu *currentSubMenu = nullptr;

    // position index
    int count = 0;

    int currentDevId = -1;

    // Get the list
    Studio &studio = m_doc->getStudio();
    InstrumentList list = studio.getPresentationInstruments();

    // For each instrument
    for (InstrumentList::iterator it = list.begin(); it != list.end(); ++it) {

        if (!(*it)) continue; // sanity check

        // get the Localized instrument name, with the string hackery performed
        // in Instrument
        QString iname((*it)->getLocalizedPresentationName());

        // translate the program name
        //
        // Note we are converting the string from std to Q back to std then to
        // C.  This is obviously ridiculous, but the fact that we have programName
        // here at all makes me think it exists as some kind of necessary hack
        // to coax tr() into behaving nicely.  I decided to change it as little
        // as possible to get it to compile, and not refactor this down to the
        // simplest way to call tr() on a C string.
        QString programName(strtoqstr((*it)->getProgramName()));
        programName = QObject::tr(programName.toStdString().c_str());

        Device *device = (*it)->getDevice();
        DeviceId devId = device->getId();
        bool connectedIcon = false;

        // Determine the proper program name and whether it is connected

        if ((*it)->getType() == Instrument::SoftSynth) {
            programName = "";
            AudioPluginInstance *plugin =
                    (*it)->getPlugin(Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                // we don't translate any plugin program names or other texts
                programName = strtoqstr(plugin->getDisplayName());
                connectedIcon = (plugin->getIdentifier() != "");
            }
        } else if ((*it)->getType() == Instrument::Audio) {
            connectedIcon = true;
        } else {
            QString conn = RosegardenSequencer::getInstance()->
                    getConnection(devId);
            connectedIcon = (conn != "");
        }

        // These two are for selecting the correct icon to display.
        bool instrUsedByMe = false;
        bool instrUsedByAnyone = false;

        if (thisTrackInstr && thisTrackInstr->getId() == (*it)->getId()) {
            instrUsedByMe = true;
            instrUsedByAnyone = true;
        }

        // If we have switched to a new device, we'll create a new submenu
        if (devId != (DeviceId)(currentDevId)) {

            currentDevId = int(devId);

            // For selecting the correct icon to display.
            bool deviceUsedByAnyone = false;

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

            QIcon icon
                (connectedIcon ?
                 (deviceUsedByAnyone ?
                  connectedUsedPixmap : connectedPixmap) :
                 (deviceUsedByAnyone ?
                  unconnectedUsedPixmap : unconnectedPixmap));

            // Create a submenu for this device
            QMenu *subMenu = new QMenu(instrumentPopup);
            subMenu->setMouseTracking(true);
            subMenu->setIcon(icon);
            // Not needed so long as AA_DontShowIconsInMenus is false.
            //subMenu->menuAction()->setIconVisibleInMenu(true);

            // Menu title
            QString deviceName = QObject::tr(device->getName().c_str());
            subMenu->setTitle(deviceName);

            // QObject name
            subMenu->setObjectName(deviceName);

            // Add the submenu to the popup menu
            instrumentPopup->addMenu(subMenu);

            // Connect the submenu to slotInstrumentSelected()
            connect(subMenu, SIGNAL(triggered(QAction*)),
                    this, SLOT(slotInstrumentSelected(QAction*)));

            currentSubMenu = subMenu;

        } else if (!instrUsedByMe) {

            // Search the tracks to see if anyone else is using this
            // instrument
            for (Composition::trackcontainer::iterator tit =
                     comp.getTracks().begin();
                 tit != comp.getTracks().end(); ++tit) {

                if (tit->second->getInstrument() == (*it)->getId()) {
                    instrUsedByAnyone = true;
                    break;
                }
            }
        }

        QIcon icon
            (connectedIcon ?
             (instrUsedByAnyone ?
              instrUsedByMe ?
              connectedSelectedPixmap :
              connectedUsedPixmap : connectedPixmap) :
             (instrUsedByAnyone ?
              instrUsedByMe ?
              unconnectedSelectedPixmap :
              unconnectedUsedPixmap : unconnectedPixmap));


        // Create an action for this instrument
        QAction* action = new QAction(instrumentPopup);
        action->setIcon(icon);
        // Not needed so long as AA_DontShowIconsInMenus is false.
        //action->setIconVisibleInMenu(true);

        // Action text
        if (programName != "") iname += " (" + programName + ")";
        action->setText(iname);

        // Item index used to find the proper instrument once the user makes
        // a selection from the menu.
        action->setData(QVariant(count));

        // QObject object name.
        action->setObjectName(iname + QString(count));

        // Add the action to the current submenu
        if (currentSubMenu)
            currentSubMenu->addAction(action);

        // Next item index
        count++;
    }
}

void
TrackButtons::slotInstrumentSelected(QAction* action)
{
    // The action data field has the instrument index.
    slotInstrumentSelected(action->data().toInt());
}

void
TrackButtons::selectInstrument(Track *track, Instrument *instrument)
{
    // Inform the rest of the system of the instrument change.

    // ??? This routine needs to go for two reasons:
    //
    //     1. TrackParameterBox calls this.  UI to UI connections should be
    //        avoided.  It would be better to copy/paste this over to TPB
    //        to avoid the connection.  But then we have double-maintenance.
    //        See reason 2.
    //
    //     2. The UI shouldn't know so much about the other objects in the
    //        system.  The following updates should be done by their
    //        respective objects.
    //
    //     A "TrackStaticSignals::instrumentChanged(Track *, Instrument *)"
    //     notification is probably the best way to get rid of this routine.
    //     It could be emitted from Track::setInstrument().  Normally emitting
    //     from setters is bad, but in this case, it is necessary.  We need
    //     to know about every single change when it occurs.
    //     Then ControlBlockSignalHandler (new class to avoid deriving
    //     ControlBlock from QObject), InstrumentSignalHandler (new class to
    //     handle signals for all Instrument instances), and
    //     SequenceManager (already derives from QObject, might want to
    //     consider a new SequenceManagerSignalHandler to avoid additional
    //     dependency on QObject) can connect and do what needs to be done in
    //     response.  Rationale for this over doc modified is that we
    //     can't simply refresh everything (Instrument::sendChannelSetup()
    //     sends out data), and it is expensive to detect what has actually
    //     changed (we would have to cache the Track->Instrument mapping and
    //     check it for changes).

    const TrackId trackId = track->getId();

    // *** ControlBlock

    ControlBlock::getInstance()->
            setInstrumentForTrack(trackId, instrument->getId());

    // *** Send out BS/PC

    // Make sure the Device is in sync with the Instrument's settings.
    instrument->sendChannelSetup();

    // *** SequenceManager

    // In case the sequencer is currently playing, we need to regenerate
    // all the events with the new channel number.

    Composition &comp = m_doc->getComposition();
    SequenceManager *sequenceManager = m_doc->getSequenceManager();

    // For each segment in the composition
    for (Composition::iterator i = comp.begin();
         i != comp.end();
         ++i) {

        Segment *segment = (*i);

        // If this Segment is on this Track, let SequenceManager know
        // that the Instrument has changed.
        // Segments on this track are now playing on a new
        // instrument, so they're no longer ready (making them
        // ready is done just-in-time elsewhere), nor is thru
        // channel ready.
        if (segment->getTrack() == trackId)
            sequenceManager->segmentInstrumentChanged(segment);

    }
}

void
TrackButtons::slotInstrumentSelected(int instrumentIndex)
{
    //RG_DEBUG << "slotInstrumentSelected(): instrumentIndex =" << instrumentIndex;

    Instrument *instrument =
            m_doc->getStudio().getInstrumentFromList(instrumentIndex);

    //RG_DEBUG << "slotInstrumentSelected(): instrument " << inst;

    if (!instrument) {
        RG_WARNING << "slotInstrumentSelected(): WARNING: Can't find Instrument";
        return;
    }

    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackByPosition(m_popupTrackPos);

    if (!track) {
        RG_WARNING << "slotInstrumentSelected(): WARNING: Can't find Track";
        return;
    }

    // No change?  Bail.
    if (instrument->getId() == track->getInstrument())
        return;

    // Select the new instrument for the track.

    // ??? This sends a trackChanged() notification.  It shouldn't.  We should
    //     send one here.
    track->setInstrument(instrument->getId());
    // ??? This is what we should do.
    //comp.notifyTrackChanged(track);

    m_doc->slotDocumentModified();

    // Notify IPB, ControlBlock, and SequenceManager.
    selectInstrument(track, instrument);
}

void
TrackButtons::changeLabelDisplayMode(TrackLabel::DisplayMode mode)
{
    // Set new mode
    m_labelDisplayMode = mode;

    // For each track, set the display mode and update.
    for (int i = 0; i < m_tracks; i++) {
        m_trackLabels[i]->setDisplayMode(mode);
        m_trackLabels[i]->updateLabel();
    }
}

void
TrackButtons::slotSynchroniseWithComposition()
{
    //RG_DEBUG << "slotSynchroniseWithComposition()";

    Composition &comp = m_doc->getComposition();

    for (int i = 0; i < m_tracks; i++) {
        updateUI(comp.getTrackByPosition(i));
    }
}

#if 0
void
TrackButtons::slotLabelSelected(int position)
{
    Track *track =
        m_doc->getComposition().getTrackByPosition(position);

    if (track) {
        emit trackSelected(track->getId());
    }
}
#endif

void
TrackButtons::slotTPBInstrumentSelected(TrackId trackId, int instrumentIndex)
{
    //RG_DEBUG << "TrackButtons::slotTPBInstrumentSelected( trackId =" << trackId << ", instrumentIndex =" << instrumentIndex << ")";

    // Set the position for slotInstrumentSelected().
    // ??? This isn't good.  Should have a selectTrack() that takes the
    //     track position and the instrument index.  slotInstrumentSelected()
    //     could call it.
    m_popupTrackPos =
            m_doc->getComposition().getTrackById(trackId)->getPosition();
    slotInstrumentSelected(instrumentIndex);
}

int
TrackButtons::labelWidth()
{
    return m_trackLabelWidth -
           ((m_cellSize - m_buttonGap) * 2 + m_vuSpacing * 2 + m_vuWidth);
}

int
TrackButtons::trackHeight(TrackId trackId)
{
    int multiple = m_doc->
            getComposition().getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple == 0)
        multiple = 1;

    return m_cellSize * multiple - m_borderGap;
}

QFrame*
TrackButtons::makeButton(Track *track)
{
    if (track == nullptr) return nullptr;

    TrackId trackId = track->getId();


    // *** Horizontal Box ***

    QFrame *trackHBox = new QFrame(this);
    QHBoxLayout *hblayout = new QHBoxLayout(trackHBox);
    trackHBox->setLayout(hblayout);
    hblayout->setMargin(0);
    hblayout->setSpacing(0);

    trackHBox->setMinimumSize(labelWidth(), trackHeight(trackId));
    trackHBox->setFixedHeight(trackHeight(trackId));

    trackHBox->setFrameShape(QFrame::StyledPanel);
    trackHBox->setFrameShadow(QFrame::Raised);

    // We will be changing the background color, so turn on auto-fill.
    trackHBox->setAutoFillBackground(true);

    // Insert a little gap
    hblayout->addSpacing(m_vuSpacing);


    // *** VU Meter ***

    TrackVUMeter *vuMeter = new TrackVUMeter(trackHBox,
                                             VUMeter::PeakHold,
                                             m_vuWidth,
                                             m_buttonGap,
                                             track->getPosition());

    m_trackMeters.push_back(vuMeter);

    hblayout->addWidget(vuMeter);

    // Insert a little gap
    hblayout->addSpacing(m_vuSpacing);


    // *** Mute LED ***

    LedButton *mute = new LedButton(
            GUIPalette::getColour(GUIPalette::MuteTrackLED), trackHBox);
    mute->setToolTip(tr("Mute track"));
    hblayout->addWidget(mute);

    connect(mute, SIGNAL(stateChanged(bool)),
            m_muteSigMapper, SLOT(map()));
    m_muteSigMapper->setMapping(mute, track->getPosition());

    m_muteLeds.push_back(mute);

    mute->setFixedSize(m_cellSize - m_buttonGap, m_cellSize - m_buttonGap);


    // *** Record LED ***

    Rosegarden::Instrument *ins =
        m_doc->getStudio().getInstrumentById(track->getInstrument());

    LedButton *record = new LedButton(getRecordLedColour(ins), trackHBox);
    record->setToolTip(tr("Record on this track"));
    hblayout->addWidget(record);

    connect(record, SIGNAL(stateChanged(bool)),
            m_recordSigMapper, SLOT(map()));
    m_recordSigMapper->setMapping(record, track->getPosition());

    m_recordLeds.push_back(record);

    record->setFixedSize(m_cellSize - m_buttonGap, m_cellSize - m_buttonGap);


    // *** Solo LED ***

    LedButton *solo = new LedButton(
            GUIPalette::getColour(GUIPalette::SoloTrackLED), trackHBox);
    solo->setToolTip(tr("Solo track"));
    hblayout->addWidget(solo);

    connect(solo, SIGNAL(stateChanged(bool)),
            m_soloSigMapper, SLOT(map()));
    m_soloSigMapper->setMapping(solo, track->getPosition());

    m_soloLeds.push_back(solo);

    solo->setFixedSize(m_cellSize - m_buttonGap, m_cellSize - m_buttonGap);


    // *** Track Label ***

    TrackLabel *trackLabel =
            new TrackLabel(trackId, track->getPosition(), trackHBox);
    hblayout->addWidget(trackLabel);

    hblayout->addSpacing(m_vuSpacing);

    trackLabel->setDisplayMode(m_labelDisplayMode);

    trackLabel->setFixedSize(labelWidth(), m_cellSize - m_buttonGap);
    trackLabel->setFixedHeight(m_cellSize - m_buttonGap);
    trackLabel->setIndent(7);

    connect(trackLabel, &TrackLabel::renameTrack,
            this, &TrackButtons::slotRenameTrack);

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

    case Instrument::InvalidInstrument:
    default:
        RG_DEBUG << "TrackButtons::slotUpdateTracks() - invalid instrument type, this is probably a BUG!";
        return Qt::green;

    }

}

void
TrackButtons::tracksAdded(const Composition *, std::vector<TrackId> &/*trackIds*/)
{
    //RG_DEBUG << "TrackButtons::tracksAdded()";

    // ??? This is a bit heavy-handed as it just adds a track button, then
    //     recreates all the track buttons.  We might be able to just add the
    //     one that is needed.
    slotUpdateTracks();
}

void
TrackButtons::trackChanged(const Composition *, Track* track)
{
    //RG_DEBUG << "trackChanged()";
    //RG_DEBUG << "  Position:" << track->getPosition();
    //RG_DEBUG << "  Armed:" << track->isArmed();

    updateUI(track);
}

void
TrackButtons::tracksDeleted(const Composition *, std::vector<TrackId> &/*trackIds*/)
{
    //RG_DEBUG << "TrackButtons::tracksDeleted()";

    // ??? This is a bit heavy-handed as it just deletes a track button,
    //     then recreates all the track buttons.  We might be able to just
    //     delete the one that is going away.
    slotUpdateTracks();
}

void
TrackButtons::trackSelectionChanged(const Composition *, TrackId trackId)
{
    //RG_DEBUG << "TrackButtons::trackSelectionChanged()" << trackId;
    Track *track = m_doc->getComposition().getTrackById(trackId);
    selectTrack(track->getPosition());
}

void
TrackButtons::segmentRemoved(const Composition *, Segment *)
{
    // If recording causes the track heights to change, this makes sure
    // they go back if needed when recording stops.

    slotUpdateTracks();
}

void
TrackButtons::slotTrackSelected(int trackId)
{
    // Select the track.
    m_doc->getComposition().setSelectedTrack(trackId);

    // Old notification mechanism
    // ??? This should be replaced with emitDocumentModified() below.
    m_doc->getComposition().notifyTrackSelectionChanged(trackId);

    // Older mechanism.  Keeping this until we can completely replace it
    // with emitDocumentModified() below.
    emit trackSelected(trackId);

    // New notification mechanism.
    // This should replace all others.
    m_doc->emitDocumentModified();
}

void
TrackButtons::slotDocumentModified(bool)
{
    // Full and immediate update.
    // ??? Note that updates probably happen elsewhere.  This will result
    //     in duplicate updates.  All other updates should be removed and
    //     this should be the only update.
    slotUpdateTracks();
}


}
