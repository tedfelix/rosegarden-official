/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "LilypondOptionsDialog.h"
#include "document/io/LilypondExporter.h"
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "document/RosegardenGUIDoc.h"
#include "misc/Strings.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <klocale.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <iostream>


namespace Rosegarden
{

LilypondOptionsDialog::LilypondOptionsDialog(QWidget *parent,
	RosegardenGUIDoc *doc,
        QString windowCaption,
        QString heading) :
        KDialogBase(parent, 0, true,
                    (windowCaption = "" ? i18n("LilyPond Export/Preview") : windowCaption),
                    Apply | Ok | Cancel),
	m_doc(doc)
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    QVBox * mainbox = makeVBoxMainWidget();

    //
    // Arrange options in "General" and "Advanced" tabs.
    //

    QTabWidget * tabWidget = new QTabWidget(mainbox);

    QVBox * vboxGeneral = new QVBox();
    tabWidget->addTab(vboxGeneral,i18n("General options"));

    QVBox * vboxAdvanced = new QVBox();
    tabWidget->addTab(vboxAdvanced,i18n("Advanced options"));

    QVBox * vboxHeaders = new QVBox();
    tabWidget->addTab(vboxHeaders,i18n("Headers"));

    vboxGeneral->setSpacing(5);
    vboxGeneral->setMargin(5);

    vboxAdvanced->setSpacing(5);
    vboxAdvanced->setMargin(5);

    vboxHeaders->setSpacing(5);
    vboxHeaders->setMargin(5);

    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Basic options"), vboxGeneral);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    QGridLayout *layoutBasic = new QGridLayout(frameBasic, 3, 2, 10, 5);

    layoutBasic->addWidget(new QLabel(
                          i18n("Compatibility level"), frameBasic), 0, 0);

