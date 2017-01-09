/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
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
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "RosegardenParameterBox.h"
#include "commands/segment/SegmentSyncCommand.h"
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


TrackParameterBox::TrackParameterBox(QWidget *parent) :
    RosegardenParameterBox(tr("Track"), tr("Track Parameters"), parent),
    m_doc(NULL),
    m_selectedTrackId(NO_TRACK),
    m_lastInstrumentType(Instrument::InvalidInstrument)
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

    m_recordingFiltersFrame = new CollapsingFrame(
            tr("Recording filters"), this, "trackparametersrecord", false);

    QWidget *recordingFilters = new QWidget(m_recordingFiltersFrame);
    m_recordingFiltersFrame->setWidget(recordingFilters);
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
    m_recordingChannel->addItem(tr("All"));
    for (int i = 1; i < 17; ++i) {
        m_recordingChannel->addItem(QString::number(i));
    }
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

    m_staffExportOptionsFrame = new CollapsingFrame(
            tr("Staff export options"), this, "trackstaffgroup", false);

    QWidget *staffExportOptions = new QWidget(m_staffExportOptionsFrame);
    m_staffExportOptionsFrame->setWidget(staffExportOptions);
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
    m_presetLabel = new QLabel(tr("Preset"), createSegmentsWith);
    m_presetLabel->setFont(m_font);

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
    m_clefLabel = new QLabel(tr("Clef"), createSegmentsWith);
    m_clefLabel->setFont(m_font);
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
    m_transposeLabel = new QLabel(tr("Transpose"), createSegmentsWith);
    m_transposeLabel->setFont(m_font);
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
    m_pitchLabel = new QLabel(tr("Pitch"), createSegmentsWith);
    m_pitchLabel->setFont(m_font);

    // Lowest playable note
    m_lowestLabel = new QLabel(tr("Lowest"), createSegmentsWith);
    m_lowestLabel->setFont(m_font);

    m_lowest = new QPushButton(tr("---"), createSegmentsWith);
    m_lowest->setFont(m_font);
    m_lowest->setToolTip(tr("<qt><p>Choose the lowest suggested playable note, using a staff</p></qt>"));
    connect(m_lowest, SIGNAL(released()),
            SLOT(slotLowestPressed()));

    // Highest playable note
    m_highestLabel = new QLabel(tr("Highest"), createSegmentsWith);
    m_highestLabel->setFont(m_font);

    m_highest = new QPushButton(tr("---"), createSegmentsWith);
    m_highest->setFont(m_font);
    m_highest->setToolTip(tr("<qt><p>Choose the highest suggested playable note, using a staff</p></qt>"));
    connect(m_highest, SIGNAL(released()),
            SLOT(slotHighestPressed()));

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
    groupLayout->addWidget(m_presetLabel, 0, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_preset, 0, 1, 1, 3);
    groupLayout->addWidget(m_load, 0, 4, 1, 2);
    // Row 1: Clef/Transpose
    groupLayout->addWidget(m_clefLabel, 1, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_clef, 1, 1, 1, 2);
    groupLayout->addWidget(m_transposeLabel, 1, 3, 1, 2, Qt::AlignRight);
    groupLayout->addWidget(m_transpose, 1, 5, 1, 1);
    // Row 2: Pitch/Lowest/Highest
    groupLayout->addWidget(m_pitchLabel, 2, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_lowestLabel, 2, 1, Qt::AlignRight);
    groupLayout->addWidget(m_lowest, 2, 2, 1, 1);
    groupLayout->addWidget(m_highestLabel, 2, 3, Qt::AlignRight);
    groupLayout->addWidget(m_highest, 2, 4, 1, 2);
    // Row 3: Color
    groupLayout->addWidget(colorLabel, 3, 0, Qt::AlignLeft);
    groupLayout->addWidget(m_color, 3, 1, 1, 5);

    groupLayout->setColumnStretch(1, 1);
    groupLayout->setColumnStretch(2, 2);

    // Connections

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));

    // Layout

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_trackLabel, 0, 0);
    mainLayout->addWidget(playbackParametersFrame, 1, 0);
    mainLayout->addWidget(m_recordingFiltersFrame, 2, 0);
    mainLayout->addWidget(m_staffExportOptionsFrame, 3, 0);
    mainLayout->addWidget(m_createSegmentsWithFrame, 4, 0);

    // Box

    setContentsMargins(2, 7, 2, 2);

    updateWidgets2();
}

