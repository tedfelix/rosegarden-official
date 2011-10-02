/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "VUMeter.h"

#include "misc/Debug.h"
#include "base/AudioLevel.h"
#include "gui/general/GUIPalette.h"
#include "gui/rulers/VelocityColour.h"
#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QWidget>


namespace Rosegarden
{

VUMeter::VUMeter(QWidget *parent,
                 VUMeterType type,
                 bool stereo,
                 bool hasRecord,
                 int width,
                 int height,
                 VUAlignment alignment):
    QLabel(parent),
    m_originalHeight(height),
    m_active(true),
    m_type(type),
    m_alignment(alignment),
    m_levelLeft(0),
    m_recordLevelLeft(0),
    m_peakLevelLeft(0),
    m_levelStepLeft(m_baseLevelStep),
    m_recordLevelStepLeft(m_baseLevelStep),
    m_fallTimerLeft(0),
    m_peakTimerLeft(0),
    m_levelRight(0),
    m_recordLevelRight(0),
    m_peakLevelRight(0),
    m_levelStepRight(0),
    m_recordLevelStepRight(0),
    m_fallTimerRight(0),
    m_peakTimerRight(0),
    m_showPeakLevel(true),
    m_baseLevelStep(3),
    m_stereo(stereo),
    m_hasRecord(hasRecord)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);

    // Work out if we need peak hold first
    //
    switch (m_type) {
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
            this, SLOT(slotReduceLevelLeft()));

    if (m_showPeakLevel) {
        m_peakTimerLeft = new QTimer();

        connect(m_peakTimerLeft, SIGNAL(timeout()),
                this, SLOT(slotStopShowingPeakLeft()));
    }

    if (stereo) {
        m_fallTimerRight = new QTimer();

        connect(m_fallTimerRight, SIGNAL(timeout()),
                this, SLOT(slotReduceLevelRight()));

        if (m_showPeakLevel) {
            m_peakTimerRight = new QTimer();
            connect(m_peakTimerRight, SIGNAL(timeout()),
                    this, SLOT(slotStopShowingPeakRight()));
        }

    }

    setMinimumSize(width, m_originalHeight);
    setMaximumSize(width, m_originalHeight);

    if (m_alignment == Vertical)
        m_maxLevel = height;
    else
        m_maxLevel = width;

    int max = m_maxLevel;
    int red, orange, green;

    if (m_type == AudioPeakHoldShort) {
        red = AudioLevel::dB_to_fader( 0.0, max, AudioLevel::ShortFader);
        orange = AudioLevel::dB_to_fader( -2.0, max, AudioLevel::ShortFader);
        green = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::ShortFader);
        m_background = QColor(32, 32, 32);
    } else if (m_type == AudioPeakHoldLong) {
        red = AudioLevel::dB_to_fader( 0.0, max, AudioLevel::LongFader);
        orange = AudioLevel::dB_to_fader( -2.0, max, AudioLevel::LongFader);
        green = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::LongFader);
        m_background = QColor(32, 32, 32);
    } else if (m_type == AudioPeakHoldIEC) {
        red = AudioLevel::dB_to_fader( -0.1, max, AudioLevel::IEC268Meter);
        orange = AudioLevel::dB_to_fader( -6.0, max, AudioLevel::IEC268Meter);
        green = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::IEC268Meter);
        m_background = QColor(32, 32, 32);
    } else if (m_type == AudioPeakHoldIECLong) {
        red = AudioLevel::dB_to_fader( 0.0, max, AudioLevel::IEC268LongMeter);
        orange = AudioLevel::dB_to_fader( -6.0, max, AudioLevel::IEC268LongMeter);
        green = AudioLevel::dB_to_fader( -10.0, max, AudioLevel::IEC268LongMeter);
        m_background = QColor(32, 32, 32);
    } else {
        red = max * 92 / 100;
        orange = max * 60 / 100;
        green = max * 10 / 100;
        m_background = QColor(Qt::black);
    }

    if (m_type == AudioPeakHoldLong ||
        m_type == AudioPeakHoldShort ||
        m_type == AudioPeakHoldIEC ||
        m_type == AudioPeakHoldIECLong) {
        m_velocityColour =
            new VelocityColour(GUIPalette::getColour(GUIPalette::LevelMeterSolidRed),
                               GUIPalette::getColour(GUIPalette::LevelMeterSolidOrange),
                               GUIPalette::getColour(GUIPalette::LevelMeterSolidGreen),
                               max, red, orange, green);
    } else {
        m_velocityColour =
            new VelocityColour(GUIPalette::getColour(GUIPalette::LevelMeterRed),
                               GUIPalette::getColour(GUIPalette::LevelMeterOrange),
                               GUIPalette::getColour(GUIPalette::LevelMeterGreen),
                               max, red, orange, green);
    }
}

