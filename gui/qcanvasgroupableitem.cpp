/***************************************************************************
                          qcanvasgroupableitem.cpp  -  description
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

#include <qcanvas.h>

#include "qcanvasitemgroup.h"
#include "qcanvasgroupableitem.h"
#include "rosedebug.h"

QCanvasGroupableItem::QCanvasGroupableItem(QCanvasItem *i,
                                           QCanvasItemGroup *g,
                                           bool withRelativeCoords)
    : m_group(g),
      m_item(i)
{
//     kdDebug(KDEBUG_AREA) << "QCanvasGroupableItem() - this : " << this
//                          << " - group : " << g
//                          << " - item : " << i << endl;

    if (withRelativeCoords)
        group()->addItemWithRelativeCoords(item());
    else
        group()->addItem(item());
}

QCanvasGroupableItem::~QCanvasGroupableItem()
{
//     kdDebug(KDEBUG_AREA) << "~QCanvasGroupableItem() - this : " << this
//                          << " - group : " << group()
//                          << " - item : " << item() << endl;

    // remove from the item group if we're still attached to one
    if (group())
        group()->removeItem(item());
}

void
QCanvasGroupableItem::relativeMoveBy(double dx, double dy)
{
    m_item->moveBy(dx + m_group->x(),
                   dy + m_group->y());
}

void
QCanvasGroupableItem::detach()
{
    m_group = 0;
}
