/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Thumbwheel.h"

#include "base/Profiler.h"
#include "misc/ConfigGroups.h"
#include "gui/general/ThornStyle.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QInputDialog>
#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <iostream>

namespace Rosegarden
{

Thumbwheel::Thumbwheel(Qt::Orientation orientation,
                       bool useRed,
                       QWidget *parent) :
    QWidget(parent),
    m_min(0),
    m_max(100),
    m_default(50),
    m_value(50),
    m_rotation(0.5),
    m_orientation(orientation),
    m_speed(1.0),
    m_tracking(true),
    m_showScale(true),
    m_clicked(false),
    m_atDefault(true),
    m_clickRotation(m_rotation),
    m_showTooltip(true),
    m_bright(true),
    m_useRed(useRed)
{
    // NOTE: we should avoid using highlight() and mid() and so on, even though
    // they happen to produce nice results on my everyday setup, because these
    // will change according to external color preferences, and can produce
    // horrible results with the Thorn style.  (I need to fix this in the Rotary
    // and Fader code, and anywhere else it appears.)
}

Thumbwheel::~Thumbwheel()
{
}

void
Thumbwheel::setShowToolTip(bool show)
{
    m_showTooltip = show;
}

void
Thumbwheel::setMinimumValue(int min)
{
    if (m_min == min) return;

    m_min = min;
    if (m_max <= m_min) m_max = m_min + 1;
    if (m_value < m_min) m_value = m_min;
    if (m_value > m_max) m_value = m_max;

    m_rotation = float(m_value - m_min) / float(m_max - m_min);
    update();
}

int
Thumbwheel::getMinimumValue() const
{
    return m_min;
}

void
Thumbwheel::setMaximumValue(int max)
{
    if (m_max == max) return;

    m_max = max;
    if (m_min >= m_max) m_min = m_max - 1;
    if (m_value < m_min) m_value = m_min;
    if (m_value > m_max) m_value = m_max;

    m_rotation = float(m_value - m_min) / float(m_max - m_min);
    update();
}

int
Thumbwheel::getMaximumValue() const
{
    return m_max;
}

void
Thumbwheel::setDefaultValue(int deft)
{
    if (m_default == deft) return;

    m_default = deft;
    if (m_atDefault) {
        setValue(m_default);
        m_atDefault = true; // setValue unsets this
        m_cache = QImage();
        emit valueChanged(getValue());
    }
}

int
Thumbwheel::getDefaultValue() const
{
    return m_default;
}

void
Thumbwheel::setValue(int value)
{
//    std::cerr << "Thumbwheel::setValue(" << value << ") (from " << m_value
//              << ", rotation " << m_rotation << ")" << " (visible " << isVisible() << ")" << std::endl;

    if (m_value != value) {

        m_atDefault = false;

        if (value < m_min) value = m_min;
        if (value > m_max) value = m_max;
        m_value = value;
    }

    m_rotation = float(m_value - m_min) / float(m_max - m_min);
    m_cache = QImage();
    if (isVisible()) update();
}

void
Thumbwheel::resetToDefault()
{
    if (m_default == m_value) return;
    setValue(m_default);
    m_atDefault = true;
    m_cache = QImage();
    emit valueChanged(getValue());
}

int
Thumbwheel::getValue() const
{
    return m_value;
}

void
Thumbwheel::scroll(bool up)
{
    int step = lrintf(m_speed);
    if (step == 0) step = 1;

    if (up) {
        setValue(m_value + step);
    } else {
        setValue(m_value - step);
    }
    
    emit valueChanged(getValue());
}

void
Thumbwheel::setSpeed(float speed)
{
    m_speed = speed;
}

float
Thumbwheel::getSpeed() const
{
    return m_speed;
}

void
Thumbwheel::setTracking(bool tracking)
{
    m_tracking = tracking;
}

bool
Thumbwheel::getTracking() const
{
    return m_tracking;
}

void
Thumbwheel::setShowScale(bool showScale)
{
    m_showScale = showScale;
}

bool
Thumbwheel::getShowScale() const
{
    return m_showScale;
}

void
Thumbwheel::enterEvent(QEvent *)
{
    emit mouseEntered();
}

void
Thumbwheel::leaveEvent(QEvent *)
{
    emit mouseLeft();
}

void
Thumbwheel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MidButton ||
        ((e->button() == Qt::LeftButton) &&
         (e->modifiers() & Qt::ControlModifier))) {
        resetToDefault();
    } else if (e->button() == Qt::LeftButton) {
        m_clicked = true;
        m_clickPos = e->pos();
        m_clickRotation = m_rotation;
    }
}

