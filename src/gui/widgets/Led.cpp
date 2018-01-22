/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#define RG_MODULE_STRING "[Led]"

#include "Led.h"

#include "misc/Debug.h"

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

    // Draw it at three times the size we need.  Then we'll scale it down
    // to make it look smooth and anti-aliased.
    const int scale = 3;
    width2 *= scale;

    // ??? rename: bigPixmap?
    QPixmap tmpMap(width2, width2);

    // Fill in the pixmap's background.
    tmpMap.fill(palette().window().color());

    // Begin painting on the big QPixmap.
    // ??? rename: bigPainter?
    QPainter tmpPainter(&tmpMap);

    // We are going to do our own anti-aliasing.
    tmpPainter.setRenderHint(QPainter::Antialiasing, false);

    // Set the color of the LED according to given parameters
    QColor color = m_state ? m_color : m_offColor;


    // *** Draw the flat LED

    tmpPainter.setPen(Qt::NoPen);
    QBrush brush(color);
    tmpPainter.setBrush(brush);
    tmpPainter.drawEllipse(0, 0, width2, width2);


    // *** Draw the catchlight (specular highlight or bright spot) on the LED.

    // The catchlight gives the LED a 3D bulb-like appearance.

    // Draw using modified "old" painter routine taken from KDEUI's Led widget.

    // Switch off the brush to avoid filling the ellipse.
    tmpPainter.setBrush(Qt::NoBrush);

    QPen pen;
    pen.setWidth(2 * scale);

    // Compute the center of the catchlight, (pos, pos).
    int pos = width2 / 5 + 1;
    // Shrink the catchlight to about 2/3 of the complete LED.
    int catchlightWidth = width2 * 2 / 3;

    int lightFactor = (130 * 2 / (catchlightWidth ? catchlightWidth : 1)) + 100;

    QColor catchlightColor = color;

    // Now draw the catchlight on the LED:
    while (catchlightWidth)
    {
        // make color lighter
        catchlightColor = catchlightColor.lighter(lightFactor);
        pen.setColor(catchlightColor);
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

        // Shift toward the center.
        ++pos;
    }

    // Final center point of the catchlight.
    tmpPainter.drawPoint(pos, pos);


    // *** Draw the round sunken frame around the LED.

    // ### shouldn't this value be smaller for smaller LEDs?
    // Given that there's no way to get an LED any size other than 16x16,
    // it makes no difference.
    pen.setWidth(2 * scale);

    // Set the initial color value to white.
    QColor frameColor = Qt::white;
    // Start drawing the shadow border at -45 degrees (-45*16 = -720).
    const int angle = -720;

    // Compute the frameRect.
    const int halfPenWidth = pen.width() / 2;
    const int frameWidth = width2 - halfPenWidth - scale + 1;
    const QRect frameRect(halfPenWidth, halfPenWidth, frameWidth, frameWidth);

    // From the highlight to the shadow (7.5 degrees to 180 in steps of
    // 15 degrees).
    for (int arc = 120; arc < 2880; arc += 240)
    {
        pen.setColor(frameColor);
        tmpPainter.setPen(pen);

        // Upper right half
        tmpPainter.drawArc(frameRect, angle + arc, 240);
        // Lower left half
        tmpPainter.drawArc(frameRect, angle - arc, 240);

        // Darken the frame as we go from the highlight to the shadow.
        frameColor = frameColor.darker(110);
    }

    tmpPainter.end();


    // *** Scale the big pixmap back down to the proper size.

    // Convert to a QImage for scaling.  We scale the image down to
    // make it look smooth.
    QImage image = tmpMap.toImage();
    width2 /= scale;
    image = image.scaled(width2, width2,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);


    // *** Draw it.

    // Draw the image to the display.
    painter.drawImage(0, 0, image);
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
