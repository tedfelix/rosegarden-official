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

#define RG_MODULE_STRING "[PitchBendWidget]"
#define RG_NO_DEBUG_PRINT

#include "PitchBendWidget.h"

#include "base/Event.h"
#include "base/MidiTypes.h"  // For PitchBend::EventType...
#include "misc/PreferenceInt.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


namespace
{

    QString PitchBendWidgetGroup{"PitchBendWidget"};
    PreferenceInt a_valueSetting(PitchBendWidgetGroup, "Value", 8192);

}


PitchBendWidget::PitchBendWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != PitchBend::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // Pitch Bend Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("Pitch Bend Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Number
    QLabel *pitchBendLabel = new QLabel(tr("Pitch Bend:"), propertiesGroup);
    QString tipText = tr("8192 is center");
    pitchBendLabel->setToolTip(tipText);
    propertiesLayout->addWidget(pitchBendLabel, row, 0);

    m_pitchBendSpinBox = new QSpinBox(propertiesGroup);
    m_pitchBendSpinBox->setToolTip(tipText);
    m_pitchBendSpinBox->setMinimum(0);
    // 8192 is centered.
    m_pitchBendSpinBox->setMaximum(16383);
    int value{a_valueSetting.get()};
    int msb{value >> 7};
    if (event.has(PitchBend::MSB))
        msb = event.get<Int>(PitchBend::MSB);
    int lsb{value & 0x7F};
    if (event.has(PitchBend::LSB))
        lsb = event.get<Int>(PitchBend::LSB);
    m_pitchBendSpinBox->setValue((msb << 7) + lsb);
    propertiesLayout->addWidget(m_pitchBendSpinBox, row, 1);
}

PitchBendWidget::~PitchBendWidget()
{
    a_valueSetting.set(m_pitchBendSpinBox->value());
}

EventWidget::PropertyNameSet
PitchBendWidget::getPropertyFilter() const
{
    return PropertyNameSet{PitchBend::MSB, PitchBend::LSB};
}

void PitchBendWidget::updateEvent(Event &event) const
{
    const int value = m_pitchBendSpinBox->value();
    event.set<Int>(PitchBend::MSB, value >> 7);
    event.set<Int>(PitchBend::LSB, value & 127);
}


}
