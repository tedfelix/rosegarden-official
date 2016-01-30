/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TrackParameterBox]"

#include "TrackParameterBox.h"

#include "gui/widgets/SqueezedLabel.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "misc/ConfigGroups.h"
#include "base/AudioPluginInstance.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/StaffExportTypes.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "commands/segment/SegmentSyncCommand.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/widgets/CollapsingFrame.h"
#include "gui/widgets/ColourTable.h"
#include "gui/general/GUIPalette.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include "sound/PluginIdentifier.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "misc/Debug.h"
#include "sequencer/RosegardenSequencer.h"

#include <QColorDialog>
#include <QLayout>
#include <QApplication>
#include <QComboBox>
#include <QSettings>
#include <QMessageBox>
#include <QTabWidget>
#include <QColor>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QRegExp>
#include <QScrollArea>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QCheckBox>


namespace Rosegarden
{

TrackParameterBox::TrackParameterBox(RosegardenDocument *doc,
                                     QWidget *parent)
        : RosegardenParameterBox(tr("Track"),
                                 tr("Track Parameters"),
                                 parent),
          m_doc(doc),
          m_highestPlayable(127),
          m_lowestPlayable(0),
          m_selectedTrackId((int)NO_TRACK),
          m_lastInstrumentType(-1)
{
    setObjectName("Track Parameter Box");

    QFont font(m_font);

    QFont title_font(m_font);
    title_font.setBold(true);

    QFontMetrics metrics(font);
    int width11 = metrics.width("12345678901");
    int width20 = metrics.width("12345678901234567890");
    int width22 = metrics.width("1234567890123456789012");
    int width25 = metrics.width("1234567890123456789012345");

    // Set up default expansions for the collapsing elements.  These
    // CollapsingFrame objects keep track of their own settings per object name
    // internally, however, they do not provide a way to specify an initial
    // default setting, so we have to do that here.
    // See the "name" parameter to CollapsingFrame's ctor.
    // ??? CollapsingFrame should provide a way to pass a default so this
    //     code can be moved into its ctor.
    QSettings settings;
    settings.beginGroup(CollapsingFrameConfigGroup);

    // ??? Move these down with their widgets.  Then move into
    //     CollapsingFrame's ctor.
    const QString trackParametersPlayback = "trackparametersplayback";
    bool expanded = qStrToBool(settings.value(trackParametersPlayback, "true")) ;
    settings.setValue(trackParametersPlayback, expanded);

    const QString trackParametersRecord = "trackparametersrecord";
    expanded = qStrToBool(settings.value(trackParametersRecord, "false")) ;
    settings.setValue(trackParametersRecord, expanded);

    const QString trackParametersDefaults = "trackparametersdefaults";
    expanded = qStrToBool(settings.value(trackParametersDefaults, "false")) ;
    settings.setValue(trackParametersDefaults, expanded);

    const QString trackStaffGroup = "trackstaffgroup";
    expanded = qStrToBool(settings.value(trackStaffGroup, "false")) ;
    settings.setValue(trackStaffGroup, expanded);

    settings.endGroup();

    // Widgets

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(1);

    int row = 0;

    // track label
    //
    m_trackLabel = new SqueezedLabel (tr("<untitled>"), this);
    m_trackLabel->setAlignment(Qt::AlignCenter);
    m_trackLabel->setFont(m_font);
    mainLayout->addWidget(m_trackLabel, 0, 0);

    // Playback parameters

    CollapsingFrame *playbackParametersFrame = new CollapsingFrame(
            tr("Playback parameters"), this, trackParametersPlayback);
    m_playbackGroup = new QFrame(playbackParametersFrame);
    playbackParametersFrame->setWidget(m_playbackGroup);
    m_playbackGroup->setContentsMargins(3, 3, 3, 3);
    QGridLayout *groupLayout = new QGridLayout(m_playbackGroup);
    groupLayout->setMargin(0);
    groupLayout->setSpacing(2);

    // playback group title
    //
    row = 0;

    // playback device
    //
    //    row++;
    QLabel *devLabel = new QLabel(tr("Device"), m_playbackGroup);
    devLabel->setFont(m_font);
    groupLayout->addWidget(devLabel, row, 0);
    m_playDevice = new QComboBox(m_playbackGroup);
    m_playDevice->setToolTip(tr("<qt><p>Choose the device this track will use for playback.</p><p>Click <img src=\":pixmaps/toolbar/manage-midi-devices.xpm\"> to connect this device to a useful output if you do not hear sound</p></qt>"));
    m_playDevice->setMinimumWidth(width25);
    m_playDevice->setFont(m_font);
    groupLayout->addWidget(m_playDevice, row, 1, row- row+1, 2);

    // playback instrument
    //
    row++;
    QLabel *insLabel = new QLabel(tr("Instrument"), m_playbackGroup);
    insLabel->setFont(m_font);
    groupLayout->addWidget(insLabel, row, 0, row- row+1, 1- 0+1);
    m_instrument = new QComboBox(m_playbackGroup);
    m_instrument->setFont(m_font);
    m_instrument->setToolTip(tr("<qt><p>Choose the instrument this track will use for playback. (Configure the instrument in <b>Instrument Parameters</b>).</p></qt>"));
    m_instrument->setMaxVisibleItems(16);
    m_instrument->setMinimumWidth(width22);
    groupLayout->addWidget(m_instrument, row, 2);

    groupLayout->setColumnStretch(groupLayout->columnCount() - 1, 1);

    mainLayout->addWidget(playbackParametersFrame, 1, 0);

    // Recording filters

    CollapsingFrame *recordingFiltersFrame = new CollapsingFrame(
            tr("Recording filters"), this, trackParametersRecord);
    m_recordGroup = new QFrame(recordingFiltersFrame);
    recordingFiltersFrame->setWidget(m_recordGroup);
    m_recordGroup->setContentsMargins(3, 3, 3, 3);
    groupLayout = new QGridLayout(m_recordGroup);
    groupLayout->setMargin(0);
    groupLayout->setSpacing(2);

    // recording group title
    //
    row = 0;

    // recording device
    QLabel *label = new QLabel(tr("Device"), m_recordGroup);
    label->setFont(m_font);
    groupLayout->addWidget(label, row, 0);
    m_recDevice = new QComboBox(m_recordGroup);
    m_recDevice->setFont(m_font);
    m_recDevice->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected device, filtering anything else out</p></qt>"));
    m_recDevice->setMinimumWidth(width25);
    groupLayout->addWidget(m_recDevice, row, 1, row- row+1, 2);

    // recording channel
    //
    row++;
    label = new QLabel(tr("Channel"), m_recordGroup);
    label->setFont(font);
    groupLayout->addWidget(label, row, 0, 1, 2);
    m_recChannel = new QComboBox(m_recordGroup);
    m_recChannel->setFont(m_font);
    m_recChannel->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected channel, filtering anything else out</p></qt>"));
    m_recChannel->setMaxVisibleItems(17);
    m_recChannel->setMinimumWidth(width11);
    groupLayout->addWidget(m_recChannel, row, 2);

    groupLayout->setColumnStretch(groupLayout->columnCount() - 1, 1);

    mainLayout->addWidget(recordingFiltersFrame, 2, 0);

    // Staff export options

    CollapsingFrame *staffExportOptionsFrame = new CollapsingFrame(
            tr("Staff export options"), this, trackStaffGroup);
    m_staffGroup = new QFrame(staffExportOptionsFrame);
    staffExportOptionsFrame->setWidget(m_staffGroup);
    m_staffGroup->setContentsMargins(2, 2, 2, 2);
    groupLayout = new QGridLayout(m_staffGroup);
    groupLayout->setSpacing(2);
    groupLayout->setMargin(0);
    groupLayout->setColumnStretch(1, 1);

    row = 0;

    // Notation size (export only)
    //
    // NOTE: This is the only way to get a \small or \tiny inserted before the
    // first note in LilyPond export.  Setting the actual staff size on a
    // per-staff (rather than per-score) basis is something the author of the
    // LilyPond documentation has no idea how to do, so we settle for this,
    // which is not as nice, but actually a lot easier to implement.
    m_staffGrpLbl = new QLabel(tr("Notation size:"), m_staffGroup);
    m_staffGrpLbl->setFont(m_font);
    groupLayout->addWidget(m_staffGrpLbl, row, 0, Qt::AlignLeft);
    m_staffSizeCombo = new QComboBox(m_staffGroup);
    m_staffSizeCombo->setFont(m_font);
    m_staffSizeCombo->setToolTip(tr("<qt><p>Choose normal, \\small or \\tiny font size for notation elements on this (normal-sized) staff when exporting to LilyPond.</p><p>This is as close as we get to enabling you to print parts in cue size</p></qt>"));
    m_staffSizeCombo->setMinimumWidth(width11);
    m_staffSizeCombo->addItem(tr("Normal"), StaffTypes::Normal);
    m_staffSizeCombo->addItem(tr("Small"), StaffTypes::Small);
    m_staffSizeCombo->addItem(tr("Tiny"), StaffTypes::Tiny);

    groupLayout->addWidget(m_staffSizeCombo, row, 1, row- row+1, 2);

    // Staff bracketing (export only at the moment, but using this for GUI
    // rendering would be nice in the future!) //!!! 
    row++;
    m_grandStaffLbl = new QLabel(tr("Bracket type:"), m_staffGroup);
    m_grandStaffLbl->setFont(m_font);
    groupLayout->addWidget(m_grandStaffLbl, row, 0, Qt::AlignLeft);
    m_staffBracketCombo = new QComboBox(m_staffGroup);
    m_staffBracketCombo->setFont(m_font);
    m_staffBracketCombo->setToolTip(tr("<qt><p>Bracket staffs in LilyPond<br>(fragile, use with caution)</p><qt>"));
    m_staffBracketCombo->setMinimumWidth(width11);
    m_staffBracketCombo->addItem(tr("-----"), Brackets::None);
    m_staffBracketCombo->addItem(tr("[----"), Brackets::SquareOn);
    m_staffBracketCombo->addItem(tr("----]"), Brackets::SquareOff);
    m_staffBracketCombo->addItem(tr("[---]"), Brackets::SquareOnOff);
    m_staffBracketCombo->addItem(tr("{----"), Brackets::CurlyOn);
    m_staffBracketCombo->addItem(tr("----}"), Brackets::CurlyOff);
    m_staffBracketCombo->addItem(tr("{[---"), Brackets::CurlySquareOn);
    m_staffBracketCombo->addItem(tr("---]}"), Brackets::CurlySquareOff);

    groupLayout->addWidget(m_staffBracketCombo, row, 1, row- row+1, 2);

    mainLayout->addWidget(staffExportOptionsFrame, 3, 0);

    // Create segments with

    CollapsingFrame *createSegmentsWithFrame = new CollapsingFrame(
            tr("Create segments with"), this, trackParametersDefaults);
    m_defaultsGroup = new QFrame(createSegmentsWithFrame);
    createSegmentsWithFrame->setWidget(m_defaultsGroup);
    m_defaultsGroup->setContentsMargins(3, 3, 3, 3);
    groupLayout = new QGridLayout(m_defaultsGroup);
    groupLayout->setSpacing(2);
    groupLayout->setMargin(0);
    groupLayout->setColumnStretch(1, 1);

    row = 0;

    // preset picker
    m_psetLbl = new QLabel(tr("Preset"), m_defaultsGroup);
    m_psetLbl->setFont(m_font);
    groupLayout->addWidget(m_psetLbl, row, 0, Qt::AlignLeft);

    m_presetLbl = new QLabel(tr("<none>"), m_defaultsGroup);
    m_presetLbl->setFont(m_font);
    m_presetLbl->setObjectName("SPECIAL_LABEL");
    m_presetLbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_presetLbl->setMinimumWidth(width20);
    groupLayout->addWidget(m_presetLbl, row, 1, 1, 3);

    m_presetButton = new QPushButton(tr("Load"), m_defaultsGroup);
    m_presetButton->setFont(m_font);
    m_presetButton->setToolTip(tr("<qt><p>Load a segment parameters preset from our comprehensive database of real-world instruments.</p><p>When you create new segments, they will have these parameters at the moment of creation.  To use these parameters on existing segments (eg. to convert an existing part in concert pitch for playback on a Bb trumpet) use <b>Segments -> Convert notation for</b> in the notation editor.</p></qt>"));
    groupLayout->addWidget(m_presetButton, row, 4, 1, 2);

    // default clef
    //
    row++;
    m_clefLbl = new QLabel(tr("Clef"), m_defaultsGroup);
    m_clefLbl->setFont(m_font);
    groupLayout->addWidget(m_clefLbl, row, 0, Qt::AlignLeft);
    m_defClef = new QComboBox(m_defaultsGroup);
    m_defClef->setFont(m_font);
    m_defClef->setToolTip(tr("<qt><p>New segments will be created with this clef inserted at the beginning</p></qt>"));
    m_defClef->setMinimumWidth(width11);
    m_defClef->addItem(tr("treble", "Clef name"), TrebleClef);
    m_defClef->addItem(tr("bass", "Clef name"), BassClef);
    m_defClef->addItem(tr("crotales", "Clef name"), CrotalesClef);
    m_defClef->addItem(tr("xylophone", "Clef name"), XylophoneClef);
    m_defClef->addItem(tr("guitar", "Clef name"), GuitarClef);
    m_defClef->addItem(tr("contrabass", "Clef name"), ContrabassClef);
    m_defClef->addItem(tr("celesta", "Clef name"), CelestaClef);
    m_defClef->addItem(tr("old celesta", "Clef name"), OldCelestaClef);
    m_defClef->addItem(tr("french", "Clef name"), FrenchClef);
    m_defClef->addItem(tr("soprano", "Clef name"), SopranoClef);
    m_defClef->addItem(tr("mezzosoprano", "Clef name"), MezzosopranoClef);
    m_defClef->addItem(tr("alto", "Clef name"), AltoClef);
    m_defClef->addItem(tr("tenor", "Clef name"), TenorClef);
    m_defClef->addItem(tr("baritone", "Clef name"), BaritoneClef);
    m_defClef->addItem(tr("varbaritone", "Clef name"), VarbaritoneClef);
    m_defClef->addItem(tr("subbass", "Clef name"), SubbassClef);
    m_defClef->addItem(tr("twobar", "Clef name"), TwoBarClef);
    groupLayout->addWidget(m_defClef, row, 1, 1, 2);

    // default transpose
    //
    m_transpLbl = new QLabel(tr("Transpose"), m_defaultsGroup);
    m_transpLbl->setFont(m_font);
    groupLayout->addWidget(m_transpLbl, row, 3, 1, 2, Qt::AlignRight);
    m_defTranspose = new QComboBox(m_defaultsGroup);
    m_defTranspose->setFont(m_font);
    m_defTranspose->setToolTip(tr("<qt><p>New segments will be created with this transpose property set</p></qt>"));
    connect(m_defTranspose, SIGNAL(activated(int)),
            SLOT(slotTransposeIndexChanged(int)));

    int transposeRange = 48;
    for (int i = -transposeRange; i < transposeRange + 1; i++) {
        m_defTranspose->addItem(QString("%1").arg(i));
        if (i == 0)
            m_defTranspose->setCurrentIndex(m_defTranspose->count() - 1);
    }

    groupLayout->addWidget(m_defTranspose, row, 5, 1, 1);

    // highest/lowest playable note
    //
    row++;
    m_rangeLbl = new QLabel(tr("Pitch"), m_defaultsGroup);
    m_rangeLbl->setFont(m_font);
    groupLayout->addWidget(m_rangeLbl, row, 0, Qt::AlignLeft);

    label = new QLabel(tr("Lowest"), m_defaultsGroup);
    label->setFont(m_font);
    groupLayout->addWidget(label, row, 1, Qt::AlignRight);

    m_lowButton = new QPushButton(tr("---"), m_defaultsGroup);
    m_lowButton->setFont(m_font);
    m_lowButton->setToolTip(tr("<qt><p>Choose the lowest suggested playable note, using a staff</p></qt>"));
    groupLayout->addWidget(m_lowButton, row, 2, 1, 1);
    groupLayout->setColumnStretch(2, 2);

    label = new QLabel(tr("Highest"), m_defaultsGroup);
    label->setFont(m_font);
    groupLayout->addWidget(label, row, 3, Qt::AlignRight);

    m_highButton = new QPushButton(tr("---"), m_defaultsGroup);
    m_highButton->setFont(m_font);
    m_highButton->setToolTip(tr("<qt><p>Choose the highest suggested playable note, using a staff</p></qt>"));
    groupLayout->addWidget(m_highButton, row, 4, 1, 2);

//    m_lowButton->setMaximumWidth(width11);
//    m_highButton->setMaximumWidth(width11);

    updateHighLow();

    // default color
    //
    row++;
    m_colorLbl = new QLabel(tr("Color"), m_defaultsGroup);
    m_colorLbl->setFont(m_font);
    groupLayout->addWidget(m_colorLbl, row, 0, Qt::AlignLeft);
    m_defColor = new QComboBox(m_defaultsGroup);
    m_defColor->setFont(m_font);
    m_defColor->setToolTip(tr("<qt><p>New segments will be created using this color</p></qt>"));
    m_defColor->setEditable(false);
    m_defColor->setMaxVisibleItems(20);
    groupLayout->addWidget(m_defColor, row, 1, 1, 5);

    // populate combo from doc colors
    slotDocColoursChanged();
    
    // Force a popluation of Record / Playback Devices (Playback was not populating).
    slotPopulateDeviceLists();

    mainLayout->addWidget(createSegmentsWithFrame, 4, 0);


    // Configure the empty final row to accommodate any extra vertical space.
    //
//    mainLayout->setRowStretch(mainLayout->rowCount() - 1, 1);

    // Connections
    connect(m_playDevice, SIGNAL(activated(int)),
             this, SLOT(slotPlaybackDeviceChanged(int)));

    connect(m_instrument, SIGNAL(activated(int)),
             this, SLOT(slotInstrumentChanged(int)));

    connect(m_recDevice, SIGNAL(activated(int)),
             this, SLOT(slotRecordingDeviceChanged(int)));

    connect(m_recChannel, SIGNAL(activated(int)),
             this, SLOT(slotRecordingChannelChanged(int)));

    connect(m_defClef, SIGNAL(activated(int)),
             this, SLOT(slotClefChanged(int)));

    // Detect when the document colours are updated
    connect(m_doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotDocColoursChanged()));

