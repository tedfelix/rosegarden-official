// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"
#include "MidiDevice.h"

#include "SoundDriver.h"

#include "config.h"

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
#include <qtooltip.h>
#include <qvbox.h>
#include <qstringlist.h>
#include <qtable.h>
#include <qheader.h>
#include <klineeditdlg.h>

#include <kcombobox.h>
#include <klistview.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kcolordialog.h>

#include "constants.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenconfigurationpage.h"
#include "notationhlayout.h"
#include "notationstrings.h"
#include "notationview.h"
#include "matrixview.h"
#include "notestyle.h"
#include "notefont.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "notefontviewer.h"
#include "sequencemanager.h"
#include "notefont.h"
#include "matrixtool.h"
#include "notationtool.h"
#include "segmenttool.h"
#include "editcommands.h"
#include "studiocontrol.h"
#include "widgets.h"
#include "colourwidgets.h"
#include "audiopluginmanager.h"
#include "diskspace.h"
#include "segmentcommands.h"

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

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 KConfig *cfg,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(doc, cfg, parent, name)
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

GeneralConfigurationPage::GeneralConfigurationPage(RosegardenGUIDoc *doc,
                                                   KConfig *cfg,
                                                   QWidget *parent, const char *name)
    : TabbedConfigurationPage(cfg, parent, name),
      m_doc(doc),
      m_client(0),
      m_countIn(0),
      m_midiPitchOctave(0),
      m_externalAudioEditorPath(0),
      m_nameStyle(0)
{
    m_cfg->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    //
    // "Appearance" tab
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                             4, 2, // nbrow, nbcol -- one extra row improves layout
                             10, 5);
    layout->addWidget(new QLabel(i18n("Note name style"),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Base octave number for MIDI pitch display"),
                                 frame), 1, 0);

    QVBox *box = new QVBox(frame);
    new QLabel(i18n("Use textured backgrounds on canvas areas"), box);
    new QLabel(i18n("    (takes effect only from next restart)"), box);
    layout->addWidget(box, 2, 0);

    m_nameStyle = new KComboBox(frame);
    m_nameStyle->insertItem(i18n("Always use US names (e.g. quarter, 8th)"));
    m_nameStyle->insertItem(i18n("Localised (where available)"));
    m_nameStyle->setCurrentItem(m_cfg->readUnsignedNumEntry("notenamestyle", Local));
    layout->addWidget(m_nameStyle, 0, 1);

    m_midiPitchOctave = new QSpinBox(frame);
    m_midiPitchOctave->setMaxValue(10);
    m_midiPitchOctave->setMinValue(-10);
    m_midiPitchOctave->setValue(m_cfg->readNumEntry("midipitchoctave", -2));

    layout->addWidget(m_midiPitchOctave, 1, 1);

    m_backgroundTextures = new QCheckBox(frame);
    layout->addWidget(m_backgroundTextures, 2, 1);

    m_backgroundTextures->setChecked(m_cfg->readBoolEntry("backgroundtextures",
                                                          false));

    addTab(frame, i18n("Presentation"));

    //
    // "Behaviour" tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             4, 2, // nbrow, nbcol
                             10, 5);

    layout->addWidget(new QLabel(i18n("Default editor (for double-click on segment)"),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Number of count-in bars when recording"),
                                 frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Always use default studio when loading files"),
		                 frame), 2, 0);

    m_client = new KComboBox(frame);
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

    m_studio = new QCheckBox(frame);
    m_studio->setChecked(m_cfg->readBoolEntry("alwaysusedefaultstudio", false));
    layout->addWidget(m_studio, 2, 1);

    addTab(frame, i18n("Behaviour"));

    //
    // External editor tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             2, 3, // nbrow, nbcol
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

    //
    // Autosave tab
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
                             2, 2, // nbrow, nbcol
                             10, 5);

    layout->addWidget(new QLabel(i18n("Auto-save interval (in seconds)"),
                                 frame), 0, 0);
    m_autosaveInterval = new QSpinBox(0, 600, 10, frame);
    m_autosaveInterval->setValue(m_cfg->readUnsignedNumEntry("autosaveinterval", 60));
    layout->addWidget(m_autosaveInterval, 0, 1);

    addTab(frame, i18n("Auto-save"));

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
    m_cfg->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    int countIn = getCountInSpin();
    m_cfg->writeEntry("countinbars", countIn);

    int client = getDblClickClient();
    m_cfg->writeEntry("doubleclickclient", client);

    bool studio = getUseDefaultStudio();
    m_cfg->writeEntry("alwaysusedefaultstudio", studio);

    int octave = m_midiPitchOctave->value();
    m_cfg->writeEntry("midipitchoctave", octave);

    int namestyle = getNoteNameStyle();
    m_cfg->writeEntry("notenamestyle", namestyle);

    m_cfg->writeEntry("backgroundtextures", m_backgroundTextures->isChecked());

    unsigned int autosaveInterval = m_autosaveInterval->value();
    m_cfg->writeEntry("autosaveinterval", autosaveInterval);
    emit updateAutoSaveInterval(autosaveInterval);

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
    m_cfg->setGroup(NotationView::ConfigGroup);

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          6, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget
        (new QLabel(i18n("Notation font"), frame), 0, 0);
    layout->addWidget
        (new QLabel(i18n("Font size for single-staff views"), frame),
         2, 0);
    layout->addWidget
        (new QLabel(i18n("Font size for multi-staff views"), frame),
         3, 0);
    layout->addWidget
        (new QLabel(i18n("Font size for printing (pt)"), frame),
         4, 0);

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

    m_viewButton = 0;

#ifdef HAVE_XFT
    QHBox *fontBox = new QHBox(frame);
    m_font = new KComboBox(fontBox);
    m_viewButton = new QPushButton(i18n("View"), fontBox);
    fontBox->setStretchFactor(m_font, 2);
    fontBox->setStretchFactor(m_viewButton, 1);
    layout->addWidget(fontBox, 0, 1);
    QObject::connect(m_viewButton, SIGNAL(clicked()),
		     this, SLOT(slotViewButtonPressed()));
#else
    m_font = new KComboBox(frame);
    layout->addWidget(m_font, 0, 1);
