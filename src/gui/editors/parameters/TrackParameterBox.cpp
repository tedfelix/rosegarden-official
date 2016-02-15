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
                                     QWidget *parent) :
    RosegardenParameterBox(tr("Track"), tr("Track Parameters"), parent),
    m_doc(doc),
    m_selectedTrackId((int)NO_TRACK),
    m_lastInstrumentType(-1),
    m_lowestPlayable(0),
    m_highestPlayable(127)
{
    setObjectName("Track Parameter Box");

    QFontMetrics metrics(m_font);
    const int width11 = metrics.width("12345678901");
    const int width20 = metrics.width("12345678901234567890");
    const int width22 = metrics.width("1234567890123456789012");
    const int width25 = metrics.width("1234567890123456789012345");

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

    // Label
    m_trackLabel = new SqueezedLabel(tr("<untitled>"), this);
    m_trackLabel->setAlignment(Qt::AlignCenter);
    m_trackLabel->setFont(m_font);

    // Playback parameters

    // Outer collapsing frame
    CollapsingFrame *playbackParametersFrame = new CollapsingFrame(
            tr("Playback parameters"), this, trackParametersPlayback);

    // Inner fixed widget
    // We need an inner widget so that we can have a layout.  The outer
    // CollapsingFrame already has its own layout.
    QWidget *playbackParameters = new QWidget(playbackParametersFrame);
    playbackParametersFrame->setWidget(playbackParameters);
    playbackParameters->setContentsMargins(3, 3, 3, 3);

    // Device
    QLabel *playbackDeviceLabel = new QLabel(tr("Device"), playbackParameters);
    playbackDeviceLabel->setFont(m_font);
    m_playDevice = new QComboBox(playbackParameters);
    m_playDevice->setToolTip(tr("<qt><p>Choose the device this track will use for playback.</p><p>Click <img src=\":pixmaps/toolbar/manage-midi-devices.xpm\"> to connect this device to a useful output if you do not hear sound</p></qt>"));
    m_playDevice->setMinimumWidth(width25);
    m_playDevice->setFont(m_font);
    connect(m_playDevice, SIGNAL(activated(int)),
            this, SLOT(slotPlaybackDeviceChanged(int)));

    // Instrument
    QLabel *instrumentLabel = new QLabel(tr("Instrument"), playbackParameters);
    instrumentLabel->setFont(m_font);
    m_instrument = new QComboBox(playbackParameters);
    m_instrument->setFont(m_font);
    m_instrument->setToolTip(tr("<qt><p>Choose the instrument this track will use for playback. (Configure the instrument in <b>Instrument Parameters</b>).</p></qt>"));
    m_instrument->setMaxVisibleItems(16);
    m_instrument->setMinimumWidth(width22);
    connect(m_instrument, SIGNAL(activated(int)),
            this, SLOT(slotInstrumentChanged(int)));

    // Playback parameters layout

    // This automagically becomes playbackParameters's layout.
    QGridLayout *groupLayout = new QGridLayout(playbackParameters);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setSpacing(2);
    // Row 0: Device
    groupLayout->addWidget(playbackDeviceLabel, 0, 0);
    groupLayout->addWidget(m_playDevice, 0, 1, 1, 2);
    // Row 1: Instrument
    groupLayout->addWidget(instrumentLabel, 1, 0, 1, 2);
    groupLayout->addWidget(m_instrument, 1, 2);
    // Let column 2 fill the rest of the space.
    groupLayout->setColumnStretch(2, 1);

    // Recording filters

    CollapsingFrame *recordingFiltersFrame = new CollapsingFrame(
            tr("Recording filters"), this, trackParametersRecord);

    QWidget *recordingFilters = new QWidget(recordingFiltersFrame);
    recordingFiltersFrame->setWidget(recordingFilters);
    recordingFilters->setContentsMargins(3, 3, 3, 3);

    // Device
    QLabel *recordDeviceLabel = new QLabel(tr("Device"), recordingFilters);
    recordDeviceLabel->setFont(m_font);
    m_recDevice = new QComboBox(recordingFilters);
    m_recDevice->setFont(m_font);
    m_recDevice->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected device, filtering anything else out</p></qt>"));
    m_recDevice->setMinimumWidth(width25);
    connect(m_recDevice, SIGNAL(activated(int)),
            this, SLOT(slotRecordingDeviceChanged(int)));

    // Channel
    QLabel *channelLabel = new QLabel(tr("Channel"), recordingFilters);
    channelLabel->setFont(m_font);
    m_recChannel = new QComboBox(recordingFilters);
    m_recChannel->setFont(m_font);
    m_recChannel->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected channel, filtering anything else out</p></qt>"));
    m_recChannel->setMaxVisibleItems(17);
    m_recChannel->setMinimumWidth(width11);
    connect(m_recChannel, SIGNAL(activated(int)),
            this, SLOT(slotRecordingChannelChanged(int)));

    // Thru Routing
    QLabel *thruLabel = new QLabel(tr("Thru Routing"), recordingFilters);
    thruLabel->setFont(m_font);
    m_thruRouting = new QComboBox(recordingFilters);
    m_thruRouting->setFont(m_font);
    //m_thruRouting->setToolTip(tr("<qt><p>Routing from the input device and channel to the instrument.</p></qt>"));
    m_thruRouting->setMinimumWidth(width11);
    m_thruRouting->addItem(tr("Auto"), Track::Auto);
    m_thruRouting->addItem(tr("On"), Track::On);
    m_thruRouting->addItem(tr("Off"), Track::Off);
    m_thruRouting->addItem(tr("When Armed"), Track::WhenArmed);
    connect(m_thruRouting, SIGNAL(activated(int)),
            this, SLOT(slotThruRoutingChanged(int)));

    // Recording filters layout

    groupLayout = new QGridLayout(recordingFilters);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setSpacing(2);
    // Row 0: Device
    groupLayout->addWidget(recordDeviceLabel, 0, 0);
    groupLayout->addWidget(m_recDevice, 0, 1);
    // Row 1: Channel
    groupLayout->addWidget(channelLabel, 1, 0);
    groupLayout->addWidget(m_recChannel, 1, 1);
    // Row 2: Thru Routing
    groupLayout->addWidget(thruLabel, 2, 0);
    groupLayout->addWidget(m_thruRouting, 2, 1);
    // Let column 1 fill the rest of the space.
    groupLayout->setColumnStretch(1, 1);

    // Staff export options

    CollapsingFrame *staffExportOptionsFrame = new CollapsingFrame(
            tr("Staff export options"), this, trackStaffGroup);

    QWidget *staffExportOptions = new QWidget(staffExportOptionsFrame);
    staffExportOptionsFrame->setWidget(staffExportOptions);
    staffExportOptions->setContentsMargins(2, 2, 2, 2);

    // Notation size (export only)
    //
    // NOTE: This is the only way to get a \small or \tiny inserted before the
    // first note in LilyPond export.  Setting the actual staff size on a
    // per-staff (rather than per-score) basis is something the author of the
    // LilyPond documentation has no idea how to do, so we settle for this,
    // which is not as nice, but actually a lot easier to implement.
    QLabel *notationSizeLabel = new QLabel(tr("Notation size:"), staffExportOptions);
    notationSizeLabel->setFont(m_font);
    m_notationSizeCombo = new QComboBox(staffExportOptions);
    m_notationSizeCombo->setFont(m_font);
    m_notationSizeCombo->setToolTip(tr("<qt><p>Choose normal, \\small or \\tiny font size for notation elements on this (normal-sized) staff when exporting to LilyPond.</p><p>This is as close as we get to enabling you to print parts in cue size</p></qt>"));
    m_notationSizeCombo->setMinimumWidth(width11);
    m_notationSizeCombo->addItem(tr("Normal"), StaffTypes::Normal);
    m_notationSizeCombo->addItem(tr("Small"), StaffTypes::Small);
    m_notationSizeCombo->addItem(tr("Tiny"), StaffTypes::Tiny);
    connect(m_notationSizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffSizeChanged(int)));

    // Bracket type
    // Staff bracketing (export only at the moment, but using this for GUI
    // rendering would be nice in the future!) //!!! 
    QLabel *bracketTypeLabel = new QLabel(tr("Bracket type:"), staffExportOptions);
    bracketTypeLabel->setFont(m_font);
    m_bracketTypeCombo = new QComboBox(staffExportOptions);
    m_bracketTypeCombo->setFont(m_font);
    m_bracketTypeCombo->setToolTip(tr("<qt><p>Bracket staffs in LilyPond<br>(fragile, use with caution)</p><qt>"));
    m_bracketTypeCombo->setMinimumWidth(width11);
    m_bracketTypeCombo->addItem(tr("-----"), Brackets::None);
    m_bracketTypeCombo->addItem(tr("[----"), Brackets::SquareOn);
    m_bracketTypeCombo->addItem(tr("----]"), Brackets::SquareOff);
    m_bracketTypeCombo->addItem(tr("[---]"), Brackets::SquareOnOff);
    m_bracketTypeCombo->addItem(tr("{----"), Brackets::CurlyOn);
    m_bracketTypeCombo->addItem(tr("----}"), Brackets::CurlyOff);
    m_bracketTypeCombo->addItem(tr("{[---"), Brackets::CurlySquareOn);
    m_bracketTypeCombo->addItem(tr("---]}"), Brackets::CurlySquareOff);
    connect(m_bracketTypeCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffBracketChanged(int)));

    // Staff export options layout

    groupLayout = new QGridLayout(staffExportOptions);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setSpacing(2);
    groupLayout->setColumnStretch(1, 1);
    // Row 0: Notation size
    groupLayout->addWidget(notationSizeLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_notationSizeCombo, 0, 1, 1, 2);
    // Row 1: Bracket type
    groupLayout->addWidget(bracketTypeLabel, 1, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_bracketTypeCombo, 1, 1, 1, 2);

    // Create segments with

    m_createSegmentsWithFrame = new CollapsingFrame(
            tr("Create segments with"), this, trackParametersDefaults);

    QWidget *createSegmentsWith = new QWidget(m_createSegmentsWithFrame);
    m_createSegmentsWithFrame->setWidget(createSegmentsWith);
    createSegmentsWith->setContentsMargins(3, 3, 3, 3);

    // Preset
    QLabel *presetLabel = new QLabel(tr("Preset"), createSegmentsWith);
    presetLabel->setFont(m_font);

    m_preset = new QLabel(tr("<none>"), createSegmentsWith);
    m_preset->setFont(m_font);
    m_preset->setObjectName("SPECIAL_LABEL");
    m_preset->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_preset->setMinimumWidth(width20);

    m_loadButton = new QPushButton(tr("Load"), createSegmentsWith);
    m_loadButton->setFont(m_font);
    m_loadButton->setToolTip(tr("<qt><p>Load a segment parameters preset from our comprehensive database of real-world instruments.</p><p>When you create new segments, they will have these parameters at the moment of creation.  To use these parameters on existing segments (eg. to convert an existing part in concert pitch for playback on a Bb trumpet) use <b>Segments -> Convert notation for</b> in the notation editor.</p></qt>"));
    connect(m_loadButton, SIGNAL(released()),
            SLOT(slotPresetPressed()));

    // Clef
    QLabel *clefLabel = new QLabel(tr("Clef"), createSegmentsWith);
    clefLabel->setFont(m_font);
    m_clefCombo = new QComboBox(createSegmentsWith);
    m_clefCombo->setFont(m_font);
    m_clefCombo->setToolTip(tr("<qt><p>New segments will be created with this clef inserted at the beginning</p></qt>"));
    m_clefCombo->setMinimumWidth(width11);
    m_clefCombo->addItem(tr("treble", "Clef name"), TrebleClef);
    m_clefCombo->addItem(tr("bass", "Clef name"), BassClef);
    m_clefCombo->addItem(tr("crotales", "Clef name"), CrotalesClef);
    m_clefCombo->addItem(tr("xylophone", "Clef name"), XylophoneClef);
    m_clefCombo->addItem(tr("guitar", "Clef name"), GuitarClef);
    m_clefCombo->addItem(tr("contrabass", "Clef name"), ContrabassClef);
    m_clefCombo->addItem(tr("celesta", "Clef name"), CelestaClef);
    m_clefCombo->addItem(tr("old celesta", "Clef name"), OldCelestaClef);
    m_clefCombo->addItem(tr("french", "Clef name"), FrenchClef);
    m_clefCombo->addItem(tr("soprano", "Clef name"), SopranoClef);
    m_clefCombo->addItem(tr("mezzosoprano", "Clef name"), MezzosopranoClef);
    m_clefCombo->addItem(tr("alto", "Clef name"), AltoClef);
    m_clefCombo->addItem(tr("tenor", "Clef name"), TenorClef);
    m_clefCombo->addItem(tr("baritone", "Clef name"), BaritoneClef);
    m_clefCombo->addItem(tr("varbaritone", "Clef name"), VarbaritoneClef);
    m_clefCombo->addItem(tr("subbass", "Clef name"), SubbassClef);
    m_clefCombo->addItem(tr("twobar", "Clef name"), TwoBarClef);
    connect(m_clefCombo, SIGNAL(activated(int)),
            this, SLOT(slotClefChanged(int)));

    // Transpose
    QLabel *transposeLabel = new QLabel(tr("Transpose"), createSegmentsWith);
    transposeLabel->setFont(m_font);
    m_transposeCombo = new QComboBox(createSegmentsWith);
    m_transposeCombo->setFont(m_font);
    m_transposeCombo->setToolTip(tr("<qt><p>New segments will be created with this transpose property set</p></qt>"));
    connect(m_transposeCombo, SIGNAL(activated(int)),
            SLOT(slotTransposeIndexChanged(int)));

    int transposeRange = 48;
    for (int i = -transposeRange; i < transposeRange + 1; i++) {
        m_transposeCombo->addItem(QString("%1").arg(i));
        if (i == 0)
            m_transposeCombo->setCurrentIndex(m_transposeCombo->count() - 1);
    }

    // Pitch
    QLabel *pitchLabel = new QLabel(tr("Pitch"), createSegmentsWith);
    pitchLabel->setFont(m_font);

    // Lowest playable note
    QLabel *lowestLabel = new QLabel(tr("Lowest"), createSegmentsWith);
    lowestLabel->setFont(m_font);

    m_lowestButton = new QPushButton(tr("---"), createSegmentsWith);
    m_lowestButton->setFont(m_font);
    m_lowestButton->setToolTip(tr("<qt><p>Choose the lowest suggested playable note, using a staff</p></qt>"));
    connect(m_lowestButton, SIGNAL(released()),
            SLOT(slotLowestPressed()));

    // Highest playable note
    QLabel *highestLabel = new QLabel(tr("Highest"), createSegmentsWith);
    highestLabel->setFont(m_font);

    m_highestButton = new QPushButton(tr("---"), createSegmentsWith);
    m_highestButton->setFont(m_font);
    m_highestButton->setToolTip(tr("<qt><p>Choose the highest suggested playable note, using a staff</p></qt>"));
    connect(m_highestButton, SIGNAL(released()),
            SLOT(slotHighestPressed()));

    updateHighLow();

    // Color
    QLabel *colorLabel = new QLabel(tr("Color"), createSegmentsWith);
    colorLabel->setFont(m_font);
    m_colorCombo = new QComboBox(createSegmentsWith);
    m_colorCombo->setFont(m_font);
    m_colorCombo->setToolTip(tr("<qt><p>New segments will be created using this color</p></qt>"));
    m_colorCombo->setEditable(false);
    m_colorCombo->setMaxVisibleItems(20);
    connect(m_colorCombo, SIGNAL(activated(int)),
            SLOT(slotColorChanged(int)));

    // "Create segments with" layout

    groupLayout = new QGridLayout(createSegmentsWith);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setSpacing(2);
    // Row 0: Preset/Load
    groupLayout->addWidget(presetLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_preset, 0, 1, 1, 3);
    groupLayout->addWidget(m_loadButton, 0, 4, 1, 2);
    // Row 1: Clef/Transpose
    groupLayout->addWidget(clefLabel, 1, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_clefCombo, 1, 1, 1, 2);
    groupLayout->addWidget(transposeLabel, 1, 3, 1, 2, Qt::AlignRight);
    groupLayout->addWidget(m_transposeCombo, 1, 5, 1, 1);
    // Row 2: Pitch/Lowest/Highest
    groupLayout->addWidget(pitchLabel, 2, 0, Qt::AlignLeft);
    groupLayout->addWidget(lowestLabel, 2, 1, Qt::AlignRight);
    groupLayout->addWidget(m_lowestButton, 2, 2, 1, 1);
    groupLayout->addWidget(highestLabel, 2, 3, Qt::AlignRight);
    groupLayout->addWidget(m_highestButton, 2, 4, 1, 2);
    // Row 3: Color
    groupLayout->addWidget(colorLabel, 3, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_colorCombo, 3, 1, 1, 5);

    groupLayout->setColumnStretch(1, 1);
    groupLayout->setColumnStretch(2, 2);

    // populate combo from doc colors
    slotDocColoursChanged();
    
    // Force a popluation of Record / Playback Devices (Playback was not populating).
    slotPopulateDeviceLists();

    // Connections

    // Detect when the document colours are updated
    connect(m_doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotDocColoursChanged()));

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));

    m_doc->getComposition().addObserver(this);

    // Layout

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_trackLabel, 0, 0);
    mainLayout->addWidget(playbackParametersFrame, 1, 0);
    mainLayout->addWidget(recordingFiltersFrame, 2, 0);
    mainLayout->addWidget(staffExportOptionsFrame, 3, 0);
    mainLayout->addWidget(m_createSegmentsWithFrame, 4, 0);

    // Box

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

            m_thruRouting->setCurrentIndex(0);
            m_thruRouting->setEnabled(false);

            // hide these for audio instruments
            m_createSegmentsWithFrame->setVisible(false);

        } else { // InstrumentType::Midi and InstrumentType::SoftSynth

            // show these if not audio instrument
            m_createSegmentsWithFrame->setVisible(true);

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
            m_thruRouting->setEnabled(true);
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
        m_thruRouting->setCurrentIndex((int)trk->getThruRouting());
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
    m_highestButton->setText(tmp);

    tmp = QObject::tr(lowest.getAsString(includeOctave, base).c_str(), "note name");
    tmp += tr(" %1").arg(lowest.getOctave(base));
    m_lowestButton->setText(tmp);

    m_preset->setEnabled(false);
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

    m_clefCombo->setCurrentIndex(trk->getClef());
    m_transposeCombo->setCurrentIndex(m_transposeCombo->findText(QString("%1").arg(trk->getTranspose())));
    m_colorCombo->setCurrentIndex(trk->getColor());
    m_highestPlayable = trk->getHighestPlayable();
    m_lowestPlayable = trk->getLowestPlayable();
    updateHighLow();
    // set this down here because updateHighLow just disabled the label
    m_preset->setText(strtoqstr(trk->getPresetLabel()));
    m_preset->setEnabled(true);

    m_notationSizeCombo->setCurrentIndex(trk->getStaffSize());
    m_bracketTypeCombo->setCurrentIndex(trk->getStaffBracket());
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
        m_preset->setEnabled(true);
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
TrackParameterBox::slotThruRoutingChanged(int index)
{
    if (m_selectedTrackId == (int)NO_TRACK)
        return;

    Composition &comp = m_doc->getComposition();

    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return;
    }

    Track *track = comp.getTrackById(m_selectedTrackId);

    Instrument *inst = m_doc->getStudio().getInstrumentFor(track);
    if (!inst)
        return;

    if (inst->getInstrumentType() == Instrument::Midi)
        track->setThruRouting(static_cast<Track::ThruRouting>(index));
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
    m_preset->setEnabled(false);
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
    m_preset->setEnabled(false);
}

