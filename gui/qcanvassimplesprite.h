/***************************************************************************
                          qcanvassimplesprite.h  -  description
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

#ifndef QCANVASSIMPLESPRITE_H
#define QCANVASSIMPLESPRITE_H

#include <qcanvas.h>

/**A QCanvasSprite with 1 frame only
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class QCanvasSimpleSprite : public QCanvasSprite  {
public:
    QCanvasSimpleSprite(QPixmap *pixmap, QCanvas *canvas);
    QCanvasSimpleSprite(QCanvasPixmap *pixmap, QCanvas *canvas);
    QCanvasSimpleSprite(const QString &pixmapfile, QCanvas *canvas);

    virtual ~QCanvasSimpleSprite();

protected:
    QCanvasPixmapArray *m_pixmapArray;

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               QPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               QCanvasPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               const QString &pixmapfile);

};

#endif