    // handle colour combo changes
    connect(m_defColor, SIGNAL(activated(int)),
            SLOT(slotColorChanged(int)));

    connect(m_highButton, SIGNAL(released()),
            SLOT(slotHighestPressed()));

    connect(m_lowButton, SIGNAL(released()),
            SLOT(slotLowestPressed()));

    connect(m_presetButton, SIGNAL(released()),
            SLOT(slotPresetPressed()));

    connect(m_staffSizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffSizeChanged(int)));

    connect(m_staffBracketCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffBracketChanged(int)));

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));

    // Box

    m_doc->getComposition().addObserver(this);

    setContentsMargins(2, 7, 2, 2);

    slotUpdateControls(-1);
}

TrackParameterBox::~TrackParameterBox()
{
}

void

TrackParameterBox::setDocument(RosegardenDocument *doc)
{
    if (m_doc != doc) {
        RG_DEBUG << "TrackParameterBox::setDocument\n";
        m_doc = doc;
        m_doc->getComposition().addObserver(this);
        slotPopulateDeviceLists();
    }
}

void
TrackParameterBox::slotPopulateDeviceLists()
{
    RG_DEBUG << "TrackParameterBox::slotPopulateDeviceLists()\n";
    populatePlaybackDeviceList();
    m_lastInstrumentType = -1;  // Attempt to force initial record device populate
    populateRecordingDeviceList();
    slotUpdateControls(-1);
}

