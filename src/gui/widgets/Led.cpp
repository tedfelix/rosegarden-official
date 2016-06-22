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
    m_offColor()
{
    setColor(color);
}

Led::~Led()
{
}

void
Led::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    draw(painter);
}

void
Led::draw(QPainter &painter)
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

    QPainter tmpPainter;
    QPixmap *tmpMap = 0;

    QColor backgroundColor = palette().window().color();

    // Draw it at three times the size we need.  Then we'll scale it down
    // to make it look smooth and anti-aliased.
    const int scale = 3;
    width2 *= scale;

    tmpMap = new QPixmap(width2, width2);

    // Fill in the pixmap's background.
    tmpMap->fill(backgroundColor);

    // Begin painting on the temporary QPixmap.
    tmpPainter.begin(tmpMap);

    // No need.  We are going to scale down and get anti-aliasing.
    tmpPainter.setRenderHint(QPainter::Antialiasing, false);

    // Set the color of the LED according to given parameters
    QColor color = m_state ? m_color : m_offColor;

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(color);
    tmpPainter.setBrush(brush);


    // *** Draw the flat LED

    tmpPainter.drawEllipse(scale, scale, width2 - scale*2, width2 - scale*2);


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
        tmpPainter.setPen(pen);

        tmpPainter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        tmpPainter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;
        if (!catchlightWidth)
            break;

        tmpPainter.drawEllipse(pos, pos, catchlightWidth, catchlightWidth);
        --catchlightWidth;

        ++pos;
    }

    tmpPainter.drawPoint(pos, pos);


    // *** Draw the round sunken frame around the LED.

    // ### shouldn't this value be smaller for smaller LEDs?
    // Given that there's no way to get an LED any size other than 16x16,
    // it makes no difference.
    pen.setWidth(2 * scale);
    // Switch off the brush to avoid filling the ellipse.
    brush.setStyle(Qt::NoBrush);
    tmpPainter.setBrush(brush);

    // Set the initial color value to palette().lighter() (bright) and start
    // drawing the shadow border at 45 degrees (45*16 = 720).

    int angle = -720;
    //color = palette().lighter();
    color = Qt::white;

    for (int arc = 120; arc < 2880; arc += 240)
    {
        pen.setColor(color);
        tmpPainter.setPen(pen);
        int w = width2 - pen.width() / 2 - scale + 1;
        tmpPainter.drawArc(pen.width() / 2, pen.width() / 2, w, w, angle + arc, 240);
        tmpPainter.drawArc(pen.width() / 2, pen.width() / 2, w, w, angle - arc, 240);
        //FIXME: this should somehow use the contrast value
        color = color.darker(110);
    }

    tmpPainter.end();

    // Convert to a QImage for scaling.  We scale the image down to
    // make it look smooth.
    QImage i = tmpMap->toImage();
    width2 /= scale;
    i = i.scaled(width2, width2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    delete tmpMap;
    tmpMap = NULL;

    // Convert QImage to QPixmap
    QPixmap dest(QPixmap::fromImage(i));

    // Draw the pixmap to the display.
    painter.drawPixmap(0, 0, dest);
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

    update();
}


}
