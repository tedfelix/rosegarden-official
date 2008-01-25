/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "Fader.h"
#include "TextFloat.h"

#include "misc/Debug.h"
#include "base/AudioLevel.h"
#include <qcolor.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qstring.h>
#include <qtimer.h>
#include <qwidget.h>
#include <cmath>

namespace Rosegarden
{

Fader::PixmapCache Fader::m_pixmapCache;


Fader::Fader(AudioLevel::FaderType type,
             int w, int h, QWidget *parent) :
        QWidget(parent),
        m_integral(false),
        m_vertical(h > w),
        m_min(0),
        m_max(0),
        m_type(type),
        m_clickMousePos( -1),
        m_float(new TextFloat(this)),
        m_floatTimer(new QTimer())
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
    //    if (m_vertical) {
    //	setFixedSize(w, h + m_buttonPixmap->height() + 4);
    //    } else {
    //	setFixedSize(w + m_buttonPixmap->width() + 4, h);
    //    }

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = colorGroup().mid();

    calculateGroovePixmap();
    setFader(0.0);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

Fader::Fader(int min, int max, int deflt,
             int w, int h, QWidget *parent) :
        QWidget(parent),
        m_integral(true),
        m_vertical(h > w),
        m_min(min),
        m_max(max),
        m_clickMousePos( -1),
        m_float(new TextFloat(this)),
        m_floatTimer(new QTimer())
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(w, h); // provisional
    calculateButtonPixmap();
    //    if (m_vertical) {
    //	setFixedSize(w, h + m_buttonPixmap->height() + 4);
    //    } else {
    //	setFixedSize(w + m_buttonPixmap->width() + 4, h);
    //    }

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = colorGroup().mid();

    calculateGroovePixmap();
    setFader(deflt);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
}

Fader::Fader(int min, int max, int deflt,
             bool vertical, QWidget *parent) :
        QWidget(parent),
        m_integral(true),
        m_vertical(vertical),
        m_min(min),
        m_max(max),
        m_clickMousePos( -1),
        m_float(new TextFloat(this)),
        m_floatTimer(new QTimer())
{
    setBackgroundMode(Qt::NoBackground);
    calculateButtonPixmap();

    if (m_vertical) {
        m_sliderMin = buttonPixmap()->height() / 2 + 2;
        m_sliderMax = height() - m_sliderMin;
    } else {
        m_sliderMin = buttonPixmap()->width() / 2 + 2;
        m_sliderMax = width() - m_sliderMin;
    }

    m_outlineColour = colorGroup().mid();

    calculateGroovePixmap();
    setFader(deflt);

    connect(m_floatTimer, SIGNAL(timeout()), this, SLOT(slotFloatTimeout()));
    m_float->hide();
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
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end()) {
        ColourPixmapRec::iterator j = i->second.first.find(m_outlineColour.pixel());
        if (j != i->second.first.end()) {
            return j->second;
        }
    }
    return 0;
}

QPixmap *
Fader::buttonPixmap()
{
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end()) {
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
    m_value = value;
    emit faderChanged(value);
    paintEvent(0);
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
     
        RG_DEBUG << "Fader::position_to_value - limited value = " << value << endl;
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
            int(sliderLength * (value - float(m_min)) / float(m_max - m_min) + 0.1);
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

    if (e->button() == LeftButton) {

        if (e->type() == QEvent::MouseButtonDblClick) {
            setFader(0);
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
            showFloatText();
        }
    }
}

