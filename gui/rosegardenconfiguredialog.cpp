// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
 * Code borrowed from KDE KMail : configuredialog*.h
 * Copyright (C) 2000 The KMail Development Team
 */

#include <qspinbox.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <kfiledialog.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenconfigurationpage.h"
#include "notationhlayout.h"
#include "notestyle.h"
#include "rosegardenguidoc.h"
#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"
#include "rosedebug.h"
#include "notepixmapfactory.h"
#include "matrixtool.h"
#include "notationtool.h"
#include "segmenttool.h"
#include "editcommands.h"

namespace Rosegarden
{

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(doc, parent, name)
{
    init();  
}

TabbedConfigurationPage::TabbedConfigurationPage(KConfig *cfg,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(cfg, parent, name)
{
    init();
}

void TabbedConfigurationPage::init()
{
    QVBoxLayout *vlay = new QVBoxLayout(this, 0, KDialog::spacingHint());
    m_tabWidget = new QTabWidget(this);
    vlay->addWidget(m_tabWidget);
}


void TabbedConfigurationPage::addTab(QWidget *tab, const QString &title)
{
  m_tabWidget->addTab(tab, title);
}

//------------------------------------------------------------

GeneralConfigurationPage::GeneralConfigurationPage(KConfig *cfg,
                                                   QWidget *parent, const char *name)
    : TabbedConfigurationPage(cfg, parent, name),
      m_client(0),
      m_countIn(0),
      m_midiPitchOffset(0),
      m_externalAudioEditorPath(0),
      m_selectorGreedyMode(0),
      m_nameStyle(0)

{
//     Rosegarden::Composition &comp = doc->getComposition();
//     Rosegarden::Configuration &config = doc->getConfiguration();
    m_cfg->setGroup("General Options");

    //
    // "General" tab
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          6, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel(i18n("Default editor (for double-click on segment)"),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Number of count-in bars when recording"),
                                 frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Note name style"),
                                 frame), 2, 0);
    layout->addWidget(new QLabel(i18n("MIDI pitch to string offset"),
                                 frame), 3, 0);
    layout->addWidget(new QLabel(i18n("Selector greedy mode"),
                                 frame), 4, 0);
    layout->addWidget(new QLabel(i18n("Auto-save interval (in minutes)"),
                                 frame), 5, 0);

    m_client = new QComboBox(frame);
    m_client->insertItem(i18n("Notation"));
    m_client->insertItem(i18n("Matrix"));
    m_client->insertItem(i18n("Event List"));
    m_client->setCurrentItem(m_cfg->readUnsignedNumEntry("doubleclickclient", NotationView));
    
    layout->addWidget(m_client, 0, 1);

    m_countIn = new QSpinBox(frame);
    m_countIn->setValue(m_cfg->readUnsignedNumEntry("countinbars", 2));
    m_countIn->setMaxValue(10);
    m_countIn->setMinValue(0);

    layout->addWidget(m_countIn, 1, 1);

    m_nameStyle = new QComboBox(frame);
    m_nameStyle->insertItem(i18n("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->insertItem(i18n("Localised (where available, else UK)"));
    m_nameStyle->setCurrentItem(m_cfg->readUnsignedNumEntry("notenamestyle", Local));
    layout->addWidget(m_nameStyle, 2, 1);

    m_midiPitchOffset = new QSpinBox(frame);
    m_midiPitchOffset->setValue(m_cfg->readUnsignedNumEntry("midipitchoffset", 4));
    m_midiPitchOffset->setMaxValue(10);
    m_midiPitchOffset->setMinValue(0);

    layout->addWidget(m_midiPitchOffset, 3, 1);

    m_selectorGreedyMode = new QCheckBox(frame);
    layout->addWidget(m_selectorGreedyMode, 4, 1);

    m_selectorGreedyMode->setChecked(m_cfg->readBoolEntry("selectorgreedymode",
                                                          true));

    m_autosaveInterval = new QSpinBox(frame);
    m_autosaveInterval->setValue(m_cfg->readUnsignedNumEntry("autosaveinterval", 5));
    m_autosaveInterval->setMaxValue(120);
    m_autosaveInterval->setMinValue(1);
    layout->addWidget(m_autosaveInterval, 5, 1);

    addTab(frame, i18n("General"));

    //
    // External editor tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             1, 3, // nbrow, nbcol
                             10, 5);

    layout->addWidget(new QLabel(i18n("External audio editor"), frame),
                      0, 0);

    QString externalAudioEditor = m_cfg->readEntry("externalaudioeditor",
                                                   "audacity");

    m_externalAudioEditorPath = new QLineEdit(externalAudioEditor, frame);
    m_externalAudioEditorPath->setMinimumWidth(200);
    layout->addWidget(m_externalAudioEditorPath, 0, 1);
    
    QPushButton *changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(changePathButton, 0, 2);
    connect(changePathButton, SIGNAL(released()), SLOT(slotFileDialog()));


    addTab(frame, i18n("External Editors"));

}

void
GeneralConfigurationPage::slotFileDialog()
{
    KFileDialog *fileDialog = new KFileDialog(getExternalAudioEditor(),
                                              QString::null,
                                              this, "file dialog", true);
    connect(fileDialog, SIGNAL(fileSelected(const QString&)),
            SLOT(slotFileSelected(const QString&)));

    connect(fileDialog, SIGNAL(destroyed()),
            SLOT(slotDirectoryDialogClosed()));

    if (fileDialog->exec() == QDialog::Accepted)
    {
        m_externalAudioEditorPath->setText(fileDialog->selectedFile());
    }
    delete fileDialog;
}

void GeneralConfigurationPage::apply()
{
//     Rosegarden::Composition &comp = m_doc->getComposition();
//     Rosegarden::Configuration &config = m_doc->getConfiguration();
//     comp.setCountInBars(countIn);
//     config.setDoubleClickClient(
//                                 (Rosegarden::Configuration::DoubleClickClient)client);
    m_cfg->setGroup("General Options");

    int countIn = getCountInSpin();
    m_cfg->writeEntry("countinbars", countIn);

    int client = getDblClickClient();
    m_cfg->writeEntry("doubleclickclient", client);

    int offset = getMIDIPitch2StringOffset();
    m_cfg->writeEntry("midipitchoffset", offset);

    int namestyle = getNoteNameStyle();
    m_cfg->writeEntry("notenamestyle", namestyle);

    bool greedyMode = m_selectorGreedyMode->isChecked();
    MatrixSelector::setGreedyMode(greedyMode);
    NotationSelector::setGreedyMode(greedyMode);
    SegmentSelector::setGreedyMode(greedyMode);
    m_cfg->writeEntry("selectorgreedymode", m_selectorGreedyMode->isChecked());

    unsigned int autosaveInterval = m_autosaveInterval->value();
    m_cfg->writeEntry("autosaveInterval", autosaveInterval);

    QString externalAudioEditor = getExternalAudioEditor();

    QFileInfo info(externalAudioEditor);
    if (!info.exists() || !info.isExecutable())
    {
        /*
        QString errorStr =
             i18n("External editor \"") + externalAudioEditor +
             ("\" not found or not executable.\nReverting to last editor.");
        KMessageBox::error(this, errorStr);

        // revert on gui
        m_externalAudioEditorPath->
            setText(m_cfg->readEntry("externalaudioeditor", ""));
            */
        m_cfg->writeEntry("externalaudioeditor", "");
    }
    else
        m_cfg->writeEntry("externalaudioeditor", externalAudioEditor);


}


NotationConfigurationPage::NotationConfigurationPage(KConfig *cfg,
                                                     QWidget *parent,
                                                     const char *name) :
    TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup("Notation Options");

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          4, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget
	(new QLabel(i18n("Notation font"), frame), 0, 0);
    layout->addWidget
	(new QLabel(i18n("Font size for single-staff views"), frame),
	 2, 0);
    layout->addWidget
	(new QLabel(i18n("Font size for multi-staff views"), frame),
	 3, 0);

    QFrame *subFrame = new QFrame(frame);
    QGridLayout *subLayout = new QGridLayout(subFrame,
					     4, 2, // nbrow, nbcol
					     12, 2);

    subLayout->addWidget(new QLabel(i18n("Origin:"), subFrame), 0, 0);
    subLayout->addWidget(new QLabel(i18n("Copyright:"), subFrame), 1, 0);
    subLayout->addWidget(new QLabel(i18n("Mapped by:"), subFrame), 2, 0);
    subLayout->addWidget(new QLabel(i18n("Type:"), subFrame), 3, 0);
    m_fontOriginLabel = new QLabel(subFrame);
    m_fontCopyrightLabel = new QLabel(subFrame);
    m_fontCopyrightLabel->setAlignment(Qt::WordBreak);
    m_fontCopyrightLabel->setMinimumWidth(300);
    m_fontMappedByLabel = new QLabel(subFrame);
    m_fontTypeLabel = new QLabel(subFrame);
    subLayout->addWidget(m_fontOriginLabel, 0, 1);
    subLayout->addWidget(m_fontCopyrightLabel, 1, 1);
    subLayout->addWidget(m_fontMappedByLabel, 2, 1);
    subLayout->addWidget(m_fontTypeLabel, 3, 1);

    layout->addMultiCellWidget(subFrame,
                               1, 1,
                               0, 1);

    m_font = new QComboBox(frame);
    m_font->setEditable(false);

    QString defaultFont = m_cfg->readEntry
	("notefont", strtoqstr(NotePixmapFactory::getDefaultFont()));

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {
	QString s(strtoqstr(*i));
        m_font->insertItem(s);
	if (s == defaultFont) m_font->setCurrentItem(m_font->count() - 1);
    }
    QObject::connect(m_font, SIGNAL(activated(const QString &)),
		     this, SLOT(slotFontComboChanged(const QString &)));
    layout->addWidget(m_font, 0, 1);

    m_singleStaffSize = new QComboBox(frame);
    m_singleStaffSize->setEditable(false);

    m_multiStaffSize = new QComboBox(frame);
    m_multiStaffSize->setEditable(false);

    slotFontComboChanged(defaultFont);

    layout->addWidget(m_singleStaffSize, 2, 1);
    layout->addWidget(m_multiStaffSize, 3, 1);

    addTab(frame, i18n("Font"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 5, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Default layout mode"), frame), 0, 0);

    m_layoutMode = new QComboBox(frame);
    m_layoutMode->setEditable(false);
    m_layoutMode->insertItem(i18n("Linear layout"));
    m_layoutMode->insertItem(i18n("Page layout"));
    int defaultLayoutMode = m_cfg->readNumEntry("layoutmode", 0);
    if (defaultLayoutMode >= 0 && defaultLayoutMode <= 1) {
	m_layoutMode->setCurrentItem(defaultLayoutMode);
    }
    layout->addWidget(m_layoutMode, 0, 1);
    
    layout->addWidget(new QLabel(i18n("Default spacing"), frame), 1, 0);

    m_spacing = new QComboBox(frame);
    m_spacing->setEditable(false);

    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    int defaultSpacing = m_cfg->readNumEntry("spacing", 100);

    for (std::vector<int>::iterator i = s.begin(); i != s.end(); ++i) {

	QString text("%1 %");
	if (*i == 100) text = "%1 % (normal)";
        m_spacing->insertItem(text.arg(*i));

	if (*i == defaultSpacing) {
	    m_spacing->setCurrentItem(m_spacing->count() - 1);
	}
    }

    layout->addWidget(m_spacing, 1, 1);

    layout->addWidget(new QLabel(i18n("Default smoothing"), frame), 2, 0);
    
    m_smoothing = new QComboBox(frame);
    m_smoothing->setEditable(false);

    Note::Type defaultSmoothing =
	m_cfg->readNumEntry("smoothing", Note::Shortest);

    NotePixmapFactory npf;
    for (Note::Type type = Note::Shortest; type <= Note::Longest; ++type) {
	Note note(type);
	QPixmap pmap = NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
	    (strtoqstr((std::string("menu-") + note.getReferenceName()))));
	m_smoothing->insertItem(pmap, strtoqstr(note.getEnglishName()));
	if (defaultSmoothing == type) {
	    m_smoothing->setCurrentItem(m_smoothing->count() - 1);
	}
    }

    layout->addWidget(m_smoothing, 2, 1);
    
    m_colourQuantize = new QCheckBox
	(i18n("Show smoothed notes in a different colour"), frame);
    bool defaultColourQuantize = m_cfg->readBoolEntry("colourquantize", true);
    m_colourQuantize->setChecked(defaultColourQuantize);
    layout->addWidget(m_colourQuantize, 3, 1);
    
    m_showUnknowns = new QCheckBox
	(i18n("Show non-notation events as question marks"), frame);
    bool defaultShowUnknowns = m_cfg->readBoolEntry("showunknowns", true);
    m_showUnknowns->setChecked(defaultShowUnknowns);
    layout->addWidget(m_showUnknowns, 4, 1);
    
    addTab(frame, i18n("Layout"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 5, 2, 10, 5);

    layout->addWidget
	(new QLabel(i18n("Default note style for new notes"), frame), 0, 0);

    m_noteStyle = new QComboBox(frame);
    m_noteStyle->setEditable(false);

    QString defaultStyle =
	m_cfg->readEntry("style", strtoqstr(NoteStyleFactory::DefaultStyle));
    std::vector<NoteStyleName> styles
	(NoteStyleFactory::getAvailableStyleNames());
    
    for (std::vector<NoteStyleName>::iterator i = styles.begin();
	 i != styles.end(); ++i) {

	QString styleQName(strtoqstr(*i));
	m_noteStyle->insertItem(styleQName);
	if (styleQName == defaultStyle) {
	    m_noteStyle->setCurrentItem(m_noteStyle->count() - 1);
	}
    }

    layout->addWidget(m_noteStyle, 0, 1);

    layout->addWidget
	(new QLabel(i18n("When inserting notes..."), frame), 1, 0);

    int defaultInsertType = m_cfg->readNumEntry("inserttype", 0);

    m_insertType = new QComboBox(frame);
    m_insertType->setEditable(false);
    m_insertType->insertItem
	(i18n("Split notes into ties to make durations match"));
    m_insertType->insertItem(i18n("Ignore existing durations"));
    m_insertType->setCurrentItem(defaultInsertType);

    layout->addWidget(m_insertType, 1, 1);

    bool autoBeam = m_cfg->readBoolEntry("autobeam", true);

    m_autoBeam = new QCheckBox
	(i18n("Auto-beam on insert when appropriate"), frame);
    m_autoBeam->setChecked(autoBeam);
    layout->addWidget(m_autoBeam, 2, 1);

    bool collapse = m_cfg->readBoolEntry("collapse", false);

    m_collapseRests = new QCheckBox
	(i18n("Collapse rests after erase"), frame);
    m_collapseRests->setChecked(collapse);
    layout->addWidget(m_collapseRests, 3, 1);


    layout->addWidget
	(new QLabel(i18n("Default paste type"), frame), 4, 0);

    m_pasteType = new QComboBox(frame);
    m_pasteType->setEditable(false);

    unsigned int defaultPasteType = m_cfg->readUnsignedNumEntry
	("pastetype", PasteEventsCommand::Restricted);

    PasteEventsCommand::PasteTypeMap pasteTypes =
	PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
	 i != pasteTypes.end(); ++i) {
	m_pasteType->insertItem(i18n(strtoqstr(i->second)));
    }

    m_pasteType->setCurrentItem(defaultPasteType);
    layout->addWidget(m_pasteType, 4, 1);


    addTab(frame, i18n("Editing"));
}

void
NotationConfigurationPage::slotFontComboChanged(const QString &font)
{
    std::string fontStr = qstrtostr(font);

    populateSizeCombo(m_singleStaffSize, fontStr,
		      m_cfg->readUnsignedNumEntry
		      ("singlestaffnotesize",
		       NotePixmapFactory::getDefaultSize(fontStr)));
    populateSizeCombo(m_multiStaffSize, fontStr,
		      m_cfg->readUnsignedNumEntry
		      ("multistaffnotesize",
		       NotePixmapFactory::getDefaultSize(fontStr)));

    try {
	NoteFont noteFont(fontStr);
	const NoteFontMap &map(noteFont.getNoteFontMap());
	m_fontOriginLabel->setText(strtoqstr(map.getOrigin()));
	m_fontCopyrightLabel->setText(strtoqstr(map.getCopyright()));
	m_fontMappedByLabel->setText(strtoqstr(map.getMappedBy()));
	if (map.isSmooth()) {
	    m_fontTypeLabel->setText(strtoqstr(map.getType() + " (smooth)"));
	} else {
	    m_fontTypeLabel->setText(strtoqstr(map.getType() + " (jaggy)"));
	}
    } catch (NoteFont::BadFont f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
    } catch (NoteFontMap::MappingFileReadFailed f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
    }
}

void
NotationConfigurationPage::populateSizeCombo(QComboBox *combo,
					     std::string font,
					     int defaultSize)
{
    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(font);
    combo->clear();
    
    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {
        combo->insertItem(QString("%1").arg(*i));
	if (*i == defaultSize) combo->setCurrentItem(combo->count() - 1);
    }
}

void
NotationConfigurationPage::apply()
{
    m_cfg->setGroup("Notation Options");

    m_cfg->writeEntry("notefont", m_font->currentText());
    m_cfg->writeEntry("singlestaffnotesize",
		      m_singleStaffSize->currentText().toUInt());
    m_cfg->writeEntry("multistaffnotesize",
		      m_multiStaffSize->currentText().toUInt());

    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    m_cfg->writeEntry("spacing", s[m_spacing->currentItem()]);

    m_cfg->writeEntry("layoutmode", m_layoutMode->currentItem());
    m_cfg->writeEntry("smoothing",
		      m_smoothing->currentItem() + Note::Shortest);
    m_cfg->writeEntry("colourquantize", m_colourQuantize->isChecked());
    m_cfg->writeEntry("showunknowns", m_showUnknowns->isChecked());
    m_cfg->writeEntry("style", m_noteStyle->currentText());
    m_cfg->writeEntry("inserttype", m_insertType->currentItem());
    m_cfg->writeEntry("autobeam", m_autoBeam->isChecked());
    m_cfg->writeEntry("collapse", m_collapseRests->isChecked());
    m_cfg->writeEntry("pastetype", m_pasteType->currentItem());
}


MatrixConfigurationPage::MatrixConfigurationPage(KConfig *cfg,
                                                 QWidget *parent,
                                                 const char *name) :
    TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup("Matrix Options");

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          4, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel("Nothing here yet", frame), 0, 0);