void
TrackParameterBox::slotTransposeIndexChanged(int index)
{
    slotTransposeTextChanged(m_transposeCombo->itemText(index));
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

    m_colorCombo->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i = 0;

    for (RCMap::const_iterator it = temp.begin(); it != temp.end(); ++it) {
        QString qtrunc(QObject::tr(it->second.second.c_str()));
        QPixmap colour(15, 15);
        colour.fill(GUIPalette::convertColour(it->second.first));
        if (qtrunc == "") {
            m_colorCombo->addItem(colour, tr("Default"), i);
        } else {
            // truncate name to 25 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide (This expands from 15 because the
            // translators wrote books instead of copying the style of
            // TheShortEnglishNames, and because we have that much room to
            // spare.)
            if (qtrunc.length() > 25)
                qtrunc = qtrunc.left(22) + "...";
            m_colorCombo->addItem(colour, qtrunc, i);
        }
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_colorCombo->addItem(tr("Add New Color"), m_addColourPos);

    // remove the item we just inserted; this leaves the translation alone, but
    // eliminates the useless option
    //
    //!!! fix after release
    m_colorCombo->removeItem(m_addColourPos);

    m_colorCombo->setCurrentIndex(0);
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

    m_preset->setEnabled(false);
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

    m_preset->setEnabled(false);
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
            m_preset->setText(dialog.getName());
            trk->setPresetLabel(qstrtostr(dialog.getName()));
            if (dialog.getConvertAllSegments()) {
                SegmentSyncCommand* command = new SegmentSyncCommand(
                        comp.getSegments(), m_selectedTrackId,
                        dialog.getTranspose(), dialog.getLowRange(), 
                        dialog.getHighRange(),
                        clefIndexToClef(dialog.getClef()));
                CommandHistory::getInstance()->addCommand(command);
            }
            m_clefCombo->setCurrentIndex(dialog.getClef());
                     
            m_transposeCombo->setCurrentIndex(m_transposeCombo->findText(QString("%1").arg(dialog.getTranspose())));

            m_highestPlayable = dialog.getHighRange();
            m_lowestPlayable = dialog.getLowRange();
            updateHighLow();
            slotClefChanged(dialog.getClef());
            slotTransposeChanged(dialog.getTranspose());

            // the preceding slots will have set this disabled, so we
            // re-enable it until it is subsequently re-disabled by the
            // user overriding the preset, calling one of the above slots
            // in the normal course
            m_preset->setEnabled(true);
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