void
TrackParameterBox::populatePlaybackDeviceList()
{
    RG_DEBUG << "TrackParameterBox::populatePlaybackDeviceList()\n";
    m_playDevice->clear();
    m_playDeviceIds.clear();
    m_instrument->clear();
    m_instrumentIds.clear();
    m_instrumentNames.clear();

    Studio &studio = m_doc->getStudio();

    // Get the list
    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;
    int currentDevId = -1;

    for (it = list.begin(); it != list.end(); ++it) {

        if (! (*it))
            continue; // sanity check

        QString iname(QObject::tr((*it)->getName().c_str()));
        QString pname(QObject::tr((*it)->getProgramName().c_str()));
        Device *device = (*it)->getDevice();
        DeviceId devId = device->getId();

        if ((*it)->getType() == Instrument::SoftSynth) {
            iname.replace(QObject::tr("Synth plugin"), "");
            pname = "";
            AudioPluginInstance *plugin = (*it)->getPlugin
                                          (Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                pname = strtoqstr(plugin->getProgram());
                QString identifier = strtoqstr(plugin->getIdentifier());
                if (identifier != "") {
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
                }
            }
        }

        if (devId != (DeviceId)(currentDevId)) {
            currentDevId = int(devId);
            QString deviceName = QObject::tr(device->getName().c_str());
            m_playDevice->addItem(deviceName);
            m_playDeviceIds.push_back(currentDevId);
        }

        if (pname != "") iname += " (" + pname + ")";
        // cut off the redundant eg. "General MIDI Device" that appears in the
        // combo right above here anyway
        iname = iname.mid(iname.indexOf("#"), iname.length());
        m_instrumentIds[currentDevId].push_back((*it)->getId());
        m_instrumentNames[currentDevId].append(iname);

    }

    m_playDevice->setCurrentIndex(-1);
    m_instrument->setCurrentIndex(-1);
}

