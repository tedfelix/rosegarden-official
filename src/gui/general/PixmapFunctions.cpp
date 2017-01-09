/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PixmapFunctions.h"

#include <QBitmap>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include <iostream>

namespace Rosegarden
{

QBitmap
PixmapFunctions::generateMask(const QPixmap &map, const QRgb &px)
{
    QImage i(map.toImage());
    // QImage im(i.width(), i.height(), 1, 2, QImage::LittleEndian);
    QImage im(i.width(), i.height(), QImage::Format_MonoLSB);

    for (int y = 0; y < i.height(); ++y) {
        for (int x = 0; x < i.width(); ++x) {
            if (i.pixel(x, y) != px) {
                im.setPixel(x, y, 1);
            } else {
                im.setPixel(x, y, 0);
            }
        }
    }

    QBitmap m = QBitmap::fromImage(im);
    return m;
}

QBitmap
PixmapFunctions::generateMask(const QPixmap &map)
{
    QImage i(map.toImage());
    QImage im(i.width(), i.height(), QImage::Format_MonoLSB);

    QRgb px0(i.pixel(0, 0));
    QRgb px1(i.pixel(i.width() - 1, 0));
    QRgb px2(i.pixel(i.width() - 1, i.height() - 1));
    QRgb px3(i.pixel(0, i.height() - 1));

    QRgb px(px0);
    if (px0 != px2 && px1 == px3)
        px = px1;

    for (int y = 0; y < i.height(); ++y) {
        for (int x = 0; x < i.width(); ++x) {
            if (i.pixel(x, y) != px) {
                im.setPixel(x, y, 1);
            } else {
                im.setPixel(x, y, 0);
            }
        }
    }

    QBitmap m = QBitmap::fromImage(im);
    return m;
}

QPixmap
PixmapFunctions::colourPixmap(const QPixmap &map, int hue, int minimum, int saturation)
{
    // assumes pixmap is currently in shades of grey; maps black ->
    // solid colour and greys -> shades of colour

    QImage image = map.toImage();

// This function has become obsolete and is no longer needed here anyway.
    // save a copy of the original alpha channel
//  QImage alpha = image.alphaChannel();

    int s, v;

    bool warned = false;

    for (int y = 0; y < image.height(); ++y) {

        for (int x = 0; x < image.width(); ++x) {

            QRgb oldPixel = image.pixel(x, y);
            QColor oldColour(oldPixel);  // This doesn't seem to pick up the alpha channel.

            // Explicitly set the alpha channel, making it a little stronger so that
            // colored note heads won't appear smaller than ordinary black ones.
            oldColour.setAlpha(int(qAlpha(oldPixel) * 1.5) > 255 ? 255 : int(qAlpha(oldPixel) * 1.5));

            int oldHue;
            oldColour.getHsv(&oldHue, &s, &v);

            int newHue = hue;

            if (oldHue >= 0) {
                if (!warned) {
                    std::cerr << "PixmapFunctions::recolour: Not a greyscale pixmap "
                              << "(found rgb value " << oldColour.red() << ","
                              << oldColour.green() << "," << oldColour.blue()
                              << "), hoping for the best" << std::endl;
                    warned = true;
                }
                newHue = hue;
            }

            // use the specified saturation, if present; otherwise the old
            // behaviour of subtracting the minimum value setting from a maximum
            // saturation of 255
            int newSaturation = (saturation == SaturationNotSpecified  ? 255 - v : saturation);

            QColor newColour = QColor::fromHsv(                
                                 newHue,
                                 newSaturation,
                                 v > minimum ? v : minimum);

//          QRgb newPixel = qRgba(newColour.red(),
//                                newColour.green(),
//                                newColour.blue(),
//                                qAlpha(oldPixel));

            // For some reason, while in the raster-graphics mode, the alpha channel
            // inverts the hues, rather than simply controlling pixel transparency.
            //
            // One way around the problem seems to be to modulate the individual
            // color components with the alpha channel, in addition to applying the
            // alpha channel in the usual way.
            QRgb newPixel = qRgba(int(newColour.red() * oldColour.alphaF()),
                                  int(newColour.green() * oldColour.alphaF()),
                                  int(newColour.blue() * oldColour.alphaF()),
                                  oldColour.alpha());

            image.setPixel(x, y, newPixel);
        }
    }

// This function has become obsolete and is no longer needed here anyway.
    // restore the original alpha channel
//  image.setAlphaChannel(alpha);

    QPixmap rmap = QPixmap::fromImage(image);
// This function is no longer needed here.
//  if (!map.mask().isNull()) rmap.setMask(map.mask());
    return rmap;
}

QPixmap
PixmapFunctions::shadePixmap(const QPixmap &map)
{
    QImage image = map.toImage();

    int h, s, v;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {

            QColor pixel(image.pixel(x, y));

            pixel.getHsv(&h, &s, &v);

            int newV =  255 - ((255 - v) / 2);
            QColor newColor = QColor::fromHsv(h, s, newV);
            
            image.setPixel(x, y, newColor.rgb());
            
        }
    }

