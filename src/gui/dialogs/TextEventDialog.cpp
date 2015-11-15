/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TextEventDialog.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/LineEdit.h"

#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QBitmap>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QApplication>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

TextEventDialog::TextEventDialog(QWidget *parent,
                                 NotePixmapFactory *npf,
                                 Text defaultText,
                                 int maxLength) :
        QDialog(parent),
        m_notePixmapFactory(npf),
        m_styles(Text::getUserStyles()) /*,
            //m_directives(Text::getLilyPondDirectives()) */
{
    setModal(true);
    setWindowTitle(tr("Text"));
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    QWidget *vbox = dynamic_cast<QWidget*>( this );
    vbox->setLayout( vboxLayout );

    QGroupBox *entryBox = new QGroupBox( tr("Specification"), vbox );
    vboxLayout->addWidget(entryBox);
    QGroupBox *exampleBox = new QGroupBox( tr("Preview"), vbox );
    QVBoxLayout *exampleBoxLayout = new QVBoxLayout;
    exampleBox->setLayout(exampleBoxLayout);

    // frame inside group box to contain white background
    QFrame *innerFrame = new QFrame;
    innerFrame->setStyleSheet("background: white");
    QVBoxLayout *innerFrameLayout = new QVBoxLayout;
    innerFrame->setLayout(innerFrameLayout);
    exampleBoxLayout->addWidget(innerFrame); 
    vboxLayout->addWidget(exampleBox);

    QGridLayout *entryGridLay = new QGridLayout;
    entryGridLay->addWidget(new QLabel(tr("Text:  ")), 0, 0);
    m_text = new LineEdit;
    m_text->setText(strtoqstr(defaultText.getText()));
    if (maxLength > 0) m_text->setMaxLength(maxLength);
    entryGridLay->addWidget(m_text, 0, 1);

    // style combo
    entryGridLay->addWidget(new QLabel(tr("Style:  ")), 1, 0);
    m_typeCombo = new QComboBox;
    entryGridLay->addWidget(m_typeCombo, 1, 1);

    // Optional widget
    m_optionLabel = new QStackedWidget;
    m_optionWidget = new QStackedWidget;
    m_optionLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    m_optionWidget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    entryGridLay->addWidget(m_optionLabel, 2, 0);
    entryGridLay->addWidget(m_optionWidget, 2, 1);

    m_blankLabel = new QLabel(" ");
    m_blankWidget = new QLabel(" ");
    m_optionLabel->addWidget(m_blankLabel);
    m_optionWidget->addWidget(m_blankWidget);
    m_optionLabel->setCurrentWidget(m_blankLabel);
    m_optionWidget->setCurrentWidget(m_blankWidget);

    for (unsigned int i = 0; i < m_styles.size(); ++i)
    {

        std::string style = m_styles[i];

        // if the style is in this list, we can tr it (kludgy):

        if (style == Text::Dynamic) {                           // index //
            m_typeCombo->addItem(tr("Dynamic"));           // 0

        } else if (style == Text::Direction) {
            m_typeCombo->addItem(tr("Direction"));         // 1

        } else if (style == Text::LocalDirection) {
            m_typeCombo->addItem(tr("Local Direction"));   // 2

        } else if (style == Text::Tempo) {
            m_typeCombo->addItem(tr("Tempo"));             // 3

        } else if (style == Text::LocalTempo) {
            m_typeCombo->addItem(tr("Local Tempo"));       // 4

        } else if (style == Text::Lyric) {
            m_typeCombo->addItem(tr("Lyric"));             // 5

        } else if (style == Text::Chord) {
            m_typeCombo->addItem(tr("Chord"));             // 6

        } else if (style == Text::Annotation) {
            m_typeCombo->addItem(tr("Annotation"));        // 7

        } else if (style == Text::LilyPondDirective) {
            m_typeCombo->addItem(tr("LilyPond Directive")); // 8

        } else {
            // not tr()-able

            std::string styleName;
            styleName += (char)toupper(style[0]);
            styleName += style.substr(1);

            int uindex = styleName.find('_');
            if (uindex > 0) {
                styleName =
                    styleName.substr(0, uindex) + " " +
                    styleName.substr(uindex + 1);
            }

            m_typeCombo->addItem(strtoqstr(styleName));
        }

        if (style == defaultText.getTextType()) {
            m_typeCombo->setCurrentIndex(m_typeCombo->count() - 1);
        }
    }

    m_verseLabel = new QLabel(tr("Verse:  "));
    m_optionLabel->addWidget(m_verseLabel);
    m_verseSpin = new QSpinBox;
    m_optionWidget->addWidget(m_verseSpin);
    m_verseSpin->setMinimum(1);
    m_verseSpin->setMaximum(12);
    m_verseSpin->setSingleStep(1);
    m_verseSpin->setValue(defaultText.getVerse() + 1);

    // dynamic shortcuts combo
    m_dynamicShortcutLabel = new QLabel(tr("Dynamic:  "));
    m_optionLabel->addWidget(m_dynamicShortcutLabel);

    m_dynamicShortcutCombo = new QComboBox;
    m_optionWidget->addWidget(m_dynamicShortcutCombo);
    m_dynamicShortcutCombo->addItem(tr("ppp"));
    m_dynamicShortcutCombo->addItem(tr("pp"));
    m_dynamicShortcutCombo->addItem(tr("p"));
    m_dynamicShortcutCombo->addItem(tr("mp"));
    m_dynamicShortcutCombo->addItem(tr("mf"));
    m_dynamicShortcutCombo->addItem(tr("f"));
    m_dynamicShortcutCombo->addItem(tr("ff"));
    m_dynamicShortcutCombo->addItem(tr("fff"));
    m_dynamicShortcutCombo->addItem(tr("rfz"));
    m_dynamicShortcutCombo->addItem(tr("sf"));

    // direction shortcuts combo
    m_directionShortcutLabel = new QLabel(tr("Direction:  "));
    m_optionLabel->addWidget(m_directionShortcutLabel);

    m_directionShortcutCombo = new QComboBox;
    m_optionWidget->addWidget(m_directionShortcutCombo);
    // fake breath mark removed - it was just an ordinary canned text, so there
    // are no long-term consequences for removing this one
    m_directionShortcutCombo->addItem(tr("D.C. al Fine"));
    m_directionShortcutCombo->addItem(tr("D.S. al Fine"));
    m_directionShortcutCombo->addItem(tr("Fine"));
    m_directionShortcutCombo->addItem(tr("D.S. al Coda"));
    m_directionShortcutCombo->addItem(tr("to Coda"));
    m_directionShortcutCombo->addItem(tr("Coda"));

    // local direction shortcuts combo
    m_localDirectionShortcutLabel = new QLabel(tr("Local Direction:  "));
    m_optionLabel->addWidget(m_localDirectionShortcutLabel);

    m_localDirectionShortcutCombo = new QComboBox;
    m_optionWidget->addWidget(m_localDirectionShortcutCombo);
    m_localDirectionShortcutCombo->addItem(tr("accel."));
    m_localDirectionShortcutCombo->addItem(tr("ritard."));
    m_localDirectionShortcutCombo->addItem(tr("ralletando"));
    m_localDirectionShortcutCombo->addItem(tr("a tempo"));
    m_localDirectionShortcutCombo->addItem(tr("legato"));
    m_localDirectionShortcutCombo->addItem(tr("simile"));
    m_localDirectionShortcutCombo->addItem(tr("pizz."));
    m_localDirectionShortcutCombo->addItem(tr("arco"));
    m_localDirectionShortcutCombo->addItem(tr("non vib."));
    m_localDirectionShortcutCombo->addItem(tr("sul pont."));
    m_localDirectionShortcutCombo->addItem(tr("sul tasto"));
    m_localDirectionShortcutCombo->addItem(tr("con legno"));
    m_localDirectionShortcutCombo->addItem(tr("sul tasto"));
    m_localDirectionShortcutCombo->addItem(tr("sul G"));
    m_localDirectionShortcutCombo->addItem(tr("ordinario"));
    m_localDirectionShortcutCombo->addItem(tr("Muta in "));
    m_localDirectionShortcutCombo->addItem(tr("volti subito "));
    m_localDirectionShortcutCombo->addItem(tr("soli"));
    m_localDirectionShortcutCombo->addItem(tr("div."));

    // tempo shortcuts combo
    m_tempoShortcutLabel = new QLabel(tr("Tempo:  "));
    m_optionLabel->addWidget(m_tempoShortcutLabel);

    m_tempoShortcutCombo = new QComboBox;
    m_optionWidget->addWidget(m_tempoShortcutCombo);
    m_tempoShortcutCombo->addItem(tr("Grave"));
    m_tempoShortcutCombo->addItem(tr("Adagio"));
    m_tempoShortcutCombo->addItem(tr("Largo"));
    m_tempoShortcutCombo->addItem(tr("Lento"));
    m_tempoShortcutCombo->addItem(tr("Andante"));
    m_tempoShortcutCombo->addItem(tr("Moderato"));
    m_tempoShortcutCombo->addItem(tr("Allegretto"));
    m_tempoShortcutCombo->addItem(tr("Allegro"));
    m_tempoShortcutCombo->addItem(tr("Vivace"));
    m_tempoShortcutCombo->addItem(tr("Presto"));
    m_tempoShortcutCombo->addItem(tr("Prestissimo"));
    m_tempoShortcutCombo->addItem(tr("Maestoso"));
    m_tempoShortcutCombo->addItem(tr("Sostenuto"));
    m_tempoShortcutCombo->addItem(tr("Tempo Primo"));

    // local tempo shortcuts combo (duplicates the non-local version, because
    // nobody is actually sure what is supposed to distinguish Tempo from
    // Local Tempo, or what this text style is supposed to be good for in the
    // way of standard notation)
    m_localTempoShortcutLabel = new QLabel(tr("Local Tempo:  "));
    m_optionLabel->addWidget(m_localTempoShortcutLabel);

    m_localTempoShortcutCombo = new QComboBox;
    m_optionWidget->addWidget(m_localTempoShortcutCombo);
    m_localTempoShortcutCombo->addItem(tr("Grave"));
    m_localTempoShortcutCombo->addItem(tr("Adagio"));
    m_localTempoShortcutCombo->addItem(tr("Largo"));
    m_localTempoShortcutCombo->addItem(tr("Lento"));
    m_localTempoShortcutCombo->addItem(tr("Andante"));
    m_localTempoShortcutCombo->addItem(tr("Moderato"));
    m_localTempoShortcutCombo->addItem(tr("Allegretto"));
    m_localTempoShortcutCombo->addItem(tr("Allegro"));
    m_localTempoShortcutCombo->addItem(tr("Vivace"));
    m_localTempoShortcutCombo->addItem(tr("Presto"));
    m_localTempoShortcutCombo->addItem(tr("Prestissimo"));
    m_localTempoShortcutCombo->addItem(tr("Maestoso"));
    m_localTempoShortcutCombo->addItem(tr("Sostenuto"));
    m_localTempoShortcutCombo->addItem(tr("Tempo Primo"));

    // LilyPond directive combo
    m_directiveLabel = new QLabel(tr("Directive:  "));
    m_optionLabel->addWidget(m_directiveLabel);

    m_lilyPondDirectiveCombo = new QComboBox;
    m_optionWidget->addWidget(m_lilyPondDirectiveCombo);

    // not tr()able, because the directive exporter currently depends on the
    // textual contents of these strings, not some more abstract associated
    // type label

// Remove DEPRECATED items from appearing in the future, though we have to
// support them on the business end for all eternity and beyond
//    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Segno));
//    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Coda));
    
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Alternate1));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Alternate2));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::BarDouble));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::BarEnd));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::BarDot));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Gliss));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Arpeggio));
    // never implemented:
    //    m_lilyPondDirectiveCombo->addItem(Text::ArpeggioUp);
    //    m_lilyPondDirectiveCombo->addItem(Text::ArpeggioDn);
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Tiny));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::Small));
    m_lilyPondDirectiveCombo->addItem(strtoqstr(Text::NormalSize));

    entryBox->setLayout(entryGridLay);

    int ls = m_notePixmapFactory->getLineSpacing();

    int mapWidth = 200;
    QPixmap map(mapWidth, ls * 5 + 1);
    QBitmap mask(mapWidth, ls * 5 + 1);

    map.fill(Qt::white);

    QPainter p;

    p.begin(&map);

    p.setPen(QColor(Qt::black));

    for (int i = 0; i < 5; ++i)
    {
        p.drawLine(0, ls * i, mapWidth - 1, ls * i);
    }

    p.end();

    m_staffAboveLabel = new QLabel("staff", exampleBox );
    innerFrameLayout->addWidget(m_staffAboveLabel);
    m_staffAboveLabel->setPixmap(map);

    m_textExampleLabel = new QLabel(tr("Example"), exampleBox );
    innerFrameLayout->addWidget(m_textExampleLabel);

    m_staffBelowLabel = new QLabel("staff", exampleBox );
    innerFrameLayout->addWidget(m_staffBelowLabel);
    m_staffBelowLabel->setPixmap(map);

    // restore last setting for shortcut combos
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_dynamicShortcutCombo->setCurrentIndex( settings.value("dynamic_shortcut", 0).toInt() );
    m_directionShortcutCombo->setCurrentIndex( settings.value("direction_shortcut", 0).toInt() );
    m_localDirectionShortcutCombo->setCurrentIndex( settings.value("local_direction_shortcut", 0).toInt() );
    m_tempoShortcutCombo->setCurrentIndex( settings.value("tempo_shortcut", 0).toInt() );
    m_localTempoShortcutCombo->setCurrentIndex( settings.value("local_tempo_shortcut", 0).toInt() );
    m_lilyPondDirectiveCombo->setCurrentIndex( settings.value("lilyPond_directive_combo", 0).toInt() );

    m_prevChord = settings.value("previous_chord", "").toString();
    m_prevLyric = settings.value("previous_lyric", "").toString();
    m_prevAnnotation = settings.value("previous_annotation", "").toString();

    QObject::connect(m_text, SIGNAL(textChanged(const QString &)),
                     this, SLOT(slotTextChanged(const QString &)));
    QObject::connect(m_typeCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotTypeChanged(const QString &)));
    QObject::connect(m_dynamicShortcutCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotDynamicShortcutChanged(const QString &)));
    QObject::connect(m_directionShortcutCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotDirectionShortcutChanged(const QString &)));
    QObject::connect(m_localDirectionShortcutCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotLocalDirectionShortcutChanged(const QString &)));
    QObject::connect(m_tempoShortcutCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotTempoShortcutChanged(const QString &)));
    QObject::connect(m_localTempoShortcutCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotLocalTempoShortcutChanged(const QString &)));
    QObject::connect(m_lilyPondDirectiveCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotLilyPondDirectiveChanged(const QString &)));

    QObject::connect(m_optionLabel, SIGNAL(currentChanged(int)), this, SLOT(slotUpdateSize(int)));
    QObject::connect(m_optionWidget, SIGNAL(currentChanged(int)), this, SLOT(slotUpdateSize(int)));
        
    m_text->setFocus();
    slotTypeChanged(strtoqstr(getTextType()));

    // a hacky little fix for #1512143, to restore the capability to edit
    // existing annotations and other whatnots
    //!!! tacking another one of these on the bottom strikes me as lame in the
    // extreme, but it works, and it costs little, and other solutions I can
    // imagine would cost so much more.
    m_text->setText(strtoqstr(defaultText.getText()));
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    vboxLayout->addWidget(buttonBox, 1, 0);
    //vboxLayout->setRowStretch(0, 10);
    
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotOK()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));

    settings.endGroup();

    adjustSize();
}

