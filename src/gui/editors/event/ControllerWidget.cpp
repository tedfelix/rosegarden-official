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

#define RG_MODULE_STRING "[ControllerWidget]"
#define RG_NO_DEBUG_PRINT

#include "ControllerWidget.h"

#include "base/Event.h"
#include "base/MidiProgram.h"  // For MidiMinValue, etc...
#include "base/MidiTypes.h"  // For Controller::NUMBER...

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


ControllerWidget::ControllerWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != Controller::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);

    // Controller Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("Controller Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Number
    QLabel *numberLabel = new QLabel(tr("Number:"), propertiesGroup);
    propertiesLayout->addWidget(numberLabel, row, 0);

    m_numberComboBox = new QComboBox(propertiesGroup);
    for (int number = MidiMinValue; number <= MidiMaxValue; ++number) {
        m_numberComboBox->addItem(
                QString("%1  %2").arg(number).arg(Controller::getName(number)));
    }
    int controllerNumber{0};
    if (event.has(Controller::NUMBER))
        controllerNumber = event.get<Int>(Controller::NUMBER);
    m_numberComboBox->setCurrentIndex(controllerNumber);
    propertiesLayout->addWidget(m_numberComboBox, row, 1);

    ++row;

    // Value
    QLabel *valueLabel = new QLabel(tr("Value:"), propertiesGroup);
    propertiesLayout->addWidget(valueLabel, row, 0);

    m_valueSpinBox = new QSpinBox(propertiesGroup);
    m_valueSpinBox->setMinimum(MidiMinValue);
    m_valueSpinBox->setMaximum(MidiMaxValue);
    int value{0};
    if (event.has(Controller::VALUE))
        value = event.get<Int>(Controller::VALUE);
    m_valueSpinBox->setValue(value);
    propertiesLayout->addWidget(m_valueSpinBox, row, 1);

    ++row;

}

EventWidget::PropertyNameSet
ControllerWidget::getPropertyFilter() const
{
    return PropertyNameSet{Controller::NUMBER, Controller::VALUE};
}

void ControllerWidget::updateEvent(Event &event) const
{
    event.set<Int>(Controller::NUMBER, m_numberComboBox->currentIndex());
    event.set<Int>(Controller::VALUE, m_valueSpinBox->value());
}


}
