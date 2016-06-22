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
#include <QSettings>
#include <QPixmap>

#include "misc/ConfigGroups.h"

namespace Rosegarden
{


Led::Led(const QColor& col, QWidget *parent) :
    QWidget(parent),
    m_state(On),
    m_color(),
    m_darkFactor(300),
    m_offColor(),
    m_thorn(false),
    m_offPixmap(NULL),
    m_onPixmap(NULL)
{
    setColor(col);
    m_offColor = col.dark(300);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();
}

Led::~Led()
{
    delete m_offPixmap;
    delete m_onPixmap;
}

void
Led::paintEvent(QPaintEvent *)
{
    int	width = this->width();

    // Make sure the LED is round!
    if (width > this->height())
        width = this->height();

    // leave one pixel border
    width -= 2;

    // Can't be seen?  Bail.
    if (width <= 0)
        return;

    QPainter paint;
    int scale = 1;
    QPixmap *tmpMap = 0;
    const bool smooth = true;

    // If smooth is enabled, we draw to the two pixmaps, on_map and off_map.
    if (smooth)
    {
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
        width *= scale;

        tmpMap = new QPixmap(width, width);

        // Fill in the pixmap's background.
        QColor bg = m_thorn ? QColor::fromRgb(0xDD, 0xDD, 0xDD) : palette().window().color();
        tmpMap->fill(bg);

        paint.begin(tmpMap);

    } else {
        paint.begin(this);
    }

    paint.setRenderHint(QPainter::Antialiasing, false);

    // Set the color of the LED according to given parameters
    QColor color = ( m_state ) ? m_color : m_offColor;

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    QBrush brush;
    brush.setStyle( Qt::SolidPattern );
    brush.setColor( color );
    paint.setBrush( brush );                // Assign the brush to the painter


    // *** Draw the flat LED

    paint.drawEllipse( scale, scale, width - scale*2, width - scale*2 );


    // *** Draw the catchlight (specular highlight or bright spot) on the LED.

    // The catchlight gives the LED a 3D bulb-like appearance.

    // Draw using modified "old" painter routine taken from KDEUI's Led widget.

    QPen pen;
    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( 2 * scale );

    // Compute the center of the catchlight, (pos, pos).
    int pos = width / 5 + 1;
    // Shrink the catchlight to about 2/3 of the complete LED.
    int light_width = width * 2 / 3;

    // Calculate the LED's "light factor":
    int light_quote = (130 * 2 / (light_width ? light_width : 1)) + 100;

    // Now draw the bright spot on the LED:
    while (light_width)
    {
        // make color lighter
        color = color.light( light_quote );
        pen.setColor( color );
        paint.setPen( pen );
        paint.drawEllipse( pos, pos, light_width, light_width );
        light_width--;
        if (!light_width)
            break;
        paint.drawEllipse( pos, pos, light_width, light_width );
        light_width--;
        if (!light_width)
            break;
        paint.drawEllipse( pos, pos, light_width, light_width );
        pos++;
        light_width--;
    }

    paint.drawPoint(pos, pos);


    // *** Draw the round sunken frame around the LED.

    pen.setWidth( 2 * scale + 1 ); // ### shouldn't this value be smaller for smaller LEDs?
    brush.setStyle( Qt::NoBrush );              // Switch off the brush
    paint.setBrush( brush );                        // This avoids filling of the ellipse

    // Set the initial color value to palette().light() (bright) and start
    // drawing the shadow border at 45 degrees (45*16 = 720).

    int angle = -720;
    //color = palette().light();
    color = Qt::white;

    for ( int arc = 120; arc < 2880; arc += 240 )
    {
        pen.setColor( color );
        paint.setPen( pen );
        int w = width - pen.width() / 2 - scale + 1;
        paint.drawArc( pen.width() / 2, pen.width() / 2, w, w, angle + arc, 240 );
        paint.drawArc( pen.width() / 2, pen.width() / 2, w, w, angle - arc, 240 );
        color = color.dark( 110 ); //FIXME: this should somehow use the contrast value
    }	// end for ( angle = 720; angle < 6480; angle += 160 )

    paint.end();

    if (smooth)
    {
        QPixmap *&dest = m_state ? m_onPixmap : m_offPixmap;

        // Convert to a QImage for scaling.  We scale the image down to
        // make it look smooth.
        QImage i = tmpMap->toImage();
        width /= 3;
        i = i.scaled(width, width, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        delete tmpMap;

        // Save the pixmap to on_map or off_map.
        dest = new QPixmap(QPixmap::fromImage(i));

        // Draw the pixmap to the display.
        paint.begin(this);
        paint.drawPixmap(0, 0, *dest);
        paint.end();
    }
}

Led::State
Led::state() const
{
    return m_state;
}

QColor
Led::color() const
{
    return m_color;
}

void
Led::setState( State state )
{
    if (m_state != state) {
        m_state = state;
        update();
    }
}

void
Led::setColor(const QColor& col)
{
    if (m_color != col) {
        m_color = col;
        m_offColor = col.dark(m_darkFactor);
        delete m_onPixmap;
        m_onPixmap = 0;
        delete m_offPixmap;
        m_offPixmap = 0;
        update();
    }
}

void
Led::setDarkFactor(int darkfactor)
{
    if (m_darkFactor != darkfactor) {
        m_darkFactor = darkfactor;
        m_offColor = m_color.dark(darkfactor);
        update();
    }
}

int
Led::darkFactor() const
{
    return m_darkFactor;
}

void
Led::toggle()
{
    m_state = (m_state == On) ? Off : On;
    update();
}

void
Led::on()
{
    setState(On);
}

void
Led::off()
{
    setState(Off);
}

QSize
Led::sizeHint() const
{
    return QSize(16, 16);
}

QSize
Led::minimumSizeHint() const
{
    return QSize(16, 16 );
}

}


