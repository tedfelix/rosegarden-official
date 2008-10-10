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


#include "NotationConfigurationPage.h"
#include <QLayout>

#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "commands/edit/PasteEventsCommand.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/HeadersGroup.h"
#include "gui/editors/notation/NotationHLayout.h"
#include "gui/editors/notation/NoteFontFactory.h"
#include "gui/editors/notation/NoteFont.h"
#include "gui/editors/notation/NoteFontMap.h"
#include "gui/editors/notation/NoteFontViewer.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "gui/widgets/QuantizeParameters.h"
#include "TabbedConfigurationPage.h"
#include <QComboBox>
#include <QSettings>
#include <kfontrequester.h>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QToolTip>
#include <QGridLayout>
#include <algorithm>

namespace Rosegarden
{

NotationConfigurationPage::NotationConfigurationPage(QWidget *parent,
        const char *name) :
        TabbedConfigurationPage(parent, name)
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    QFrame *frame;
    QGridLayout *layout;

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    int row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(i18n("Default layout mode"), frame), row, 0);

    m_layoutMode = new QComboBox(frame);
    m_layoutMode->setEditable(false);
    m_layoutMode->addItem(i18n("Linear layout"));
    m_layoutMode->addItem(i18n("Continuous page layout"));
    m_layoutMode->addItem(i18n("Multiple page layout"));
    int defaultLayoutMode = settings.value("layoutmode", 0).toInt() ;
    if (defaultLayoutMode >= 0 && defaultLayoutMode <= 2) {
        m_layoutMode->setCurrentIndex(defaultLayoutMode);
    }
    layout->addWidget(m_layoutMode, row, 1, row- row+1, 2);
    ++row;

    layout->addWidget(new QLabel(i18n("Default spacing"), frame), row, 0);

    m_spacing = new QComboBox(frame);
    m_spacing->setEditable(false);

    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    int defaultSpacing = settings.value("spacing", 100).toInt() ;

    for (std::vector<int>::iterator i = s.begin(); i != s.end(); ++i) {

        QString text("%1 %");
        if (*i == 100)
            text = i18n("%1 % (normal)");
        m_spacing->addItem(text.arg(*i));

        if (*i == defaultSpacing) {
            m_spacing->setCurrentIndex(m_spacing->count() - 1);
        }
    }

    layout->addWidget(m_spacing, row, 1, row- row+1, 2);

    ++row;

    layout->addWidget(new QLabel(i18n("Default duration factor"), frame), row, 0);

    m_proportion = new QComboBox(frame);
    m_proportion->setEditable(false);

    s = NotationHLayout::getAvailableProportions();
    int defaultProportion = settings.value("proportion", 60).toInt() ;

    for (std::vector<int>::iterator i = s.begin(); i != s.end(); ++i) {

        QString text = QString("%1 %").arg(*i);
        if (*i == 40)
            text = i18n("%1 % (normal)", *i);
        else if (*i == 0)
            text = i18n("None");
        else if (*i == 100)
            text = i18n("Full");
        m_proportion->addItem(text);

        if (*i == defaultProportion) {
            m_proportion->setCurrentIndex(m_proportion->count() - 1);
        }
    }

    layout->addWidget(m_proportion, row, 1, row- row+1, 2);
    ++row;

    layout->addWidget(new QLabel(i18n("Show track headers (linear layout only)"),
                                      frame), row, 0);

    m_showTrackHeaders = new QComboBox(frame);
    m_showTrackHeaders->setEditable(false);
    m_showTrackHeaders->addItem(i18n("Never"), HeadersGroup::ShowNever);
    m_showTrackHeaders->addItem(i18n("When needed"), HeadersGroup::ShowWhenNeeded);
    m_showTrackHeaders->addItem(i18n("Always"), HeadersGroup::ShowAlways);
    int defaultShowTrackHeaders = settings.value("shownotationheader", 
                                                 HeadersGroup::DefaultShowMode).toInt() ;
    if (HeadersGroup::isValidShowMode(defaultShowTrackHeaders)) {
        m_showTrackHeaders->setCurrentIndex(defaultShowTrackHeaders);
    }
    m_showTrackHeaders->setToolTip(QString(i18n(
        "\"Always\" and \"Never\" mean what they usually mean\n"
        "\"When needed\" means \"when staves are too many to all fit"
        " in the current window\"")));

    layout->addWidget(m_showTrackHeaders, row, 1, row- row+1, 2);
    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("Show non-notation events as question marks"), frame),
         row, 0, 1, 2);
    m_showUnknowns = new QCheckBox(frame);
    bool defaultShowUnknowns = qStrToBool( settings.value("showunknowns", "false" ) ) ;
    m_showUnknowns->setChecked(defaultShowUnknowns);
    layout->addWidget(m_showUnknowns, row, 2);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("Show notation-quantized notes in a different color"), frame),
         row, 0, 1, 2);
    m_colourQuantize = new QCheckBox(frame);
    bool defaultColourQuantize = qStrToBool( settings.value("colourquantize", "false" ) ) ;
    m_colourQuantize->setChecked(defaultColourQuantize);
    layout->addWidget(m_colourQuantize, row, 2);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("Show \"invisible\" events in grey"), frame),
         row, 0, 1, 2);
    m_showInvisibles = new QCheckBox(frame);
    bool defaultShowInvisibles = qStrToBool( settings.value("showinvisibles", "true" ) ) ;
    m_showInvisibles->setChecked(defaultShowInvisibles);
    layout->addWidget(m_showInvisibles, row, 2);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("Show notes outside suggested playable range in red"), frame),
         row, 0, 1, 2);
    m_showRanges = new QCheckBox(frame);
    bool defaultShowRanges = qStrToBool( settings.value("showranges", "true" ) ) ;
    m_showRanges->setChecked(defaultShowRanges);
    layout->addWidget(m_showRanges, row, 2);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("Highlight superimposed notes with a halo effect"), frame),
         row, 0, 1, 2);
    m_showCollisions = new QCheckBox(frame);
    bool defaultShowCollisions = qStrToBool( settings.value("showcollisions", "true" ) ) ;
    m_showCollisions->setChecked(defaultShowCollisions);
    layout->addWidget(m_showCollisions, row, 2);
    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    layout->addWidget
        (new QLabel
         (i18n("When recording MIDI, split-and-tie long notes at barlines"), frame),
         row, 0, 1, 2);
    m_splitAndTie = new QCheckBox(frame);
    bool defaultSplitAndTie = qStrToBool( settings.value("quantizemakeviable", "false" ) ) ;
    m_splitAndTie->setChecked(defaultSplitAndTie);
    layout->addWidget(m_splitAndTie, row, 2);
    ++row;

    layout->setRowStretch(row, 10);
    frame->setLayout(layout);

    addTab(frame, i18n("Layout"));

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget
        (new QLabel(i18n("Default note style for new notes"), frame),
         row, 0, 1, 2);

    layout->setColumnStretch(2, 10);

    m_noteStyle = new QComboBox(frame);
    m_noteStyle->setEditable(false);
    m_untranslatedNoteStyle.clear();

    QString defaultStyle = settings.value("style", strtoqstr(NoteStyleFactory::DefaultStyle)).toString();
    std::vector<NoteStyleName> styles
    (NoteStyleFactory::getAvailableStyleNames());

    for (std::vector<NoteStyleName>::iterator i = styles.begin();
            i != styles.end(); ++i) {

        QString styleQName(strtoqstr(*i));
        m_untranslatedNoteStyle.append(styleQName);
        m_noteStyle->addItem(i18n(styleQName.toUtf8()));
        if (styleQName == defaultStyle) {
            m_noteStyle->setCurrentIndex(m_noteStyle->count() - 1);
        }
    }

    layout->addWidget(m_noteStyle, row, 2);
    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    layout->addWidget
    (new QLabel(i18n("When inserting notes..."), frame), row, 0);

    int defaultInsertType = settings.value("inserttype", 0).toInt() ;

    m_insertType = new QComboBox(frame);
    m_insertType->setEditable(false);
    m_insertType->addItem
    (i18n("Split notes into ties to make durations match"));
    m_insertType->addItem(i18n("Ignore existing durations"));
    m_insertType->setCurrentIndex(defaultInsertType);

    layout->addWidget(m_insertType, row, 1, row- row+1, 2);
    ++row;

    bool autoBeam = qStrToBool( settings.value("autobeam", "true" ) ) ;

    layout->addWidget
        (new QLabel
         (i18n("Auto-beam on insert when appropriate"), frame),
         row, 0, 1, 2);
    m_autoBeam = new QCheckBox(frame);
    m_autoBeam->setChecked(autoBeam);
    layout->addWidget(m_autoBeam, row, 2, row- row+1, 1);

    ++row;

    bool collapse = qStrToBool( settings.value("collapse", "false" ) ) ;

    layout->addWidget
        (new QLabel
         (i18n("Collapse rests after erase"), frame),
         row, 0, 1, 2);
    m_collapseRests = new QCheckBox(frame);
    m_collapseRests->setChecked(collapse);
    layout->addWidget(m_collapseRests, row, 2, row- row+1, 1);
    ++row;

    layout->setRowMinimumHeight(row, 20);
    ++row;

    layout->addWidget
    (new QLabel(i18n("Default paste type"), frame), row, 0);

    m_pasteType = new QComboBox(frame);
    m_pasteType->setEditable(false);

    unsigned int defaultPasteType = settings.value("pastetype",
            PasteEventsCommand::Restricted).toUInt();

    PasteEventsCommand::PasteTypeMap pasteTypes =
        PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
            i != pasteTypes.end(); ++i) {
        m_pasteType->addItem(i->second);
    }

    m_pasteType->setCurrentIndex(defaultPasteType);
    layout->addWidget(m_pasteType, row, 1, row- row+1, 2);
    ++row;

    layout->setRowStretch(row, 10);
    frame->setLayout(layout);

    addTab(frame, i18n("Editing"));

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);
    
    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(i18n("Accidentals in one octave..."), frame), row, 0);
    m_accOctavePolicy = new QComboBox(frame);
    m_accOctavePolicy->addItem(i18n("Affect only that octave"));
    m_accOctavePolicy->addItem(i18n("Require cautionaries in other octaves"));
    m_accOctavePolicy->addItem(i18n("Affect all subsequent octaves"));
    int accOctaveMode = settings.value("accidentaloctavemode", 1).toInt() ;
    if (accOctaveMode >= 0 && accOctaveMode < 3) {
        m_accOctavePolicy->setCurrentIndex(accOctaveMode);
    }
    layout->addWidget(m_accOctavePolicy, row, 1);
    ++row;

    layout->addWidget(new QLabel(i18n("Accidentals in one bar..."), frame), row, 0);
    m_accBarPolicy = new QComboBox(frame);
    m_accBarPolicy->addItem(i18n("Affect only that bar"));
    m_accBarPolicy->addItem(i18n("Require cautionary resets in following bar"));
    m_accBarPolicy->addItem(i18n("Require explicit resets in following bar"));
    int accBarMode = settings.value("accidentalbarmode", 0).toInt() ;
    if (accBarMode >= 0 && accBarMode < 3) {
        m_accBarPolicy->setCurrentIndex(accBarMode);
    }
    layout->addWidget(m_accBarPolicy, row, 1);
    ++row;

    layout->addWidget(new QLabel(i18n("Key signature cancellation style"), frame), row, 0);
    m_keySigCancelMode = new QComboBox(frame);
    m_keySigCancelMode->addItem(i18n("Cancel only when entering C major or A minor"));
    m_keySigCancelMode->addItem(i18n("Cancel whenever removing sharps or flats"));
    m_keySigCancelMode->addItem(i18n("Cancel always"));
    int cancelMode = settings.value("keysigcancelmode", 1).toInt() ;
    if (cancelMode >= 0 && cancelMode < 3) {
        m_keySigCancelMode->setCurrentIndex(cancelMode);
    }
    layout->addWidget(m_keySigCancelMode, row, 1);
    ++row;

    layout->setRowStretch(row, 10);
    frame->setLayout(layout);

    addTab(frame, i18n("Accidentals"));

