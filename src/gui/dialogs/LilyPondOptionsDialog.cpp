/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "LilyPondOptionsDialog.h"

#include "misc/ConfigGroups.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenDocument.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/widgets/LilyVersionAwareCheckBox.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUrl>
#include <QDesktopServices>
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
    RosegardenDocument *doc,
        QString windowCaption,
        QString /* heading */,
        bool createdFromNotationEditor):
    QDialog(parent),
    m_doc(doc),
    m_createdFromNotationEditor(createdFromNotationEditor)
{
    setModal(true);
    setWindowTitle((windowCaption = "" ? tr("LilyPond Export/Preview") : windowCaption));

    QGridLayout *metaGridLayout = new QGridLayout;

    QWidget *mainbox = new QWidget(this);
    QVBoxLayout *mainboxLayout = new QVBoxLayout;
    metaGridLayout->addWidget(mainbox, 0, 0);

    //
    // Arrange options in "Layout" and "Headers" tabs.
    //

    QTabWidget *tabWidget = new QTabWidget(mainbox);
    mainboxLayout->addWidget(tabWidget);

    QFrame *layoutFrame = new QFrame();
    tabWidget->addTab(layoutFrame, tr("Layout"));

    layoutFrame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layoutGrid = new QGridLayout;
    layoutGrid->setSpacing(5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, tr("Headers"));
//     m_headersPage->setSpacing(5);
//     m_headersPage->setMargin(5);
    
    
    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox(tr("Basic options"), layoutFrame);
    QVBoxLayout *basicOptionsBoxLayout = new QVBoxLayout;

    layoutGrid->addWidget(basicOptionsBox, 0, 0);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    frameBasic->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutBasic = new QGridLayout;
    layoutBasic->setSpacing(5);
    basicOptionsBoxLayout->addWidget(frameBasic);

    layoutBasic->addWidget(new QLabel(
                          tr("Export content"), frameBasic), 0, 0);

    m_lilyExportSelection = new QComboBox(frameBasic);
    m_lilyExportSelection->setToolTip(tr("<qt>Choose which tracks or segments to export</qt>"));
    m_lilyExportSelection->addItem(tr("All tracks"));
    m_lilyExportSelection->addItem(tr("Non-muted tracks"));
    m_lilyExportSelection->addItem(tr("Selected track"));
    m_lilyExportSelection->addItem(tr("Selected segments"));
    if (m_createdFromNotationEditor) {
        // The following item is shown only when the dialog is opened
        // from the notation editor
        QString itemName = tr("Edited segments");
        m_lilyExportSelection->addItem(itemName);
        m_editedSegmentsIndex = m_lilyExportSelection->findText(itemName);
    }

    layoutBasic->addWidget(m_lilyExportSelection, 0, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Compatibility level"), frameBasic), 1, 0);

    m_lilyLanguage = new QComboBox(frameBasic);
    m_lilyLanguage->setToolTip(tr("<qt>Set the LilyPond version you have installed. If you have a newer version of LilyPond, choose the highest version Rosegarden supports.</qt>"));

    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.6")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.8")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.10")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.12")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.14")));
    layoutBasic->addWidget(m_lilyLanguage, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Paper size"), frameBasic), 2, 0);

    QHBoxLayout *hboxPaper = new QHBoxLayout;
    m_lilyPaperSize = new QComboBox(frameBasic);
    m_lilyPaperSize->setToolTip(tr("<qt>Set the paper size</qt>"));
    m_lilyPaperSize->addItem(tr("A3"));
    m_lilyPaperSize->addItem(tr("A4"));
    m_lilyPaperSize->addItem(tr("A5"));
    m_lilyPaperSize->addItem(tr("A6"));
    m_lilyPaperSize->addItem(tr("Legal"));
    m_lilyPaperSize->addItem(tr("US Letter"));
    m_lilyPaperSize->addItem(tr("Tabloid"));
    m_lilyPaperSize->addItem(tr("do not specify"));

    m_lilyPaperLandscape = new QCheckBox(tr("Landscape"), frameBasic);
    m_lilyPaperLandscape->setToolTip(tr("<qt>If checked, your score will print in landscape orientation instead of the default portrait orientation</qt>"));

    hboxPaper->addWidget(m_lilyPaperSize);
    hboxPaper->addWidget(new QLabel(" ", frameBasic)); // fixed-size spacer
    hboxPaper->addWidget(m_lilyPaperLandscape);
    layoutBasic->addLayout(hboxPaper, 2, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Staff size"), frameBasic), 3, 0);

    m_lilyFontSize = new QComboBox(frameBasic);
    m_lilyFontSize->setToolTip(tr("<qt><p>Choose the staff size of the score.  LilyPond will scale staff contents relative to this size.</p><p>Sizes marked * may provide the best rendering quality.</p></qt>"));
    for (unsigned int i = 0; i < MAX_POINTS; i++) {
        bool recommended = false;
        int printSize = i + FONT_OFFSET;
        switch (printSize) {
            case 11:
            case 13:
            case 14:
            case 19:
            case 20:
            case 23:
            case 26: recommended = true; break;
            default: recommended = false;
        }
        QString fontString = tr("%1 pt %2").arg(printSize).arg(recommended ? tr(" *") : "");
        m_lilyFontSize->addItem(fontString);
    }
    layoutBasic->addWidget(m_lilyFontSize, 3, 1);

    //
    // LilyPond export: Notation options
    //

    QGroupBox *specificOptionsBox = new QGroupBox(tr("Advanced options"), layoutFrame);
    QVBoxLayout *specificOptionsBoxLayout = new QVBoxLayout;
    layoutGrid->addWidget(specificOptionsBox, 2, 0);

    QFrame *frameNotation = new QFrame(specificOptionsBox);
    frameNotation->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutNotation = new QGridLayout;
    layoutNotation->setSpacing(5);
    specificOptionsBoxLayout->addWidget(frameNotation);

    m_lilyTempoMarks = new QComboBox(frameNotation);
    m_lilyTempoMarks->addItem(tr("None"));
    m_lilyTempoMarks->addItem(tr("First"));
    m_lilyTempoMarks->addItem(tr("All"));

    layoutNotation->addWidget(new QLabel(
             tr("Export tempo marks "), frameNotation), 0, 0);
    layoutNotation->addWidget(m_lilyTempoMarks, 0, 1);
    m_lilyTempoMarks->setToolTip(tr("<qt>Choose how often to show tempo marks in your score</qt>"));

    layoutNotation->addWidget(new QLabel(
             tr("Export lyrics"), frameNotation), 1, 0);
    m_lilyExportLyrics = new QComboBox(frameNotation);
    m_lilyExportLyrics->addItem(tr("None"));
    m_lilyExportLyrics->addItem(tr("Left"));
    m_lilyExportLyrics->addItem(tr("Center"));
    m_lilyExportLyrics->addItem(tr("Right"));
    layoutNotation->addWidget(m_lilyExportLyrics, 1, 1);
    m_lilyExportLyrics->setToolTip(tr("<qt>Set the position of the <b>lyrics</b> in relation to the notes</qt>"));


    m_lilyExportBeams = new QCheckBox(
                            tr("Export beamings"), frameNotation);
    layoutNotation->addWidget(m_lilyExportBeams, 2, 0, 1, 2);
    m_lilyExportBeams->setToolTip(tr("<qt>If checked, Rosegarden's beamings will be exported.  Otherwise, LilyPond will calculate beams automatically.</qt>"));

    // recycle this for a new option to ignore the track brackets (so it is less
    // obnoxious to print single parts where brackets are in place)
    m_lilyExportStaffGroup = new QCheckBox(
                                 tr("Export track staff brackets"), frameNotation);
    layoutNotation->addWidget(m_lilyExportStaffGroup, 3, 0, 1, 2);
    m_lilyExportStaffGroup->setToolTip(tr("<qt>Track staff brackets are found in the <b>Track Parameters</b> box, and may be used to group staffs in various ways</qt>"));

    m_useShortNames = new LilyVersionAwareCheckBox(tr("Print short staff names"), frameNotation, LILYPOND_VERSION_2_10);
    m_useShortNames->setToolTip(tr("<qt>Useful for large, complex scores, this prints the short name every time there is a line break in the score, making it easier to follow which line belongs to which instrument across pages; requires LilyPond 2.10 or higher</qt>"));
    layoutNotation->addWidget(m_useShortNames, 4, 0, 1, 2);

    layoutGrid->setRowStretch(4, 10);

    m_lilyChordNamesMode = new QCheckBox(
                           tr("Interpret chord texts as lead sheet chord names"), frameNotation);
    layoutNotation->addWidget(m_lilyChordNamesMode, 5, 0, 1, 2);
    m_lilyChordNamesMode->setToolTip(tr("<qt><p>There is a tutorial on how to use this feature at http://www.rosegardenmusic.com/tutorials/supplemental/chordnames/index.html</p></qt>"));

    m_lilyRaggedBottom = new QCheckBox(
                           tr("Ragged bottom (systems will not be spread vertically across the page)"), frameNotation);
    layoutNotation->addWidget(m_lilyRaggedBottom, 6, 0, 1, 2);
    m_lilyRaggedBottom->setToolTip(tr("<qt><p>Useful for multi-page scores: this may prevent ugly final pages</p></qt>"));

    m_lilyMarkerMode = new QComboBox(frameNotation);
    m_lilyMarkerMode->addItem(tr("No markers"));
    m_lilyMarkerMode->addItem(tr("Rehearsal marks"));
    m_lilyMarkerMode->addItem(tr("Marker text"));

    layoutNotation->addWidget(new QLabel(
                                   tr("Export markers"), frameNotation), 7, 0);
    layoutNotation->addWidget(m_lilyMarkerMode, 7, 1);
    m_lilyMarkerMode->setToolTip(tr("<qt>Markers are found on the <b>Marker Ruler</b>.  They may be exported as text, or as rehearsal marks.</qt>"));

    m_lilyNoteLanguage = new QComboBox(frameNotation);
    // NB: language strings are specific to LilyPond so are not translated here
    m_lilyNoteLanguage->addItem("Arabic");
    m_lilyNoteLanguage->addItem("Catalan");
    m_lilyNoteLanguage->addItem("Deutsch");
    m_lilyNoteLanguage->addItem("English");
    m_lilyNoteLanguage->addItem("Espanol");
    m_lilyNoteLanguage->addItem("Italiano");
    m_lilyNoteLanguage->addItem("Nederlands");
    m_lilyNoteLanguage->addItem("Norsk");
    m_lilyNoteLanguage->addItem("Portugues");
    m_lilyNoteLanguage->addItem("Suomi");
    m_lilyNoteLanguage->addItem("Svenska");
    m_lilyNoteLanguage->addItem("Vlaams");

    layoutNotation->addWidget(new QLabel(
            tr("Notation language"), frameNotation), 8, 0);
    layoutNotation->addWidget(m_lilyNoteLanguage, 8, 1);
    m_lilyNoteLanguage->setToolTip(tr("<qt>Outputs note names and accidentals in any of LilyPond's supported languages</qt>"));

//     m_lilyRepeatMode = new QComboBox(frameNotation);
//     m_lilyRepeatMode->addItem(tr("Old mode"));
//     m_lilyRepeatMode->addItem(tr("Repeat when possible"));
//     m_lilyRepeatMode->addItem(tr("Always unfold"));

//     layoutNotation->addWidget(new QLabel(
//                                    tr("Repeat mode"), frameNotation),8, 0);
//     layoutNotation->addWidget(m_lilyRepeatMode, 8, 1);
    m_lilyRepeatMode = new QCheckBox(
                           tr("Use repeat when possible"), frameNotation);
    m_lilyRepeatMode->setToolTip(tr("<qt>How to export repeating segments: When unchecked, "
                                    "repeating segments are always unfolded.</qt>"));
    layoutNotation->addWidget(m_lilyRepeatMode, 9, 0);

    m_lilyDrawBarAtVolta = new QCheckBox(
                           tr("Draw bar line at volta"), frameNotation);
    m_lilyDrawBarAtVolta->setToolTip(tr("<qt>If checked a bar line is always "
                                    "drawn when a volta begins even if it "
                                    "begins in the middle of a bar.</qt>"));
    layoutNotation->addWidget(m_lilyDrawBarAtVolta, 10, 0);

    m_cancelAccidentals = new QCheckBox(tr("Cancel accidentals"));
    layoutNotation->addWidget(m_cancelAccidentals, 11, 0);
    m_cancelAccidentals->setToolTip(tr("<qt>When checked, natural signs are automatically printed to cancel any accidentals from previous key signatures. This cancelation behavior is separate from, and not related to how Rosegarden displays accidental cancelation in the notation editor.</qt>"));

    m_lilyExportEmptyStaves = new QCheckBox(tr("Export empty staves"));
    layoutNotation->addWidget(m_lilyExportEmptyStaves, 12, 0);
    m_lilyExportEmptyStaves->setToolTip(tr("<qt>When checked, LilyPond will print all staves, even if they are empty.  Turning this option off may reduce clutter on scores that feature long silences for some instruments.</qt>"));

    // as of lucky 13 here, I'm seeing we really need to tidy up the layout soon
    m_fingeringsInStaff = new QCheckBox(tr("Allow fingerings inside staff"));
    layoutNotation->addWidget(m_fingeringsInStaff, 13, 0);
    m_fingeringsInStaff->setToolTip(tr("<qt>When checked, LilyPond is allowed print fingerings inside the staff.  This can improve rendering in polyphonic scores with fingerings in different voices, and is on by default.</qt>"));


    basicOptionsBox->setLayout(basicOptionsBoxLayout);
    specificOptionsBox->setLayout(specificOptionsBoxLayout);

    layoutFrame->setLayout(layoutGrid);

    frameNotation->setLayout(layoutNotation);
    frameBasic->setLayout(layoutBasic);

    mainbox->setLayout(mainboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metaGridLayout->addWidget(buttonBox, 1, 0);
    metaGridLayout->setRowStretch(0, 10);

    setLayout(metaGridLayout);

    connect(m_lilyLanguage, SIGNAL(activated(int)), m_useShortNames, SLOT(slotCheckVersion(int)));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(help()));

    populateDefaultValues();
    
    // Initally enable or disable m_useShortNames according to inital setting of m_lilyLanguage
    m_useShortNames->checkVersion(m_lilyLanguage->currentIndex());

    resize(minimumSizeHint());
}

void
LilyPondOptionsDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-lilypondoptions-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
    

void
LilyPondOptionsDialog::populateDefaultValues()
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    m_lilyLanguage->setCurrentIndex(settings.value("lilylanguage", 0).toUInt());
    // See also setDefaultLilyPondVersion below
    int defaultPaperSize = 1; // A4
    if (QLocale::system().country() == QLocale::UnitedStates) {
        defaultPaperSize = 5; // Letter
    }
    m_lilyPaperSize->setCurrentIndex(settings.value("lilypapersize", defaultPaperSize).toUInt());
    m_lilyPaperLandscape->setChecked(qStrToBool(settings.value("lilypaperlandscape", "false")));
    m_lilyFontSize->setCurrentIndex(settings.value("lilyfontsize", FONT_20).toUInt());
    m_lilyRaggedBottom->setChecked(qStrToBool(settings.value("lilyraggedbottom", "false")));
    m_useShortNames->setChecked(qStrToBool(settings.value("lilyuseshortnames", "true")));
    m_lilyExportEmptyStaves->setChecked(qStrToBool(settings.value("lilyexportemptystaves", "false")));
    m_lilyChordNamesMode->setChecked(qStrToBool(settings.value("lilychordnamesmode", "false")));
    m_lilyExportLyrics->setCurrentIndex(settings.value("lilyexportlyrics", 1).toUInt());
    m_lilyTempoMarks->setCurrentIndex(settings.value("lilyexporttempomarks", 0).toUInt());
    if (m_createdFromNotationEditor) {
        // This item is the default when the dialog is opened from the notation
        // editor.
        m_lilyExportSelection->setCurrentIndex(m_editedSegmentsIndex);
    } else {
        m_lilyExportSelection->setCurrentIndex(settings.value("lilyexportselection", 1).toUInt());
    }
    m_lilyExportBeams->setChecked(settings.value("lilyexportbeamings", "false").toBool());
    m_lilyExportStaffGroup->setChecked(settings.value("lilyexportstaffbrackets", "true").toBool());
    m_lilyMarkerMode->setCurrentIndex(settings.value("lilyexportmarkermode", 0).toUInt());
    m_lilyNoteLanguage->setCurrentIndex(settings.value("lilyexportnotelanguage", 6).toUInt());
//    m_lilyRepeatMode->setCurrentIndex(settings.value("lilyrepeatmode", 0).toUInt());
    m_lilyRepeatMode->setChecked(settings.value("lilyexportrepeat", "true").toBool());
    m_lilyDrawBarAtVolta->setChecked(settings.value("lilydrawbaratvolta", "true").toBool());
    m_cancelAccidentals->setChecked(settings.value("lilycancelaccidentals", "false").toBool());
    m_fingeringsInStaff->setChecked(settings.value("lilyfingeringsinstaff", "true").toBool());

    std::cerr << "QSettings for LilyPond (populateDefaultValues):" << std::endl
              << "  lilyexportmarkermode: " << settings.value("lilyexportmarkermode").toUInt() << std::endl
              << "  lilyraggedbottom: " << (settings.value("lilyraggedbottom").toBool() ? "true" : "false") << std::endl
              << std::endl;

    settings.endGroup();
}


