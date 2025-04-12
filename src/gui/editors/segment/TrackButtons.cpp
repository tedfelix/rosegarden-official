/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
#include "misc/Preferences.h"

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


namespace
{
    // Constants
    constexpr int borderGap = 1;
    constexpr int buttonGap = 8;
    constexpr int vuSpacing = 2;
    constexpr int minWidth = 200;

    // Colors

    // Parent background color.  This is the area where there are no
    // buttons.
    QColor getBackgroundColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(32, 32, 32);
        else
            return QColor(0xDD, 0xDD, 0xDD);
    }

    // Normal button background color.
    // This is the color of a button that is in normal state as opposed to
    // Archive.
    QColor getButtonBackgroundColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(64, 64, 64);
        else
            return QColor(0xDD, 0xDD, 0xDD);
    }

    // Archive button background color.
    QColor getArchiveButtonBackgroundColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(Qt::black);
        else
            return QColor(0x88, 0x88, 0x88);
    }

    // Color for the numbers to the left.  The label text color is
    // handled in TrackLabel::updatePalette().
    QColor getTextColor()
    {
        if (Rosegarden::Preferences::getTheme() ==
                Rosegarden::Preferences::DarkTheme)
            return QColor(Qt::white);
        else
            return QColor(Qt::black);
    }

}


namespace Rosegarden
{


TrackButtons::TrackButtons(int trackCellHeight,
                           bool showTrackLabels,
                           int overallHeight,
                           QWidget* parent) :
        QFrame(parent),
        m_layout(new QVBoxLayout(this)),
        m_recordSigMapper(new QSignalMapper(this)),
        m_muteSigMapper(new QSignalMapper(this)),
        m_soloSigMapper(new QSignalMapper(this)),
        m_clickedSigMapper(new QSignalMapper(this)),
        m_instListSigMapper(new QSignalMapper(this)),
        m_tracks(RosegardenDocument::currentDocument->getComposition().getNbTracks()),
//        m_offset(4),
        m_trackCellHeight(trackCellHeight),
        m_popupTrackPos(0)
{
    setFrameStyle(Plain);

    QPalette pal = palette();
    pal.setColor(backgroundRole(), getBackgroundColor());
    pal.setColor(foregroundRole(), getTextColor());
    setPalette(pal);

    // when we create the widget, what are we looking at?
    if (showTrackLabels) {
        m_labelDisplayMode = TrackLabel::ShowTrack;
    } else {
        m_labelDisplayMode = TrackLabel::ShowInstrument;
    }

    m_layout->setContentsMargins(0, 0, 0, 0);
    // Set the spacing between vertical elements
    m_layout->setSpacing(borderGap);

    // Now draw the buttons and labels and meters
    //
    makeButtons();

    m_layout->addStretch(20);

    // connect signal mappers
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(m_recordSigMapper, &QSignalMapper::mappedInt,
            this, &TrackButtons::slotToggleRecord);
    connect(m_muteSigMapper, &QSignalMapper::mappedInt,
            this, &TrackButtons::slotToggleMute);
    connect(m_soloSigMapper, &QSignalMapper::mappedInt,
            this, &TrackButtons::slotToggleSolo);
    connect(m_instListSigMapper, &QSignalMapper::mappedInt,
            this, &TrackButtons::slotInstrumentMenu);
    connect(m_clickedSigMapper, &QSignalMapper::mappedInt,
            this, &TrackButtons::slotTrackSelected);
#else
    connect(m_recordSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleRecord(int)));
    connect(m_muteSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleMute(int)));
    connect(m_soloSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotToggleSolo(int)));
    connect(m_instListSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotInstrumentMenu(int)));
    connect(m_clickedSigMapper, SIGNAL(mapped(int)),
            this, SLOT(slotTrackSelected(int)));
#endif

    // We have to force the height for the moment
    //
    setMinimumHeight(overallHeight);

    // We never remove the observer so we should be ok here.
    RosegardenDocument::currentDocument->getComposition().addObserver(this);

    // We do not care about RWM::documentLoaded() because if a new
    // document is being loaded, we are going away.  A new TrackButtons
    // instance is created for each new document.
    //connect(RosegardenMainWindow::self(),
    //            &RosegardenMainWindow::documentLoaded,
    //        this, &TrackButtons::slotDocumentLoaded);
}

