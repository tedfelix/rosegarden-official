/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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

#include <QFont>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QWidget>

#include <cmath>


namespace Rosegarden
{

PluginControl::PluginControl(QWidget *parent,
                             ControlType type,
                             PluginPort *port,
                             AudioPluginManager *aPM,
                             int index,
                             float initialValue,
                             bool showBounds):
        QWidget(parent),
        m_type(type),
        m_port(port),
        m_pluginManager(aPM),
        m_index(index)
{
    setObjectName("PluginControl");
    QGridLayout *hbox = new QGridLayout(this);
    hbox->setMargin(0);

    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 ) / 10);

    QLabel *controlTitle =
        new QLabel(QString("%1    ").arg(strtoqstr(port->getName())), this);
    controlTitle->setFont(plainFont);
    controlTitle->setMinimumWidth
        (QFontMetrics(controlTitle->font()).width("Bandwidth 1"));

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
        float pageStep = step * 10.f;
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

        float displayLower = lowerBound, displayUpper = upperBound;

        bool logarithmic = (port->getDisplayHint() & PluginPort::Logarithmic);

        if (logarithmic) {
            float logthresh = -10;
            float thresh = powf(10, logthresh);
            if (lowerBound > thresh) lowerBound = log10f(lowerBound);
            else {
                if (upperBound > 1) lowerBound = 0;
                else lowerBound = logthresh;
            }
            if (upperBound > thresh) upperBound = log10f(upperBound);
            else upperBound = logthresh;

            step = (upperBound - lowerBound) / 100.0;
            pageStep = step * 10.f;
            initialValue = log10f(initialValue);
        }

        QLabel *low;
        if (port->getDisplayHint() &
            (PluginPort::Integer | PluginPort::Toggled)) {
            low = new QLabel(QString("%1").arg(int(displayLower)), this);
        } else {
            low = new QLabel(QString("%1").arg(displayLower), this);
        }
        low->setFont(plainFont);
        low->setMinimumWidth(QFontMetrics(plainFont).width("0.001"));
        low->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

//        std::cerr << "port " << port->getName() << ": lower bound "
//                  << displayLower << ", upper bound " << displayUpper
//                  << ", logarithmic " << logarithmic << ", default "
//                  << initialValue << ", actual lower " << lowerBound
//                  << ", actual upper " << upperBound << ", step "
//                  << step << std::endl;

        m_dial = new ::Rosegarden::Rotary(this,
                                          lowerBound,    // min
                                          upperBound,    // max
                                          step,          // step
                                          pageStep,      // page step
                                          initialValue,  // initial
                                          30,            // size
                                          ticks,
                                          snapToTicks,
                                          false,         // centred
                                          logarithmic);

        m_dial->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPlugin));

        connect(m_dial, SIGNAL(valueChanged(float)),
                this, SLOT(slotValueChanged(float)));

        QLabel *upp;
        if (port->getDisplayHint() &
            (PluginPort::Integer | PluginPort::Toggled)) {
            upp = new QLabel(QString("%1").arg(int(displayUpper)), this);
        } else {
            upp = new QLabel(QString("%1").arg(displayUpper), this);
        }
        upp->setFont(plainFont);
        upp->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        upp->setMinimumWidth(QFontMetrics(plainFont).width("99999"));

        int col = 0;
        hbox->setColumnStretch(col++, 10);
        hbox->addWidget(controlTitle, 0, col++);

        if (showBounds) {
            low->show();
            hbox->addWidget(low, 0, col++);
        } else {
            low->hide();
        }

        hbox->addWidget(m_dial, 0, col++);

        if (showBounds) {
            upp->show();
            hbox->addWidget(upp, 0, col++);
        } else {
            upp->hide();
        }

        hbox->setColumnStretch(col++, 10);
    }
}

void
PluginControl::setValue(float value, bool emitSignals)
{
    float position = value;
    if (m_port->getDisplayHint() & PluginPort::Logarithmic) {
        position = log10f(position);
    }
    if (!emitSignals) m_dial->blockSignals(true);
    m_dial->setPosition(position);
    if (!emitSignals) m_dial->blockSignals(false);
    else emit valueChanged(value);
}

float
PluginControl::getValue() const
{
    if (m_port->getDisplayHint() & PluginPort::Logarithmic) {
        return m_dial == nullptr ? 0 : powf(10, m_dial->getPosition());
    } else {
        return m_dial == nullptr ? 0 : m_dial->getPosition();
    }
}

void
PluginControl::slotValueChanged(float value)
{
    if (m_port->getDisplayHint() & PluginPort::Logarithmic) {
        emit valueChanged(powf(10, value));
    } else {
        emit valueChanged(value);
    }
}

}
