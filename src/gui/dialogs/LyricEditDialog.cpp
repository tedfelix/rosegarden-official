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


#include "LyricEditDialog.h"


#include "base/Event.h"
#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRegExp>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

LyricEditDialog::LyricEditDialog(QWidget *parent,
                                 Segment *segment) :
    QDialog(parent),
    m_segment(segment),
    m_verseCount(0)
{
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    setModal(true);
    setWindowTitle(tr("Edit Lyrics"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *groupBox = new QGroupBox( tr("Lyrics for this segment"), vbox );
    QVBoxLayout *groupBoxLayout = new QVBoxLayout;
    vboxLayout->addWidget(groupBox);
    vbox->setLayout(vboxLayout);

    QWidget *hbox = new QWidget(groupBox);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    groupBoxLayout->addWidget(hbox);
    hboxLayout->setSpacing(5);
//    new QLabel(tr("Verse:"), hbox);
    m_verseNumber = new QComboBox( hbox );
    hboxLayout->addWidget(m_verseNumber);
    m_verseNumber->setEditable(false);
    connect(m_verseNumber, SIGNAL(activated(int)), this, SLOT(slotVerseNumberChanged(int)));
    m_verseAddButton = new QPushButton(tr("Add Verse"), hbox );
    hboxLayout->addWidget(m_verseAddButton);
    connect(m_verseAddButton, SIGNAL(clicked()), this, SLOT(slotAddVerse()));
    m_verseRemoveButton = new QPushButton(tr("Remove Verse"), hbox );
    hboxLayout->addWidget(m_verseRemoveButton);
    connect(m_verseRemoveButton, SIGNAL(clicked()), this, SLOT(slotRemoveVerse()));
    QFrame *f = new QFrame( hbox );
    hboxLayout->addWidget(f);
    hbox->setLayout(hboxLayout);
    hboxLayout->setStretchFactor(f, 10);

    m_textEdit = new QTextEdit(groupBox);
    groupBoxLayout->addWidget(m_textEdit);
    if (Thorn) m_textEdit->setStyleSheet("background: white");

    m_textEdit->setMinimumWidth(300);
    m_textEdit->setMinimumHeight(200);

    m_currentVerse = 0;
    unparse();
    verseDialogRepopulate();

    //&&& QTextEdit has a new API, and it's not clear what the analog of this
    // function should be.  Since this setCursorPosition(0,0) looks like a very
    // default kind of thing, I'm going out on a limb and guessing that this is
    // probably now superfluous.  I figure something like the cursor position in
    // the lyric editor being messed up is much easier to address if and when we
    // can see the problem, so this is a good candidate for outright removal,
    // flagged appropriately. (dmm)
    //
    // m_textEdit->setCursorPosition(0,0);
    m_textEdit->setFocus();

    groupBox->setLayout(groupBoxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}

void
LyricEditDialog::slotVerseNumberChanged(int verse)
{
    NOTATION_DEBUG << "LyricEditDialog::slotVerseNumberChanged(" << verse << ")";

    QString text = m_textEdit->toPlainText();
    m_texts[m_currentVerse] = text;
    m_textEdit->setPlainText(m_texts[verse]);
    m_currentVerse = verse;
}

void
LyricEditDialog::slotAddVerse()
{
    NOTATION_DEBUG << "LyricEditDialog::slotAddVerse";

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
    NOTATION_DEBUG << "LyricEditDialog::slotRemoveVerse";

    RG_DEBUG << "deleting at position " << m_currentVerse;
    std::vector<QString>::iterator itr = m_texts.begin();
    for (int i = 0; i < m_currentVerse; ++i) ++itr;

    RG_DEBUG << "text being deleted is: " << *itr;
    if (m_verseCount > 1) {
        m_texts.erase(itr);
        m_verseCount--;
        if (m_currentVerse == m_verseCount) m_currentVerse--;
    } else {
        RG_DEBUG << "deleting last verse";
        m_texts.clear();
        m_texts.push_back(m_skeleton);
    }
    verseDialogRepopulate();
}

void
LyricEditDialog::countVerses()
{
    m_verseCount = m_segment->getVerseCount();
    if (m_verseCount == 0) m_verseCount = 1;
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
    for (int v = 0; v < m_verseCount; ++v) {
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
            for (int v = 0; v < m_verseCount; ++v) m_texts[v] += fragment;
        }

        if (isNote) {
            if ((myTime > lastTime) || firstNote) {
                m_skeleton += " .";
                for (int v = 0; v < m_verseCount; ++v) {
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
        m_textEdit->setPlainText(m_texts[0]);
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
    if (verse == m_verseNumber->currentIndex()) {
        return m_textEdit->toPlainText();
    } else {
        return m_texts[verse];
    }
}

void
LyricEditDialog::verseDialogRepopulate()
{
    m_verseNumber->clear();

    for (int i = 0; i < m_verseCount; ++i) {
        m_verseNumber->addItem(tr("Verse %1").arg(i + 1));
    }

    if (m_verseCount == 12)
        m_verseAddButton->setEnabled(false);
    else
        m_verseAddButton->setEnabled(true);

    if (m_verseCount == 0)
        m_verseRemoveButton->setEnabled(false);
    else
        m_verseRemoveButton->setEnabled(true);

    m_verseNumber->setCurrentIndex(m_currentVerse);

    RG_DEBUG << "m_currentVerse = " << m_currentVerse << ", text = " << m_texts[m_currentVerse];
    m_textEdit->setPlainText(m_texts[m_currentVerse]);
}


void
LyricEditDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:lyricEditDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:lyricEditDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
