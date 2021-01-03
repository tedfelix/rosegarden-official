/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
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

#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QString>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>


namespace Rosegarden
{

LyricEditDialog::LyricEditDialog(QWidget *parent,
                                 std::vector<Segment *> &segments,
                                 Segment *segment) :
    QDialog(parent),
    m_segment(segment),
    m_segmentSelectMenu(nullptr),
    m_descr1(nullptr),
    m_descr2(nullptr),
    m_verseCount(0),
    m_previousVerseCount(0)
{
    setModal(true);
    setWindowTitle(tr("Edit Lyrics"));

    // If several segments, setup a menu to change the selected one
    if (segments.size() > 1) {
        m_segmentSelectMenu = new QMenu(this);
        m_menuActionsMap.clear();
        Composition *comp = m_segment->getComposition();

        // Associate a description with each segment and use a multimap
        // to sort them
        std::multimap<QString, Segment *> segDescriptionMap;

        for (std::vector<Segment *>::iterator it = segments.begin();
                it != segments.end(); ++it) {

            // Get segment characteristics
            timeT segStart = (*it)->getStartTime();
            timeT segEnd = (*it)->getEndMarkerTime();
            int barStart = comp->getBarNumber(segStart) + 1;
            int barEnd = comp->getBarNumber(segEnd - 1) + 1;
            QString label = strtoqstr((*it)->getLabel());

            // Shorten too long labels
            if (label.length() > 53) label = label.left(50) + "...";

            // Create the description
            QString segDescr = QString(tr("Track %1, bar %2 to %3: \"%4\""))
                    .arg(comp->getTrackPositionById((*it)->getTrack()) + 1)
                    .arg(barStart)
                    .arg(barEnd)
                    .arg(label);

            // Insert description and segment in the map
            segDescriptionMap.insert(std::pair<QString, Segment *>(segDescr, *it));
        }

        // Populate the menu with an entry for each segment.
        // Reading the segments from the multimap sorts them by description.
        for (std::multimap<QString, Segment *>::iterator it = segDescriptionMap.begin();
                it != segDescriptionMap.end(); ++it) {
            m_menuActionsMap[m_segmentSelectMenu->addAction((*it).first)] = (*it).second;
        }
    }

    // Begin dialog layout
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    // Add the following elements of layout only if more than
    // one segment is opened in the notation editor
    if (m_segmentSelectMenu) {

        // QLabels to display a description of the selected segment
        m_descr1 = new QLabel("");
        vboxLayout->addWidget(m_descr1);
        m_descr2 = new QLabel("");
        vboxLayout->addWidget(m_descr2);

        // Write out the description
        showDescriptionOfSelectedSegment();

        // Add a button to give the user a chance to select another segment
        QPushButton *selectSegment = new QPushButton(tr("Select another segment"));

        // Avoid button to become exaggeratedly large when dialog is resized
        QWidget *hb = new QWidget(vbox);
        QHBoxLayout *hbl = new QHBoxLayout;
        hb->setLayout(hbl);
        vboxLayout->addWidget(hb);
        QFrame *fr = new QFrame(hb);
        hbl->addWidget(fr);
        hbl->setStretchFactor(fr, 10);
        hbl->addWidget(selectSegment);

        // Connect the button to the menu and the menu to the appropriate slot
        selectSegment->setMenu(m_segmentSelectMenu);
        connect(m_segmentSelectMenu, &QMenu::triggered,
                this, &LyricEditDialog::slotSegmentChanged);
    }

    // Continue dialog layout
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
    connect(m_verseAddButton, &QAbstractButton::clicked, this, &LyricEditDialog::slotAddVerse);
    m_verseRemoveButton = new QPushButton(tr("Remove Verse"), hbox );
    hboxLayout->addWidget(m_verseRemoveButton);
    connect(m_verseRemoveButton, &QAbstractButton::clicked, this, &LyricEditDialog::slotRemoveVerse);

    // Avoid buttons to become exaggeratedly large when dialog is resized
    QFrame *f = new QFrame( hbox );
    hboxLayout->addWidget(f);
    hbox->setLayout(hboxLayout);
    hboxLayout->setStretchFactor(f, 10);

    m_textEdit = new QTextEdit(groupBox);
    groupBoxLayout->addWidget(m_textEdit);

    m_textEdit->setMinimumWidth(300);
    m_textEdit->setMinimumHeight(200);

    m_currentVerse = 0;
    unparse();
    verseDialogRepopulate();

    m_previousTexts = m_texts;
    m_previousVerseCount = m_verseCount;

    m_textEdit->setFocus();

    groupBox->setLayout(groupBoxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel
                                                     | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &LyricEditDialog::slotHelpRequested);
}

void
LyricEditDialog::showDescriptionOfSelectedSegment()
{
    // Get selected segment characteristics
    Composition *comp = m_segment->getComposition();
    timeT segStart = m_segment->getStartTime();
    timeT segEnd = m_segment->getEndMarkerTime();
    int barStart = comp->getBarNumber(segStart) + 1;
    int barEnd = comp->getBarNumber(segEnd - 1) + 1;
    QString label = strtoqstr(m_segment->getLabel());

    // Shorten too long labels
    if (label.length() > 53) label = label.left(50) + "...";

    // Create the description (on two lines)
    QString descr1 = QString(tr("Selected segment lays on track %1, bar %2 to %3"))
            .arg(comp->getTrackPositionById(m_segment->getTrack()) + 1)
            .arg(barStart)
            .arg(barEnd);
    QString descr2 = QString(tr("and is labeled \"%1\""))
            .arg(label);

    // Write out the two lines
    m_descr1->setText(descr1);
    m_descr2->setText(descr2);
}

void
LyricEditDialog::slotSegmentChanged(QAction * a)
{
    Segment * newSeg= m_menuActionsMap[a];

    // Do nothing if segment unchanged
    if (m_segment == newSeg) return;

    // Lyrics of current segment have been modified
    //     (1) if verse count has decreased
    //     (2) or if some preexisting verse have changed
    //     (3) or if verse count has increased and new verses are not skeletons

    bool changed = false;
    if (m_verseCount < m_previousVerseCount) {
        changed = true;                  // (1)
    } else {
        for (int i = 0; i < m_verseCount; i++) {
            if (i < m_previousVerseCount) {
                if (m_previousTexts[i] != getLyricData(i)) {
                    changed = true;      // (2)
                    break;
                }
            } else {
                if (getLyricData(i) != m_skeleton) {
                    changed = true;      // (3)
                    break;
                }
            }
        }
    }

    // If lyrics have been modified, give the user a chance to keep the changes
    if (changed) {
        int okToChange = QMessageBox::warning( this, tr("Rosegarden - Warning"),
            tr("<qt><p>The current segment lyrics have been modified.</p>"
               "<p>The modifications will be lost if a new segment is selected.</p>"
               "<p>Do you really want to select a new segment?</p></qt>"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

        // Do nothing if user replied no
        if (okToChange != QMessageBox::Yes) return;
    }

    m_segment = newSeg;
    showDescriptionOfSelectedSegment();

    m_texts.clear();
    m_currentVerse = 0;
    unparse();
    verseDialogRepopulate();

    m_previousTexts = m_texts;
    m_previousVerseCount = m_verseCount;

    m_textEdit->setFocus();
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

    // If no verse, add an empty one to give a workplace to the user
    // (else the user would need to press the "add verse" button)
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
