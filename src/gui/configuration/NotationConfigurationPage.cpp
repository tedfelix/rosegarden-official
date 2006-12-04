/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "NotationConfigurationPage.h"
#include <qlayout.h>

#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "commands/edit/PasteEventsCommand.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/NotationHLayout.h"
#include "gui/editors/notation/NoteFontFactory.h"
#include "gui/editors/notation/NoteFont.h"
#include "gui/editors/notation/NoteFontMap.h"
#include "gui/editors/notation/NoteFontViewer.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "gui/widgets/QuantizeParameters.h"
#include "TabbedConfigurationPage.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kfontrequester.h>
#include <kmessagebox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfont.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qwidget.h>


namespace Rosegarden
{

NotationConfigurationPage::NotationConfigurationPage(KConfig *cfg,
        QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup(NotationViewConfigGroup);

    QFrame *mainFrame = new QFrame(m_tabWidget);
    QGridLayout *mainLayout = new QGridLayout(mainFrame, 2, 2, 10, 5);

    QGroupBox *noteFontBox = new QGroupBox(1, Horizontal, i18n("Notation font"), mainFrame);
    QGroupBox *otherFontBox = new QGroupBox(1, Horizontal, i18n("Other fonts"), mainFrame);
    QGroupBox *descriptionBox = new QGroupBox(1, Horizontal, i18n("Description"), mainFrame);

    mainLayout->addWidget(noteFontBox, 0, 0);
    mainLayout->addMultiCellWidget(otherFontBox, 1, 1, 0, 1);
    mainLayout->addWidget(descriptionBox, 0, 1);

    QFrame *frame = new QFrame(noteFontBox);
    QGridLayout *layout = new QGridLayout(frame, 5, 3, 10, 5);

    layout->addWidget
    (new QLabel(i18n("Font size for single-staff views"), frame),
     1, 0);
    layout->addWidget
    (new QLabel(i18n("Font size for multi-staff views"), frame),
     2, 0);
    layout->addWidget
    (new QLabel(i18n("Font size for printing (pt)"), frame),
     3, 0);

    QFrame *subFrame = new QFrame(descriptionBox);
    QGridLayout *subLayout = new QGridLayout(subFrame,
                             4, 2,  // nbrow, nbcol
                             12, 2);

    subLayout->addWidget(new QLabel(i18n("Origin:"), subFrame), 0, 0);
    subLayout->addWidget(new QLabel(i18n("Copyright:"), subFrame), 1, 0);
    subLayout->addWidget(new QLabel(i18n("Mapped by:"), subFrame), 2, 0);
    subLayout->addWidget(new QLabel(i18n("Type:"), subFrame), 3, 0);
    m_fontOriginLabel = new QLabel(subFrame);
    m_fontOriginLabel->setAlignment(Qt::WordBreak);
    m_fontOriginLabel->setFixedWidth(250);
    m_fontCopyrightLabel = new QLabel(subFrame);
    m_fontCopyrightLabel->setAlignment(Qt::WordBreak);
    m_fontCopyrightLabel->setFixedWidth(250);
    m_fontMappedByLabel = new QLabel(subFrame);
    m_fontTypeLabel = new QLabel(subFrame);
    subLayout->addWidget(m_fontOriginLabel, 0, 1);
    subLayout->addWidget(m_fontCopyrightLabel, 1, 1);
    subLayout->addWidget(m_fontMappedByLabel, 2, 1);
    subLayout->addWidget(m_fontTypeLabel, 3, 1);

    //    layout->addMultiCellWidget(subFrame,
    //                               0, 3,
    //                               2, 2);

    m_viewButton = 0;

    m_font = new KComboBox(frame);

#ifdef HAVE_XFT

    m_viewButton = new QPushButton(i18n("View"), frame);
    layout->addWidget(m_font, 0, 0);
    layout->addWidget(m_viewButton, 0, 1);
    QObject::connect(m_viewButton, SIGNAL(clicked()),
                     this, SLOT(slotViewButtonPressed()));
#else

    layout->addMultiCellWidget(m_font, 0, 0, 0, 1);
#endif

    QPushButton *rescanButton = new QPushButton(i18n("Rescan available fonts"), frame);
    layout->addWidget(rescanButton, 4, 0);
    QObject::connect(rescanButton, SIGNAL(clicked()),
                     this, SLOT(slotRescanFonts()));


    /*
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
    */
    m_font->setEditable(false);

    m_singleStaffSize = new KComboBox(frame);
    m_singleStaffSize->setEditable(false);

    m_multiStaffSize = new KComboBox(frame);
    m_multiStaffSize->setEditable(false);

    m_printingSize = new KComboBox(frame);
    m_printingSize->setEditable(false);

    slotPopulateFontCombo(false);

    QObject::connect(m_font, SIGNAL(activated(int)),
                     this, SLOT(slotFontComboChanged(int)));

    layout->addWidget(m_singleStaffSize, 1, 1);
    layout->addWidget(m_multiStaffSize, 2, 1);
    layout->addWidget(m_printingSize, 3, 1);

    frame = new QFrame(otherFontBox);
    QGridLayout *otherLayout = new QGridLayout(frame, 2, 2, 10, 5);

    otherLayout->addWidget
    (new QLabel(i18n("Text font"), frame),
     0, 0);
    otherLayout->addWidget
    (new QLabel(i18n("Time Signature font"), frame),
     1, 0);

    m_textFont = new KFontRequester(frame);
    m_timeSigFont = new KFontRequester(frame);

    QFont defaultTextFont(NotePixmapFactory::defaultSerifFontFamily),
    defaultTimeSigFont(NotePixmapFactory::defaultTimeSigFontFamily);

    QFont textFont = m_cfg->readFontEntry("textfont", &defaultTextFont);
    QFont timeSigFont = m_cfg->readFontEntry("timesigfont", &defaultTimeSigFont);

    m_textFont->setFont(textFont);
    m_timeSigFont->setFont(timeSigFont);

    otherLayout->addWidget(m_textFont, 0, 1);
    otherLayout->addWidget(m_timeSigFont, 1, 1);


    addTab(mainFrame, i18n("Font"));



    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 8, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Default layout mode"), frame), 0, 0);

    m_layoutMode = new KComboBox(frame);
    m_layoutMode->setEditable(false);
    m_layoutMode->insertItem(i18n("Linear layout"));
    m_layoutMode->insertItem(i18n("Continuous page layout"));
    m_layoutMode->insertItem(i18n("Multiple page layout"));
    int defaultLayoutMode = m_cfg->readNumEntry("layoutmode", 0);
    if (defaultLayoutMode >= 0 && defaultLayoutMode <= 2) {
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
        if (*i == 100)
            text = "%1 % (normal)";
        m_spacing->insertItem(text.arg(*i));

        if (*i == defaultSpacing) {
            m_spacing->setCurrentItem(m_spacing->count() - 1);
        }
    }

    layout->addWidget(m_spacing, 1, 1);

    layout->addWidget(new QLabel(i18n("Default duration factor"), frame), 2, 0);

    m_proportion = new KComboBox(frame);
    m_proportion->setEditable(false);

    s = NotationHLayout::getAvailableProportions();
    int defaultProportion = m_cfg->readNumEntry("proportion", 60);

    for (std::vector<int>::iterator i = s.begin(); i != s.end(); ++i) {

        QString text = QString("%1 %").arg(*i);
        if (*i == 40)
            text = "40 % (normal)";
        else if (*i == 0)
            text = i18n("None");
        else if (*i == 100)
            text = i18n("Full");
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
                       (i18n("Show notation-quantized notes in a different color"), frame);
    bool defaultColourQuantize = m_cfg->readBoolEntry("colourquantize", false);
    m_colourQuantize->setChecked(defaultColourQuantize);
    layout->addWidget(m_colourQuantize, 4, 1);

    m_showInvisibles = new QCheckBox
                       (i18n("Show \"invisible\" events in grey"), frame);
    bool defaultShowInvisibles = m_cfg->readBoolEntry("showinvisibles", true);
    m_showInvisibles->setChecked(defaultShowInvisibles);
    layout->addWidget(m_showInvisibles, 5, 1);

    m_showRanges = new QCheckBox
                   (i18n("Show notes outside suggested playable range in red"), frame);
    bool defaultShowRanges = m_cfg->readBoolEntry("showranges", true);
    m_showRanges->setChecked(defaultShowRanges);
    layout->addWidget(m_showRanges, 6, 1);

    addTab(frame, i18n("Layout"));



    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 6, 2, 10, 5);

    layout->addWidget
    (new QLabel(i18n("Default note style for new notes"), frame), 0, 0);

    m_noteStyle = new KComboBox(frame);
    m_noteStyle->setEditable(false);
    m_untranslatedNoteStyle.clear();

    QString defaultStyle =
        m_cfg->readEntry("style", strtoqstr(NoteStyleFactory::DefaultStyle));
    std::vector<NoteStyleName> styles
    (NoteStyleFactory::getAvailableStyleNames());

    for (std::vector<NoteStyleName>::iterator i = styles.begin();
            i != styles.end(); ++i) {

        QString styleQName(strtoqstr(*i));
        m_untranslatedNoteStyle.append(styleQName);
        m_noteStyle->insertItem(i18n(styleQName.utf8()));
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



    frame = new QFrame(m_tabWidget);
    layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Accidentals in one octave..."), frame), 0, 0);
    m_accOctavePolicy = new KComboBox(frame);
    m_accOctavePolicy->insertItem(i18n("Affect only that octave"));
    m_accOctavePolicy->insertItem(i18n("Require cautionaries in other octaves"));
    m_accOctavePolicy->insertItem(i18n("Affect all subsequent octaves"));
    int accOctaveMode = m_cfg->readNumEntry("accidentaloctavemode", 1);
    if (accOctaveMode >= 0 && accOctaveMode < 3) {
        m_accOctavePolicy->setCurrentItem(accOctaveMode);
    }
    layout->addWidget(m_accOctavePolicy, 0, 1);

    layout->addWidget(new QLabel(i18n("Accidentals in one bar..."), frame), 1, 0);
    m_accBarPolicy = new KComboBox(frame);
    m_accBarPolicy->insertItem(i18n("Affect only that bar"));
    m_accBarPolicy->insertItem(i18n("Require cautionary resets in following bar"));
    m_accBarPolicy->insertItem(i18n("Require explicit resets in following bar"));
    int accBarMode = m_cfg->readNumEntry("accidentalbarmode", 0);
    if (accBarMode >= 0 && accBarMode < 3) {
        m_accBarPolicy->setCurrentItem(accBarMode);
    }
    layout->addWidget(m_accBarPolicy, 1, 1);

    layout->addWidget(new QLabel(i18n("Key signature cancellation style:"), frame), 2, 0);
    m_keySigCancelMode = new KComboBox(frame);
    m_keySigCancelMode->insertItem(i18n("Cancel only when entering C major or A minor"));
    m_keySigCancelMode->insertItem(i18n("Cancel whenever removing sharps or flats"));
    m_keySigCancelMode->insertItem(i18n("Cancel always"));
    int cancelMode = m_cfg->readNumEntry("keysigcancelmode", 1);
    if (cancelMode >= 0 && cancelMode < 3) {
        m_keySigCancelMode->setCurrentItem(cancelMode);
    }
    layout->addWidget(m_keySigCancelMode, 2, 1);

    addTab(frame, i18n("Accidentals"));



    QString preamble =
        (i18n("Rosegarden can apply automatic quantization to recorded "
              "or imported MIDI data for notation purposes only. "
              "This does not affect playback, and does not affect "
              "editing in any of the views except notation."));

    // force to default of 2 if not used before
    int quantizeType = m_cfg->readNumEntry("quantizetype", 2);
    m_cfg->writeEntry("quantizetype", quantizeType);
    m_cfg->writeEntry("quantizenotationonly", true);

    m_quantizeFrame = new QuantizeParameters
                      (m_tabWidget, QuantizeParameters::Notation,
                       false, false, "Notation Options", preamble);

    addTab(m_quantizeFrame, i18n("Quantize"));
}

void
NotationConfigurationPage::slotViewButtonPressed()
{
#ifdef HAVE_XFT
    std::string fontName = qstrtostr(m_untranslatedFont[m_font->currentItem()]);

    try {
        NoteFont *noteFont = NoteFontFactory::getFont
                             (fontName, NoteFontFactory::getDefaultSize(fontName));
        const NoteFontMap &map(noteFont->getNoteFontMap());
        QStringList systemFontNames(map.getSystemFontNames());
        if (systemFontNames.count() == 0) {
            m_viewButton->setEnabled(false); // oops
        } else {
            NoteFontViewer *viewer =
                new NoteFontViewer(0, m_untranslatedFont[m_font->currentItem()],
                                   systemFontNames, 24);
            (void)viewer->exec(); // no return value
        }
    } catch (Exception f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
    }
#endif
}

void
NotationConfigurationPage::slotRescanFonts()
{
    slotPopulateFontCombo(true);
}

void
NotationConfigurationPage::slotPopulateFontCombo(bool rescan)
{
    QString defaultFont = m_cfg->readEntry
                          ("notefont", strtoqstr(NoteFontFactory::getDefaultFontName()));

    try {
        (void)NoteFontFactory::getFont
        (qstrtostr(defaultFont),
         NoteFontFactory::getDefaultSize(qstrtostr(defaultFont)));
    } catch (Exception e) {
        defaultFont = strtoqstr(NoteFontFactory::getDefaultFontName());
    }

    std::set
        <std::string> fs(NoteFontFactory::getFontNames(rescan));
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    m_untranslatedFont.clear();
    m_font->clear();

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {
        QString s(strtoqstr(*i));
        m_untranslatedFont.append(s);
        m_font->insertItem(i18n(s.utf8()));
        if (s == defaultFont)
            m_font->setCurrentItem(m_font->count() - 1);
    }

    slotFontComboChanged(m_font->currentItem());
}

void
NotationConfigurationPage::slotFontComboChanged(int index)
{
    std::string fontStr = qstrtostr(m_untranslatedFont[index]);

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
            m_printingSize->setCurrentItem(m_printingSize->count() - 1);
        }
    }

