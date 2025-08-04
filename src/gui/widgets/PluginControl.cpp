/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PluginControl]"
#define RG_NO_DEBUG_PRINT

#include "PluginControl.h"

#include "Rotary.h"

#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "gui/general/GUIPalette.h"
#include "gui/studio/AudioPluginManager.h"

#include <QFont>
#include <QLabel>
#include <QGridLayout>
#include <QObject>
#include <QString>
#include <QWidget>

#include <math.h>


namespace Rosegarden
{


PluginControl::PluginControl(QWidget *parent,
                             ControlType type,
                             QSharedPointer<PluginPort> port,
                             QSharedPointer<AudioPluginManager> pluginManager,
                             int index,
                             float initialValue,
                             bool showBounds):
    QWidget(parent),
    m_type(type),
    m_port(port),
    m_pluginManager(pluginManager),
    m_index(index)
{
    setObjectName("PluginControl");

    // ??? Use QHBoxLayout.
    QGridLayout *hbox = new QGridLayout(this);
    hbox->setContentsMargins(0, 0, 0, 0);

    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 ) / 10);

    // Name
    QLabel *controlTitle = new QLabel(
            QString("%1    ").arg(strtoqstr(port->getName())),
            this);
    controlTitle->setFont(plainFont);
    controlTitle->setMinimumWidth
        (QFontMetrics(controlTitle->font()).boundingRect("Bandwidth 1").width());

    // We don't do anything other than Rotary widgets.
    // ??? Probably should remove type altogether.
    if (type != ControlType::Rotary)
        return;

    float minimum = port->getLowerBound();
    float maximum = port->getUpperBound();
    // Default value was already handled when calling this constructor

    if (minimum > maximum) {
        float swap = maximum;
        maximum = minimum;
        minimum = swap;
    }

    // NoHint (float assumed)
    float step = (maximum - minimum) / 100.0;
    // Assume 11 ticks and no snap.
    float pageStep = step * 10.f;
    Rotary::TickMode ticks = Rotary::TicksNoSnap;

    // Integer
    if (port->getDisplayHint() & PluginPort::Integer) {
        step = 1.0;
        ticks = Rotary::StepTicks;
        if (maximum - minimum > 30.0)
            pageStep = 10.0;
    }

    // Toggled
    if (port->getDisplayHint() & PluginPort::Toggled) {
        minimum = 0.0;
        maximum = 1.0;
        step = maximum - minimum;
        pageStep = maximum - minimum;
        ticks = Rotary::StepTicks;
    }

    const bool logarithmic =
            (port->getDisplayHint() & PluginPort::Logarithmic);

    // Logarithmic
    if (logarithmic) {
        // Redundant, but keeping here for clarity.  Hoping eventually
        // to have a TickMode Rotary::Log.
        ticks = Rotary::TicksNoSnap;
    }

    // Minimum
    QLabel *minLabel;
    if (port->getDisplayHint() &
        (PluginPort::Integer | PluginPort::Toggled)) {
        minLabel = new QLabel(QString("%1").arg(int(minimum)), this);
    } else {
        minLabel = new QLabel(QString("%1").arg(minimum), this);
    }
    minLabel->setFont(plainFont);
    minLabel->setMinimumWidth(QFontMetrics(plainFont).boundingRect("0.001").width());
    minLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    //RG_DEBUG << "port " << port->getName() <<
    //            ": maximum " << minimum <<
    //            ", minimum " << maximum <<
    //            ", logarithmic " << logarithmic <<
    //            ", default " << initialValue <<
    //            ", step " << step;

    // Rotary
    m_dial = new Rotary(this,
                        minimum,
                        maximum,
                        step,
                        pageStep,
                        initialValue,  // initialPosition
                        30,            // size
                        ticks,
                        false,         // centred
                        logarithmic);
    m_dial->setLabel(strtoqstr(port->getName()));
    m_dial->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPlugin));

    connect(m_dial, &Rotary::valueChanged,
            this, &PluginControl::slotValueChanged);

    // Maximum
    QLabel *maxLabel;
    if (port->getDisplayHint() &
        (PluginPort::Integer | PluginPort::Toggled)) {
        maxLabel = new QLabel(QString("%1").arg(int(maximum)), this);
    } else {
        maxLabel = new QLabel(QString("%1").arg(maximum), this);
    }
    maxLabel->setFont(plainFont);
    maxLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    maxLabel->setMinimumWidth(QFontMetrics(plainFont).boundingRect("99999").width());

    int col = 0;
    hbox->setColumnStretch(col++, 10);
    hbox->addWidget(controlTitle, 0, col++);

    if (showBounds) {
        minLabel->show();
        hbox->addWidget(minLabel, 0, col++);
    } else {
        minLabel->hide();
    }

    hbox->addWidget(m_dial, 0, col++);

    if (showBounds) {
        maxLabel->show();
        hbox->addWidget(maxLabel, 0, col++);
    } else {
        maxLabel->hide();
    }

    hbox->setColumnStretch(col++, 10);
}

void
PluginControl::setValue(float value, bool emitSignals)
{
    m_dial->setPosition(value);

    if (emitSignals)
        emit valueChanged(value);
}

float
PluginControl::getValue() const
{
    if (!m_dial)
        return 0;

    return m_dial->getPosition();
}

void
PluginControl::slotValueChanged(float value)
{
    // ??? This used to be here to do conversion for log mode.  That is
    //     no longer needed.  Remove this middleman and connect things
    //     up directly to Rotary::valueChanged().  Assuming that is possible
    //     and makes sense.  Otherwise just connect Rotary to the signal and
    //     get rid of this function.

    emit valueChanged(value);
}


}
