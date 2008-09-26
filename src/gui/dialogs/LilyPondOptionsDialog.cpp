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


#include "document/ConfigGroups.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "LilyPondOptionsDialog.h"
#include "misc/Strings.h"

#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>

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

#include <iostream>


namespace Rosegarden
{

LilyPondOptionsDialog::LilyPondOptionsDialog(QDialogButtonBox::QWidget *parent,
	RosegardenGUIDoc *doc,
        QString windowCaption,
        QString heading) :
        QDialog(parent),
	m_doc(doc)
{
    //setHelp("file-printing");

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    setModal(true);
    setWindowTitle((windowCaption = "" ? i18n("LilyPond Export/Preview") : windowCaption));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *mainbox = new QWidget(this);
    QVBoxLayout *mainboxLayout = new QVBoxLayout;
    metagrid->addWidget(mainbox, 0, 0);


    //
    // Arrange options in "General" and "Advanced" tabs.
    //

    QTabWidget *tabWidget = new QTabWidget( mainbox );
    mainboxLayout->addWidget(tabWidget);
    mainbox->setLayout(mainboxLayout);

    QFrame *generalFrame;
    QFrame *advancedFrame;
    QGridLayout *generalGrid;
    QGridLayout *advancedGrid;

    generalFrame = new QFrame();
    tabWidget->addTab(generalFrame, i18n("General options"));

    generalFrame->setContentsMargins(5, 5, 5, 5);
    generalGrid = new QGridLayout(generalFrame);
    generalGrid->setSpacing(5);

    advancedFrame = new QFrame();
    tabWidget->addTab(advancedFrame, i18n("Advanced options"));

    advancedFrame->setContentsMargins(5, 5, 5, 5);
    advancedGrid = new QGridLayout(advancedFrame);
    advancedGrid->setSpacing(5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, i18n("Headers"));

//     m_headersPage->setSpacing(5);
//     m_headersPage->setMargin(5);
	advancedGrid->setSpacing(5);
	advancedGrid->setMargin(5);
	
	
    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox(i18n("Basic options"), generalFrame);
    QVBoxLayout *basicOptionsBoxLayout = new QVBoxLayout;

    generalGrid->addWidget(basicOptionsBox, 0, 0);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    frameBasic->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutBasic = new QGridLayout(frameBasic);
    layoutBasic->setSpacing(5);
    basicOptionsBoxLayout->addWidget(frameBasic);

    layoutBasic->addWidget(new QLabel(
                          i18n("Compatibility level"), frameBasic), 0, 0);

    m_lilyLanguage = new QComboBox(frameBasic);
    // See also setDefaultLilyPondVersion below

    m_lilyLanguage->addItem(i18n("LilyPond %1", i18n("2.6")));
    m_lilyLanguage->addItem(i18n("LilyPond %1", i18n("2.8")));
    m_lilyLanguage->addItem(i18n("LilyPond %1", i18n("2.10")));
    m_lilyLanguage->addItem(i18n("LilyPond %1", i18n("2.12")));
    m_lilyLanguage->setCurrentIndex( settings.value("lilylanguage", 0).toUInt() );
    layoutBasic->addWidget(m_lilyLanguage, 0, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Paper size"), frameBasic), 1, 0);

    QHBoxLayout *hboxPaper = new QHBoxLayout( frameBasic );
    m_lilyPaperSize = new QComboBox(frameBasic);
    m_lilyPaperSize->addItem(i18n("A3"));
    m_lilyPaperSize->addItem(i18n("A4"));
    m_lilyPaperSize->addItem(i18n("A5"));
    m_lilyPaperSize->addItem(i18n("A6"));
    m_lilyPaperSize->addItem(i18n("Legal"));
    m_lilyPaperSize->addItem(i18n("US Letter"));
    m_lilyPaperSize->addItem(i18n("Tabloid"));
    m_lilyPaperSize->addItem(i18n("do not specify"));
    int defaultPaperSize = 1; // A4
    if (KGlobal::locale()->country() == "us" || 
        KGlobal::locale()->country() == "US") defaultPaperSize = 5; // Letter
    m_lilyPaperSize->setCurrentIndex(settings.value("lilypapersize",
            defaultPaperSize).toUInt());

    m_lilyPaperLandscape = new QCheckBox(i18n("Landscape"), frameBasic);
    m_lilyPaperLandscape->setChecked( qStrToBool( settings.value("lilypaperlandscape", "false" ) ) );

    hboxPaper->addWidget( m_lilyPaperSize );
    hboxPaper->addWidget( new QLabel( " ", frameBasic ) ); // fixed-size spacer
    hboxPaper->addWidget( m_lilyPaperLandscape );
    layoutBasic->addLayout(hboxPaper, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Font size"), frameBasic), 2, 0);

    m_lilyFontSize = new QComboBox(frameBasic);
    int sizes[] = { 11, 13, 16, 19, 20, 23, 26 };
    for (unsigned int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        m_lilyFontSize->addItem(i18n("%1 pt", sizes[i]));
    }
    m_lilyFontSize->setCurrentIndex(settings.value("lilyfontsize", 4).toUInt());
    layoutBasic->addWidget(m_lilyFontSize, 2, 1);

    frameBasic->setLayout(layoutBasic);

    //
    // LilyPond export: Staff level options
    //

    QGroupBox *staffOptionsBox = new QGroupBox( i18n("Staff level options"), generalFrame);
    QVBoxLayout *staffOptionsBoxLayout = new QVBoxLayout;

    generalGrid->addWidget(staffOptionsBox, 1, 0);

    QFrame *frameStaff = new QFrame(staffOptionsBox);
    frameStaff->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutStaff = new QGridLayout(frameStaff);
    layoutStaff->setSpacing(5);
    staffOptionsBoxLayout->addWidget(frameStaff);

    layoutStaff->addWidget(new QLabel(
                          i18n("Export content"), frameStaff), 0, 0);

    m_lilyExportSelection = new QComboBox(frameStaff);
    m_lilyExportSelection->addItem(i18n("All tracks"));
    m_lilyExportSelection->addItem(i18n("Non-muted tracks"));
    m_lilyExportSelection->addItem(i18n("Selected track"));
    m_lilyExportSelection->addItem(i18n("Selected segments"));
    m_lilyExportSelection->setCurrentIndex( settings.value("lilyexportselection", 1).toUInt() );

    layoutStaff->addWidget(m_lilyExportSelection, 0, 1);

    m_lilyExportStaffMerge = new QCheckBox(
                                 i18n("Merge tracks that have the same name"), frameStaff);
    m_lilyExportStaffMerge->setChecked( qStrToBool( settings.value("lilyexportstaffmerge", "false" ) ) );
    layoutStaff->addWidget(m_lilyExportStaffMerge, 1, 0, 0+1, 1- 1);

    frameStaff->setLayout(layoutStaff);

    //
    // LilyPond export: Notation options
    //

    QGroupBox *notationOptionsBox = new QGroupBox(i18n("Notation options"), generalFrame);
    QVBoxLayout *notationOptionsBoxLayout = new QVBoxLayout;
    generalGrid->addWidget(notationOptionsBox, 2, 0);

    QFrame *frameNotation = new QFrame(notationOptionsBox);
    frameNotation->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutNotation = new QGridLayout(frameNotation);
    layoutNotation->setSpacing(5);
    notationOptionsBoxLayout->addWidget(frameNotation);

    m_lilyTempoMarks = new QComboBox( frameNotation );
    m_lilyTempoMarks->addItem(i18n("None"));
    m_lilyTempoMarks->addItem(i18n("First"));
    m_lilyTempoMarks->addItem(i18n("All"));
    m_lilyTempoMarks->setCurrentIndex( settings.value("lilyexporttempomarks", 0).toUInt() );

    layoutNotation->addWidget( new QLabel( 
			 i18n("Export tempo marks "), frameNotation), 0, 0 );
    layoutNotation->addWidget(m_lilyTempoMarks, 0, 1);
 
    m_lilyExportLyrics = new QCheckBox(
                             i18n("Export lyrics"), frameNotation);
    // default to lyric export == false because if you export the default
    // empty "- - -" lyrics, crap results ensue, and people will know if they
    // do need to export the lyrics - DMM
    // fixed, no "- - -" lyrics are generated for an empty lyrics
    // default again into lyrics - HJJ
    m_lilyExportLyrics->setChecked( qStrToBool( settings.value("lilyexportlyrics", "true" ) ) );
    layoutNotation->addWidget(m_lilyExportLyrics, 1, 0, 0+1, 1- 1);

    m_lilyExportBeams = new QCheckBox(
                            i18n("Export beamings"), frameNotation);
    m_lilyExportBeams->setChecked( qStrToBool( settings.value("lilyexportbeamings", "false" ) ) );
    layoutNotation->addWidget(m_lilyExportBeams, 2, 0, 0+1, 1- 1);

    // recycle this for a new option to ignore the track brackets (so it is less
    // obnoxious to print single parts where brackets are in place)
    m_lilyExportStaffGroup = new QCheckBox(
                                 i18n("Export track staff brackets"), frameNotation);
    m_lilyExportStaffGroup->setChecked( qStrToBool( settings.value("lilyexportstaffbrackets", "true" ) ) );
    layoutNotation->addWidget(m_lilyExportStaffGroup, 3, 0, 0+1, 1- 1); 

    frameNotation->setLayout(layoutNotation);

    generalGrid->setRowStretch(4, 10);

    //
    // LilyPond export: Advanced options
    //

    QGroupBox *advancedLayoutOptionsBox = new QGroupBox( i18n("Layout options"), advancedFrame);
    QVBoxLayout *advancedLayoutOptionsBoxLayout = new QVBoxLayout;
    advancedGrid->addWidget(advancedLayoutOptionsBox, 0, 0);

    QFrame *frameAdvancedLayout = new QFrame(advancedLayoutOptionsBox);
    frameAdvancedLayout->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutAdvancedLayout = new QGridLayout(frameAdvancedLayout);
    layoutAdvancedLayout->setSpacing(5);
    advancedLayoutOptionsBoxLayout->addWidget(frameAdvancedLayout);

    m_lilyLyricsHAlignment = new QComboBox( frameAdvancedLayout );
    m_lilyLyricsHAlignment->addItem(i18n("Left"));
    m_lilyLyricsHAlignment->addItem(i18n("Center"));
    m_lilyLyricsHAlignment->addItem(i18n("Right"));
    m_lilyLyricsHAlignment->setCurrentIndex( settings.value("lilylyricshalignment", 0).toUInt() );

    layoutAdvancedLayout->addWidget(new QLabel(
                          i18n("Lyrics alignment"), frameAdvancedLayout), 0, 0);
    layoutAdvancedLayout->addWidget(m_lilyLyricsHAlignment, 0, 1);

    m_lilyRaggedBottom = new QCheckBox(
                           i18n("Ragged bottom (systems will not be spread vertically across the page)"), frameAdvancedLayout);
    m_lilyRaggedBottom->setChecked( qStrToBool( settings.value("lilyraggedbottom", "false" ) ) );
    layoutAdvancedLayout->addWidget(m_lilyRaggedBottom, 1, 0, 0+1, 1- 1);

    m_lilyChordNamesMode = new QCheckBox(
                           i18n("Interpret chord texts as lead sheet chord names"), frameAdvancedLayout);
    m_lilyChordNamesMode->setChecked( qStrToBool( settings.value("lilychordnamesmode", "false" ) ) );
    layoutAdvancedLayout->addWidget(m_lilyChordNamesMode, 2, 0, 0+1, 1- 1);

    frameAdvancedLayout->setLayout(layoutAdvancedLayout);

    QGroupBox *miscOptionsBox = new QGroupBox(i18n("Miscellaneous options"), advancedFrame);
    QVBoxLayout *miscOptionsBoxLayout = new QVBoxLayout;
    advancedGrid->addWidget(miscOptionsBox, 1, 0);

    QFrame *frameMisc = new QFrame(miscOptionsBox);
    frameMisc->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutMisc = new QGridLayout(frameMisc);
    layoutMisc->setSpacing(5);
    miscOptionsBoxLayout->addWidget(frameMisc);

    m_lilyExportPointAndClick = new QCheckBox(
                                    i18n("Enable \"point and click\" debugging"), frameMisc);
    m_lilyExportPointAndClick->setChecked( qStrToBool( settings.value("lilyexportpointandclick", "false" ) ) );
    layoutMisc->addWidget(m_lilyExportPointAndClick, 0, 0, 0- 0+1, 1- 1);

    m_lilyExportMidi = new QCheckBox(
                           i18n("Export \\midi block"), frameMisc);
    m_lilyExportMidi->setChecked( qStrToBool( settings.value("lilyexportmidi", "false" ) ) );
    layoutMisc->addWidget(m_lilyExportMidi, 1, 0, 0+1, 1- 1);

    m_lilyMarkerMode = new QComboBox(frameMisc);
    m_lilyMarkerMode->addItem(i18n("No markers"));
    m_lilyMarkerMode->addItem(i18n("Rehearsal marks"));
    m_lilyMarkerMode->addItem(i18n("Marker text"));
    m_lilyMarkerMode->setCurrentIndex( settings.value("lilyexportmarkermode", 0).toUInt() );

    layoutMisc->addWidget( new QLabel( 
                             i18n("Export markers"), frameMisc),2, 0 );
    layoutMisc->addWidget(m_lilyMarkerMode, 2, 1);

    frameMisc->setLayout(layoutMisc);

    advancedGrid->setRowStretch(2, 10);

    basicOptionsBox->setLayout(basicOptionsBoxLayout);
    advancedLayoutOptionsBox->setLayout(advancedLayoutOptionsBoxLayout);
    miscOptionsBox->setLayout(miscOptionsBoxLayout);
    notationOptionsBox->setLayout(notationOptionsBoxLayout);
    staffOptionsBox->setLayout(staffOptionsBoxLayout);

    generalFrame->setLayout(generalGrid);
    advancedFrame->setLayout(advancedGrid);

    resize(minimumSize());
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

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
