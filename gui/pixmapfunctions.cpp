// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "pixmapfunctions.h"

#include <qimage.h>
#include <iostream>


QBitmap
PixmapFunctions::generateMask(const QPixmap &map, const QRgb &px)
{
    QImage i(map.convertToImage());
    QImage im(i.width(), i.height(), 1, 2, QImage::LittleEndian);

    for (int y = 0; y < i.height(); ++y) {
	for (int x = 0; x < i.width(); ++x) {
	    if (i.pixel(x, y) != px) {
		im.setPixel(x, y, 1);
	    } else {
		im.setPixel(x, y, 0);
	    }
	}
    }
    
    QBitmap m;
    m.convertFromImage(im);
    return m;
}

QBitmap
PixmapFunctions::generateMask(const QPixmap &map)
{
    QImage i(map.convertToImage());
    QImage im(i.width(), i.height(), 1, 2, QImage::LittleEndian);

    QRgb px0(i.pixel(0, 0));
    QRgb px1(i.pixel(i.width()-1, 0));
    QRgb px2(i.pixel(i.width()-1, i.height()-1));
    QRgb px3(i.pixel(0, i.height()-1));

    QRgb px(px0);
    if (px0 != px2 && px1 == px3) px = px1;
    
    for (int y = 0; y < i.height(); ++y) {
	for (int x = 0; x < i.width(); ++x) {
	    if (i.pixel(x, y) != px) {
		im.setPixel(x, y, 1);
	    } else {
		im.setPixel(x, y, 0);
	    }
	}
    }
    
    QBitmap m;
    m.convertFromImage(im);
    return m;
}

QPixmap
PixmapFunctions::colourPixmap(const QPixmap &map, int hue, int minValue)
{
    // assumes pixmap is currently in shades of grey; maps black ->
    // solid colour and greys -> shades of colour

    QImage image = map.convertToImage();

    int s, v;

    bool warned = false;
    
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {

            QColor pixel(image.pixel(x, y));

            int oldHue;
            pixel.hsv(&oldHue, &s, &v);

            if (oldHue >= 0) {
                if (!warned) {
                    std::cerr << "PixmapFunctions::recolour: Not a greyscale pixmap "
			      << "(found rgb value " << pixel.red() << ","
			      << pixel.green() << "," << pixel.blue() 
			      << "), hoping for the best" << std::endl;
                    warned = true;
                }
            }

            image.setPixel
                (x, y, QColor(hue,
                              255 - v,
                              v > minValue ? v : minValue,
                              QColor::Hsv).rgb());
        }
    }

    QPixmap rmap;
    rmap.convertFromImage(image);
    if (map.mask()) rmap.setMask(*map.mask());
    return rmap;
}

QPixmap
PixmapFunctions::flipVertical(const QPixmap &map)
{
    QPixmap rmap;
    QImage i(map.convertToImage());
    rmap.convertFromImage(i.mirror(false, true));

    if (map.mask()) {
	QImage im(map.mask()->convertToImage());
	QBitmap newMask;
	newMask.convertFromImage(im.mirror(false, true));
	rmap.setMask(newMask);
    }

    return rmap;
}

QPixmap
PixmapFunctions::flipHorizontal(const QPixmap &map)
{
    QPixmap rmap;
    QImage i(map.convertToImage());
    rmap.convertFromImage(i.mirror(true, false));

    if (map.mask()) {
	QImage im(map.mask()->convertToImage());
	QBitmap newMask;
	newMask.convertFromImage(im.mirror(true, false));
	rmap.setMask(newMask);
    }

    return rmap;
}

