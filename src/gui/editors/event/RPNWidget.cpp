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

#define RG_MODULE_STRING "[RPNWidget]"
#define RG_NO_DEBUG_PRINT

#include "RPNWidget.h"

#include "base/Event.h"
#include "base/MidiTypes.h"  // For RPN::NUMBER...

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


RPNWidget::RPNWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != RPN::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // RPN Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("RPN Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // RPN
    QLabel *rpnLabel = new QLabel(tr("RPN:"), propertiesGroup);
    propertiesLayout->addWidget(rpnLabel, row, 0);

    m_rpnSpinBox = new QSpinBox(propertiesGroup);
    m_rpnSpinBox->setMinimum(0);
    m_rpnSpinBox->setMaximum(16383);
    int rpn{0};
    if (event.has(RPN::NUMBER))
        rpn = event.get<Int>(RPN::NUMBER);
    m_rpnSpinBox->setValue(rpn);
    propertiesLayout->addWidget(m_rpnSpinBox, row, 1);

    // There are only 6 of these.  A ComboBox makes a lot more sense.
    // We could also provide more helpful support for the values.  But
    // for now let's keep it simple.
    //   0 – Pitch bend range
    //   1 – Fine tuning
    //   2 – Coarse tuning
    //   3 – Tuning program change
    //   4 – Tuning bank select
    //   5 – Modulation depth range

    ++row;

    // Value
    QLabel *valueLabel = new QLabel(tr("Value:"), propertiesGroup);
    propertiesLayout->addWidget(valueLabel, row, 0);

    m_valueSpinBox = new QSpinBox(propertiesGroup);
    m_valueSpinBox->setMinimum(0);
    m_valueSpinBox->setMaximum(16383);
    int value{0};
    if (event.has(RPN::VALUE))
        value = event.get<Int>(RPN::VALUE);
    m_valueSpinBox->setValue(value);
    propertiesLayout->addWidget(m_valueSpinBox, row, 1);

    ++row;

}

EventWidget::PropertyNameSet
RPNWidget::getPropertyFilter() const
{
    return PropertyNameSet{RPN::NUMBER, RPN::VALUE};
}

void RPNWidget::updateEvent(Event &event) const
{
    event.set<Int>(RPN::NUMBER, m_rpnSpinBox->value());
    event.set<Int>(RPN::VALUE, m_valueSpinBox->value());
}


}
