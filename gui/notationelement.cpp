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

#include "viewelementsmanager.h"
#include "notationelement.h"

#ifndef NDEBUG
#include <iostream>
#include "rosedebug.h"
#include "Element2.h"
#endif

NotationElement::NotationElement(Event *event)
    : ViewElement(event),
      m_x(0),
      m_y(0),
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
    return event()->type() == "rest";
}

void
NotationElement::setCanvasItem(QCanvasItem *e)
{
    delete m_canvasItem;
    m_canvasItem = e;
}

//////////////////////////////////////////////////////////////////////

NotationElementList::~NotationElementList()
{
    for(iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
}

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream &dbg, NotationElement &e)
{
    dbg << "NotationElement - x : " << e.x() << " - y : " << e.y();
    e.event()->dump(cerr);

    return dbg;
}

kdbgstream& operator<<(kdbgstream &dbg, NotationElementList &l)
{
    for(NotationElementList::const_iterator i = l.begin();
        i != l.end(); ++i) {
        dbg << *(*i) << endl;
    }

    return dbg;
}
#endif