    try {
        NoteFont *noteFont = NoteFontFactory::getFont
                             (fontStr, NoteFontFactory::getDefaultSize(fontStr));
        const NoteFontMap &map(noteFont->getNoteFontMap());
        m_fontOriginLabel->setText(i18n(strtoqstr(map.getOrigin())));
        m_fontCopyrightLabel->setText(i18n(strtoqstr(map.getCopyright())));
        m_fontMappedByLabel->setText(i18n(strtoqstr(map.getMappedBy())));
        if (map.isSmooth()) {
            m_fontTypeLabel->setText(
                i18n("%1 (smooth)").arg(i18n(strtoqstr(map.getType()))));
        } else {
            m_fontTypeLabel->setText(
                i18n("%1 (jaggy)").arg(i18n(strtoqstr(map.getType()))));
        }
        if (m_viewButton) {
            m_viewButton->setEnabled(map.getSystemFontNames().count() > 0);
        }
    } catch (Exception f) {
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
        if (*i == defaultSize)
            combo->setCurrentItem(combo->count() - 1);
    }
}

void
NotationConfigurationPage::apply()
{
    m_cfg->setGroup(NotationViewConfigGroup);

    m_cfg->writeEntry("notefont", m_untranslatedFont[m_font->currentItem()]);
    m_cfg->writeEntry("singlestaffnotesize",
                      m_singleStaffSize->currentText().toUInt());
    m_cfg->writeEntry("multistaffnotesize",
                      m_multiStaffSize->currentText().toUInt());
    m_cfg->writeEntry("printingnotesize",
                      m_printingSize->currentText().toUInt());
    m_cfg->writeEntry("textfont",
                      m_textFont->font());
    m_cfg->writeEntry("timesigfont",
                      m_timeSigFont->font());

    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    m_cfg->writeEntry("spacing", s[m_spacing->currentItem()]);

    s = NotationHLayout::getAvailableProportions();
    m_cfg->writeEntry("proportion", s[m_proportion->currentItem()]);

    m_cfg->writeEntry("layoutmode", m_layoutMode->currentItem());
    m_cfg->writeEntry("colourquantize", m_colourQuantize->isChecked());
    m_cfg->writeEntry("showunknowns", m_showUnknowns->isChecked());
    m_cfg->writeEntry("showinvisibles", m_showInvisibles->isChecked());
    m_cfg->writeEntry("showranges", m_showRanges->isChecked());
    m_cfg->writeEntry("style", m_untranslatedNoteStyle[m_noteStyle->currentItem()]);
    m_cfg->writeEntry("inserttype", m_insertType->currentItem());
    m_cfg->writeEntry("autobeam", m_autoBeam->isChecked());
    m_cfg->writeEntry("collapse", m_collapseRests->isChecked());
    m_cfg->writeEntry("pastetype", m_pasteType->currentItem());
    m_cfg->writeEntry("accidentaloctavemode", m_accOctavePolicy->currentItem());
    m_cfg->writeEntry("accidentalbarmode", m_accBarPolicy->currentItem());
    m_cfg->writeEntry("keysigcancelmode", m_keySigCancelMode->currentItem());

    (void)m_quantizeFrame->getQuantizer(); // this also writes to the config
}

}
#include "NotationConfigurationPage.moc"