    addTab(frame, i18n("General"));
}

void MatrixConfigurationPage::apply()
{
    m_cfg->setGroup("Matrix Options");
}


LatencyConfigurationPage::LatencyConfigurationPage(RosegardenGUIDoc *doc,
                                                   KConfig *cfg,
                                                   QWidget *parent,
                                                   const char *name)
    : TabbedConfigurationPage(cfg, parent, name),
      m_readAhead(0),
      m_playback(0),
      m_doc(doc)
{
//     Rosegarden::Configuration &config = doc->getConfiguration();
    m_cfg->setGroup("Latency Options");

    QFrame *frame = new QFrame(m_tabWidget, "general latency");
    QGridLayout *layout = new QGridLayout(frame, 6, 3,
                                          10, 5);

    layout->addMultiCellWidget(new QLabel(i18n("Higher latency improves playback quality on slower systems but reduces\noverall sequencer response.  Modifications to these values take effect\nfrom the next time playback or recording begins."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("Read ahead (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Playback (in ms)"), frame), 3, 0);

    m_readAhead = new QSlider(Horizontal, frame);

    m_readAhead->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 2, 1);

    m_readAhead->setMaxValue(500);
    layout->addWidget(new QLabel("300", frame), 2, 3);

    int readAheadValue = m_cfg->readLongNumEntry("readaheadusec", 80000) / 1000;
    m_readAhead->setValue(readAheadValue);
    m_readAhead->setTickmarks(QSlider::Below);
    layout->addWidget(m_readAhead, 2, 2);

    m_readAheadLabel = new QLabel(QString("%1").arg(readAheadValue), frame);
    layout->addWidget(m_readAheadLabel, 1, 2, Qt::AlignHCenter);

    connect(m_readAhead,
            SIGNAL(valueChanged(int)),
            SLOT(slotReadAheadChanged(int)));

    m_playback = new QSlider(Horizontal, frame);

    m_playback->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 4, 1);

    m_playback->setMaxValue(500);
    layout->addWidget(new QLabel("300", frame), 4, 3);

    int playbackValue = m_cfg->readLongNumEntry("playbacklatencyusec", 120000)
                        / 1000;
    m_playback->setValue(playbackValue);
    m_playback->setTickmarks(QSlider::Below);
    layout->addWidget(m_playback, 4, 2);

    m_playbackLabel = new QLabel(QString("%1").arg(playbackValue), frame);
    layout->addWidget(m_playbackLabel, 3, 2, Qt::AlignHCenter);
    connect(m_playback,
            SIGNAL(valueChanged(int)),
            SLOT(slotPlaybackChanged(int)));

    addTab(frame, i18n("Latency"));

