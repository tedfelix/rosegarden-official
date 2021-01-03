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

#define RG_MODULE_STRING "[TempoDialog]"

#include "TempoDialog.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/TimeWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QUrl>
#include <QDesktopServices>



namespace Rosegarden
{

TempoDialog::TempoDialog(QWidget *parent, RosegardenDocument *doc,
                         bool timeEditable):
        QDialog(parent),
        m_doc(doc),
        m_tempoTime(0)
{
    setModal(true);
    setWindowTitle(tr("Insert Tempo Change"));
    setObjectName("MinorDialog");

    QWidget* vbox = dynamic_cast<QWidget*>(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vbox->setLayout(vboxLayout);

    // group box for tempo
    QGroupBox *frame = new QGroupBox(tr("Tempo"), vbox);
    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);
    vboxLayout->addWidget(frame);

    // Set tempo
    layout->addWidget(new QLabel(tr("New tempo:"), frame), 0, 1);
    m_tempoValueSpinBox = new QDoubleSpinBox(frame);
    m_tempoValueSpinBox->setDecimals(3);
    m_tempoValueSpinBox->setMaximum(1000);
    m_tempoValueSpinBox->setMinimum(1);
    m_tempoValueSpinBox->setSingleStep(1);
    m_tempoValueSpinBox->setValue(120); // will set properly below
    layout->addWidget(m_tempoValueSpinBox, 0, 2);

    connect(m_tempoValueSpinBox, SIGNAL(valueChanged(double)),
            SLOT(slotTempoChanged(double)));

    m_tempoTap= new QPushButton(tr("Tap"), frame);
    layout->addWidget(m_tempoTap, 0, 3);
    connect(m_tempoTap, &QAbstractButton::clicked, this, &TempoDialog::slotTapClicked);


    m_tempoConstant = new QRadioButton(tr("Tempo is fixed until the following tempo change"), frame);
    m_tempoRampToNext = new QRadioButton(tr("Tempo ramps to the following tempo"), frame);
    m_tempoRampToTarget = new QRadioButton(tr("Tempo ramps to:"), frame);

    //    m_tempoTargetCheckBox = new QCheckBox(tr("Ramping to:"), frame);
    m_tempoTargetSpinBox = new QDoubleSpinBox(frame);
    m_tempoTargetSpinBox->setDecimals(3);
    m_tempoTargetSpinBox->setMaximum(1000);
    m_tempoTargetSpinBox->setMinimum(1);
    m_tempoTargetSpinBox->setSingleStep(1);
    m_tempoTargetSpinBox->setValue(120);

    //    layout->addWidget(m_tempoTargetCheckBox, 1, 0, 0+1, 1- 1, AlignRight);
    //    layout->addWidget(m_tempoTargetSpinBox, 1, 2);

    layout->addWidget(m_tempoConstant, 1, 1, 1, 2);
    layout->addWidget(m_tempoRampToNext, 2, 1, 1, 2);
    layout->addWidget(m_tempoRampToTarget, 3, 1);
    layout->addWidget(m_tempoTargetSpinBox, 3, 2);

    //    connect(m_tempoTargetCheckBox, SIGNAL(clicked()),
    //            SLOT(slotTargetCheckBoxClicked()));
    connect(m_tempoConstant, &QAbstractButton::clicked,
            this, &TempoDialog::slotTempoConstantClicked);
    connect(m_tempoRampToNext, &QAbstractButton::clicked,
            this, &TempoDialog::slotTempoRampToNextClicked);
    connect(m_tempoRampToTarget, &QAbstractButton::clicked,
            this, &TempoDialog::slotTempoRampToTargetClicked);
    connect(m_tempoTargetSpinBox, SIGNAL(valueChanged(double)),
            SLOT(slotTargetChanged(double)));

    m_tempoBeatLabel = new QLabel(frame);
    layout->addWidget(m_tempoBeatLabel, 0, 4);

    m_tempoBeat = new QLabel(frame);
    layout->addWidget(m_tempoBeat, 0, 5);

    m_tempoBeatsPerMinute = new QLabel(frame);
    layout->addWidget(m_tempoBeatsPerMinute, 0, 6);

    frame->setLayout(layout);

    m_timeEditor = nullptr;

    if (timeEditable) {

        m_timeEditor = new TimeWidget
            (tr("Time of tempo change"),
             vbox, &m_doc->getComposition(), 0, true);
        vboxLayout->addWidget(m_timeEditor);

    } else {
    
        // group box for scope (area)
        QGroupBox *scopeGroup = new QGroupBox(tr("Scope"), vbox);
        vboxLayout->addWidget(scopeGroup);

        QVBoxLayout * scopeBoxLayout = new QVBoxLayout(scopeGroup);
        scopeBoxLayout->setSpacing(5);
        scopeBoxLayout->setMargin(5);

        QVBoxLayout * currentBoxLayout = scopeBoxLayout;
        QWidget * currentBox = scopeGroup;
        
        QLabel *child_15 = new QLabel(tr("The pointer is currently at "), currentBox);
        currentBoxLayout->addWidget(child_15);
        m_tempoTimeLabel = new QLabel(currentBox);
        currentBoxLayout->addWidget(m_tempoTimeLabel);
        m_tempoBarLabel = new QLabel(currentBox);
        currentBoxLayout->addWidget(m_tempoBarLabel);
        QLabel *spare = new QLabel(currentBox);
        currentBoxLayout->addWidget(spare);
        currentBox->setLayout(currentBoxLayout);
        currentBoxLayout->setStretchFactor(spare, 20);

        m_tempoStatusLabel = new QLabel(scopeGroup);
        scopeBoxLayout->addWidget(m_tempoStatusLabel);
        scopeGroup->setLayout(scopeBoxLayout);

        QWidget * changeWhereBox = scopeGroup;
        QVBoxLayout * changeWhereBoxLayout = scopeBoxLayout;
        
        spare = new QLabel("      ", changeWhereBox);
        changeWhereBoxLayout->addWidget(spare);
        
        QWidget *changeWhereVBox = new QWidget(changeWhereBox);
        QVBoxLayout *changeWhereVBoxLayout = new QVBoxLayout;
        changeWhereBoxLayout->addWidget(changeWhereVBox);
        changeWhereBox->setLayout(changeWhereBoxLayout);
        changeWhereBoxLayout->setStretchFactor(changeWhereVBox, 20);

        m_tempoChangeHere = new QRadioButton(tr("Apply this tempo from here onwards"), changeWhereVBox);
        changeWhereVBoxLayout->addWidget(m_tempoChangeHere);

        m_tempoChangeBefore = new QRadioButton(tr("Replace the last tempo change"), changeWhereVBox);
        changeWhereVBoxLayout->addWidget(m_tempoChangeBefore);
        m_tempoChangeBeforeAt = new QLabel(changeWhereVBox);
        changeWhereVBoxLayout->addWidget(m_tempoChangeBeforeAt);
        m_tempoChangeBeforeAt->hide();

        m_tempoChangeStartOfBar = new QRadioButton(tr("Apply this tempo from the start of this bar"), changeWhereVBox);
        changeWhereVBoxLayout->addWidget(m_tempoChangeStartOfBar);

        m_tempoChangeGlobal = new QRadioButton(tr("Apply this tempo to the whole composition"), changeWhereVBox);
        changeWhereVBoxLayout->addWidget(m_tempoChangeGlobal);

        QWidget *optionHBox = new QWidget(changeWhereVBox);
        changeWhereVBoxLayout->addWidget(optionHBox);
        changeWhereVBox->setLayout(changeWhereVBoxLayout);
        QHBoxLayout *optionHBoxLayout = new QHBoxLayout;
        QLabel *child_6 = new QLabel("         ", optionHBox);
        optionHBoxLayout->addWidget(child_6);
        m_defaultBox = new QCheckBox(tr("Also make this the default tempo"), optionHBox);
        optionHBoxLayout->addWidget(m_defaultBox);
        spare = new QLabel(optionHBox);
        optionHBoxLayout->addWidget(spare);
        optionHBox->setLayout(optionHBoxLayout);
        optionHBoxLayout->setStretchFactor(spare, 20);

        connect(m_tempoChangeHere, &QAbstractButton::clicked,
                this, &TempoDialog::slotActionChanged);
        connect(m_tempoChangeBefore, &QAbstractButton::clicked,
                this, &TempoDialog::slotActionChanged);
        connect(m_tempoChangeStartOfBar, &QAbstractButton::clicked,
                this, &TempoDialog::slotActionChanged);
        connect(m_tempoChangeGlobal, &QAbstractButton::clicked,
                this, &TempoDialog::slotActionChanged);

        m_tempoChangeBefore->setChecked(true);

        // disable initially
        m_defaultBox->setEnabled(false);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    vboxLayout->addWidget(buttonBox);
    
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &TempoDialog::slotHelpRequested);

    populateTempo();    
}

TempoDialog::~TempoDialog()
{}

void
TempoDialog::setTempoPosition(timeT time)
{
    m_tempoTime = time;
    populateTempo();
}

void
TempoDialog::populateTempo()
{
    Composition &comp = m_doc->getComposition();
    tempoT tempo = comp.getTempoAtTime(m_tempoTime);
    std::pair<bool, tempoT> ramping(false, tempo);

    int tempoChangeNo = comp.getTempoChangeNumberAt(m_tempoTime);
    if (tempoChangeNo >= 0) {
        tempo = comp.getTempoChange(tempoChangeNo).second;
        ramping = comp.getTempoRamping(tempoChangeNo, false);
    }

    m_tempoValueSpinBox->setValue(comp.getTempoQpm(tempo));

    if (ramping.first) {
        if (ramping.second) {
            m_tempoTargetSpinBox->setEnabled(true);
            m_tempoTargetSpinBox->setValue(comp.getTempoQpm(ramping.second));
            m_tempoConstant->setChecked(false);
            m_tempoRampToNext->setChecked(false);
            m_tempoRampToTarget->setChecked(true);
        } else {
            ramping = comp.getTempoRamping(tempoChangeNo, true);
            m_tempoTargetSpinBox->setEnabled(false);
            m_tempoTargetSpinBox->setValue(comp.getTempoQpm(ramping.second));
            m_tempoConstant->setChecked(false);
            m_tempoRampToNext->setChecked(true);
            m_tempoRampToTarget->setChecked(false);
        }
    } else {
        m_tempoTargetSpinBox->setEnabled(false);
        m_tempoTargetSpinBox->setValue(comp.getTempoQpm(tempo));
        m_tempoConstant->setChecked(true);
        m_tempoRampToNext->setChecked(false);
        m_tempoRampToTarget->setChecked(false);
    }

    //    m_tempoTargetCheckBox->setChecked(ramping.first);
    m_tempoTargetSpinBox->setEnabled(ramping.first);

    updateBeatLabels(comp.getTempoQpm(tempo));

    if (m_timeEditor) {
        m_timeEditor->slotSetTime(m_tempoTime);
        return ;
    }

    RealTime tempoTime = comp.getElapsedRealTime(m_tempoTime);
    QString milliSeconds;
    milliSeconds.sprintf("%03d", tempoTime.msec());
    m_tempoTimeLabel->setText(tr("%1.%2 s,").arg(tempoTime.sec)
                               .arg(milliSeconds));

    int barNo = comp.getBarNumber(m_tempoTime);
    if (comp.getBarStart(barNo) == m_tempoTime) {
        m_tempoBarLabel->setText
        (tr("at the start of measure %1.").arg(barNo + 1));
        m_tempoChangeStartOfBar->setEnabled(false);
    } else {
        m_tempoBarLabel->setText(
            tr("in the middle of measure %1.").arg(barNo + 1));
        m_tempoChangeStartOfBar->setEnabled(true);
    }

    m_tempoChangeBefore->setEnabled(false);
    m_tempoChangeBeforeAt->setEnabled(false);

    bool havePrecedingTempo = false;

    if (tempoChangeNo >= 0) {

        timeT lastTempoTime = comp.getTempoChange(tempoChangeNo).first;
        if (lastTempoTime < m_tempoTime) {

            RealTime lastRT = comp.getElapsedRealTime(lastTempoTime);
            QString lastms;
            lastms.sprintf("%03d", lastRT.msec());
            int lastBar = comp.getBarNumber(lastTempoTime);
            m_tempoChangeBeforeAt->setText
            (tr("        (at %1.%2 s, in measure %3)").arg(lastRT.sec)
              .arg(lastms).arg(lastBar + 1));
            m_tempoChangeBeforeAt->show();

            m_tempoChangeBefore->setEnabled(true);
            m_tempoChangeBeforeAt->setEnabled(true);

            havePrecedingTempo = true;
        }
    }

    if (comp.getTempoChangeCount() > 0) {

        if (havePrecedingTempo) {
            m_tempoStatusLabel->hide();
        } else {
            m_tempoStatusLabel->setText
            (tr("There are no preceding tempo changes."));
        }

        m_tempoChangeGlobal->setEnabled(true);

    } else {

        m_tempoStatusLabel->setText
        (tr("There are no other tempo changes."));

        m_tempoChangeGlobal->setEnabled(false);
    }

    m_defaultBox->setEnabled(false);
}

void
TempoDialog::updateBeatLabels(double qpm)
{
    Composition &comp = m_doc->getComposition();

    // If the time signature's beat is not a crotchet, need to show
    // bpm separately

    timeT beat = comp.getTimeSignatureAt(m_tempoTime).getBeatDuration();
    if (beat == Note(Note::Crotchet).getDuration()) {
        m_tempoBeatLabel->setText(tr(" bpm"));
        m_tempoBeatLabel->show();
        m_tempoBeat->hide();
        m_tempoBeatsPerMinute->hide();
    } else {
        m_tempoBeatLabel->setText("  ");

        timeT error = 0;
        m_tempoBeat->setPixmap(NotePixmapFactory::makeNoteMenuPixmap(beat, error));
        m_tempoBeat->setMaximumWidth(25);
        if (error) {
            m_tempoBeat->setPixmap
                (NotePixmapFactory::makeToolbarPixmap("menu-no-note"));
        }

        m_tempoBeatsPerMinute->setText
            (QString("= %1 ").arg
             (int(qpm * Note(Note::Crotchet).getDuration() / beat)));
        m_tempoBeatLabel->show();
        m_tempoBeat->show();
        m_tempoBeatsPerMinute->show();
    }
}

void
TempoDialog::slotTempoChanged(double val)
{
    updateBeatLabels(val);
}

void
TempoDialog::slotTargetChanged(double)
{
    //...
}

void
TempoDialog::slotTempoConstantClicked()
{
    m_tempoRampToNext->setChecked(false);
    m_tempoRampToTarget->setChecked(false);
    m_tempoTargetSpinBox->setEnabled(false);
}

void
TempoDialog::slotTempoRampToNextClicked()
{
    m_tempoConstant->setChecked(false);
    m_tempoRampToTarget->setChecked(false);
    m_tempoTargetSpinBox->setEnabled(false);
}

void
TempoDialog::slotTempoRampToTargetClicked()
{
    m_tempoConstant->setChecked(false);
    m_tempoRampToNext->setChecked(false);
    m_tempoTargetSpinBox->setEnabled(true);
}

void
TempoDialog::slotActionChanged()
{
    m_defaultBox->setEnabled(m_tempoChangeGlobal->isChecked());
}

void
TempoDialog::accept()
{
    tempoT tempo = Composition::getTempoForQpm(m_tempoValueSpinBox->value());
    RG_DEBUG << "Tempo is " << tempo;

    tempoT target = -1;
    if (m_tempoRampToNext->isChecked()) {
        target = 0;
    } else if (m_tempoRampToTarget->isChecked()) {
        target = Composition::getTempoForQpm(m_tempoTargetSpinBox->value());
    }

    RG_DEBUG << "Target is " << target;

    if (m_timeEditor) {

        emit changeTempo(m_timeEditor->getTime(),
                         tempo,
                         target,
                         AddTempo);

    } else {

        TempoDialogAction action = AddTempo;

        if (m_tempoChangeBefore->isChecked()) {
            action = ReplaceTempo;
        } else if (m_tempoChangeStartOfBar->isChecked()) {
            action = AddTempoAtBarStart;
        } else if (m_tempoChangeGlobal->isChecked()) {
            action = GlobalTempo;
            if (m_defaultBox->isChecked()) {
                action = GlobalTempoWithDefault;
            }
        }

        emit changeTempo(m_tempoTime,
                         tempo,
                         target,
                         action);
    }

    QDialog::accept();
}

void
TempoDialog::slotTapClicked()
{
    QTime now = QTime::currentTime();

    if (m_tapMinusOne != QTime()) {

        int ms1 = m_tapMinusOne.msecsTo(now);

        if (ms1 < 10000) {

            int msec = ms1;

            if (m_tapMinusTwo != QTime()) {
                int ms2 = m_tapMinusTwo.msecsTo(m_tapMinusOne);
                if (ms2 < 10000) {
                    msec = (ms1 + ms2) / 2;
                }
            }

            int bpm = 60000 / msec;
            m_tempoValueSpinBox->setValue(bpm);
        }
    }

    m_tapMinusTwo = m_tapMinusOne;
    m_tapMinusOne = now;
}



void
TempoDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:tempoDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:tempoDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}

