/***************************************************************************
                          notationcanvasview.h  -  description
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

#ifndef NOTATIONCANVASVIEW_H
#define NOTATIONCANVASVIEW_H

#include <qcanvas.h>

class StaffLine;

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationCanvasView : public QCanvasView
{
public:
    NotationCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                       const char *name=0, WFlags f=0);

    /** Callback for a mouse button press event in the canvas */
    virtual void contentsMousePressEvent(QMouseEvent *e);
    /** Callback for a mouse button release event in the canvas */
    virtual void contentsMouseReleaseEvent(QMouseEvent *e);
    /** Callback for a mouse move event in the canvas */
    virtual void contentsMouseMoveEvent(QMouseEvent *e);

signals:
    void noteInserted(int pitch, QMouseEvent*);
    
protected:

    void insertNote(const StaffLine*, QMouseEvent*);

    QCanvasItem* m_movingItem;
    bool m_draggingItem;

};


#endif