void
LilyPondOptionsDialog::slotApply()
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    settings.setValue("lilylanguage", m_lilyLanguage->currentIndex());
    settings.setValue("lilypapersize", m_lilyPaperSize->currentIndex());
    settings.setValue("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    settings.setValue("lilyfontsize", m_lilyFontSize->currentIndex());
    settings.setValue("lilyraggedbottom", m_lilyRaggedBottom->isChecked());
    settings.setValue("lilyuseshortnames", m_useShortNames->isChecked());
    settings.setValue("lilyexportemptystaves", m_lilyExportEmptyStaves->isChecked());
    settings.setValue("lilychordnamesmode", m_lilyChordNamesMode->isChecked());
    settings.setValue("lilyexportlyrics", m_lilyExportLyrics->currentIndex());
    settings.setValue("lilyexporttempomarks", m_lilyTempoMarks->currentIndex());
    if (m_createdFromNotationEditor and
            (m_lilyExportSelection->currentIndex() == m_editedSegmentsIndex)) {
        // The folowing value means that the dialog is opened from the
        // notation editor and that the "edited segments" option is selected.
        // In such a case, "lilyexportselection" is not needed and keeps its
        // previous value.
        // "lilyexportselection" should never have the "edited segments" option
        // value as it could latter be gotten from a place where it has no sense.
        settings.setValue("lilyexporteditedsegments", true);
    } else {
        settings.setValue("lilyexporteditedsegments", false);
        settings.setValue("lilyexportselection", m_lilyExportSelection->currentIndex());
    }
    settings.setValue("lilyexportbeamings", m_lilyExportBeams->isChecked());
    settings.setValue("lilyexportstaffbrackets", m_lilyExportStaffGroup->isChecked());
    settings.setValue("lilyexportmarkermode", m_lilyMarkerMode->currentIndex());
    settings.setValue("lilyexportnotelanguage", m_lilyNoteLanguage->currentIndex());
//    settings.setValue("lilyrepeatmode", m_lilyRepeatMode->currentIndex());
    settings.setValue("lilyexportrepeat", m_lilyRepeatMode->isChecked());
    settings.setValue("lilydrawbaratvolta", m_lilyDrawBarAtVolta->isChecked());
    settings.setValue("lilycancelaccidentals", m_cancelAccidentals->isChecked());
    settings.setValue("lilyfingeringsinstaff", m_fingeringsInStaff->isChecked());

    std::cerr << "QSettings for LilyPond (slotApply):" << std::endl
              << "  lilyexportmarkermode: " << settings.value("lilyexportmarkermode").toUInt() << std::endl
              << "  lilyraggedbottom: " << (settings.value("lilyraggedbottom").toBool() ? "true" : "false") << std::endl
              << std::endl;

    settings.endGroup();

    m_headersPage->apply();
}
 
void
LilyPondOptionsDialog::accept()
{
    slotApply();
    QDialog::accept();
}

void
LilyPondOptionsDialog::setDefaultLilyPondVersion(QString version)
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

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
