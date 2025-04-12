
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#include "gui/general/ActionFileClient.h"
#include <QSize>
#include <QWidget>
#include "base/Event.h"


class QPaintEvent;
class QMouseEvent;
class QMenu;
class QMainWindow;

namespace Rosegarden
{


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
                     QWidget* parent = nullptr,
                     const char* name = nullptr);

    ~MarkerRuler() override;
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void scrollHoriz(int x);

    void setWidth(int width) { m_width = width; }

signals:
    /// Set the pointer position on mouse single click
    void setPointerPosition(timeT);

    /// Open the marker editor window on double click
    void editMarkers();

    /// add a marker
    void addMarker(timeT);
    
    void deleteMarker(int, timeT, QString name, QString description);

protected slots:
    void slotInsertMarkerHere();
    void slotInsertMarkerAtPointer();
    void slotDeleteMarker();
    void slotEditMarker();
    
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

    void createMenu();
    timeT getClickPosition();
    Rosegarden::Marker* getMarkerAtClickPosition();
    
    //--------------- Data members ---------------------------------
    int m_currentXOffset;
    int m_width;
    int m_clickX;
    
    QMenu 	*m_menu;
    
    RosegardenDocument *m_doc;
    RulerScale *m_rulerScale;
    QMainWindow* m_parentMainWindow;

};


}

#endif