#endif

    m_font->setEditable(false);

    QString defaultFont = m_cfg->readEntry
        ("notefont", strtoqstr(NoteFontFactory::getDefaultFontName()));

    try {
	(void)NoteFontFactory::getFont
	    (qstrtostr(defaultFont),
	     NoteFontFactory::getDefaultSize(qstrtostr(defaultFont)));
    } catch (Rosegarden::Exception e) {
	defaultFont = strtoqstr(NoteFontFactory::getDefaultFontName());
    }

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {
        QString s(strtoqstr(*i));
        m_font->insertItem(s);
        if (s == defaultFont) m_font->setCurrentItem(m_font->count() - 1);
    }
    QObject::connect(m_font, SIGNAL(activated(const QString &)),
                     this, SLOT(slotFontComboChanged(const QString &)));

    m_singleStaffSize = new KComboBox(frame);
    m_singleStaffSize->setEditable(false);

    m_multiStaffSize = new KComboBox(frame);
    m_multiStaffSize->setEditable(false);

    m_printingSize = new KComboBox(frame);
    m_printingSize->setEditable(false);

    slotFontComboChanged(defaultFont);

    layout->addWidget(m_singleStaffSize, 2, 1);
    layout->addWidget(m_multiStaffSize, 3, 1);
    layout->addWidget(m_printingSize, 4, 1);

    addTab(frame, i18n("Font"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 6, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Default layout mode"), frame), 0, 0);

    m_layoutMode = new KComboBox(frame);
    m_layoutMode->setEditable(false);
    m_layoutMode->insertItem(i18n("Linear layout"));
    m_layoutMode->insertItem(i18n("Continuous page layout"));
    m_layoutMode->insertItem(i18n("Multiple page layout"));
    int defaultLayoutMode = m_cfg->readNumEntry("layoutmode", 0);
    if (defaultLayoutMode >= 0 && defaultLayoutMode <= 1) {
        m_layoutMode->setCurrentItem(defaultLayoutMode);
    }
    layout->addWidget(m_layoutMode, 0, 1);
    
    layout->addWidget(new QLabel(i18n("Default spacing"), frame), 1, 0);

    m_spacing = new KComboBox(frame);
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
    
    layout->addWidget(new QLabel(i18n("Default proportion"), frame), 2, 0);

    m_proportion = new KComboBox(frame);
    m_proportion->setEditable(false);

    s = NotationHLayout::getAvailableProportions();
    int defaultProportion = m_cfg->readNumEntry("proportion", 60);

    for (std::vector<int>::iterator i = s.begin(); i != s.end(); ++i) {

        QString text = QString("%1 %").arg(*i);
        if (*i == 40) text = "40 % (normal)";
        else if (*i == 0) text = i18n("None");
        else if (*i == 100) text = i18n("Full");
        m_proportion->insertItem(text);

        if (*i == defaultProportion) {
            m_proportion->setCurrentItem(m_proportion->count() - 1);
        }
    }

    layout->addWidget(m_proportion, 2, 1);

    m_showUnknowns = new QCheckBox
        (i18n("Show non-notation events as question marks"), frame);
    bool defaultShowUnknowns = m_cfg->readBoolEntry("showunknowns", false);
    m_showUnknowns->setChecked(defaultShowUnknowns);
    layout->addWidget(m_showUnknowns, 3, 1);

    m_colourQuantize = new QCheckBox
        (i18n("Show notation-quantized notes in a different colour"), frame);
    bool defaultColourQuantize = m_cfg->readBoolEntry("colourquantize", false);
    m_colourQuantize->setChecked(defaultColourQuantize);
    layout->addWidget(m_colourQuantize, 4, 1);

    addTab(frame, i18n("Layout"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 6, 2, 10, 5);

    layout->addWidget
        (new QLabel(i18n("Default note style for new notes"), frame), 0, 0);

    m_noteStyle = new KComboBox(frame);
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

    m_insertType = new KComboBox(frame);
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

    m_pasteType = new KComboBox(frame);
    m_pasteType->setEditable(false);

    unsigned int defaultPasteType = m_cfg->readUnsignedNumEntry
        ("pastetype", PasteEventsCommand::Restricted);

    PasteEventsCommand::PasteTypeMap pasteTypes =
        PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
         i != pasteTypes.end(); ++i) {
        m_pasteType->insertItem(i->second);
    }

    m_pasteType->setCurrentItem(defaultPasteType);
    layout->addWidget(m_pasteType, 4, 1);

    addTab(frame, i18n("Editing"));

    QString preamble =
        (i18n("Rosegarden can apply automatic quantization to recorded "
              "or imported MIDI data for notation purposes only. "
              "This does not affect playback, and does not affect "
              "editing in any of the views except notation."));

    // force to default of 2 if not used before
    int quantizeType = m_cfg->readNumEntry("quantizetype", 2);
    m_cfg->writeEntry("quantizetype", quantizeType);
    m_cfg->writeEntry("quantizenotationonly", true);

    m_quantizeFrame = new RosegardenQuantizeParameters
        (m_tabWidget, RosegardenQuantizeParameters::Notation,
	 false, false, "Notation Options", preamble);

    addTab(m_quantizeFrame, i18n("Quantize"));
}

void
NotationConfigurationPage::slotViewButtonPressed()
{
#ifdef HAVE_XFT
    std::string fontName = qstrtostr(m_font->currentText());

    try {
	NoteFont *noteFont = NoteFontFactory::getFont
	    (fontName, NoteFontFactory::getDefaultSize(fontName));
        const NoteFontMap &map(noteFont->getNoteFontMap());
	QStringList systemFontNames(map.getSystemFontNames());
	if (systemFontNames.count() == 0) {
	    m_viewButton->setEnabled(false); // oops
	} else {
	    NoteFontViewer *viewer =
		new NoteFontViewer(0, m_font->currentText(),
				   systemFontNames, 24);
	    (void)viewer->exec(); // no return value
	}
    } catch (Rosegarden::Exception f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
    }
#endif
}

void
NotationConfigurationPage::slotFontComboChanged(const QString &font)
{
    std::string fontStr = qstrtostr(font);

    populateSizeCombo(m_singleStaffSize, fontStr,
                      m_cfg->readUnsignedNumEntry
                      ("singlestaffnotesize",
                       NoteFontFactory::getDefaultSize(fontStr)));
    populateSizeCombo(m_multiStaffSize, fontStr,
                      m_cfg->readUnsignedNumEntry
                      ("multistaffnotesize",
                       NoteFontFactory::getDefaultSize(fontStr)));

    int printpt = m_cfg->readUnsignedNumEntry("printingnotesize", 5);
    for (int i = 2; i < 16; ++i) {
	m_printingSize->insertItem(QString("%1").arg(i));
	if (i == printpt) {
	    m_printingSize->setCurrentItem(m_printingSize->count()-1);
	}
    }

    try {
	NoteFont *noteFont = NoteFontFactory::getFont
	    (fontStr, NoteFontFactory::getDefaultSize(fontStr));
        const NoteFontMap &map(noteFont->getNoteFontMap());
        m_fontOriginLabel->setText(strtoqstr(map.getOrigin()));
        m_fontCopyrightLabel->setText(strtoqstr(map.getCopyright()));
        m_fontMappedByLabel->setText(strtoqstr(map.getMappedBy()));
        if (map.isSmooth()) {
            m_fontTypeLabel->setText(strtoqstr(map.getType() + " (smooth)"));
        } else {
            m_fontTypeLabel->setText(strtoqstr(map.getType() + " (jaggy)"));
        }
	if (m_viewButton) {
	    m_viewButton->setEnabled(map.getSystemFontNames().count() > 0);
	}
    } catch (Rosegarden::Exception f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
    }
}

void
NotationConfigurationPage::populateSizeCombo(QComboBox *combo,
                                             std::string font,
                                             int defaultSize)
{
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(font);
    combo->clear();
    
    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {
        combo->insertItem(QString("%1").arg(*i));
        if (*i == defaultSize) combo->setCurrentItem(combo->count() - 1);
    }
}

void
NotationConfigurationPage::apply()
{
    m_cfg->setGroup(NotationView::ConfigGroup);

    m_cfg->writeEntry("notefont", m_font->currentText());
    m_cfg->writeEntry("singlestaffnotesize",
                      m_singleStaffSize->currentText().toUInt());
    m_cfg->writeEntry("multistaffnotesize",
                      m_multiStaffSize->currentText().toUInt());
    m_cfg->writeEntry("printingnotesize",
                      m_printingSize->currentText().toUInt());

    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    m_cfg->writeEntry("spacing", s[m_spacing->currentItem()]);

    s = NotationHLayout::getAvailableProportions();
    m_cfg->writeEntry("proportion", s[m_proportion->currentItem()]);

    m_cfg->writeEntry("layoutmode", m_layoutMode->currentItem());
    m_cfg->writeEntry("colourquantize", m_colourQuantize->isChecked());
    m_cfg->writeEntry("showunknowns", m_showUnknowns->isChecked());
    m_cfg->writeEntry("style", m_noteStyle->currentText());
    m_cfg->writeEntry("inserttype", m_insertType->currentItem());
    m_cfg->writeEntry("autobeam", m_autoBeam->isChecked());
    m_cfg->writeEntry("collapse", m_collapseRests->isChecked());
    m_cfg->writeEntry("pastetype", m_pasteType->currentItem());

    (void)m_quantizeFrame->getQuantizer(); // this also writes to the config
}


MatrixConfigurationPage::MatrixConfigurationPage(KConfig *cfg,
                                                 QWidget *parent,
                                                 const char *name) :
    TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup(MatrixView::ConfigGroup);

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          4, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel("Nothing here yet", frame), 0, 0);

    addTab(frame, i18n("General"));
}