void
TrackParameterBox::setDocument(RosegardenDocument *doc)
{
    // No change?  Bail.
    if (m_doc == doc)
        return;

    m_doc = doc;

    m_doc->getComposition().addObserver(this);

    // Populate color combo from the document colors.
    // ??? Now that we actually load these from the new document, we can
    //     see what's actually in .rg files.  Older ones have almost no
    //     colors at all (e.g. the Vivaldi op44 example file).  Since we
    //     don't allow editing of the colors, we should probably just
    //     always put the standard color list into a document when it is
    //     loaded.
    slotDocColoursChanged();

    // Detect when the document colours are updated.
    // ??? Document colors can never be updated.  See explanation in
    //     slotDocColoursChanged().
    //connect(m_doc, SIGNAL(docColoursChanged()),
    //        this, SLOT(slotDocColoursChanged()));

    updateWidgets2();
}

void
TrackParameterBox::devicesChanged()
{
    updateWidgets2();
}

void
TrackParameterBox::trackChanged(const Composition *, Track *track)
{
    if (!track)
        return;

    if (track->getId() != m_selectedTrackId)
        return;

    updateWidgets2();
}

void
TrackParameterBox::trackSelectionChanged(const Composition *, TrackId newTrackId)
{
    // No change?  Bail.
    if (newTrackId == m_selectedTrackId)
        return;

    m_preset->setEnabled(true);

    m_selectedTrackId = newTrackId;

    updateWidgets2();
}

void
TrackParameterBox::slotPlaybackDeviceChanged(int index)
{
    //RG_DEBUG << "slotPlaybackDeviceChanged(" << index << ")";

    // If nothing is selected, bail.
    if (index < 0)
        return;

    // Out of range?  Bail.
    if (index >= static_cast<int>(m_playbackDeviceIds2.size()))
        return;

    Track *track = getTrack();
    if (!track)
        return;

    // Switch the Track to the same instrument # on this new Device.

    DeviceId deviceId = m_playbackDeviceIds2[index];

    Device *device = m_doc->getStudio().getDevice(deviceId);
    if (!device)
        return;

    // Query the Studio to get an Instrument for this Device.
    InstrumentList instrumentList = device->getPresentationInstruments();

    // Try to preserve the Instrument number (channel) if possible.
    int instrumentIndex = m_instrument->currentIndex();
    if (instrumentIndex >= static_cast<int>(instrumentList.size()))
        instrumentIndex = 0;

    // Set the Track's Instrument to the new Instrument.
    track->setInstrument(instrumentList[instrumentIndex]->getId());

    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    // ??? Redundant notification.  Track::setInstrument() already does
    //     this.  It shouldn't.
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);

    // ??? The following needs to go away.

    RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getTrackButtons()->selectInstrument(
                    track, instrumentList[instrumentIndex]);
}