VUMeter::~VUMeter()
{
    delete m_velocityColour;
    delete m_peakTimerRight;
    delete m_peakTimerLeft;
    delete m_fallTimerRight;
    delete m_fallTimerLeft;
}

void
VUMeter::setLevel(double level)
{
    setLevel(level, level, false);
}

void
VUMeter::setLevel(double leftLevel, double rightLevel)
{
    setLevel(leftLevel, rightLevel, false);
}

void
VUMeter::setRecordLevel(double level)
{
    setLevel(level, level, true);
}

void
VUMeter::setRecordLevel(double leftLevel, double rightLevel)
{
    setLevel(leftLevel, rightLevel, true);
}

void
VUMeter::setLevel(double leftLevel, double rightLevel, bool record)
{
    if (!isVisible())
        return ;

    //    RG_DEBUG << "setLevel(" << (void *)this << "): record=" << record << ", leftLevel=" << leftLevel << ", hasRecord=" << m_hasRecord << endl;

    if (record && !m_hasRecord)
        return ;

    short &ll = (record ? m_recordLevelLeft : m_levelLeft);
    short &lr = (record ? m_recordLevelRight : m_levelRight);

    short pll = ll;
    short plr = lr;

    switch (m_type) {

    case AudioPeakHoldShort:
        ll = AudioLevel::dB_to_fader
            (leftLevel, m_maxLevel, AudioLevel::ShortFader);
        lr = AudioLevel::dB_to_fader
            (rightLevel, m_maxLevel, AudioLevel::ShortFader);
        break;

    case AudioPeakHoldLong:
        ll = AudioLevel::dB_to_fader
            (leftLevel, m_maxLevel, AudioLevel::LongFader);
        lr = AudioLevel::dB_to_fader
            (rightLevel, m_maxLevel, AudioLevel::LongFader);
        break;

    case AudioPeakHoldIEC:
        ll = AudioLevel::dB_to_fader
            (leftLevel, m_maxLevel, AudioLevel::IEC268Meter);
        lr = AudioLevel::dB_to_fader
            (rightLevel, m_maxLevel, AudioLevel::IEC268Meter);
        break;

    case AudioPeakHoldIECLong:
        ll = AudioLevel::dB_to_fader
            (leftLevel, m_maxLevel, AudioLevel::IEC268LongMeter);
        lr = AudioLevel::dB_to_fader
            (rightLevel, m_maxLevel, AudioLevel::IEC268LongMeter);
        break;

    case Plain:
    case PeakHold:
    case FixedHeightVisiblePeakHold:
    default:
        ll = (int)(double(m_maxLevel) * leftLevel);
        lr = (int)(double(m_maxLevel) * rightLevel);
    };

    if (ll < 0)
        ll = 0;
    if (ll > m_maxLevel)
        ll = m_maxLevel;
    if (lr < 0)
        lr = 0;
    if (lr > m_maxLevel)
        lr = m_maxLevel;

    if (record) {
        m_recordLevelStepLeft = m_baseLevelStep;
        m_recordLevelStepRight = m_baseLevelStep;
    } else {
        m_levelStepLeft = m_baseLevelStep;
        m_levelStepRight = m_baseLevelStep;
    }

    // Only start the timer when we need it
    if (ll > 0) {
        if (m_fallTimerLeft->isActive() == false) {
            m_fallTimerLeft->start(40); // 40 ms per level fall iteration
            meterStart();
        }
    }

    if (lr > 0) {
        if (m_fallTimerRight && m_fallTimerRight->isActive() == false) {
            m_fallTimerRight->start(40); // 40 ms per level fall iteration
            meterStart();
        }
    }

    if (!record) {

        // Reset level and reset timer if we're exceeding the
        // current peak
        //
        if (ll >= m_peakLevelLeft && m_showPeakLevel) {
            m_peakLevelLeft = ll;

            if (m_peakTimerLeft->isActive())
                m_peakTimerLeft->stop();

            m_peakTimerLeft->start(1000); // milliseconds of peak hold
        }

        if (lr >= m_peakLevelRight && m_showPeakLevel) {
            m_peakLevelRight = lr;

            if (m_peakTimerRight) {
                if (m_peakTimerRight->isActive())
                    m_peakTimerRight->stop();

                m_peakTimerRight->start(1000); // milliseconds of peak hold
            }
        }
    }

    if (m_active && (ll != pll || lr != plr)) {
        update();
    }
}

