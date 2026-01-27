/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Thumbwheel]"
#define RG_NO_DEBUG_PRINT

#include "Thumbwheel.h"

#include "misc/Debug.h"
#include "gui/general/ThornStyle.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QInputDialog>
#include <QPainter>
#include <QPainterPath>

#include <math.h>


namespace Rosegarden
{


Thumbwheel::Thumbwheel(Qt::Orientation orientation,
                       QWidget *parent) :
    QWidget(parent),
    m_orientation(orientation)
{
    // NOTE: we should avoid using highlight() and mid() and so on, even though
    // they happen to produce nice results on my everyday setup, because these
    // will change according to external color preferences, and can produce
    // horrible results with the Thorn style.  (I need to fix this in the Rotary
    // and Fader code, and anywhere else it appears.)
}

void
Thumbwheel::setMinimumValue(int min)
{
    m_min = min;
}

void
Thumbwheel::setMaximumValue(int max)
{
    m_max = max;
}

void
Thumbwheel::setDefaultValue(int defaultValue)
{
    m_default = defaultValue;
    setValue(m_default);
}

void
Thumbwheel::setValue(int value)
{
    if (value < m_min)
        value = m_min;
    if (value > m_max)
        value = m_max;

    m_value = value;

    // Clear the cache.
    m_cache = QImage();

    if (isVisible())
        update();
}

void
Thumbwheel::resetToDefault()
{
    // Already there?  Bail.
    if (m_value == m_default)
        return;

    setValue(m_default);

    // Clear the cache.
    // ??? But setValue() already did this and potentially redrew it.  Why
    //     lose the work that it did?
    //m_cache = QImage();

    emit valueChanged(m_value);
}

void
Thumbwheel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton ||
        ((e->button() == Qt::LeftButton) &&
         (e->modifiers() & Qt::ControlModifier))) {
        resetToDefault();
    } else if (e->button() == Qt::LeftButton) {
        m_leftButtonPressed = true;
        m_clickPos = e->pos();
        m_clickValue = m_value;
    }
}

void
Thumbwheel::mouseDoubleClickEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

    bool ok = false;

    const int newValue = QInputDialog::getInt(
            this,
            tr("Enter new value"),
            tr("Enter a new value from %1 to %2:").arg(m_min).arg(m_max),
            m_value,
            m_min,
            m_max,
            1,  // step
            &ok);

    if (ok) {
        setValue(newValue);
        // Let everyone know.
        emit valueChanged(m_value);
    }
}

void
Thumbwheel::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_leftButtonPressed)
        return;

    // Compute the distance the mouse has moved in the relevant direction
    // in pixels.
    double dist = 0;
    if (m_orientation == Qt::Horizontal)
        dist = e->pos().x() - m_clickPos.x();
    else
        dist = e->pos().y() - m_clickPos.y();

    const int newValue = m_clickValue +
            lround(dist * m_speed * double(m_max - m_min) / 100.0);

    setValue(newValue);
    emit valueChanged(m_value);
}

void
Thumbwheel::mouseReleaseEvent(QMouseEvent * /*e*/)
{
    if (!m_leftButtonPressed)
        return;

    m_leftButtonPressed = false;
}

void
Thumbwheel::wheelEvent(QWheelEvent *e)
{
    // We'll handle this.  Don't pass to parent.
    e->accept();

    int step = lround(m_speed);
    if (step == 0) step = 1;

    if (e->angleDelta().y() > 0)
        setValue(m_value + step);
    else if (e->angleDelta().y() < 0)
        setValue(m_value - step);

    emit valueChanged(getValue());
}

void
Thumbwheel::paintEvent(QPaintEvent *)
{
    // Cache available?  Use it.
    if (!m_cache.isNull()) {
        QPainter paint(this);
        paint.drawImage(0, 0, m_cache);
        return;
    }

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
    QColor bg = (ThornStyle::isEnabled() ?
            QColor(0xED, 0xED, 0xFF) : palette().window().color());
    if (!m_bright)
        bg = bg.darker(125);
    paint.fillRect(subclip, bg);

    paint.setRenderHint(QPainter::Antialiasing, true);

    double w  = width();
    double w0 = 0.5;
    double w1 = w - 0.5;

    double h  = height();
    double h0 = 0.5;
    double h1 = h - 0.5;

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

    double rotation = double(m_value - m_min) / double(m_max - m_min);
    double radians = rotation * 1.5 * M_PI;

    //RG_DEBUG << "value =" << m_value << ", min =" << m_min << ", max =" << m_max << ", rotation =" << rotation;

    w = (m_orientation == Qt::Horizontal ? width() : height()) - bw*2;

    // total number of notches on the entire wheel
    int notches = 25;

    // radius of the wheel including invisible part
    int radius = int(w / 2 + 2);

    for (int i = 0; i < notches; ++i) {

        double a0 = (2.f * M_PI * i) / notches + radians;
        double a1 = a0 + M_PI / (notches * 2);
        double a2 = (2.f * M_PI * (i + 1)) / notches + radians;

        double depth = cos((a0 + a2) / 2);
        if (depth < 0) continue;

        double x0 = radius * sin(a0) + w/2;
        double x1 = radius * sin(a1) + w/2;
        double x2 = radius * sin(a2) + w/2;
        if (x2 < 0 || x0 > w) continue;

        if (x0 < 0) x0 = 0;
        if (x2 > w) x2 = w;

        x0 += bw;
        x1 += bw;
        x2 += bw;

        int grey = lround(120 * depth);

        QColor fc = QColor(grey, grey, grey);
        QColor oc = (ThornStyle::isEnabled() ? QColor(0xAA, 0xAA, 0xFF) : palette().highlight().color());
        if (m_red)
            oc = Qt::red;
        if (!m_bright)
            oc = oc.darker(125);

        paint.setPen(fc);

        if (m_showScale) {

            paint.setBrush(oc);

            double prop;
            if (i >= notches / 4) {
                prop = double(notches - (((i - double(notches) / 4.f) * 4.f) / 3.f))
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


}
