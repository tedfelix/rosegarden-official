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


#include "TextEventDialog.h"
#include <kapplication.h>

#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <qbitmap.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

TextEventDialog::TextEventDialog(QWidget *parent,
                                 NotePixmapFactory *npf,
                                 Text defaultText,
                                 int maxLength) :
        KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel | Help),
        m_notePixmapFactory(npf),
        m_styles(Text::getUserStyles()) /*,
            //m_directives(Text::getLilypondDirectives()) */
{
    setHelp("nv-text");
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *entryBox = new QGroupBox
                          (1, Horizontal, i18n("Specification"), vbox);
    QGroupBox *exampleBox = new QGroupBox
                            (1, Horizontal, i18n("Preview"), vbox);

    QGrid *entryGrid = new QGrid(2, QGrid::Horizontal, entryBox);

    new QLabel(i18n("Text:  "), entryGrid);
    m_text = new QLineEdit(entryGrid);
    m_text->setText(strtoqstr(defaultText.getText()));
    if (maxLength > 0)
        m_text->setMaxLength(maxLength);

    // style combo
    new QLabel(i18n("Style:  "), entryGrid);
    m_typeCombo = new KComboBox(entryGrid);

    for (unsigned int i = 0; i < m_styles.size(); ++i)
    {

        std::string style = m_styles[i];

        // if the style is in this list, we can i18n it (kludgy):

        if (style == Text::Dynamic) {                           // index //
            m_typeCombo->insertItem(i18n("Dynamic"));           // 0

        } else if (style == Text::Direction) {
            m_typeCombo->insertItem(i18n("Direction"));         // 1

        } else if (style == Text::LocalDirection) {
            m_typeCombo->insertItem(i18n("Local Direction"));   // 2

        } else if (style == Text::Tempo) {
            m_typeCombo->insertItem(i18n("Tempo"));             // 3

        } else if (style == Text::LocalTempo) {
            m_typeCombo->insertItem(i18n("Local Tempo"));       // 4

        } else if (style == Text::Lyric) {
            m_typeCombo->insertItem(i18n("Lyric"));             // 5

        } else if (style == Text::Chord) {
            m_typeCombo->insertItem(i18n("Chord"));             // 6

        } else if (style == Text::Annotation) {
            m_typeCombo->insertItem(i18n("Annotation"));        // 7

        } else if (style == Text::LilypondDirective) {
            m_typeCombo->insertItem(i18n("LilyPond Directive")); // 8

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

            m_typeCombo->insertItem(strtoqstr(styleName));
        }

        if (style == defaultText.getTextType()) {
            m_typeCombo->setCurrentItem(m_typeCombo->count() - 1);
        }
    }

    // dynamic shortcuts combo
    m_dynamicShortcutLabel = new QLabel(i18n("Dynamic:  "), entryGrid);
    m_dynamicShortcutLabel->hide();

    m_dynamicShortcutCombo = new KComboBox(entryGrid);
    m_dynamicShortcutCombo->insertItem(i18n("ppp"));
    m_dynamicShortcutCombo->insertItem(i18n("pp"));
    m_dynamicShortcutCombo->insertItem(i18n("p"));
    m_dynamicShortcutCombo->insertItem(i18n("mp"));
    m_dynamicShortcutCombo->insertItem(i18n("mf"));
    m_dynamicShortcutCombo->insertItem(i18n("f"));
    m_dynamicShortcutCombo->insertItem(i18n("ff"));
    m_dynamicShortcutCombo->insertItem(i18n("fff"));
    m_dynamicShortcutCombo->insertItem(i18n("rfz"));
    m_dynamicShortcutCombo->insertItem(i18n("sf"));
    m_dynamicShortcutCombo->hide();

    // direction shortcuts combo
    m_directionShortcutLabel = new QLabel(i18n("Direction:  "), entryGrid);
    m_directionShortcutLabel->hide();

    m_directionShortcutCombo = new KComboBox(entryGrid);
    // note, the "  ," is a breath mark; the extra spaces are a cheap hack to
    // try to improve the probability of Rosegarden drawing the blasted thing
    // where it's supposed to go, without the need to micro-diddle each and
    // every bliffin' one.  (Micro-diddling is not exportable to LilyPond
    // either, is it?  I rather doubt it.)
    m_directionShortcutCombo->insertItem(i18n("  ,"));
    m_directionShortcutCombo->insertItem(i18n("D.C. al Fine"));
    m_directionShortcutCombo->insertItem(i18n("D.S. al Fine"));
    m_directionShortcutCombo->insertItem(i18n("Fine"));
    m_directionShortcutCombo->insertItem(i18n("D.S. al Coda"));
    m_directionShortcutCombo->insertItem(i18n("to Coda"));
    m_directionShortcutCombo->insertItem(i18n("Coda"));
    m_directionShortcutCombo->hide();

    // local direction shortcuts combo
    m_localDirectionShortcutLabel = new QLabel(i18n("Local Direction:  "), entryGrid);
    m_localDirectionShortcutLabel->hide();

    m_localDirectionShortcutCombo = new KComboBox(entryGrid);
    m_localDirectionShortcutCombo->insertItem(i18n("accel."));
    m_localDirectionShortcutCombo->insertItem(i18n("ritard."));
    m_localDirectionShortcutCombo->insertItem(i18n("ralletando"));
    m_localDirectionShortcutCombo->insertItem(i18n("a tempo"));
    m_localDirectionShortcutCombo->insertItem(i18n("legato"));
    m_localDirectionShortcutCombo->insertItem(i18n("simile"));
    m_localDirectionShortcutCombo->insertItem(i18n("pizz."));
    m_localDirectionShortcutCombo->insertItem(i18n("arco"));
    m_localDirectionShortcutCombo->insertItem(i18n("non vib."));
    m_localDirectionShortcutCombo->insertItem(i18n("sul pont."));
    m_localDirectionShortcutCombo->insertItem(i18n("sul tasto"));
    m_localDirectionShortcutCombo->insertItem(i18n("con legno"));
    m_localDirectionShortcutCombo->insertItem(i18n("sul tasto"));
    m_localDirectionShortcutCombo->insertItem(i18n("sul G"));
    m_localDirectionShortcutCombo->insertItem(i18n("ordinario"));
    m_localDirectionShortcutCombo->insertItem(i18n("Muta in "));
    m_localDirectionShortcutCombo->insertItem(i18n("volti subito "));
    m_localDirectionShortcutCombo->insertItem(i18n("soli"));
    m_localDirectionShortcutCombo->insertItem(i18n("div."));
    m_localDirectionShortcutCombo->hide();

    // tempo shortcuts combo
    m_tempoShortcutLabel = new QLabel(i18n("Tempo:  "), entryGrid);
    m_tempoShortcutLabel->hide();

    m_tempoShortcutCombo = new KComboBox(entryGrid);
    m_tempoShortcutCombo->insertItem(i18n("Grave"));
    m_tempoShortcutCombo->insertItem(i18n("Adagio"));
    m_tempoShortcutCombo->insertItem(i18n("Largo"));
    m_tempoShortcutCombo->insertItem(i18n("Lento"));
    m_tempoShortcutCombo->insertItem(i18n("Andante"));
    m_tempoShortcutCombo->insertItem(i18n("Moderato"));
    m_tempoShortcutCombo->insertItem(i18n("Allegretto"));
    m_tempoShortcutCombo->insertItem(i18n("Allegro"));
    m_tempoShortcutCombo->insertItem(i18n("Vivace"));
    m_tempoShortcutCombo->insertItem(i18n("Presto"));
    m_tempoShortcutCombo->insertItem(i18n("Prestissimo"));
    m_tempoShortcutCombo->insertItem(i18n("Maestoso"));
    m_tempoShortcutCombo->insertItem(i18n("Sostenuto"));
    m_tempoShortcutCombo->insertItem(i18n("Tempo Primo"));
    m_tempoShortcutCombo->hide();

    // local tempo shortcuts combo (duplicates the non-local version, because
    // nobody is actually sure what is supposed to distinguish Tempo from
    // Local Tempo, or what this text style is supposed to be good for in the
    // way of standard notation)
    m_localTempoShortcutLabel = new QLabel(i18n("Local Tempo:  "), entryGrid);
    m_localTempoShortcutLabel->hide();

    m_localTempoShortcutCombo = new KComboBox(entryGrid);
    m_localTempoShortcutCombo->insertItem(i18n("Grave"));
    m_localTempoShortcutCombo->insertItem(i18n("Adagio"));
    m_localTempoShortcutCombo->insertItem(i18n("Largo"));
    m_localTempoShortcutCombo->insertItem(i18n("Lento"));
    m_localTempoShortcutCombo->insertItem(i18n("Andante"));
    m_localTempoShortcutCombo->insertItem(i18n("Moderato"));
    m_localTempoShortcutCombo->insertItem(i18n("Allegretto"));
    m_localTempoShortcutCombo->insertItem(i18n("Allegro"));
    m_localTempoShortcutCombo->insertItem(i18n("Vivace"));
    m_localTempoShortcutCombo->insertItem(i18n("Presto"));
    m_localTempoShortcutCombo->insertItem(i18n("Prestissimo"));
    m_localTempoShortcutCombo->insertItem(i18n("Maestoso"));
    m_localTempoShortcutCombo->insertItem(i18n("Sostenuto"));
    m_localTempoShortcutCombo->insertItem(i18n("Tempo Primo"));
    m_localTempoShortcutCombo->hide();

    // Lilypond directive combo
    m_directiveLabel = new QLabel(i18n("Directive:  "), entryGrid);
    m_directiveLabel->hide();

    m_lilypondDirectiveCombo = new KComboBox(entryGrid);
    m_lilypondDirectiveCombo->hide();

    // not i18nable, because the directive exporter currently depends on the
    // textual contents of these strings, not some more abstract associated
    // type label
    m_lilypondDirectiveCombo->insertItem(Text::Segno);
    m_lilypondDirectiveCombo->insertItem(Text::Coda);
    m_lilypondDirectiveCombo->insertItem(Text::Alternate1);
    m_lilypondDirectiveCombo->insertItem(Text::Alternate2);
    m_lilypondDirectiveCombo->insertItem(Text::BarDouble);
    m_lilypondDirectiveCombo->insertItem(Text::BarEnd);
    m_lilypondDirectiveCombo->insertItem(Text::BarDot);
    m_lilypondDirectiveCombo->insertItem(Text::Gliss);
    m_lilypondDirectiveCombo->insertItem(Text::Arpeggio);
    //    m_lilypondDirectiveCombo->insertItem(Text::ArpeggioUp);
    //    m_lilypondDirectiveCombo->insertItem(Text::ArpeggioDn);
    m_lilypondDirectiveCombo->insertItem(Text::Tiny);
    m_lilypondDirectiveCombo->insertItem(Text::Small);
    m_lilypondDirectiveCombo->insertItem(Text::NormalSize);

    QVBox *exampleVBox = new QVBox(exampleBox);

    int ls = m_notePixmapFactory->getLineSpacing();

    int mapWidth = 200;
    QPixmap map(mapWidth, ls * 5 + 1);
    QBitmap mask(mapWidth, ls * 5 + 1);

    map.fill();
    mask.fill(Qt::color0);

    QPainter p, pm;

    p.begin(&map);
    pm.begin(&mask);

    p.setPen(Qt::black);
    pm.setPen(Qt::white);

    for (int i = 0; i < 5; ++i)
    {
        p.drawLine(0, ls * i, mapWidth - 1, ls * i);
        pm.drawLine(0, ls * i, mapWidth - 1, ls * i);
    }

    p.end();
    pm.end();

    map.setMask(mask);

    m_staffAboveLabel = new QLabel("staff", exampleVBox);
    m_staffAboveLabel->setPixmap(map);

    m_textExampleLabel = new QLabel(i18n("Example"), exampleVBox);

    m_staffBelowLabel = new QLabel("staff", exampleVBox);
    m_staffBelowLabel->setPixmap(map);

    // restore last setting for shortcut combos
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    m_dynamicShortcutCombo->setCurrentItem(config->readNumEntry("dynamic_shortcut", 0));
    m_directionShortcutCombo->setCurrentItem(config->readNumEntry("direction_shortcut", 0));
    m_localDirectionShortcutCombo->setCurrentItem(config->readNumEntry("local_direction_shortcut", 0));
    m_tempoShortcutCombo->setCurrentItem(config->readNumEntry("tempo_shortcut", 0));
    m_localTempoShortcutCombo->setCurrentItem(config->readNumEntry("local_tempo_shortcut", 0));
    m_lilypondDirectiveCombo->setCurrentItem(config->readNumEntry("lilypond_directive_combo", 0));

    m_prevChord = config->readEntry("previous_chord", "");
    m_prevLyric = config->readEntry("previous_lyric", "");
    m_prevAnnotation = config->readEntry("previous_annotation", "");

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
    QObject::connect(m_lilypondDirectiveCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotLilypondDirectiveChanged(const QString &)));

    m_text->setFocus();
    slotTypeChanged(strtoqstr(getTextType()));

    // a hacky little fix for #1512143, to restore the capability to edit
    // existing annotations and other whatnots
    //!!! tacking another one of these on the bottom strikes me as lame in the
    // extreme, but it works, and it costs little, and other solutions I can
    // imagine would cost so much more.
    m_text->setText(strtoqstr(defaultText.getText()));
}

