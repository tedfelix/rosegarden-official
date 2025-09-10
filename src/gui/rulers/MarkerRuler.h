
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKERRULER_H
#define RG_MARKERRULER_H

#include "base/TimeT.h"
#include "gui/general/ActionFileClient.h"

#include <QPointer>
#include <QSize>
#include <QWidget>

class QPaintEvent;
class QMouseEvent;
class QMenu;


namespace Rosegarden
{


class AutoScroller;
class Marker;
class RulerScale;
class RosegardenDocument;


/// The ruler that shows the bar numbers and the markers.
class MarkerRuler : public QWidget, public ActionFileClient
{
    Q_OBJECT

public:
    MarkerRuler(RosegardenDocument *doc,
                RulerScale *rulerScale,
                QWidget *parent);
    void setAutoScroller(QPointer<AutoScroller> autoScroller)
            { m_autoScroller = autoScroller; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void scrollHoriz(int x);

    void setWidth(int width) { m_width = width; }

protected:

    void paintEvent(QPaintEvent *paintEvent) override;
    void mousePressEvent(QMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QMouseEvent *mouseEvent) override;
    void mouseDoubleClickEvent(QMouseEvent *mouseEvent) override;

private slots:

    void slotInsertMarkerHere();
    void slotInsertMarkerAtPointer();
    void slotDeleteMarker();
    void slotRenameMarker();
    void slotEditMarker();
    void slotManageMarkers();
    
private:

    RosegardenDocument *m_doc;
    
    /// Horizontal scroll offset.  E.g. -100 if we are scrolled right 100 pixels.
    int m_currentXOffset{0};
    int m_width{-1};

    /// Click position.  Set by mousePressEvent().
    int m_clickX{0};
    /// Returns the Marker at m_clickX if there is one.
    Marker *getMarkerAtClickPosition(int *clickDelta = nullptr);
    
    bool m_leftPressed{false};

    /// Storing an ID instead of a pointer for safety.
    int m_dragMarkerID{-1};
    bool m_dragging{false};
    /// Maintains the grab point.
    /**
     * Take into account click position relative to the marker start so if
     * e.g. we grab the marker at its right edge, we hold the marker at its
     * right edge throughout the drag.
     */
    int m_dragClickDelta{0};
    timeT m_dragTime{0};

    QMenu *m_menu{nullptr};
    void createMenu();
    
    RulerScale *m_rulerScale;

    QPointer<AutoScroller> m_autoScroller;

};


}

#endif