/*
    QString preamble =
        (i18n("Rosegarden can apply automatic quantization to recorded "
              "or imported MIDI data for notation purposes only. "
              "This does not affect playback, and does not affect "
              "editing in any of the views except notation."));

    // force to default of 2 if not used before
    int quantizeType = settings.value("quantizetype", 2).toInt() ;
    settings.setValue("quantizetype", quantizeType);
    settings.setValue("quantizenotationonly", true);

    m_quantizeFrame = new QuantizeParameters
                      (m_tabWidget, QuantizeParameters::Notation,
                       false, false, "Notation Options", preamble);

    addTab(m_quantizeFrame, i18n("Quantize"));
*/
    row = 0;

//    QFrame *mainFrame = new QFrame(m_tabWidget);
//    QGridLayout *mainLayout = new QGridLayout(mainFrame, 2, 4, 10, 5);

//    QGroupBox *noteFontBox = new QGroupBox(1, Horizontal, i18n("Notation font"), mainFrame);
//    QGroupBox *otherFontBox = new QGroupBox(1, Horizontal, i18n("Other fonts"), mainFrame);
//    QGroupBox *descriptionBox = new QGroupBox(1, Horizontal, i18n("Description"), mainFrame);

//    mainLayout->addWidget(noteFontBox, 0, 0);
//    mainLayout->addWidget(otherFontBox, 1, 0);