void MatrixConfigurationPage::apply()
{
    m_cfg->setGroup(MatrixView::ConfigGroup);
}


LatencyConfigurationPage::LatencyConfigurationPage(RosegardenGUIDoc *doc,
                                                   KConfig *cfg,
                                                   QWidget *parent,
                                                   const char *name)
    : TabbedConfigurationPage(doc, cfg, parent, name)
{
//     Rosegarden::Configuration &config = doc->getConfiguration();
    m_cfg->setGroup(Rosegarden::LatencyOptionsConfigGroup);

#ifdef NOT_DEFINED
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
#endif // NOT_DEFINED

}

void LatencyConfigurationPage::apply()
{
    m_cfg->setGroup(Rosegarden::LatencyOptionsConfigGroup);

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
    int jackPlaybackValue = m_doc->getAudioPlayLatency().msec() +
                            m_doc->getAudioPlayLatency().sec * 1000;

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = m_doc->getAudioRecordLatency().msec() +
                          m_doc->getAudioRecordLatency().sec * 1000;
    m_jackRecord->setValue(jackRecordValue);
}


// -------------------  SequencerConfigurationPage ---------------------
//

SequencerConfigurationPage::SequencerConfigurationPage(
                                                   RosegardenGUIDoc *doc,
                                                   KConfig *cfg,
                                                   QWidget *parent,
                                                   const char *name):
    TabbedConfigurationPage(cfg, parent, name)
{
    // set the document in the super class
    m_doc = doc;

    m_cfg->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // ---------------- General tab ------------------
    //
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    layout->addWidget(new QLabel(i18n("Sequencer status"), frame), 0, 0);
    
    QString status(i18n("Unknown"));
    Rosegarden::SequenceManager *mgr = doc->getSequenceManager();
    if (mgr) {
	int driverStatus = mgr->getSoundDriverStatus();
	switch (driverStatus) {
	case Rosegarden::AUDIO_OK:
	    status = i18n("No MIDI, audio OK"); break;
	case Rosegarden::MIDI_OK:
	    status = i18n("MIDI OK, no audio"); break;
	case Rosegarden::AUDIO_OK | Rosegarden::MIDI_OK:
	    status = i18n("MIDI OK, audio OK"); break;
	default:
	    status = i18n("No driver"); break;
	}
    }

    layout->addWidget(new QLabel(status, frame), 0, 1);

    QPushButton *showStatusButton = new QPushButton(i18n("Show detailed status"),
						    frame);
    QObject::connect(showStatusButton, SIGNAL(clicked()),
		     this, SLOT(slotShowStatus()));
    layout->addWidget(showStatusButton, 0, 2);

    // Send Controllers
    //
    QLabel *label = new QLabel(i18n("Send MIDI Controllers at start of playback\n     (will incur noticeable initial delay)"), frame);

    QString controllerTip = i18n("Rosegarden can send all MIDI Controllers (Pan, Reverb etc) to all MIDI devices every\ntime you hit play if you so wish.  Please note that this option will usually incur a\ndelay at the start of playback due to the amount of data being transmitted.");
    QToolTip::add(label, controllerTip);
    layout->addWidget(label, 1, 0);

    m_sendControllersAtPlay = new QCheckBox(frame);
    bool sendControllers = m_cfg->readBoolEntry("alwayssendcontrollers", false);
    m_sendControllersAtPlay->setChecked(sendControllers);
    QToolTip::add(m_sendControllersAtPlay, controllerTip);
    layout->addMultiCellWidget(m_sendControllersAtPlay, 1, 1, 1, 2);

    // Command-line
    //
    layout->addWidget(new QLabel(i18n("Sequencer command line options\n     (takes effect only from next restart)"), frame), 2, 0);

    m_sequencerArguments = new QLineEdit("", frame);
    layout->addMultiCellWidget(m_sequencerArguments, 2, 2, 1, 2);

    // Get the options
    //
    QString commandLine = m_cfg->readEntry("commandlineoptions", "");
    m_sequencerArguments->setText(commandLine);

    addTab(frame, i18n("General"));


    // --------------------- Startup control ----------------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 4, 4, 10, 5);

#ifdef HAVE_LIBJACK
    label = new QLabel(i18n("Rosegarden can start the JACK audio daemon (jackd) for you\nautomatically if it isn't already running when Rosegarden starts.\n\nThis is recommended for beginners and those who use Rosegarden as their main\naudio application, but it might not be to the liking of advanced users.\n\nIf you want to start JACK automatically, make sure the command includes a full\npath where necessary as well as any command-line arguments you want to use.\n\nFor example: /usr/local/bin/jackd -d alsa -d hw -r44100 -p 2048 -n 2\n"), frame);

    layout->addMultiCellWidget(label, 1, 1, 0, 3);

    // JACK control things
    //
    bool startJack = m_cfg->readBoolEntry("jackstart", false);
    m_startJack = new QCheckBox(frame);
    m_startJack->setChecked(startJack);

    connect(m_startJack, SIGNAL(released()),
            this, SLOT(slotJackToggled()));

    layout->addWidget(new QLabel(i18n("Start JACK when Rosegarden starts"), frame), 2, 0);

    layout->addWidget(m_startJack, 2, 1);

    layout->addWidget(new QLabel(i18n("JACK command (including path as necessary)"), frame),
                      3, 0);

    QString jackPath = m_cfg->readEntry("jackcommand", 
                                        "/usr/local/bin/jackd -d alsa -d hw -r 44100 -p 2048 -n 2");
    m_jackPath = new QLineEdit(jackPath, frame);

    layout->addMultiCellWidget(m_jackPath, 3, 3, 1, 3);

    // set the initial state
    slotJackToggled();

#endif // HAVE_LIBJACK

    addTab(frame, i18n("Startup"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 10, 3,
			     10, 5);

    layout->addMultiCellWidget(new QLabel(i18n("Longer buffers usually improve playback quality, but use more memory and slow response."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("Event read-ahead (ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Audio mix buffer (ms)"), frame), 3, 0);
    layout->addWidget(new QLabel(i18n("Audio file read buffer (ms)"), frame), 5, 0);
    layout->addWidget(new QLabel(i18n("Audio file write buffer (ms)"), frame), 7, 0);
    layout->addWidget(new QLabel(i18n("Per-file limit for cacheable audio files"), frame), 9, 0);

    m_readAhead = new QSlider(Horizontal, frame);

    m_readAhead->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 2, 1);

    m_readAhead->setMaxValue(500);
    layout->addWidget(new QLabel("500", frame), 2, 3);
    
    m_readAhead->setLineStep(20);
    m_readAhead->setPageStep(20);

    int readAheadValue = m_cfg->readLongNumEntry("readaheadusec", 80000) / 1000;
    m_readAhead->setValue(readAheadValue);
    m_readAhead->setTickmarks(QSlider::Below);
    layout->addWidget(m_readAhead, 2, 2);

    m_readAheadLabel = new QLabel(QString("%1").arg(readAheadValue), frame);
    layout->addWidget(m_readAheadLabel, 1, 2, Qt::AlignHCenter);

    connect(m_readAhead,
            SIGNAL(valueChanged(int)),
            SLOT(slotReadAheadChanged(int)));

    m_audioMix = new QSlider(Horizontal, frame);

    m_audioMix->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 4, 1);

    m_audioMix->setMaxValue(500);
    layout->addWidget(new QLabel("500", frame), 4, 3);

    m_audioMix->setLineStep(20);
    m_audioMix->setPageStep(20);

    int audioMixValue = m_cfg->readLongNumEntry("audiomixusec", 60000) / 1000;
    m_audioMix->setValue(audioMixValue);
    m_audioMix->setTickmarks(QSlider::Below);
    layout->addWidget(m_audioMix, 4, 2);

    m_audioMixLabel = new QLabel(QString("%1").arg(audioMixValue), frame);
    layout->addWidget(m_audioMixLabel, 3, 2, Qt::AlignHCenter);

    connect(m_audioMix,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioMixChanged(int)));


    m_audioRead = new QSlider(Horizontal, frame);

    m_audioRead->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 6, 1);

    m_audioRead->setMaxValue(500);
    layout->addWidget(new QLabel("500", frame), 6, 3);

    m_audioRead->setLineStep(20);
    m_audioRead->setPageStep(20);

    int audioReadValue = m_cfg->readLongNumEntry("audioreadusec", 80000) / 1000;
    m_audioRead->setValue(audioReadValue);
    m_audioRead->setTickmarks(QSlider::Below);
    layout->addWidget(m_audioRead, 6, 2);

    m_audioReadLabel = new QLabel(QString("%1").arg(audioReadValue), frame);
    layout->addWidget(m_audioReadLabel, 5, 2, Qt::AlignHCenter);

    connect(m_audioRead,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioReadChanged(int)));


    m_audioWrite = new QSlider(Horizontal, frame);

    m_audioWrite->setMinValue(50);
    layout->addWidget(new QLabel("50", frame), 8, 1);

    m_audioWrite->setMaxValue(1000);
    layout->addWidget(new QLabel("1000", frame), 8, 3);

    m_audioWrite->setLineStep(50);
    m_audioWrite->setPageStep(50);

    int audioWriteValue = m_cfg->readLongNumEntry("audiowriteusec", 200000) / 1000;
    m_audioWrite->setValue(audioWriteValue);
    m_audioWrite->setTickmarks(QSlider::Below);
    layout->addWidget(m_audioWrite, 8, 2);

    m_audioWriteLabel = new QLabel(QString("%1").arg(audioWriteValue), frame);
    layout->addWidget(m_audioWriteLabel, 7, 2, Qt::AlignHCenter);

    connect(m_audioWrite,
            SIGNAL(valueChanged(int)),
            SLOT(slotAudioWriteChanged(int)));


    m_smallFile = new QSlider(Horizontal, frame);

    m_smallFile->setMinValue(5);
    layout->addWidget(new QLabel("32KB", frame), 10, 1);

    m_smallFile->setMaxValue(15);
    layout->addWidget(new QLabel("32MB", frame), 10, 3);

    int smallFileValue = m_cfg->readLongNumEntry("smallaudiofilekbytes", 128);
    int powerOfTwo = 1;
    while (1 << powerOfTwo < smallFileValue) ++powerOfTwo;
    m_smallFile->setValue(powerOfTwo);
    m_smallFile->setTickmarks(QSlider::Below);
    layout->addWidget(m_smallFile, 10, 2);

    if (smallFileValue < 1024) {
	m_smallFileLabel = new QLabel(QString("%1KB").arg(smallFileValue),
				      frame);
    } else {
	m_smallFileLabel = new QLabel(QString("%1MB").arg(smallFileValue/1024),
				      frame);
    }
    layout->addWidget(m_smallFileLabel, 9, 2, Qt::AlignHCenter);

    connect(m_smallFile,
            SIGNAL(valueChanged(int)),
            SLOT(slotSmallFileChanged(int)));

    addTab(frame, i18n("Buffers"));


    // ------------------ Record tab ---------------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 6, 2, 10, 5);

    int increment = 0;

