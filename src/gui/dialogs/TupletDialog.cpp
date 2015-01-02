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


#include "TupletDialog.h"
#include <QLayout>

#include "base/NotationTypes.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

TupletDialog::TupletDialog(QWidget *parent, Note::Type defaultUnitType,
                           timeT maxDuration) :
        QDialog(parent),
        m_maxDuration(maxDuration)
{
    //setHelp("nv-tuplets");
    setModal(true);
    setWindowTitle(tr("Tuplet"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *timingBox = new QGroupBox( tr("New timing for tuplet group"), vbox );
    timingBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *timingLayout = new QGridLayout(timingBox);
    timingLayout->setSpacing(5);
    vboxLayout->addWidget(timingBox);

    if (m_maxDuration > 0) {

        // bit of a sanity check
        if (maxDuration < Note(Note::Semiquaver).getDuration()) {
            maxDuration = Note(Note::Semiquaver).getDuration();
        }

        Note::Type maxUnitType =
            Note::getNearestNote(maxDuration / 2, 0).getNoteType();
        if (defaultUnitType > maxUnitType)
            defaultUnitType = maxUnitType;
    }

    timingLayout->addWidget(new QLabel(tr("Play "), timingBox), 0, 0);

    m_untupledCombo = new QComboBox(timingBox);
    timingLayout->addWidget(m_untupledCombo, 0, 1);

    m_unitCombo = new QComboBox(timingBox);
    timingLayout->addWidget(m_unitCombo, 0, 2);

    for (Note::Type t = Note::Shortest; t <= Note::Longest; ++t) {
        Note note(t);
        timeT duration(note.getDuration());
        if (maxDuration > 0 && (2 * duration > maxDuration))
            break;
        timeT e; // error factor, ignore
        m_unitCombo->addItem(NotePixmapFactory::makeNoteMenuPixmap(duration, e),
                             NotationStrings::makeNoteMenuLabel(duration, false, e, true));
        if (defaultUnitType == t) {
            m_unitCombo->setCurrentIndex(m_unitCombo->count() - 1);
        }
    }

    timingLayout->addWidget(new QLabel(tr("in the time of  "), timingBox), 1, 0);

    m_tupledCombo = new QComboBox(timingBox);
    timingLayout->addWidget(m_tupledCombo, 1, 1);

    m_hasTimingAlready = new QCheckBox
        (tr("Timing is already correct: update display only"), timingBox);
    m_hasTimingAlready->setChecked(false);
    timingLayout->addWidget(m_hasTimingAlready, 2, 0, 1, 3);

    timingBox->setLayout(timingLayout);

    connect(m_hasTimingAlready, SIGNAL(clicked()), this, SLOT(slotHasTimingChanged()));

    updateUntupledCombo();
    updateTupledCombo();

    m_timingDisplayGrid = new QGroupBox( tr("Timing calculations"), vbox );
    QGridLayout *timingDisplayLayout = new QGridLayout(m_timingDisplayGrid);
    vboxLayout->addWidget(m_timingDisplayGrid);

    int row = 0;

    if (maxDuration > 0) {

        timingDisplayLayout->addWidget(new QLabel(tr("Selected region:")),
                                         row, 0);
        timingDisplayLayout->addWidget(new QLabel(""), row, 1);
        m_selectionDurationDisplay = new QLabel("x");
        timingDisplayLayout->addWidget(m_selectionDurationDisplay, row, 2);
        m_selectionDurationDisplay->setAlignment(Qt::AlignVCenter |
                Qt::AlignRight);
        row++;
    } else {
        m_selectionDurationDisplay = 0;
    }

    timingDisplayLayout->addWidget(new QLabel(tr("Group with current timing:")),
                                     row, 0);
    m_untupledDurationCalculationDisplay = new QLabel("x");
    timingDisplayLayout->addWidget(m_untupledDurationCalculationDisplay,
                                     row, 1);
    m_untupledDurationDisplay = new QLabel("x");
    m_untupledDurationDisplay->setAlignment(Qt::AlignVCenter |
                                            Qt::AlignRight);
    timingDisplayLayout->addWidget(m_untupledDurationDisplay, row, 2);
    row++;

    timingDisplayLayout->addWidget(new QLabel(tr("Group with new timing:")),
                                     row, 0);
    m_tupledDurationCalculationDisplay = new QLabel("x");
    timingDisplayLayout->addWidget(m_tupledDurationCalculationDisplay,
                                     row, 1);
    m_tupledDurationDisplay = new QLabel("x");
    m_tupledDurationDisplay->setAlignment(Qt::AlignVCenter |
                                          Qt::AlignRight);
    timingDisplayLayout->addWidget(m_tupledDurationDisplay, row, 2);
    row++;

    timingDisplayLayout->addWidget(new QLabel(tr("Gap created by timing change:")),
                                     row, 0);
    m_newGapDurationCalculationDisplay = new QLabel("x");
    timingDisplayLayout->addWidget(m_newGapDurationCalculationDisplay,
                                     row, 1);
    m_newGapDurationDisplay = new QLabel("x");
    m_newGapDurationDisplay->setAlignment(Qt::AlignVCenter |
                                          Qt::AlignRight);
    timingDisplayLayout->addWidget(m_newGapDurationDisplay, row, 2);
    row++;

    if (maxDuration > 0) {

        timingDisplayLayout->addWidget(new QLabel(tr("Unchanged at end of selection:")),
                                         row, 0);
        m_unchangedDurationCalculationDisplay = new QLabel("x");
        timingDisplayLayout->addWidget(m_unchangedDurationCalculationDisplay,
                                         row, 1);
        m_unchangedDurationDisplay = new QLabel("x");
        m_unchangedDurationDisplay->setAlignment(Qt::AlignVCenter |
                                                 Qt::AlignRight);
        timingDisplayLayout->addWidget(m_unchangedDurationDisplay, row, 2);

    } else {
        m_unchangedDurationDisplay = 0;
    }

    m_timingDisplayGrid->setLayout(timingDisplayLayout);

    vbox->setLayout(vboxLayout);

    updateTimingDisplays();

    QObject::connect(m_unitCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotUnitChanged(const QString &)));

    QObject::connect(m_untupledCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotUntupledChanged(const QString &)));
    QObject::connect(m_untupledCombo, SIGNAL(textChanged(const QString &)),
                     this, SLOT(slotUntupledChanged(const QString &)));

    QObject::connect(m_tupledCombo, SIGNAL(activated(const QString &)),
                     this, SLOT(slotTupledChanged(const QString &)));
    QObject::connect(m_tupledCombo, SIGNAL(textChanged(const QString &)),
                     this, SLOT(slotTupledChanged(const QString &)));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}