//    QFrame *mainFrame = new QFrame(m_tabWidget);
    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

//    frame = new QFrame(noteFontBox);
//    layout = new QGridLayout(frame, 5, 2, 10, 5);

    m_viewButton = 0;

    layout->addWidget(new QLabel(i18n("Notation font"), frame), 0, 0);

    m_font = new QComboBox(frame);

#ifdef HAVE_XFT
    m_viewButton = new QPushButton(i18n("View"), frame);
    layout->addWidget(m_font, row, 1, row- row+1, 2);
    layout->addWidget(m_viewButton, row, 3);
    QObject::connect(m_viewButton, SIGNAL(clicked()),
                     this, SLOT(slotViewButtonPressed()));
#else
    layout->addWidget(m_font, row, 1, row- row+1, 3);
#endif
    m_font->setEditable(false);
    QObject::connect(m_font, SIGNAL(activated(int)),
                     this, SLOT(slotFontComboChanged(int)));
    ++row;

    QFrame *subFrame = new QFrame(frame);
    subFrame->setContentsMargins(12, 12, 12, 12);
    QGridLayout *subLayout = new QGridLayout(subFrame);
    subLayout->setSpacing(2);

    QFont font = m_font->font();
    font.setPointSize((font.pointSize() * 9) / 10);

    QLabel *originLabel = new QLabel(i18n("Origin:"), subFrame);
    originLabel->setFont(font);
    subLayout->addWidget(originLabel, 0, 0);

    QLabel *copyrightLabel = new QLabel(i18n("Copyright:"), subFrame);
    copyrightLabel->setFont(font);
    subLayout->addWidget(copyrightLabel, 1, 0);

    QLabel *mappedLabel = new QLabel(i18n("Mapped by:"), subFrame); 
    mappedLabel->setFont(font);
    subLayout->addWidget(mappedLabel, 2, 0);

    QLabel *typeLabel = new QLabel(i18n("Type:"), subFrame);
    typeLabel->setFont(font);
    subLayout->addWidget(typeLabel, 3, 0);

    m_fontOriginLabel = new QLabel(subFrame);
    m_fontOriginLabel->setWordWrap(true);
    m_fontOriginLabel->setFont(font);
