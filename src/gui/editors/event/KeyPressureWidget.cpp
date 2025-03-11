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

#define RG_MODULE_STRING "[KeyPressureWidget]"
#define RG_NO_DEBUG_PRINT

#include "KeyPressureWidget.h"

#include "base/Event.h"
#include "base/MidiProgram.h"  // For MidiMinValue, etc...
#include "base/MidiTypes.h"  // For KeyPressure::EventType...
#include "gui/dialogs/PitchDialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


KeyPressureWidget::KeyPressureWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent),
    m_parent(parent)
{
    if (event.getType() != KeyPressure::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);

    // Key Pressure Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("Key Pressure Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Pitch
    QLabel *pitchLabel = new QLabel(tr("Pitch:"), propertiesGroup);
    propertiesLayout->addWidget(pitchLabel, row, 0);

    m_pitchSpinBox = new QSpinBox(propertiesGroup);
    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);
    int pitch{0};
    if (event.has(KeyPressure::PITCH))
        pitch = event.get<Int>(KeyPressure::PITCH);
    m_pitchSpinBox->setValue(pitch);
    propertiesLayout->addWidget(m_pitchSpinBox, row, 1);

    QPushButton *pitchEditButton = new QPushButton(tr("edit"), propertiesGroup);
    propertiesLayout->addWidget(pitchEditButton, row, 2);
    connect(pitchEditButton, &QPushButton::clicked,
            this, &KeyPressureWidget::slotEditPitch);

    ++row;

    // Pressure
    QLabel *pressureLabel = new QLabel(tr("Pressure:"), propertiesGroup);
    propertiesLayout->addWidget(pressureLabel, row, 0);

    m_pressureSpinBox = new QSpinBox(propertiesGroup);
    m_pressureSpinBox->setMinimum(MidiMinValue);
    m_pressureSpinBox->setMaximum(MidiMaxValue);
    int pressure{0};
    if (event.has(KeyPressure::PRESSURE))
        pressure = event.get<Int>(KeyPressure::PRESSURE);
    m_pressureSpinBox->setValue(pressure);
    propertiesLayout->addWidget(m_pressureSpinBox, row, 1);

    ++row;

}

EventWidget::PropertyNameSet
KeyPressureWidget::getPropertyFilter() const
{
    return PropertyNameSet{KeyPressure::PITCH, KeyPressure::PRESSURE};
}

void
KeyPressureWidget::slotEditPitch(bool /*checked*/)
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchSpinBox->value());
    if (dialog.exec() == QDialog::Accepted)
        m_pitchSpinBox->setValue(dialog.getPitch());
}

void
KeyPressureWidget::updateEvent(Event &event) const
{
    event.set<Int>(KeyPressure::PITCH, m_pitchSpinBox->value());
    event.set<Int>(KeyPressure::PRESSURE, m_pressureSpinBox->value());
}


}
