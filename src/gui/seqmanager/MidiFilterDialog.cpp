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
    setWindowTitle(tr("Modify MIDI Filters"));
    setModal(true);

    // Grid Layout for the button box at the bottom.
    // ??? metaGrid is only used for the button box at the bottom.
    //     QVBoxLayout would suffice, I think.  Even better, use the
    //     grid layout for the two group boxes and the button box.
    //     That will remove the HBox layout.
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    // HBox Layout for the two group boxes.
    QWidget *hBox = new QWidget(this);
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    metagrid->addWidget(hBox, 0, 0);

    // THRU

    m_thruBox = new QGroupBox(tr("THRU events to ignore"), hBox);
    // VBox Layout for the check boxes in the THRU group box.
    QVBoxLayout *thruBoxLayout = new QVBoxLayout;
    m_thruBox->setLayout(thruBoxLayout);
    hBoxLayout->addWidget(m_thruBox);

    const MidiFilter thruFilter = m_doc->getStudio().getMIDIThruFilter();

    // Note
    m_noteThru = new QCheckBox(tr("Note"), m_thruBox);
    m_noteThru->setChecked((thruFilter & MappedEvent::MidiNote) != 0);
    connect(m_noteThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_noteThru);

    // Program Change
    m_progThru = new QCheckBox(tr("Program Change"), m_thruBox);
    m_progThru->setChecked((thruFilter & MappedEvent::MidiProgramChange) != 0);
    connect(m_progThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_progThru);

    // Key Pressure
    m_keyThru = new QCheckBox(tr("Key Pressure"), m_thruBox);
    m_keyThru->setChecked((thruFilter & MappedEvent::MidiKeyPressure) != 0);
    connect(m_keyThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_keyThru);

    // Channel Pressure
    m_chanThru = new QCheckBox(tr("Channel Pressure"), m_thruBox);
    m_chanThru->setChecked((thruFilter & MappedEvent::MidiChannelPressure) != 0);
    connect(m_chanThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_chanThru);

    // Pitch Bend
    m_pitchThru = new QCheckBox(tr("Pitch Bend"), m_thruBox);
    m_pitchThru->setChecked((thruFilter & MappedEvent::MidiPitchBend) != 0);
    connect(m_pitchThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_pitchThru);

    // Controller
    m_contThru = new QCheckBox(tr("Controller"), m_thruBox);
    m_contThru->setChecked((thruFilter & MappedEvent::MidiController) != 0);
    connect(m_contThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_contThru);

    // System Exclusive
    m_sysThru = new QCheckBox(tr("System Exclusive"), m_thruBox);
    m_sysThru->setChecked((thruFilter & MappedEvent::MidiSystemMessage) != 0);
    connect(m_sysThru, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    thruBoxLayout->addWidget(m_sysThru);

    // RECORD

    m_recordBox = new QGroupBox(tr("RECORD events to ignore"), hBox );
    // VBox Layout for the check boxes in the RECORD group box.
    QVBoxLayout *recordBoxLayout = new QVBoxLayout;
    m_recordBox->setLayout(recordBoxLayout);
    hBoxLayout->addWidget(m_recordBox);

    const MidiFilter recordFilter = m_doc->getStudio().getMIDIRecordFilter();

    // Note
    m_noteRecord = new QCheckBox(tr("Note"), m_recordBox);
    m_noteRecord->setChecked((recordFilter & MappedEvent::MidiNote) != 0);
    connect(m_noteRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_noteRecord);

    // Program Change
    m_progRecord = new QCheckBox(tr("Program Change"), m_recordBox);
    m_progRecord->setChecked((recordFilter & MappedEvent::MidiProgramChange) != 0);
    connect(m_progRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_progRecord);

    // Key Pressure
    m_keyRecord = new QCheckBox(tr("Key Pressure"), m_recordBox);
    m_keyRecord->setChecked((recordFilter & MappedEvent::MidiKeyPressure) != 0);
    connect(m_keyRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_keyRecord);

    // Channel Pressure
    m_chanRecord = new QCheckBox(tr("Channel Pressure"), m_recordBox);
    m_chanRecord->setChecked((recordFilter & MappedEvent::MidiChannelPressure) != 0);
    connect(m_chanRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_chanRecord);

    // Pitch Bend
    m_pitchRecord = new QCheckBox(tr("Pitch Bend"), m_recordBox);
    m_pitchRecord->setChecked((recordFilter & MappedEvent::MidiPitchBend) != 0);
    connect(m_pitchRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_pitchRecord);

    // Controller
    m_contRecord = new QCheckBox(tr("Controller"), m_recordBox);
    m_contRecord->setChecked((recordFilter & MappedEvent::MidiController) != 0);
    connect(m_contRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_contRecord);

    // System Exclusive
    m_sysRecord = new QCheckBox(tr("System Exclusive"), m_recordBox);
    m_sysRecord->setChecked((recordFilter & MappedEvent::MidiSystemMessage) != 0);
    connect(m_sysRecord, &QCheckBox::clicked,
            this, &MidiFilterDialog::slotClicked);
    recordBoxLayout->addWidget(m_sysRecord);

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

    // Apply
    m_applyButton = m_buttonBox->button(QDialogButtonBox::Apply);
    connect(m_applyButton, &QAbstractButton::clicked,
            this, &MidiFilterDialog::slotApply);
    // No changes yet, so disable.
    m_applyButton->setEnabled(false);
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
