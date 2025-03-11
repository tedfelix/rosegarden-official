/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ProgramChangeWidget]"
#define RG_NO_DEBUG_PRINT

#include "ProgramChangeWidget.h"

#include "base/Event.h"
#include "base/MidiProgram.h"  // For MidiMinValue...
#include "base/MidiTypes.h"  // For ProgramChange::PROGRAM...

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


ProgramChangeWidget::ProgramChangeWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent),
    m_parent(parent)
{
    if (event.getType() != ProgramChange::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);

    // Program Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("Program Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Number
    QLabel *programLabel = new QLabel(tr("Program:"), propertiesGroup);
    propertiesLayout->addWidget(programLabel, row, 0);

    m_programSpinBox = new QSpinBox(propertiesGroup);
    m_programSpinBox->setMinimum(MidiMinValue + 1);
    m_programSpinBox->setMaximum(MidiMaxValue + 1);
    int number{0};
    if (event.has(ProgramChange::PROGRAM))
        number = event.get<Int>(ProgramChange::PROGRAM);
    m_programSpinBox->setValue(number + 1);
    propertiesLayout->addWidget(m_programSpinBox, row, 1);
}

EventWidget::PropertyNameSet
ProgramChangeWidget::getPropertyFilter() const
{
    return PropertyNameSet{ProgramChange::PROGRAM};
}

void ProgramChangeWidget::updateEvent(Event &event) const
{
    event.set<Int>(ProgramChange::PROGRAM, m_programSpinBox->value() - 1);
}


}