void
TrackParameterBox::slotInstrumentChanged(int index)
{
    //RG_DEBUG << "slotInstrumentChanged(" << index << ")";

    // If nothing is selected, bail.
    if (index < 0)
        return;

    // Out of range?  Bail.
    if (index >= static_cast<int>(m_instrumentIds2.size()))
        return;

    Track *track = getTrack();
    if (!track)
        return;

    track->setInstrument(m_instrumentIds2[index]);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    // ??? Redundant notification.  Track::setInstrument() already does
    //     this.  It shouldn't.
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);

    // ??? The following needs to go away.

    Instrument *instrument =
            m_doc->getStudio().getInstrumentById(m_instrumentIds2[index]);
    if (!instrument)
        return;
    RosegardenMainWindow::self()->getView()->getTrackEditor()->
            getTrackButtons()->selectInstrument(
                    track, instrument);
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
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotRecordingDeviceChanged(int index)
{
    //RG_DEBUG << "slotRecordingDeviceChanged(" << index << ")";

    Track *track = getTrack();
    if (!track)
        return;

    track->setMidiInputDevice(m_recordingDeviceIds2[index]);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    // ??? Redundant notification.  Track::setMidiInputDevice() already does
    //     this.  It shouldn't.
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotRecordingChannelChanged(int index)
{
    //RG_DEBUG << "slotRecordingChannelChanged(" << index << ")";

    Track *track = getTrack();
    if (!track)
        return;

    track->setMidiInputChannel(index - 1);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    // ??? Redundant notification.  Track::setMidiInputChannel() already does
    //     this.  It shouldn't.
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotThruRoutingChanged(int index)
{
    Track *track = getTrack();
    if (!track)
        return;

    track->setThruRouting(static_cast<Track::ThruRouting>(index));
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    // ??? Redundant notification.  Track::setThruRouting() already does
    //     this.  It shouldn't.
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotInstrumentChanged(Instrument *)
{
    updateWidgets2();
}

void
TrackParameterBox::slotClefChanged(int clef)
{
    //RG_DEBUG << "slotClefChanged(" << clef << ")";

    Track *track = getTrack();
    if (!track)
        return;

    track->setClef(clef);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);

    m_preset->setEnabled(false);
}

void
TrackParameterBox::slotTransposeChanged(int index)
{
    const QString text = m_transpose->itemText(index);

    if (text.isEmpty())
        return;

    const int transpose = text.toInt();

    Track *track = getTrack();
    if (!track)
        return;

    track->setTranspose(transpose);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);

    m_preset->setEnabled(false);
}

void
TrackParameterBox::slotDocColoursChanged()
{
    // The color combobox is handled differently from the others.  Since
    // there are 420 strings of up to 25 chars in here, it would be
    // expensive to detect changes by comparing vectors of strings.

    // For now, we'll handle the document colors changed notification
    // and reload the combobox then.

    // See the comments on RosegardenDocument::docColoursChanged()
    // in RosegardenDocument.h.

    // Note that as of this writing (August 2016) there is no way
    // to modify the document colors.  See ColourConfigurationPage
    // which was probably meant to be used by DocumentConfigureDialog.

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

#if 0
// Removing this since it has never been in there.
    m_color->addItem(tr("Add New Color"));
    m_addColourPos = m_color->count() - 1;
#endif

    const Track *track = getTrack();

    if (track)
        m_color->setCurrentIndex(track->getColor());
}

void
TrackParameterBox::slotColorChanged(int index)
{
    //RG_DEBUG << "slotColorChanged(" << index << ")";

    Track *track = getTrack();
    if (!track)
        return;

    track->setColor(index);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);

#if 0
// This will never happen since the "Add Color" option is never added.
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
    Track *track = getTrack();
    if (!track)
        return;

    PitchPickerDialog dialog(
            0, track->getHighestPlayable(), tr("Highest playable note"));

    // Launch the PitchPickerDialog.  If the user clicks OK...
    if (dialog.exec() == QDialog::Accepted) {
        track->setHighestPlayable(dialog.getPitch());

        m_doc->slotDocumentModified();

        // Notify observers
        // This will trigger a call to updateWidgets2().
        Composition &comp = m_doc->getComposition();
        comp.notifyTrackChanged(track);

        m_preset->setEnabled(false);
    }
}