void
TrackParameterBox::populateRecordingDeviceList()
{
    RG_DEBUG << "TrackParameterBox::populateRecordingDeviceList()\n";

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);
    Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
    if (!inst)
        return ;

    if (m_lastInstrumentType != (char)inst->getInstrumentType()) {
        m_lastInstrumentType = (char)inst->getInstrumentType();

        m_recDevice->clear();
        m_recDeviceIds.clear();
        m_recChannel->clear();

        if (inst->getInstrumentType() == Instrument::Audio) {

            m_recDeviceIds.push_back(Device::NO_DEVICE);
            m_recDevice->addItem(tr("Audio"));
            m_recChannel->addItem(tr("Audio"));

            m_recDevice->setEnabled(false);
            m_recChannel->setEnabled(false);

            // hide these for audio instruments
            m_defaultsGroup->parentWidget()->setVisible(false);

        } else { // InstrumentType::Midi and InstrumentType::SoftSynth

            // show these if not audio instrument
            m_defaultsGroup->parentWidget()->setVisible(true);

            m_recDeviceIds.push_back(Device::ALL_DEVICES);
            m_recDevice->addItem(tr("All"));

            DeviceList *devices = m_doc->getStudio().getDevices();
            DeviceListConstIterator it;
            for (it = devices->begin(); it != devices->end(); it++) {
                MidiDevice *dev =
                    dynamic_cast<MidiDevice*>(*it);
                if (dev) {
                    if (dev->getDirection() == MidiDevice::Record
                        && dev->isRecording()) {
                        QString deviceName = QObject::tr(dev->getName().c_str());
                        m_recDevice->addItem(deviceName);
                        m_recDeviceIds.push_back(dev->getId());
                    }
                }
            }

            m_recChannel->addItem(tr("All"));
            for (int i = 1; i < 17; ++i) {
                m_recChannel->addItem(QString::number(i));
            }

            m_recDevice->setEnabled(true);
            m_recChannel->setEnabled(true);
        }
    }

    if (inst->getInstrumentType() == Instrument::Audio) {
        m_recDevice->setCurrentIndex(0);
        m_recChannel->setCurrentIndex(0);
    } else {
        m_recDevice->setCurrentIndex(0);
        m_recChannel->setCurrentIndex((int)trk->getMidiInputChannel() + 1);
        for (unsigned int i = 0; i < m_recDeviceIds.size(); ++i) {
            if (m_recDeviceIds[i] == trk->getMidiInputDevice()) {
                m_recDevice->setCurrentIndex(i);
                break;
            }
        }
    }
}

