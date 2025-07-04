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

#define RG_MODULE_STRING "[ChannelPressureWidget]"
#define RG_NO_DEBUG_PRINT

#include "ChannelPressureWidget.h"

#include "base/Event.h"
#include "base/MidiProgram.h"  // For MidiMinValue, etc...
#include "base/MidiTypes.h"  // For ChannelPressure::EventType...
#include "misc/PreferenceInt.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


namespace
{

    QString ChannelPressureWidgetGroup{"ChannelPressureWidget"};
    PreferenceInt a_pressureSetting(ChannelPressureWidgetGroup, "Pressure", 0);

}


ChannelPressureWidget::ChannelPressureWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != ChannelPressure::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // Channel Pressure Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("Channel Pressure Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Pressure
    QLabel *pressureLabel = new QLabel(tr("Pressure:"), propertiesGroup);
    propertiesLayout->addWidget(pressureLabel, row, 0);

    m_pressureSpinBox = new QSpinBox(propertiesGroup);
    m_pressureSpinBox->setMinimum(MidiMinValue);
    m_pressureSpinBox->setMaximum(MidiMaxValue);
    int pressure{a_pressureSetting.get()};
    if (event.has(ChannelPressure::PRESSURE))
        pressure = event.get<Int>(ChannelPressure::PRESSURE);
    m_pressureSpinBox->setValue(pressure);
    propertiesLayout->addWidget(m_pressureSpinBox, row, 1);

    ++row;

}

ChannelPressureWidget::~ChannelPressureWidget()
{
    a_pressureSetting.set(m_pressureSpinBox->value());
}

EventWidget::PropertyNameSet
ChannelPressureWidget::getPropertyFilter() const
{
    return PropertyNameSet{ChannelPressure::PRESSURE};
}

void ChannelPressureWidget::updateEvent(Event &event) const
{
    event.set<Int>(ChannelPressure::PRESSURE, m_pressureSpinBox->value());
}


}
