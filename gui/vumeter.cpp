// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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


#include <iostream>

#include "AudioLevel.h"

#include "vumeter.h"
#include "colours.h"
#include "rosedebug.h"

using Rosegarden::AudioLevel;


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
        case AudioPeakHoldShort:
        case AudioPeakHoldLong:
        case AudioPeakHoldIEC:
        case AudioPeakHoldIECLong:
        case FixedHeightVisiblePeakHold:
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

    if (m_alignment == Vertical) m_maxLevel = height;
    else m_maxLevel = width;

    int max = m_maxLevel;
    int red, orange, green;

    if (m_type == AudioPeakHoldShort) {
	red    = AudioLevel::dB_to_fader(   0.0, max, AudioLevel::ShortFader);
	orange = AudioLevel::dB_to_fader(  -2.0, max, AudioLevel::ShortFader);
	green  = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::ShortFader);
	m_background = QColor(50, 50, 50);
    } else if (m_type == AudioPeakHoldLong) {
	red    = AudioLevel::dB_to_fader(   0.0, max, AudioLevel::LongFader);
	orange = AudioLevel::dB_to_fader(  -2.0, max, AudioLevel::LongFader);
	green  = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::LongFader);
	m_background = QColor(50, 50, 50);
    } else if (m_type == AudioPeakHoldIEC) {
	red    = AudioLevel::dB_to_fader(  -0.1, max, AudioLevel::IEC268Meter);
	orange = AudioLevel::dB_to_fader(  -6.0, max, AudioLevel::IEC268Meter);
	green  = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::IEC268Meter);
	m_background = QColor(50, 50, 50);
    } else if (m_type == AudioPeakHoldIECLong) {
	red    = AudioLevel::dB_to_fader(   0.0, max, AudioLevel::IEC268LongMeter);
	orange = AudioLevel::dB_to_fader(  -6.0, max, AudioLevel::IEC268LongMeter);
	green  = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::IEC268LongMeter);
	m_background = QColor(50, 50, 50);
    } else {
	red    = max * 92 / 100;
	orange = max * 60 / 100;
	green  = max * 10 / 100;
	m_background = Qt::black;
    }

    if (m_type == AudioPeakHoldLong ||
	m_type == AudioPeakHoldShort ||
	m_type == AudioPeakHoldIEC ||
	m_type == AudioPeakHoldIECLong) {
	m_velocityColour =
	    new VelocityColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterSolidRed),
			       Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterSolidOrange),
			       Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterSolidGreen),
			       max, red, orange, green);
    } else {
	m_velocityColour =
	    new VelocityColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterRed),
			       Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterOrange),
			       Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::LevelMeterGreen),
			       max, red, orange, green);
    }
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
    switch (m_type) {

    case AudioPeakHoldShort:
	m_levelLeft = AudioLevel::dB_to_fader
	    (leftLevel, m_maxLevel, AudioLevel::ShortFader);
	m_levelRight = AudioLevel::dB_to_fader
	    (rightLevel, m_maxLevel, AudioLevel::ShortFader);
	break;

    case AudioPeakHoldLong:
	m_levelLeft = AudioLevel::dB_to_fader
	    (leftLevel, m_maxLevel, AudioLevel::LongFader);
	m_levelRight = AudioLevel::dB_to_fader
	    (rightLevel, m_maxLevel, AudioLevel::LongFader);
	break;

    case AudioPeakHoldIEC:
	m_levelLeft = AudioLevel::dB_to_fader
	    (leftLevel, m_maxLevel, AudioLevel::IEC268Meter);
	m_levelRight = AudioLevel::dB_to_fader
	    (rightLevel, m_maxLevel, AudioLevel::IEC268Meter);
	break;

    case AudioPeakHoldIECLong:
	m_levelLeft = AudioLevel::dB_to_fader
	    (leftLevel, m_maxLevel, AudioLevel::IEC268LongMeter);
	m_levelRight = AudioLevel::dB_to_fader
	    (rightLevel, m_maxLevel, AudioLevel::IEC268LongMeter);
	break;

    default:
	m_levelLeft = (int)(double(m_maxLevel) * leftLevel);
	m_levelRight = (int)(double(m_maxLevel) * rightLevel);
    };

    if (m_levelLeft < 0) m_levelLeft = 0;
    if (m_levelLeft > m_maxLevel) m_levelLeft = m_maxLevel;
    if (m_levelRight < 0) m_levelRight = 0;
    if (m_levelRight > m_maxLevel) m_levelRight = m_maxLevel;

    m_levelStepLeft = m_baseLevelStep;
    m_levelStepRight = m_baseLevelStep;

    // Only start the timer when we need it
    if (m_levelLeft > 0) {
	if (m_fallTimerLeft->isActive() == false) {
	    m_fallTimerLeft->start(40); // 40 ms per level fall iteration
	    meterStart();
	}
    }

    if (m_levelRight > 0) {
	if (m_fallTimerRight && m_fallTimerRight->isActive() == false) {
	    m_fallTimerRight->start(40); // 40 ms per level fall iteration
	    meterStart();
	}
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

    if (m_type == VUMeter::AudioPeakHoldShort ||
	m_type == VUMeter::AudioPeakHoldLong ||
	m_type == VUMeter::AudioPeakHoldIEC ||
	m_type == VUMeter::AudioPeakHoldIECLong)
    {
	paint.setPen(m_background);
	paint.setBrush(m_background);
	paint.drawRect(0, 0, width(), height());

        drawMeterLevel(&paint);

	paint.setPen(colorGroup().background());
	paint.drawPoint(0, 0);
	paint.drawPoint(width()-1, 0);
	paint.drawPoint(0, height()-1);
	paint.drawPoint(width()-1, height()-1);
    }
    else if (m_type == VUMeter::FixedHeightVisiblePeakHold)
    {
        //RG_DEBUG << "VUMeter::paintEvent - height = " << height() << endl;
	paint.setPen(m_background);
	paint.setBrush(m_background);
	paint.drawRect(0, 0, width(), height());

        if (m_fallTimerLeft->isActive())
            drawMeterLevel(&paint);
        else
        {
            meterStop();
            drawFrame(&paint);
            drawContents(&paint);
        }
    }
    else
    {
        if (m_fallTimerLeft->isActive())
        {
	    paint.setPen(m_background);
	    paint.setBrush(m_background);
	    paint.drawRect(0, 0, width(), height());
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
VUMeter::drawColouredBar(QPainter *paint, int channel,
			 int x, int y, int w, int h)
{
    if (m_type == AudioPeakHoldLong ||
	m_type == AudioPeakHoldShort ||
	m_type == AudioPeakHoldIEC ||
	m_type == AudioPeakHoldIECLong) {

	Qt::BrushStyle style = Qt::SolidPattern;

	int medium = m_velocityColour->getMediumKnee(),
	    loud = m_velocityColour->getLoudKnee();

	if (m_alignment == Vertical) {
	    if (h > loud) {
		paint->setPen(m_velocityColour->getLoudColour());
		paint->setBrush(QBrush(m_velocityColour->getLoudColour(),
				       style));
		paint->drawRect(x, y, w, h - loud);
	    }
	} else {
	    if (w > loud) {
		paint->setPen(m_velocityColour->getLoudColour());
		paint->setBrush(QBrush(m_velocityColour->getLoudColour(),
				       style));
		paint->drawRect(x + loud, y, w - loud, h);
	    }
	}

	if (m_alignment == Vertical) {
	    if (h > medium) {
		paint->setPen(m_velocityColour->getMediumColour());
		paint->setBrush(QBrush(m_velocityColour->getMediumColour(),
				       style));
		paint->drawRect(x, y + (h > loud ? (h - loud) : 0),
				w, std::min(h - medium, loud - medium));
	    }
	} else {
	    if (w > medium) {
		paint->setPen(m_velocityColour->getMediumColour());
		paint->setBrush(QBrush(m_velocityColour->getMediumColour(),
				       style));
		paint->drawRect(x + medium, y,
				std::min(w - medium, loud - medium), h);
	    }
	}

	if (m_alignment == Vertical) {
	    paint->setPen(m_velocityColour->getQuietColour());
	    paint->setBrush(QBrush(m_velocityColour->getQuietColour(),
				   style));
	    paint->drawRect(x, y + (h > medium ? (h - medium) : 0),
			    w, std::min(h, medium));
	} else {
	    paint->setPen(m_velocityColour->getQuietColour());
	    paint->setBrush(QBrush(m_velocityColour->getQuietColour(),
				   style));
	    paint->drawRect(x, y, std::min(w, medium), h);
	}
	
    } else {

	if (channel == 0) {

	    QColor mixedColour = m_velocityColour->getColour(m_levelLeft);

            paint->setPen(mixedColour);
            paint->setBrush(mixedColour);

	} else {

	    QColor mixedColour = m_velocityColour->getColour(m_levelRight);
	    
            paint->setPen(mixedColour);
            paint->setBrush(mixedColour);
	}

        //RG_DEBUG << "VUMeter::drawColouredBar - level = " << m_levelLeft << endl;

	paint->drawRect(x, y, w, h);
    }
}

void
VUMeter::drawMeterLevel(QPainter* paint)
{
    int medium = m_velocityColour->getMediumKnee(),
	loud = m_velocityColour->getLoudKnee();

    if (m_stereo)
    {
        if (m_alignment == VUMeter::Vertical)
        {
            int hW = width() / 2;

            // Draw the left bar
            //
            int y = height() - (m_levelLeft * height()) / m_maxLevel;

	    drawColouredBar(paint, 0, 0, y, hW - 1, height() - y);

	    paint->setPen(m_background);
	    paint->setBrush(m_background);
	    paint->drawRect(0, 0, hW, y);

            if (m_showPeakLevel)
            {
		int h = (m_peakLevelLeft * height()) / m_maxLevel;
                y = height() - h;

		if (h > loud) {
		    paint->setPen(Qt::red); // brighter than the red meter bar
		    paint->drawLine(0, y-1, hW - 2, y-1); 
		    paint->drawLine(0, y+1, hW - 2, y+1);
		}		    

                paint->setPen(Qt::white);
                paint->drawLine(0, y, hW - 2, y);
            }

            // Draw the right bar
            //
            y = height() - (m_levelRight * height()) / m_maxLevel;
	    drawColouredBar(paint, 1, hW + 1, y, width() - hW + 1, height() - y);

	    paint->setPen(m_background);
	    paint->setBrush(m_background);
	    paint->drawRect(hW, 0, width() - hW + 2, y);

            if (m_showPeakLevel)
            {
		int h = (m_peakLevelRight * height()) / m_maxLevel;
                y = height() - h;

		if (h > loud) {
		    paint->setPen(Qt::red); // brighter than the red meter bar
		    paint->drawLine(hW + 1, y-1, width(), y-1); 
		    paint->drawLine(hW + 1, y+1, width(), y+1);
		}		    

                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                paint->drawLine(hW + 1, y, width(), y);
            }
        }
        else // horizontal
        {
	    paint->setPen(m_background);
	    paint->setBrush(m_background);
	    paint->drawRect(0, 0, width(), height());

            int x = (m_levelLeft * width()) / m_maxLevel;
	    if (x > 0) paint->drawRect(0, 0, x, height());

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                // show peak level
                x = m_peakLevelLeft * width() / m_maxLevel;
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
        // Paint a vertical meter according to type
        //
        if (m_alignment == VUMeter::Vertical)
        {
            int y = height() - (m_levelLeft * height()) / m_maxLevel;
	    drawColouredBar(paint, 0, 0, y, width(), height());

	    paint->setPen(m_background);
	    paint->setBrush(m_background);
	    paint->drawRect(0, 0, width(), y);

            /*
            RG_DEBUG << "VUMeter::drawMeterLevel - height = " << height() 
                     << ", vertical rect height = " << y << endl;
                     */

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                y = height() - (m_peakLevelLeft * height()) / m_maxLevel;

                paint->drawLine(0, y, width(), y);
            }
        }
        else
        {
            int x = (m_levelLeft * width()) / m_maxLevel;
	    if (x > 0) drawColouredBar(paint, 0, 0, 0, x, height());

	    paint->setPen(m_background);
	    paint->setBrush(m_background);
	    paint->drawRect(x, 0, width() - x, height());

            if (m_showPeakLevel)
            {
                paint->setPen(Qt::white);
                paint->setBrush(Qt::white);

                // show peak level
                x = (m_peakLevelLeft * width()) / m_maxLevel;
                if (x < (width() - 1))
                    x++;
                else
                    x = width() - 1;

                paint->drawLine(x, 0, x, height());
            }
        }
    }
}

// Level range 0 - m_maxLevel
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


