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

#define RG_MODULE_STRING "[NRPNWidget]"
#define RG_NO_DEBUG_PRINT

#include "NRPNWidget.h"

#include "base/Event.h"
#include "base/MidiTypes.h"  // For NRPN::NUMBER...

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>


namespace Rosegarden
{


NRPNWidget::NRPNWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != NRPN::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // NRPN Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(
            tr("NRPN Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // NRPN
    QLabel *nrpnLabel = new QLabel(tr("NRPN:"), propertiesGroup);
    propertiesLayout->addWidget(nrpnLabel, row, 0);

    m_nrpnSpinBox = new QSpinBox(propertiesGroup);
    m_nrpnSpinBox->setMinimum(0);
    m_nrpnSpinBox->setMaximum(16383);
    int nrpn{0};
    if (event.has(NRPN::NUMBER))
        nrpn = event.get<Int>(NRPN::NUMBER);
    m_nrpnSpinBox->setValue(nrpn);
    propertiesLayout->addWidget(m_nrpnSpinBox, row, 1);

    ++row;

    // Value
    QLabel *valueLabel = new QLabel(tr("Value:"), propertiesGroup);
    propertiesLayout->addWidget(valueLabel, row, 0);

    m_valueSpinBox = new QSpinBox(propertiesGroup);
    m_valueSpinBox->setMinimum(0);
    m_valueSpinBox->setMaximum(16383);
    int value{0};
    if (event.has(NRPN::VALUE))
        value = event.get<Int>(NRPN::VALUE);
    m_valueSpinBox->setValue(value);
    propertiesLayout->addWidget(m_valueSpinBox, row, 1);

    ++row;

}

EventWidget::PropertyNameSet
NRPNWidget::getPropertyFilter() const
{
    return PropertyNameSet{NRPN::NUMBER, NRPN::VALUE};
}

void NRPNWidget::updateEvent(Event &event) const
{
    event.set<Int>(NRPN::NUMBER, m_nrpnSpinBox->value());
    event.set<Int>(NRPN::VALUE, m_valueSpinBox->value());
}


}
