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

            int y = l * (noteHeight + 1);

            staffLine->setPoints(0,y, len,y);
            staffLine->moveBy(0,14);
        }

    // bass
    //

    // clef
    clefPixmap = new QCanvasPixmapArray("pixmaps/clef-bass.xpm");

    clef = new QCanvasSpriteGroupable(clefPixmap, canvas, this);

    clef->moveBy(0, 7*noteHeight + noteHeight / 2 + 2);

    // horizontal lines
    //
    for(unsigned int l = 0; l < nbLines; ++l) {

            QCanvasLineGroupable *staffLine = new QCanvasLineGroupable(canvas, this);

            int y = l * (noteHeight + 1) + (7 * noteHeight);

            staffLine->setPoints(0,y, len,y);
            staffLine->moveBy(0,14);
        }


    // Add vertical lines
    //
    QCanvasLineGroupable *staffVertLine = new QCanvasLineGroupable(canvas, this);

    int vertLineHeight = 11 * (noteHeight + 1) - noteHeight / 2 - 2;

    staffVertLine->setPoints(0,14,
                             0, vertLineHeight + 14);

    QPen pen(black, 3); // QCanvasLine says this is not supported yet (kde-qt-addon v1.90)
    staffVertLine->setPen(pen);

//     staffVertLine = new QCanvasLineGroupable(canvas(), staff);
//     staffVertLine->setPoints(5,14,
//                              5, vertLineHeight + 14);


    //setActive(false); // don't react to mousePress events
    setActive(true);

}


const unsigned int Staff::noteHeight = 8;
const unsigned int Staff::noteWidth = 9;
const unsigned int Staff::stalkLen = noteHeight * 7/2 - 6;
const unsigned int Staff::nbLines = 5;

