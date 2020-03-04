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

#define RG_MODULE_STRING "[GeneralConfigurationPage]"

#include "GeneralConfigurationPage.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
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
    layout->addWidget(m_useJackTransport, row, 1, row- row+1, 2);

    ++row;

    settings.endGroup();
#endif

    settings.beginGroup(GeneralOptionsConfigGroup);

    // Skip a row.  Leave some space for the next field.
    layout->setRowMinimumHeight(row, 20);
    ++row;

    // Sequencer status
    layout->addWidget(new QLabel(tr("Sequencer status"), frame), row, 0);

    QString status(tr("Unknown"));
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
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

    // Use Thorn style
    label = new QLabel(tr("Use Thorn style"), frame);
    tipText = tr("<qt>When checked, Rosegarden will use the Thorn look and feel, otherwise default system preferences will be used the next time Rosegarden starts.</qt>");
    label->setToolTip(tipText);
    layout->addWidget(label, row, 0);

    m_Thorn = new QCheckBox;
    m_Thorn->setToolTip(tipText);
    m_Thorn->setChecked(settings.value("use_thorn_style", true).toBool());
    connect(m_Thorn, &QCheckBox::stateChanged,
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_Thorn, row, 1, 1, 3);

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
    //     group for each call?
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

    m_trackSize = new QComboBox(frame);
    m_trackSize->addItem(tr("Small"));
    m_trackSize->addItem(tr("Medium"));
    m_trackSize->addItem(tr("Large"));
    m_trackSize->setCurrentIndex(
            settings.value("track_size", 0).toInt());
    connect(m_trackSize, static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::activated),
            this, &GeneralConfigurationPage::slotModified);
    layout->addWidget(m_trackSize, row, 1, 1, 3);

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
    m_pdfViewer->addItem(tr("Okular (KDE 4.x)"));
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
    m_filePrinter->addItem(tr("Gtk-LP (GNOME)"),1);
    m_filePrinter->addItem(tr("lpr (no GUI)"),2);
    m_filePrinter->addItem(tr("lp (no GUI)"),3);
    m_filePrinter->addItem(tr("HPLIP (Qt 4)"),4);
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

#ifdef HAVE_LIBJACK
    settings.beginGroup(SequencerOptionsConfigGroup);

    bool jackTransport = m_useJackTransport->isChecked();

    // 0 -> nothing, 1 -> sync, 2 -> master
    int jackValue = jackTransport ? 1 : 0;

    settings.setValue("jacktransport", jackTransport);

    MappedEvent mEjackValue(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemJackTransport,
                            MidiByte(jackValue));
    StudioControl::sendMappedEvent(mEjackValue);

    settings.endGroup();
#endif // HAVE_LIBJACK

    // Presentation tab

    settings.beginGroup(GeneralOptionsConfigGroup);

    const bool thornChanged =
            (settings.value("use_thorn_style", true).toBool() !=
             m_Thorn->isChecked());
    //RG_DEBUG << "apply(): NB. use_thorn_style = " << settings.value("use_thorn_style", true).toBool() << ", m_Thorn->isChecked() = " << m_Thorn->isChecked();
    settings.setValue("use_thorn_style", m_Thorn->isChecked());
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
    settings.endGroup();

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

    if (thornChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("You must restart Rosegarden for the presentation change to take effect."));
    }

    if (trackSizeChanged) {
        QMessageBox::information(this, tr("Rosegarden"),
                tr("You must restart Rosegarden or open a file for the track size change to take effect."));
    }
}


}
