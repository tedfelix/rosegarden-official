// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
                 VUMeterType type,
                 bool stereo,
                 int width,
                 int height,
                 VUAlignment alignment,
                 const char *name):
    QLabel(parent, name),
    m_originalHeight(height),
    m_type(type),
    m_alignment(alignment),
    m_levelLeft(0),
    m_peakLevelLeft(0),
    m_levelStepLeft(m_baseLevelStep),
    m_fallTimerLeft(0),
    m_peakTimerLeft(0),
    m_levelRight(0),
    m_peakLevelRight(0),
    m_levelStepRight(0),
    m_fallTimerRight(0),
    m_peakTimerRight(0),
    m_showPeakLevel(true),
    m_baseLevelStep(3),
    m_stereo(stereo)
{
    // Work out if we need peak hold first
    //
    switch (m_type)
    {
        case PeakHold:
        case AudioPeakHold:
            m_showPeakLevel = true;
            break;

        default:
        case Plain:
            m_showPeakLevel = false;
            break;
    }

    // Always init the left fall timer
    //
    m_fallTimerLeft = new QTimer();

    connect(m_fallTimerLeft, SIGNAL(timeout()),
            this,            SLOT(slotReduceLevelLeft()));

    if (m_showPeakLevel)
    {
        m_peakTimerLeft = new QTimer();

        connect(m_peakTimerLeft, SIGNAL(timeout()),
                this,            SLOT(slotStopShowingPeakLeft()));
    }

    if (stereo)
    {
        m_fallTimerRight = new QTimer();

        connect(m_fallTimerRight, SIGNAL(timeout()),
                this,             SLOT(slotReduceLevelRight()));

        if (m_showPeakLevel)
        {
            m_peakTimerRight = new QTimer();
            connect(m_peakTimerRight, SIGNAL(timeout()),
                    this,             SLOT(slotStopShowingPeakRight()));
        }

    }

    setMinimumSize(width, m_originalHeight);
    setMaximumSize(width, m_originalHeight);

    m_velocityColour =
        new VelocityColour(RosegardenGUIColours::LevelMeterRed,
                           RosegardenGUIColours::LevelMeterOrange,
                           RosegardenGUIColours::LevelMeterGreen,
                           100, // max
                           92,  // red knee
                           60,  // orange knee
                           10); // green knee
}

VUMeter::~VUMeter()
{
}

void
VUMeter::setLevel(double level)
{
    setLevel(level, level);
}