void
TrackParameterBox::updateHighLow()
{
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;

    trk->setHighestPlayable(m_highestPlayable);
    trk->setLowestPlayable(m_lowestPlayable);

    Accidental accidental = Accidentals::NoAccidental;

    Pitch highest(m_highestPlayable, accidental);
    Pitch lowest(m_lowestPlayable, accidental);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    int base = settings.value("midipitchoctave", -2).toInt() ;
    settings.endGroup();

    bool includeOctave = false;

    // NOTE: this now uses a new, overloaded version of Pitch::getAsString()
    // that explicitly works with the key of C major, and does not allow the
    // calling code to specify how the accidentals should be written out.
    //
    // Separate the note letter from the octave to avoid undue burden on
    // translators having to retranslate the same thing but for a number
    // difference
    QString tmp = QObject::tr(highest.getAsString(includeOctave, base).c_str(), "note name");
    tmp += tr(" %1").arg(highest.getOctave(base));
    m_highButton->setText(tmp);

    tmp = QObject::tr(lowest.getAsString(includeOctave, base).c_str(), "note name");
    tmp += tr(" %1").arg(lowest.getOctave(base));
    m_lowButton->setText(tmp);

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotUpdateControls(int /*dummy*/)
{
    RG_DEBUG << "TrackParameterBox::slotUpdateControls()\n";

    slotPlaybackDeviceChanged(-1);
    slotInstrumentChanged(-1);

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);

    m_defClef->setCurrentIndex(trk->getClef());
    m_defTranspose->setCurrentIndex(m_defTranspose->findText(QString("%1").arg(trk->getTranspose())));
    m_defColor->setCurrentIndex(trk->getColor());
    m_highestPlayable = trk->getHighestPlayable();
    m_lowestPlayable = trk->getLowestPlayable();
    updateHighLow();
    // set this down here because updateHighLow just disabled the label
    m_presetLbl->setText(strtoqstr(trk->getPresetLabel()));
    m_presetLbl->setEnabled(true);

    m_staffSizeCombo->setCurrentIndex(trk->getStaffSize());
    m_staffBracketCombo->setCurrentIndex(trk->getStaffBracket());
}

