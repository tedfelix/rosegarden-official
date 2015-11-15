/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
#include <QColor>
#include <QApplication>
#include <QSettings>

#include "misc/ConfigGroups.h"

namespace Rosegarden
{

class Led::LedPrivate
{
    friend class Led;

    int dark_factor;
    QColor offcolor;
    QPixmap *off_map;
    QPixmap *on_map;
};


Led::Led(const QColor& col, QWidget *parent) :
    QWidget(parent),
    led_state(On)
{
    d = new Led::LedPrivate;
    d->dark_factor = 300;
    d->offcolor = col.dark(300);
    d->off_map = 0;
    d->on_map = 0;
    setColor(col);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();
}

Led::~Led()
{
    delete d->off_map;
    delete d->on_map;
    delete d;
}

void
Led::paintEvent(QPaintEvent *)
{
    QPainter paint;
    QColor color;
    QBrush brush;
    QPen pen;

    // First of all we want to know what area should be updated
    // Initialize coordinates, width, and height of the LED
    int	width = this->width();

    // Make sure the LED is round!
    if (width > this->height())
        width = this->height();
    width -= 2; // leave one pixel border
    if (width < 0)
        width = 0;

    // maybe we could stop HERE, if width <=0 ?

    int scale = 1;
    QPixmap *tmpMap = 0;
    bool smooth = true;

    if (smooth)
    {
        if (led_state) {
            if (d->on_map) {
                paint.begin(this);
                paint.drawPixmap(0, 0, *d->on_map);
                paint.end();
                return ;
            }
        } else {
            if (d->off_map) {
                paint.begin(this);
                paint.drawPixmap(0, 0, *d->off_map);
                paint.end();
                return ;
            }
        }

        scale = 3;
        width *= scale;

        tmpMap = new QPixmap(width, width);
        QColor bg = m_Thorn ? QColor::fromRgb(0xDD, 0xDD, 0xDD) : palette().window().color();

        tmpMap->fill(bg);
        paint.begin(tmpMap);

    } else
    {
        paint.begin(this);
    }

    paint.setRenderHint(QPainter::Antialiasing, false);

    // Set the color of the LED according to given parameters
    color = ( led_state ) ? led_color : d->offcolor;

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    brush.setStyle( Qt::SolidPattern );
    brush.setColor( color );
    paint.setBrush( brush );                // Assign the brush to the painter

    // Draws a "flat" LED with the given color:
    paint.drawEllipse( scale, scale, width - scale*2, width - scale*2 );

    // Draw the bright light spot of the LED now, using modified "old"
    // painter routine taken from KDEUI´s Led widget:

    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( 2 * scale );

    // shrink the light on the LED to a size about 2/3 of the complete LED
    int pos = width / 5 + 1;
    int light_width = width;
    light_width *= 2;
    light_width /= 3;

    // Calculate the LED´s "light factor":
    int light_quote = (130 * 2 / (light_width ? light_width : 1)) + 100;

    // Now draw the bright spot on the LED:
    while (light_width)
    {
        color = color.light( light_quote );                      // make color lighter
        pen.setColor( color );                                   // set color as pen color
        paint.setPen( pen );                                     // select the pen for drawing
        paint.drawEllipse( pos, pos, light_width, light_width ); // draw the ellipse (circle)
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

    // Drawing of bright spot finished, now draw a thin border
    // around the LED which resembles a shadow with light coming
    // from the upper left.

    pen.setWidth( 2 * scale + 1 ); // ### shouldn't this value be smaller for smaller LEDs?
    brush.setStyle( Qt::NoBrush );              // Switch off the brush
    paint.setBrush( brush );                        // This avoids filling of the ellipse

    // Set the initial color value to palette().light() (bright) and start
    // drawing the shadow border at 45° (45*16 = 720).

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
    //
    // painting done

    if (smooth)
    {
        QPixmap *&dest = led_state ? d->on_map : d->off_map;
        QImage i = tmpMap->toImage();
        width /= 3;
        i = i.scaled(width, width, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        delete tmpMap;
        dest = new QPixmap(QPixmap::fromImage(i));
        paint.begin(this);
        paint.drawPixmap(0, 0, *dest);
        paint.end();
    }
}

Led::State
Led::state() const
{
    return led_state;
}

QColor
Led::color() const
{
    return led_color;
}

void
Led::setState( State state )
{
    if (led_state != state) {
        led_state = state;
        update();
    }
}

void
Led::setColor(const QColor& col)
{
    if (led_color != col) {
        led_color = col;
        d->offcolor = col.dark(d->dark_factor);
        delete d->on_map;
        d->on_map = 0;
        delete d->off_map;
        d->off_map = 0;
        update();
    }
}

void
Led::setDarkFactor(int darkfactor)
{
    if (d->dark_factor != darkfactor) {
        d->dark_factor = darkfactor;
        d->offcolor = led_color.dark(darkfactor);
        update();
    }
}

int
Led::darkFactor() const
{
    return d->dark_factor;
}

void
Led::toggle()
{
    led_state = (led_state == On) ? Off : On;
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