void
Fader::wheelEvent(QWheelEvent *e)
{
    int buttonPosition = value_to_position(m_value);
    if (e->state() & ShiftButton) {
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
    RG_DEBUG << "Fader::wheelEvent - button position = " << buttonPosition << endl;
    setFader(position_to_value(buttonPosition));
    RG_DEBUG << "Fader::wheelEvent - value = " << m_value << endl;

    showFloatText();
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

    m_float->setText(text);

    // Reposition - we need to sum the relative positions up to the
    // topLevel or dialog to please move().
    //
    QWidget *par = parentWidget();
    QPoint totalPos = this->pos();

    while (par->parentWidget() && !par->isTopLevel() && !par->isDialog()) {
        totalPos += par->pos();
        par = par->parentWidget();
    }

    // Move just top/right
    //
    m_float->move(totalPos + QPoint(width() + 2, 0));

    // Show
    m_float->show();

    // one shot, 500ms
    m_floatTimer->start(500, true);
}

void
Fader::slotFloatTimeout()
{
    m_float->hide();
}

void
Fader::calculateGroovePixmap()
{
    QPixmap *& map = m_pixmapCache[SizeRec(width(), height())].first[m_outlineColour.pixel()];

    delete map;
    map = new QPixmap(width(), height());
    map->fill(colorGroup().background());
    QPainter paint(map);
    paint.setBrush(colorGroup().background());

    if (m_vertical) {

        paint.setPen(m_outlineColour);
        paint.drawRect(0, 0, width(), height());

        if (m_integral) {
            //...
        } else {
            for (int dB = -70; dB <= 10; ) {
                int position = value_to_position(float(dB));
                if (position >= 0 &&
                        position < m_sliderMax - m_sliderMin) {
                    if (dB == 0)
                        paint.setPen(colorGroup().dark());
                    else
                        paint.setPen(colorGroup().midlight());
                    paint.drawLine(1, (m_sliderMax - position),
                                   width() - 2, (m_sliderMax - position));
                }
                if (dB < -10)
                    dB += 10;
                else
                    dB += 2;
            }
        }

        paint.setPen(colorGroup().dark());
        paint.setBrush(colorGroup().mid());
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
    PixmapCache::iterator i = m_pixmapCache.find(SizeRec(width(), height()));
    if (i != m_pixmapCache.end() && i->second.second)
        return ;

    QPixmap *& map = m_pixmapCache[SizeRec(width(), height())].second;

    if (m_vertical) {

        int buttonHeight = height() / 7;
        buttonHeight /= 10;
        ++buttonHeight;
        buttonHeight *= 10;
        ++buttonHeight;
        int buttonWidth = width() * 2 / 3;
        buttonWidth /= 5;
        ++buttonWidth;
        buttonWidth *= 5;
        if (buttonWidth > width() - 2)
            buttonWidth = width() - 2;

        map = new QPixmap(buttonWidth, buttonHeight);
        map->fill(colorGroup().background());

        int x = 0;
        int y = 0;

        QPainter paint(map);

        paint.setPen(colorGroup().light());
        paint.drawLine(x + 1, y, x + buttonWidth - 2, y);
        paint.drawLine(x, y + 1, x, y + buttonHeight - 2);

        paint.setPen(colorGroup().midlight());
        paint.drawLine(x + 1, y + 1, x + buttonWidth - 2, y + 1);
        paint.drawLine(x + 1, y + 1, x + 1, y + buttonHeight - 2);

        paint.setPen(colorGroup().mid());
        paint.drawLine(x + 2, y + buttonHeight - 2, x + buttonWidth - 2,
                       y + buttonHeight - 2);
        paint.drawLine(x + buttonWidth - 2, y + 2, x + buttonWidth - 2,
                       y + buttonHeight - 2);

        paint.setPen(colorGroup().dark());
        paint.drawLine(x + 1, y + buttonHeight - 1, x + buttonWidth - 2,
                       y + buttonHeight - 1);
        paint.drawLine(x + buttonWidth - 1, y + 1, x + buttonWidth - 1,
                       y + buttonHeight - 2);

        paint.setPen(colorGroup().shadow());
        paint.drawLine(x + 1, y + buttonHeight / 2, x + buttonWidth - 2,
                       y + buttonHeight / 2);

        paint.setPen(colorGroup().mid());
        paint.drawLine(x + 1, y + buttonHeight / 2 - 1, x + buttonWidth - 2,
                       y + buttonHeight / 2 - 1);
        paint.drawPoint(x, y + buttonHeight / 2);

        paint.setPen(colorGroup().light());
        paint.drawLine(x + 1, y + buttonHeight / 2 + 1, x + buttonWidth - 2,
                       y + buttonHeight / 2 + 1);

        paint.setPen(colorGroup().button());
        paint.setBrush(colorGroup().button());
        paint.drawRect(x + 2, y + 2, buttonWidth - 4, buttonHeight / 2 - 4);
        paint.drawRect(x + 2, y + buttonHeight / 2 + 2,
                       buttonWidth - 4, buttonHeight / 2 - 4);

        paint.end();
    } else {
        //...
    }
}

}
#include "Fader.moc"
