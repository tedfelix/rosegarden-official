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

#include "base/AudioPluginInstance.h"
#include "gui/general/ClefIndex.h"  // Clef enum
#include "gui/widgets/CollapsingFrame.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Composition.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/InputDialog.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "gui/widgets/LineEdit.h"
#include "base/MidiDevice.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "sound/PluginIdentifier.h"
#include "gui/general/PresetHandlerDialog.h"
#include "document/RosegardenDocument.h"
#include "RosegardenParameterBox.h"
#include "commands/segment/SegmentSyncCommand.h"
#include "gui/widgets/SqueezedLabel.h"
#include "base/StaffExportTypes.h"  // StaffTypes, Brackets
#include "base/Studio.h"
#include "base/Track.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QWidget>


namespace Rosegarden
{


TrackParameterBox::TrackParameterBox(RosegardenDocument *doc,
                                     QWidget *parent) :
    RosegardenParameterBox(tr("Track"), tr("Track Parameters"), parent),
    m_doc(doc),
    m_selectedTrackId((int)NO_TRACK),
    m_lastInstrumentType(Instrument::InvalidInstrument),
    m_lowestPlayable(0),
    m_highestPlayable(127)
{
    setObjectName("Track Parameter Box");

    QFontMetrics metrics(m_font);
    const int width11 = metrics.width("12345678901");
    const int width20 = metrics.width("12345678901234567890");
    const int width22 = metrics.width("1234567890123456789012");
    const int width25 = metrics.width("1234567890123456789012345");

    // Widgets

    // Label
    m_trackLabel = new SqueezedLabel(tr("<untitled>"), this);
    m_trackLabel->setAlignment(Qt::AlignCenter);
    m_trackLabel->setFont(m_font);

    // Playback parameters

    // Outer collapsing frame
    CollapsingFrame *playbackParametersFrame = new CollapsingFrame(
            tr("Playback parameters"), this, "trackparametersplayback", true);

    // Inner fixed widget
    // We need an inner widget so that we can have a layout.  The outer
    // CollapsingFrame already has its own layout.
    QWidget *playbackParameters = new QWidget(playbackParametersFrame);
    playbackParametersFrame->setWidget(playbackParameters);
    playbackParameters->setContentsMargins(3, 3, 3, 3);

    // Device
    QLabel *playbackDeviceLabel = new QLabel(tr("Device"), playbackParameters);
    playbackDeviceLabel->setFont(m_font);
    m_playbackDevice = new QComboBox(playbackParameters);
    m_playbackDevice->setToolTip(tr("<qt><p>Choose the device this track will use for playback.</p><p>Click <img src=\":pixmaps/toolbar/manage-midi-devices.xpm\"> to connect this device to a useful output if you do not hear sound</p></qt>"));
    m_playbackDevice->setMinimumWidth(width25);
    m_playbackDevice->setFont(m_font);
    connect(m_playbackDevice, SIGNAL(activated(int)),
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

    // Archive
    QLabel *archiveLabel = new QLabel(tr("Archive"), playbackParameters);
    archiveLabel->setFont(m_font);
    m_archive = new QCheckBox(playbackParameters);
    m_archive->setFont(m_font);
    m_archive->setToolTip(tr("<qt><p>Check this to archive a track.  Archived tracks will not make sound.</p></qt>"));
    connect(m_archive, SIGNAL(clicked(bool)),
            this, SLOT(slotArchiveChanged(bool)));

    // Playback parameters layout

    // This automagically becomes playbackParameters's layout.
    QGridLayout *groupLayout = new QGridLayout(playbackParameters);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setVerticalSpacing(2);
    groupLayout->setHorizontalSpacing(5);
    // Row 0: Device
    groupLayout->addWidget(playbackDeviceLabel, 0, 0);
    groupLayout->addWidget(m_playbackDevice, 0, 1);
    // Row 1: Instrument
    groupLayout->addWidget(instrumentLabel, 1, 0);
    groupLayout->addWidget(m_instrument, 1, 1);
    // Row 2: Archive
    groupLayout->addWidget(archiveLabel, 2, 0);
    groupLayout->addWidget(m_archive, 2, 1);
    // Let column 1 fill the rest of the space.
    groupLayout->setColumnStretch(1, 1);

    // Recording filters

    CollapsingFrame *recordingFiltersFrame = new CollapsingFrame(
            tr("Recording filters"), this, "trackparametersrecord", false);

    QWidget *recordingFilters = new QWidget(recordingFiltersFrame);
    recordingFiltersFrame->setWidget(recordingFilters);
    recordingFilters->setContentsMargins(3, 3, 3, 3);

    // Device
    QLabel *recordDeviceLabel = new QLabel(tr("Device"), recordingFilters);
    recordDeviceLabel->setFont(m_font);
    m_recordingDevice = new QComboBox(recordingFilters);
    m_recordingDevice->setFont(m_font);
    m_recordingDevice->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected device, filtering anything else out</p></qt>"));
    m_recordingDevice->setMinimumWidth(width25);
    connect(m_recordingDevice, SIGNAL(activated(int)),
            this, SLOT(slotRecordingDeviceChanged(int)));

    // Channel
    QLabel *channelLabel = new QLabel(tr("Channel"), recordingFilters);
    channelLabel->setFont(m_font);
    m_recordingChannel = new QComboBox(recordingFilters);
    m_recordingChannel->setFont(m_font);
    m_recordingChannel->setToolTip(tr("<qt><p>This track will only record Audio/MIDI from the selected channel, filtering anything else out</p></qt>"));
    m_recordingChannel->setMaxVisibleItems(17);
    m_recordingChannel->setMinimumWidth(width11);
    connect(m_recordingChannel, SIGNAL(activated(int)),
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
    groupLayout->setVerticalSpacing(2);
    groupLayout->setHorizontalSpacing(5);
    // Row 0: Device
    groupLayout->addWidget(recordDeviceLabel, 0, 0);
    groupLayout->addWidget(m_recordingDevice, 0, 1);
    // Row 1: Channel
    groupLayout->addWidget(channelLabel, 1, 0);
    groupLayout->addWidget(m_recordingChannel, 1, 1);
    // Row 2: Thru Routing
    groupLayout->addWidget(thruLabel, 2, 0);
    groupLayout->addWidget(m_thruRouting, 2, 1);
    // Let column 1 fill the rest of the space.
    groupLayout->setColumnStretch(1, 1);

    // Staff export options

    CollapsingFrame *staffExportOptionsFrame = new CollapsingFrame(
            tr("Staff export options"), this, "trackstaffgroup", false);

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
    m_notationSize = new QComboBox(staffExportOptions);
    m_notationSize->setFont(m_font);
    m_notationSize->setToolTip(tr("<qt><p>Choose normal, \\small or \\tiny font size for notation elements on this (normal-sized) staff when exporting to LilyPond.</p><p>This is as close as we get to enabling you to print parts in cue size</p></qt>"));
    m_notationSize->setMinimumWidth(width11);
    m_notationSize->addItem(tr("Normal"), StaffTypes::Normal);
    m_notationSize->addItem(tr("Small"), StaffTypes::Small);
    m_notationSize->addItem(tr("Tiny"), StaffTypes::Tiny);
    connect(m_notationSize, SIGNAL(activated(int)),
            this, SLOT(slotNotationSizeChanged(int)));

    // Bracket type
    // Staff bracketing (export only at the moment, but using this for GUI
    // rendering would be nice in the future!) //!!! 
    QLabel *bracketTypeLabel = new QLabel(tr("Bracket type:"), staffExportOptions);
    bracketTypeLabel->setFont(m_font);
    m_bracketType = new QComboBox(staffExportOptions);
    m_bracketType->setFont(m_font);
    m_bracketType->setToolTip(tr("<qt><p>Bracket staffs in LilyPond<br>(fragile, use with caution)</p><qt>"));
    m_bracketType->setMinimumWidth(width11);
    m_bracketType->addItem(tr("-----"), Brackets::None);
    m_bracketType->addItem(tr("[----"), Brackets::SquareOn);
    m_bracketType->addItem(tr("----]"), Brackets::SquareOff);
    m_bracketType->addItem(tr("[---]"), Brackets::SquareOnOff);
    m_bracketType->addItem(tr("{----"), Brackets::CurlyOn);
    m_bracketType->addItem(tr("----}"), Brackets::CurlyOff);
    m_bracketType->addItem(tr("{[---"), Brackets::CurlySquareOn);
    m_bracketType->addItem(tr("---]}"), Brackets::CurlySquareOff);
    connect(m_bracketType, SIGNAL(activated(int)),
            this, SLOT(slotBracketTypeChanged(int)));

    // Staff export options layout

    groupLayout = new QGridLayout(staffExportOptions);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setVerticalSpacing(2);
    groupLayout->setHorizontalSpacing(5);
    groupLayout->setColumnStretch(1, 1);
    // Row 0: Notation size
    groupLayout->addWidget(notationSizeLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_notationSize, 0, 1, 1, 2);
    // Row 1: Bracket type
    groupLayout->addWidget(bracketTypeLabel, 1, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_bracketType, 1, 1, 1, 2);

    // Create segments with

    m_createSegmentsWithFrame = new CollapsingFrame(
            tr("Create segments with"), this, "trackparametersdefaults", false);

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

    m_load = new QPushButton(tr("Load"), createSegmentsWith);
    m_load->setFont(m_font);
    m_load->setToolTip(tr("<qt><p>Load a segment parameters preset from our comprehensive database of real-world instruments.</p><p>When you create new segments, they will have these parameters at the moment of creation.  To use these parameters on existing segments (eg. to convert an existing part in concert pitch for playback on a Bb trumpet) use <b>Segments -> Convert notation for</b> in the notation editor.</p></qt>"));
    connect(m_load, SIGNAL(released()),
            SLOT(slotLoadPressed()));

    // Clef
    QLabel *clefLabel = new QLabel(tr("Clef"), createSegmentsWith);
    clefLabel->setFont(m_font);
    m_clef = new QComboBox(createSegmentsWith);
    m_clef->setFont(m_font);
    m_clef->setToolTip(tr("<qt><p>New segments will be created with this clef inserted at the beginning</p></qt>"));
    m_clef->setMinimumWidth(width11);
    m_clef->addItem(tr("treble", "Clef name"), TrebleClef);
    m_clef->addItem(tr("bass", "Clef name"), BassClef);
    m_clef->addItem(tr("crotales", "Clef name"), CrotalesClef);
    m_clef->addItem(tr("xylophone", "Clef name"), XylophoneClef);
    m_clef->addItem(tr("guitar", "Clef name"), GuitarClef);
    m_clef->addItem(tr("contrabass", "Clef name"), ContrabassClef);
    m_clef->addItem(tr("celesta", "Clef name"), CelestaClef);
    m_clef->addItem(tr("old celesta", "Clef name"), OldCelestaClef);
    m_clef->addItem(tr("french", "Clef name"), FrenchClef);
    m_clef->addItem(tr("soprano", "Clef name"), SopranoClef);
    m_clef->addItem(tr("mezzosoprano", "Clef name"), MezzosopranoClef);
    m_clef->addItem(tr("alto", "Clef name"), AltoClef);
    m_clef->addItem(tr("tenor", "Clef name"), TenorClef);
    m_clef->addItem(tr("baritone", "Clef name"), BaritoneClef);
    m_clef->addItem(tr("varbaritone", "Clef name"), VarbaritoneClef);
    m_clef->addItem(tr("subbass", "Clef name"), SubbassClef);
    m_clef->addItem(tr("twobar", "Clef name"), TwoBarClef);
    connect(m_clef, SIGNAL(activated(int)),
            this, SLOT(slotClefChanged(int)));

    // Transpose
    QLabel *transposeLabel = new QLabel(tr("Transpose"), createSegmentsWith);
    transposeLabel->setFont(m_font);
    m_transpose = new QComboBox(createSegmentsWith);
    m_transpose->setFont(m_font);
    m_transpose->setToolTip(tr("<qt><p>New segments will be created with this transpose property set</p></qt>"));
    connect(m_transpose, SIGNAL(activated(int)),
            SLOT(slotTransposeChanged(int)));

    int transposeRange = 48;
    for (int i = -transposeRange; i < transposeRange + 1; i++) {
        m_transpose->addItem(QString("%1").arg(i));
        if (i == 0)
            m_transpose->setCurrentIndex(m_transpose->count() - 1);
    }

    // Pitch
    QLabel *pitchLabel = new QLabel(tr("Pitch"), createSegmentsWith);
    pitchLabel->setFont(m_font);

    // Lowest playable note
    QLabel *lowestLabel = new QLabel(tr("Lowest"), createSegmentsWith);
    lowestLabel->setFont(m_font);

    m_lowest = new QPushButton(tr("---"), createSegmentsWith);
    m_lowest->setFont(m_font);
    m_lowest->setToolTip(tr("<qt><p>Choose the lowest suggested playable note, using a staff</p></qt>"));
    connect(m_lowest, SIGNAL(released()),
            SLOT(slotLowestPressed()));

    // Highest playable note
    QLabel *highestLabel = new QLabel(tr("Highest"), createSegmentsWith);
    highestLabel->setFont(m_font);

    m_highest = new QPushButton(tr("---"), createSegmentsWith);
    m_highest->setFont(m_font);
    m_highest->setToolTip(tr("<qt><p>Choose the highest suggested playable note, using a staff</p></qt>"));
    connect(m_highest, SIGNAL(released()),
            SLOT(slotHighestPressed()));

    updateHighLow();

    // Color
    QLabel *colorLabel = new QLabel(tr("Color"), createSegmentsWith);
    colorLabel->setFont(m_font);
    m_color = new QComboBox(createSegmentsWith);
    m_color->setFont(m_font);
    m_color->setToolTip(tr("<qt><p>New segments will be created using this color</p></qt>"));
    m_color->setEditable(false);
    m_color->setMaxVisibleItems(20);
    connect(m_color, SIGNAL(activated(int)),
            SLOT(slotColorChanged(int)));

    // "Create segments with" layout

    groupLayout = new QGridLayout(createSegmentsWith);
    groupLayout->setContentsMargins(5,0,0,5);
    groupLayout->setVerticalSpacing(2);
    groupLayout->setHorizontalSpacing(5);
    // Row 0: Preset/Load
    groupLayout->addWidget(presetLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_preset, 0, 1, 1, 3);
    groupLayout->addWidget(m_load, 0, 4, 1, 2);
    // Row 1: Clef/Transpose
    groupLayout->addWidget(clefLabel, 1, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_clef, 1, 1, 1, 2);
    groupLayout->addWidget(transposeLabel, 1, 3, 1, 2, Qt::AlignRight);
    groupLayout->addWidget(m_transpose, 1, 5, 1, 1);
    // Row 2: Pitch/Lowest/Highest
    groupLayout->addWidget(pitchLabel, 2, 0, Qt::AlignLeft);
    groupLayout->addWidget(lowestLabel, 2, 1, Qt::AlignRight);
    groupLayout->addWidget(m_lowest, 2, 2, 1, 1);
    groupLayout->addWidget(highestLabel, 2, 3, Qt::AlignRight);
    groupLayout->addWidget(m_highest, 2, 4, 1, 2);
    // Row 3: Color
    groupLayout->addWidget(colorLabel, 3, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_color, 3, 1, 1, 5);

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

void
TrackParameterBox::setDocument(RosegardenDocument *doc)
{
    // No change?  Bail.
    if (m_doc == doc)
        return;

    m_doc = doc;

    m_doc->getComposition().addObserver(this);

    // updateWidgets()
    slotPopulateDeviceLists();
}

void
TrackParameterBox::slotPopulateDeviceLists()
{
    populatePlaybackDeviceList();

    // Force a record device populate.
    m_lastInstrumentType = Instrument::InvalidInstrument;
    populateRecordingDeviceList();

    slotUpdateControls(-1);
}

void
TrackParameterBox::populatePlaybackDeviceList()
{
    // ??? Make this smart enough to realize there is no need to change
    //     anything.  Then it can be called more frequently.  E.g. cache
    //     the list of Devices and Instruments and check to see if there
    //     have been any changes.

    m_playbackDevice->clear();
    m_playbackDeviceIds.clear();

    m_instrument->clear();
    m_instrumentIds.clear();
    m_instrumentNames.clear();

    // Haven't found a valid device yet.
    DeviceId currentDeviceId = Device::NO_DEVICE;

    // Get the instruments
    InstrumentList instrumentList = m_doc->getStudio().getPresentationInstruments();

    // For each instrument in the studio.
    for (InstrumentList::const_iterator instrumentIter = instrumentList.begin();
         instrumentIter != instrumentList.end();
         ++instrumentIter) {
        const Instrument *instrument = (*instrumentIter);

        // Not valid?  Try the next.
        if (!instrument)
            continue;

        QString instrumentName(QObject::tr(instrument->getName().c_str()));
        QString programName(QObject::tr(instrument->getProgramName().c_str()));

        if (instrument->getType() == Instrument::SoftSynth) {
            instrumentName.replace(QObject::tr("Synth plugin"), "");

            programName = "";

            AudioPluginInstance *plugin =
                    instrument->getPlugin(Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin)
                programName = strtoqstr(plugin->getDisplayName());
        }

        Device *device = instrument->getDevice();
        DeviceId deviceId = device->getId();

        // If we've encountered a new device, add it to the combo.
        if (deviceId != currentDeviceId) {
            currentDeviceId = deviceId;

            m_playbackDevice->addItem(QObject::tr(device->getName().c_str()));
            m_playbackDeviceIds.push_back(currentDeviceId);
        }

        if (programName != "")
            instrumentName += " (" + programName + ")";

        // cut off the redundant eg. "General MIDI Device" that appears in the
        // combo right above here anyway
        instrumentName = instrumentName.mid(
                instrumentName.indexOf("#"), instrumentName.length());

        m_instrumentIds[currentDeviceId].push_back(instrument->getId());
        m_instrumentNames[currentDeviceId].append(instrumentName);
    }

    m_playbackDevice->setCurrentIndex(-1);
    m_instrument->setCurrentIndex(-1);

    // Note that the instrument combo (m_instrument) is not updated
    // by this routine.  slotPlaybackDeviceChanged() copies the strings
    // from m_instrumentNames to m_instrument.
}

void
TrackParameterBox::populateRecordingDeviceList()
{
    Track *track = getTrack();
    if (!track)
        return;

    Instrument *instrument = m_doc->getStudio().getInstrumentFor(track);
    if (!instrument)
        return;

    // If we've changed instrument type, e.g. from Audio to MIDI,
    // reload the combos.
    if (m_lastInstrumentType != instrument->getInstrumentType()) {
        m_lastInstrumentType = instrument->getInstrumentType();

        m_recordingDevice->clear();
        m_recordingDeviceIds.clear();
        m_recordingChannel->clear();

        if (instrument->getInstrumentType() == Instrument::Audio) {

            m_recordingDeviceIds.push_back(Device::NO_DEVICE);

            m_recordingDevice->addItem(tr("Audio"));
            m_recordingDevice->setEnabled(false);

            m_recordingChannel->addItem(tr("Audio"));
            m_recordingChannel->setEnabled(false);

            m_thruRouting->setCurrentIndex(0);
            m_thruRouting->setEnabled(false);

            // hide these for audio instruments
            // ??? We should probably do the same for the "Recording filters"
            //     and "Staff export options" frames.  That would simplify
            //     things.  The above code would go away.
            m_createSegmentsWithFrame->setVisible(false);

        } else { // InstrumentType::Midi and InstrumentType::SoftSynth

            // show these if not audio instrument
            m_createSegmentsWithFrame->setVisible(true);

            m_recordingDeviceIds.push_back(Device::ALL_DEVICES);
            m_recordingDevice->addItem(tr("All"));

            DeviceList *devices = m_doc->getStudio().getDevices();

            // For each device
            for (DeviceListConstIterator it = devices->begin();
                 it != devices->end();
                 ++it) {
                MidiDevice *device = dynamic_cast<MidiDevice *>(*it);

                // If this isn't a MIDI device, try the next.
                if (!device)
                    continue;

                // If this is a device capable of being recorded
                if (device->getDirection() == MidiDevice::Record  &&
                    device->isRecording()) {
                    // Add it to the recording device list.

                    QString deviceName = QObject::tr(device->getName().c_str());
                    m_recordingDevice->addItem(deviceName);
                    m_recordingDeviceIds.push_back(device->getId());
                }
            }

            m_recordingChannel->addItem(tr("All"));
            for (int i = 1; i < 17; ++i) {
                m_recordingChannel->addItem(QString::number(i));
            }

            m_recordingDevice->setEnabled(true);
            m_recordingChannel->setEnabled(true);
            m_thruRouting->setEnabled(true);
        }
    }

    // Set the comboboxes to the proper items.

    if (instrument->getInstrumentType() == Instrument::Audio) {
        m_recordingDevice->setCurrentIndex(0);
        m_recordingChannel->setCurrentIndex(0);
    } else {
        m_recordingDevice->setCurrentIndex(0);
        m_recordingChannel->setCurrentIndex((int)track->getMidiInputChannel() + 1);
        // Search for the track's recording device and set the combo to match.
        for (unsigned int i = 0; i < m_recordingDeviceIds.size(); ++i) {
            if (m_recordingDeviceIds[i] == track->getMidiInputDevice()) {
                m_recordingDevice->setCurrentIndex(i);
                break;
            }
        }
        m_thruRouting->setCurrentIndex((int)track->getThruRouting());
    }
}

void
TrackParameterBox::updateHighLow()
{
    Composition &composition = m_doc->getComposition();
    Track *track = composition.getTrackById(composition.getSelectedTrack());
    if (!track)
        return;

    // Set the highest/lowest in the Track.

    track->setHighestPlayable(m_highestPlayable);
    track->setLowestPlayable(m_lowestPlayable);

    // Update the text on the highest/lowest pushbuttons.

    const Accidental accidental = Accidentals::NoAccidental;

    const Pitch highest(m_highestPlayable, accidental);
    const Pitch lowest(m_lowestPlayable, accidental);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    const int octaveBase = settings.value("midipitchoctave", -2).toInt() ;
    settings.endGroup();

    const bool includeOctave = false;

    // NOTE: this now uses a new, overloaded version of Pitch::getAsString()
    // that explicitly works with the key of C major, and does not allow the
    // calling code to specify how the accidentals should be written out.
    //
    // Separate the note letter from the octave to avoid undue burden on
    // translators having to retranslate the same thing but for a number
    // difference
    QString tmp = QObject::tr(highest.getAsString(includeOctave, octaveBase).c_str(), "note name");
    tmp += tr(" %1").arg(highest.getOctave(octaveBase));
    m_highest->setText(tmp);

    tmp = QObject::tr(lowest.getAsString(includeOctave, octaveBase).c_str(), "note name");
    tmp += tr(" %1").arg(lowest.getOctave(octaveBase));
    m_lowest->setText(tmp);

    m_preset->setEnabled(false);
}

void
TrackParameterBox::slotUpdateControls(int /*dummy*/)
{
    // Device
    slotPlaybackDeviceChanged(-1);
    // Instrument
    slotInstrumentChanged(-1);

    Track *trk = getTrack();
    if (!trk)
        return;

    // Playback parameters
    m_archive->setChecked(trk->isArchived());

    // Create segments with
    m_clef->setCurrentIndex(trk->getClef());
    m_transpose->setCurrentIndex(m_transpose->findText(QString("%1").arg(trk->getTranspose())));
    m_color->setCurrentIndex(trk->getColor());
    m_highestPlayable = trk->getHighestPlayable();
    m_lowestPlayable = trk->getLowestPlayable();
    updateHighLow();
    // set this down here because updateHighLow() just disabled the label
    m_preset->setText(strtoqstr(trk->getPresetLabel()));
    m_preset->setEnabled(true);

    // Staff export options
    m_notationSize->setCurrentIndex(trk->getStaffSize());
    m_bracketType->setCurrentIndex(trk->getStaffBracket());
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
TrackParameterBox::trackSelectionChanged(const Composition *, TrackId newTrackId)
{
    // No change?  Bail.
    if ((int)newTrackId == m_selectedTrackId)
        return;

    m_preset->setEnabled(true);

    m_selectedTrackId = newTrackId;
    selectedTrackNameChanged();
    slotUpdateControls(-1);
}

void
TrackParameterBox::selectedTrackNameChanged()
{
    Track *trk = getTrack();
    if (!trk)
        return;

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
    //RG_DEBUG << "slotPlaybackDeviceChanged(" << index << ")";

    DeviceId deviceId = Device::NO_DEVICE;

    // Nothing is selected, sync with the track's device.
    if (index == -1) {
        Track *trk = getTrack();
        if (!trk)
            return;

        Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
        if (!inst)
            return;

        deviceId = inst->getDevice()->getId();

        // Assume not found.
        int pos = -1;

        // Find the device in the playback devices.
        // this works because we don't have a usable index, and we're having to
        // figure out what to set to.  the external change was to 10001
        // (ALL_DEVICES), so we
        // hunt through our mangled data structure to find that.  this jibes
        // with the notion that our own representation of what's what does not
        // remotely match the TB menu representation.  Our data is the same, but
        // in a completely different and utterly nonsensical order, so we can
        // find it accurately if we know what we're hunting for, otherwise,
        // we're fucked
        for (IdsVector::const_iterator it = m_playbackDeviceIds.begin();
             it != m_playbackDeviceIds.end();
             ++it) {
            ++pos;
            if ((*it) == deviceId)
                break;
        }

        m_playbackDevice->setCurrentIndex(pos);
    } else {
        deviceId = m_playbackDeviceIds[index];
    }

    // used to be "General MIDI Device #7" now we change to "QSynth Device" and
    // we want to remember the #7 bit
    int previousIndex = m_instrument->currentIndex();

    // clear the instrument combo and re-populate it from the new device
    m_instrument->clear();
    m_instrument->addItems(m_instrumentNames[deviceId]);

    // try to keep the same index (the #7 bit) as was in use previously, unless
    // the new instrument has fewer indices available than the previous one did,
    // in which case we just go with the highest valid index available
    if (previousIndex > m_instrument->count())
        previousIndex = m_instrument->count();

    populateRecordingDeviceList();

    if (index != -1) {
        m_instrument->setCurrentIndex(previousIndex);
        slotInstrumentChanged(previousIndex);
    }
}

void
TrackParameterBox::slotInstrumentChanged(int index)
{
    //RG_DEBUG << "slotInstrumentChanged(" << index << ")";

    // If nothing is selected, sync with the tracks instrument.
    if (index == -1) {
        Composition &comp = m_doc->getComposition();

        Track *trk = comp.getTrackById(comp.getSelectedTrack());
        if (!trk)
            return;

        Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
        if (!inst)
            return;

        DeviceId devId = inst->getDevice()->getId();

        // Assume not found.
        int pos = -1;

        // Find the instrument in the instrument list.
        for (IdsVector::const_iterator it = m_instrumentIds[devId].begin();
             it != m_instrumentIds[devId].end();
             ++it) {
            ++pos;
            if ((*it) == trk->getInstrument())
                break;
        }

        m_instrument->setCurrentIndex(pos);
    } else {
        //devId = m_playbackDeviceIds[m_playbackDevice->currentIndex()];

        // Calculate an index to use in Studio::getInstrumentFromList() which
        // gets emitted to TrackButtons, and TrackButtons actually does the work
        // of assigning the instrument to the track, for some bizarre reason.
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
        // For each device that needs to be skipped.
        for (int n = 0; n < m_playbackDevice->currentIndex(); ++n) {
            DeviceId id = m_playbackDeviceIds[n];
            Device *dev = m_doc->getStudio().getDevice(id);

            InstrumentList il = dev->getPresentationInstruments();

            // Accumulate the number of instruments that need to be skipped.
            // get the number of instruments belonging to the device (not the
            // studio)
            prepend += il.size();
        }

        // Convert from TrackParameterBox index to TrackButtons index.
        index += prepend;

        //RG_DEBUG << "slotInstrumentChanged() index = " << index;

        // ??? This check can be done before the for loop to avoid unnecessary
        //     work.
        if (m_doc->getComposition().haveTrack(m_selectedTrackId)) {
            // emit the index we've calculated, relative to the studio list
            // TrackButtons does the rest of the work for us.
            // ??? Why not make the change directly to the Composition, then
            //     fire off an existing CompositionObserver notification?
            //     That should get rid of the for loop above.  See
            //     slotArchiveChanged() for an example.
            emit instrumentSelected(m_selectedTrackId, index);
        }
    }
}

void
TrackParameterBox::slotArchiveChanged(bool checked)
{
    //RG_DEBUG << "slotArchiveChanged(" << checked << ")";

    Track *track = getTrack();

    if (!track)
        return;

    track->setArchived(checked);
    m_doc->slotDocumentModified();

    // Notify observers
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotRecordingDeviceChanged(int index)
{
    //RG_DEBUG << "slotRecordingDeviceChanged(" << index << ")";

    Track *trk = getTrack();
    if (!trk)
        return;

    Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
    if (!inst)
        return;

    // Audio instruments do not support different recording devices.
    if (inst->getInstrumentType() == Instrument::Audio)
        return;

    trk->setMidiInputDevice(m_recordingDeviceIds[index]);
}

void
TrackParameterBox::slotRecordingChannelChanged(int index)
{
    //RG_DEBUG << "slotRecordingChannelChanged(" << index << ")";

    Track *trk = getTrack();
    if (!trk)
        return;

    Instrument *inst = m_doc->getStudio().getInstrumentFor(trk);
    if (!inst)
        return;

    // Audio instruments do not support different recording channels.
    if (inst->getInstrumentType() == Instrument::Audio)
        return;

    trk->setMidiInputChannel(index - 1);
}

void
TrackParameterBox::slotThruRoutingChanged(int index)
{
    Track *track = getTrack();
    if (!track)
        return;

    Instrument *inst = m_doc->getStudio().getInstrumentFor(track);
    if (!inst)
        return;

    // Thru routing is only supported for MIDI instruments.
    if (inst->getInstrumentType() != Instrument::Midi)
        return;

    track->setThruRouting(static_cast<Track::ThruRouting>(index));
    m_doc->slotDocumentModified();

    // Notify observers
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotInstrumentChanged(Instrument *)
{
    populatePlaybackDeviceList();
    slotUpdateControls(-1);
}

void
TrackParameterBox::slotClefChanged(int clef)
{
    //RG_DEBUG << "slotClefChanged(" << clef << ")";

    Track *trk = getTrack();
    if (!trk)
        return;

    trk->setClef(clef);
    m_preset->setEnabled(false);
}

void
TrackParameterBox::transposeChanged(int transpose)
{
    //RG_DEBUG << "transposeChanged(" << transpose << ")";

    Track *trk = getTrack();
    if (!trk)
        return;

    trk->setTranspose(transpose);
    m_preset->setEnabled(false);
}

void
TrackParameterBox::slotTransposeChanged(int index)
{
    QString text = m_transpose->itemText(index);

    if (text.isEmpty())
        return;

    int value = text.toInt();
    transposeChanged(value);
}

void
TrackParameterBox::slotDocColoursChanged()
{
    m_color->clear();

    // Populate it from Composition::m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    // For each color in the segment color map
    for (RCMap::const_iterator colourIter = temp.begin();
         colourIter != temp.end();
         ++colourIter) {
        QString colourName(QObject::tr(colourIter->second.second.c_str()));

        QPixmap colourIcon(15, 15);
        colourIcon.fill(GUIPalette::convertColour(colourIter->second.first));

        if (colourName == "") {
            m_color->addItem(colourIcon, tr("Default"));
        } else {
            // truncate name to 25 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide (This expands from 15 because the
            // translators wrote books instead of copying the style of
            // TheShortEnglishNames, and because we have that much room to
            // spare.)
            if (colourName.length() > 25)
                colourName = colourName.left(22) + "...";

            m_color->addItem(colourIcon, colourName);
        }
    }

    m_color->addItem(tr("Add New Color"));
    m_addColourPos = m_color->count() - 1;
    // ??? Remove since this isn't working?
    m_color->removeItem(m_addColourPos);

    m_color->setCurrentIndex(0);
}

void
TrackParameterBox::slotColorChanged(int index)
{
    //RG_DEBUG << "slotColorChanged(" << index << ")";

    Track *trk = getTrack();
    if (!trk)
        return;

    trk->setColor(index);

#if 0
    // ??? This will never happen since the "Add Color" option is
    //     removed.
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
#endif
}

void
TrackParameterBox::slotHighestPressed()
{
    if (m_selectedTrackId == (int)NO_TRACK)
        return;

    Composition &comp = m_doc->getComposition();

    // Make sure the selected track is valid.
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
    if (m_selectedTrackId == (int)NO_TRACK)
        return;

    Composition &comp = m_doc->getComposition();

    // Make sure the selected track is valid.
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
TrackParameterBox::slotLoadPressed()
{
    // Inherits style.  Centers on main window.
    //PresetHandlerDialog dialog(this);
    // Does not inherit style?  Centers on monitor #1?
    PresetHandlerDialog dialog(0);

    Track *trk = getTrack();
    if (!trk)
        return;

    try {
        if (dialog.exec() == QDialog::Accepted) {
            m_preset->setText(dialog.getName());
            trk->setPresetLabel(qstrtostr(dialog.getName()));

            // If we need to convert the track's segments
            if (dialog.getConvertAllSegments()) {
                Composition &comp = m_doc->getComposition();
                SegmentSyncCommand* command = new SegmentSyncCommand(
                        comp.getSegments(), m_selectedTrackId,
                        dialog.getTranspose(), dialog.getLowRange(), 
                        dialog.getHighRange(),
                        clefIndexToClef(dialog.getClef()));
                CommandHistory::getInstance()->addCommand(command);
            }

            m_clef->setCurrentIndex(dialog.getClef());
            trk->setClef(dialog.getClef());
                     
            m_transpose->setCurrentIndex(m_transpose->findText(
                    QString("%1").arg(dialog.getTranspose())));
            trk->setTranspose(dialog.getTranspose());

            m_highestPlayable = dialog.getHighRange();
            m_lowestPlayable = dialog.getLowRange();
            updateHighLow();

            // updateHighLow() will have set this disabled, so we
            // re-enable it until it is subsequently re-disabled by the
            // user overriding the preset, calling one of the above slots
            // in the normal course
            // ??? This is really subtle.  We should probably just clear it
            //     when modifications are made.  After all, it's no longer
            //     the selected preset.  Or add "(modified)"?  Or change
            //     color?  Maybe we should just get rid of
            //     m_preset?  It doesn't really help much.  It's not even
            //     saved to the .rg file.
            m_preset->setEnabled(true);
        }
    } catch (Exception e) {  // from PresetHandlerDialog
        // !!! This should be a more verbose error to pass along the
        //     row/column of the corruption.
        QMessageBox::warning(0, tr("Rosegarden"),
                tr("The instrument preset database is corrupt.  Check your installation."));
    }
}

void
TrackParameterBox::slotNotationSizeChanged(int index)
{
    Track *trk = getTrack();
    if (!trk)
        return;

    trk->setStaffSize(index);
}

void
TrackParameterBox::slotBracketTypeChanged(int index)
{
    Track *trk = getTrack();
    if (!trk)
        return;

    trk->setStaffBracket(index);
}

Track *
TrackParameterBox::getTrack()
{
    if (m_selectedTrackId == (int)NO_TRACK)
        return NULL;

    if (!m_doc)
        return NULL;

    Composition &comp = m_doc->getComposition();

    // If the track is gone, bail.
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = (int)NO_TRACK;
        return NULL;
    }

    return comp.getTrackById(m_selectedTrackId);
}


}