#ifdef HAVE_LIBJACK
    frame = new QFrame(m_tabWidget, i18n("JACK latency"));
    layout = new QGridLayout(frame, 6, 5, 10, 10);

    layout->addMultiCellWidget(new QLabel(i18n("Use the \"Fetch JACK latencies\" button to discover the latency values set at\nthe sequencer.  It's recommended that you use the returned values but it's also\npossible to override them manually using the sliders.  Note that if you change\nyour JACK server parameters you should always fetch the latency values again.\nThe latency values will be stored by Rosegarden for use next time."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("JACK playback latency (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("JACK record latency (in ms)"), frame), 3, 0);

    m_fetchLatencyValues = new QPushButton(i18n("Fetch JACK latencies"),
                                                frame);

    layout->addWidget(m_fetchLatencyValues, 1, 3);

    connect(m_fetchLatencyValues, SIGNAL(released()),
            SLOT(slotFetchLatencyValues()));

    int jackPlaybackValue = (m_cfg->readLongNumEntry(
                                 "jackplaybacklatencyusec", 0)/1000) +
                            (m_cfg->readLongNumEntry(
                                 "jackplaybacklatencysec", 0) * 1000);

    m_jackPlayback = new QSlider(Horizontal, frame);
    m_jackPlayback->setTickmarks(QSlider::Below);
    layout->addMultiCellWidget(m_jackPlayback, 3, 3, 2, 3);

    QLabel *jackPlaybackLabel = new QLabel(QString("%1").arg(jackPlaybackValue),
                                           frame);
    layout->addWidget(jackPlaybackLabel, 2, 2, Qt::AlignHCenter);
    connect(m_jackPlayback, SIGNAL(valueChanged(int)),
            jackPlaybackLabel, SLOT(setNum(int)));

    m_jackPlayback->setMinValue(0);
    layout->addWidget(new QLabel("0", frame), 3, 1, Qt::AlignRight);

    m_jackPlayback->setMaxValue(500);
    layout->addWidget(new QLabel("500", frame), 3, 4, Qt::AlignLeft);

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = (m_cfg->readLongNumEntry(
                              "jackrecordlatencyusec", 0)/1000) +
                          (m_cfg->readLongNumEntry(
                              "jackrecordlatencysec", 0) * 1000);

    m_jackRecord = new QSlider(Horizontal, frame);
    m_jackRecord->setTickmarks(QSlider::Below);
    layout->addMultiCellWidget(m_jackRecord, 5, 5, 2, 3);

    QLabel *jackRecordLabel = new QLabel(QString("%1").arg(jackRecordValue),
                                           frame);
    layout->addWidget(jackRecordLabel, 4, 2, Qt::AlignHCenter);
    connect(m_jackRecord, SIGNAL(valueChanged(int)),
            jackRecordLabel, SLOT(setNum(int)));

    m_jackRecord->setMinValue(0);
    layout->addWidget(new QLabel("0", frame), 5, 1, Qt::AlignRight);

    m_jackRecord->setMaxValue(500);
    m_jackRecord->setValue(jackRecordValue);
    layout->addWidget(new QLabel("500", frame), 5, 4, Qt::AlignLeft);

    addTab(frame, i18n("JACK Latency"));
#endif  // HAVE_LIBJACK

}

void LatencyConfigurationPage::apply()
{
    m_cfg->setGroup("Latency Options");

//     Rosegarden::Configuration &config = m_doc->getConfiguration();
//     config.setReadAhead((RealTime(0, (readAhead * 1000))));
//     config.setPlaybackLatency((RealTime(0, (playback * 1000))));

    int readAhead = getReadAheadValue();
    m_cfg->writeEntry("readaheadusec", readAhead * 1000);
    m_cfg->writeEntry("readaheadsec", 0L);
    
    int playback = getPlaybackValue();
    m_cfg->writeEntry("playbacklatencyusec", playback * 1000);
    m_cfg->writeEntry("playbacklatencysec", 0L);

#ifdef HAVE_LIBJACK

    int jackPlayback = getJACKPlaybackValue();
    m_cfg->writeEntry("jackplaybacklatencysec", jackPlayback / 1000);
    m_cfg->writeEntry("jackplaybacklatencyusec", jackPlayback * 1000);

    int jackRecord = getJACKRecordValue();
    m_cfg->writeEntry("jackrecordlatencysec", jackRecord / 1000);
    m_cfg->writeEntry("jackrecordlatencyusec", jackRecord * 1000);

#endif  // HAVE_LIBJACK

}

// Fetch values from sequencer and apply them
//
void LatencyConfigurationPage::slotFetchLatencyValues()
{
    int jackPlaybackValue = m_doc->getAudioPlayLatency().usec / 1000 +
                            m_doc->getAudioPlayLatency().sec * 1000;

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = m_doc->getAudioRecordLatency().usec / 1000 +
                          m_doc->getAudioRecordLatency().sec * 1000;
    m_jackRecord->setValue(jackRecordValue);
}

// With these two slots just make sure that read ahead isn't 
// greater than the total playback latency.
//
void LatencyConfigurationPage::slotReadAheadChanged(int value)
{
    m_readAheadLabel->setNum(value);

    if (value > m_playback->value())
    {
        m_playback->setValue(value);
        m_playbackLabel->setNum(value);
    }
}

void LatencyConfigurationPage::slotPlaybackChanged(int value)
{
    m_playbackLabel->setNum(value);

    if (value < m_readAhead->value())
    {
        m_readAhead->setValue(value);
        m_readAheadLabel->setNum(value);
    }

}

// -------------------  SequencerConfigurationPage ---------------------
//

SequencerConfigurationPage::SequencerConfigurationPage(
                                                   RosegardenGUIDoc * /*doc*/,
                                                   KConfig *cfg,
                                                   QWidget *parent,
                                                   const char *name):
    TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup("Sequencer Options");
    QFrame *frame = new QFrame(m_tabWidget);

    QGridLayout *layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addMultiCellWidget(new QLabel(i18n("Sequencer command line options"), frame),
                               1, 1,
                               0, 1);

    layout->addWidget(new QLabel(i18n("Options:"), frame), 2, 0);
    m_sequencerArguments = new QLineEdit("", frame);
    layout->addWidget(m_sequencerArguments, 2, 1);

    layout->addWidget(new QLabel(i18n("Any change made here will come into effect the next time you start Rosegarden."),
                               frame),
                               3, 1);

    // Get the options
    //
    QString commandLine = m_cfg->readEntry("commandlineoptions", "");
    m_sequencerArguments->setText(commandLine);

    addTab(frame, i18n("Sequencer Options"));
}

void
SequencerConfigurationPage::apply()
{
    m_cfg->setGroup("Sequencer Options");
    m_cfg->writeEntry("commandlineoptions", m_sequencerArguments->text());
}

// ---


DocumentMetaConfigurationPage::DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
							     QWidget *parent,
							     const char *name) :
    TabbedConfigurationPage(doc, parent, name),
    m_copyright(0)
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 4, 2,
                                          10, 5);

    layout->addWidget(new QLabel(i18n("Filename:"), frame), 0, 0);
    layout->addWidget(new QLabel(doc->getTitle(), frame), 0, 1);

    layout->addWidget(new QLabel(i18n("Duration:"), frame), 1, 0);
    Rosegarden::timeT d = doc->getComposition().getDuration();
    Rosegarden::RealTime rtd = doc->getComposition().getElapsedRealTime(d);
    layout->addWidget
	(new QLabel(i18n("%1 minutes %2.%3%4 seconds (%5 units, %6 bars)")
		    .arg(rtd.sec / 60).arg(rtd.sec % 60)
		    .arg(rtd.usec / 100000).arg((rtd.usec / 10000) % 10)
		    .arg(d).arg(doc->getComposition().getBarNumber(d) + 1),
		    frame), 1, 1);

    layout->addWidget(new QLabel(i18n("Segments:"), frame), 2, 0);
    layout->addWidget(new QLabel(QString("%1 on %2 tracks")
				 .arg(doc->getComposition().getNbSegments())
				 .arg(doc->getComposition().getNbTracks()),
				 frame), 2, 1);

    layout->addWidget(new QLabel(i18n("Copyright"), frame), 3, 0);
    m_copyright = new QLineEdit
	(strtoqstr(doc->getComposition().getCopyrightNote()), frame);
    m_copyright->setMinimumWidth(300);
    layout->addWidget(m_copyright, 3, 1);
    
    addTab(frame, i18n("About"));
}