void
TrackParameterBox::trackChanged(const Composition *, Track *track)
{
    if (!track)
        return;

    if (track->getId() != (unsigned)m_selectedTrackId)
        return;

    // Update the track name in case it has changed.
    selectedTrackNameChanged();
}

void
TrackParameterBox::tracksDeleted(const Composition *, std::vector<TrackId> &trackIds)
{
    //RG_DEBUG << "TrackParameterBox::tracksDeleted(), selected is " << m_selectedTrackId;

    // For each deleted track
    for (unsigned i = 0; i < trackIds.size(); ++i) {
        // If this is the selected track
        if ((int)trackIds[i] == m_selectedTrackId) {
            slotSelectedTrackChanged();
            return;
        }
    }
}

void
TrackParameterBox::trackSelectionChanged(const Composition *, TrackId)
{
    slotSelectedTrackChanged();
}

void
TrackParameterBox::slotSelectedTrackChanged()
{
    RG_DEBUG << "TrackParameterBox::slotSelectedTrackChanged()";

    Composition &comp = m_doc->getComposition();
    TrackId newTrack = comp.getSelectedTrack();
    if ((int)newTrack != m_selectedTrackId) {
        m_presetLbl->setEnabled(true);
        m_selectedTrackId = newTrack;
        selectedTrackNameChanged();
        slotUpdateControls(-1);
    }
}

void
TrackParameterBox::selectedTrackNameChanged()
{
    RG_DEBUG << "TrackParameterBox::selectedTrackNameChanged()";

    if (m_selectedTrackId == (int)NO_TRACK) return;

    Composition &comp = m_doc->getComposition();

    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);
    QString trackName = strtoqstr(trk->getLabel());
    if (trackName.isEmpty())
        trackName = tr("<untitled>");
    else
        trackName.truncate(20);
    int trackNum = trk->getPosition() + 1;
    m_trackLabel->setText(tr("[ Track %1 - %2 ]").arg(trackNum).arg(trackName));
}

void
TrackParameterBox::slotPlaybackDeviceChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotPlaybackDeviceChanged(" << index << ")\n";
    DeviceId devId;
    if (index == -1) {
        if (m_selectedTrackId == (int)NO_TRACK) return ;
        Composition &comp = m_doc->getComposition();
        Track *trk = comp.getTrackById(m_selectedTrackId);
        if (!trk)
            return ;
        Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
        if (!inst)
            return ;
        devId = inst->getDevice()->getId();
        int pos = -1;
        IdsVector::const_iterator it;
        // this works because we don't have a usable index, and we're having to
        // figure out what to set to.  the external change was to 10001, so we
        // hunt through our mangled data structure to find that.  this jibes
        // with the notion that our own representation of what's what does not
        // remotely match the TB menu representation.  Our data is the same, but
        // in a completely different and utterly nonsensical order, so we can
        // find it accurately if we know what we're hunting for, otherwise,
        // we're fucked
        for (it = m_playDeviceIds.begin(); it != m_playDeviceIds.end(); ++it) {
            pos++;
            if ((*it) == devId) break;
        }

        m_playDevice->setCurrentIndex(pos);
    } else {
        devId = m_playDeviceIds[index];
    }

    // used to be "General MIDI Device #7" now we change to "QSynth Device" and
    // we want to remember the #7 bit
    int previousIndex = m_instrument->currentIndex();

    // clear the instrument combo and re-populate it from the new device
    m_instrument->clear();
    m_instrument->addItems(m_instrumentNames[devId]);

    // try to keep the same index (the #7 bit) as was in use previously, unless
    // the new instrument has fewer indices available than the previous one did,
    // in which case we just go with the highest valid index available
    if (previousIndex > m_instrument->count()) previousIndex = m_instrument->count();

    populateRecordingDeviceList();

    if (index != -1) {
        m_instrument->setCurrentIndex(previousIndex);
        slotInstrumentChanged(previousIndex);
    }
}