void
TrackParameterBox::slotLowestPressed()
{
    Track *track = getTrack();
    if (!track)
        return;

    PitchPickerDialog dialog(
            0, track->getLowestPlayable(), tr("Lowest playable note"));

    // Launch the PitchPickerDialog.  If the user clicks OK...
    if (dialog.exec() == QDialog::Accepted) {
        track->setLowestPlayable(dialog.getPitch());

        m_doc->slotDocumentModified();

        // Notify observers
        // This will trigger a call to updateWidgets2().
        Composition &comp = m_doc->getComposition();
        comp.notifyTrackChanged(track);

        m_preset->setEnabled(false);
    }
}

void
TrackParameterBox::slotLoadPressed()
{
    // Inherits style.  Centers on main window.
    //PresetHandlerDialog dialog(this);
    // Does not inherit style?  Centers on monitor #1?
    PresetHandlerDialog dialog(0);

    Track *track = getTrack();
    if (!track)
        return;

    try {
        // Launch the PresetHandlerDialog.  If the user selects OK...
        if (dialog.exec() == QDialog::Accepted) {
            // Update the Track.
            track->setPresetLabel(qstrtostr(dialog.getName()));
            track->setClef(dialog.getClef());
            track->setTranspose(dialog.getTranspose());
            track->setHighestPlayable(dialog.getHighRange());
            track->setLowestPlayable(dialog.getLowRange());

            // Enable this until it is subsequently re-disabled by the
            // user overriding the preset, calling one of the other slots
            // in the normal course.
            // ??? This is too subtle.  We should probably just clear it
            //     when modifications are made.  After all, it's no longer
            //     the selected preset.  Or add "(modified)"?  Or change
            //     color?  Or allow the user to edit it and save it to the
            //     .rg file as part of the Track.
            m_preset->setEnabled(true);

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

            m_doc->slotDocumentModified();

            // Notify observers
            // This will trigger a call to updateWidgets2().
            Composition &comp = m_doc->getComposition();
            comp.notifyTrackChanged(track);
        }
    } catch (Exception &e) {  // from PresetHandlerDialog
        // !!! This should be a more verbose error to pass along the
        //     row/column of the corruption.
        QMessageBox::warning(0, tr("Rosegarden"),
                tr("The instrument preset database is corrupt.  Check your installation."));
    }
}