//    m_fontOriginLabel->setFixedWidth(250);
    m_fontCopyrightLabel = new QLabel(subFrame);
    m_fontCopyrightLabel->setWordWrap(true);
    m_fontCopyrightLabel->setFont(font);
//    m_fontCopyrightLabel->setFixedWidth(250);
    m_fontMappedByLabel = new QLabel(subFrame);
    m_fontMappedByLabel->setFont(font);
    m_fontTypeLabel = new QLabel(subFrame);
    m_fontTypeLabel->setFont(font);
    subLayout->addWidget(m_fontOriginLabel, 0, 1);
    subLayout->addWidget(m_fontCopyrightLabel, 1, 1);
    subLayout->addWidget(m_fontMappedByLabel, 2, 1);
    subLayout->addWidget(m_fontTypeLabel, 3, 1);

    subLayout->setColumnStretch(1, 10);
    subFrame->setLayout(subLayout);

    layout->addWidget(subFrame,
                               row,
                               0, row-
                               row+1, 3-
                               1);
    ++row;

    layout->addWidget
        (new QLabel(i18n("Font size for single-staff views"), frame),
         row, 0, 1, 2);
    m_singleStaffSize = new QComboBox(frame);
    m_singleStaffSize->setEditable(false);
    layout->addWidget(m_singleStaffSize, row, 2, row- row+1, 1);
    ++row;

    layout->addWidget
        (new QLabel(i18n("Font size for multi-staff views"), frame),
         row, 0, 1, 2);
    m_multiStaffSize = new QComboBox(frame);
    m_multiStaffSize->setEditable(false);
    layout->addWidget(m_multiStaffSize, row, 2, row- row+1, 1);
    ++row;

    layout->addWidget
        (new QLabel(i18n("Font size for printing (pt)"), frame),
         row, 0, 1, 2);
    m_printingSize = new QComboBox(frame);
    m_printingSize->setEditable(false);
    layout->addWidget(m_printingSize, row, 2, row- row+1, 1);
    ++row;

    slotPopulateFontCombo(false);

    layout->setRowMinimumHeight(row, 15);
    ++row;

    QFont defaultTextFont(NotePixmapFactory::defaultSerifFontFamily),
        defaultSansFont(NotePixmapFactory::defaultSansSerifFontFamily),
        defaultTimeSigFont(NotePixmapFactory::defaultTimeSigFontFamily);

    layout->addWidget
        (new QLabel(i18n("Text font"), frame), row, 0);
    m_textFont = new KFontRequester(frame);
    QFont textFont = defaultTextFont;
    QVariant fv = settings.value("textfont", textFont);
    if (fv.canConvert(QVariant::Font)) textFont = fv.value<QFont>();
    m_textFont->setFont(textFont);
    layout->addWidget(m_textFont, row, 1, row- row+1, 3);
    ++row;

    layout->addWidget
        (new QLabel(i18n("Sans-serif font"), frame), row, 0);
    m_sansFont = new KFontRequester(frame);
    QFont sansFont = defaultTextFont;
    fv = settings.value("sansfont", sansFont);
    if (fv.canConvert(QVariant::Font)) sansFont = fv.value<QFont>();
    m_sansFont->setFont(sansFont);
    layout->addWidget(m_sansFont, row, 1, row- row+1, 3);
    ++row;

