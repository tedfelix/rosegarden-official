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

#define RG_MODULE_STRING "[GeneralConfigurationPage]"

#include "GeneralConfigurationPage.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "misc/Preferences.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/studio/StudioControl.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>


namespace Rosegarden
{


GeneralConfigurationPage::GeneralConfigurationPage(QWidget *parent) :
    TabbedConfigurationPage(parent)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);

    QFrame *frame;
    QGridLayout *layout;
    QLabel *label = nullptr;


    // *** Behavior tab

    frame = new QFrame(m_tabWidget);
    addTab(frame, tr("Behavior"));
    layout = new QGridLayout(frame);
    layout->setContentsMargins(15, 25, 15, 10);
    layout->setSpacing(5);

    int row = 0;

    // Double-click opens segment in
    layout->addWidget(
            new QLabel(tr("Double-click opens segment in"), frame),
            row, 0);

    m_openSegmentsIn = new QComboBox(frame);
    m_openSegmentsIn->addItem(tr("Notation editor"));
    m_openSegmentsIn->addItem(tr("Matrix editor"));
    m_openSegmentsIn->addItem(tr("Event List editor"));
    m_openSegmentsIn->setCurrentIndex(
            settings.value("doubleclickclient", NotationView).toUInt());
    connect(m_openSegmentsIn, static_cast<void(QComboBox::*)(int)>(
                &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_openSegmentsIn, row, 1, 1, 2);

    ++row;

    // Number of count-in measures when recording
    layout->addWidget(
            new QLabel(tr("Number of count-in measures when recording"),
                       frame),
            row, 0);

    m_countIn = new QSpinBox(frame);
    m_countIn->setMinimum(0);
    m_countIn->setMaximum(10);
    m_countIn->setValue(settings.value("countinbars", 0).toUInt());
    connect(m_countIn, SIGNAL(valueChanged(int)), this, SLOT(slotModified()));
    layout->addWidget(m_countIn, row, 1, 1, 2);

    ++row;

    layout->addWidget(new QLabel(tr("Enable metronome during"),
                                 frame), row, 0);

    m_enableMetronomeDuring = new QComboBox(frame);
    connect(m_enableMetronomeDuring,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    m_enableMetronomeDuring->addItem(tr("Count-in"));
    m_enableMetronomeDuring->addItem(tr("Recording"));
    m_enableMetronomeDuring->addItem(tr("Count-in and Recording"));
    m_enableMetronomeDuring->setCurrentIndex(
            settings.value("enableMetronomeDuring", DuringBoth).toUInt());

    layout->addWidget(m_enableMetronomeDuring, row, 1, 1, 2);

    ++row;

    // Auto-save interval
    layout->addWidget(new QLabel(tr("Auto-save interval"), frame), row, 0);

    m_autoSaveInterval = new QComboBox(frame);
    m_autoSaveInterval->addItem(tr("Every 30 seconds"));
    m_autoSaveInterval->addItem(tr("Every minute"));
    m_autoSaveInterval->addItem(tr("Every five minutes"));
    m_autoSaveInterval->addItem(tr("Every half an hour"));
    m_autoSaveInterval->addItem(tr("Never"));

    bool doAutoSave = settings.value("autosave", true).toBool();
    int autoSaveInterval = settings.value("autosaveinterval", 300).toUInt();

    if (!doAutoSave  ||  autoSaveInterval == 0) {
        m_autoSaveInterval->setCurrentIndex(4);  // Never
    } else if (autoSaveInterval < 45) {
        m_autoSaveInterval->setCurrentIndex(0);  // Every 30 seconds
    } else if (autoSaveInterval < 150) {
        m_autoSaveInterval->setCurrentIndex(1);  // Every minute
    } else if (autoSaveInterval < 900) {
        m_autoSaveInterval->setCurrentIndex(2);  // Every five minutes
    } else {
        m_autoSaveInterval->setCurrentIndex(3);  // Every half an hour
    }

    connect(m_autoSaveInterval,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_autoSaveInterval, row, 1, 1, 2);

    ++row;

    // Append suffixes to segment labels
    layout->addWidget(
            new QLabel(tr("Append suffixes to segment labels"), frame),
            row, 0);

    m_appendSuffixes = new QCheckBox(frame);
    // I traditionally had these turned off, and when they reappeared, I found
    // them incredibly annoying, so I'm making false the default:
    m_appendSuffixes->setChecked(settings.value("appendlabel", false).toBool());
    connect(m_appendSuffixes, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_appendSuffixes, row, 1, 1, 2);

    ++row;

    // Use track name for new segments
    label = new QLabel(tr("Use track name for new segments"), frame);
    QString tipText = tr(
            "<qt><p>If checked, the label for new segments will always be the "
            "same as the track name.</p></qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_useTrackName = new QCheckBox(frame);
    m_useTrackName->setToolTip(tipText);
    // Leaving unchecked by default to remain compatible with earlier behaviour
    m_useTrackName->setChecked(settings.value("usetrackname", false).toBool());
    connect(m_useTrackName, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_useTrackName, row, 1, 1, 2);

    ++row;

    // Enable editing during playback
    // ??? We need to fix the crashes when editing during playback and
    //     always allow editing during playback at some point.
    label = new QLabel(tr("Enable editing during playback"), frame);
    tipText = tr(
            "<qt><p>WARNING: Editing during playback can lead to "
            "instability, crashes, and data loss.  Save frequently.</p></qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_enableEditingDuringPlayback = new QCheckBox(frame);
    m_enableEditingDuringPlayback->setToolTip(tipText);
    m_enableEditingDuringPlayback->setChecked(
            settings.value("enableEditingDuringPlayback", false).toBool());
    connect(m_enableEditingDuringPlayback, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_enableEditingDuringPlayback, row, 1, 1, 2);

    ++row;

    settings.endGroup();

    settings.beginGroup(RecentFilesConfigGroup);

    // Clean recent files list
    label = new QLabel(tr("Clean recent files list"), frame);
    tipText = tr(
            "<qt><p>Remove entries from the recent files list that no "
            "longer exist.</p></qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_cleanRecentFilesList = new QCheckBox(frame);
    m_cleanRecentFilesList->setToolTip(tipText);
    m_cleanRecentFilesList->setChecked(
            settings.value("cleanRecentFilesList", false).toBool());
    connect(m_cleanRecentFilesList, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_cleanRecentFilesList, row, 1, 1, 2);

    ++row;

    settings.endGroup();

#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

    // Use JACK transport
    layout->addWidget(new QLabel(tr("Use JACK transport"), frame), row, 0);

    // ??? Just a checkbox for now.  Originally, three settings were
    //     proposed for this:
    //       - Ignore JACK transport
    //       - Sync
    //       - Sync, and offer timebase master
    //     Not sure whether those are still relevant.  Capturing here in case.
    m_useJackTransport = new QCheckBox(frame);
    m_useJackTransport->setChecked(
            settings.value("jacktransport", false).toBool());
    connect(m_useJackTransport, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_useJackTransport, row, 1, 1, 2);

    ++row;

    settings.endGroup();

    // JACK stop at auto stop
    label = new QLabel(tr("JACK stop at auto stop"), frame);
    layout->addWidget(label, row, 0);
    tipText = tr(
            "<qt><p>Unchecking this will allow the JACK transport to roll past "
            "the end of the Rosegarden composition.  This will lead to the "
            "JACK transport being out of sync with Rosegarden's transport and "
            "could cause unexpected starting and stopping of the transport."
            "</p></qt>");
    label->setToolTip(tipText);
    m_jackStopAtAutoStop = new QCheckBox(frame);
    m_jackStopAtAutoStop->setToolTip(tipText);
    m_jackStopAtAutoStop->setChecked(Preferences::getJACKStopAtAutoStop());
    connect(m_jackStopAtAutoStop, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_jackStopAtAutoStop, row, 1, 1, 2);

    ++row;

#endif

    layout->addWidget(new QLabel(tr("Stop playback at end of last segment"),
                                 frame), row, 0);
    m_stopPlaybackAtEnd = new QCheckBox(frame);
    m_stopPlaybackAtEnd->setChecked(Preferences::getStopAtSegmentEnd());
    connect(m_stopPlaybackAtEnd, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_stopPlaybackAtEnd, row, 1, 1, 2);

    ++row;

    layout->addWidget(new QLabel(tr("Jump to loop"),
                                 frame), row, 0);
    m_jumpToLoop = new QCheckBox(frame);
    m_jumpToLoop->setChecked(Preferences::getJumpToLoop());
    connect(m_jumpToLoop, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_jumpToLoop, row, 1, 1, 2);

    ++row;

    layout->addWidget(new QLabel(tr("Advanced Looping"),
                                 frame), row, 0);
    m_advancedLooping = new QCheckBox(frame);
    m_advancedLooping->setChecked(Preferences::getAdvancedLooping());
    connect(m_advancedLooping, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_advancedLooping, row, 1, 1, 2);

    ++row;

    layout->addWidget(new QLabel(tr("Auto Channels (experimental)"),
                                 frame), row, 0);
    m_autoChannels = new QCheckBox(frame);
    m_autoChannels->setChecked(Preferences::getAutoChannels());
    connect(m_autoChannels, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_autoChannels, row, 1, 1, 2);

    ++row;

    layout->addWidget(new QLabel(tr("LV2 Plugin Support (beta)"),
                                 frame), row, 0);
    m_lv2 = new QCheckBox(frame);
    m_lv2->setChecked(Preferences::getLV2());
    connect(m_lv2, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_lv2, row, 1, 1, 2);

    ++row;

    label = new QLabel(tr("Drag with dynamic modifiers (main/matrix)"), frame);
    tipText = tr(
            "<qt><p>If set, the CTRL and ALT keys can be pressed or released "
            "while a drag is in progress to change copy/move behavior.  "
            "This applies to the main window and the matrix editor.</p></qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);
    m_dynamicDrag = new QCheckBox(frame);
    m_dynamicDrag->setToolTip(tipText);
    m_dynamicDrag->setChecked(Preferences::getDynamicDrag());
    connect(m_dynamicDrag, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);

    layout->addWidget(m_dynamicDrag, row, 1, 1, 2);

    ++row;

    settings.beginGroup(GeneralOptionsConfigGroup);

    // Skip a row.  Leave some space for the next field.
    layout->setRowMinimumHeight(row, 20);
    ++row;

    // Sequencer status
    layout->addWidget(new QLabel(tr("Sequencer status"), frame), row, 0);

    QString status(tr("Unknown"));
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    SequenceManager *mgr = doc->getSequenceManager();
    if (mgr) {
        int driverStatus = mgr->getSoundDriverStatus() & (AUDIO_OK | MIDI_OK);
        switch (driverStatus) {
        case AUDIO_OK:
            status = tr("No MIDI, audio OK");
            break;
        case MIDI_OK:
            status = tr("MIDI OK, no audio");
            break;
        case AUDIO_OK | MIDI_OK:
            status = tr("MIDI OK, audio OK");
            break;
        default:
            status = tr("No driver");
            break;
        }
    }
    layout->addWidget(new QLabel(status, frame), row, 1);

    QPushButton *detailsButton =
            new QPushButton(tr("Details..."), frame);
    QObject::connect(detailsButton, &QAbstractButton::clicked,
                     this, &GeneralConfigurationPage::slotShowStatus);
    layout->addWidget(detailsButton, row, 2, Qt::AlignRight);

    ++row;

    // Make the last row stretch to fill the rest of the space.
    layout->setRowStretch(row, 10);


    // *** Presentation tab

    frame = new QFrame(m_tabWidget);
    addTab(frame, tr("Presentation"));
    layout = new QGridLayout(frame);
    layout->setContentsMargins(15, 25, 15, 10);
    layout->setSpacing(5);

    row = 0;

    // Dark mode (Thorn style)
    label = new QLabel(tr("Theme"), frame);
    layout->addWidget(label, row, 0);

    m_theme = new QComboBox(frame);
    m_theme->addItem(tr("Native (Light)"));
    m_theme->addItem(tr("Classic (Medium)"));
    m_theme->addItem(tr("Dark"));
    m_theme->setCurrentIndex(Preferences::getTheme());
    connect(m_theme, static_cast<void(QComboBox::*)(int)>(
                &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_theme, row, 1, 1, 3);

    ++row;

    // Note name style
    layout->addWidget(new QLabel(tr("Note name style"),
                                 frame), row, 0);

    m_nameStyle = new QComboBox(frame);
    m_nameStyle->addItem(tr("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->addItem(tr("Localized (where available)"));
    m_nameStyle->setCurrentIndex(
            settings.value("notenamestyle", Local).toUInt());
    connect(m_nameStyle, static_cast<void(QComboBox::*)(int)>(
                &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_nameStyle, row, 1, 1, 3);

    ++row;

    // Show textured background on
    layout->addWidget(
            new QLabel(tr("Show textured background on"), frame), row, 0);

    m_backgroundTextures = new QCheckBox(tr("Main window"), frame);
    m_backgroundTextures->setChecked(
            settings.value("backgroundtextures", true).toBool());
    connect(m_backgroundTextures, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_backgroundTextures, row, 1);

    m_notationBackgroundTextures = new QCheckBox(tr("Notation"), frame);
    // ??? Wow this is cumbersome.  Maybe we should just prepend the
    //     group for each call?  Or, even better, use Preferences.
    settings.endGroup();
    settings.beginGroup(NotationViewConfigGroup);
    m_notationBackgroundTextures->setChecked(
            settings.value("backgroundtextures", true).toBool());
    settings.endGroup();
    settings.beginGroup(GeneralOptionsConfigGroup);
    connect(m_notationBackgroundTextures, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_notationBackgroundTextures, row, 2);

    ++row;

    // Show full path in window titles
    layout->addWidget(
            new QLabel(tr("Show full path in window titles")),
            row, 0);

    m_longTitles = new QCheckBox;
    m_longTitles->setChecked(
            settings.value("long_window_titles", false).toBool());
    connect(m_longTitles, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_longTitles, row, 1);

    ++row;

    // Track size
    layout->addWidget(new QLabel(tr("Track size"), frame), row, 0);

    // The actual values are defined in ./editors/segment/TrackEditor.cpp
    m_trackSize = new QComboBox(frame);
    m_trackSize->addItem(tr("Small"));
    m_trackSize->addItem(tr("Medium"));
    m_trackSize->addItem(tr("Large"));
    m_trackSize->addItem(tr("Extra Large"));

    m_trackSize->setCurrentIndex(
            settings.value("track_size", 0).toInt());
    connect(m_trackSize, static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    tipText = tr(
            "<qt><p>Select the track size factor. Larger sizes are useful on "
            "HDPI displays.</p></qt>");
    m_trackSize->setToolTip(tipText);
    layout->addWidget(m_trackSize, row, 1, 1, 3);

    ++row;

    // Track label width (after buttons, i.e. just the text)
    label = new QLabel(tr("Track Label width"), frame);
    tipText = tr(
            "<qt><p>Select the width of track labels. This is the text "
            "after the mute, record and solo buttons</p></qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_trackLabelWidth = new QComboBox(frame);
    m_trackLabelWidth->addItem(tr("Narrow"));
    m_trackLabelWidth->addItem(tr("Medium"));
    m_trackLabelWidth->addItem(tr("Wide"));

    m_trackLabelWidth->setCurrentIndex(
        settings.value("track_label_width", 2).toInt());
    connect(m_trackLabelWidth, static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_trackLabelWidth, row, 1, 1, 3);

    ++row;

    // Use native file dialogs
    layout->addWidget(
            new QLabel(tr("Use native file dialogs")),
            row, 0);

    m_useNativeFileDialogs = new QCheckBox;
    m_useNativeFileDialogs->setChecked(Preferences::getUseNativeFileDialogs());
    connect(m_useNativeFileDialogs, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_useNativeFileDialogs, row, 1);

    ++row;

    // Make the last row stretch to fill the rest of the space.
    layout->setRowStretch(row, 10);


    // *** External Applications tab

    frame = new QFrame(m_tabWidget);
    addTab(frame, tr("External Applications"));
    layout = new QGridLayout(frame);
    layout->setContentsMargins(15, 25, 15, 10);
    layout->setSpacing(5);

    row = 0;

    // Explanation
    label = new QLabel(tr("<qt>Rosegarden relies on external applications to provide certain features.  Each selected application must be installed and available on your path.  When choosing an application to use, please ensure that it can run from a \"run command\" box (typically <b>Alt+F2</b>) which should allow Rosegarden to make use of it when necessary.<br></qt>"),
                       frame);
    label->setWordWrap(true);
    layout->addWidget(label, row, 0, 1, 4);

    ++row;

    // PDF viewer
    label = new QLabel(tr("PDF viewer"), frame);
    tipText = tr("Used to preview generated LilyPond output");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_pdfViewer = new QComboBox(frame);
    m_pdfViewer->setToolTip(tipText);
    m_pdfViewer->addItem(tr("Okular (KDE)"));
    m_pdfViewer->addItem(tr("Evince (GNOME)"));
    m_pdfViewer->addItem(tr("Adobe Acrobat Reader (non-free)"));
    m_pdfViewer->addItem(tr("MuPDF"));
    m_pdfViewer->addItem(tr("ePDFView"));
    m_pdfViewer->addItem(tr("xdg-open (recommended)"));
    settings.endGroup();
    settings.beginGroup(ExternalApplicationsConfigGroup);
    m_pdfViewer->setCurrentIndex(settings.value("pdfviewer", xdgOpen).toUInt());
    connect(m_pdfViewer, static_cast<void(QComboBox::*)(int)>(
            &QComboBox::activated),
        this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_pdfViewer, row, 1, 1, 3);

    ++row;

    // Command-line file printing utility
    label = new QLabel(tr("Command-line file printing utility"), frame);
    tipText = tr("Used to print generated LilyPond output without previewing it");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_filePrinter = new QComboBox(frame);
    m_filePrinter->setToolTip(tipText);
    m_filePrinter->addItem(tr("Gtk-LP (GNOME)"));
    m_filePrinter->addItem(tr("lp (no GUI)"));
    m_filePrinter->addItem(tr("lpr (no GUI)"));
    m_filePrinter->addItem(tr("HPLIP (HP Printers)"));
    // now that I'm actually on KDE 4.2, I see no more KPrinter.  I'll default
    // to Lpr instead.
    m_filePrinter->setCurrentIndex(settings.value("fileprinter", Lpr).toUInt());
    connect(m_filePrinter, static_cast<void(QComboBox::*)(int)>(
            &QComboBox::activated),
        this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_filePrinter, row, 1, 1, 3);

    ++row;

    // Make the last row stretch to fill the rest of the space.
    layout->setRowStretch(row, 10);
}

void
GeneralConfigurationPage::slotShowStatus()
{
    ShowSequencerStatusDialog dialog(this);
    dialog.exec();
}

void GeneralConfigurationPage::apply()
{
    QSettings settings;

    // Behavior tab

    settings.beginGroup(GeneralOptionsConfigGroup);

    settings.setValue("doubleclickclient", m_openSegmentsIn->currentIndex());
    settings.setValue("countinbars", m_countIn->value());
    settings.setValue("enableMetronomeDuring",
                      m_enableMetronomeDuring->currentIndex());

    if (m_autoSaveInterval->currentIndex() == 4) {
        settings.setValue("autosave", false);
    } else {
        unsigned interval = 0;
        settings.setValue("autosave", true);
        if (m_autoSaveInterval->currentIndex() == 0) {
            interval = 30;
        } else if (m_autoSaveInterval->currentIndex() == 1) {
            interval = 60;
        } else if (m_autoSaveInterval->currentIndex() == 2) {
            interval = 300;
        } else {
            interval = 1800;
        }
        settings.setValue("autosaveinterval", interval);
        emit updateAutoSaveInterval(interval);
    }

    settings.setValue("appendlabel", m_appendSuffixes->isChecked());
    settings.setValue("usetrackname", m_useTrackName->isChecked());
    settings.setValue("enableEditingDuringPlayback",
            m_enableEditingDuringPlayback->isChecked());

    settings.endGroup();

    settings.beginGroup(RecentFilesConfigGroup);
    settings.setValue("cleanRecentFilesList",
            m_cleanRecentFilesList->isChecked());
    settings.endGroup();

#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

    bool jackTransport = m_useJackTransport->isChecked();

    // 0 -> nothing, 1 -> sync, 2 -> master
    int jackValue = jackTransport ? 1 : 0;

    settings.setValue("jacktransport", jackTransport);

    MappedEvent mEjackValue;
    mEjackValue.setInstrumentId(MidiInstrumentBase);  // ??? Needed?
    mEjackValue.setType(MappedEvent::SystemJackTransport);
    mEjackValue.setData1(MidiByte(jackValue));
    StudioControl::sendMappedEvent(mEjackValue);

    settings.endGroup();

    Preferences::setJACKStopAtAutoStop(m_jackStopAtAutoStop->isChecked());

#endif // HAVE_LIBJACK


    Preferences::setStopAtSegmentEnd(m_stopPlaybackAtEnd->isChecked());
    Preferences::setJumpToLoop(m_jumpToLoop->isChecked());
    Preferences::setAdvancedLooping(m_advancedLooping->isChecked());
    Preferences::setAutoChannels(m_autoChannels->isChecked());
    Preferences::setLV2(m_lv2->isChecked());
    Preferences::setDynamicDrag(m_dynamicDrag->isChecked());

    // Presentation tab

    settings.beginGroup(GeneralOptionsConfigGroup);

    const bool themeChanged =
            (Preferences::getTheme() != m_theme->currentIndex());
    Preferences::setTheme(m_theme->currentIndex());
    settings.setValue("notenamestyle", m_nameStyle->currentIndex());
    const bool mainTextureChanged =
            (settings.value("backgroundtextures", true).toBool() !=
             m_backgroundTextures->isChecked());
    settings.setValue("backgroundtextures", m_backgroundTextures->isChecked());

    settings.endGroup();

    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("backgroundtextures", m_notationBackgroundTextures->isChecked());
    settings.endGroup();

    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("long_window_titles", m_longTitles->isChecked());
    const bool trackSizeChanged =
            (settings.value("track_size", 0).toInt() !=
             m_trackSize->currentIndex());
    settings.setValue("track_size", m_trackSize->currentIndex());

    const bool trackLabelWidthChanged =
             (settings.value("track_label_width", 2).toInt() !=
              m_trackLabelWidth->currentIndex());
    settings.setValue("track_label_width", m_trackLabelWidth->currentIndex());
    settings.endGroup();

    Preferences::setUseNativeFileDialogs(m_useNativeFileDialogs->isChecked());

    // External Applications tab

    settings.beginGroup(ExternalApplicationsConfigGroup);

    settings.setValue("pdfviewer", m_pdfViewer->currentIndex());
    settings.setValue("fileprinter", m_filePrinter->currentIndex());

    settings.endGroup();

    // Restart Warnings

    if (mainTextureChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("Changes to the textured background in the main window will not take effect until you restart Rosegarden."));
    }

    if (themeChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("You must restart Rosegarden for the presentation change to take effect."));
    }

    if (trackSizeChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("You must restart Rosegarden or open a file for the track size change to take effect."));
    }

    if (trackLabelWidthChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("You must restart Rosegarden or open a file for the track label width change to take effect."));
    }

}


}
