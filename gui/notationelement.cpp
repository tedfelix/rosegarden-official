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
      m_group(0),
      m_canvasItem(0)
{
    if (isGroup()) {
        EventList *g = event->group();
        
        m_group = ViewElementsManager::notationElementList(g->begin(),
                                                           g->end());
    }
}

NotationElement::~NotationElement()
{
    // de-register from "observer"
    delete m_group;
    delete m_canvasItem;
}

bool
NotationElement::isRest() const
{
    return event()->type() == "rest";
}

bool
NotationElement::isGroup() const
{
    return event()->type() == "group";
}


void
NotationElement::setCanvasItem(QCanvasItem *e)
{
    delete m_canvasItem;
    m_canvasItem = e;
}

bool operator<(NotationElement &e1, NotationElement &e2)
{
//     kdDebug(KDEBUG_AREA) << "operator<(e1.m_x = "
//                          << e1.m_x << ", e2.m_x = " << e2.m_x << ")" << endl;
    return e1.m_x < e2.m_x;
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
    if (e.isGroup()) {
        dbg << "Group NotationElement" << endl;
    } else {
        dbg << "NotationElement - x : " << e.x() << " - y : " << e.y();
        e.event()->dump(cerr);
    }

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
