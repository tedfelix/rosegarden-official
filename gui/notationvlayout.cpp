/***************************************************************************
                          notationvlayout.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
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

#include "notationvlayout.h"
#include "rosedebug.h"

NotationVLayout::NotationVLayout(Staff &s)
    : m_staff(s)
{
}

void
NotationVLayout::layout(NotationElement *el)
{

    if (el->isRest()) {

        el->setY(m_staff.pitchYOffset(17));
        // all rest pixmaps are sized so that they will be correctly
        // displayed when placed at that pitch height

    } else {

        try {
            int pitch = el->event()->get<Int>("pitch");

            //     kdDebug(KDEBUG_AREA) << "pitch : " << pitch
            //                          << " - height : " << m_pitchToHeight[pitch]
            //                          << " - staffOffset : " << m_staffOffsetY
            //                          << " - height + staffOffset : " << m_pitchToHeight[pitch] + m_staffOffsetY
            //                          << endl;

            el->setY(m_staff.pitchYOffset(pitch));
    
            kdDebug(KDEBUG_AREA) << "NotationVLayout::layout : pitch : "
                                 << pitch << " - y : " << el->y() << endl;
        }
        catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "NotationVLayout::layout : couldn't get pitch for element"
                                 << endl;
            el->setY(0);
        }
        
    }
}