#ifdef HAVE_LIBJACK

    label = new QLabel(i18n("Number of audio inputs:"), frame);
    m_jackInputs = new QSpinBox(frame);

    layout->addWidget(label,        0, 0);
    layout->addWidget(m_jackInputs, 0, 1);
    ++increment;

    int jackAudioInputs = m_cfg->readNumEntry("jackaudioinputs", 2);

    m_jackInputs->setValue(jackAudioInputs);
    m_jackInputs->setMinValue(2);
    m_jackInputs->setMaxValue(64); // completely arbitrary of course!

    label = new QLabel(i18n("Number of audio submasters:"), frame);
    m_submasters = new QSpinBox(frame);

    layout->addWidget(label,        1, 0);
    layout->addWidget(m_submasters, 1, 1);
    ++increment;

    int submasterCount = m_cfg->readNumEntry("audiosubmasters", 4);
    m_submasters->setValue(submasterCount);
    m_submasters->setMinValue(0);
    m_submasters->setMaxValue(8);

    label = new QLabel(i18n("Create JACK outputs for instrument faders"), frame);
    m_createFaderOuts = new QCheckBox(frame);
    m_createFaderOuts->setChecked(m_cfg->readBoolEntry("audiofaderouts", false));

    layout->addWidget(label, 2, 0);
    layout->addWidget(m_createFaderOuts, 2, 1);
    ++increment;

    label = new QLabel(i18n("Create JACK outputs for submasters"), frame);
    m_createSubmasterOuts = new QCheckBox(frame);
    m_createSubmasterOuts->setChecked(m_cfg->readBoolEntry("audiosubmasterouts",
							   false));

    layout->addWidget(label, 3, 0);
    layout->addWidget(m_createSubmasterOuts, 3, 1);
    ++increment;

