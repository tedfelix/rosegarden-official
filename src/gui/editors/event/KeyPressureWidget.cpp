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
#include "base/Pitch.h"
#include "base/MidiTypes.h"  // For KeyPressure::EventType...
#include "gui/dialogs/PitchDialog.h"
#include "misc/PreferenceInt.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


namespace
{

    QString KeyPressureWidgetGroup{"KeyPressureWidget"};
    PreferenceInt a_pitchSetting(KeyPressureWidgetGroup, "Pitch", 60);
    PreferenceInt a_pressureSetting(KeyPressureWidgetGroup, "Pressure", 0);

}


KeyPressureWidget::KeyPressureWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != KeyPressure::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

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

    m_pitchComboBox = new QComboBox(propertiesGroup);
    for (int pitch = 0; pitch < 128; ++pitch) {
        m_pitchComboBox->addItem(QString("%1 (%2)").
                arg(Pitch::toStringOctave(pitch)).
                arg(pitch));
    }
    int pitch{a_pitchSetting.get()};
    if (event.has(KeyPressure::PITCH))
        pitch = event.get<Int>(KeyPressure::PITCH);
    m_pitchComboBox->setCurrentIndex(pitch);
    propertiesLayout->addWidget(m_pitchComboBox, row, 1);

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
    int pressure{a_pressureSetting.get()};
    if (event.has(KeyPressure::PRESSURE))
        pressure = event.get<Int>(KeyPressure::PRESSURE);
    m_pressureSpinBox->setValue(pressure);
    propertiesLayout->addWidget(m_pressureSpinBox, row, 1);

    ++row;

}

KeyPressureWidget::~KeyPressureWidget()
{
    a_pitchSetting.set(m_pitchComboBox->currentIndex());
    a_pressureSetting.set(m_pressureSpinBox->value());
}

EventWidget::PropertyNameSet
KeyPressureWidget::getPropertyFilter() const
{
    return PropertyNameSet{KeyPressure::PITCH, KeyPressure::PRESSURE};
}

void
KeyPressureWidget::slotEditPitch(bool /*checked*/)
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchComboBox->currentIndex());
    if (dialog.exec() == QDialog::Accepted)
        m_pitchComboBox->setCurrentIndex(dialog.getPitch());
}

void
KeyPressureWidget::updateEvent(Event &event) const
{
    event.set<Int>(KeyPressure::PITCH, m_pitchComboBox->currentIndex());
    event.set<Int>(KeyPressure::PRESSURE, m_pressureSpinBox->value());
}


}
