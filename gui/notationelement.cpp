/***************************************************************************
                          notationelement.cpp  -  description
                             -------------------
    begin                : Sun Sep 10 2000
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

#include <qcanvas.h>

#include "notationelement.h"
#include "rosedebug.h"

NotationElement::NotationElement(Event *event)
    : ViewElement(event),
      m_canvasItem(0)
{
}

NotationElement::~NotationElement()
{
    // de-register from "observer"
    delete m_canvasItem;
}

bool
NotationElement::isRest() const
{
    return event()->getType() == "rest";
}


void
NotationElement::setCanvasItem(QCanvasItem *e)
{
    delete m_canvasItem;
    m_canvasItem = e;
}

bool operator<(NotationElement &e1, NotationElement &e2)
{
    kdDebug(KDEBUG_AREA) << "operator<(e1.m_x = "
                         << e1.m_x << ", e2.m_x = " << e2.m_x << endl;
    return e1.m_x < e2.m_x;
}
