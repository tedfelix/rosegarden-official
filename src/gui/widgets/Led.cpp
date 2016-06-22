/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    This file is based on KLed from the KDE libraries
    Copyright (C) 1998 Jörg Habenicht (j.habenicht@europemail.com)

    Including antialiasing implementation from Chris Cannam, April 2004

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Led.h"

#include "misc/ConfigGroups.h"

#include <QPainter>
#include <QImage>
#include <QPixmap>

namespace Rosegarden
{


Led::Led(const QColor &color, QWidget *parent) :
    QWidget(parent),
    m_state(On),
    m_backgroundColor(),
    m_color(),
    m_darkFactor(300),
    m_offColor(),
    m_offPixmap(NULL),
    m_onPixmap(NULL)
{
    setColor(color);
}

Led::~Led()
{
    delete m_offPixmap;
    m_offPixmap = NULL;

    delete m_onPixmap;
    m_onPixmap = NULL;
}

#if 1
// Original version.
void
Led::paintEvent(QPaintEvent *)
{
    int	width2 = width();

    // Make sure the LED is round!
    if (width2 > height())
        width2 = height();

    // leave one pixel border
    width2 -= 2;

    // Can't be seen?  Bail.
    if (width2 <= 0)
        return;

    QPainter paint;
    int scale = 1;
    QPixmap *tmpMap = 0;
    const bool smooth = true;

    // If smooth is enabled, we draw to tmpMap scaled up.
    if (smooth)
    {
        QColor backgroundColor = palette().window().color();

        // If the background color has changed.
        if (backgroundColor != m_backgroundColor) {
            // Invalidate the cached pixmaps.
            delete m_onPixmap;
            m_onPixmap = NULL;
            delete m_offPixmap;
            m_offPixmap = NULL;

            // Cache the background color so we can detect changes.
            m_backgroundColor = backgroundColor;
        }

        // Check to see if we already have pixmaps cached.  If we
        // do, use them and bail.

        if (m_state) {
            if (m_onPixmap) {
                paint.begin(this);
                paint.drawPixmap(0, 0, *m_onPixmap);
                paint.end();
                return ;
            }
        } else {
            if (m_offPixmap) {
                paint.begin(this);
                paint.drawPixmap(0, 0, *m_offPixmap);
                paint.end();
                return ;
            }
        }

        // We don't have any pixmaps cached.  Generate them.

        // Draw it at three times the size we need.  Then we'll scale it down
        // to make it look smooth and anti-aliased.
        scale = 3;
        width2 *= scale;

        tmpMap = new QPixmap(width2, width2);

        // Fill in the pixmap's background.
        tmpMap->fill(backgroundColor);

        paint.begin(tmpMap);

    } else {
        paint.begin(this);
    }

    paint.setRenderHint(QPainter::Antialiasing, false);

    // Set the color of the LED according to given parameters
    QColor color = m_state ? m_color : m_offColor;

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(color);
    paint.setBrush(brush);


    // *** Draw the flat LED

    paint.drawEllipse(scale, scale, width2 - scale*2, width2 - scale*2);


    // *** Draw the catchlight (specular highlight or bright spot) on the LED.

    // The catchlight gives the LED a 3D bulb-like appearance.

    // Draw using modified "old" painter routine taken from KDEUI's Led widget.

    QPen pen;
    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth(2 * scale);

    // Compute the center of the catchlight, (pos, pos).
    int pos = width2 / 5 + 1;
    // Shrink the catchlight to about 2/3 of the complete LED.
    int catchlightWidth = width2 * 2 / 3;

    int lightFactor = (130 * 2 / (catchlightWidth ? catchlightWidth : 1)) + 100;

    // Now draw the catchlight on the LED:
    while (catchlightWidth)
    {
        // make color lighter
        color = color.lighter(lightFactor);
        pen.setColor(color);
        paint.setPen(pen);

        paint.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        paint.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        paint.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;

        ++pos;
    }

    paint.drawPoint(pos, pos);


    // *** Draw the round sunken frame around the LED.

    // ### shouldn't this value be smaller for smaller LEDs?
    pen.setWidth(2 * scale + 1);
    // Switch off the brush to avoid filling the ellipse.
    brush.setStyle(Qt::NoBrush);
    paint.setBrush(brush);

    // Set the initial color value to palette().lighter() (bright) and start
    // drawing the shadow border at 45 degrees (45*16 = 720).

    int angle = -720;
    //color = palette().lighter();
    color = Qt::white;

    for (int arc = 120; arc < 2880; arc += 240)
    {
        pen.setColor(color);
        paint.setPen(pen);
        int w = width2 - pen.width() / 2 - scale + 1;
        paint.drawArc(pen.width() / 2, pen.width() / 2, w, w, angle + arc, 240);
        paint.drawArc(pen.width() / 2, pen.width() / 2, w, w, angle - arc, 240);
        //FIXME: this should somehow use the contrast value
        color = color.darker(110);
    }

    paint.end();

    if (smooth)
    {
        QPixmap *&dest = m_state ? m_onPixmap : m_offPixmap;

        // Convert to a QImage for scaling.  We scale the image down to
        // make it look smooth.
        QImage i = tmpMap->toImage();
        width2 /= 3;
        i = i.scaled(width2, width2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        delete tmpMap;
        tmpMap = NULL;

        // Save the pixmap to on_map or off_map.
        dest = new QPixmap(QPixmap::fromImage(i));

        // Draw the pixmap to the display.
        paint.begin(this);
        paint.drawPixmap(0, 0, *dest);
        paint.end();
    }
}
#else
// Simpler version.  Not as nice looking as the original version.
// In progress....
void
Led::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    draw(painter);
}
#endif

void
Led::draw(QPainter &painter)
{
    QRect window = painter.window();

    int width2 = window.width();

    // Make sure the LED is round!
    if (width2 > window.height())
        width2 = window.height();

    // leave one pixel border
    width2 -= 2;

    // Can't be seen?  Bail.
    if (width2 <= 0)
        return;

    // This shouldn't matter.  We need to rewrite this code so that it
    // does a great job no matter the scale.  Then we can scale it up/down
    // all we want to make it look smoother.  The key will be to make the
    // parts sized based on a percentage of the width.  Of course, at small
    // sizes, it will start to look bad.  That's when scaling up/down will
    // help.
    int scale = 1;

    // This actually looks ok.  It's not as good as the scaled up/down
    // version, but it's close.  I think with some tweaking, we can get
    // this looking better.  Then we can think about scaling to make
    // it even better if needed.  This is faster anyway.
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Set the color of the LED according to given parameters
    QColor color = m_state ? m_color : m_offColor;

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(color);
    painter.setBrush(brush);


    // *** Draw the flat LED

    painter.drawEllipse(scale, scale, width2 - scale*2, width2 - scale*2);


    // *** Draw the catchlight (specular highlight or bright spot) on the LED.

    // The catchlight gives the LED a 3D bulb-like appearance.

    // Draw using modified "old" painter routine taken from KDEUI's Led widget.

    QPen pen;
    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth(2 * scale);

    // Compute the center of the catchlight, (pos, pos).
    int pos = width2 / 5 + 1;
    // Shrink the catchlight to about 2/3 of the complete LED.
    int catchlightWidth = width2 * 2 / 3;

    int lightFactor = (130 * 2 / (catchlightWidth ? catchlightWidth : 1)) + 100;

    // Now draw the catchlight on the LED:
    while (catchlightWidth)
    {
        // make color lighter
        color = color.lighter(lightFactor);
        pen.setColor(color);
        painter.setPen(pen);

        painter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        painter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        painter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;

        ++pos;
    }

    painter.drawPoint(pos, pos);


    // *** Draw the round sunken frame around the LED.

    // ??? Shouldn't this value be smaller for smaller LEDs?  Yes.  It should
    //     be a percentage of the width.
    int penWidth = 2;
    int w = width2 - penWidth;
    pen.setWidth(penWidth);
    // Switch off the brush to avoid filling the ellipse.
    brush.setStyle(Qt::NoBrush);
    painter.setBrush(brush);

    // Set the initial color value to palette().lighter() (bright) and start
    // drawing the shadow border at 45 degrees (45*16 = 720).

    // -45 degrees (-720/16) is where the highlight is.
    int angle = -720;
    //color = palette().lighter();
    color = Qt::white;

    // For every 15 degrees (240/16).
    for (int arc = 120; arc < 2880; arc += 240)
    {
        pen.setColor(color);
        painter.setPen(pen);

        // x, y, w, h,start angle, span angle
        painter.drawArc(penWidth / 2, penWidth / 2, w, w, angle + arc, 240);
        painter.drawArc(penWidth / 2, penWidth / 2, w, w, angle - arc, 240);

        //FIXME: this should somehow use the contrast value
        color = color.darker(110);
    }

    painter.end();
}

void
Led::setState(State state)
{
    // No change?  Bail.
    if (state == m_state)
        return;

    m_state = state;
    update();
}

void
Led::setColor(const QColor &color)
{
    // No change?  Bail.
    if (color == m_color)
        return;

    m_color = color;
    m_offColor = color.darker(m_darkFactor);

    // Cached pixmaps are obsolete.
    delete m_onPixmap;
    m_onPixmap = NULL;
    delete m_offPixmap;
    m_offPixmap = NULL;

    update();
}


}