Text
TextEventDialog::getText() const
{
    Text text(getTextString(), getTextType());
    text.setVerse(m_verseSpin->value() - 1);
    return text;
}

std::string
TextEventDialog::getTextType() const
{
    return m_styles[m_typeCombo->currentIndex()];
}

std::string
TextEventDialog::getTextString() const
{
    return std::string(qstrtostr(m_text->text()));
}

void
TextEventDialog::slotTextChanged(const QString &qtext)
{
    std::string type(getTextType());

    QString qtrunc(qtext);
    if (qtrunc.length() > 20)
        qtrunc = qtrunc.left(20) + "...";
    std::string text(qstrtostr(qtrunc));
    if (text == "")
        text = "Sample";

    Text rtext(text, type);
    m_textExampleLabel->setPixmap(m_notePixmapFactory->makeTextPixmap(rtext));
}

void
TextEventDialog::slotTypeChanged(const QString &)
{
    std::string type(getTextType());

    QString qtrunc(m_text->text());
    if (qtrunc.length() > 20)
        qtrunc = qtrunc.left(20) + "...";
    std::string text(qstrtostr(qtrunc));
    if (text == "")
        text = "Sample";

    Text rtext(text, type);
    m_textExampleLabel->setPixmap(m_notePixmapFactory->makeTextPixmap(rtext));

    //
    // swap widgets in and out, depending on the current text type
    //
    if (type == Text::Dynamic) {
        m_optionLabel->setCurrentWidget(m_dynamicShortcutLabel);
        m_optionWidget->setCurrentWidget(m_dynamicShortcutCombo);
        slotDynamicShortcutChanged(strtoqstr(text));
    }

    if (type == Text::Direction) {
        m_optionLabel->setCurrentWidget(m_directionShortcutLabel);
        m_optionWidget->setCurrentWidget(m_directionShortcutCombo);
        slotDirectionShortcutChanged(strtoqstr(text));
    }

    if (type == Text::LocalDirection) {
        m_optionLabel->setCurrentWidget(m_localDirectionShortcutLabel);
        m_optionWidget->setCurrentWidget(m_localDirectionShortcutCombo);
        slotLocalDirectionShortcutChanged(strtoqstr(text));
    }

    if (type == Text::Tempo) {
        m_optionLabel->setCurrentWidget(m_tempoShortcutLabel);
        m_optionWidget->setCurrentWidget(m_tempoShortcutCombo);
        slotTempoShortcutChanged(strtoqstr(text));
    }

    if (type == Text::LocalTempo) {
        m_optionLabel->setCurrentWidget(m_localTempoShortcutLabel);
        m_optionWidget->setCurrentWidget(m_localTempoShortcutCombo);
        slotLocalTempoShortcutChanged(strtoqstr(text));
    }

    if (type == Text::Lyric) {
        m_optionLabel->setCurrentWidget(m_verseLabel);
        m_optionWidget->setCurrentWidget(m_verseSpin);
    }

    if (type == Text::Annotation ||
        type == Text::Chord ||
        type == Text::UnspecifiedType) {

        m_optionLabel->setCurrentWidget(m_blankLabel);
        m_optionWidget->setCurrentWidget(m_blankWidget);
    }

    // restore previous text of appropriate type
    if (type == Text::Lyric)
        m_text->setText(m_prevLyric);
    else if (type == Text::Chord)
        m_text->setText(m_prevChord);
    else if (type == Text::Annotation)
        m_text->setText(m_prevAnnotation);

    //
    // LilyPond directives only taking temporary residence here; will move out
    // into some new class eventually
    //
    if (type == Text::LilyPondDirective) {
        m_optionWidget->setCurrentWidget(m_lilyPondDirectiveCombo);
        m_optionLabel->setCurrentWidget(m_directiveLabel);
        m_staffAboveLabel->hide();
        m_staffBelowLabel->show();
        m_text->setReadOnly(true);
        m_text->setEnabled(false);
        slotLilyPondDirectiveChanged(strtoqstr(text));
    } else {
        m_text->setReadOnly(false);
        m_text->setEnabled(true);

        if (type == Text::Dynamic ||
            type == Text::LocalDirection ||
            type == Text::UnspecifiedType ||
            type == Text::Lyric ||
            type == Text::Annotation) {

            m_staffAboveLabel->show();
            m_staffBelowLabel->hide();
        } else {
            m_staffAboveLabel->hide();
            m_staffBelowLabel->show();
        }
    }

    adjustSize();
}