    QPixmap rmap = QPixmap::fromImage(image);
    if (!map.mask().isNull()) rmap.setMask(map.mask());
    return rmap;
}

QPixmap
PixmapFunctions::flipVertical(const QPixmap &map)
{
    QImage i(map.toImage());
    QPixmap rmap = QPixmap::fromImage(i.mirrored(false, true));

    if (!map.mask().isNull()) {
        QImage im(map.mask().toImage());
        QBitmap newMask = QBitmap::fromImage(im.mirrored(false, true));
        rmap.setMask(newMask);
    }

    return rmap;
}

QPixmap
PixmapFunctions::flipHorizontal(const QPixmap &map)
{
    QImage i(map.toImage());
    QPixmap rmap = QPixmap::fromImage(i.mirrored(true, false));

    if (!map.mask().isNull()) {
        QImage im(map.mask().toImage());
        QBitmap newMask = QBitmap::fromImage(im.mirrored(true, false));
        rmap.setMask(newMask);
    }

    return rmap;
}

std::pair<QPixmap, QPixmap>
PixmapFunctions::splitPixmap(const QPixmap &pixmap, int x)
{
    //@@@ JAS ?need error check on pixmap.width and x? (x <= width)
    QPixmap left(x, pixmap.height());
    left.fill(Qt::transparent);

    QPixmap right(pixmap.width() - x, pixmap.height());
    right.fill(Qt::transparent);

    QPainter paint;

    paint.begin(&left);
    paint.drawPixmap(0, 0, pixmap, 0, 0, left.width(), left.height());
    paint.end();

    paint.begin(&right);
    paint.drawPixmap(0, 0, pixmap, left.width(), 0, right.width(), right.height());
    paint.end();

    return std::pair<QPixmap, QPixmap>(left, right);
}

void
PixmapFunctions::drawPixmapMasked(QPixmap &dest, QBitmap &destMask,
                                  int x0, int y0,
                                  const QPixmap &src)
{
    QImage idp(dest.toImage());
    QImage idm(destMask.toImage());
    QImage isp(src.toImage());
    QImage ism(src.mask().toImage());

    for (int y = 0; y < isp.height(); ++y) {
        for (int x = 0; x < isp.width(); ++x) {

            if (x >= ism.width())
                continue;
            if (y >= ism.height())
                continue;

            if (ism.depth() == 1 && ism.pixel(x, y) == 0)
                continue;
            if (ism.pixel(x, y) == QColor(Qt::white).rgb())
                continue;

            int x1 = x + x0;
            int y1 = y + y0;
            if (x1 < 0 || x1 >= idp.width())
                continue;
            if (y1 < 0 || y1 >= idp.height())
                continue;

            idp.setPixel(x1, y1, isp.pixel(x, y));
            idm.setPixel(x1, y1, 1);
        }
    }

    dest = QPixmap::fromImage(idp);
    destMask = QPixmap::fromImage(idm);
}

}
