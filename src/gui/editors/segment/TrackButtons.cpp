/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackButtons.h"
#include <QLayout>

#include <klocale.h>
#include <kstandarddirs.h>
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
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/general/GUIPalette.h"
#include "gui/kdeext/KLedButton.h"
#include "sound/AudioFileManager.h"
#include "sound/PluginIdentifier.h"
#include "TrackLabel.h"
#include "TrackVUMeter.h"
#include <kglobal.h>
#include <kled.h>
#include <kmessagebox.h>
#include <QCursor>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <qpopupmenu.h>
#include <QSignalMapper>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <qwidgetstack.h>
#include <QToolTip>

namespace Rosegarden
{

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
        m_trackLabelWidth(trackLabelWidth),
        m_popupItem(0),
        m_lastSelected( -1)
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
            this, SLOT(slotToggleRecordTrack(int)));

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
{}

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

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i) {
        track = m_doc->getComposition().getTrackByPosition(i);

        if (track) {
            ins = m_doc->getStudio().getInstrumentById(track->getInstrument());

            // Set mute button from track
            //
            if (track->isMuted())
                m_muteLeds[i]->off();
            else
                m_muteLeds[i]->on();

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
            m_trackLabels[i]->getInstrumentLabel()->setText
                (strtoqstr(ins->getPresentationName()));
            if (ins->sendsProgramChange()) {
                m_trackLabels[i]->setAlternativeLabel(strtoqstr(ins->getProgramName()));
            }

        } else {
            m_trackLabels[i]->getInstrumentLabel()->setText(i18n("<no instrument>"));
        }

        m_trackLabels[i]->update();
    }

}

std::vector<int>
TrackButtons::mutedTracks()
{
    std::vector<int> mutedTracks;

    for (TrackId i = 0; i < m_tracks; i++) {
        if (m_muteLeds[i]->state() == KLed::Off)
            mutedTracks.push_back(i);
    }

    return mutedTracks;
}