/*!!! No -- not much point in having the time sig font here: it's only
 * used if the time sig characters are not found in the notation font,
 * and our default notation font has all the characters we need

    layout->addWidget
        (new QLabel(i18n("Time Signature font"), frame), row, 0);
    m_timeSigFont = new KFontRequester(frame);
    QFont timeSigFont = settings->readFontEntry("timesigfont", &defaultTimeSigFont);
    m_timeSigFont->setFont(timeSigFont);
    layout->addWidget(m_timeSigFont, row, 1, row- row+1, 3);
    ++row;
*/

    frame->setLayout(layout);

//    addTab(mainFrame, i18n("Font"));
    addTab(frame, i18n("Font"));
    settings.endGroup();
}

void
NotationConfigurationPage::slotViewButtonPressed()
{
#ifdef HAVE_XFT
    std::string fontName = qstrtostr(m_untranslatedFont[m_font->currentIndex()]);

    try {
        NoteFont *noteFont = NoteFontFactory::getFont
                             (fontName, NoteFontFactory::getDefaultSize(fontName));
        const NoteFontMap &map(noteFont->getNoteFontMap());
        QStringList systemFontNames(map.getSystemFontNames());
        if (systemFontNames.count() == 0) {
            m_viewButton->setEnabled(false); // oops
        } else {
            NoteFontViewer *viewer =
                new NoteFontViewer(0, m_untranslatedFont[m_font->currentIndex()],
                                   systemFontNames, 24);
            (void)viewer->exec(); // no return value
        }
    } catch (Exception f) {
        QMessageBox::critical(0, "", i18n(f.getMessage().c_str()) );
    }
#endif
}

void
NotationConfigurationPage::slotPopulateFontCombo(bool rescan)
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    QString defaultFont = settings.value("notefont",
            strtoqstr(NoteFontFactory::getDefaultFontName())).toString();

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
        m_font->addItem(i18n(s.toUtf8()));
        if (s == defaultFont)
            m_font->setCurrentIndex(m_font->count() - 1);
    }

    slotFontComboChanged(m_font->currentIndex());
}

