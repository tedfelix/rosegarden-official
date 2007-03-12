/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PluginControl.h"
#include "Rotary.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "gui/general/GUIPalette.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/widgets/Rotary.h"
#include <qfont.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qobject.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

PluginControl::PluginControl(QWidget *parent,
                             QGridLayout *layout,
                             ControlType type,
                             PluginPort *port,
                             AudioPluginManager *aPM,
                             int index,
                             float initialValue,
                             bool showBounds,
                             bool hidden):
        QObject(parent),
        m_layout(layout),
        m_type(type),
        m_port(port),
        m_pluginManager(aPM),
        m_index(index)
{
    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 ) / 10);

    QLabel *controlTitle =
        new QLabel(QString("%1    ").arg(strtoqstr(port->getName())), parent);
    controlTitle->setFont(plainFont);

    if (type == Rotary) {
        float lowerBound = port->getLowerBound();
        float upperBound = port->getUpperBound();
        // Default value was already handled when calling this constructor

        if (lowerBound > upperBound) {
            float swap = upperBound;
            upperBound = lowerBound;
            lowerBound = swap;
        }

        float step = (upperBound - lowerBound) / 100.0;
        float pageStep = step * 10.0;
        Rotary::TickMode ticks = Rotary::PageStepTicks;
        bool snapToTicks = false;

        if (port->getDisplayHint() & PluginPort::Integer) {
            step = 1.0;
            ticks = Rotary::StepTicks;
            if (upperBound - lowerBound > 30.0)
                pageStep = 10.0;
            snapToTicks = true;
        }
        if (port->getDisplayHint() & PluginPort::Toggled) {
            lowerBound = -0.0001;
            upperBound = 1.0001;
            step = 1.0;
            pageStep = 1.0;
            ticks = Rotary::StepTicks;
            snapToTicks = true;
        }

        QLabel *low;
        if (port->getDisplayHint() &
                (PluginPort::Integer |
                 PluginPort::Toggled)) {
            low = new QLabel(QString("%1").arg(int(lowerBound)), parent);
        } else {
            low = new QLabel(QString("%1").arg(lowerBound), parent);
        }
        low->setFont(plainFont);

        m_dial = new ::Rosegarden::Rotary(parent,
                            lowerBound,    // min
                            upperBound,    // max
                            step,          // step
                            pageStep,      // page step
                            initialValue,  // initial
                            30,            // size
                            ticks,
                            snapToTicks);

        m_dial->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPlugin));

        //m_dial->setPosition(defaultValue);
        //emit valueChanged(defaultValue);

        connect(m_dial, SIGNAL(valueChanged(float)),
                this, SLOT(slotValueChanged(float)));

        QLabel *upp;
        if (port->getDisplayHint() &
                (PluginPort::Integer |
                 PluginPort::Toggled)) {
            upp = new QLabel(QString("%1").arg(int(upperBound)), parent);
        } else {
            upp = new QLabel(QString("%1").arg(upperBound), parent);
        }
        upp->setFont(plainFont);

        QWidgetItem *item;

        if (!hidden) {
            controlTitle->show();
            item = new QWidgetItem(controlTitle);
            item->setAlignment(Qt::AlignRight | Qt::AlignBottom);
            m_layout->addItem(item);
        } else {
            controlTitle->hide();
        }

        if (showBounds && !hidden) {
            low->show();
            item = new QWidgetItem(low);
            item->setAlignment(Qt::AlignRight | Qt::AlignBottom);
            m_layout->addItem(item);
        } else {
            low->hide();
        }

        if (!hidden) {
            m_dial->show();
            item = new QWidgetItem(m_dial);
            item->setAlignment(Qt::AlignCenter);
            m_layout->addItem(item);
        } else {
            m_dial->hide();
        }

        if (showBounds && !hidden) {
            upp->show();
            item = new QWidgetItem(upp);
            item->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
            m_layout->addItem(item);
        } else {
            upp->hide();
        }
    }
}

void
PluginControl::setValue(float value, bool emitSignals)
{
    if (!emitSignals)
        m_dial->blockSignals(true);
    m_dial->setPosition(value);
    if (!emitSignals)
        m_dial->blockSignals(false);
    else
        emit valueChanged(value);
}

float
PluginControl::getValue() const
{
    return m_dial == 0 ? 0 : m_dial->getPosition();
}

void
PluginControl::slotValueChanged(float value)
{
    emit valueChanged(value);
}

}
#include "PluginControl.moc"