TrackButtons::~TrackButtons()
{
    // CRASH!  Probably RosegardenDocument::currentDocument is gone...
    // Probably don't need to disconnect as we only go away when the
    // doc and composition do.  shared_ptr would help here.
    //RosegardenDocument::currentDocument->getComposition().removeObserver(this);
}

void
TrackButtons::updateUI(Track *track)
{
    if (!track)
        return;

    int pos = track->getPosition();

    if (pos < 0  ||  pos >= m_tracks)
        return;

    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;


    // *** Button Colors

    QFrame *hbox = m_trackHBoxes.at(pos);
    QPalette palette = hbox->palette();
    if (track->isArchived()) {
        palette.setColor(hbox->backgroundRole(),
                         getArchiveButtonBackgroundColor());
    } else {
        palette.setColor(hbox->backgroundRole(), getButtonBackgroundColor());
    }

    hbox->setPalette(palette);


    // *** Mute LED

    if (track->isMuted())
        m_muteLeds[pos]->off();
    else
        m_muteLeds[pos]->on();

    if (track->isArchived())
        m_muteLeds[pos]->hide();
    else
        m_muteLeds[pos]->show();


    // *** Record LED

    Instrument *ins =
            document->getStudio().getInstrumentById(track->getInstrument());
    m_recordLeds[pos]->setColor(getRecordLedColour(ins));

    // Note: setRecord() used to be used to do this.  But that would
    //       set the track in the composition to record as well as setting
    //       the button on the UI.  This seems better and works fine.
    bool recording =
            document->getComposition().isTrackRecording(track->getId());
    setRecordButton(pos, recording);

    if (track->isArchived())
        m_recordLeds[pos]->hide();
    else
        m_recordLeds[pos]->show();


    // *** Solo LED

    // ??? An Led::setState(bool) would be handy.
    m_soloLeds[pos]->setState(track->isSolo() ? Led::On : Led::Off);

    if (track->isArchived())
        m_soloLeds[pos]->hide();
    else
        m_soloLeds[pos]->show();


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

    label->setSelected(
            track->getId() == document->getComposition().getSelectedTrack());
    label->setArchived(track->isArchived());

}

void
TrackButtons::makeButtons()
{
    if (!RosegardenDocument::currentDocument)
        return;

    //RG_DEBUG << "makeButtons()";

    // Create a horizontal box filled with widgets for each track

    for (int i = 0; i < m_tracks; ++i) {
        Track *track = RosegardenDocument::currentDocument->getComposition().getTrackByPosition(i);

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
                    QCoreApplication::translate("INSTRUMENT",
                                                ins->getProgramName().c_str()));
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
        Track *track = RosegardenDocument::currentDocument->getComposition().getTrackByPosition(i);

        if (!track)
            continue;

        updateUI(track);

    }
}

void
TrackButtons::slotToggleMute(int position)
{
    //RG_DEBUG << "TrackButtons::slotToggleMute( position =" << pos << ")";

    if (!RosegardenDocument::currentDocument)
        return;

    if (position < 0  ||  position >= m_tracks)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackByPosition(position);

    if (!track)
        return;

    // Toggle the mute state
    track->setMuted(!track->isMuted());

    // Notify observers
    comp.notifyTrackChanged(track);
    RosegardenDocument::currentDocument->slotDocumentModified();
}

void TrackButtons::toggleSolo()
{
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    int pos = comp.getTrackPositionById(comp.getSelectedTrack());

    if (pos == -1)
        return;

    slotToggleSolo(pos);
}