//###
// There's some problem here, and in the faders too, where width() and height()
// seem to return something one pixel larger than what's actually visible.  It
// seems to be the case that the actual object we're drawing on is the correct
// size, and its viewport is undersized by one, but I haven't worked out a way
// to make its viewport full-sized, so instead we just try to compress
// everything by one pixel to bring it into view.  This causes some random minor
// artifacts, like the fader button bottom shadow not appearing, but I figure
// this will be good enough for a first release anyway.  Here in VUMeter, I'm
// working on the outside border being one pixel too short, and working on the
// rounded blank zero state meter being cut off so it's only rounded on one
// side.

// The above problem may have been because drawRect() draws a rectangle that
// is the specified width and height plus the width of the pen.  After
// changing all of this over to the faster fillRect(), adjusting
// the width and height by one appears to no longer be necessary.

void
VUMeter::paintEvent(QPaintEvent *e)
{
//    RG_DEBUG << "VUMeter::paintEvent - height = " << height() << endl;
    QPainter paint(this);

    paint.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();

    if (m_type == VUMeter::AudioPeakHoldShort ||
        m_type == VUMeter::AudioPeakHoldLong ||
        m_type == VUMeter::AudioPeakHoldIEC ||
        m_type == VUMeter::AudioPeakHoldIECLong) {
        // Clearing the background is not necessary as drawMeterLevel()
        // fills the part without the bar with the background color.
//        paint.fillRect(0, 0, w, h, m_background);

        drawMeterLevel(&paint);

        paint.setPen(palette().background().color());
        paint.drawPoint(0, 0);
        paint.drawPoint(w, 0);
        paint.drawPoint(0, h - 1);
        paint.drawPoint(w, h - 1);
    } else if (m_type == VUMeter::FixedHeightVisiblePeakHold) {
        // Clearing the background is not necessary as drawMeterLevel()
        // fills the part without the bar with the background color.
//        paint.fillRect(0, 0, w, h, m_background);

        if (m_fallTimerLeft->isActive())
            drawMeterLevel(&paint);
        else {
            meterStop();
//&&            drawFrame(&paint);  //Q3Support -- not needed?
            paint.end();
            QLabel::paintEvent(e);
        }
    } else {
        if (m_fallTimerLeft->isActive()) {
            // Clearing the background is not necessary as drawMeterLevel()
            // fills the part without the bar with the background color.
//            paint.fillRect(0, 0, w, h, m_background);
            drawMeterLevel(&paint);
        } else {
            meterStop();
            QColor background = palette().color(backgroundRole());
            // Fill with background grey so the black text (QLabel track
            // number) will be visible.
            paint.fillRect(0, 0, w, h, background);
            paint.end();
            // Let QLabel draw the track number text.
            QLabel::paintEvent(e);			
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

        int medium = m_velocityColour->getMediumKnee(),
            loud = m_velocityColour->getLoudKnee();

        if (m_alignment == Vertical) {
            if (h > loud) {
                paint->fillRect(x, y, w, h - loud,
                                m_velocityColour->getLoudColour());
            }
        } else {  // horizontal
            if (w > loud) {
                paint->fillRect(x + loud, y, w - loud, h,
                                m_velocityColour->getLoudColour());
            }
        }

        if (m_alignment == Vertical) {
            if (h > medium) {
                paint->fillRect(x, y + (h > loud ? (h - loud) : 0),
                                w, std::min(h - medium, loud - medium),
                                m_velocityColour->getMediumColour());
            }
        } else {  // horizontal
            if (w > medium) {
                paint->fillRect(x + medium, y,
                                std::min(w - medium, loud - medium), h,
                                m_velocityColour->getMediumColour());
            }
        }

        if (m_alignment == Vertical) {
            paint->fillRect(x, y + (h > medium ? (h - medium) : 0),
                            w, std::min(h, medium),
                            m_velocityColour->getQuietColour());
        } else {  // horizontal
            paint->fillRect(x, y, std::min(w, medium), h,
                            m_velocityColour->getQuietColour());
        }

    } else {

        QColor mixedColour;

        if (channel == 0) {
            mixedColour = m_velocityColour->getColour(m_levelLeft);
        } else {
            mixedColour = m_velocityColour->getColour(m_levelRight);
        }

        //        RG_DEBUG << "VUMeter::drawColouredBar - level = " << m_levelLeft << endl;

        paint->fillRect(x, y, w, h, mixedColour);
    }
}

void
VUMeter::drawMeterLevel(QPainter* paint)
{
//    int medium = m_velocityColour->getMediumKnee();
    int loud = m_velocityColour->getLoudKnee();

    if (m_stereo) {
        if (m_alignment == VUMeter::Vertical) {
            int hW = width() / 2;

            int midWidth = 1;
            if (m_hasRecord) {
                if (width() > 10) {
                    midWidth = 2;
                }
            }

            // Draw the left bar
            //
            int y = height() - (m_levelLeft * height()) / m_maxLevel;
            int ry = height() - (m_recordLevelLeft * height()) / m_maxLevel;

            drawColouredBar(paint, 0, 0, y, hW - midWidth, height() - y);

            if (m_hasRecord) {
                drawColouredBar(paint, 0, hW - midWidth, ry, midWidth + 1, height() - ry);
            }

            paint->fillRect(0, 0, hW - midWidth, y, m_background);

            if (m_hasRecord) {
                paint->fillRect(hW - midWidth, 0, midWidth + 1, ry, m_background);
            }

            if (m_showPeakLevel) {
                int h = (m_peakLevelLeft * height()) / m_maxLevel;
                y = height() - h;

                if (h > loud) {
                    paint->setPen(Qt::red); // brighter than the red meter bar
                    paint->drawLine(0, y - 1, hW - midWidth - 1, y - 1);
                    paint->drawLine(0, y + 1, hW - midWidth - 1, y + 1);
                }

                paint->setPen(Qt::white);
                paint->drawLine(0, y, hW - midWidth - 1, y);
            }

            // Draw the right bar
            //
            y = height() - (m_levelRight * height()) / m_maxLevel;
            ry = height() - (m_recordLevelRight * height()) / m_maxLevel;
            drawColouredBar(paint, 1, hW + midWidth, y, hW - midWidth, height() - y);

            if (m_hasRecord) {
                drawColouredBar(paint, 1, hW, ry, midWidth + 1, height() - ry);
            }

            paint->fillRect(hW + midWidth, 0, hW - midWidth + 1, y, m_background);

            if (m_hasRecord) {
                paint->fillRect(hW, 0, midWidth, ry, m_background);
            }

            if (m_showPeakLevel) {
                int h = (m_peakLevelRight * height()) / m_maxLevel;
                y = height() - h;

                if (h > loud) {
                    paint->setPen(Qt::red); // brighter than the red meter bar
                    paint->drawLine(hW + midWidth, y - 1, width(), y - 1);
                    paint->drawLine(hW + midWidth, y + 1, width(), y + 1);
                }

                paint->setPen(Qt::white);
                paint->drawLine(hW + midWidth, y, width(), y);
            }
        } else // horizontal
        {
            paint->fillRect(0, 0, width(), height(), m_background);

            int x = (m_levelLeft * width()) / m_maxLevel;
            if (x > 0)
                paint->fillRect(0, 0, x, height(), m_background);

            if (m_showPeakLevel) {
                // show peak level
                x = m_peakLevelLeft * width() / m_maxLevel;
                if (x < (width() - 1))
                    x++;
                else
                    x = width() - 1;

                paint->setPen(Qt::white);
                paint->drawLine(x, 0, x, height());
            }
        }
    } else {  // monaural
        // Paint a vertical meter according to type
        //
        if (m_alignment == VUMeter::Vertical) {
            int y = height() - (m_levelLeft * height()) / m_maxLevel;
            drawColouredBar(paint, 0, 0, y, width(), height());

            paint->fillRect(0, 0, width(), y, m_background);

            /*
              RG_DEBUG << "VUMeter::drawMeterLevel - height = " << height() 
              << ", vertical rect height = " << y << endl;
            */

            if (m_showPeakLevel) {
                y = height() - (m_peakLevelLeft * height()) / m_maxLevel;

                paint->setPen(Qt::white);
                paint->drawLine(0, y, width(), y);
            }
        } else {  // Horizontal
            int x = (m_levelLeft * width()) / m_maxLevel;
            if (x > 0)
                drawColouredBar(paint, 0, 0, 0, x, height());

            paint->fillRect(x, 0, width() - x, height(), m_background);

            if (m_showPeakLevel) {
                // show peak level
                x = (m_peakLevelLeft * width()) / m_maxLevel;
                if (x < (width() - 1))
                    x++;
                else
                    x = width() - 1;

                paint->setPen(Qt::white);
                paint->drawLine(x, 0, x, height());
            }
        }
    }
}

void
VUMeter::slotReduceLevelRight()
{
    m_levelStepRight = int(m_levelRight) * m_baseLevelStep / 100 + 1;
    if (m_levelStepRight < 1)
        m_levelStepRight = 1;

    m_recordLevelStepRight = int(m_recordLevelRight) * m_baseLevelStep / 100 + 1;
    if (m_recordLevelStepRight < 1)
        m_recordLevelStepRight = 1;

    if (m_levelRight > 0)
        m_levelRight -= m_levelStepRight;
    if (m_recordLevelRight > 0)
        m_recordLevelRight -= m_recordLevelStepRight;

    if (m_levelRight <= 0) {
        m_levelRight = 0;
        m_peakLevelRight = 0;
    }

    if (m_recordLevelRight <= 0)
        m_recordLevelRight = 0;

    if (m_levelRight == 0 && m_recordLevelRight == 0) {
        // Always stop the timer when we don't need it
        if (m_fallTimerRight)
            m_fallTimerRight->stop();
        meterStop();
    }

    update();
}

void
VUMeter::slotReduceLevelLeft()
{
    m_levelStepLeft = int(m_levelLeft) * m_baseLevelStep / 100 + 1;
    if (m_levelStepLeft < 1)
        m_levelStepLeft = 1;

    m_recordLevelStepLeft = int(m_recordLevelLeft) * m_baseLevelStep / 100 + 1;
    if (m_recordLevelStepLeft < 1)
        m_recordLevelStepLeft = 1;

    if (m_levelLeft > 0)
        m_levelLeft -= m_levelStepLeft;
    if (m_recordLevelLeft > 0)
        m_recordLevelLeft -= m_recordLevelStepLeft;

    if (m_levelLeft <= 0) {
        m_levelLeft = 0;
        m_peakLevelLeft = 0;
    }

    if (m_recordLevelLeft <= 0)
        m_recordLevelLeft = 0;

    if (m_levelLeft == 0 && m_recordLevelLeft == 0) {
        // Always stop the timer when we don't need it
        if (m_fallTimerLeft)
            m_fallTimerLeft->stop();
        meterStop();
    }

    update();
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

}
#include "VUMeter.moc"