void
TextEventDialog::slotOK()
{
    // store last setting for shortcut combos
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("dynamic_shortcut", m_dynamicShortcutCombo->currentIndex());
    settings.setValue("direction_shortcut", m_directionShortcutCombo->currentIndex());
    settings.setValue("local_direction_shortcut", m_localDirectionShortcutCombo->currentIndex());
    settings.setValue("tempo_shortcut", m_tempoShortcutCombo->currentIndex());
    settings.setValue("local_tempo_shortcut", m_localTempoShortcutCombo->currentIndex());
    // temporary home:
    settings.setValue("lilyPond_directive_combo", m_lilyPondDirectiveCombo->currentIndex());

    // store  last chord, lyric, annotation, depending on what's currently in
    // the text entry widget
    int index = m_typeCombo->currentIndex();
    if (index == 5)
        settings.setValue("previous_chord", m_text->text());
    else if (index == 6)
        settings.setValue("previous_lyric", m_text->text());
    else if (index == 7)
        settings.setValue("previous_annotation", m_text->text());

    settings.endGroup();

    accept();
}

void
TextEventDialog::slotDynamicShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(m_dynamicShortcutCombo->currentText());
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotDirectionShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(m_directionShortcutCombo->currentText());
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLocalDirectionShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(m_localDirectionShortcutCombo->currentText());
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotTempoShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(m_tempoShortcutCombo->currentText());
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLocalTempoShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(m_localTempoShortcutCombo->currentText());
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLilyPondDirectiveChanged(const QString &)
{
    m_text->setText(m_lilyPondDirectiveCombo->currentText());
}

void
TextEventDialog::slotUpdateSize(int i)
{
    // (i is just the index of the active widget in the QStackedWidget and is
    // only interesting to track to make sure something is changing)
    std::cerr << "TextEventDialog::slotUpdateSize(" << i << ")" << std::endl;
    adjustSize();
}



void
TextEventDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:textEventDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:textEventDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
