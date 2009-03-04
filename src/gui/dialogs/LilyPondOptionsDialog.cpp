/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "document/ConfigGroups.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "LilyPondOptionsDialog.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QSettings>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <QLocale>

#include <iostream>


namespace Rosegarden
{

LilyPondOptionsDialog::LilyPondOptionsDialog(QWidget *parent,
	RosegardenGUIDoc *doc,
        QString windowCaption,
        QString heading) :
        QDialog(parent),
	m_doc(doc)
{
    //setHelp("file-printing");

    setModal(true);
    setWindowTitle((windowCaption = "" ? tr("LilyPond Export/Preview") : windowCaption));

    QGridLayout *metaGridLayout = new QGridLayout;

    QWidget *mainbox = new QWidget(this);
    QVBoxLayout *mainboxLayout = new QVBoxLayout;
    metaGridLayout->addWidget(mainbox, 0, 0);

    //
    // Arrange options in "General" and "Advanced" tabs.
    //

    QTabWidget *tabWidget = new QTabWidget( mainbox );
    mainboxLayout->addWidget(tabWidget);

    QFrame *generalFrame = new QFrame();
    tabWidget->addTab(generalFrame, tr("General options"));

    generalFrame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *generalGrid = new QGridLayout;
    generalGrid->setSpacing(5);

    QFrame *advancedFrame = new QFrame();
    tabWidget->addTab(advancedFrame, tr("Advanced options"));

    advancedFrame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *advancedGrid = new QGridLayout;
    advancedGrid->setSpacing(5);
//     advancedGrid->setMargin(5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, tr("Headers"));
//     m_headersPage->setSpacing(5);
//     m_headersPage->setMargin(5);
	
	
    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox(tr("Basic options"), generalFrame);
    QVBoxLayout *basicOptionsBoxLayout = new QVBoxLayout;

    generalGrid->addWidget(basicOptionsBox, 0, 0);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    frameBasic->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutBasic = new QGridLayout;
    layoutBasic->setSpacing(5);
    basicOptionsBoxLayout->addWidget(frameBasic);

    layoutBasic->addWidget(new QLabel(
                          tr("Compatibility level"), frameBasic), 0, 0);

    m_lilyLanguage = new QComboBox(frameBasic);

    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.6")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.8")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.10")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.12")));
    layoutBasic->addWidget(m_lilyLanguage, 0, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Paper size"), frameBasic), 1, 0);

    QHBoxLayout *hboxPaper = new QHBoxLayout;
    m_lilyPaperSize = new QComboBox(frameBasic);
    m_lilyPaperSize->addItem(tr("A3"));
    m_lilyPaperSize->addItem(tr("A4"));
    m_lilyPaperSize->addItem(tr("A5"));
    m_lilyPaperSize->addItem(tr("A6"));
    m_lilyPaperSize->addItem(tr("Legal"));
    m_lilyPaperSize->addItem(tr("US Letter"));
    m_lilyPaperSize->addItem(tr("Tabloid"));
    m_lilyPaperSize->addItem(tr("do not specify"));

    m_lilyPaperLandscape = new QCheckBox(tr("Landscape"), frameBasic);

    hboxPaper->addWidget( m_lilyPaperSize );
    hboxPaper->addWidget( new QLabel( " ", frameBasic ) ); // fixed-size spacer
    hboxPaper->addWidget( m_lilyPaperLandscape );
    layoutBasic->addLayout(hboxPaper, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Font size"), frameBasic), 2, 0);

    m_lilyFontSize = new QComboBox(frameBasic);
    int sizes[] = { 11, 13, 16, 19, 20, 23, 26 };
    for (unsigned int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        m_lilyFontSize->addItem(tr("%1 pt").arg(sizes[i]));
    }
    layoutBasic->addWidget(m_lilyFontSize, 2, 1);

    //
    // LilyPond export: Staff level options
    //

    QGroupBox *staffOptionsBox = new QGroupBox( tr("Staff level options"), generalFrame);
    QVBoxLayout *staffOptionsBoxLayout = new QVBoxLayout;

    generalGrid->addWidget(staffOptionsBox, 1, 0);

    QFrame *frameStaff = new QFrame(staffOptionsBox);
    frameStaff->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutStaff = new QGridLayout;
    layoutStaff->setSpacing(5);
    staffOptionsBoxLayout->addWidget(frameStaff);

    layoutStaff->addWidget(new QLabel(
                          tr("Export content"), frameStaff), 0, 0);

    m_lilyExportSelection = new QComboBox(frameStaff);
    m_lilyExportSelection->addItem(tr("All tracks"));
    m_lilyExportSelection->addItem(tr("Non-muted tracks"));
    m_lilyExportSelection->addItem(tr("Selected track"));
    m_lilyExportSelection->addItem(tr("Selected segments"));

    layoutStaff->addWidget(m_lilyExportSelection, 0, 1);

    m_lilyExportStaffMerge = new QCheckBox(
                                 tr("Merge tracks that have the same name"), frameStaff);
    layoutStaff->addWidget(m_lilyExportStaffMerge, 1, 0, 0+1, 1- 0+1);



    //
    // LilyPond export: Notation options
    //

    QGroupBox *notationOptionsBox = new QGroupBox(tr("Notation options"), generalFrame);
    QVBoxLayout *notationOptionsBoxLayout = new QVBoxLayout;
    generalGrid->addWidget(notationOptionsBox, 2, 0);

    QFrame *frameNotation = new QFrame(notationOptionsBox);
    frameNotation->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutNotation = new QGridLayout;
    layoutNotation->setSpacing(5);
    notationOptionsBoxLayout->addWidget(frameNotation);

    m_lilyTempoMarks = new QComboBox( frameNotation );
    m_lilyTempoMarks->addItem(tr("None"));
    m_lilyTempoMarks->addItem(tr("First"));
    m_lilyTempoMarks->addItem(tr("All"));

    layoutNotation->addWidget( new QLabel( 
			 tr("Export tempo marks "), frameNotation), 0, 0 );
    layoutNotation->addWidget(m_lilyTempoMarks, 0, 1);
 
    m_lilyExportLyrics = new QCheckBox(
                             tr("Export lyrics"), frameNotation);
    layoutNotation->addWidget(m_lilyExportLyrics, 1, 0, 0+1, 1- 0+1);

    m_lilyExportBeams = new QCheckBox(
                            tr("Export beamings"), frameNotation);
    layoutNotation->addWidget(m_lilyExportBeams, 2, 0, 0+1, 1- 0+1);

    // recycle this for a new option to ignore the track brackets (so it is less
    // obnoxious to print single parts where brackets are in place)
    m_lilyExportStaffGroup = new QCheckBox(
                                 tr("Export track staff brackets"), frameNotation);
    layoutNotation->addWidget(m_lilyExportStaffGroup, 3, 0, 0+1, 1- 0+1); 

    generalGrid->setRowStretch(4, 10);

    //
    // LilyPond export: Advanced options
    //

    QGroupBox *advancedLayoutOptionsBox = new QGroupBox( tr("Layout options"), advancedFrame);
    QVBoxLayout *advancedLayoutOptionsBoxLayout = new QVBoxLayout;
    advancedGrid->addWidget(advancedLayoutOptionsBox, 0, 0);

    QFrame *frameAdvancedLayout = new QFrame(advancedLayoutOptionsBox);
    frameAdvancedLayout->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutAdvancedLayout = new QGridLayout;
    layoutAdvancedLayout->setSpacing(5);
    advancedLayoutOptionsBoxLayout->addWidget(frameAdvancedLayout);

    m_lilyLyricsHAlignment = new QComboBox( frameAdvancedLayout );
    m_lilyLyricsHAlignment->addItem(tr("Left"));
    m_lilyLyricsHAlignment->addItem(tr("Center"));
    m_lilyLyricsHAlignment->addItem(tr("Right"));

    layoutAdvancedLayout->addWidget(new QLabel(
                          tr("Lyrics alignment"), frameAdvancedLayout), 0, 0);
    layoutAdvancedLayout->addWidget(m_lilyLyricsHAlignment, 0, 1);

    m_lilyRaggedBottom = new QCheckBox(
                           tr("Ragged bottom (systems will not be spread vertically across the page)"), frameAdvancedLayout);
    layoutAdvancedLayout->addWidget(m_lilyRaggedBottom, 1, 0, 0+1, 1- 0+1);


    m_lilyChordNamesMode = new QCheckBox(
                           tr("Interpret chord texts as lead sheet chord names"), frameAdvancedLayout);
    layoutAdvancedLayout->addWidget(m_lilyChordNamesMode, 2, 0, 0+1, 1- 0+1);


    QGroupBox *miscOptionsBox = new QGroupBox(tr("Miscellaneous options"), advancedFrame);
    QVBoxLayout *miscOptionsBoxLayout = new QVBoxLayout;
    advancedGrid->addWidget(miscOptionsBox, 1, 0);

    QFrame *frameMisc = new QFrame(miscOptionsBox);
    frameMisc->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutMisc = new QGridLayout;
    layoutMisc->setSpacing(5);
    miscOptionsBoxLayout->addWidget(frameMisc);

    m_lilyExportPointAndClick = new QCheckBox(
                                    tr("Enable \"point and click\" debugging"), frameMisc);
    layoutMisc->addWidget(m_lilyExportPointAndClick, 0, 0, 0- 0+1, 1- 0+1);

    m_lilyExportMidi = new QCheckBox(
                           tr("Export \\midi block"), frameMisc);
    layoutMisc->addWidget(m_lilyExportMidi, 1, 0, 0+1, 1- 0+1);

    m_lilyMarkerMode = new QComboBox(frameMisc);
    m_lilyMarkerMode->addItem(tr("No markers"));
    m_lilyMarkerMode->addItem(tr("Rehearsal marks"));
    m_lilyMarkerMode->addItem(tr("Marker text"));

    layoutMisc->addWidget( new QLabel( 
                             tr("Export markers"), frameMisc),2, 0 );
    layoutMisc->addWidget(m_lilyMarkerMode, 2, 1);


    advancedGrid->setRowStretch(2, 10);

    basicOptionsBox->setLayout(basicOptionsBoxLayout);
    advancedLayoutOptionsBox->setLayout(advancedLayoutOptionsBoxLayout);
    miscOptionsBox->setLayout(miscOptionsBoxLayout);
    notationOptionsBox->setLayout(notationOptionsBoxLayout);
    staffOptionsBox->setLayout(staffOptionsBoxLayout);

    frameMisc->setLayout(layoutMisc);
    frameAdvancedLayout->setLayout(layoutAdvancedLayout);
    generalFrame->setLayout(generalGrid);

    frameNotation->setLayout(layoutNotation);
    frameStaff->setLayout(layoutStaff);
    frameBasic->setLayout(layoutBasic);
    advancedFrame->setLayout(advancedGrid);

    mainbox->setLayout(mainboxLayout);

    // resize(minimumSize()); // HANGS!!!

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metaGridLayout->addWidget(buttonBox, 1, 0);
    metaGridLayout->setRowStretch(0, 10);

    setLayout(metaGridLayout);


    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    populateDefaultValues();
}

void
LilyPondOptionsDialog::populateDefaultValues()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_lilyLanguage->setCurrentIndex( settings.value("lilylanguage", 0).toUInt() );
    // See also setDefaultLilyPondVersion below
    int defaultPaperSize = 1; // A4
    if (QLocale::system().country() == QLocale::UnitedStates) {
        defaultPaperSize = 5; // Letter
    }
    m_lilyPaperSize->setCurrentIndex(settings.value("lilypapersize", defaultPaperSize).toUInt());
    m_lilyPaperLandscape->setChecked( qStrToBool( settings.value("lilypaperlandscape", "false" ) ) );
    m_lilyFontSize->setCurrentIndex(settings.value("lilyfontsize", 4).toUInt());
    m_lilyRaggedBottom->setChecked( qStrToBool( settings.value("lilyraggedbottom", "false" ) ) );
    m_lilyChordNamesMode->setChecked( qStrToBool( settings.value("lilychordnamesmode", "false" ) ) );
    m_lilyExportLyrics->setChecked( qStrToBool( settings.value("lilyexportlyrics", "true" ) ) );
    m_lilyExportMidi->setChecked( qStrToBool( settings.value("lilyexportmidi", "false" ) ) );
    m_lilyTempoMarks->setCurrentIndex( settings.value("lilyexporttempomarks", 0).toUInt() );
    m_lilyExportSelection->setCurrentIndex( settings.value("lilyexportselection", 1).toUInt() );
    m_lilyExportPointAndClick->setChecked( qStrToBool( settings.value("lilyexportpointandclick", "false" ) ) );
    m_lilyExportBeams->setChecked( qStrToBool( settings.value("lilyexportbeamings", "false" ) ) );
    m_lilyExportStaffMerge->setChecked( qStrToBool( settings.value("lilyexportstaffmerge", "false" ) ) );
    m_lilyExportStaffGroup->setChecked( qStrToBool( settings.value("lilyexportstaffbrackets", "true" ) ) );
    m_lilyLyricsHAlignment->setCurrentIndex( settings.value("lilylyricshalignment", 0).toUInt() );
    m_lilyMarkerMode->setCurrentIndex( settings.value("lilyexportmarkermode", 0).toUInt() );

    settings.endGroup();
}


