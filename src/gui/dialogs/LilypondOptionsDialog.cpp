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
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include "document/ConfigGroups.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

LilypondOptionsDialog::LilypondOptionsDialog(QWidget *parent,
        QString windowCaption,
        QString heading) :
        KDialogBase(parent, 0, true,
                    (windowCaption = "" ? i18n("LilyPond Export/Preview") : windowCaption),
                    Ok | Cancel)
{
    QVBox * vbox = makeVBoxMainWidget();

    QGroupBox *basicOptionsBox = new QGroupBox
                           (1, Horizontal,
                            (heading == "" ? i18n("Basic options") : heading), vbox);

    QGroupBox *advancedOptionsBox = new QGroupBox
                           (1, Horizontal,
                            (heading == "" ? i18n("Advanced options") : heading), vbox);

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    QGridLayout *layoutBasic = new QGridLayout(frameBasic, 4, 2, 10, 5);

    QFrame *frameAdvanced = new QFrame(advancedOptionsBox);
    QGridLayout *layoutAdvanced = new QGridLayout(frameAdvanced, 4, 2, 10, 5);

    layoutBasic->addWidget(new QLabel(
                          i18n("Compatibility level"), frameBasic), 0, 0);

    m_lilyLanguage = new KComboBox(frameBasic);
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
    m_lilyPaperSize->setCurrentItem(config->readUnsignedNumEntry("lilypapersize", 1));
    m_lilyPaperLandscape = new QCheckBox(
                              i18n("Landscape"), frameBasic);
    m_lilyPaperLandscape->setChecked(config->readBoolEntry("lilypaperlandscape", false));

    hboxPaper->addWidget( m_lilyPaperSize );
    hboxPaper->addItem( new QSpacerItem( 2, 9, QSizePolicy::Minimum, 
			    QSizePolicy::Expanding ) );
    hboxPaper->addWidget( m_lilyPaperLandscape );
    layoutBasic->addLayout(hboxPaper, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Font size"), frameBasic), 2, 0);

    m_lilyFontSize = new KComboBox(frameBasic);
    m_lilyFontSize->insertItem("11");
    m_lilyFontSize->insertItem("13");
    m_lilyFontSize->insertItem("16");
    m_lilyFontSize->insertItem("19");
    m_lilyFontSize->insertItem("20");
    m_lilyFontSize->insertItem("23");
    m_lilyFontSize->insertItem("26");
    m_lilyFontSize->setCurrentItem(config->readUnsignedNumEntry("lilyfontsize", 4));
    layoutBasic->addWidget(m_lilyFontSize, 2, 1);

    layoutBasic->addWidget(new QLabel(
                          i18n("Export content"), frameBasic), 3, 0);
                          
    m_lilyExportSelection = new KComboBox(frameBasic);
    m_lilyExportSelection->insertItem(i18n("All tracks"));
    m_lilyExportSelection->insertItem(i18n("Non-muted tracks"));
    m_lilyExportSelection->insertItem(i18n("Selected track"));
    m_lilyExportSelection->insertItem(i18n("Selected segments"));
    m_lilyExportSelection->setCurrentItem(config->readUnsignedNumEntry("lilyexportselection", 1));
    layoutBasic->addWidget(m_lilyExportSelection, 3, 1);
  
    m_lilyExportHeaders = new QCheckBox(
                              i18n("Export Document Properties as \\header block"), frameAdvanced);
    m_lilyExportHeaders->setChecked(config->readBoolEntry("lilyexportheaders", true));
    layoutAdvanced->addWidget(m_lilyExportHeaders, 0, 0);

    m_lilyExportBeams = new QCheckBox(
                            i18n("Export beamings"), frameAdvanced);
    m_lilyExportBeams->setChecked(config->readBoolEntry("lilyexportbeamings", false));
    layoutAdvanced->addWidget(m_lilyExportBeams, 0, 1);

    m_lilyExportPointAndClick = new QCheckBox(
                                    i18n("Enable \"point and click\" debugging"), frameAdvanced);
    m_lilyExportPointAndClick->setChecked(config->readBoolEntry("lilyexportpointandclick", false));
    layoutAdvanced->addWidget(m_lilyExportPointAndClick, 1, 0);

    m_lilyExportLyrics = new QCheckBox(
                             i18n("Export \\lyric blocks"), frameAdvanced);
    // default to lyric export == false because if you export the default
    // empty "- - -" lyrics, crap results ensue, and people will know if they
    // do need to export the lyrics - DMM
    m_lilyExportLyrics->setChecked(config->readBoolEntry("lilyexportlyrics", false));
    layoutAdvanced->addWidget(m_lilyExportLyrics, 1, 1);
  
    m_lilyExportStaffGroup = new QCheckBox(
                                 i18n("Add staff group bracket"), frameAdvanced);
    m_lilyExportStaffGroup->setChecked(config->readBoolEntry("lilyexportstaffgroup", false));
    layoutAdvanced->addWidget(m_lilyExportStaffGroup, 2, 0);

    m_lilyExportMidi = new QCheckBox(
                           i18n("Export \\midi block"), frameAdvanced);
    m_lilyExportMidi->setChecked(config->readBoolEntry("lilyexportmidi", false));
    layoutAdvanced->addWidget(m_lilyExportMidi, 2, 1);

    m_lilyExportStaffMerge = new QCheckBox(
                                 i18n("Merge tracks that have the same name"), frameAdvanced);
    m_lilyExportStaffMerge->setChecked(config->readBoolEntry("lilyexportstaffmerge", false));
    layoutAdvanced->addWidget(m_lilyExportStaffMerge, 3, 0);

    QHBoxLayout *hbox = new QHBoxLayout( frameAdvanced );
    m_lilyTempoMarks = new KComboBox( frameAdvanced );
    m_lilyTempoMarks->insertItem(i18n("None"));
    m_lilyTempoMarks->insertItem(i18n("First"));
    m_lilyTempoMarks->insertItem(i18n("All"));
    m_lilyTempoMarks->setCurrentItem(config->readUnsignedNumEntry("lilyexporttempomarks", 0));

    hbox->addWidget( new QLabel( 
			 i18n("Export tempo marks "), frameAdvanced ) );
    hbox->addItem( new QSpacerItem( 2, 9, QSizePolicy::Minimum, 
		       QSizePolicy::Expanding ) );
    hbox->addWidget(m_lilyTempoMarks);
    layoutAdvanced->addLayout(hbox, 3, 1);
}

void
LilypondOptionsDialog::slotOk()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    config->writeEntry("lilylanguage", m_lilyLanguage->currentItem());
    config->writeEntry("lilypapersize", m_lilyPaperSize->currentItem());
    config->writeEntry("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    config->writeEntry("lilyfontsize", m_lilyFontSize->currentItem());
    config->writeEntry("lilyexportlyrics", m_lilyExportLyrics->isChecked());
    config->writeEntry("lilyexportheader", m_lilyExportHeaders->isChecked());
    config->writeEntry("lilyexportmidi", m_lilyExportMidi->isChecked());
    config->writeEntry("lilyexporttempomarks", m_lilyTempoMarks->currentItem());
    config->writeEntry("lilyexportselection", m_lilyExportSelection->currentItem());
    config->writeEntry("lilyexportpointandclick", m_lilyExportPointAndClick->isChecked());
    config->writeEntry("lilyexportbeamings", m_lilyExportBeams->isChecked());
    config->writeEntry("lilyexportstaffgroup", m_lilyExportStaffGroup->isChecked());
    config->writeEntry("lilyexportstaffmerge", m_lilyExportStaffMerge->isChecked());

    accept();
}

}
#include "LilypondOptionsDialog.moc"
