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
    m_originalHeight(height),
    m_type(type),
    m_level(0),
    m_peakLevel(0),
    m_levelStep(3),
    m_showPeakLevel(true)
{
    setMinimumSize(width, m_originalHeight);
    setMaximumSize(width, m_originalHeight);

    connect(&m_fallTimer, SIGNAL(timeout()),
            this,         SLOT(reduceLevel()));

    connect(&m_peakTimer, SIGNAL(timeout()),
            this,         SLOT(stopShowingPeak()));

}

VUMeter::~VUMeter()
{
}

void
VUMeter::setLevel(const double &level)
{
    m_level = (int)(100.0 * level);

    if (m_level < 0) m_level = 0;
    if (m_level > 100) m_level = 100;

    // Only start the timer when we need it
    if(m_fallTimer.isActive() == false)
    {
        m_fallTimer.start(40); // 40 ms per level fall iteration
        meterStart();
    }
    // Reset level and reset timer if we're exceeding the
    // current peak
    //
    if (m_level >= m_peakLevel)
    {
        m_peakLevel = m_level;
        m_showPeakLevel = true;

        if (m_peakTimer.isActive())
            m_peakTimer.stop();

        m_peakTimer.start(1000); // milliseconds of peak hold
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
    paint->setPen(Qt::black);
    paint->setBrush(Qt::black);
    paint->drawRect(0, 0, width(), height());

    paint->setPen(colorGroup().background());
    paint->setBrush(colorGroup().background());

    if (m_type == PeakHold && m_showPeakLevel)
    {
        paint->setPen(Qt::white);
        paint->setBrush(Qt::white);

        // show peak level
        int x = m_peakLevel * width() / 100;
        if (x < (width() - 1))
            x++;
        else
            x = width() - 1;

        paint->drawLine(x, 0, x, height());
    }

    /*
    if (m_level < 20)
    {
        paint->setPen(RosegardenGUIColours::LevelMeterGreen);
        paint->setBrush(RosegardenGUIColours::LevelMeterGreen);
    }
    else //if (m_level < 60) // fade from green to orange
    {
        int redSign = 1;
        int blueSign = 1;
        int greenSign = 1;

        int startRed = RosegardenGUIColours::LevelMeterGreen.red();
        int startBlue = RosegardenGUIColours::LevelMeterGreen.blue();
        int startGreen = RosegardenGUIColours::LevelMeterGreen.green();

        int endRed = RosegardenGUIColours::LevelMeterOrange.red();
        int endBlue = RosegardenGUIColours::LevelMeterOrange.blue();
        int endGreen = RosegardenGUIColours::LevelMeterOrange.green();

        if (startRed > endRed) redSign = -1;
        if (startBlue > endBlue) blueSign = -1;
        if (startGreen > endGreen) greenSign = -1;

        QColor mix(redSign * (int)(((double)(endRed - startRed))/((double)(m_level - 19))),
                   blueSign * (int)(((double)(endBlue - startBlue))/((double)(m_level - 19))),
                   greenSign * (int)(((double)(endGreen - startGreen))/((double)(m_level - 19))));

        //cout << "RED = " << mix.red() << endl;
        //cout << "BLUE = " << mix.blue() << endl;
        //cout << "GREEN = " << mix.green() << endl;

        paint->setPen(mix);
        paint->setBrush(mix);
    }
    else // fade from orange to red
    {
    }
    */

    if (m_level > 75)
    {
        paint->setPen(RosegardenGUIColours::LevelMeterRed);
        paint->setBrush(RosegardenGUIColours::LevelMeterRed);
    }
    else if (m_level > 50)
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

    // Make room for a peak hold marker if we need to
    if (m_type == PeakHold && x >= width() - 1) x--;

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
        m_peakLevel = 0;

        // Always stop the timer when we don't need it
        m_fallTimer.stop();
        meterStop();
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}


void
VUMeter::stopShowingPeak()
{
    m_showPeakLevel = false;
    m_peakLevel = 0;
}