void
TrackButtons::slotToggleMutedTrack(int mutedTrackPos)
{
    RG_DEBUG << "TrackButtons::slotToggleMutedTrack(" << mutedTrackPos << ")\n";

    if (mutedTrackPos < 0 || mutedTrackPos > (int)m_tracks )
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

    if (position >= m_trackHBoxes.size()) {
        RG_DEBUG << "%%%%%%%%% BIG PROBLEM : TrackButtons::removeButtons() was passed a non-existing index\n";
        return ;
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
    Composition &comp = m_doc->getComposition();
    unsigned int newNbTracks = comp.getNbTracks();
    Track *track = 0;

    std::cerr << "TrackButtons::slotUpdateTracks" << std::endl;

    if (newNbTracks < m_tracks) {
        for (unsigned int i = m_tracks; i > newNbTracks; --i)
            removeButtons(i - 1);
    } else if (newNbTracks > m_tracks) {
        for (unsigned int i = m_tracks; i < newNbTracks; ++i) {
            track = m_doc->getComposition().getTrackByPosition(i);
            if (track) {
                QFrame *trackHBox = makeButton(track->getId());

                if (trackHBox) {
                    trackHBox->show();
                    m_layout->insertWidget(i, trackHBox);
                    m_trackHBoxes.push_back(trackHBox);
                }
            } else
                RG_DEBUG << "TrackButtons::slotUpdateTracks - can't find TrackId for position " << i << endl;
        }
    }

    // Set height
    //
    for (unsigned int i = 0; i < m_trackHBoxes.size(); ++i) {

        track = comp.getTrackByPosition(i);

        if (track) {
            
            int multiple = m_doc->getComposition()
                .getMaxContemporaneousSegmentsOnTrack(track->getId());
            if (multiple == 0) multiple = 1;

            // nasty dupe from makeButton

            int buttonGap = 8;
            int vuWidth = 20;
            int vuSpacing = 2;

            int labelWidth = m_trackLabelWidth -
                ((m_cellSize - buttonGap) * 2 +
                 vuSpacing * 2 + vuWidth);

            m_trackHBoxes[i]->setMinimumSize
                (labelWidth, m_cellSize * multiple - m_borderGap);

            m_trackHBoxes[i]->setFixedHeight
                (m_cellSize * multiple - m_borderGap);
        }
    }

    // Renumber all the labels
    //
    for (unsigned int i = 0; i < m_trackLabels.size(); ++i) {
        track = comp.getTrackByPosition(i);

        if (track) {
            m_trackLabels[i]->setId(track->getId());

            QLabel *trackLabel = m_trackLabels[i]->getTrackLabel();

            if (track->getLabel() == std::string("")) {
                Instrument *ins =
                    m_doc->getStudio().getInstrumentById(track->getInstrument());
                if (ins && ins->getType() == Instrument::Audio) {
                    trackLabel->setText(i18n("<untitled audio>"));
                } else {
                    trackLabel->setText(i18n("<untitled>"));
                }
            } else {
                trackLabel->setText(strtoqstr(track->getLabel()));
            }

            //             RG_DEBUG << "TrackButtons::slotUpdateTracks - set button mapping at pos "
            //                      << i << " to track id " << track->getId() << endl;
            setButtonMapping(m_trackLabels[i], track->getId());
        }
    }
    m_tracks = newNbTracks;

    // Set record status and colour

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i) {

        track = comp.getTrackByPosition(i);

        if (track) {

            setRecordTrack(i, comp.isTrackRecording(track->getId()));

            Instrument *ins =
                m_doc->getStudio().getInstrumentById(track->getInstrument());

            if (ins &&
                    ins->getType() == Instrument::Audio) {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordAudioTrackLED));
            } else {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordMIDITrackLED));
            }
        }
    }

    // repopulate the buttons
    populateButtons();
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
            if (KMessageBox::warningContinueCancel
                    (this,
                     i18n("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
                     i18n("Warning"),
                     i18n("Set audio file path")) == KMessageBox::Continue) {
                RosegardenGUIApp::self()->slotOpenAudioPathSettings();
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

            /* Obsolete code: audio, MIDI and plugin tracks behave the same now.
                      plcl, 06/2006 - Multitrack MIDI recording
                      
                   bool unselect;

            if (audio) {
            unselect = (otherTrack->getInstrument() == track->getInstrument());
            } else {
            // our track is not an audio track, check that the
            // other isn't either
            Instrument *otherInstrument =
             m_doc->getStudio().getInstrumentById(otherTrack->getInstrument());
            bool otherAudio = (otherInstrument &&
              otherInstrument->getType() == 
              Instrument::Audio);

            unselect = !otherAudio;
            }

            if (unselect) { */

            if (otherTrack->getInstrument() == track->getInstrument()) {
                // found another record track of the same type (and
                // with the same instrument, if audio): unselect that

                //!!! should we tell the user, particularly for the
                //audio case? might seem odd otherwise

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
    m_doc->getComposition().setTrackRecording
    (m_trackLabels[position]->getId(), state);
}

void
TrackButtons::setRecordButton(int position, bool state)
{
    if (position < 0 || position >= (int)m_tracks)
        return ;

    KLedButton* led = m_recordLeds[position];

    led->setState(state ? KLed::On : KLed::Off);
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

    for (unsigned int i = 0; i < m_trackLabels.size(); ++i) {
        if (m_trackLabels[i]->isSelected())
            retList.push_back(i);
    }

    return retList;
}

void
TrackButtons::slotRenameTrack(QString newName, TrackId trackId)
{
    m_doc->getCommandHistory()->addCommand
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

    for (unsigned int i = 0; i < m_trackMeters.size(); ++i) {
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

    for (unsigned int i = 0; i < m_trackMeters.size(); ++i) {
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

    QString instrumentName = i18n("<no instrument>");
    Track *track = comp.getTrackByPosition(position);

    Instrument *instrument = 0;
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

    // Do this here as well as in slotInstrumentPopupActivated, so as
    // to restore the correct alternative label even if no other
    // program was selected from the menu
    if (track != 0) {
        instrument = studio.getInstrumentById(track->getInstrument());
        if (instrument) {
            m_trackLabels[position]->getInstrumentLabel()->
                setText(strtoqstr(instrument->getPresentationName()));
            m_trackLabels[position]->clearAlternativeLabel();
            if (instrument->sendsProgramChange()) {
                m_trackLabels[position]->setAlternativeLabel
                    (strtoqstr(instrument->getProgramName()));
            }
        }
    }
}

void
TrackButtons::populateInstrumentPopup(Instrument *thisTrackInstr, QPopupMenu* instrumentPopup)
{
    static QPixmap connectedPixmap, unconnectedPixmap,
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

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    // clear the popup
    instrumentPopup->clear();

    std::vector<QPopupMenu*> instrumentSubMenus;

    // position index
    int i = 0;

    // Get the list
    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;
    int currentDevId = -1;
    bool deviceUsedByAnyone = false;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it))
            continue; // sanity check

        QString iname(strtoqstr((*it)->getPresentationName()));
        QString pname(strtoqstr((*it)->getProgramName()));
        Device *device = (*it)->getDevice();
        DeviceId devId = device->getId();
        bool connected = false;

        if ((*it)->getType() == Instrument::SoftSynth) {
            pname = "";
            AudioPluginInstance *plugin = (*it)->getPlugin
                (Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
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
            connected = (device->getConnection() != "");
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

            QPopupMenu *subMenu = new QPopupMenu(instrumentPopup);
            QString deviceName = strtoqstr(device->getName());
            instrumentPopup->addItem(iconSet, deviceName, subMenu);
            instrumentSubMenus.push_back(subMenu);

            // Connect up the submenu
            //
            connect(subMenu, SIGNAL(activated(int)),
                    SLOT(slotInstrumentPopupActivated(int)));

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

        if (pname != "")
            iname += " (" + pname + ")";

        instrumentSubMenus[instrumentSubMenus.size() - 1]->addItem(iconSet, iname, i++);
    }

}

void
TrackButtons::slotInstrumentPopupActivated(int item)
{
    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated " << item << endl;

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    Instrument *inst = studio.getInstrumentFromList(item);

    RG_DEBUG << "TrackButtons::slotInstrumentPopupActivated: instrument " << inst << endl;

    if (inst != 0) {
        Track *track = comp.getTrackByPosition(m_popupItem);

        if (track != 0) {
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

            if (inst->getType() == Instrument::Audio) {
                m_recordLeds[m_popupItem]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordAudioTrackLED));
            } else {
                m_recordLeds[m_popupItem]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordMIDITrackLED));
            }
        } else
            RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";
    } else
        RG_DEBUG << "slotInstrumentPopupActivated() - can't find item!\n";

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

            if (ins && ins->getType() == Instrument::Audio) {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordAudioTrackLED));
            } else {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordMIDITrackLED));
            }
        }
    }
}

