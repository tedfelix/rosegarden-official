
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PIXMAPFUNCTIONS_H_
#define _RG_PIXMAPFUNCTIONS_H_

#include <qbitmap.h>
#include <qpixmap.h>
#include <utility>


class QRgb;


namespace Rosegarden
{



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

    /**
     * Make a pixmap grey, or otherwise reduce its intensity.
     */
    static QPixmap shadePixmap(const QPixmap &map);

    /// Return a QPixmap that is a mirror image of map (including mask)
    static QPixmap flipVertical(const QPixmap &map);

    /// Return a QPixmap that is a mirror image of map (including mask)
    static QPixmap flipHorizontal(const QPixmap &map);

    /// Return left and right parts of the QPixmap
    static std::pair<QPixmap, QPixmap> splitPixmap(const QPixmap &original, int x);

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


}

#endif
