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


#include "LilyPondOptionsDialog.h"
#include "document/io/LilyPondExporter.h"
#include "gui/configuration/HeadersConfigurationPage.h"

#include <QLayout>
#include <kapplication.h>

#include "document/ConfigGroups.h"
#include "document/RosegardenGUIDoc.h"
#include "misc/Strings.h"
#include <QComboBox>
#include <klineedit.h>
#include <kconfig.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <kglobal.h>
#include <klocale.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
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
    setHelp("file-printing");

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

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

    generalGrid = new QGridLayout(generalFrame, 1, 1, 5, 5);

    advancedFrame = new QFrame();
    tabWidget->addTab(advancedFrame, i18n("Advanced options"));

    advancedGrid = new QGridLayout(advancedFrame, 1, 1, 5, 5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, i18n("Headers"));

    m_headersPage->setSpacing(5);
    m_headersPage->setMargin(5);

    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Basic options"), generalFrame);
    generalGrid->addWidget(basicOptionsBox, 0, 0);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    QGridLayout *layoutBasic = new QGridLayout(frameBasic, 3, 2, 10, 5);

    layoutBasic->addWidget(new QLabel(
                          i18n("Compatibility level"), frameBasic), 0, 0);

    m_lilyLanguage = new QComboBox(frameBasic);
    // See also setDefaultLilyPondVersion below
    m_lilyLanguage->addItem(i18n("LilyPond %1").arg("2.6"));
    m_lilyLanguage->addItem(i18n("LilyPond %1").arg("2.8"));
    m_lilyLanguage->addItem(i18n("LilyPond %1").arg("2.10"));
    m_lilyLanguage->addItem(i18n("LilyPond %1").arg("2.12"));
    m_lilyLanguage->setCurrentIndex(config->readUnsignedNumEntry("lilylanguage", 0));
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
    m_lilyPaperSize->setCurrentIndex(config->readUnsignedNumEntry
                                    ("lilypapersize", defaultPaperSize));

    m_lilyPaperLandscape = new QCheckBox(i18n("Landscape"), frameBasic);
    m_lilyPaperLandscape->setChecked(config->readBoolEntry("lilypaperlandscape", false));

    hboxPaper->addWidget( m_lilyPaperSize );
    hboxPaper->addWidget( new QLabel( " ", frameBasic ) ); // fixed-size spacer
    hboxPaper->addWidget( m_lilyPaperLandscape );
    layoutBasic->addLayout(hboxPaper, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Font size"), frameBasic), 2, 0);

    m_lilyFontSize = new QComboBox(frameBasic);
    int sizes[] = { 11, 13, 16, 19, 20, 23, 26 };
    for (int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        m_lilyFontSize->addItem(i18n("%1 pt").arg(sizes[i]));
    }
    m_lilyFontSize->setCurrentIndex(config->readUnsignedNumEntry
                                   ("lilyfontsize", 4));
    layoutBasic->addWidget(m_lilyFontSize, 2, 1);

    //
    // LilyPond export: Staff level options
    //

    QGroupBox *staffOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Staff level options"), generalFrame);
    generalGrid->addWidget(staffOptionsBox, 1, 0);

    QFrame *frameStaff = new QFrame(staffOptionsBox);
    QGridLayout *layoutStaff = new QGridLayout(frameStaff, 2, 2, 10, 5);

    layoutStaff->addWidget(new QLabel(
                          i18n("Export content"), frameStaff), 0, 0);

    m_lilyExportSelection = new QComboBox(frameStaff);
    m_lilyExportSelection->addItem(i18n("All tracks"));
    m_lilyExportSelection->addItem(i18n("Non-muted tracks"));
    m_lilyExportSelection->addItem(i18n("Selected track"));
    m_lilyExportSelection->addItem(i18n("Selected segments"));
    m_lilyExportSelection->setCurrentIndex(config->readUnsignedNumEntry("lilyexportselection", 1));

    layoutStaff->addWidget(m_lilyExportSelection, 0, 1);

    m_lilyExportStaffMerge = new QCheckBox(
                                 i18n("Merge tracks that have the same name"), frameStaff);
    m_lilyExportStaffMerge->setChecked(config->readBoolEntry("lilyexportstaffmerge", false));
    layoutStaff->addWidget(m_lilyExportStaffMerge, 1, 0, 0+1, 1- 1);

    //
    // LilyPond export: Notation options
    //

    QGroupBox *notationOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Notation options"), generalFrame);
    generalGrid->addWidget(notationOptionsBox, 2, 0);

    QFrame *frameNotation = new QFrame(notationOptionsBox);
    QGridLayout *layoutNotation = new QGridLayout(frameNotation, 4, 2, 10, 5);

    m_lilyTempoMarks = new QComboBox( frameNotation );
    m_lilyTempoMarks->addItem(i18n("None"));
    m_lilyTempoMarks->addItem(i18n("First"));
    m_lilyTempoMarks->addItem(i18n("All"));
    m_lilyTempoMarks->setCurrentIndex(config->readUnsignedNumEntry("lilyexporttempomarks", 0));

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
    m_lilyExportLyrics->setChecked(config->readBoolEntry("lilyexportlyrics", true));
    layoutNotation->addWidget(m_lilyExportLyrics, 1, 0, 0+1, 1- 1);

    m_lilyExportBeams = new QCheckBox(
                            i18n("Export beamings"), frameNotation);
    m_lilyExportBeams->setChecked(config->readBoolEntry("lilyexportbeamings", false));
    layoutNotation->addWidget(m_lilyExportBeams, 2, 0, 0+1, 1- 1);

    // recycle this for a new option to ignore the track brackets (so it is less
    // obnoxious to print single parts where brackets are in place)
    m_lilyExportStaffGroup = new QCheckBox(
                                 i18n("Export track staff brackets"), frameNotation);
    m_lilyExportStaffGroup->setChecked(config->readBoolEntry("lilyexportstaffbrackets", true));
    layoutNotation->addWidget(m_lilyExportStaffGroup, 3, 0, 0+1, 1- 1); 

    generalGrid->setRowStretch(4, 10);

    //
    // LilyPond export: Advanced options
    //

    QGroupBox *advancedLayoutOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Layout options"), advancedFrame);
    advancedGrid->addWidget(advancedLayoutOptionsBox, 0, 0);

    QFrame *frameAdvancedLayout = new QFrame(advancedLayoutOptionsBox);
    QGridLayout *layoutAdvancedLayout = new QGridLayout(frameAdvancedLayout, 3, 2, 10, 5);

    m_lilyLyricsHAlignment = new QComboBox( frameAdvancedLayout );
    m_lilyLyricsHAlignment->addItem(i18n("Left"));
    m_lilyLyricsHAlignment->addItem(i18n("Center"));
    m_lilyLyricsHAlignment->addItem(i18n("Right"));
    m_lilyLyricsHAlignment->setCurrentIndex(config->readUnsignedNumEntry("lilylyricshalignment", 0));

    layoutAdvancedLayout->addWidget(new QLabel(
                          i18n("Lyrics alignment"), frameAdvancedLayout), 0, 0);
    layoutAdvancedLayout->addWidget(m_lilyLyricsHAlignment, 0, 1);

    m_lilyRaggedBottom = new QCheckBox(
                           i18n("Ragged bottom (systems will not be spread vertically across the page)"), frameAdvancedLayout);
    m_lilyRaggedBottom->setChecked(config->readBoolEntry("lilyraggedbottom", false));
    layoutAdvancedLayout->addWidget(m_lilyRaggedBottom, 1, 0, 0+1, 1- 1);

    m_lilyChordNamesMode = new QCheckBox(
                           i18n("Interpret chord texts as lead sheet chord names"), frameAdvancedLayout);
    m_lilyChordNamesMode->setChecked(config->readBoolEntry("lilychordnamesmode", false));
    layoutAdvancedLayout->addWidget(m_lilyChordNamesMode, 2, 0, 0+1, 1- 1);

    QGroupBox *miscOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Miscellaneous options"), advancedFrame);
    advancedGrid->addWidget(miscOptionsBox, 1, 0);

    QFrame *frameMisc = new QFrame(miscOptionsBox);
    QGridLayout *layoutMisc = new QGridLayout(frameMisc, 2, 2, 10, 5);

    m_lilyExportPointAndClick = new QCheckBox(
                                    i18n("Enable \"point and click\" debugging"), frameMisc);
    m_lilyExportPointAndClick->setChecked(config->readBoolEntry("lilyexportpointandclick", false));
    layoutMisc->addWidget(m_lilyExportPointAndClick, 0, 0, 0- 0+1, 1- 1);

    m_lilyExportMidi = new QCheckBox(
                           i18n("Export \\midi block"), frameMisc);
    m_lilyExportMidi->setChecked(config->readBoolEntry("lilyexportmidi", false));
    layoutMisc->addWidget(m_lilyExportMidi, 1, 0, 0+1, 1- 1);

    m_lilyMarkerMode = new QComboBox(frameMisc);
    m_lilyMarkerMode->addItem(i18n("No markers"));
    m_lilyMarkerMode->addItem(i18n("Rehearsal marks"));
    m_lilyMarkerMode->addItem(i18n("Marker text"));
    m_lilyMarkerMode->setCurrentIndex(config->readUnsignedNumEntry("lilyexportmarkermode", 0));

    layoutMisc->addWidget( new QLabel( 
                             i18n("Export markers"), frameMisc),2, 0 );
    layoutMisc->addWidget(m_lilyMarkerMode, 2, 1);
    
    advancedGrid->setRowStretch(2, 10);

    resize(minimumSize());
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
LilyPondOptionsDialog::slotApply()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    config->writeEntry("lilylanguage", m_lilyLanguage->currentIndex());
    config->writeEntry("lilypapersize", m_lilyPaperSize->currentIndex());
    config->writeEntry("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    config->writeEntry("lilyfontsize", m_lilyFontSize->currentIndex());
    config->writeEntry("lilyraggedbottom", m_lilyRaggedBottom->isChecked());
    config->writeEntry("lilychordnamesmode", m_lilyChordNamesMode->isChecked());
    config->writeEntry("lilyexportlyrics", m_lilyExportLyrics->isChecked());
    config->writeEntry("lilyexportmidi", m_lilyExportMidi->isChecked());
    config->writeEntry("lilyexporttempomarks", m_lilyTempoMarks->currentIndex());
    config->writeEntry("lilyexportselection", m_lilyExportSelection->currentIndex());
    config->writeEntry("lilyexportpointandclick", m_lilyExportPointAndClick->isChecked());
    config->writeEntry("lilyexportbeamings", m_lilyExportBeams->isChecked());
    config->writeEntry("lilyexportstaffmerge", m_lilyExportStaffMerge->isChecked());
    config->writeEntry("lilyexportstaffbrackets", m_lilyExportStaffGroup->isChecked());
    config->writeEntry("lilylyricshalignment", m_lilyLyricsHAlignment->currentIndex());
    config->writeEntry("lilyexportmarkermode", m_lilyMarkerMode->currentIndex());
    m_headersPage->apply();
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
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);
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
        config->writeEntry("lilylanguage", index);
    }
}

}
#include "LilyPondOptionsDialog.moc"
