/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiFilterDialog]"
#define RG_NO_DEBUG_PRINT

#include "MidiFilterDialog.h"

#include "document/RosegardenDocument.h"
#include "gui/seqmanager/SequenceManager.h"
#include "sound/MappedEvent.h"
#include "misc/Debug.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

MidiFilterDialog::MidiFilterDialog(QWidget *parent,
                                   RosegardenDocument *doc):
    QDialog(parent),
    m_doc(doc)
{
    setModal(true);
    setWindowTitle(tr("Modify MIDI Filters"));

    // Grid Layout for the button box at the bottom.
    // ??? metaGrid is only used for the button box at the bottom.
    //     QVBoxLayout would suffice, I think.  Even better, use the
    //     grid layout for the two group boxes and the button box.
    //     That will remove one layout.
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    // HBox Layout for the two group boxes.
    QWidget *hBox = new QWidget(this);
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    metagrid->addWidget(hBox, 0, 0);

    // THRU

    m_thruBox = new QGroupBox(tr("THRU events to ignore"), hBox );
    // VBox Layout for the check boxes in the THRU group box.
    QVBoxLayout *thruBoxLayout = new QVBoxLayout;
    hBoxLayout->addWidget(m_thruBox);

    // ??? Rearrange.

    m_noteThru = new QCheckBox(tr("Note"), m_thruBox);
    m_progThru = new QCheckBox(tr("Program Change"), m_thruBox);
    m_keyThru = new QCheckBox(tr("Key Pressure"), m_thruBox);
    m_chanThru = new QCheckBox(tr("Channel Pressure"), m_thruBox);
    m_pitchThru = new QCheckBox(tr("Pitch Bend"), m_thruBox);
    m_contThru = new QCheckBox(tr("Controller"), m_thruBox);
    m_sysThru = new QCheckBox(tr("System Exclusive"), m_thruBox);

    thruBoxLayout->addWidget(m_noteThru);
    thruBoxLayout->addWidget(m_progThru);
    thruBoxLayout->addWidget(m_keyThru);
    thruBoxLayout->addWidget(m_chanThru);
    thruBoxLayout->addWidget(m_pitchThru);
    thruBoxLayout->addWidget(m_contThru);
    thruBoxLayout->addWidget(m_sysThru);
    m_thruBox->setLayout(thruBoxLayout);

    MidiFilter thruFilter = m_doc->getStudio().getMIDIThruFilter();

    if (thruFilter & MappedEvent::MidiNote)
        m_noteThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiProgramChange)
        m_progThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiKeyPressure)
        m_keyThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiChannelPressure)
        m_chanThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiPitchBend)
        m_pitchThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiController)
        m_contThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiSystemMessage)
        m_sysThru->setChecked(true);

    // RECORD

    m_recordBox = new QGroupBox(tr("RECORD events to ignore"), hBox );
    // VBox Layout for the check boxes in the RECORD group box.
    QVBoxLayout *recordBoxLayout = new QVBoxLayout;
    hBoxLayout->addWidget(m_recordBox);

    // ??? Rearrange.

    m_noteRecord = new QCheckBox(tr("Note"), m_recordBox);
    m_progRecord = new QCheckBox(tr("Program Change"), m_recordBox);
    m_keyRecord = new QCheckBox(tr("Key Pressure"), m_recordBox);
    m_chanRecord = new QCheckBox(tr("Channel Pressure"), m_recordBox);
    m_pitchRecord = new QCheckBox(tr("Pitch Bend"), m_recordBox);
    m_contRecord = new QCheckBox(tr("Controller"), m_recordBox);
    m_sysRecord = new QCheckBox(tr("System Exclusive"), m_recordBox);

    recordBoxLayout->addWidget(m_noteRecord);
    recordBoxLayout->addWidget(m_progRecord);
    recordBoxLayout->addWidget(m_keyRecord);
    recordBoxLayout->addWidget(m_chanRecord);
    recordBoxLayout->addWidget(m_pitchRecord);
    recordBoxLayout->addWidget(m_contRecord);
    recordBoxLayout->addWidget(m_sysRecord);
    m_recordBox->setLayout(recordBoxLayout);

    MidiFilter recordFilter =
        m_doc->getStudio().getMIDIRecordFilter();

    if (recordFilter & MappedEvent::MidiNote)
        m_noteRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiProgramChange)
        m_progRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiKeyPressure)
        m_keyRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiChannelPressure)
        m_chanRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiPitchBend)
        m_pitchRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiController)
        m_contRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiSystemMessage)
        m_sysRecord->setChecked(true);

    hBox->setLayout(hBoxLayout);

    // Button Box

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok    |
                                       QDialogButtonBox::Apply |
                                       QDialogButtonBox::Close |
                                       QDialogButtonBox::Help);
    metagrid->addWidget(m_buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &MidiFilterDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::helpRequested,
            this, &MidiFilterDialog::help);

    m_applyButton = m_buttonBox->button(QDialogButtonBox::Apply);
    connect(m_applyButton, &QAbstractButton::clicked,
            this, &MidiFilterDialog::slotApply);
    // No changes yet, so disable.
    m_applyButton->setEnabled(false);


    connect(m_noteThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_progThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_keyThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_chanThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_pitchThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_contThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_sysThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);

    connect(m_noteRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_progRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_keyRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_chanRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_pitchRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_contRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    connect(m_sysRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);

}

