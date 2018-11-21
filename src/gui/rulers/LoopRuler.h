
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LOOPRULER_H
#define RG_LOOPRULER_H

#include "base/SnapGrid.h"
#include <QSize>
#include <QWidget>
#include <QPen>
#include "base/Event.h"


class QPaintEvent;
class QPainter;
class QMouseEvent;


namespace Rosegarden
{

class RulerScale;
class RosegardenDocument;


/**
 * LoopRuler is a widget that shows bar and beat durations on a
 * ruler-like scale, and reacts to mouse clicks by sending relevant
 * signals to modify position pointer and playback/looping states.
*/
class LoopRuler : public QWidget
{
    Q_OBJECT

public:
    LoopRuler(RosegardenDocument *doc,
              RulerScale *rulerScale,
              int height = 0,
              bool invert = false,
              bool isForMainWindow = false,
              QWidget* parent = nullptr);

    ~LoopRuler();

    void setSnapGrid(const SnapGrid *grid);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void scrollHoriz(int x);

    void setMinimumWidth(int width) { m_width = width; }

    bool hasActiveMousePress() { return m_activeMousePress; }

    bool getLoopingMode() { return m_loopingMode; }
    
public slots:
    void slotSetLoopMarker(timeT startLoop,
                           timeT endLoop);

protected:
    double mouseEventToSceneX(QMouseEvent *mE);

    void mousePressEvent       (QMouseEvent*) override;
    void mouseReleaseEvent     (QMouseEvent*) override;
    void mouseDoubleClickEvent (QMouseEvent*) override;
    void mouseMoveEvent        (QMouseEvent*) override;

    void paintEvent(QPaintEvent*) override;

    void setLoopingMode(bool value) { m_loopingMode = value; }
    void drawBarSections(QPainter*);
    void drawLoopMarker(QPainter*);  // between loop positions

signals:
    // The three main functions that this class performs
    //
    /// Set the pointer position on mouse single click
    void setPointerPosition(timeT);

    /// Set the pointer position on mouse drag
    void dragPointerToPosition(timeT);

    /// Set pointer position and start playing on double click
    void setPlayPosition(timeT);

    /// Set a playing loop
    void setLoop(timeT, timeT);

    /// Set the loop end position on mouse drag
    void dragLoopToPosition(timeT);

    void startMouseMove(int directionConstraint);
    void stopMouseMove();
    void mouseMove();

protected:

    //--------------- Data members ---------------------------------
    int  m_height;
    bool m_invert;
    bool m_isForMainWindow;
    int  m_currentXOffset;
    int  m_width;
    bool m_activeMousePress;
    // remeber the mouse x pos in mousePressEvent and in mouseMoveEvent so that
    // we can emit it in mouseReleaseEvent to update the pointer position in other views
    double m_lastMouseXPos;

    RosegardenDocument *m_doc;
    RulerScale *m_rulerScale;
    SnapGrid   m_defaultGrid;
    SnapGrid   *m_loopGrid;
    const SnapGrid   *m_grid;
    QPen        m_quickMarkerPen;
    bool m_loopingMode;
    timeT m_startLoop;
    timeT m_endLoop;
};


}

#endif