#endif // HAVE_LIBJACK

    label = new QLabel(i18n("Minutes of audio recording:"), frame);
    m_audioRecordMinutes = new QSpinBox(frame);

    layout->addWidget(label,                0 + increment, 0);
    layout->addWidget(m_audioRecordMinutes, 0 + increment, 1);

    int audioRecordMinutes = m_cfg->readNumEntry("audiorecordminutes", 5);

    m_audioRecordMinutes->setValue(audioRecordMinutes);
    m_audioRecordMinutes->setMinValue(1);
    m_audioRecordMinutes->setMaxValue(60);

    addTab(frame, i18n("Record and Mix"));

    //  -------------- Synchronisation tab -----------------
    //
    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 5, 2, 10, 5);

    // Timer selection
    // 
    label = new QLabel(i18n("Sequencer timer"), frame);
    layout->addWidget(label, 0, 0);

    m_timer = new KComboBox(frame);
    layout->addWidget(m_timer, 0, 1); //, Qt::AlignHCenter);

    QStringList timers = m_doc->getTimers();
    QString currentTimer = m_doc->getCurrentTimer();
    currentTimer = m_cfg->readEntry("timer", currentTimer);

    for (unsigned int i = 0; i < timers.size(); ++i) {
	m_timer->insertItem(timers[i]);
	if (timers[i] == currentTimer) m_timer->setCurrentItem(i);
    }

    // MIDI Clock
    //
    label = new QLabel(i18n("Send MIDI Clock and System messages"), frame);
    layout->addWidget(label, 1, 0);
    m_midiClockEnabled = new QCheckBox(frame);
    layout->addWidget(m_midiClockEnabled, 1, 1);

    bool midiClock = m_cfg->readBoolEntry("midiclock", false);
    m_midiClockEnabled->setChecked(midiClock);

    // JACK Transport
    //
    label = new QLabel(i18n("JACK transport mode"), frame);
    layout->addWidget(label, 2, 0);

    m_jackTransport = new KComboBox(frame);
    layout->addWidget(m_jackTransport, 2, 1); //, Qt::AlignHCenter);

    m_jackTransport->insertItem(i18n("Ignore JACK transport"));
    m_jackTransport->insertItem(i18n("Sync"));
    m_jackTransport->insertItem(i18n("Sync, and offer timebase master"));

    bool jackMaster = m_cfg->readBoolEntry("jackmaster", false);
    bool jackTransport = m_cfg->readBoolEntry("jacktransport", false);

    if (jackMaster && jackTransport)
        m_jackTransport->setCurrentItem(2);
    else 
    {
        if (jackTransport)
            m_jackTransport->setCurrentItem(1);
        else
            m_jackTransport->setCurrentItem(0);
    }

    // MMC Transport
    //
    label = new QLabel(i18n("MMC transport mode"), frame);
    layout->addWidget(label, 3, 0);
    
    m_mmcTransport = new KComboBox(frame);
    layout->addWidget(m_mmcTransport, 3, 1); //, Qt::AlignHCenter);

    m_mmcTransport->insertItem(i18n("Off"));
    m_mmcTransport->insertItem(i18n("MMC Slave"));
    m_mmcTransport->insertItem(i18n("MMC Master"));

    bool mmcMaster = m_cfg->readBoolEntry("mmcmaster", false);
    bool mmcTransport = m_cfg->readBoolEntry("mmctransport", false);

    if (mmcMaster && mmcTransport)
        m_mmcTransport->setCurrentItem(2);
    else
    {
        if (mmcTransport)
            m_mmcTransport->setCurrentItem(1);
        else
            m_mmcTransport->setCurrentItem(0);
    }


    addTab(frame, i18n("Synchronisation"));
}

void
SequencerConfigurationPage::slotReadAheadChanged(int v)
{
    // event read-ahead must always be at least 10ms more than
    // the audio mix or read buffer, and the read buffer should
    // be at least as long as the mix buffer.

    if (m_audioMix->value() > v-10) m_audioMix->setValue(v-10);
    m_readAheadLabel->setNum(v);
}

void
SequencerConfigurationPage::slotAudioMixChanged(int v)
{
    if (m_readAhead->value() < v+10) m_readAhead->setValue(v+10);
    m_audioMixLabel->setNum(v);
}

void
SequencerConfigurationPage::slotAudioReadChanged(int v)
{
    m_audioReadLabel->setNum(v);
}

void
SequencerConfigurationPage::slotAudioWriteChanged(int v)
{
    m_audioWriteLabel->setNum(v);
}

void
SequencerConfigurationPage::slotSmallFileChanged(int v)
{
    QString text;
    v = 1 << v;
    if (v < 1024) text = QString("%1KB").arg(v);
    else text = QString("%1MB").arg(v/1024);
    m_smallFileLabel->setText(text);
}

void
SequencerConfigurationPage::slotShowStatus()
{
    ShowSequencerStatusDialog *dialog = new ShowSequencerStatusDialog(this);
    dialog->exec();
}

void
SequencerConfigurationPage::slotJackToggled()
{
#ifdef HAVE_LIBJACK
    /*
    if (m_startJack->isChecked())
        m_jackPath->setDisabled(false);
    else
        m_jackPath->setDisabled(true);

        */
#endif // HAVE_LIBJACK
}