void
Thumbwheel::mouseDoubleClickEvent(QMouseEvent *mouseEvent)
{
    //!!! needs a common base class with AudioDial (and Panner?)

    if (mouseEvent->button() != Qt::LeftButton) {
        return;
    }

    bool ok = false;

    int newValue = QInputDialog::getInt
        (this,
         tr("Enter new value"),
         tr("Enter a new value from %1 to %2:")
         .arg(m_min).arg(m_max),
         getValue(), m_min, m_max, 1, &ok);
    
    if (ok) {
        setValue(newValue);
        // Let everyone know.
        emit valueChanged(getValue());
    }
}


void
Thumbwheel::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_clicked) return;
    int dist = 0;
    if (m_orientation == Qt::Horizontal) {
        dist = e->x() - m_clickPos.x();
    } else {
        dist = e->y() - m_clickPos.y();
    }

    float rotation = m_clickRotation + (m_speed * dist) / 100;
    if (rotation < 0.f) rotation = 0.f;
    if (rotation > 1.f) rotation = 1.f;
    int value = lrintf(m_min + (m_max - m_min) * m_rotation);
    if (value != m_value) {
        setValue(value);
        if (m_tracking) emit valueChanged(getValue());
        m_rotation = rotation;
    } else if (fabsf(rotation - m_rotation) > 0.001) {
        m_rotation = rotation;
        repaint();
    }
}

void
Thumbwheel::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_clicked) return;
    bool reallyTracking = m_tracking;
    m_tracking = true;
    mouseMoveEvent(e);
    m_tracking = reallyTracking;
    m_clicked = false;
}

void
Thumbwheel::wheelEvent(QWheelEvent *e)
{
    int step = lrintf(m_speed);
    if (step == 0) step = 1;

    if (e->delta() > 0) {
        setValue(m_value + step);
    } else {
        setValue(m_value - step);
    }
    
    emit valueChanged(getValue());
}

