// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include "Event.h"
#include "SnapGrid.h"

#include "rosegardenguidoc.h"
#include "widgets.h"

namespace Rosegarden { class RulerScale; }

/**
 * LoopRuler is a widget that shows bar and beat durations on a
 * ruler-like scale, and reacts to mouse clicks by sending relevant
 * signals to modify position pointer and playback/looping states.
*/
class LoopRuler : public QWidget, public HZoomable
{
    Q_OBJECT

public:
    LoopRuler(Rosegarden::RulerScale *rulerScale,
              int height = 0,
	      double xorigin = 0.0,
	      bool invert = false,
              QWidget* parent = 0,
              const char *name = 0);

    ~LoopRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void scrollHoriz(int x);

    void setMinimumWidth(int width) { m_width = width; }

    void setHorizScaleFactor(double dy) { m_hScaleFactor = dy; }

public slots:
    void slotSetLoopingMode(bool value);
    void slotSetLoopMarker(Rosegarden::timeT startLoop,
			   Rosegarden::timeT endLoop);

protected:
    // ActiveItem interface
    virtual void mousePressEvent       (QMouseEvent*);
    virtual void mouseReleaseEvent     (QMouseEvent*);
    virtual void mouseDoubleClickEvent (QMouseEvent*);
    virtual void mouseMoveEvent        (QMouseEvent*);

    virtual void paintEvent(QPaintEvent*);

signals:
    // The three main functions that this class performs
    //
    /// Set the pointer position on mouse single click or drag
    void setPointerPosition(Rosegarden::timeT);

    /// Set pointer position and start playing on double click
    void setPlayPosition(Rosegarden::timeT);

    /// Set a playing loop
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

private:
    void drawBarSections(QPainter*);
    void drawLoopMarker(QPainter*);  // between loop positions

    //--------------- Data members ---------------------------------
    int  m_height;
    double m_xorigin;
    bool m_invert;
    int  m_currentXOffset;
    int  m_width;

    Rosegarden::RulerScale *m_rulerScale;
    Rosegarden::SnapGrid    m_grid;
    
    bool m_loop;
    Rosegarden::timeT m_startLoop;
    Rosegarden::timeT m_endLoop;
};

#endif // _LOOPRULER_H_

