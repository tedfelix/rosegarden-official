/***************************************************************************
                          staffline.cpp  -  description
                             -------------------
    begin                : Thu Sep 28 2000
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

#include "staffline.h"

#include "rosedebug.h"

StaffLine::StaffLine(QCanvas *c, QCanvasItemGroup *g, int height) :
    QCanvasLineGroupable(c, g),
    m_height(height)
{
    // empty
}

void
StaffLine::setHighlighted(bool highlighted)
{
//     kdDebug(KDEBUG_AREA) << "StaffLine::setHighlighted("
//                          << highlighted << ")\n";

    if (highlighted) {

        m_normalPen = pen();
        QPen newPen = m_normalPen;
        newPen.setColor(red);
        setPen(newPen);

    } else {

        setPen(m_normalPen);

    }
}

