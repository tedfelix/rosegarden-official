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

#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
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
    m_doc(doc),
    m_buttonBox(nullptr)
{
    //setHelp("studio-midi-filters");

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

    QCheckBox *noteThru = new QCheckBox(tr("Note"), m_thruBox);
    QCheckBox *progThru = new QCheckBox(tr("Program Change"), m_thruBox);
    QCheckBox *keyThru = new QCheckBox(tr("Key Pressure"), m_thruBox);
    QCheckBox *chanThru = new QCheckBox(tr("Channel Pressure"), m_thruBox);
    QCheckBox *pitchThru = new QCheckBox(tr("Pitch Bend"), m_thruBox);
    QCheckBox *contThru = new QCheckBox(tr("Controller"), m_thruBox);
    QCheckBox *sysThru = new QCheckBox(tr("System Exclusive"), m_thruBox);

    noteThru->setObjectName("Note");
    progThru->setObjectName("Program Change");
    keyThru->setObjectName("Key Pressure");
    chanThru->setObjectName("Channel Pressure");
    pitchThru->setObjectName("Pitch Bend");
    contThru->setObjectName("Controller");
    sysThru->setObjectName("System Exclusive");

    thruBoxLayout->addWidget(noteThru);
    thruBoxLayout->addWidget(progThru);
    thruBoxLayout->addWidget(keyThru);
    thruBoxLayout->addWidget(chanThru);
    thruBoxLayout->addWidget(pitchThru);
    thruBoxLayout->addWidget(contThru);
    thruBoxLayout->addWidget(sysThru);
    m_thruBox->setLayout(thruBoxLayout);

    MidiFilter thruFilter = m_doc->getStudio().getMIDIThruFilter();

    if (thruFilter & MappedEvent::MidiNote)
        noteThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiProgramChange)
        progThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiKeyPressure)
        keyThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiChannelPressure)
        chanThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiPitchBend)
        pitchThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiController)
        contThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiSystemMessage)
        sysThru->setChecked(true);

    // RECORD

    m_recordBox = new QGroupBox(tr("RECORD events to ignore"), hBox );
    // VBox Layout for the check boxes in the RECORD group box.
    QVBoxLayout *recordBoxLayout = new QVBoxLayout;
    hBoxLayout->addWidget(m_recordBox);

    // ??? Rearrange.

    QCheckBox *noteRecord = new QCheckBox(tr("Note"), m_recordBox);
    QCheckBox *progRecord = new QCheckBox(tr("Program Change"), m_recordBox);
    QCheckBox *keyRecord = new QCheckBox(tr("Key Pressure"), m_recordBox);
    QCheckBox *chanRecord = new QCheckBox(tr("Channel Pressure"), m_recordBox);
    QCheckBox *pitchRecord = new QCheckBox(tr("Pitch Bend"), m_recordBox);
    QCheckBox *contRecord = new QCheckBox(tr("Controller"), m_recordBox);
    QCheckBox *sysRecord = new QCheckBox(tr("System Exclusive"), m_recordBox);

    noteRecord->setObjectName("Note");
    progRecord->setObjectName("Program Change");
    keyRecord->setObjectName("Key Pressure");
    chanRecord->setObjectName("Channel Pressure");
    pitchRecord->setObjectName("Pitch Bend");
    contRecord->setObjectName("Controller");
    sysRecord->setObjectName("System Exclusive");

    recordBoxLayout->addWidget(noteRecord);
    recordBoxLayout->addWidget(progRecord);
    recordBoxLayout->addWidget(keyRecord);
    recordBoxLayout->addWidget(chanRecord);
    recordBoxLayout->addWidget(pitchRecord);
    recordBoxLayout->addWidget(contRecord);
    recordBoxLayout->addWidget(sysRecord);
    m_recordBox->setLayout(recordBoxLayout);

    MidiFilter recordFilter =
        m_doc->getStudio().getMIDIRecordFilter();

    if (recordFilter & MappedEvent::MidiNote)
        noteRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiProgramChange)
        progRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiKeyPressure)
        keyRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiChannelPressure)
        chanRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiPitchBend)
        pitchRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiController)
        contRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiSystemMessage)
        sysRecord->setChecked(true);

    hBox->setLayout(hBoxLayout);

    // Button Box

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok    |
                                       QDialogButtonBox::Apply |
                                       QDialogButtonBox::Close |
                                       QDialogButtonBox::Help);
    metagrid->addWidget(m_buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::helpRequested, this, &MidiFilterDialog::help);

    m_applyButton = m_buttonBox->button(QDialogButtonBox::Apply);
    connect(m_applyButton, &QAbstractButton::clicked, this, &MidiFilterDialog::slotApply);


    connect(noteThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(progThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(keyThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(chanThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(pitchThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(contThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(sysThru, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);

    connect(noteRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(progRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(keyRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(chanRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(pitchRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(contRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);
    connect(sysRecord, &QCheckBox::stateChanged,
            this, &MidiFilterDialog::slotSetModified);

    // setting the thing up initially changes states and trips signals, so we
    // have to do this to wipe the slate clean initially after all the false
    // positives
    // ??? Or we could hook up to clicked() which does not trigger on
    //     programmatic changes.
    setModified(false);
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

    MidiFilter thruFilter{0};
    MidiFilter recordFilter{0};

    // ??? Instead of all this findChild(), make them members.
    //     That's a lot faster and less code.  No need to set
    //     object name.

    if (m_thruBox->findChild<QCheckBox*>("Note")->isChecked())
        thruFilter |= MappedEvent::MidiNote;

    if (m_thruBox->findChild<QCheckBox*>("Program Change")->isChecked())
        thruFilter |= MappedEvent::MidiProgramChange;

    if (m_thruBox->findChild<QCheckBox*>("Key Pressure")->isChecked())
        thruFilter |= MappedEvent::MidiKeyPressure;

    if (m_thruBox->findChild<QCheckBox*>("Channel Pressure")->isChecked())
        thruFilter |= MappedEvent::MidiChannelPressure;

    if (m_thruBox->findChild<QCheckBox*>("Pitch Bend")->isChecked())
        thruFilter |= MappedEvent::MidiPitchBend;

    if (m_thruBox->findChild<QCheckBox*>("Controller")->isChecked())
        thruFilter |= MappedEvent::MidiController;

    if (m_thruBox->findChild<QCheckBox*>("System Exclusive")->isChecked())
        thruFilter |= MappedEvent::MidiSystemMessage;

    if (m_recordBox->findChild<QCheckBox*>("Note")->isChecked())
        recordFilter |= MappedEvent::MidiNote;

    if (m_recordBox->findChild<QCheckBox*>("Program Change")->isChecked())
        recordFilter |= MappedEvent::MidiProgramChange;

    if (m_recordBox->findChild<QCheckBox*>("Key Pressure")->isChecked())
        recordFilter |= MappedEvent::MidiKeyPressure;

    if (m_recordBox->findChild<QCheckBox*>("Channel Pressure")->isChecked())
        recordFilter |= MappedEvent::MidiChannelPressure;

    if (m_recordBox->findChild<QCheckBox*>("Pitch Bend")->isChecked())
        recordFilter |= MappedEvent::MidiPitchBend;

    if (m_recordBox->findChild<QCheckBox*>("Controller")->isChecked())
        recordFilter |= MappedEvent::MidiController;

    if (m_recordBox->findChild<QCheckBox*>("System Exclusive")->isChecked())
        recordFilter |= MappedEvent::MidiSystemMessage;

    // Send to Studio
    m_doc->getStudio().setMIDIThruFilter(thruFilter);
    m_doc->getStudio().setMIDIRecordFilter(recordFilter);

    // Send to Sequencer
    if (m_doc->getSequenceManager())
        m_doc->getSequenceManager()->filtersChanged(thruFilter, recordFilter);

    setModified(false);
}

void
MidiFilterDialog::accept()
{
    slotApply();
    QDialog::accept();
}

void
MidiFilterDialog::slotSetModified(int)
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
