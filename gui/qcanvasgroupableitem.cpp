// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

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
