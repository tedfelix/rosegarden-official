/***************************************************************************
                          staff.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "staff.h"
#include "qcanvaslinegroupable.h"
#include "qcanvasspritegroupable.h"


Staff::Staff(QCanvas *canvas)
    : QCanvasItemGroup(canvas)
{

    // treble
    //

    // clef
    QCanvasPixmapArray *clefPixmap = new QCanvasPixmapArray("pixmaps/clef-treble.xpm");

    QCanvasSpriteGroupable *clef = new QCanvasSpriteGroupable(clefPixmap, canvas, this);

    // horizontal lines
    //
    int w = canvas->width();
    int len = w - (w / 10);

    for(unsigned int l = 0; l < nbLines; ++l) {

            QCanvasLineGroupable *staffLine = new QCanvasLineGroupable(canvas, this);

            int y = l * lineWidth;

            staffLine->setPoints(0,y, len,y);
            staffLine->moveBy(0,linesOffset);
        }

    // bass
    //

    // clef
    clefPixmap = new QCanvasPixmapArray("pixmaps/clef-bass.xpm");

    clef = new QCanvasSpriteGroupable(clefPixmap, canvas, this);

    clef->moveBy(0, 7*noteHeight + noteHeight / 2);

    // horizontal lines
    //
    for(unsigned int l = 0; l < nbLines; ++l) {

            QCanvasLineGroupable *staffLine = new QCanvasLineGroupable(canvas, this);

            int y = l * lineWidth;

            staffLine->setPoints(0,y, len,y);
            staffLine->moveBy(0,linesOffset + (6 * lineWidth));
        }


    // Add vertical lines
    //
    QCanvasLineGroupable *staffVertLine = new QCanvasLineGroupable(canvas, this);

    int vertLineHeight = 11 * lineWidth - lineWidth / 2 - 4;

    staffVertLine->setPoints(0,linesOffset,
                             0,vertLineHeight + linesOffset);

    QPen pen(black, 3); // QCanvasLine says this is not supported yet (Qt 2.2beta)
    staffVertLine->setPen(pen);

//     staffVertLine = new QCanvasLineGroupable(canvas(), staff);
//     staffVertLine->setPoints(5,linesOffset,
//                              5, vertLineHeight + linesOffset);


    //setActive(false); // don't react to mousePress events
    setActive(false);

}


const unsigned int Staff::noteHeight = 8;
const unsigned int Staff::lineWidth = noteHeight + 1;
const unsigned int Staff::noteWidth = 9;
const unsigned int Staff::stalkLen = noteHeight * 7/2 - 6;
const unsigned int Staff::nbLines = 5;
const unsigned int Staff::linesOffset = 14;

