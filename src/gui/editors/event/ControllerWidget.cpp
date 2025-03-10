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

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


// ??? Promote this up someplace more common so EventListEditor and
//     others can use it.
// ??? Why not just load these into a combo box?  Then there will be
//     no need for update trickery.
// ??? We should also show these on the EventListEditor.
// ??? We should also consider using the controller list that is in the
//     Device.  That is probably more accurate in some cases and should
//     have higher priority than these names.  Maybe bold-face the names
//     we get from the Device to indicate "official".
// ??? Move this to a more central location.
typedef std::map<int, QString> ControllerNames;
static const ControllerNames controllerNames = {
    { 0, "Bank Select MSB" },
    { 1, "Mod Wheel MSB" },
    { 2, "Breath Controller MSB" },
    { 4, "Foot Pedal MSB" },
    { 5, "Portamento Time MSB" },
    { 6, "Data Entry MSB" },
    { 7, "Volume" },
    { 8, "Stereo Balance MSB" },
    { 10, "Pan" },
    { 11, "Expression" },
    { 12, "Effect 1 MSB" },
    { 13, "Effect 2 MSB" },
    { 32, "Bank Select LSB" },
    { 33, "Mod Wheel LSB" },
    { 34, "Breath Controller LSB" },
    { 36, "Foot Pedal LSB" },
    { 37, "Portamento Time LSB" },
    { 38, "Data Entry LSB" },
    { 39, "Volume LSB" },
    { 40, "Stereo Balance LSB" },
    { 42, "Pan LSB" },
    { 43, "Expression LSB" },
    { 44, "Effect 1 LSB" },
    { 45, "Effect 2 LSB" },
    { 64, "Sustain" },
    { 65, "Portamento On/Off" },
    { 66, "Sostenuto" },
    { 67, "Soft Pedal" },
    { 68, "Legato" },
    { 69, "Hold Pedal 2" },
    { 91, "Reverb" },
    { 92, "Tremolo" },
    { 93, "Chorus" },
    { 94, "Detuning" },
    { 95, "Phaser" },
    { 96, "Data +" },
    { 97, "Data -" },
    { 98, "NRPN LSB" },
    { 99, "NRPN MSB" },
    { 100, "RPN LSB" },
    { 101, "RPN MSB" },
    { 120, "Channel Mute" },
    { 121, "Reset All Controllers" },
    { 122, "Local On/Off" },
    { 123, "All MIDI Notes Off" },
    { 124, "Omni Off" },
    { 125, "Omni On" },
    { 126, "Mono On/Off" },
    { 127, "Poly On/Off" }
};

ControllerWidget::ControllerWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent),
    m_parent(parent)
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
    for (int i = MidiMinValue; i <= MidiMaxValue; ++i) {
        QString name;
        ControllerNames::const_iterator nameIter = controllerNames.find(i);
        if (nameIter != controllerNames.end())
            name = nameIter->second;
        m_numberComboBox->addItem(QString("%1  %2").arg(i).arg(name));
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
