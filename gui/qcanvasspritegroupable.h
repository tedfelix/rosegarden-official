/***************************************************************************
                          qcanvasspritegroupable.h  -  description
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

#ifndef QCANVASSPRITEGROUPABLE_H
#define QCANVASSPRITEGROUPABLE_H

#include <qcanvasgroupableitem.h>

/**A QCanvasSprite that can be put in a QCanvasGroup

  *@author Guillaume Laurent
  */

class QCanvasSpriteGroupable : public QCanvasSprite, public QCanvasGroupableItem  {
public:
    QCanvasSpriteGroupable(QCanvasPixmapArray*,
                           QCanvas*,
                           QCanvasItemGroup*);
};

#endif