void
TupletDialog::slotHasTimingChanged()
{
    updateUntupledCombo();
    updateTupledCombo();
    m_timingDisplayGrid->setEnabled(!m_hasTimingAlready->isChecked());
}

Note::Type
TupletDialog::getUnitType() const
{
    return Note::Shortest + m_unitCombo->currentIndex();
}

int
TupletDialog::getUntupledCount() const
{
    bool isNumeric = true;
    int count = m_untupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric)
        return 1;
    else
        return count;
}

int
TupletDialog::getTupledCount() const
{
    bool isNumeric = true;
    int count = m_tupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric)
        return 1;
    else
        return count;
}

bool
TupletDialog::hasTimingAlready() const
{
    return m_hasTimingAlready->isChecked();
}

void
TupletDialog::updateUntupledCombo()
{
    // Untupled combo can contain numbers up to the maximum
    // duration divided by the unit duration.  If there's no
    // maximum, we'll have to put in some likely values and
    // allow the user to edit it.  Both the numerical combos
    // should possibly be spinboxes, except I think I like
    // being able to "suggest" a few values

    int maximum = 12;

    if (m_maxDuration) {
        if (m_hasTimingAlready->isChecked()) {
            maximum = (m_maxDuration * 2) / Note(getUnitType()).getDuration();
        } else {
            maximum = m_maxDuration / Note(getUnitType()).getDuration();
        }
    }

    QString previousText = m_untupledCombo->currentText();
    if (previousText.toInt() == 0) {
        if (maximum < 3)
            previousText = QString("%1").arg(maximum);
        else
            previousText = "3";
    }

    m_untupledCombo->clear();
    bool setText = false;

    for (int i = 1; i <= maximum; ++i) {
        QString text = QString("%1").arg(i);
        m_untupledCombo->addItem(text);
        if (m_hasTimingAlready->isChecked()) {
            if (i == (m_maxDuration * 3) / (Note(getUnitType()).getDuration()*2)) {
                m_untupledCombo->setCurrentIndex(m_untupledCombo->count() - 1);
            }
        } else if (text == previousText) {
            m_untupledCombo->setCurrentIndex(m_untupledCombo->count() - 1);
            setText = true;
        }
    }

    if (!setText) {
        m_untupledCombo->setEditText(previousText);
    }
}