void
LilyPondOptionsDialog::slotApply()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("lilylanguage", m_lilyLanguage->currentIndex());
    settings.setValue("lilypapersize", m_lilyPaperSize->currentIndex());
    settings.setValue("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    settings.setValue("lilyfontsize", m_lilyFontSize->currentIndex());
    settings.setValue("lilyraggedbottom", m_lilyRaggedBottom->isChecked());
    settings.setValue("lilychordnamesmode", m_lilyChordNamesMode->isChecked());
    // default to lyric export == false because if you export the default
    // empty "- - -" lyrics, crap results ensue, and people will know if they
    // do need to export the lyrics - DMM
    // fixed, no "- - -" lyrics are generated for an empty lyrics
    // default again into lyrics - HJJ
    settings.setValue("lilyexportlyrics", m_lilyExportLyrics->isChecked());
    settings.setValue("lilyexportmidi", m_lilyExportMidi->isChecked());
    settings.setValue("lilyexporttempomarks", m_lilyTempoMarks->currentIndex());
    settings.setValue("lilyexportselection", m_lilyExportSelection->currentIndex());
    settings.setValue("lilyexportpointandclick", m_lilyExportPointAndClick->isChecked());
    settings.setValue("lilyexportbeamings", m_lilyExportBeams->isChecked());
    settings.setValue("lilyexportstaffmerge", m_lilyExportStaffMerge->isChecked());
    settings.setValue("lilyexportstaffbrackets", m_lilyExportStaffGroup->isChecked());
    settings.setValue("lilylyricshalignment", m_lilyLyricsHAlignment->currentIndex());
    settings.setValue("lilyexportmarkermode", m_lilyMarkerMode->currentIndex());
    m_headersPage->apply();

    settings.endGroup();
}
 
void
LilyPondOptionsDialog::slotOk()
{
    slotApply();
    accept();
}

void
LilyPondOptionsDialog::setDefaultLilyPondVersion(QString version)
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    int index = -1;
    bool unstable = false;
    if (version == "2.6" || version.startsWith("2.6.")) {
        index = 0;
    } else if (version == "2.7" || version.startsWith("2.7.")) {
        unstable = true;
        index = 1;
    } else if (version == "2.8" || version.startsWith("2.8.")) {
        index = 1;
    } else if (version == "2.9" || version.startsWith("2.9.")) {
        unstable = true;
        index = 2;
    } else if (version == "2.10" || version.startsWith("2.10.")) {
        index = 2;
    } else if (version == "2.11" || version.startsWith("2.11.")) {
        unstable = true;
        index = 3;
    } else if (version == "2.12" || version.startsWith("2.12.")) {
        index = 3;
    }
    if (unstable) {
        std::cerr << "\nWARNING: Unstable LilyPond version detected, selecting next language version up\n" << std::endl;
    }
    if (index >= 0) {
        settings.setValue("lilylanguage", index);
    }

    settings.endGroup();
}

}
#include "LilyPondOptionsDialog.moc"
