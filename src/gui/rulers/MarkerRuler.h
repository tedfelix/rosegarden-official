
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

#ifndef RG_MARKERRULER_H
#define RG_MARKERRULER_H

#include "gui/general/HZoomable.h"
#include "gui/general/ActionFileClient.h"
#include <QSize>
#include <QWidget>
#include "base/Event.h"


class QPaintEvent;
class QMouseEvent;
class QFont;
class QMenu;
class QMainWindow;

namespace Rosegarden
{

class Marker;
class RulerScale;
class RosegardenDocument;


class MarkerRuler : public QWidget, public HZoomable, public ActionFileClient
{
    Q_OBJECT

public:
    MarkerRuler(RosegardenDocument *doc,
                     RulerScale *rulerScale,
                     int buttonHeight,
                     double xorigin = 0.0,
                     QWidget* parent = 0,
                     const char* name = 0);
//                      WFlags f=0);

    virtual ~MarkerRuler();
    
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

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

    /// Set a loop range
    void setLoop(timeT, timeT);

protected slots:
    void slotInsertMarkerHere();
    void slotInsertMarkerAtPointer();
    void slotDeleteMarker();
    void slotEditMarker();
    
protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    void createMenu();
    timeT getClickPosition();
    Rosegarden::Marker* getMarkerAtClickPosition();
    
    //--------------- Data members ---------------------------------
    int m_barHeight;
    double m_xorigin;
    int m_currentXOffset;
    int m_width;
    int m_clickX;
    
    QFont 	*m_barFont;
    QMenu 	*m_menu;
    
    RosegardenDocument *m_doc;
    RulerScale *m_rulerScale;
    QMainWindow* m_parentMainWindow;

};


}

#endif
