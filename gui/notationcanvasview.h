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

#include "notepixmapfactory.h"

class StaffLine;

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    NotationCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                       const char *name=0, WFlags f=0);

    ~NotationCanvasView();

    /** Callback for a mouse button press event in the canvas */
    virtual void contentsMousePressEvent(QMouseEvent *e);
    /** Callback for a mouse button release event in the canvas */
    virtual void contentsMouseReleaseEvent(QMouseEvent *e);
    /** Callback for a mouse move event in the canvas */
    virtual void contentsMouseMoveEvent(QMouseEvent *e);

    void setCurrentNotePixmap(QCanvasPixmap note);

public slots:

    void currentNoteChanged(Note::Type);

signals:
    void noteInserted(int pitch, const QPoint&);
    
protected:

    void insertNote(const StaffLine*, const QPoint&);

    
    StaffLine* m_currentHighlightedLine;

    QCanvasSprite *m_currentNotePixmap;

    NotePixmapFactory m_notePixmapFactory;
    
};


#endif