void
TrackParameterBox::slotNotationSizeChanged(int index)
{
    Track *track = getTrack();
    if (!track)
        return;

    track->setStaffSize(index);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

void
TrackParameterBox::slotBracketTypeChanged(int index)
{
    Track *track = getTrack();
    if (!track)
        return;

    track->setStaffBracket(index);
    m_doc->slotDocumentModified();

    // Notify observers
    // This will trigger a call to updateWidgets2().
    Composition &comp = m_doc->getComposition();
    comp.notifyTrackChanged(track);
}

Track *
TrackParameterBox::getTrack()
{
    if (m_selectedTrackId == NO_TRACK)
        return NULL;

    if (!m_doc)
        return NULL;

    Composition &comp = m_doc->getComposition();

    // If the track is gone, bail.
    if (!comp.haveTrack(m_selectedTrackId)) {
        m_selectedTrackId = NO_TRACK;
        return NULL;
    }

    return comp.getTrackById(m_selectedTrackId);
}

void
TrackParameterBox::updatePlaybackDevice(DeviceId deviceId)
{
    const DeviceList &deviceList = *(m_doc->getStudio().getDevices());

    // Generate local device name and ID lists to compare against the members.

    std::vector<DeviceId> deviceIds;
    std::vector<std::string> deviceNames;

    // For each Device
    for (size_t deviceIndex = 0;
         deviceIndex < deviceList.size();
         ++deviceIndex) {

        const Device &device = *(deviceList[deviceIndex]);

        // ??? A Device::isInput() would be simpler.  Derivers would
        //     implement appropriately.  Then this would simplify to:
        //
        //        // If this is an input device, skip it.
        //        if (device.isInput())
        //            continue;

        const MidiDevice *midiDevice =
                dynamic_cast<const MidiDevice *>(deviceList[deviceIndex]);

        // If this is a MIDI input device, skip it.
        if (midiDevice  &&
            midiDevice->getDirection() == MidiDevice::Record)
            continue;

        deviceIds.push_back(device.getId());
        deviceNames.push_back(device.getName());
    }

    // If there has been an actual change
    if (deviceIds != m_playbackDeviceIds2  ||
        deviceNames != m_playbackDeviceNames) {

        // Update the cache.
        m_playbackDeviceIds2 = deviceIds;
        m_playbackDeviceNames = deviceNames;

        // Reload the combobox

        m_playbackDevice->clear();

        // For each playback Device, add the name to the combobox.
        // ??? If we used a QStringList, we could just call addItems().
        for (size_t deviceIndex = 0;
             deviceIndex < m_playbackDeviceNames.size();
             ++deviceIndex) {
            m_playbackDevice->addItem(
                    QObject::tr(m_playbackDeviceNames[deviceIndex].c_str()));
        }
    }

    // Find the current device in the device ID list.

    // Assume not found.
    int currentIndex = -1;

    // For each Device
    for (size_t deviceIndex = 0;
         deviceIndex < m_playbackDeviceIds2.size();
         ++deviceIndex) {
        // If this is the selected device
        if (m_playbackDeviceIds2[deviceIndex] == deviceId) {
            currentIndex = deviceIndex;
            break;
        }
    }

    // Set the index.
    m_playbackDevice->setCurrentIndex(currentIndex);
}

void
TrackParameterBox::updateInstrument(const Instrument *instrument)
{
    // As with the Device field above, this will rarely change and it is
    // expensive to clear and reload.  So, we should cache enough info to
    // detect a real change.  This would be Instrument names and IDs.

    const DeviceId deviceId = instrument->getDevice()->getId();
    const Device &device = *(m_doc->getStudio().getDevice(deviceId));

    const InstrumentList instrumentList = device.getPresentationInstruments();

    // Generate local instrument name and ID lists to compare against the
    // members.

    std::vector<InstrumentId> instrumentIds;
    std::vector<QString> instrumentNames;

    // For each instrument
    for (size_t instrumentIndex = 0;
         instrumentIndex < instrumentList.size();
         ++instrumentIndex) {
        const Instrument &loopInstrument = *(instrumentList[instrumentIndex]);

        instrumentIds.push_back(loopInstrument.getId());

        QString instrumentName(QObject::tr(loopInstrument.getName().c_str()));
        QString programName(
                QObject::tr(loopInstrument.getProgramName().c_str()));

        if (loopInstrument.getType() == Instrument::SoftSynth) {

            instrumentName.replace(QObject::tr("Synth plugin"), "");

            programName = "";

            AudioPluginInstance *plugin =
                    instrument->getPlugin(Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin)
                programName = strtoqstr(plugin->getDisplayName());
        }

        if (programName != "")
            instrumentName += " (" + programName + ")";

        // cut off the redundant eg. "General MIDI Device" that appears in the
        // combo right above here anyway
        instrumentName = instrumentName.mid(
                instrumentName.indexOf("#"), instrumentName.length());

        instrumentNames.push_back(instrumentName);
    }

    // If there has been an actual change
    if (instrumentIds != m_instrumentIds2  ||
        instrumentNames != m_instrumentNames2) {

        // Update the cache.
        m_instrumentIds2 = instrumentIds;
        m_instrumentNames2 = instrumentNames;

        // Reload the combobox

        m_instrument->clear();

        // For each instrument, add the name to the combobox.
        // ??? If we used a QStringList, we could just call addItems().
        for (size_t instrumentIndex = 0;
             instrumentIndex < m_instrumentNames2.size();
             ++instrumentIndex) {
            m_instrument->addItem(m_instrumentNames2[instrumentIndex]);
        }
    }

    // Find the current instrument in the instrument ID list.

    const InstrumentId instrumentId = instrument->getId();

    // Assume not found.
    int currentIndex = -1;

    // For each Instrument
    for (size_t instrumentIndex = 0;
         instrumentIndex < m_instrumentIds2.size();
         ++instrumentIndex) {
        // If this is the selected Instrument
        if (m_instrumentIds2[instrumentIndex] == instrumentId) {
            currentIndex = instrumentIndex;
            break;
        }
    }

    // Set the index.
    m_instrument->setCurrentIndex(currentIndex);
}

void
TrackParameterBox::updateRecordingDevice(DeviceId deviceId)
{
    // As with playback devices, the list of record devices will rarely
    // change and it is expensive to clear and reload.  Handle like the
    // others.  Cache names and IDs and only reload if a real change is
    // detected.

    const DeviceList &deviceList = *(m_doc->getStudio().getDevices());

    // Generate local recording device name and ID lists to compare against
    // the members.

    std::vector<DeviceId> recordingDeviceIds;
    std::vector<QString> recordingDeviceNames;

    recordingDeviceIds.push_back(Device::ALL_DEVICES);
    recordingDeviceNames.push_back(tr("All"));

    // For each Device
    for (size_t deviceIndex = 0;
         deviceIndex < deviceList.size();
         ++deviceIndex) {

        const Device &device = *(deviceList[deviceIndex]);

        // ??? A Device::isOutput() would be simpler.  Derivers would
        //     implement appropriately.  Then this would simplify to:
        //
        //        // If this is an output device, skip it.
        //        if (device.isOutput())
        //            continue;
        //
        //        // Add it to the recording device lists.
        //        ...

        const MidiDevice *midiDevice =
                dynamic_cast<const MidiDevice *>(deviceList[deviceIndex]);

        // If this isn't a MIDI device, try the next.
        if (!midiDevice)
            continue;

        // If this is a device capable of being recorded
        // ??? What does isRecording() really mean?
        if (midiDevice->getDirection() == MidiDevice::Record  &&
            midiDevice->isRecording()) {
            // Add it to the recording device lists.
            recordingDeviceIds.push_back(device.getId());
            recordingDeviceNames.push_back(
                    QObject::tr(midiDevice->getName().c_str()));
        }
    }

    // If there has been an actual change
    if (recordingDeviceIds != m_recordingDeviceIds2  ||
        recordingDeviceNames != m_recordingDeviceNames) {

        // Update the cache
        m_recordingDeviceIds2 = recordingDeviceIds;
        m_recordingDeviceNames = recordingDeviceNames;

        // Reload the combobox

        m_recordingDevice->clear();

        // For each playback Device, add the name to the combobox.
        // ??? If we used a QStringList, we could just call addItems().
        for (size_t deviceIndex = 0;
             deviceIndex < m_recordingDeviceNames.size();
             ++deviceIndex) {
            m_recordingDevice->addItem(m_recordingDeviceNames[deviceIndex]);
        }
    }

    // Find the current record device in the record device ID list.

    // Assume not found.
    int currentIndex = -1;

    // For each ID
    for (size_t deviceIndex = 0;
         deviceIndex < m_recordingDeviceIds2.size();
         ++deviceIndex) {
        // If this is the selected device
        if (m_recordingDeviceIds2[deviceIndex] == deviceId) {
            currentIndex = deviceIndex;
            break;
        }
    }

    // Set the index.
    m_recordingDevice->setCurrentIndex(currentIndex);
}

void
TrackParameterBox::updateWidgets2()
{
    Track *track = getTrack();
    if (!track)
        return;

    Instrument *instrument = m_doc->getStudio().getInstrumentFor(track);
    if (!instrument)
        return;

    // *** Track Label

    QString trackName = strtoqstr(track->getLabel());
    if (trackName.isEmpty())
        trackName = tr("<untitled>");
    else
        trackName.truncate(20);

    const int trackNum = track->getPosition() + 1;

    m_trackLabel->setText(tr("[ Track %1 - %2 ]").arg(trackNum).arg(trackName));

    // *** Playback parameters

    // Device
    updatePlaybackDevice(instrument->getDevice()->getId());

    // Instrument
    updateInstrument(instrument);

    // Archive
    m_archive->setChecked(track->isArchived());

    // If the current Instrument is an Audio Instrument...
    if (instrument->getInstrumentType() == Instrument::Audio) {

        // Hide irrelevant portions.

        m_recordingFiltersFrame->setVisible(false);
        m_staffExportOptionsFrame->setVisible(false);

        // In the Create segments with... frame, only the color combo is
        // useful for an Audio track.
        m_presetLabel->setVisible(false);
        m_preset->setVisible(false);
        m_load->setVisible(false);
        m_clefLabel->setVisible(false);
        m_clef->setVisible(false);
        m_transposeLabel->setVisible(false);
        m_transpose->setVisible(false);
        m_pitchLabel->setVisible(false);
        m_lowestLabel->setVisible(false);
        m_lowest->setVisible(false);
        m_highestLabel->setVisible(false);
        m_highest->setVisible(false);

    } else {  // MIDI or soft synth

        // Show everything.

        m_recordingFiltersFrame->setVisible(true);
        m_staffExportOptionsFrame->setVisible(true);

        // Create segments with... frame
        m_presetLabel->setVisible(true);
        m_preset->setVisible(true);
        m_load->setVisible(true);
        m_clefLabel->setVisible(true);
        m_clef->setVisible(true);
        m_transposeLabel->setVisible(true);
        m_transpose->setVisible(true);
        m_pitchLabel->setVisible(true);
        m_lowestLabel->setVisible(true);
        m_lowest->setVisible(true);
        m_highestLabel->setVisible(true);
        m_highest->setVisible(true);
    }

    // *** Recording filters

    // Device
    updateRecordingDevice(track->getMidiInputDevice());

    // Channel
    m_recordingChannel->setCurrentIndex((int)track->getMidiInputChannel() + 1);

    // Thru Routing
    m_thruRouting->setCurrentIndex((int)track->getThruRouting());

    // *** Staff export options

    // Notation size
    m_notationSize->setCurrentIndex(track->getStaffSize());

    // Bracket type
    m_bracketType->setCurrentIndex(track->getStaffBracket());

    // *** Create segments with

    // Preset (Label)
    m_preset->setText(strtoqstr(track->getPresetLabel()));

    // Clef
    m_clef->setCurrentIndex(track->getClef());

    // Transpose
    m_transpose->setCurrentIndex(
            m_transpose->findText(QString("%1").arg(track->getTranspose())));

    // Pitch Lowest

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    const int octaveBase = settings.value("midipitchoctave", -2).toInt() ;
    settings.endGroup();

    const bool includeOctave = false;

    const Pitch lowest(track->getLowestPlayable(), Accidentals::NoAccidental);

    // NOTE: this now uses a new, overloaded version of Pitch::getAsString()
    // that explicitly works with the key of C major, and does not allow the
    // calling code to specify how the accidentals should be written out.
    //
    // Separate the note letter from the octave to avoid undue burden on
    // translators having to retranslate the same thing but for a number
    // difference
    QString tmp = QObject::tr(lowest.getAsString(includeOctave, octaveBase).c_str(), "note name");
    tmp += tr(" %1").arg(lowest.getOctave(octaveBase));
    m_lowest->setText(tmp);

    // Pitch Highest

    const Pitch highest(track->getHighestPlayable(), Accidentals::NoAccidental);

    tmp = QObject::tr(highest.getAsString(includeOctave, octaveBase).c_str(), "note name");
    tmp += tr(" %1").arg(highest.getOctave(octaveBase));
    m_highest->setText(tmp);

    // Color
    // Note: We only update the combobox contents if there is an actual
    //       change to the document's colors.  See slotDocColoursChanged().
    m_color->setCurrentIndex(track->getColor());
}


}