void
SequencerConfigurationPage::apply()
{
    m_cfg->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // ---------- General -----------
    //
    m_cfg->writeEntry("commandlineoptions", m_sequencerArguments->text());
    m_cfg->writeEntry("alwayssendcontrollers",
                       m_sendControllersAtPlay->isChecked());

    m_cfg->writeEntry("readaheadusec", m_readAhead->value() * 1000);
    m_cfg->writeEntry("readaheadsec", 0L);
    
    m_cfg->writeEntry("audiomixusec", m_audioMix->value() * 1000);
    m_cfg->writeEntry("audiomixsec", 0L);
    
    m_cfg->writeEntry("audioreadusec", m_audioRead->value() * 1000);
    m_cfg->writeEntry("audioreadsec", 0L);
    
    m_cfg->writeEntry("audiowriteusec", m_audioWrite->value() * 1000);
    m_cfg->writeEntry("audiowritesec", 0L);

    m_cfg->writeEntry("smallaudiofilekbytes", 1 << m_smallFile->value());

#ifdef HAVE_LIBJACK

    // Jack control
    //
    m_cfg->writeEntry("jackstart", m_startJack->isChecked());
    m_cfg->writeEntry("jackcommand", m_jackPath->text());

    // Jack audio inputs
    //
    m_cfg->writeEntry("jackaudioinputs", m_jackInputs->value());

    m_cfg->writeEntry("audiosubmasters", m_submasters->value());
    m_cfg->writeEntry("audiofaderouts", m_createFaderOuts->isChecked());
    m_cfg->writeEntry("audiosubmasterouts", m_createSubmasterOuts->isChecked());

    // Audio record minutes
    //
    m_cfg->writeEntry("audiorecordminutes", m_audioRecordMinutes->value());

    Rosegarden::MappedEvent mEjackInputs(Rosegarden::MidiInstrumentBase, // InstrumentId
                                         Rosegarden::MappedEvent::SystemAudioInputs,
                                         Rosegarden::MidiByte(m_jackInputs->value()));

    Rosegarden::StudioControl::sendMappedEvent(mEjackInputs);

    

    m_cfg->writeEntry("timer", m_timer->currentText());
    m_doc->setCurrentTimer(m_timer->currentText());
    


    // Write the JACK entry
    //
    int jackValue = m_jackTransport->currentItem();
    bool jackTransport, jackMaster;

    switch (jackValue)
    {
        case 2:
            jackTransport = true;
            jackMaster = true;
            break;

        case 1:
            jackTransport = true;
            jackMaster = false;
            break;

        default:
            jackValue = 0;

        case 0:
            jackTransport = false;
            jackMaster = false;
            break;
    }

    // Write the items
    //
    m_cfg->writeEntry("jacktransport", jackTransport);
    m_cfg->writeEntry("jackmaster", jackMaster);

    // Now send it
    //
    Rosegarden::MappedEvent mEjackValue(Rosegarden::MidiInstrumentBase, // InstrumentId
                                        Rosegarden::MappedEvent::SystemJackTransport,
                                        Rosegarden::MidiByte(jackValue));
 
    Rosegarden::StudioControl::sendMappedEvent(mEjackValue);
#endif // HAVE_LIBJACK

    // Now write the MMC entry
    //
    bool mmcTransport, mmcMaster;
    int mmcValue = m_mmcTransport->currentItem();

    switch(mmcValue)
    {
        case 2:
            mmcTransport = true;
            mmcMaster = true;
            break;

        case 1:
            mmcTransport = true;
            mmcMaster = false;
            break;

        default:
            mmcValue = 0;

        case 0:
            mmcTransport = false;
            mmcMaster = false;
            break;

    }

    // Write the entries
    //
    m_cfg->writeEntry("mmctransport", mmcTransport);
    m_cfg->writeEntry("mmcmaster", mmcMaster);

    // Now send it
    //
    Rosegarden::MappedEvent mEmccValue(Rosegarden::MidiInstrumentBase, // InstrumentId
                                       Rosegarden::MappedEvent::SystemMMCTransport,
                                       Rosegarden::MidiByte(mmcValue));

    Rosegarden::StudioControl::sendMappedEvent(mEmccValue);


    // ------------- MIDI Clock and System messages ------------
    //
    bool midiClock = m_midiClockEnabled->isChecked();
    m_cfg->writeEntry("midiclock", midiClock);

    // Now send it
    //
    Rosegarden::MappedEvent mEMIDIClock(Rosegarden::MidiInstrumentBase, // InstrumentId
                                        Rosegarden::MappedEvent::SystemMIDIClock,
                                        Rosegarden::MidiByte(midiClock));

    Rosegarden::StudioControl::sendMappedEvent(mEMIDIClock);
}

// ---

/*
static QString absTimeToString(Rosegarden::Composition &comp,
			       Rosegarden::timeT absTime,
			       Rosegarden::RealTime rt)
{
    return i18n("%1 minutes %2.%3%4 seconds (%5 units, %6 bars)")
	.arg(rt.sec / 60).arg(rt.sec % 60)
	.arg(rt.msec() / 100).arg((rt.msec() / 10) % 10)
	.arg(absTime).arg(comp.getBarNumber(absTime) + 1);
}
*/
static QString durationToString(Rosegarden::Composition &comp,
				Rosegarden::timeT absTime,
				Rosegarden::timeT duration,
				Rosegarden::RealTime rt)
{
    return i18n("%1 minutes %2.%3%4 seconds (%5 units, %6 bars)")
	.arg(rt.sec / 60).arg(rt.sec % 60)
	.arg(rt.msec() / 100).arg((rt.msec() / 10) % 10)
	.arg(duration).arg(comp.getBarNumber(absTime + duration) -
			   comp.getBarNumber(absTime));
}


class SegmentDataItem : public QTableItem
{
public:
    SegmentDataItem(QTable *t, QString s) :
	QTableItem(t, QTableItem::Never, s) { }
    virtual int alignment() const { return Qt::AlignCenter; }
};

