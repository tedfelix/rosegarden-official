
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MARKERRULER_H_
#define _RG_MARKERRULER_H_

#include "gui/general/HZoomable.h"
#include <qsize.h>
#include <qwidget.h>
#include <kxmlguiclient.h>
#include "base/Event.h"


class QPaintEvent;
class QMouseEvent;
class QFont;
class QPopupMenu;
class KMainWindow;

namespace Rosegarden
{

class Marker;
class RulerScale;
class RosegardenGUIDoc;


class MarkerRuler : public QWidget, public HZoomable, public KXMLGUIClient
{
    Q_OBJECT

public:
    MarkerRuler(RosegardenGUIDoc *doc,
                     RulerScale *rulerScale,
                     int buttonHeight,
                     double xorigin = 0.0,
                     QWidget* parent = 0,
                     const char* name = 0,
                     WFlags f=0);

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
    
    void deleteMarker(timeT, QString name, QString description);

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
    
    QFont *m_barFont;
    QPopupMenu *m_menu;
    
    RosegardenGUIDoc *m_doc;
    RulerScale *m_rulerScale;
    KMainWindow* m_parentMainWindow;

};


}

#endif
