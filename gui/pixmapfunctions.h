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

#ifndef PIXMAPFUNCTIONS_H
#define PIXMAPFUNCTIONS_H

#include <qpixmap.h>
#include <qbitmap.h>

class PixmapFunctions
{
public:
    /**
     * Generate a heuristic mask for the given pixmap.  Unlike
     * QPixmap::createHeuristicMask, this removes from the mask all
     * pixels that are apparently "background" even if they appear in
     * holes in the middle of the image.  This is more usually what we
     * want than the default behaviour of createHeuristicMask.
     *
     * The rgb value specifies the colour to treat as background.
     *
     * This function is slow.
     */
    static QBitmap generateMask(const QPixmap &map, const QRgb &rgb);

    /**
     * Generate a heuristic mask for the given pixmap.  Unlike
     * QPixmap::createHeuristicMask, this removes from the mask all
     * pixels that are apparently "background" even if they appear in
     * holes in the middle of the image.  This is more usually what we
     * want than the default behaviour of createHeuristicMask.
     *
     * This function calculates its own estimated colour to match as
     * background.
     *
     * This function is slow.
     */
    static QBitmap generateMask(const QPixmap &map);

    /**
     * Colour a greyscale pixmap with the given hue.
     * minValue specifies the minimum value (in the HSV sense) that
     * will be used for any recoloured pixel.
     */
    static QPixmap colourPixmap(const QPixmap &map, int hue, int minValue);

    /// Return a QPixmap that is a mirror image of map (including mask)
    static QPixmap flipVertical(const QPixmap &map);

    /// Return a QPixmap that is a mirror image of map (including mask)
    static QPixmap flipHorizontal(const QPixmap &map);

    /**
     * Using QPainter::drawPixmap to draw one pixmap on another does
     * not appear to take the mask into account properly.  Background
     * pixels in the second pixmap erase foreground pixels in the
     * first one, regardless of whether they're masked or not.  This
     * function does what I expect.
     *
     * Note that the source pixmap _must_ have a mask.
     */
    static void drawPixmapMasked(QPixmap &dest, QBitmap &destMask,
				 int x, int y,
				 const QPixmap &source);
};

#endif