DocumentMetaConfigurationPage::DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
                                                             QWidget *parent,
                                                             const char *name) :
    TabbedConfigurationPage(doc, parent, name)
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 2, 2, 10, 5);

    m_metadata = new KListView(frame);
    m_metadata->addColumn("Name");
    m_metadata->addColumn("Value");
    m_metadata->setFullWidth();
    m_metadata->setItemsRenameable(true);
    m_metadata->setRenameable(0);
    m_metadata->setRenameable(1);
    m_metadata->setItemMargin(5);
    //m_metadata->setSelectionModeExt(KListView::NoSelection);
    m_metadata->setDefaultRenameAction(QListView::Accept);
    m_metadata->setShowSortIndicator(true);

    Rosegarden::Configuration &metadata = doc->getComposition().getMetadata();
    std::vector<std::string> names(metadata.getPropertyNames());
    for (unsigned int i = 0; i < names.size(); ++i) {
        QString name(strtoqstr(names[i]));
        // property names stored in lower case
        name = name.left(1).upper() + name.right(name.length()-1);
        new KListViewItem(m_metadata, name,
                          strtoqstr(metadata.get<String>(names[i])));
    }

    layout->addMultiCellWidget(m_metadata, 0, 0, 0, 1);

    QPushButton* addPropButton = new QPushButton(i18n("Add New Property"),
                                                 frame);
    layout->addWidget(addPropButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deletePropButton = new QPushButton(i18n("Delete Property"),
                                                    frame);
    layout->addWidget(deletePropButton, 1, 1, Qt::AlignHCenter);

    connect(addPropButton, SIGNAL(clicked()),
            this, SLOT(slotAddNewProperty()));

    connect(deletePropButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteProperty()));
    
    addTab(frame, i18n("Description"));

    Rosegarden::Composition &comp = doc->getComposition();
    std::set<Rosegarden::TrackId> usedTracks;
    
    int audioSegments = 0, internalSegments = 0;
    for (Rosegarden::Composition::iterator ci = comp.begin();
	 ci != comp.end(); ++ci) {
	usedTracks.insert((*ci)->getTrack());
	if ((*ci)->getType() == Rosegarden::Segment::Audio) ++audioSegments;
	else ++internalSegments;
    }

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame,
			     6, 2,
			     10, 5);

    layout->addWidget(new QLabel(i18n("Filename:"), frame), 0, 0);
    layout->addWidget(new QLabel(doc->getTitle(), frame), 0, 1);

    layout->addWidget(new QLabel(i18n("Formal duration (to end marker):"), frame), 1, 0);
    Rosegarden::timeT d = comp.getEndMarker();
    Rosegarden::RealTime rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 1, 1);

    layout->addWidget(new QLabel(i18n("Playing duration:"), frame), 2, 0);
    d = comp.getDuration();
    rtd = comp.getElapsedRealTime(d);
    layout->addWidget(new QLabel(durationToString(comp, 0, d, rtd), frame), 2, 1);

    layout->addWidget(new QLabel(i18n("Tracks:"), frame), 3, 0);
    layout->addWidget(new QLabel(i18n("%1 used, %2 total")
				 .arg(usedTracks.size())
                                 .arg(comp.getNbTracks()),
                                 frame), 3, 1);

    layout->addWidget(new QLabel(i18n("Segments:"), frame), 4, 0);
    layout->addWidget(new QLabel(i18n("%1 MIDI, %2 audio, %3 total")
				 .arg(internalSegments)
				 .arg(audioSegments)
                                 .arg(internalSegments + audioSegments),
                                 frame), 4, 1);
    
    addTab(frame, i18n("Statistics"));

    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 1, 1, 10, 5);

    QTable *table = new QTable(1, 11, frame, "Segment Table");
    table->setSelectionMode(QTable::NoSelection);
    table->horizontalHeader()->setLabel(0, i18n("Type"));
    table->horizontalHeader()->setLabel(1, i18n("Track"));
    table->horizontalHeader()->setLabel(2, i18n("Label"));
    table->horizontalHeader()->setLabel(3, i18n("Time"));
    table->horizontalHeader()->setLabel(4, i18n("Duration"));
    table->horizontalHeader()->setLabel(5, i18n("Events"));
    table->horizontalHeader()->setLabel(6, i18n("Polyphony"));
    table->horizontalHeader()->setLabel(7, i18n("Repeat"));
    table->horizontalHeader()->setLabel(8, i18n("Quantize"));
    table->horizontalHeader()->setLabel(9, i18n("Transpose"));
    table->horizontalHeader()->setLabel(10, i18n("Delay"));
    table->setNumRows(audioSegments + internalSegments);

    table->setColumnWidth(0, 50);
    table->setColumnWidth(1, 50);
    table->setColumnWidth(2, 150);
    table->setColumnWidth(3, 80);
    table->setColumnWidth(4, 80);
    table->setColumnWidth(5, 80);
    table->setColumnWidth(6, 80);
    table->setColumnWidth(7, 80);
    table->setColumnWidth(8, 80);
    table->setColumnWidth(9, 80);
    table->setColumnWidth(10, 80);

    int i = 0;

    for (Rosegarden::Composition::iterator ci = comp.begin();
	 ci != comp.end(); ++ci) {

	Rosegarden::Segment *s = *ci;
	
	table->setItem(i, 0, new SegmentDataItem
		       (table,
			s->getType() == Rosegarden::Segment::Audio ?
			i18n("Audio") : i18n("MIDI")));

	table->setItem(i, 1, new SegmentDataItem
		       (table,
			QString("%1").arg(s->getTrack() + 1)));

	QPixmap colourPixmap(16, 16);
	Rosegarden::Colour colour =
	    comp.getSegmentColourMap().getColourByIndex(s->getColourIndex());
	colourPixmap.fill(RosegardenGUIColours::convertColour(colour));

	table->setItem(i, 2,
		       new QTableItem(table, QTableItem::Never,
				      strtoqstr(s->getLabel()),
				      colourPixmap));

	table->setItem(i, 3, new SegmentDataItem
		       (table,
			QString("%1").arg(s->getStartTime())));

	table->setItem(i, 4, new SegmentDataItem
		       (table,
			QString("%1").arg(s->getEndMarkerTime() -
					  s->getStartTime())));

	std::set<long> notesOn;
	std::multimap<Rosegarden::timeT, long> noteOffs;
	int events = 0, notes = 0, poly = 0, maxPoly = 0;

	for (Rosegarden::Segment::iterator si = s->begin();
	     s->isBeforeEndMarker(si); ++si) {
	    ++events;
	    if ((*si)->isa(Rosegarden::Note::EventType)) {
		++notes;
		Rosegarden::timeT startTime = (*si)->getAbsoluteTime();
		Rosegarden::timeT endTime = startTime + (*si)->getDuration();
		if (endTime == startTime) continue;
		while (!noteOffs.empty() &&
		       (startTime >= noteOffs.begin()->first)) {
		    notesOn.erase(noteOffs.begin()->second);
		    noteOffs.erase(noteOffs.begin());
		}
		long pitch = 0;
		(*si)->get<Int>(Rosegarden::BaseProperties::PITCH, pitch);
		notesOn.insert(pitch);
		noteOffs.insert(std::multimap<Rosegarden::timeT, long>::value_type(endTime, pitch));
		poly = notesOn.size();
		if (poly > maxPoly) maxPoly = poly;
	    }
	}

	table->setItem(i, 5, new SegmentDataItem
		       (table,
			QString("%1").arg(events)));

	table->setItem(i, 6, new SegmentDataItem
		       (table,
			QString("%1").arg(maxPoly)));

	table->setItem(i, 7, new SegmentDataItem
		       (table,
			s->isRepeating() ? i18n("Yes") : i18n("No")));

	Rosegarden::timeT discard;

	if (s->getQuantizer() && s->hasQuantization()) {
	    Rosegarden::timeT unit = s->getQuantizer()->getUnit();
	    table->setItem(i, 8, new SegmentDataItem
			   (table, 
			    NotationStrings::makeNoteMenuLabel
			    (unit, true, discard, false)));
	} else {
	    table->setItem(i, 8, new SegmentDataItem
			   (table, 
			    i18n("Off")));
	}

	table->setItem(i, 9, new SegmentDataItem
		       (table,
			QString("%1").arg(s->getTranspose())));

	if (s->getDelay() != 0) {
	    if (s->getRealTimeDelay() != Rosegarden::RealTime::zeroTime) {
		table->setItem(i, 10, new SegmentDataItem
			       (table,
				QString("%1 + %2 ms")
				.arg(NotationStrings::makeNoteMenuLabel
				     (s->getDelay(), true, discard, false))
				.arg(s->getRealTimeDelay().sec * 1000 +
				     s->getRealTimeDelay().msec())));
	    } else {
		table->setItem(i, 10, new SegmentDataItem
			       (table, 
				NotationStrings::makeNoteMenuLabel
				(s->getDelay(), true, discard, false)));
	    }
	} else if (s->getRealTimeDelay() != Rosegarden::RealTime::zeroTime) {
	    table->setItem(i, 10, new SegmentDataItem
			   (table, 
			    QString("%2 ms")
			    .arg(s->getRealTimeDelay().sec * 1000 +
				 s->getRealTimeDelay().msec())));
	} else {
	    table->setItem(i, 10, new SegmentDataItem
			   (table,
			    i18n("None")));
	}

	++i;
    }

    layout->addWidget(table, 0, 0);

    addTab(frame, i18n("Segment Summary"));

}

void
DocumentMetaConfigurationPage::slotAddNewProperty()
{
    QString propertyName;
    int i = 0;

    while (1) {
        propertyName =
	    (i > 0 ? i18n("{new property %1}").arg(i) : i18n("{new property}"));
        if (!m_doc->getComposition().getMetadata().has(qstrtostr(propertyName))) break;
        ++i;
    }

    new KListViewItem(m_metadata, propertyName, i18n("{undefined}"));
}

void
DocumentMetaConfigurationPage::slotDeleteProperty()
{
    delete m_metadata->currentItem();
}

