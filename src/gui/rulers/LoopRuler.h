
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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
//#include "base/Event.h"

#include <QSize>
#include <QWidget>
#include <QPen>

class QPaintEvent;
class QPainter;
class QMouseEvent;


namespace Rosegarden
{


class RulerScale;
class RosegardenDocument;


/// The ruler that shows the the beat ticks and the loop range.
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
              int height,
              bool invert,
              bool displayQuickMarker,
              QWidget *parent);
    ~LoopRuler() override;

    void setSnapGrid(const SnapGrid *grid);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void scrollHoriz(int x);

    void setMinimumWidth(int width) { m_width = width; }
    
signals:

    /// Set the pointer position on mouse single click
    /**
     * StandardRuler connects this to
     * RosegardenDocument::slotSetPointerPosition().
     *
     * ??? Call into m_doc directly and get rid of this signal.
     */
    void setPointerPosition(timeT);

    /// Set the pointer position on mouse drag
    /**
     * This goes through StandardRuler to the three main windows: Notation,
     * Matrix, Segment Canvas.
     */
    void dragPointerToPosition(timeT);

    /// Set pointer position and start playing on double click
    /**
     * Connected to RosegardenMainWindow::slotSetPlayPosition() by
     * StandardRuler.
     */
    void setPlayPosition(timeT);

    // These signals are used by Notation, Matrix, and the Segment Canvas
    // for auto scroll.
    void startMouseMove(int directionConstraint);
    void stopMouseMove();

protected:

    // QWidget overrides
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent *) override;

private slots:

    /// RosegardenDocument::loopChanged() handler.
    void slotLoopChanged();

private:

    RosegardenDocument *m_doc;

    /**
     * If true, the quick marker (Shift+Ctrl+M) position will be displayed
     * as a red line.
     */
    bool m_displayQuickMarker;
    QPen m_quickMarkerPen;

    // Used by sizeHint().
    int m_width = -1;
    int m_height;

    /// Invert drawing for the bottom ruler.
    bool m_invert;

    /// Negative scroll position.
    int m_currentXOffset;
    double mouseEventToSceneX(QMouseEvent *mouseEvent);

    // Remember the mouse x pos in mousePressEvent() and in mouseMoveEvent() so
    // that we can emit it in mouseReleaseEvent() to update the pointer position
    // in other views.
    double m_lastMouseXPos = 0;

    RulerScale *m_rulerScale;
    void drawBarSections(QPainter *);

    SnapGrid m_defaultGrid;
    SnapGrid *m_loopGrid;
    const SnapGrid *m_grid;

    /// Whether we are dragging and drawing a loop.
    bool m_loopDrag = false;

    /// The start of the loop while dragging.
    timeT m_startDrag = 0;
    /// The end of the loop while dragging.
    timeT m_endDrag = 0;

    /// between loop positions
    void drawLoopMarker(QPainter *);

};


}

#endif