void
NotationConfigurationPage::slotFontComboChanged(int index)
{
    std::string fontStr = qstrtostr(m_untranslatedFont[index]);

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    populateSizeCombo(m_singleStaffSize, fontStr, settings.value(
            "singlestaffnotesize",
            NoteFontFactory::getDefaultSize(fontStr)).toInt());
    populateSizeCombo(m_multiStaffSize, fontStr, settings.value(
            "multistaffnotesize",
            NoteFontFactory::getDefaultSize(fontStr)).toInt());

    int printpt = settings.value("printingnotesize", 5).toUInt() ;
    for (int i = 2; i < 16; ++i) {
        m_printingSize->addItem(QString("%1").arg(i));
        if (i == printpt) {
            m_printingSize->setCurrentIndex(m_printingSize->count() - 1);
        }
    }

    try {
        NoteFont *noteFont = NoteFontFactory::getFont
                             (fontStr, NoteFontFactory::getDefaultSize(fontStr));
        const NoteFontMap &map(noteFont->getNoteFontMap());
		m_fontOriginLabel->setText( i18n( map.getOrigin().c_str() ) );
		m_fontCopyrightLabel->setText( i18n( map.getCopyright().c_str() ) );
		m_fontMappedByLabel->setText( i18n( map.getMappedBy().c_str() ) );
        if (map.isSmooth()) {
            m_fontTypeLabel->setText(
									 i18n("%1 (smooth)", i18n( map.getType().c_str() )) );
        } else {
            m_fontTypeLabel->setText(
									 i18n("%1 (jaggy)", i18n( map.getType().c_str() )) );
        }
        if (m_viewButton) {
            m_viewButton->setEnabled(map.getSystemFontNames().count() > 0);
        }
    } catch (Exception f) {
		QMessageBox::critical(0, "", i18n( f.getMessage().c_str() ) );
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
        combo->addItem(QString("%1").arg(*i));
        if (*i == defaultSize)
            combo->setCurrentIndex(combo->count() - 1);
    }
}

void
NotationConfigurationPage::apply()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("notefont", m_untranslatedFont[m_font->currentIndex()]);
    settings.setValue("singlestaffnotesize",
                      m_singleStaffSize->currentText().toUInt());
    settings.setValue("multistaffnotesize",
                      m_multiStaffSize->currentText().toUInt());
    settings.setValue("printingnotesize",
                      m_printingSize->currentText().toUInt());
    settings.setValue("textfont",
                      m_textFont->font());
    settings.setValue("sansfont",
                      m_sansFont->font());
/*!!!
    settings.setValue("timesigfont",
                      m_timeSigFont->font());
*/
    std::vector<int> s = NotationHLayout::getAvailableSpacings();
    settings.setValue("spacing", s[m_spacing->currentIndex()]);

    s = NotationHLayout::getAvailableProportions();
    settings.setValue("proportion", s[m_proportion->currentIndex()]);

    settings.setValue("layoutmode", m_layoutMode->currentIndex());
    settings.setValue("colourquantize", m_colourQuantize->isChecked());
    settings.setValue("showunknowns", m_showUnknowns->isChecked());
    settings.setValue("showinvisibles", m_showInvisibles->isChecked());
    settings.setValue("showranges", m_showRanges->isChecked());
    settings.setValue("showcollisions", m_showCollisions->isChecked());
    settings.setValue("shownotationheader",
                       m_showTrackHeaders->currentIndex());
    settings.setValue("style", m_untranslatedNoteStyle[m_noteStyle->currentIndex()]);
    settings.setValue("inserttype", m_insertType->currentIndex());
    settings.setValue("autobeam", m_autoBeam->isChecked());
    settings.setValue("collapse", m_collapseRests->isChecked());
    settings.setValue("pastetype", m_pasteType->currentIndex());
    settings.setValue("accidentaloctavemode", m_accOctavePolicy->currentIndex());
    settings.setValue("accidentalbarmode", m_accBarPolicy->currentIndex());
    settings.setValue("keysigcancelmode", m_keySigCancelMode->currentIndex());

    settings.setValue("quantizemakeviable", m_splitAndTie->isChecked());

//    (void)m_quantizeFrame->getQuantizer(); // this also writes to the config
    settings.endGroup();
}

}
#include "NotationConfigurationPage.moc"