void
Thumbwheel::paintEvent(QPaintEvent *)
{
    Profiler profiler("Thumbwheel::paintEvent");

    if (!m_cache.isNull()) {
        QPainter paint(this);
        paint.drawImage(0, 0, m_cache);
        return;
    }

    Profiler profiler2("Thumbwheel::paintEvent (no cache)");

    m_cache = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    m_cache.fill(Qt::transparent);

    int bw = 3;

    QRect subclip;
    if (m_orientation == Qt::Horizontal) {
        subclip = QRect(bw, bw+1, width() - bw*2, height() - bw*2 - 2);
    } else {
        subclip = QRect(bw+1, bw, width() - bw*2 - 2, height() - bw*2);
    }

    QPainter paint(&m_cache);
    paint.setClipRect(rect());
    QColor bg = (ThornStyle::isEnabled() ? QColor(0xED, 0xED, 0xFF) : palette().background().color());
    if (!m_bright) bg = bg.darker(125);
    paint.fillRect(subclip, bg);

    paint.setRenderHint(QPainter::Antialiasing, true);

    float w  = width();
    float w0 = 0.5;
    float w1 = w - 0.5;

    float h  = height();
    float h0 = 0.5;
    float h1 = h - 0.5;

    for (int i = bw-1; i >= 0; --i) {

        int grey = (i + 1) * (256 / (bw + 1));
        QColor fc = QColor(grey, grey, grey);
        paint.setPen(fc);

        QPainterPath path;

        if (m_orientation == Qt::Horizontal) {
            path.moveTo(w0 + i, h0 + i + 2);
            path.quadTo(w/2, i * 1.25, w1 - i, h0 + i + 2);
            path.lineTo(w1 - i, h1 - i - 2);
            path.quadTo(w/2, h - i * 1.25, w0 + i, h1 - i - 2);
            path.closeSubpath();
        } else {
            path.moveTo(w0 + i + 2, h0 + i);
            path.quadTo(i * 1.25, h/2, w0 + i + 2, h1 - i);
            path.lineTo(w1 - i - 2, h1 - i);
            path.quadTo(w - i * 1.25, h/2, w1 - i - 2, h0 + i);
            path.closeSubpath();
        }

        paint.drawPath(path);
    }

    paint.setClipRect(subclip);

    float radians = m_rotation * 1.5f * M_PI;

//    std::cerr << "value = " << m_value << ", min = " << m_min << ", max = " << m_max << ", rotation = " << rotation << std::endl;

    w = (m_orientation == Qt::Horizontal ? width() : height()) - bw*2;

    // total number of notches on the entire wheel
    int notches = 25;
    
    // radius of the wheel including invisible part
    int radius = int(w / 2 + 2);

    for (int i = 0; i < notches; ++i) {

        float a0 = (2.f * M_PI * i) / notches + radians;
        float a1 = a0 + M_PI / (notches * 2);
        float a2 = (2.f * M_PI * (i + 1)) / notches + radians;

        float depth = cosf((a0 + a2) / 2);
        if (depth < 0) continue;

        float x0 = radius * sinf(a0) + w/2;
        float x1 = radius * sinf(a1) + w/2;
        float x2 = radius * sinf(a2) + w/2;
        if (x2 < 0 || x0 > w) continue;

        if (x0 < 0) x0 = 0;
        if (x2 > w) x2 = w;

        x0 += bw;
        x1 += bw;
        x2 += bw;

        int grey = lrintf(120 * depth);

        QColor fc = QColor(grey, grey, grey);
        QColor oc = (ThornStyle::isEnabled() ? QColor(0xAA, 0xAA, 0xFF) : palette().highlight().color());
        if (m_useRed) oc = Qt::red;
        if (!m_bright) oc = oc.darker(125);

        paint.setPen(fc);

        if (m_showScale) {

            paint.setBrush(oc);

            float prop;
            if (i >= notches / 4) {
                prop = float(notches - (((i - float(notches) / 4.f) * 4.f) / 3.f))
                    / notches;
            } else {
                prop = 0.f;
            }
            
            if (m_orientation == Qt::Horizontal) {
                paint.drawRect(QRectF(x1, height() - (height() - bw*2) * prop - bw,
                                      x2 - x1, height() * prop));
            } else {
                paint.drawRect(QRectF(bw, x1, (width() - bw*2) * prop, x2 - x1));
            }
        }

        paint.setPen(fc);
        // calculated above, takes Thorn state into account
        paint.setBrush(bg);

        if (m_orientation == Qt::Horizontal) {
            paint.drawRect(QRectF(x0, bw, x1 - x0, height() - bw*2));
        } else {
            paint.drawRect(QRectF(bw, x0, width() - bw*2, x1 - x0));
        }
    }

    QPainter paint2(this);
    paint2.drawImage(0, 0, m_cache);
}

QSize
Thumbwheel::sizeHint() const
{
    if (m_orientation == Qt::Horizontal) {
        return QSize(80, 12);
    } else {
        return QSize(12, 80);
    }
}

void
Thumbwheel::setBright(const bool v)
{
    m_bright = v;
    update();
}


}


