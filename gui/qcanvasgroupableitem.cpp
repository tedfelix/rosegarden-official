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

#include "qcanvasgroupableitem.h"

QCanvasGroupableItem::QCanvasGroupableItem(QCanvasItem *i, QCanvasItemGroup *g)
    : m_group(g),
      m_item(i)
{
    group()->addItem(item());
}

QCanvasGroupableItem::~QCanvasGroupableItem()
{
    group()->removeItem(item());
}