std::string
TextEventDialog::getTextType() const
{
    return m_styles[m_typeCombo->currentItem()];
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
    // Lilypond directives only taking temporary residence here; will move out
    // into some new class eventually
    //
    if (type == Text::LilypondDirective) {
        m_lilypondDirectiveCombo->show();
        m_directiveLabel->show();
        m_staffAboveLabel->hide();
        m_staffBelowLabel->show();
        m_text->setReadOnly(true);
        m_text->setEnabled(false);
        slotLilypondDirectiveChanged(text);
    } else {
        m_lilypondDirectiveCombo->hide();
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
    }
}

void
TextEventDialog::slotOK()
{
    // store last setting for shortcut combos
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    config->writeEntry("dynamic_shortcut", m_dynamicShortcutCombo->currentItem());
    config->writeEntry("direction_shortcut", m_directionShortcutCombo->currentItem());
    config->writeEntry("local_direction_shortcut", m_localDirectionShortcutCombo->currentItem());
    config->writeEntry("tempo_shortcut", m_tempoShortcutCombo->currentItem());
    config->writeEntry("local_tempo_shortcut", m_localTempoShortcutCombo->currentItem());
    // temporary home:
    config->writeEntry("lilypond_directive_combo", m_lilypondDirectiveCombo->currentItem());

    // store  last chord, lyric, annotation, depending on what's currently in
    // the text entry widget
    int index = m_typeCombo->currentItem();
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
TextEventDialog::slotLilypondDirectiveChanged(const QString &)
{
    m_text->setText(strtoqstr(m_lilypondDirectiveCombo->currentText()));
}

}
#include "TextEventDialog.moc"