void
TrackParameterBox::slotInstrumentChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentChanged(" << index << ")\n";
    DeviceId devId;
    Instrument *inst;
    if (index == -1) {
        Composition &comp = m_doc->getComposition();
        Track *trk = comp.getTrackById(comp.getSelectedTrack());
        if (!trk)
            return ;
        inst = m_doc->getStudio().getInstrumentFor(trk);
        if (!inst)
            return ;
        devId = inst->getDevice()->getId();

        int pos = -1;
        IdsVector::const_iterator it;
        for (it = m_instrumentIds[devId].begin(); it != m_instrumentIds[devId].end(); ++it) {
            pos++;
            if ((*it) == trk->getInstrument()) break;
        }
        m_instrument->setCurrentIndex(pos);
    } else {
        devId = m_playDeviceIds[m_playDevice->currentIndex()];

        // Calculate an index to use in Studio::getInstrumentFromList() which
        // gets emitted to TrackButtons, and TrackButtons actually does the work
        // of assigning the track to the instrument, for some bizarre reason.
        //
        // This new method for calculating the index works by:
        //
        // 1. for every play device combo index between 0 and its current index, 
        //
        // 2. get the device that corresponds with that combo box index, and
        //
        // 3. figure out how many instruments that device contains, then
        //
        // 4. Add it all up.  That's how many slots we have to jump over to get
        //    to the point where the instrument combo box index we're working
        //    with here will target the correct instrument in the studio list.
        //
        // I'm sure this whole architecture seemed clever once, but it's an
        // unmaintainable pain in the ass is what it is.  We changed one
        // assumption somewhere, and the whole thing fell on its head,
        // swallowing two entire days of my life to put back with the following
        // magic lines of code:
        int prepend = 0;
        for (int n = 0; n < m_playDevice->currentIndex(); n++) {
            DeviceId id = m_playDeviceIds[n];
            Device *dev = m_doc->getStudio().getDevice(id);

            // get the number of instruments belonging to the device (not the
            // studio)
            InstrumentList il = dev->getPresentationInstruments();
            prepend += il.size();
        }

        index += prepend;

        // emit the index we've calculated, relative to the studio list
        RG_DEBUG << "TrackParameterBox::slotInstrumentChanged() index = " << index << "\n";
        if (m_doc->getComposition().haveTrack(m_selectedTrackId)) {
            emit instrumentSelected(m_selectedTrackId, index);
        }
    }
}

void
TrackParameterBox::slotRecordingDeviceChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotRecordingDeviceChanged(" << index << ")" << endl;
    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);

    Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
    if (!inst) return ;
    if (inst->getInstrumentType() == Instrument::Audio) {
        //Not implemented yet
    } else {
        trk->setMidiInputDevice(m_recDeviceIds[index]);
    }
}

void
TrackParameterBox::slotRecordingChannelChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotRecordingChannelChanged(" << index << ")" << endl;
    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);

    Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
    if (!inst) return ;
    if (inst->getInstrumentType() == Instrument::Audio) {
        //Not implemented yet
    } else {
        trk->setMidiInputChannel(index - 1);
    }
}

void
TrackParameterBox::slotInstrumentChanged(Instrument * /*instrument*/)
{
    //RG_DEBUG << "TrackParameterBox::slotInstrumentChanged()";
    populatePlaybackDeviceList();
    slotUpdateControls(-1);
}

void
TrackParameterBox::showAdditionalControls(bool)
{
    // No longer needed.  Remove when we strip the last of the classic vs.
    // tabbed garbage out of the other parameter boxes
}

void
TrackParameterBox::slotClefChanged(int clef)
{
    RG_DEBUG << "TrackParameterBox::slotClefChanged(" << clef << ")" << endl;
    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }
    Track *trk = comp.getTrackById(m_selectedTrackId);
    trk->setClef(clef);
    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotTransposeChanged(int transpose)
{
    RG_DEBUG << "TrackParameterBox::slotTransposeChanged(" << transpose << ")" << endl;
    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }
    Track *trk = comp.getTrackById(m_selectedTrackId);
    trk->setTranspose(transpose);
    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotTransposeIndexChanged(int index)
{
    slotTransposeTextChanged(m_defTranspose->itemText(index));
}

void
TrackParameterBox::slotTransposeTextChanged(QString text)
{
    if (text.isEmpty()) return;
    int value = text.toInt();
    slotTransposeChanged(value);
}