void
TupletDialog::updateTupledCombo()
{
    // should contain all positive integers less than the
    // largest value in the untupled combo.  In principle
    // we can support values larger, but we can't quite
    // do the tupleting transformation yet

    int untupled = getUntupledCount();

    QString previousText = m_tupledCombo->currentText();
    if (previousText.toInt() == 0 ||
            previousText.toInt() > untupled) {
        if (untupled < 2)
            previousText = QString("%1").arg(untupled);
        else
            previousText = "2";
    }

    m_tupledCombo->clear();

    for (int i = 1; i < untupled; ++i) {
        QString text = QString("%1").arg(i);
        m_tupledCombo->addItem(text);
        if (m_hasTimingAlready->isChecked()) {
            if (i == m_maxDuration / Note(getUnitType()).getDuration()) {
                m_tupledCombo->setCurrentIndex(m_tupledCombo->count() - 1);
            }
        } else if (text == previousText) {
            m_tupledCombo->setCurrentIndex(m_tupledCombo->count() - 1);
        }
    }
}

void
TupletDialog::updateTimingDisplays()
{
    timeT unitDuration = Note(getUnitType()).getDuration();

    int untupledCount = getUntupledCount();
    int tupledCount = getTupledCount();

    timeT untupledDuration = unitDuration * untupledCount;
    timeT tupledDuration = unitDuration * tupledCount;

    if (m_selectionDurationDisplay) {
        m_selectionDurationDisplay->setText(QString("%1").arg(m_maxDuration));
    }

    m_untupledDurationCalculationDisplay->setText
    (QString("  %1 x %2 = ").arg(untupledCount).arg(unitDuration));
    m_untupledDurationDisplay->setText
    (QString("%1").arg(untupledDuration));

    m_tupledDurationCalculationDisplay->setText
    (QString("  %1 x %2 = ").arg(tupledCount).arg(unitDuration));
    m_tupledDurationDisplay->setText
    (QString("%1").arg(tupledDuration));

    m_newGapDurationCalculationDisplay->setText
    (QString("  %1 - %2 = ").arg(untupledDuration).arg(tupledDuration));
    m_newGapDurationDisplay->setText
    (QString("%1").arg(untupledDuration - tupledDuration));

    if (m_selectionDurationDisplay && m_unchangedDurationDisplay) {
        if (m_maxDuration != untupledDuration) {
            m_unchangedDurationCalculationDisplay->setText
            (QString("  %1 - %2 = ").arg(m_maxDuration).arg(untupledDuration));
        } else {
            m_unchangedDurationCalculationDisplay->setText("");
        }
        m_unchangedDurationDisplay->setText
        (QString("%1").arg(m_maxDuration - untupledDuration));
    }
}

void
TupletDialog::slotUnitChanged(const QString &)
{
    updateUntupledCombo();
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotUntupledChanged(const QString &)
{
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotTupledChanged(const QString &)
{
    updateTimingDisplays();
}


void
TupletDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:tupletDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:tupletDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
#include "TupletDialog.moc"
