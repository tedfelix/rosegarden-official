// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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


#ifndef _LOOPRULER_H_
#define _LOOPRULER_H_

#include <qcanvas.h>
#include "Event.h"
#include "rosegardenguidoc.h"


// Creates a canvas widget that reacts to mouse clicks and
// sends relevant signals to modify position pointer and
// playback/looping states.
//
// Hopefully use this same class in TrackEditor and editing
// clients - they will have to be synced-up or managed at
// some point.
//
//

class LoopRuler : public QCanvasView
{
    Q_OBJECT

public:
    LoopRuler(RosegardenGUIDoc *doc,
              QCanvas* canvas = 0,
              QWidget* parent = 0,
              const int &bars = 0,
              const int &barWidth = 0,
              const int &height = 0,
              const char *name = 0);

    ~LoopRuler();

public slots:
    void setLoopingMode(bool value) { m_loop = value; }

protected:
    // ActiveItem interface
    virtual void contentsMousePressEvent(QMouseEvent *mE);
    virtual void contentsMouseReleaseEvent(QMouseEvent *mE);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent *mE);
    virtual void contentsMouseMoveEvent(QMouseEvent *mE);

signals:
    // The three main functions that this class performs
    //

    // Set the pointer position on mouse single click
    //
    void setPointerPosition(Rosegarden::timeT);

    // Set pointer position and start playing on double click
    //
    void setPlayPosition(Rosegarden::timeT);

    // Set a playing loop
    //
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

private:

    void drawBarSections();
    void drawLoopMarker();  // between loop positions

    // Get drawing position from pointer position and vice versa
    //
    int getXPosition(const Rosegarden::timeT &pos);
    Rosegarden::timeT getPointerPosition(const int &xPos);

    int m_bars;
    int m_barWidth;
    int m_height;
    QCanvas          *m_canvas;
    RosegardenGUIDoc *m_doc;
   
    bool m_loop;
    Rosegarden::timeT m_startLoop;
    Rosegarden::timeT m_endLoop;

    QCanvasRectangle *m_loopMarker;

    std::map<int, int> m_barWidthMap;

};

#endif // _LOOPRULER_H_