void
DocumentMetaConfigurationPage::apply()
{
    Rosegarden::Configuration &metadata = m_doc->getComposition().getMetadata();
    metadata.clear();

    for (QListViewItem *item = m_metadata->firstChild();
         item != 0; item = item->nextSibling()) {
        
        metadata.set<String>(qstrtostr(item->text(0).lower()),
                             qstrtostr(item->text(1)));
    }

    m_doc->slotDocumentModified();
}


// ---------------- AudioConfigurationPage -------------------
//
AudioConfigurationPage::AudioConfigurationPage(RosegardenGUIDoc *doc,
                                               QWidget *parent,
                                               const char *name)
    : TabbedConfigurationPage(doc, parent, name)
{
    Rosegarden::AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 4, 3,
                                          10, 5);
    layout->addWidget(new QLabel(i18n("Audio file path:"), frame), 0, 0);
    m_path = new QLabel(QString(afm.getAudioPath().c_str()), frame);
    layout->addWidget(m_path, 0, 1);
    
    m_changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(m_changePathButton, 0, 2);

    m_diskSpace = new QLabel(frame);
    layout->addWidget(new QLabel(i18n("Disk space remaining:"), frame), 1, 0);
    layout->addWidget(m_diskSpace, 1, 1);

    m_minutesAtStereo = new QLabel(frame);
    layout->addWidget(
            new QLabel(i18n("Equivalent minutes of 16-bit stereo:"), 
            frame), 2, 0);

    layout->addWidget(m_minutesAtStereo, 2, 1, AlignCenter);

    calculateStats();

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, i18n("Modify audio path"));
}

void
AudioConfigurationPage::calculateStats()
{
    Rosegarden::DiskSpace *space;
    try
    {
        space = new Rosegarden::DiskSpace(m_path->text());
    }
    catch (QString e)
    {
        KMessageBox::error(this, e);
        return;
    }
    
    float mbSize = float(space->getFreeKBytes())/1024.0;
    QString mbSizeStr;
    mbSizeStr.sprintf("%10.3f", mbSize);
    m_diskSpace->setText(QString("%1 MB").arg(mbSizeStr));

    // Work out minutes of recordable stereo from centralised sample rate value
    //
    Rosegarden::AudioPluginManager *apm = m_doc->getPluginManager();

    if (apm == 0 || apm->getSampleRate() == 0)
    {
        m_minutesAtStereo->setText(i18n("<sample rate not available>"));
        return;
    }

    // Work out total bytes and divide this by the sample rate times the
    // number of channels (2) times the number of bytes per sample (2)
    // times 60 seconds.
    //
    float stereoMins = ( float(space->getFreeKBytes()) * 1024.0 ) / 
                       ( float(apm->getSampleRate()) * 2.0 * 2.0 * 60.0 );
    QString minsStr;
    minsStr.sprintf("%8.1f", stereoMins);

    m_minutesAtStereo->
        setText(QString("%1 %2 %3Hz").arg(minsStr)
                                     .arg(i18n("minutes at"))
                                     .arg(apm->getSampleRate()));
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
        calculateStats();
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
        m_doc->slotDocumentModified();
    }
}

ColourConfigurationPage::ColourConfigurationPage(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
    : TabbedConfigurationPage(doc, parent, name)
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 2, 2,
                                          10, 5);

    m_map = m_doc->getComposition().getSegmentColourMap();

    m_colourtable = new RosegardenColourTable(frame, m_map, m_listmap);
    m_colourtable->setFixedHeight(280);

    layout->addMultiCellWidget(m_colourtable, 0, 0, 0, 1);

    QPushButton* addColourButton = new QPushButton(i18n("Add New Color"),
                                                   frame);
    layout->addWidget(addColourButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deleteColourButton = new QPushButton(i18n("Delete Color"),
                                                      frame);
    layout->addWidget(deleteColourButton, 1, 1, Qt::AlignHCenter);

    connect(addColourButton, SIGNAL(clicked()),
            this, SLOT(slotAddNew()));

    connect(deleteColourButton, SIGNAL(clicked()),
            this, SLOT(slotDelete()));

    connect(this,  SIGNAL(docColoursChanged()),
            m_doc, SLOT(slotDocColoursChanged()));

    connect(m_colourtable, SIGNAL(entryTextChanged(unsigned int, QString)),
            this,  SLOT(slotTextChanged(unsigned int, QString)));

    connect(m_colourtable, SIGNAL(entryColourChanged(unsigned int, QColor)),
            this,  SLOT(slotColourChanged(unsigned int, QColor)));

    addTab(frame, i18n("Color Map"));

}

void
ColourConfigurationPage::slotTextChanged(unsigned int index, QString string)
{
    m_map.modifyNameByIndex(m_listmap[index], string.ascii());
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::slotColourChanged(unsigned int index, QColor color)
{
    m_map.modifyColourByIndex(m_listmap[index], RosegardenGUIColours::convertColour(color));
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::apply()
{
    SegmentColourMapCommand *command = new SegmentColourMapCommand(m_doc, m_map);
    m_doc->getCommandHistory()->addCommand(command);
    emit docColoursChanged();
}

void
ColourConfigurationPage::slotAddNew()
{
    QColor temp;

    bool ok = false;

    QString newName = KLineEditDlg::getText(i18n("New Color Name"), i18n("Enter new name"),
                                            i18n("New"), &ok);
    if ((ok == true) && (!newName.isEmpty()))
    {
        KColorDialog box(this, "", true);

        int result = box.getColor( temp );

        if (result == KColorDialog::Accepted)
        {
            Rosegarden::Colour temp2 = RosegardenGUIColours::convertColour(temp);
            m_map.addItem(temp2, qstrtostr(newName));
            m_colourtable->populate_table(m_map, m_listmap);
        }
    // Else we don't do anything as they either didn't give a name 
    //  or didn't give a colour
    }

}

void
ColourConfigurationPage::slotDelete()
{
    QTableSelection temp = m_colourtable->selection(0);

    if ((!temp.isActive()) || (temp.topRow()==0))
        return;

    unsigned int toDel = temp.topRow();

    m_map.deleteItemByIndex(m_listmap[toDel]);
    m_colourtable->populate_table(m_map, m_listmap);

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
    : ConfigureDialogBase(parent, name)
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
    page = new GeneralConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    connect(page, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SIGNAL(updateAutoSaveInterval(unsigned int)));

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

    // Notation Page
    pageWidget = addPage(NotationConfigurationPage::iconLabel(),
                         NotationConfigurationPage::title(),
                         loadIcon(NotationConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new NotationConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
/*
    // Matrix Page
    pageWidget = addPage(MatrixConfigurationPage::iconLabel(),
                         MatrixConfigurationPage::title(),
                         loadIcon(MatrixConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new MatrixConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Latency Page
    //
    pageWidget = addPage(LatencyConfigurationPage::iconLabel(),
                         LatencyConfigurationPage::title(),
                         loadIcon(LatencyConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new LatencyConfigurationPage(doc, cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
*/
}

//------------------------------------------------------------

DocumentConfigureDialog::DocumentConfigureDialog(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigureDialogBase(parent, name)
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

    // Colour Page
    pageWidget = addPage(ColourConfigurationPage::iconLabel(),
                         ColourConfigurationPage::title(),
                         loadIcon(ColourConfigurationPage::iconName()));

    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new ColourConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
}

}