void
TrackParameterBox::slotDocColoursChanged()
{
    RG_DEBUG << "TrackParameterBox::slotDocColoursChanged()" << endl;

    m_defColor->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i = 0;

    for (RCMap::const_iterator it = temp.begin(); it != temp.end(); ++it) {
        QString qtrunc(QObject::tr(it->second.second.c_str()));
        QPixmap colour(15, 15);
        colour.fill(GUIPalette::convertColour(it->second.first));
        if (qtrunc == "") {
            m_defColor->addItem(colour, tr("Default"), i);
        } else {
            // truncate name to 25 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide (This expands from 15 because the
            // translators wrote books instead of copying the style of
            // TheShortEnglishNames, and because we have that much room to
            // spare.)
            if (qtrunc.length() > 25)
                qtrunc = qtrunc.left(22) + "...";
            m_defColor->addItem(colour, qtrunc, i);
        }
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_defColor->addItem(tr("Add New Color"), m_addColourPos);

    // remove the item we just inserted; this leaves the translation alone, but
    // eliminates the useless option
    //
    //!!! fix after release
    m_defColor->removeItem(m_addColourPos);

    m_defColor->setCurrentIndex(0);
}

void
TrackParameterBox::slotColorChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotColorChanged(" << index << ")" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }
    Track *trk = comp.getTrackById(m_selectedTrackId);

    trk->setColor(index);

    if (index == m_addColourPos) {
        ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;
        
        QString newName = InputDialog::getText(this,
                                               tr("New Color Name"),
                                               tr("Enter new name:"),
                                               LineEdit::Normal,
                                               tr("New"), &ok);
        
        if ((ok == true) && (!newName.isEmpty())) {
//             QColorDialog box(this, "", true);
//             int result = box.getColor(newColour);
            
            //QRgb QColorDialog::getRgba(0xffffffff, &ok, this);
            QColor newColor = QColorDialog::getColor(Qt::white, this);

            if (newColor.isValid()) {
                Colour newRColour = GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name
        // or didn't give a colour
    }
}

void
TrackParameterBox::slotHighestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotHighestPressed()" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    PitchPickerDialog dialog(0, m_highestPlayable, tr("Highest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_highestPlayable = dialog.getPitch();
        updateHighLow();
    }

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotLowestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotLowestPressed()" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    PitchPickerDialog dialog(0, m_lowestPlayable, tr("Lowest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_lowestPlayable = dialog.getPitch();
        updateHighLow();
    }

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotPresetPressed()
{
    RG_DEBUG << "TrackParameterBox::slotPresetPressed()" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    //PresetHandlerDialog dialog(this);
    PresetHandlerDialog dialog(0); // no parent means no style from group box parent, but what about popup location?

    Track *trk = comp.getTrackById(m_selectedTrackId);
    try {
        if (dialog.exec() == QDialog::Accepted) {
            m_presetLbl->setText(dialog.getName());
            trk->setPresetLabel(qstrtostr(dialog.getName()));
            if (dialog.getConvertAllSegments()) {
                SegmentSyncCommand* command = new SegmentSyncCommand(
                        comp.getSegments(), m_selectedTrackId,
                        dialog.getTranspose(), dialog.getLowRange(), 
                        dialog.getHighRange(),
                        clefIndexToClef(dialog.getClef()));
                CommandHistory::getInstance()->addCommand(command);
            }
            m_defClef->setCurrentIndex(dialog.getClef());
                     
            m_defTranspose->setCurrentIndex(m_defTranspose->findText(QString("%1").arg(dialog.getTranspose())));

            m_highestPlayable = dialog.getHighRange();
            m_lowestPlayable = dialog.getLowRange();
            updateHighLow();
            slotClefChanged(dialog.getClef());
            slotTransposeChanged(dialog.getTranspose());

            // the preceding slots will have set this disabled, so we
            // re-enable it until it is subsequently re-disabled by the
            // user overriding the preset, calling one of the above slots
            // in the normal course
            m_presetLbl->setEnabled(true);
        }
    } catch (Exception e) {
        //!!! This should be a more verbose error to pass along the
        // row/column of the corruption, but I can't be bothered to work
        // that out just at the moment.  Hopefully this code will never
        // execute anyway.
        QMessageBox::warning(0, tr("Rosegarden"), tr("The instrument preset database is corrupt.  Check your installation."));
    }

}

void
TrackParameterBox::slotStaffSizeChanged(int index) 
{
    RG_DEBUG << "TrackParameterBox::sotStaffSizeChanged()" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);

    trk->setStaffSize(index);
}


void
TrackParameterBox::slotStaffBracketChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::sotStaffBracketChanged()" << endl;

    if (m_selectedTrackId == (int)NO_TRACK) return;
    Composition &comp = m_doc->getComposition();
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *trk = comp.getTrackById(m_selectedTrackId);

    trk->setStaffBracket(index);
}

QString
TrackParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement arrangement) const
{
    if (arrangement == RosegardenParameterArea::CLASSIC_STYLE) {
        return tr("Segment");
    } else {
        return "";
    }
}

}