void
DocumentMetaConfigurationPage::apply()
{
    Rosegarden::Composition &comp = m_doc->getComposition();
    QString copyright = m_copyright->text();
    
    if (!copyright.isNull()) {
        comp.setCopyrightNote(qstrtostr(copyright));
    }
}


AudioConfigurationPage::AudioConfigurationPage(RosegardenGUIDoc *doc,
                                               QWidget *parent,
                                               const char *name)
    : TabbedConfigurationPage(doc, parent, name),
      m_path(0),
      m_changePathButton(0)
{
    Rosegarden::AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 1, 4,
                                          10, 5);
    layout->addWidget(new QLabel(i18n("Audio file path"), frame), 0, 0);
    m_path = new QLineEdit(QString(afm.getAudioPath().c_str()), frame);
    m_path->setMinimumWidth(200);
    layout->addMultiCellWidget(m_path, 0, 1, 0, 2);
    
    m_changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(m_changePathButton, 0, 3);

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, i18n("Modify audio path"));
}

void
AudioConfigurationPage::slotFileDialog()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();

    KFileDialog *fileDialog = new KFileDialog(QString(afm.getAudioPath().c_str()),
                                              QString::null,
                                              this, "file dialog", true);
    fileDialog->setMode(KFile::Directory);

    connect(fileDialog, SIGNAL(fileSelected(const QString&)),
            SLOT(slotFileSelected(const QString&)));

    connect(fileDialog, SIGNAL(destroyed()),
            SLOT(slotDirectoryDialogClosed()));

    if (fileDialog->exec() == QDialog::Accepted)
    {
        m_path->setText(fileDialog->selectedFile());
    }
    delete fileDialog;
}