    m_lilyLanguage = new KComboBox(frameBasic);
    // See also setDefaultLilypondVersion below
    m_lilyLanguage->insertItem(i18n("LilyPond %1").arg("2.6"));
    m_lilyLanguage->insertItem(i18n("LilyPond %1").arg("2.8"));
    m_lilyLanguage->insertItem(i18n("LilyPond %1").arg("2.10"));
    m_lilyLanguage->insertItem(i18n("LilyPond %1").arg("2.12"));
    m_lilyLanguage->setCurrentItem(config->readUnsignedNumEntry("lilylanguage", 0));
    layoutBasic->addWidget(m_lilyLanguage, 0, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Paper size"), frameBasic), 1, 0);

    QHBoxLayout *hboxPaper = new QHBoxLayout( frameBasic );
    m_lilyPaperSize = new KComboBox(frameBasic);
    m_lilyPaperSize->insertItem(i18n("A3"));
    m_lilyPaperSize->insertItem(i18n("A4"));
    m_lilyPaperSize->insertItem(i18n("A5"));
    m_lilyPaperSize->insertItem(i18n("A6"));
    m_lilyPaperSize->insertItem(i18n("Legal"));
    m_lilyPaperSize->insertItem(i18n("US Letter"));
    m_lilyPaperSize->insertItem(i18n("Tabloid"));
    m_lilyPaperSize->insertItem(i18n("do not specify"));
    int defaultPaperSize = 1; // A4
    if (KGlobal::locale()->country() == "us" || 
        KGlobal::locale()->country() == "US") defaultPaperSize = 5; // Letter
    m_lilyPaperSize->setCurrentItem(config->readUnsignedNumEntry
                                    ("lilypapersize", defaultPaperSize));

    m_lilyPaperLandscape = new QCheckBox(i18n("Landscape"), frameBasic);
    m_lilyPaperLandscape->setChecked(config->readBoolEntry("lilypaperlandscape", false));

    hboxPaper->addWidget( m_lilyPaperSize );
    hboxPaper->addItem( new QSpacerItem( 2, 9, QSizePolicy::Minimum, 
			    QSizePolicy::Expanding ) );
    hboxPaper->addWidget( m_lilyPaperLandscape );
    layoutBasic->addLayout(hboxPaper, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Font size"), frameBasic), 2, 0);

    m_lilyFontSize = new KComboBox(frameBasic);
    int sizes[] = { 11, 13, 16, 19, 20, 23, 26 };
    for (int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
        m_lilyFontSize->insertItem(i18n("%1 pt").arg(sizes[i]));
    }
    m_lilyFontSize->setCurrentItem(config->readUnsignedNumEntry
                                   ("lilyfontsize", 4));
    layoutBasic->addWidget(m_lilyFontSize, 2, 1);

    //
    // LilyPond export: Staff level options
    //

    QGroupBox *staffOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Staff level options"), vboxGeneral);

    QFrame *frameStaff = new QFrame(staffOptionsBox);
    QGridLayout *layoutStaff = new QGridLayout(frameStaff, 2, 2, 10, 5);

    layoutStaff->addWidget(new QLabel(
                          i18n("Export content"), frameStaff), 0, 0);

    m_lilyExportSelection = new KComboBox(frameStaff);
    m_lilyExportSelection->insertItem(i18n("All tracks"));
    m_lilyExportSelection->insertItem(i18n("Non-muted tracks"));
    m_lilyExportSelection->insertItem(i18n("Selected track"));
    m_lilyExportSelection->insertItem(i18n("Selected segments"));
    m_lilyExportSelection->setCurrentItem(config->readUnsignedNumEntry("lilyexportselection", 1));
    layoutStaff->addWidget(m_lilyExportSelection, 0, 1);

    m_lilyExportStaffMerge = new QCheckBox(
                                 i18n("Merge tracks that have the same name"), frameStaff);
    m_lilyExportStaffMerge->setChecked(config->readBoolEntry("lilyexportstaffmerge", false));
    layoutStaff->addMultiCellWidget(m_lilyExportStaffMerge, 1, 1, 0, 1);

    //
    // LilyPond export: Notation options
    //

    QGroupBox *notationOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Notation options"), vboxGeneral);

    QFrame *frameNotation = new QFrame(notationOptionsBox);
    QGridLayout *layoutNotation = new QGridLayout(frameNotation, 4, 2, 10, 5);

    m_lilyTempoMarks = new KComboBox( frameNotation );
    m_lilyTempoMarks->insertItem(i18n("None"));
    m_lilyTempoMarks->insertItem(i18n("First"));
    m_lilyTempoMarks->insertItem(i18n("All"));
    m_lilyTempoMarks->setCurrentItem(config->readUnsignedNumEntry("lilyexporttempomarks", 0));

    layoutNotation->addWidget( new QLabel( 
			 i18n("Export tempo marks "), frameNotation), 0, 0 );
    layoutNotation->addWidget(m_lilyTempoMarks, 0, 1);
 
    m_lilyExportLyrics = new QCheckBox(
                             i18n("Export lyrics"), frameNotation);
    // default to lyric export == false because if you export the default
    // empty "- - -" lyrics, crap results ensue, and people will know if they
    // do need to export the lyrics - DMM
    // fixed, no "- - -" lyrics is generated for an empty lyrics
    // default again into lyrics - HJJ
    m_lilyExportLyrics->setChecked(config->readBoolEntry("lilyexportlyrics", true));
    layoutNotation->addMultiCellWidget(m_lilyExportLyrics, 1, 1, 0, 1);

    m_lilyExportBeams = new QCheckBox(
                            i18n("Export beamings"), frameNotation);
    m_lilyExportBeams->setChecked(config->readBoolEntry("lilyexportbeamings", false));
    layoutNotation->addMultiCellWidget(m_lilyExportBeams, 2, 2, 0, 1);

    m_lilyExportStaffGroup = new QCheckBox(
                                 i18n("Add staff group bracket"), frameNotation);
    m_lilyExportStaffGroup->setChecked(config->readBoolEntry("lilyexportstaffgroup", false));
    layoutNotation->addMultiCellWidget(m_lilyExportStaffGroup, 3, 3, 0, 1);

    //
    // LilyPond export: Extra options
    //

    QGroupBox *extraOptionsBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Extra options"), vboxAdvanced);

    QFrame *frameExtra = new QFrame(extraOptionsBox);
    QGridLayout *layoutExtra = new QGridLayout(frameExtra, 3, 2, 10, 5);

    m_lilyExportPointAndClick = new QCheckBox(
                                    i18n("Enable \"point and click\" debugging"), frameExtra);
    m_lilyExportPointAndClick->setChecked(config->readBoolEntry("lilyexportpointandclick", false));
    layoutExtra->addMultiCellWidget(m_lilyExportPointAndClick, 0, 0, 0, 1);

    m_lilyExportMidi = new QCheckBox(
                           i18n("Export \\midi block"), frameExtra);
    m_lilyExportMidi->setChecked(config->readBoolEntry("lilyexportmidi", false));
    layoutExtra->addMultiCellWidget(m_lilyExportMidi, 1, 1, 0, 1);

    m_lilyRaggedBottom = new QCheckBox(
                           i18n("Ragged bottom (systems will not be spread vertically across the page)"), frameExtra);
    m_lilyRaggedBottom->setChecked(config->readBoolEntry("lilyraggedbottom", false));
    layoutExtra->addMultiCellWidget(m_lilyRaggedBottom, 2, 2, 0, 1);

    m_lilyLyricsHAlignment = new KComboBox( frameExtra );
    m_lilyLyricsHAlignment->insertItem(i18n("Left"));
    m_lilyLyricsHAlignment->insertItem(i18n("Center"));
    m_lilyLyricsHAlignment->insertItem(i18n("Right"));
    m_lilyLyricsHAlignment->setCurrentItem(config->readUnsignedNumEntry("lilylyricshalignment", 0));

    layoutExtra->addWidget(new QLabel(
                          i18n("Lyrics alignment"), frameExtra), 3, 0);
    layoutExtra->addWidget(m_lilyLyricsHAlignment, 3, 1);

    //
    // LilyPond export: Headers
    //

    QGroupBox *headersBox = new QGroupBox
                           (1, Horizontal,
                            i18n("Printable headers"), vboxHeaders);

    QFrame *frameHeaders = new QFrame(headersBox);
    QGridLayout *layoutHeaders = new QGridLayout(frameHeaders, 10, 6, 10, 5);

    // grab user headers from metadata
    Configuration metadata = (&m_doc->getComposition())->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();
    std::vector<PropertyName> fixedKeys =
	CompositionMetadataKeys::getFixedKeys();

    for (unsigned int index = 0; index < fixedKeys.size(); index++) {
	std::string key = fixedKeys[index].getName();
	std::string header = "";
	for (unsigned int i = 0; i < propertyNames.size(); ++i) {
	    std::string property = propertyNames [i];
	    if (property == key) {
		header = metadata.get<String>(property);
	    }
	}

	unsigned int row = 0, col = 0, width = 1;
	QLineEdit *editHeader = new QLineEdit( QString( strtoqstr( header ) ), frameHeaders );
	if (key == headerDedication) {  
	    m_editDedication = editHeader;
	    row = 0; col = 2; width = 2;
	} else if (key == headerTitle) {       
	    m_editTitle = editHeader;	
	    row = 1; col = 1; width = 4;
	} else if (key == headerSubtitle) {
	    m_editSubtitle = editHeader;
	    row = 2; col = 1; width = 4;
	} else if (key == headerSubsubtitle) { 
	    m_editSubsubtitle = editHeader;
	    row = 3; col = 2; width = 2;
	} else if (key == headerPoet) {        
	    m_editPoet = editHeader;
	    row = 4; col = 0; width = 2;
	} else if (key == headerInstrument) {  
	    m_editInstrument = editHeader;
	    row = 4; col = 2; width = 2;
	} else if (key == headerComposer) {    
	    m_editComposer = editHeader;
	    row = 4; col = 4; width = 2; 
	} else if (key == headerMeter) {       
	    m_editMeter = editHeader;
	    row = 5; col = 0; width = 3; 
	} else if (key == headerArranger) {    
	    m_editArranger = editHeader;
	    row = 5; col = 3; width = 3; 
	} else if (key == headerPiece) {       
	    m_editPiece = editHeader;
	    row = 6; col = 0; width = 3; 
	} else if (key == headerOpus) {        
	    m_editOpus = editHeader;
	    row = 6; col = 3; width = 3; 
	} else if (key == headerCopyright) {   
	    m_editCopyright = editHeader;
	    row = 8; col = 1; width = 4; 
	} else if (key == headerTagline) {     
	    m_editTagline = editHeader;
	    row = 9; col = 1; width = 4; 
	}

	// editHeader->setReadOnly( true );
	editHeader->setAlignment( (col == 0 ? Qt::AlignLeft : (col >= 3 ? Qt::AlignRight : Qt::AlignCenter) ));

	layoutHeaders->addMultiCellWidget(editHeader, row, row, col, col+(width-1) );

	//
	// ToolTips
	//
	QToolTip::add( editHeader, key );
    }
    QLabel *separator = new QLabel(i18n("The composition comes here."), frameHeaders);
    separator->setAlignment( Qt::AlignCenter );
    layoutHeaders->addMultiCellWidget(separator, 7, 7, 1, 4 );
}

void
LilypondOptionsDialog::slotApply()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    config->writeEntry("lilylanguage", m_lilyLanguage->currentItem());
    config->writeEntry("lilypapersize", m_lilyPaperSize->currentItem());
    config->writeEntry("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    config->writeEntry("lilyfontsize", m_lilyFontSize->currentItem());
    config->writeEntry("lilyraggedbottom", m_lilyRaggedBottom->isChecked());
    config->writeEntry("lilyexportlyrics", m_lilyExportLyrics->isChecked());
    config->writeEntry("lilyexportmidi", m_lilyExportMidi->isChecked());
    config->writeEntry("lilyexporttempomarks", m_lilyTempoMarks->currentItem());
    config->writeEntry("lilyexportselection", m_lilyExportSelection->currentItem());
    config->writeEntry("lilyexportpointandclick", m_lilyExportPointAndClick->isChecked());
    config->writeEntry("lilyexportbeamings", m_lilyExportBeams->isChecked());
    config->writeEntry("lilyexportstaffgroup", m_lilyExportStaffGroup->isChecked());
    config->writeEntry("lilyexportstaffmerge", m_lilyExportStaffMerge->isChecked());
    config->writeEntry("lilylyricshalignment", m_lilyLyricsHAlignment->currentItem());

    //
    // Update header fields
    //

    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    metadata.set<String>(CompositionMetadataKeys::Dedication, qstrtostr(m_editDedication->text()));
    metadata.set<String>(CompositionMetadataKeys::Title, qstrtostr(m_editTitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subtitle, qstrtostr(m_editSubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subsubtitle, qstrtostr(m_editSubsubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Poet, qstrtostr(m_editPoet->text()));
    metadata.set<String>(CompositionMetadataKeys::Composer, qstrtostr(m_editComposer->text()));
    metadata.set<String>(CompositionMetadataKeys::Meter, qstrtostr(m_editMeter->text()));
    metadata.set<String>(CompositionMetadataKeys::Opus, qstrtostr(m_editOpus->text()));
    metadata.set<String>(CompositionMetadataKeys::Arranger, qstrtostr(m_editArranger->text()));
    metadata.set<String>(CompositionMetadataKeys::Instrument, qstrtostr(m_editInstrument->text()));
    metadata.set<String>(CompositionMetadataKeys::Piece, qstrtostr(m_editPiece->text()));
    metadata.set<String>(CompositionMetadataKeys::Copyright, qstrtostr(m_editCopyright->text()));
    metadata.set<String>(CompositionMetadataKeys::Tagline, qstrtostr(m_editTagline->text()));

    m_doc->slotDocumentModified();
}
 
void
LilypondOptionsDialog::slotOk()
{
    slotApply();
    accept();
}

void
LilypondOptionsDialog::setDefaultLilypondVersion(QString version)
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
        std::cerr << "\nWARNING: Unstable Lilypond version detected, selecting next language version up\n" << std::endl;
    }
    if (index >= 0) {
        config->writeEntry("lilylanguage", index);
    }
}

}
#include "LilypondOptionsDialog.moc"