void
VUMeter::setLevel(double leftLevel, double rightLevel)
{
    m_levelLeft = (int)(100.0 * leftLevel);
    m_levelRight = (int)(100.0 * rightLevel);

    if (m_levelLeft < 0) m_levelLeft = 0;
    if (m_levelLeft > 100) m_levelLeft = 100;
    if (m_levelRight < 0) m_levelRight = 0;
    if (m_levelRight > 100) m_levelRight = 100;

    m_levelStepLeft = m_baseLevelStep;
    m_levelStepRight = m_baseLevelStep;

    // Only start the timer when we need it
    if(m_fallTimerLeft->isActive() == false)
    {
        m_fallTimerLeft->start(40); // 40 ms per level fall iteration
        meterStart();
    }

    if (m_fallTimerRight && m_fallTimerRight->isActive() == false)
    {
        m_fallTimerRight->start(40); // 40 ms per level fall iteration
        meterStart();
    }

    // Reset level and reset timer if we're exceeding the
    // current peak
    //
    if (m_levelLeft >= m_peakLevelLeft && m_showPeakLevel)
    {
        m_peakLevelLeft = m_levelLeft;

        if (m_peakTimerLeft->isActive())
            m_peakTimerLeft->stop();

        m_peakTimerLeft->start(1000); // milliseconds of peak hold
    }

    if (m_levelRight >= m_peakLevelRight && m_showPeakLevel)
    {
        m_peakLevelRight = m_levelRight;

        if (m_peakTimerRight)
        {
            if (m_peakTimerRight->isActive())
                m_peakTimerRight->stop();

            m_peakTimerRight->start(1000); // milliseconds of peak hold
        }
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}

void
VUMeter::paintEvent(QPaintEvent*)
{
    QPainter paint(this);

    if (m_type == VUMeter::AudioPeakHold)
    {
        drawMeterLevel(&paint);
    }
    else
    {
        if (m_fallTimerLeft->isActive())
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
}

void
VUMeter::drawMeterLevel(QPainter* paint)
{
    // Clear the rectangle.
    //
    paint->setPen(Qt::black);
    paint->setBrush(Qt::black);
    paint->drawRect(0, 0, width(), height());


    // Get the colour from the VelocityColour helper.
    //
    QColor mixedColour = m_velocityColour->getColour(m_levelLeft);

    if (m_stereo)
    {

        paint->setPen(mixedColour);
        paint->setBrush(mixedColour);

        if (m_alignment == VUMeter::Vertical)
        {
            int hW = width() / 2;

            // Draw the left bar
            //
            int y = height() - (m_levelLeft * height()) / 100;
            paint->drawRect(0, y, hW - 1, height() - y);

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                y = height() - (m_peakLevelLeft * height()) / 100;

                paint->drawLine(0, y, hW - 1, y);
            }

            // Select new colour for right bar
            mixedColour = m_velocityColour->getColour(m_levelRight);
            paint->setPen(mixedColour);
            paint->setBrush(mixedColour);

            // Draw the right bar
            //
            y = height() - (m_levelRight * height()) / 100;
            paint->drawRect(hW + 1, y, width(), height() - y);

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                y = height() - (m_peakLevelRight * height()) / 100;

                paint->drawLine(hW + 1, y, width(), y);
            }


        }
        else // horizontal
        {
            cout << "HERE HORIZ STERE" << endl;
            int x = (m_levelLeft * width()) / 100;
            paint->drawRect(0, 0, x, height());

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                // show peak level
                x = m_peakLevelLeft * width() / 100;
                if (x < (width() - 1))
                    x++;
                else
                    x = width() - 1;

                paint->drawLine(x, 0, x, height());
            }
        }
    }
    else
    {
        paint->setPen(mixedColour);
        paint->setBrush(mixedColour);

        // Paint a vertical meter according to type
        //
        if (m_alignment == VUMeter::Vertical)
        {
            int y = height() - (m_levelLeft * height()) / 100;
            paint->drawRect(0, height(), width(), y);

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                y = height() - (m_peakLevelLeft * height()) / 100;

                /*
                if (y < (height() - 1))
                    x++;
                else
                    x = width() - 1;
                    */

                paint->drawLine(0, y, width(), y);
            }
        }
        else
        {
            int x = (m_levelLeft * width()) / 100;
            paint->drawRect(0, 0, x, height());

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                // show peak level
                x = (m_peakLevelLeft * width()) / 100;
                if (x < (width() - 1))
                    x++;
                else
                    x = width() - 1;

                paint->drawLine(x, 0, x, height());
            }
        }
    }
}

// Level range 0 - 100
//
void
VUMeter::slotReduceLevelRight()
{
    m_levelStepRight = m_levelRight * m_baseLevelStep / 100 + 1;
    if (m_levelStepRight < 1)
	m_levelStepRight = 1;

    if (m_levelRight > 0)
        m_levelRight -= m_levelStepRight;

    if (m_levelRight <= 0)
    {
        m_levelRight = 0;
        m_peakLevelRight = 0;

        // Always stop the timer when we don't need it
        if (m_fallTimerRight) m_fallTimerRight->stop();
        meterStop();
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}

void
VUMeter::slotReduceLevelLeft()
{
    m_levelStepLeft = m_levelLeft * m_baseLevelStep / 100 + 1;
    if (m_levelStepLeft < 1)
	m_levelStepLeft = 1;

    if (m_levelLeft > 0)
        m_levelLeft -= m_levelStepLeft;

    if (m_levelLeft <= 0)
    {
        m_levelLeft = 0;
        m_peakLevelLeft = 0;

        // Always stop the timer when we don't need it
        m_fallTimerLeft->stop();
        meterStop();
    }

    QPainter paint(this);
    drawMeterLevel(&paint);
}


void
VUMeter::slotStopShowingPeakRight()
{
    m_peakLevelRight = 0;
}

void
VUMeter::slotStopShowingPeakLeft()
{
    m_peakLevelLeft = 0;
}


