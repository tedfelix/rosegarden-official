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


Staff::Staff(QCanvas *canvas, Staff::Clef clef)
    : QCanvasItemGroup(canvas)
{

    // clef
    //
    QCanvasPixmapArray *clefPixmap;
    
    if (clef == Treble) {
        
        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-treble.xpm");

    } else if (clef == Bass) {

        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-bass.xpm");

    } else if (clef == Alto) {

        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-alto.xpm");

    } else if (clef == Tenor) {
        
        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-tenor.xpm");

    }
    
    
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

    // Add vertical line
    //
    QCanvasLineGroupable *staffVertLine = new QCanvasLineGroupable(canvas, this);

    int vertLineHeight = nbLines * lineWidth - lineWidth / 2 - 4;

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

int
Staff::pitch0YOffset() const
{
    return nbLines * lineWidth - lineWidth / 2 - 15;
}


const unsigned int Staff::noteHeight = 8;
const unsigned int Staff::lineWidth = noteHeight + 1;
const unsigned int Staff::noteWidth = 9;
const unsigned int Staff::stalkLen = noteHeight * 7/2 - 6;
const unsigned int Staff::nbLines = 5;
const unsigned int Staff::linesOffset = 14;

