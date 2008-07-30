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


#include "LyricEditDialog.h"

#include "base/Event.h"
#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include <kdialogbase.h>
#include <qgroupbox.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextedit.h>
#include <qvbox.h>
#include <qwidget.h>
#include <kcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>


namespace Rosegarden
{

LyricEditDialog::LyricEditDialog(QWidget *parent,
                                 Segment *segment) :
    KDialogBase(parent, 0, true, i18n("Edit Lyrics"), Ok | Cancel | Help),
    m_segment(segment),
    m_verseCount(0)
{
    setHelp("nv-text-lyrics");

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Lyrics for this segment"), vbox);

    QHBox *hbox = new QHBox(groupBox);
    hbox->setSpacing(5);
//    new QLabel(i18n("Verse:"), hbox);
    m_verseNumber = new KComboBox(hbox);
    m_verseNumber->setEditable(false);
    connect(m_verseNumber, SIGNAL(activated(int)), this, SLOT(slotVerseNumberChanged(int)));
    m_verseAddButton = new QPushButton(i18n("Add Verse"), hbox);
    connect(m_verseAddButton, SIGNAL(clicked()), this, SLOT(slotAddVerse()));
    m_verseRemoveButton = new QPushButton(i18n("Remove Verse"), hbox);
    connect(m_verseRemoveButton, SIGNAL(clicked()), this, SLOT(slotRemoveVerse()));
    QFrame *f = new QFrame(hbox);
    hbox->setStretchFactor(f, 10);

    m_textEdit = new QTextEdit(groupBox);
    m_textEdit->setTextFormat(Qt::PlainText);

    m_textEdit->setMinimumWidth(300);
    m_textEdit->setMinimumHeight(200);

    m_currentVerse = 0;
    unparse();
    verseDialogRepopulate();

}

void
LyricEditDialog::slotVerseNumberChanged(int verse)
{
    NOTATION_DEBUG << "LyricEditDialog::slotVerseNumberChanged(" << verse << ")" << endl;

    QString text = m_textEdit->text();
    m_texts[m_currentVerse] = text;
    m_textEdit->setText(m_texts[verse]);
    m_currentVerse = verse;
}

void
LyricEditDialog::slotAddVerse()
{
    NOTATION_DEBUG << "LyricEditDialog::slotAddVerse" << endl;

    m_texts.push_back(m_skeleton);

    m_verseCount++;

// NOTE slotVerseNumberChanged should be called with m_currentVerse argument
//  if we ever decide to add new verse between existing ones
    slotVerseNumberChanged(m_verseCount - 1);
    verseDialogRepopulate();
}

void
LyricEditDialog::slotRemoveVerse()
{
    NOTATION_DEBUG << "LyricEditDialog::slotRemoveVerse" << endl;

    std::cerr << "deleting at position " << m_currentVerse << std::endl;
    std::vector<QString>::iterator itr = m_texts.begin();
    for (int i = 0; i < m_currentVerse; ++i) ++itr;

    std::cerr << "text being deleted is: " << *itr << std::endl;
    m_texts.erase(itr);

    m_verseCount--;
    if (m_currentVerse == m_verseCount) m_currentVerse--;
    verseDialogRepopulate();
}

void
LyricEditDialog::countVerses()
{
    m_verseCount = 1;

    for (Segment::iterator i = m_segment->begin();
         m_segment->isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Text::EventType)) {

            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {

                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);

                if (verse >= m_verseCount) m_verseCount = verse + 1;
            }
        }
    }
}

void
LyricEditDialog::unparse()
{
    // This and SetLyricsCommand::execute() are opposites that will
    // need to be kept in sync with any changes in one another.  (They
    // should really both be in a common lyric management class.)

    countVerses();

    Composition *comp = m_segment->getComposition();

    bool firstNote = true;
    timeT lastTime = m_segment->getStartTime();
    int lastBarNo = comp->getBarNumber(lastTime);
    std::map<int, bool> haveLyric;

    QString fragment = QString("[%1] ").arg(lastBarNo + 1);

    m_skeleton = fragment;
    m_texts.clear();
    for (size_t v = 0; v < m_verseCount; ++v) {
        m_texts.push_back(fragment);
        haveLyric[v] = false;
    }

    for (Segment::iterator i = m_segment->begin();
         m_segment->isBeforeEndMarker(i); ++i) {

        bool isNote = (*i)->isa(Note::EventType);
        bool isLyric = false;

        if (!isNote) {
            if ((*i)->isa(Text::EventType)) {
                std::string textType;
                if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                    textType == Text::Lyric) {
                    isLyric = true;
                }
            }
        } else {
            if ((*i)->has(BaseProperties::TIED_BACKWARD) &&
                (*i)->get<Bool>(BaseProperties::TIED_BACKWARD)) {
                continue;
            }
        }

        if (!isNote && !isLyric) continue;

        timeT myTime = (*i)->getNotationAbsoluteTime();
        int myBarNo = comp->getBarNumber(myTime);

        if (myBarNo > lastBarNo) {

            fragment = "";

            while (myBarNo > lastBarNo) {
                fragment += " /";
                ++lastBarNo;
            }

            fragment += QString("\n[%1] ").arg(myBarNo + 1);

            m_skeleton += fragment;
            for (size_t v = 0; v < m_verseCount; ++v) m_texts[v] += fragment;
        }

        if (isNote) {
            if ((myTime > lastTime) || firstNote) {
                m_skeleton += " .";
                for (size_t v = 0; v < m_verseCount; ++v) {
                    if (!haveLyric[v]) m_texts[v] += " .";
                    haveLyric[v] = false;
                }
                lastTime = myTime;
                firstNote = false;
            }
        }

        if (isLyric) {

            std::string ssyllable;
            (*i)->get<String>(Text::TextPropertyName, ssyllable);

            long verse = 0;
            (*i)->get<Int>(Text::LyricVersePropertyName, verse);

            QString syllable(strtoqstr(ssyllable));
            syllable.replace(QRegExp("\\s+"), "~");

            m_texts[verse] += " " + syllable;
            haveLyric[verse] = true;
        }
    }

    if (!m_texts.empty()) {
        m_textEdit->setText(m_texts[0]);
    } else {
        m_texts.push_back(m_skeleton);
    }
}

int
LyricEditDialog::getVerseCount() const
{
    return m_verseCount;
}

QString
LyricEditDialog::getLyricData(int verse) const
{
    if (verse == m_verseNumber->currentItem()) {
        return m_textEdit->text();
    } else {
        return m_texts[verse];
    }
}

void
LyricEditDialog::verseDialogRepopulate()
{
    m_verseNumber->clear();

    for (int i = 0; i < m_verseCount; ++i) {
        m_verseNumber->insertItem(i18n("Verse %1").arg(i + 1));
    }

    if (m_verseCount == 12)
        m_verseAddButton->setEnabled(false);
    else
        m_verseAddButton->setEnabled(true);

    if (m_verseCount == 1)
        m_verseRemoveButton->setEnabled(false);
    else
        m_verseRemoveButton->setEnabled(true);

    m_verseNumber->setCurrentItem(m_currentVerse);

    std::cerr << "m_currentVerse = " << m_currentVerse << ", text = " << m_texts[m_currentVerse] << std::endl;
    m_textEdit->setText(m_texts[m_currentVerse]);
}

}
#include "LyricEditDialog.moc"