void
TrackButtons::changeTrackLabel(TrackId id, QString label)
{
    Composition &comp = m_doc->getComposition();
    Track *track;

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

            QString instrumentName(i18n("<no instrument>"));
            if (ins)
                instrumentName = strtoqstr(ins->getPresentationName());

            m_trackLabels[i]->getInstrumentLabel()->setText(instrumentName);

            setRecordButton(i, comp.isTrackRecording(track->getId()));

            if (ins && ins->getType() == Instrument::Audio) {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordAudioTrackLED));
            } else {
                m_recordLeds[i]->setColor
                (GUIPalette::getColour
                 (GUIPalette::RecordMIDITrackLED));
            }
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

    m_muteLeds[pos]->setState(value ? KLed::Off : KLed::On);
}

void
TrackButtons::slotTrackInstrumentSelection(TrackId trackId, int item)
{
    RG_DEBUG << "TrackButtons::slotTrackInstrumentSelection(" << trackId << ")\n";

    Composition &comp = m_doc->getComposition();
    int position = comp.getTrackById(trackId)->getPosition();
    m_popupItem = position;
    slotInstrumentPopupActivated( item );
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
    int multiple = m_doc->getComposition()
        .getMaxContemporaneousSegmentsOnTrack(trackId);
    if (multiple == 0) multiple = 1;
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

    mute = new KLedButton(Rosegarden::GUIPalette::getColour
              (Rosegarden::GUIPalette::MuteTrackLED), trackHBox);
    QToolTip::add(mute, i18n("Mute track"));
    hblayout->addWidget(mute);

    record = new KLedButton(Rosegarden::GUIPalette::getColour
                (Rosegarden::GUIPalette::RecordMIDITrackLED), trackHBox);
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
    m_recordSigMapper->setMapping(record, track->getPosition());
    m_muteSigMapper->setMapping(mute, track->getPosition());

    // Store the KLedButton
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

}
#include "TrackButtons.moc"
