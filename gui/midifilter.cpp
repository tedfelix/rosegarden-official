/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h>

#include <qbuttongroup.h>
#include <qhbox.h>
#include <qcheckbox.h>

#include "rosegardenguidoc.h"
#include "midifilter.h"
#include "MappedEvent.h"


MidiFilterDialog::MidiFilterDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc):
    KDialogBase(parent, 0, true, i18n("Modify MIDI filters..."),
                Ok | Apply | Close),
    m_doc(doc),
    m_modified(true)
{
    QHBox *hBox = makeHBoxMainWidget();

    m_thruBox =
        new QButtonGroup(1,
                         Qt::Horizontal,
                         i18n("THRU events to ignore"), hBox);

    QCheckBox *noteThru = new QCheckBox(i18n("Note"), m_thruBox);
    QCheckBox *progThru = new QCheckBox(i18n("Program Change"), m_thruBox);
    QCheckBox *keyThru = new QCheckBox(i18n("Key Pressure"), m_thruBox);
    QCheckBox *chanThru = new QCheckBox(i18n("Channel Pressure"), m_thruBox);
    QCheckBox *pitchThru = new QCheckBox(i18n("Pitch Bend"), m_thruBox);
    QCheckBox *contThru = new QCheckBox(i18n("Controller"), m_thruBox);
    QCheckBox *sysThru = new QCheckBox(i18n("System Exclusive"), m_thruBox);

    Rosegarden::MidiFilter thruFilter = m_doc->getStudio().getMIDIThruFilter();

    if (thruFilter & Rosegarden::MappedEvent::MidiNote)
        noteThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiProgramChange)
        progThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiKeyPressure)
        keyThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiChannelPressure)
        chanThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiPitchBend)
        pitchThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiController)
        contThru->setChecked(true);

    if (thruFilter & Rosegarden::MappedEvent::MidiSystemExclusive)
        sysThru->setChecked(true);

    m_recordBox =
        new QButtonGroup(1,
                         Qt::Horizontal,
                         i18n("RECORD events to ignore"), hBox);

    QCheckBox *noteRecord = new QCheckBox(i18n("Note"), m_recordBox);
    QCheckBox *progRecord = new QCheckBox(i18n("Program Change"), m_recordBox);
    QCheckBox *keyRecord = new QCheckBox(i18n("Key Pressure"), m_recordBox);
    QCheckBox *chanRecord = new QCheckBox(i18n("Channel Pressure"), m_recordBox);
    QCheckBox *pitchRecord = new QCheckBox(i18n("Pitch Bend"), m_recordBox);
    QCheckBox *contRecord = new QCheckBox(i18n("Controller"), m_recordBox);
    QCheckBox *sysRecord = new QCheckBox(i18n("System Exclusive"), m_recordBox);

    Rosegarden::MidiFilter recordFilter =
        m_doc->getStudio().getMIDIRecordFilter();

    if (recordFilter & Rosegarden::MappedEvent::MidiNote)
        noteRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiProgramChange)
        progRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiKeyPressure)
        keyRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiChannelPressure)
        chanRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiPitchBend)
        pitchRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiController)
        contRecord->setChecked(true);

    if (recordFilter & Rosegarden::MappedEvent::MidiSystemExclusive)
        sysRecord->setChecked(true);


    connect(m_thruBox, SIGNAL(released(int)),
            this, SLOT(slotSetModified()));

    connect(m_recordBox, SIGNAL(released(int)),
            this, SLOT(slotSetModified()));

    setModified(false);
}


void
MidiFilterDialog::slotApply()
{
    Rosegarden::MidiFilter thruFilter = 0,
                           recordFilter = 0;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(0))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiNote;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(1))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiProgramChange;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(2))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiKeyPressure;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(3))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiChannelPressure;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(4))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiPitchBend;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(5))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiController;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(6))->isChecked())
        thruFilter |= Rosegarden::MappedEvent::MidiSystemExclusive;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(0))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiNote;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(1))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiProgramChange;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(2))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiKeyPressure;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(3))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiChannelPressure;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(4))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiPitchBend;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(5))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiController;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(6))->isChecked())
        recordFilter |= Rosegarden::MappedEvent::MidiSystemExclusive;


    //if (m_thruBox->


    m_doc->getStudio().setMIDIThruFilter(thruFilter);
    m_doc->getStudio().setMIDIRecordFilter(recordFilter);

    setModified(false);
}

void
MidiFilterDialog::slotOk()
{
    slotApply();
    accept();
}


void
MidiFilterDialog::slotSetModified()
{
    setModified(true);
}

void
MidiFilterDialog::setModified(bool value)
{
    if (m_modified == value) return;

    if (value)
    {
        enableButtonApply(true);
    }
    else
    {
        enableButtonApply(false);
    }

    m_modified = value;

}