void
AudioConfigurationPage::apply()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();
    QString newDir = m_path->text();
    
    if (!newDir.isNull())
    {
        afm.setAudioPath(qstrtostr(newDir));
    }
}


//------------------------------------------------------------

static inline QPixmap loadIcon(const char *name)
{
  return KGlobal::instance()->iconLoader()
    ->loadIcon(QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium);
}

ConfigureDialogBase::ConfigureDialogBase(QWidget *parent,
                                         const char *name):
    KDialogBase(IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
                Ok, parent, name, true) // modal
{
}

ConfigureDialogBase::~ConfigureDialogBase()
{
}

void
ConfigureDialogBase::slotApply()
{
    for(configurationpages::iterator i = m_configurationPages.begin();
        i != m_configurationPages.end(); ++i)
        (*i)->apply();
}


void
ConfigureDialogBase::slotActivateApply()
{
//     ApplyButton->setDisabled(false);
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
}

ConfigureDialog::ConfigureDialog(RosegardenGUIDoc *doc,
                                 KConfig* cfg,
                                 QWidget *parent,
                                 const char *name)
    : ConfigureDialogBase(parent, name), m_doc(doc)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // General Page
    //
    pageWidget = addPage(GeneralConfigurationPage::iconLabel(),
                         GeneralConfigurationPage::title(),
                         loadIcon(GeneralConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new GeneralConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Notation Page
    pageWidget = addPage(NotationConfigurationPage::iconLabel(),
			 NotationConfigurationPage::title(),
			 loadIcon(NotationConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new NotationConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Matrix Page
    pageWidget = addPage(MatrixConfigurationPage::iconLabel(),
			 MatrixConfigurationPage::title(),
			 loadIcon(MatrixConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new MatrixConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Playback Page
    //
    pageWidget = addPage(LatencyConfigurationPage::iconLabel(),
                         LatencyConfigurationPage::title(),
                         loadIcon(LatencyConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new LatencyConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Sequencer Page
    //
    pageWidget = addPage(SequencerConfigurationPage::iconLabel(),
                         SequencerConfigurationPage::title(),
                         loadIcon(SequencerConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new SequencerConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

}

//------------------------------------------------------------

DocumentConfigureDialog::DocumentConfigureDialog(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigureDialogBase(parent, name),
      m_doc(doc)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // Document Meta Page
    //
    pageWidget = addPage(DocumentMetaConfigurationPage::iconLabel(),
                         DocumentMetaConfigurationPage::title(),
                         loadIcon(DocumentMetaConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new DocumentMetaConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Audio Page
    //
    pageWidget = addPage(AudioConfigurationPage::iconLabel(),
                         AudioConfigurationPage::title(),
                         loadIcon(AudioConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new AudioConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
}

}
