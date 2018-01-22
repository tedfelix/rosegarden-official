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

#define RG_MODULE_STRING "[Fader]"

#include "Fader.h"

#include "TextFloat.h"
#include "misc/Debug.h"
#include "base/AudioLevel.h"

#include <QColor>
#include <QEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QColormap>

#include <cmath>
#include <map>

namespace Rosegarden
{

typedef std::pair<int, int> SizeRec;
typedef std::map<unsigned int, QPixmap *> ColourPixmapRec; // key is QColor::pixel()
typedef std::pair<ColourPixmapRec, QPixmap *> PixmapRec;
typedef std::map<SizeRec, PixmapRec> PixmapCache;
Q_GLOBAL_STATIC(PixmapCache, faderPixmapCache);

Fader::Fader(AudioLevel::FaderType type,
             int w, int h, QWidget *parent) :
        QWidget(parent),
        m_integral(false),
        m_vertical(h > w),
        m_min(0),
        m_max(0),
        m_default(0),
        m_type(type),
        m_clickMousePos( -1)
{
    //setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
    //    if (m_vertical) {
    //    setFixedSize(w, h + m_buttonPixmap->height() + 4);
    //    } else {
    //    setFixedSize(w + m_buttonPixmap->width() + 4, h);
    //    }

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = QColor(palette().mid().color());

    calculateGroovePixmap();
    setFader(0.0);
    emit faderChanged(m_value);
}

Fader::Fader(int min, int max, int deflt,
             int w, int h, QWidget *parent) :
        QWidget(parent),
        m_integral(true),
        m_vertical(h > w),
        m_min(min),
        m_max(max),
        m_default(deflt),
        m_clickMousePos( -1)
{
//    setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
    //    if (m_vertical) {
    //    setFixedSize(w, h + m_buttonPixmap->height() + 4);
    //    } else {
    //    setFixedSize(w + m_buttonPixmap->width() + 4, h);
    //    }

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = QColor(palette().mid().color());

    calculateGroovePixmap();
    setFader(deflt);
    emit faderChanged(m_value);
}

Fader::Fader(int min, int max, int deflt,
             bool vertical, QWidget *parent) :
        QWidget(parent),
        m_integral(true),
        m_vertical(vertical),
        m_min(min),
        m_max(max),
        m_default(deflt),
        m_clickMousePos( -1)
{
//    setBackgroundMode(Qt::NoBackground);
    calculateButtonPixmap();

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = QColor(palette().mid().color());

    calculateGroovePixmap();
    setFader(deflt);
    emit faderChanged(m_value);
}

Fader::~Fader()
{}

void
Fader::setOutlineColour(QColor c)
{
    m_outlineColour = c;
    calculateGroovePixmap();
}

QPixmap *
Fader::groovePixmap()
{
    PixmapCache::iterator i = faderPixmapCache()->find(SizeRec(width(), height()));
    if (i != faderPixmapCache()->end()) {
        QColormap colorMap = QColormap::instance();
        uint pixel(colorMap.pixel(m_outlineColour));
        ColourPixmapRec::iterator j = i->second.first.find(pixel);
        if (j != i->second.first.end()) {
            return j->second;
        }
    }
    return 0;
}

QPixmap *
Fader::buttonPixmap()
{
    PixmapCache::iterator i = faderPixmapCache()->find(SizeRec(width(), height()));
    if (i != faderPixmapCache()->end()) {
        return i->second.second;
    } else
        return 0;
}

float
Fader::getFaderLevel() const
{
    return m_value;
}

void
Fader::setFader(float value)
{
    if (m_value == value)
        return;

    m_value = value;
    update();
}

float
Fader::position_to_value(int position)
{
    float value;

    if (m_integral) {
        float sliderLength = float(m_sliderMax) - float(m_sliderMin);
        value = float(position)
                * float(m_max - m_min) / sliderLength - float(m_min);
        if (value > m_max)
            value = float(m_max);
        if (value < m_min)
            value = float(m_min);
    } else {
        value = AudioLevel::fader_to_dB
                (position, m_sliderMax - m_sliderMin, m_type);
    }
    /*
        RG_DEBUG << "Fader::position_to_value - position = " << position
                 << ", new value = " << value << endl;

        if (m_min != m_max) // works for integral case
        {
            if (value > m_max) value = float(m_max);
            if (value < m_min) value = float(m_min);
        }

        RG_DEBUG << "Fader::position_to_value - limited value = " << value;
    */
    return value;
}

int
Fader::value_to_position(float value)
{
    int position;

    if (m_integral) {
        float sliderLength = float(m_sliderMax) - float(m_sliderMin);
        position =
            int(nearbyintf(sliderLength * (value - float(m_min)) / float(m_max - m_min) + 0.1));
    } else {
        position =
            AudioLevel::dB_to_fader
            (value, m_sliderMax - m_sliderMin, m_type);
    }

    return position;
}

void
Fader::paintEvent(QPaintEvent *)
{
    QPainter paint(this);

    // sanity check
    //
    //!!!
    //@@@
    //
    // I really can't figure out why this suddenly became necessary, and I don't
    // understand why faders in the MIDI mixer that weren't on the very first
    // tab didn't have these pixmaps calculated.  When I just had the return,
    // all but the first page had blank faders, and they'd still crash if you
    // clicked on their undrawn blank areas.  So I thought what the hell, what
    // happens if we try to re-create the pixmaps here if they're not already
    // present?  And it seems to work.  The volume fader on page 2 controls the
    // volume in the IPB for device 2.  The whole thing has an air of fragility
    // about it though, and I wish I truly understood what was going on here.
    if (!buttonPixmap()) {
        calculateButtonPixmap();
    }

    if (!buttonPixmap()) return;

    if (!groovePixmap()) {
        calculateGroovePixmap();
    }

    if (!groovePixmap()) return;

    int position = value_to_position(m_value);

    if (m_vertical) {

        int aboveButton = height() - position - m_sliderMin - buttonPixmap()->height() / 2;
        int belowButton = position + m_sliderMin - buttonPixmap()->height() / 2;

        if (aboveButton > 0) {
            paint.drawPixmap(0, 0,
                             *groovePixmap(),
                             0, 0,
                             groovePixmap()->width(), aboveButton);
        }

        if (belowButton > 0) {
            paint.drawPixmap(0, aboveButton + buttonPixmap()->height(),
                             *groovePixmap(),
                             0, aboveButton + buttonPixmap()->height(),
                             groovePixmap()->width(), belowButton);
        }

        int buttonMargin = (width() - buttonPixmap()->width()) / 2;

        paint.drawPixmap(buttonMargin, aboveButton, *buttonPixmap());

        paint.drawPixmap(0, aboveButton,
                         *groovePixmap(),
                         0, aboveButton,
                         buttonMargin, buttonPixmap()->height());

        paint.drawPixmap(buttonMargin + buttonPixmap()->width(), aboveButton,
                         *groovePixmap(),
                         buttonMargin + buttonPixmap()->width(), aboveButton,
                         width() - buttonMargin - buttonPixmap()->width(),
                         buttonPixmap()->height());

    } else {
        //... update
        int leftOfButton =
            (m_sliderMax - m_sliderMin) - position - buttonPixmap()->width() / 2;

        int rightOfButton =
            position - buttonPixmap()->width() / 2;

        if (leftOfButton > 0) {
            paint.drawPixmap(0, 0,
                             *groovePixmap(),
                             0, 0,
                             leftOfButton, groovePixmap()->height());
        }

        if (rightOfButton > 0) {
            paint.drawPixmap(rightOfButton + buttonPixmap()->width(), 0,
                             *groovePixmap(),
                             groovePixmap()->width() - rightOfButton, 0,
                             rightOfButton, groovePixmap()->height());
        }

        paint.drawPixmap(leftOfButton, 0, *buttonPixmap());
    }

    paint.end();
}

void
Fader::mousePressEvent(QMouseEvent *e)
{
    m_clickMousePos = -1;

    if (e->button() == Qt::LeftButton) {

        if (e->type() == QEvent::MouseButtonDblClick) {
            setFader(0);
            emit faderChanged(m_value);
            return ;
        }

        if (m_vertical) {
            int buttonPosition = value_to_position(m_value);
            int clickPosition = height() - e->y() - m_sliderMin;

            if (clickPosition < buttonPosition + buttonPixmap()->height() / 2 &&
                    clickPosition > buttonPosition - buttonPixmap()->height() / 2) {
                m_clickMousePos = clickPosition;
                m_clickButtonPos = value_to_position(m_value);
                showFloatText();
            }
        }
    } else if (e->button() == Qt::MidButton) {  // reset to centre position
        setFader((m_max + m_min) / 2.0);
        emit faderChanged(m_value);
    } else if (e->button() == Qt::RightButton) {  // reset to default
        setFader(m_default);
        emit faderChanged(m_value);
    }

}

void
Fader::mouseReleaseEvent(QMouseEvent *e)
{
    mouseMoveEvent(e);
    m_clickMousePos = -1;
}

void
Fader::mouseMoveEvent(QMouseEvent *e)
{
    if (m_clickMousePos >= 0) {
        if (m_vertical) {
            int mousePosition = height() - e->y() - m_sliderMin;
            int delta = mousePosition - m_clickMousePos;
            int buttonPosition = m_clickButtonPos + delta;
            if (buttonPosition < 0)
                buttonPosition = 0;
            if (buttonPosition > m_sliderMax - m_sliderMin) {
                buttonPosition = m_sliderMax - m_sliderMin;
            }
            setFader(position_to_value(buttonPosition));
            emit faderChanged(m_value);
            showFloatText();
        }
    }
}

void
Fader::wheelEvent(QWheelEvent *e)
{
    int buttonPosition = value_to_position(m_value);
    if (e->modifiers() & Qt::SHIFT ) {
        if (e->delta() > 0)
            buttonPosition += 10;
        else
            buttonPosition -= 10;
    } else {
        if (e->delta() > 0)
            buttonPosition += 1;
        else
            buttonPosition -= 1;
    }
    //RG_DEBUG << "wheelEvent() - button position = " << buttonPosition;
    setFader(position_to_value(buttonPosition));
    emit faderChanged(m_value);
    //RG_DEBUG << "wheelEvent() - value = " << m_value;

    showFloatText();
}

void
Fader::enterEvent(QEvent *)
{
    TextFloat::getTextFloat()->attach(this);
}

void
Fader::showFloatText()
{
    // draw on the float text

    QString text;

    if (m_integral) {
        text = QString("%1").arg(int(m_value));
    } else if (m_value == AudioLevel::DB_FLOOR) {
        text = "Off";
    } else {
        float v = fabs(m_value);
        text = QString("%1%2.%3%4%5 dB")
               .arg(m_value < 0 ? '-' : '+')
               .arg(int(v))
               .arg(int(v * 10) % 10)
               .arg(int(v * 100) % 10)
               .arg(int(v * 1000) % 10);
    }

    TextFloat *textFloat = TextFloat::getTextFloat();

    textFloat->setText(text);

    // Reposition : Move just top/right
    QPoint offset = QPoint(width() + width() / 5, + height() / 5);
    textFloat->display(offset);

    // Keep text float visible for 500ms
    textFloat->hideAfterDelay(500);
}

void
Fader::calculateGroovePixmap()
{
    QColormap colorMap = QColormap::instance();
    uint pixel(colorMap.pixel(m_outlineColour));
    QPixmap *& map = (*faderPixmapCache())[SizeRec(width(), height())].first[pixel];

    delete map;
    map = new QPixmap(width(), height());

    // The area between the groove and the border takes a very translucent tint
    // of border color
    QColor bg = m_outlineColour;
    int H = 0;
    int S = 0;
    int V = 0;
    int A = 0;
    bg.getHsv(&H, &S, &V, &A);
    A = 40;
    bg = QColor::fromHsv(H, S, V, A);

    map->fill(bg);

    QPainter paint(map);
    paint.setBrush(bg);

    if (m_vertical) {

        paint.setPen(m_outlineColour);
        paint.drawRect(0, 0, width() - 1, height() - 1);

        if (m_integral) {
            //...
        } else {
            for (int dB = -70; dB <= 10; ) {
                int position = value_to_position(float(dB));
                if (position >= 0 &&
                        position < m_sliderMax - m_sliderMin) {
                    if (dB == 0) paint.setPen(palette().dark().color());
                    else paint.setPen(palette().midlight().color());
                    paint.drawLine(1, (m_sliderMax - position),
                                   width() - 2, (m_sliderMax - position));
                }
                if (dB < -10)
                    dB += 10;
                else
                    dB += 2;
            }
        }

        // the "groove" is a dark rounded rectangle like the ones on my real
        // mixer
        paint.setPen(QColor(0x20, 0x20, 0x20));
        paint.setBrush(QColor(0x20, 0x20, 0x20));
        paint.drawRect(width() / 2 - 3, height() - m_sliderMax,
                       6, m_sliderMax - m_sliderMin);
        paint.end();
    } else {
        //...
    }
}

void
Fader::calculateButtonPixmap()
{
    PixmapCache::iterator i = faderPixmapCache()->find(SizeRec(width(), height()));
    if (i != faderPixmapCache()->end() && i->second.second)
        return ;

    QPixmap *& map = (*faderPixmapCache())[SizeRec(width(), height())].second;

    int h = height() - 1;
    int w = width() - 1;

    if (m_vertical) {

        int buttonHeight = h / 7;
        buttonHeight /= 10;
        ++buttonHeight;
        buttonHeight *= 10;
        ++buttonHeight;
        int buttonWidth = w * 2 / 3;
        buttonWidth /= 5;
        ++buttonWidth;
        buttonWidth *= 5;
        buttonWidth -= 2;
        if (buttonWidth > w - 2)
            buttonWidth = w - 2;

        map = new QPixmap(buttonWidth, buttonHeight);

        // we have to draw something with our own stylesheet-compatible colors
        // instead of pulling button colors from the palette, and presumably
        // the active system style.  I tried to use a QLinearGradient for this
        // to match the stylesheet, but it didn't work, or I made a mistake in
        // the code.  We'll just use a solid color and be done with it then.
        // This should come out of GUIPalette, I suppose, but I don't feel like
        // rebuilding half the application every time I tweak the following
        // number:
        QColor bg(QColor(0xBB, 0xBB, 0xBB));
        map->fill(bg);

        int x = 0;
        int y = 0;

        QPainter paint(map);

        paint.setPen(palette().light().color());
        paint.drawLine(x + 1, y, x + buttonWidth - 2, y);
        paint.drawLine(x, y + 1, x, y + buttonHeight - 2);

        paint.setPen(palette().midlight().color());
        paint.drawLine(x + 1, y + 1, x + buttonWidth - 2, y + 1);
        paint.drawLine(x + 1, y + 1, x + 1, y + buttonHeight - 2);

        paint.setPen(palette().mid().color());
        paint.drawLine(x + 2, y + buttonHeight - 2, x + buttonWidth - 2,
                       y + buttonHeight - 2);
        paint.drawLine(x + buttonWidth - 2, y + 2, x + buttonWidth - 2,
                       y + buttonHeight - 2);

        paint.setPen(palette().dark().color());
        paint.drawLine(x + 1, y + buttonHeight - 1, x + buttonWidth - 2,
                       y + buttonHeight - 1);
        paint.drawLine(x + buttonWidth - 1, y + 1, x + buttonWidth - 1,
                       y + buttonHeight - 2);

        paint.setPen(palette().shadow().color());
        paint.drawLine(x + 1, y + buttonHeight / 2, x + buttonWidth - 2,
                       y + buttonHeight / 2);

        paint.setPen(palette().mid().color());
        paint.drawLine(x + 1, y + buttonHeight / 2 - 1, x + buttonWidth - 2,
                       y + buttonHeight / 2 - 1);
        paint.drawPoint(x, y + buttonHeight / 2);

        paint.setPen(palette().light().color());
        paint.drawLine(x + 1, y + buttonHeight / 2 + 1, x + buttonWidth - 2,
                       y + buttonHeight / 2 + 1);

        paint.setPen(bg);
        paint.setBrush(bg);
        paint.drawRoundedRect(x + 2, y + 2, buttonWidth - 4, buttonHeight / 2 - 4, 4.0, 4.0);
        paint.drawRoundedRect(x + 2, y + buttonHeight / 2 + 2,
                       buttonWidth - 4, buttonHeight / 2 - 4, 4.0, 4.0);

        paint.end();
    } else {
        //...
    }
}

}