void
MidiFilterDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:midi-filter-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
MidiFilterDialog::slotApply()
{
    RG_DEBUG << "MidiFilterDialog::slotApply()";

    // THRU
    MidiFilter thruFilter{0};
    if (m_noteThru->isChecked())
        thruFilter |= MappedEvent::MidiNote;
    if (m_progThru->isChecked())
        thruFilter |= MappedEvent::MidiProgramChange;
    if (m_keyThru->isChecked())
        thruFilter |= MappedEvent::MidiKeyPressure;
    if (m_chanThru->isChecked())
        thruFilter |= MappedEvent::MidiChannelPressure;
    if (m_pitchThru->isChecked())
        thruFilter |= MappedEvent::MidiPitchBend;
    if (m_contThru->isChecked())
        thruFilter |= MappedEvent::MidiController;
    if (m_sysThru->isChecked())
        thruFilter |= MappedEvent::MidiSystemMessage;

    // RECORD
    MidiFilter recordFilter{0};
    if (m_noteRecord->isChecked())
        recordFilter |= MappedEvent::MidiNote;
    if (m_progRecord->isChecked())
        recordFilter |= MappedEvent::MidiProgramChange;
    if (m_keyRecord->isChecked())
        recordFilter |= MappedEvent::MidiKeyPressure;
    if (m_chanRecord->isChecked())
        recordFilter |= MappedEvent::MidiChannelPressure;
    if (m_pitchRecord->isChecked())
        recordFilter |= MappedEvent::MidiPitchBend;
    if (m_contRecord->isChecked())
        recordFilter |= MappedEvent::MidiController;
    if (m_sysRecord->isChecked())
        recordFilter |= MappedEvent::MidiSystemMessage;

    // Send to Studio
    m_doc->getStudio().setMIDIThruFilter(thruFilter);
    m_doc->getStudio().setMIDIRecordFilter(recordFilter);

    // Send to Sequencer
    if (m_doc->getSequenceManager())
        m_doc->getSequenceManager()->filtersChanged(thruFilter, recordFilter);

    // Disable the Apply button.
    setModified(false);
}

void
MidiFilterDialog::accept()
{
    slotApply();
    QDialog::accept();
}

void
MidiFilterDialog::slotClicked(bool)
{
    setModified(true);
}

void
MidiFilterDialog::setModified(bool modified)
{
    // No change?  Bail.
    if (m_modified == modified)
        return;

    m_modified = modified;

    if (m_applyButton)
        m_applyButton->setEnabled(m_modified);

}


}
