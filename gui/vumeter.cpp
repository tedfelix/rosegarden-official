// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "vumeter.h"
#include "colours.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;


VUMeter::VUMeter(QWidget *parent,
                 const VUMeterType &type,
                 const int &width,
                 const int &height,
                 const char *name):
    QLabel(parent, name),
    m_type(type),
    m_level(0),
    m_peakLevel(0),
    m_levelStep(3),
    m_originalHeight(height)
{
    setMinimumSize(width, m_originalHeight);
    setMaximumSize(width, m_originalHeight);

    connect(&m_fallTimer, SIGNAL(timeout()),
            this,         SLOT(reduceLevel()));

}

VUMeter::~VUMeter()
{
}

void
VUMeter::setLevel(const double &level)
{
    m_peakLevel = (int)(100.0 * level);

    if (m_peakLevel < 0) m_peakLevel = 0;
    if (m_peakLevel > 100) m_peakLevel = 100;

    // our internal reps are percentages
    // 
    m_level = m_peakLevel;

    // Only start the timer when we need it
    if(m_fallTimer.isActive() == false)
    {
        m_fallTimer.start(40);
        meterStart();
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}

void
VUMeter::paintEvent(QPaintEvent*)
{
    QPainter paint(this);

    if (m_fallTimer.isActive())
    {
        drawMeterLevel(&paint);
    }
    else
    {
        meterStop();
        drawFrame(&paint);
        drawContents(&paint);
    }
}

void
VUMeter::drawMeterLevel(QPainter* paint)
{
    paint->setPen(colorGroup().background());
    paint->setBrush(colorGroup().background());
    paint->drawRect(0, 0, width(), height());

    //paint->setPen(colorGroup().color(QColorGroup::Dark));
    //paint->setBrush(colorGroup().color(QColorGroup::Dark));

    if (m_level > 80)
    {
        paint->setPen(RosegardenGUIColours::LevelMeterRed);
        paint->setBrush(RosegardenGUIColours::LevelMeterRed);
    }
    else if (m_level > 60)
    {
        paint->setPen(RosegardenGUIColours::LevelMeterOrange);
        paint->setBrush(RosegardenGUIColours::LevelMeterOrange);
    }
    else
    {
        paint->setPen(RosegardenGUIColours::LevelMeterGreen);
        paint->setBrush(RosegardenGUIColours::LevelMeterGreen);
    }

    int x = m_level * width() / 100;
    paint->drawRect(0, 0, x, height());
}

// Level range 0 - 100
//
void
VUMeter::reduceLevel()
{
    if (m_level > 0)
        m_level -= m_levelStep;

    if (m_level <= 0)
    {
        m_level = 0;

        // Always stop the timer when we don't need it
        m_fallTimer.stop();
        meterStop();
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}