void
TrackButtons::slotToggleSolo(int position)
{
    //RG_DEBUG << "slotToggleSolo( position =" << position << ")";

    if (!RosegardenDocument::currentDocument)
        return;

    if (position < 0  ||  position >= m_tracks)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackByPosition(position);

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
            if (i == position)
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
    RosegardenDocument::currentDocument->slotDocumentModified();
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

    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
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
            Track *track = RosegardenDocument::currentDocument->getComposition().getTrackByPosition(i);
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

        m_trackHBoxes[i]->setMinimumSize(minWidth, trackHeight(track->getId()));
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
    if (!RosegardenDocument::currentDocument)
        return;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackByPosition(position);

    if (!track)
        return;

    // Toggle
    bool state = !comp.isTrackRecording(track->getId());

    // Update the Track
    comp.setTrackRecording(track->getId(), state);

    // Notify observers
    comp.notifyTrackChanged(track);
    RosegardenDocument::currentDocument->slotDocumentModified();

    RosegardenDocument::currentDocument->checkAudioPath(track);
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

    // ??? TrackLabel::setSelected() is called in a couple of places.
    //     We should probably just consolidate into an update routine
    //     that does the following, but using Composition::getSelectedTrack()
    //     converted to position, of course.

    // For each Track label, update selection.
    for (int i = 0; i < m_tracks; ++i)
    {
        // Selected one?
        if (i == position)
            m_trackLabels[i]->setSelected(true);
        else  // Not selected.
            m_trackLabels[i]->setSelected(false);
    }
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
    if (!RosegardenDocument::currentDocument) return;

    Track *track = RosegardenDocument::currentDocument->getComposition().getTrackById(trackId);

    if (!track) return;

    TrackLabel *label = m_trackLabels[track->getPosition()];

    // If neither label is changing, skip it
    if (label->getTrackName() == longLabel &&
        QString::fromStdString(track->getShortLabel()) == shortLabel) return;

    // Rename the track
    CommandHistory::getInstance()->addCommand(
            new RenameTrackCommand(&RosegardenDocument::currentDocument->getComposition(),
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
    Composition &comp = RosegardenDocument::currentDocument->getComposition();

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

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    const int position = comp.getTrackById(trackId)->getPosition();
    Track *track = comp.getTrackByPosition(position);

    Instrument *instrument = nullptr;

    if (track != nullptr) {
        instrument = RosegardenDocument::currentDocument->getStudio().getInstrumentById(
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

        connectedPixmap = IconLoader::loadPixmap("connected");
        connectedUsedPixmap = IconLoader::loadPixmap("connected-used");
        connectedSelectedPixmap = IconLoader::loadPixmap("connected-selected");
        unconnectedPixmap = IconLoader::loadPixmap("unconnected");
        unconnectedUsedPixmap = IconLoader::loadPixmap("unconnected-used");
        unconnectedSelectedPixmap = IconLoader::loadPixmap("unconnected-selected");

        havePixmaps = true;
    }

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    // clear the popup
    instrumentPopup->clear();

    QMenu *currentSubMenu = nullptr;

    // position index
    int count = 0;

    int currentDevId = -1;

    // Get the list
    Studio &studio = RosegardenDocument::currentDocument->getStudio();
    InstrumentList list = studio.getPresentationInstruments();

    // For each instrument
    for (InstrumentList::iterator instrumentIter = list.begin();
         instrumentIter != list.end();
         ++instrumentIter) {

        if (!(*instrumentIter)) continue; // sanity check

        // get the Localized instrument name, with the string hackery performed
        // in Instrument
        QString iname((*instrumentIter)->getLocalizedPresentationName());

        // translate the program name
        //
        // Note we are converting the string from std to Q back to std then to
        // C.  This is obviously ridiculous, but the fact that we have programName
        // here at all makes me think it exists as some kind of necessary hack
        // to coax tr() into behaving nicely.  I decided to change it as little
        // as possible to get it to compile, and not refactor this down to the
        // simplest way to call tr() on a C string.
        QString programName(strtoqstr((*instrumentIter)->getProgramName()));
        programName = QCoreApplication::translate(
                                            "INSTRUMENT",
                                            programName.toStdString().c_str());

        Device *device = (*instrumentIter)->getDevice();
        DeviceId devId = device->getId();
        bool connectedIcon = false;

        // Determine the proper program name and whether it is connected

        if ((*instrumentIter)->getType() == Instrument::SoftSynth) {
            programName = "";
            AudioPluginInstance *plugin =
                    (*instrumentIter)->getPlugin(Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                // we don't translate any plugin program names or other texts
                programName = strtoqstr(plugin->getDisplayName());
                connectedIcon = (plugin->getIdentifier() != "");
            }
        } else if ((*instrumentIter)->getType() == Instrument::Audio) {
            connectedIcon = true;
        } else {
            QString conn = RosegardenSequencer::getInstance()->
                    getConnection(devId);
            connectedIcon = (conn != "");
        }

        // These two are for selecting the correct icon to display.
        bool instrUsedByMe = false;
        bool instrUsedByAnyone = false;

        if (thisTrackInstr && thisTrackInstr->getId() == (*instrumentIter)->getId()) {
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
                // For each Track...
                for (Composition::TrackMap::iterator trackIter =
                         comp.getTracks().begin();
                     trackIter != comp.getTracks().end();
                     ++trackIter) {

                    if (trackIter->second->getInstrument() == (*instrumentIter)->getId()) {
                        instrUsedByAnyone = true;
                        deviceUsedByAnyone = true;
                        break;
                    }

                    Instrument *instr =
                        studio.getInstrumentById(trackIter->second->getInstrument());
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
            QString deviceName = QCoreApplication::translate(
                                                    "INSTRUMENT",
                                                    device->getName().c_str());
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
            // instrument.
            for (Composition::TrackMap::const_iterator trackIter =
                     comp.getTracks().begin();
                 trackIter != comp.getTracks().end();
                 ++trackIter) {
                if (trackIter->second->getInstrument() == (*instrumentIter)->getId()) {
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
        action->setObjectName(iname + QString(QChar(count)));

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

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    SequenceManager *sequenceManager = RosegardenDocument::currentDocument->getSequenceManager();

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
            RosegardenDocument::currentDocument->getStudio().getInstrumentFromList(instrumentIndex);

    //RG_DEBUG << "slotInstrumentSelected(): instrument " << inst;

    if (!instrument) {
        RG_WARNING << "slotInstrumentSelected(): WARNING: Can't find Instrument";
        return;
    }

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
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

    RosegardenDocument::currentDocument->slotDocumentModified();

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

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    for (int i = 0; i < m_tracks; i++) {
        updateUI(comp.getTrackByPosition(i));
    }
}

#if 0
void
TrackButtons::slotLabelSelected(int position)
{
    Track *track =
            RosegardenDocument::currentDocument->getComposition().getTrackByPosition(position);

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
            RosegardenDocument::currentDocument->getComposition().getTrackById(trackId)->getPosition();
    slotInstrumentSelected(instrumentIndex);
}

int
TrackButtons::trackHeight(TrackId trackId)
{
    int multiple = RosegardenDocument::currentDocument->
            getComposition().getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple == 0)
        multiple = 1;

    return m_trackCellHeight * multiple - borderGap;
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
    hblayout->setContentsMargins(0, 0, 0, 0);
    hblayout->setSpacing(0);

    trackHBox->setMinimumSize(minWidth, trackHeight(trackId));
    trackHBox->setFixedHeight(trackHeight(trackId));

    trackHBox->setFrameShape(QFrame::StyledPanel);
    trackHBox->setFrameShadow(QFrame::Raised);

    // Colors
    if (Preferences::getTheme() == Preferences::DarkTheme) {
        QPalette palette = trackHBox->palette();
        // This sets the inner highlight.
        // ??? Sometimes the inner and outer highlights get mixed up.
        //     Not sure why.  But don't expect this to do what you ask.
        palette.setColor(QPalette::Button, QColor(128,128,128));
        // This sets the outer highlight.
        // ??? Sometimes the inner and outer highlights get mixed up.
        //     Not sure why.  But don't expect this to do what you ask.
        //     Also an even more outer highlight can appear.  It's bizarre.
        //     For 128 and 64 it added a 32 highlight.
        palette.setColor(QPalette::Light, QColor(64,64,64));
        // This sets the inner lowlight.
        palette.setColor(QPalette::Dark, QColor(16,16,16));
        // This sets the outer lowlight.
        palette.setColor(QPalette::Shadow, Qt::black);
        trackHBox->setPalette(palette);
    }

    // We will be changing the background color, so turn on auto-fill.
    trackHBox->setAutoFillBackground(true);

    // Insert a little gap
    // ??? Use margins?
    hblayout->addSpacing(vuSpacing);


    // *** VU Meter ***

    const int vuHeight = m_trackCellHeight * 40 / 100;

    TrackVUMeter *vuMeter = new TrackVUMeter(
            trackHBox,  // parent
            VUMeter::PeakHold,  // type
            vuHeight * 3,  // width
            vuHeight,  // height
            track->getPosition());  // position

    m_trackMeters.push_back(vuMeter);

    hblayout->addWidget(vuMeter);

    // Insert a little gap
    hblayout->addSpacing(vuSpacing);


    // *** Mute LED ***

    LedButton *mute = new LedButton(
            GUIPalette::getColour(GUIPalette::MuteTrackLED), trackHBox);
    mute->setToolTip(tr("Mute track"));
    hblayout->addWidget(mute);

    connect(mute, SIGNAL(stateChanged(bool)),
            m_muteSigMapper, SLOT(map()));
    m_muteSigMapper->setMapping(mute, track->getPosition());

    m_muteLeds.push_back(mute);

    mute->setFixedSize(m_trackCellHeight - buttonGap, m_trackCellHeight - buttonGap);


    // *** Record LED ***

    Rosegarden::Instrument *ins =
            RosegardenDocument::currentDocument->getStudio().getInstrumentById(track->getInstrument());

    LedButton *record = new LedButton(getRecordLedColour(ins), trackHBox);
    record->setToolTip(tr("Record on this track"));
    hblayout->addWidget(record);

    connect(record, SIGNAL(stateChanged(bool)),
            m_recordSigMapper, SLOT(map()));
    m_recordSigMapper->setMapping(record, track->getPosition());

    m_recordLeds.push_back(record);

    record->setFixedSize(m_trackCellHeight - buttonGap, m_trackCellHeight - buttonGap);


    // *** Solo LED ***

    LedButton *solo = new LedButton(
            GUIPalette::getColour(GUIPalette::SoloTrackLED), trackHBox);
    solo->setToolTip(tr("Solo track"));
    hblayout->addWidget(solo);

    connect(solo, SIGNAL(stateChanged(bool)),
            m_soloSigMapper, SLOT(map()));
    m_soloSigMapper->setMapping(solo, track->getPosition());

    m_soloLeds.push_back(solo);

    solo->setFixedSize(m_trackCellHeight - buttonGap, m_trackCellHeight - buttonGap);


    // *** Track Label ***

    TrackLabel *trackLabel = new TrackLabel(
            trackId,
            track->getPosition(),
            m_trackCellHeight - buttonGap,
            trackHBox);
    hblayout->addWidget(trackLabel, 10);

    hblayout->addSpacing(vuSpacing);

    trackLabel->setDisplayMode(m_labelDisplayMode);
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


    // Squash it down to its smallest width.
    trackHBox->setFixedWidth(hblayout->minimumSize().width());


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
    Track *track = RosegardenDocument::currentDocument->getComposition().getTrackById(trackId);
    if (!track)
        return;

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
    RosegardenDocument::currentDocument->getComposition().setSelectedTrack(trackId);

    // Old notification mechanism
    // ??? This should be replaced with emitDocumentModified() below.
    RosegardenDocument::currentDocument->getComposition().notifyTrackSelectionChanged(trackId);

    // Older mechanism.  Keeping this until we can completely replace it
    // with emitDocumentModified() below.
    emit trackSelected(trackId);

    // New notification mechanism.
    // This should replace all others.
    RosegardenDocument::currentDocument->emitDocumentModified();
}

void
TrackButtons::slotDocumentModified(bool)
{
    // ??? This isn't connected to any signal and is never called.
    //     See the "TrackButtons Notification Project" page on the wiki:
    //     https://www.rosegardenmusic.com/wiki/dev:tnp

    // ??? See bug #1625 which requires this.

    // ??? See bug #1623 whose experimental solution in MatrixWidget
    //     is affected by this.

    // Full and immediate update.
    // ??? While we are transitioning to this approach, there will likely
    //     be duplicate updates.  Eventually, all other updates should be
    //     removed and this should be the only update that occurs.
    slotUpdateTracks();
}


}
