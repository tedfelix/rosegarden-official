/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EventTypeDialog]"
#define RG_NO_DEBUG_PRINT

#include "EventTypeDialog.h"

#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/ConfigGroups.h"
#include "misc/PreferenceInt.h"
#include "misc/Strings.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>


namespace Rosegarden
{


// Remember the last event type selected for next time.
static PreferenceInt eventType(GeneralOptionsConfigGroup, "eventtype", 0);


EventTypeDialog::EventTypeDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle("Event Type");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Select Event type
    mainLayout->addWidget(new QLabel(tr("Select Event type to insert.")));

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(strtoqstr(Note::EventType));
    m_typeCombo->addItem(strtoqstr(Controller::EventType));
    m_typeCombo->addItem(strtoqstr(KeyPressure::EventType));
    m_typeCombo->addItem(strtoqstr(ChannelPressure::EventType));
    m_typeCombo->addItem(strtoqstr(ProgramChange::EventType));
    m_typeCombo->addItem(strtoqstr(SystemExclusive::EventType));
    m_typeCombo->addItem(strtoqstr(PitchBend::EventType));
    m_typeCombo->addItem(strtoqstr(NRPN::EventType));
    m_typeCombo->addItem(strtoqstr(RPN::EventType));
    m_typeCombo->addItem(strtoqstr(Indication::EventType));
    m_typeCombo->addItem(strtoqstr(Text::EventType));
    m_typeCombo->addItem(strtoqstr(Note::EventRestType));
    m_typeCombo->addItem(strtoqstr(Clef::EventType));
    m_typeCombo->addItem(strtoqstr(Key::EventType));
    m_typeCombo->addItem(strtoqstr(Guitar::Chord::EventType));
    m_typeCombo->setCurrentIndex(eventType.get());
    mainLayout->addWidget(m_typeCombo);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EventTypeDialog::slotAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EventTypeDialog::slotAccept()
{
    eventType.set(m_typeCombo->currentIndex());

    QDialog::accept();
}

std::string
EventTypeDialog::getType() const
{
    return qstrtostr(m_typeCombo->currentText());
}


}
