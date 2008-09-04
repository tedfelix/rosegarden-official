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


#include "TextEventDialog.h"
#include <kapplication.h>

#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <QComboBox>
#include <kconfig.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QBitmap>
#include <qgrid.h>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QSpinBox>

namespace Rosegarden
{

TextEventDialog::TextEventDialog(QDialogButtonBox::QWidget *parent,
                                 NotePixmapFactory *npf,
                                 Text defaultText,
                                 int maxLength) :
        QDialog(parent),
        m_notePixmapFactory(npf),
        m_styles(Text::getUserStyles()) /*,
            //m_directives(Text::getLilyPondDirectives()) */
{
    setHelp("nv-text");
    setModal(true);
    setWindowTitle(i18n("Text"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *entryBox = new QGroupBox( i18n("Specification"), vbox );
    vboxLayout->addWidget(entryBox);
    QGroupBox *exampleBox = new QGroupBox( i18n("Preview"), vbox );
    vboxLayout->addWidget(exampleBox);
    vbox->setLayout(vboxLayout);

    QGrid *entryGrid = new QGrid(2, QGrid::Horizontal, entryBox);

    new QLabel(i18n("Text:  "), entryGrid);
    m_text = new QLineEdit(entryGrid);
    m_text->setText(strtoqstr(defaultText.getText()));
    if (maxLength > 0)
        m_text->setMaxLength(maxLength);

    // style combo
    new QLabel(i18n("Style:  "), entryGrid);
    m_typeCombo = new QComboBox(entryGrid);

    for (unsigned int i = 0; i < m_styles.size(); ++i)
    {

        std::string style = m_styles[i];

        // if the style is in this list, we can i18n it (kludgy):

        if (style == Text::Dynamic) {                           // index //
            m_typeCombo->addItem(i18n("Dynamic"));           // 0

        } else if (style == Text::Direction) {
            m_typeCombo->addItem(i18n("Direction"));         // 1

        } else if (style == Text::LocalDirection) {
            m_typeCombo->addItem(i18n("Local Direction"));   // 2

        } else if (style == Text::Tempo) {
            m_typeCombo->addItem(i18n("Tempo"));             // 3

        } else if (style == Text::LocalTempo) {
            m_typeCombo->addItem(i18n("Local Tempo"));       // 4

        } else if (style == Text::Lyric) {
            m_typeCombo->addItem(i18n("Lyric"));             // 5

        } else if (style == Text::Chord) {
            m_typeCombo->addItem(i18n("Chord"));             // 6

        } else if (style == Text::Annotation) {
            m_typeCombo->addItem(i18n("Annotation"));        // 7

        } else if (style == Text::LilyPondDirective) {
            m_typeCombo->addItem(i18n("LilyPond Directive")); // 8

        } else {
            // not i18n()-able

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

    m_verseLabel = new QLabel(i18n("Verse:  "), entryGrid);
    m_verseLabel->hide();
    m_verseSpin = new QSpinBox(entryGrid);
    m_verseSpin->setMinimum(1);
    m_verseSpin->setMaximum(12);
    m_verseSpin->setSingleStep(1);
    m_verseSpin->setValue(defaultText.getVerse() + 1);
    m_verseSpin->hide();

    // dynamic shortcuts combo
    m_dynamicShortcutLabel = new QLabel(i18n("Dynamic:  "), entryGrid);
    m_dynamicShortcutLabel->hide();

    m_dynamicShortcutCombo = new QComboBox(entryGrid);
    m_dynamicShortcutCombo->addItem(i18n("ppp"));
    m_dynamicShortcutCombo->addItem(i18n("pp"));
    m_dynamicShortcutCombo->addItem(i18n("p"));
    m_dynamicShortcutCombo->addItem(i18n("mp"));
    m_dynamicShortcutCombo->addItem(i18n("mf"));
    m_dynamicShortcutCombo->addItem(i18n("f"));
    m_dynamicShortcutCombo->addItem(i18n("ff"));
    m_dynamicShortcutCombo->addItem(i18n("fff"));
    m_dynamicShortcutCombo->addItem(i18n("rfz"));
    m_dynamicShortcutCombo->addItem(i18n("sf"));
    m_dynamicShortcutCombo->hide();

    // direction shortcuts combo
    m_directionShortcutLabel = new QLabel(i18n("Direction:  "), entryGrid);
    m_directionShortcutLabel->hide();

    m_directionShortcutCombo = new QComboBox(entryGrid);
    // note, the "  ," is a breath mark; the extra spaces are a cheap hack to
    // try to improve the probability of Rosegarden drawing the blasted thing
    // where it's supposed to go, without the need to micro-diddle each and
    // every bliffin' one.  (Micro-diddling is not exportable to LilyPond
    // either, is it?  I rather doubt it.)
    m_directionShortcutCombo->addItem(i18n("  ,"));
    m_directionShortcutCombo->addItem(i18n("D.C. al Fine"));
    m_directionShortcutCombo->addItem(i18n("D.S. al Fine"));
    m_directionShortcutCombo->addItem(i18n("Fine"));
    m_directionShortcutCombo->addItem(i18n("D.S. al Coda"));
    m_directionShortcutCombo->addItem(i18n("to Coda"));
    m_directionShortcutCombo->addItem(i18n("Coda"));
    m_directionShortcutCombo->hide();

    // local direction shortcuts combo
    m_localDirectionShortcutLabel = new QLabel(i18n("Local Direction:  "), entryGrid);
    m_localDirectionShortcutLabel->hide();

    m_localDirectionShortcutCombo = new QComboBox(entryGrid);
    m_localDirectionShortcutCombo->addItem(i18n("shortcut."));
    m_localDirectionShortcutCombo->addItem(i18n("ritard."));
    m_localDirectionShortcutCombo->addItem(i18n("ralletando"));
    m_localDirectionShortcutCombo->addItem(i18n("a tempo"));
    m_localDirectionShortcutCombo->addItem(i18n("legato"));
    m_localDirectionShortcutCombo->addItem(i18n("simile"));
    m_localDirectionShortcutCombo->addItem(i18n("pizz."));
    m_localDirectionShortcutCombo->addItem(i18n("arco"));
    m_localDirectionShortcutCombo->addItem(i18n("non vib."));
    m_localDirectionShortcutCombo->addItem(i18n("sul pont."));
    m_localDirectionShortcutCombo->addItem(i18n("sul tasto"));
    m_localDirectionShortcutCombo->addItem(i18n("con legno"));
    m_localDirectionShortcutCombo->addItem(i18n("sul tasto"));
    m_localDirectionShortcutCombo->addItem(i18n("sul G"));
    m_localDirectionShortcutCombo->addItem(i18n("ordinario"));
    m_localDirectionShortcutCombo->addItem(i18n("Muta in "));
    m_localDirectionShortcutCombo->addItem(i18n("volti subito "));
    m_localDirectionShortcutCombo->addItem(i18n("soli"));
    m_localDirectionShortcutCombo->addItem(i18n("div."));
    m_localDirectionShortcutCombo->hide();

    // tempo shortcuts combo
    m_tempoShortcutLabel = new QLabel(i18n("Tempo:  "), entryGrid);
    m_tempoShortcutLabel->hide();

    m_tempoShortcutCombo = new QComboBox(entryGrid);
    m_tempoShortcutCombo->addItem(i18n("Grave"));
    m_tempoShortcutCombo->addItem(i18n("Adagio"));
    m_tempoShortcutCombo->addItem(i18n("Largo"));
    m_tempoShortcutCombo->addItem(i18n("Lento"));
    m_tempoShortcutCombo->addItem(i18n("Andante"));
    m_tempoShortcutCombo->addItem(i18n("Moderato"));
    m_tempoShortcutCombo->addItem(i18n("Allegretto"));
    m_tempoShortcutCombo->addItem(i18n("Allegro"));
    m_tempoShortcutCombo->addItem(i18n("Vivace"));
    m_tempoShortcutCombo->addItem(i18n("Presto"));
    m_tempoShortcutCombo->addItem(i18n("Prestissimo"));
    m_tempoShortcutCombo->addItem(i18n("Maestoso"));
    m_tempoShortcutCombo->addItem(i18n("Sostenuto"));
    m_tempoShortcutCombo->addItem(i18n("Tempo Primo"));
    m_tempoShortcutCombo->hide();

    // local tempo shortcuts combo (duplicates the non-local version, because
    // nobody is actually sure what is supposed to distinguish Tempo from
    // Local Tempo, or what this text style is supposed to be good for in the
    // way of standard notation)
    m_localTempoShortcutLabel = new QLabel(i18n("Local Tempo:  "), entryGrid);
    m_localTempoShortcutLabel->hide();

    m_localTempoShortcutCombo = new QComboBox(entryGrid);
    m_localTempoShortcutCombo->addItem(i18n("Grave"));
    m_localTempoShortcutCombo->addItem(i18n("Adagio"));
    m_localTempoShortcutCombo->addItem(i18n("Largo"));
    m_localTempoShortcutCombo->addItem(i18n("Lento"));
    m_localTempoShortcutCombo->addItem(i18n("Andante"));
    m_localTempoShortcutCombo->addItem(i18n("Moderato"));
    m_localTempoShortcutCombo->addItem(i18n("Allegretto"));
    m_localTempoShortcutCombo->addItem(i18n("Allegro"));
    m_localTempoShortcutCombo->addItem(i18n("Vivace"));
    m_localTempoShortcutCombo->addItem(i18n("Presto"));
    m_localTempoShortcutCombo->addItem(i18n("Prestissimo"));
    m_localTempoShortcutCombo->addItem(i18n("Maestoso"));
    m_localTempoShortcutCombo->addItem(i18n("Sostenuto"));
    m_localTempoShortcutCombo->addItem(i18n("Tempo Primo"));
    m_localTempoShortcutCombo->hide();

    // LilyPond directive combo
    m_directiveLabel = new QLabel(i18n("Directive:  "), entryGrid);
    m_directiveLabel->hide();

    m_lilyPondDirectiveCombo = new QComboBox(entryGrid);
    m_lilyPondDirectiveCombo->hide();

    // not i18nable, because the directive exporter currently depends on the
    // textual contents of these strings, not some more abstract associated
    // type label
    m_lilyPondDirectiveCombo->addItem(Text::Segno);
    m_lilyPondDirectiveCombo->addItem(Text::Coda);
    m_lilyPondDirectiveCombo->addItem(Text::Alternate1);
    m_lilyPondDirectiveCombo->addItem(Text::Alternate2);
    m_lilyPondDirectiveCombo->addItem(Text::BarDouble);
    m_lilyPondDirectiveCombo->addItem(Text::BarEnd);
    m_lilyPondDirectiveCombo->addItem(Text::BarDot);
    m_lilyPondDirectiveCombo->addItem(Text::Gliss);
    m_lilyPondDirectiveCombo->addItem(Text::Arpeggio);
    //    m_lilyPondDirectiveCombo->addItem(Text::ArpeggioUp);
    //    m_lilyPondDirectiveCombo->addItem(Text::ArpeggioDn);
    m_lilyPondDirectiveCombo->addItem(Text::Tiny);
    m_lilyPondDirectiveCombo->addItem(Text::Small);
    m_lilyPondDirectiveCombo->addItem(Text::NormalSize);

    QWidget *exampleVBox = new QWidget(exampleBox);
    QVBoxLayout *exampleVBoxLayout = new QVBoxLayout;

    int ls = m_notePixmapFactory->getLineSpacing();

    int mapWidth = 200;
    QPixmap map(mapWidth, ls * 5 + 1);
    QBitmap mask(mapWidth, ls * 5 + 1);

    map.fill();
    mask.fill(QColor(Qt::color0));

    QPainter p, pm;

    p.begin(&map);
    pm.begin(&mask);

    p.setPen(QColor(Qt::black));
    pm.setPen(QColor(Qt::white));

    for (int i = 0; i < 5; ++i)
    {
        p.drawLine(0, ls * i, mapWidth - 1, ls * i);
        pm.drawLine(0, ls * i, mapWidth - 1, ls * i);
    }

    p.end();
    pm.end();

    map.setMask(mask);

    m_staffAboveLabel = new QLabel("staff", exampleVBox );
    exampleVBoxLayout->addWidget(m_staffAboveLabel);
    m_staffAboveLabel->setPixmap(map);

    m_textExampleLabel = new QLabel(i18n("Example"), exampleVBox );
    exampleVBoxLayout->addWidget(m_textExampleLabel);

    m_staffBelowLabel = new QLabel("staff", exampleVBox );
    exampleVBoxLayout->addWidget(m_staffBelowLabel);
    exampleVBox->setLayout(exampleVBoxLayout);
    m_staffBelowLabel->setPixmap(map);

    // restore last setting for shortcut combos
    QSettings *config = kapp->config();
    config->beginGroup( NotationViewConfigGroup );
    // 
    // manually-FIX, add:
    // config->endGroup();		// corresponding to: config->beginGroup( NotationViewConfigGroup );
    //  
;

    m_dynamicShortcutCombo->setCurrentIndex( config->value("dynamic_shortcut", 0).toInt() );
    m_directionShortcutCombo->setCurrentIndex( config->value("direction_shortcut", 0).toInt() );
    m_localDirectionShortcutCombo->setCurrentIndex( config->value("local_direction_shortcut", 0).toInt() );
    m_tempoShortcutCombo->setCurrentIndex( config->value("tempo_shortcut", 0).toInt() );
    m_localTempoShortcutCombo->setCurrentIndex( config->value("local_tempo_shortcut", 0).toInt() );
    m_lilyPondDirectiveCombo->setCurrentIndex( config->value("lilyPond_directive_combo", 0).toInt() );

    m_prevChord = config->value("previous_chord", "") ;
    m_prevLyric = config->value("previous_lyric", "") ;
    m_prevAnnotation = config->value("previous_annotation", "") ;

    QObject::connect(m_text, SIGNAL(textChanged(const QString &)),
                     this, SLOT(slotTextChanged(const QString &)));
    QObject::connect(m_typeCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotTypeChanged(const QString &)));
    QObject::connect(this, SIGNAL(okClicked()), this, SLOT(slotOK()));
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

    m_text->setFocus();
    slotTypeChanged(strtoqstr(getTextType()));

    // a hacky little fix for #1512143, to restore the capability to edit
    // existing annotations and other whatnots
    //!!! tacking another one of these on the bottom strikes me as lame in the
    // extreme, but it works, and it costs little, and other solutions I can
    // imagine would cost so much more.
    m_text->setText(strtoqstr(defaultText.getText()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
    m_textExampleLabel->setPixmap
    (NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeTextPixmap(rtext)));
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
    m_textExampleLabel->setPixmap
    (NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeTextPixmap(rtext)));

    //
    // swap widgets in and out, depending on the current text type
    //
    if (type == Text::Dynamic) {
        m_dynamicShortcutLabel->show();
        m_dynamicShortcutCombo->show();
        slotDynamicShortcutChanged(text);
    } else {
        m_dynamicShortcutLabel->hide();
        m_dynamicShortcutCombo->hide();
    }

    if (type == Text::Direction) {
        m_directionShortcutLabel->show();
        m_directionShortcutCombo->show();
        slotDirectionShortcutChanged(text);
    } else {
        m_directionShortcutLabel->hide();
        m_directionShortcutCombo->hide();
    }

    if (type == Text::LocalDirection) {
        m_localDirectionShortcutLabel->show();
        m_localDirectionShortcutCombo->show();
        slotLocalDirectionShortcutChanged(text);
    } else {
        m_localDirectionShortcutLabel->hide();
        m_localDirectionShortcutCombo->hide();
    }

    if (type == Text::Tempo) {
        m_tempoShortcutLabel->show();
        m_tempoShortcutCombo->show();
        slotTempoShortcutChanged(text);
    } else {
        m_tempoShortcutLabel->hide();
        m_tempoShortcutCombo->hide();
    }

    if (type == Text::LocalTempo) {
        m_localTempoShortcutLabel->show();
        m_localTempoShortcutCombo->show();
        slotLocalTempoShortcutChanged(text);
    } else {
        m_localTempoShortcutLabel->hide();
        m_localTempoShortcutCombo->hide();
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
        m_lilyPondDirectiveCombo->show();
        m_directiveLabel->show();
        m_staffAboveLabel->hide();
        m_staffBelowLabel->show();
        m_text->setReadOnly(true);
        m_text->setEnabled(false);
        slotLilyPondDirectiveChanged(text);
    } else {
        m_lilyPondDirectiveCombo->hide();
        m_directiveLabel->hide();
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

        if (type == Text::Lyric) {
            m_verseLabel->show();
            m_verseSpin->show();
        }
    }
}

void
TextEventDialog::slotOK()
{
    // store last setting for shortcut combos
    QSettings *config = kapp->config();
    config->beginGroup( NotationViewConfigGroup );
    // 
    // manually-FIX, add:
    // config->endGroup();		// corresponding to: config->beginGroup( NotationViewConfigGroup );
    //  
;

    config->writeEntry("dynamic_shortcut", m_dynamicShortcutCombo->currentIndex());
    config->writeEntry("direction_shortcut", m_directionShortcutCombo->currentIndex());
    config->writeEntry("local_direction_shortcut", m_localDirectionShortcutCombo->currentIndex());
    config->writeEntry("tempo_shortcut", m_tempoShortcutCombo->currentIndex());
    config->writeEntry("local_tempo_shortcut", m_localTempoShortcutCombo->currentIndex());
    // temporary home:
    config->writeEntry("lilyPond_directive_combo", m_lilyPondDirectiveCombo->currentIndex());

    // store  last chord, lyric, annotation, depending on what's currently in
    // the text entry widget
    int index = m_typeCombo->currentIndex();
    if (index == 5)
        config->writeEntry("previous_chord", m_text->text());
    else if (index == 6)
        config->writeEntry("previous_lyric", m_text->text());
    else if (index == 7)
        config->writeEntry("previous_annotation", m_text->text());
}

void
TextEventDialog::slotDynamicShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(strtoqstr(m_dynamicShortcutCombo->currentText()));
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotDirectionShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(strtoqstr(m_directionShortcutCombo->currentText()));
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLocalDirectionShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(strtoqstr(m_localDirectionShortcutCombo->currentText()));
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotTempoShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(strtoqstr(m_tempoShortcutCombo->currentText()));
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLocalTempoShortcutChanged(const QString &text)
{
    if (text == "" || text == "Sample") {
        m_text->setText(strtoqstr(m_localTempoShortcutCombo->currentText()));
    } else {
        m_text->setText(text);
    }
}

void
TextEventDialog::slotLilyPondDirectiveChanged(const QString &)
{
    m_text->setText(strtoqstr(m_lilyPondDirectiveCombo->currentText()));
}

}
#include "TextEventDialog.moc"
