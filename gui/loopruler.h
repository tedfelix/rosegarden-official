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


class LoopRuler : public QCanvasView
{
    Q_OBJECT

public:
    LoopRuler(QCanvas* canvas = 0,
              QWidget* parent = 0,
              const int &bars = 0,
              const int &barWidth = 0,
              const int &height = 0,
              const char *name = 0);

    ~LoopRuler();

    // ActiveItem interface
    virtual void handleMousePress(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);
    virtual void handleMouseRelease(QMouseEvent*);

private:

    void drawBars();

    int m_bars;
    int m_barWidth;
    int m_height;
    int m_barSubSections;

    QCanvas *m_canvas;

};

#endif // _LOOPRULER_H_
