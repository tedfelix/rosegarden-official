// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _BARBUTTONS_H_
#define _BARBUTTONS_H_

#include <qvbox.h>
#include <qwidget.h>

#include "hzoomable.h"

#include "rosegardenguidoc.h"


namespace Rosegarden {
    class RulerScale;
}
class LoopRuler;
class RosegardenGUIDoc;
class BarButtonsWidget;

//!!! Bad names.  BarButtonsWidget should be MarkerRuler or some such.
// BarButtons... well, I'm not sure.

class BarButtons : public QVBox
{
    Q_OBJECT

public:
    BarButtons(RosegardenGUIDoc *doc,
               Rosegarden::RulerScale *rulerScale,
	       double xorigin,
               int buttonHeight,
	       bool invert = false, // draw upside-down
               QWidget* parent = 0,
               const char* name = 0,
               WFlags f=0);

    LoopRuler* getLoopRuler() { return m_loopRuler; }

    /**
     * Make connections from the LoopRuler to the document's
     * position pointer -- the standard use for a LoopRuler.
     * If you don't call this, you'll have to connect the
     * LoopRuler's signals up to something yourself.
     */
    void connectRulerToDocPointer(RosegardenGUIDoc *doc);
    
    void setMinimumWidth(int width);

    void setHScaleFactor(double dy);

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

private:
    //--------------- Data members ---------------------------------
    bool m_invert;
    int m_loopRulerHeight;
    int m_currentXOffset;

    RosegardenGUIDoc       *m_doc;
    Rosegarden::RulerScale *m_rulerScale;

    BarButtonsWidget *m_hButtonBar;
    LoopRuler *m_loopRuler;
};


class BarButtonsWidget : public QWidget, public HZoomable
{
    Q_OBJECT

public:
    BarButtonsWidget(RosegardenGUIDoc *doc,
                     Rosegarden::RulerScale *rulerScale,
                     int buttonHeight,
		     double xorigin = 0.0,
                     QWidget* parent = 0,
                     const char* name = 0,
                     WFlags f=0);

    virtual ~BarButtonsWidget();
    
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void scrollHoriz(int x);

    void setWidth(int width) { m_width = width; }

signals:
    /// Set the pointer position on mouse single click
    void setPointerPosition(Rosegarden::timeT);

    /// Open the marker editor window on double click
    void editMarkers();

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);

    //--------------- Data members ---------------------------------
    int m_barHeight;
    double m_xorigin;
    int m_currentXOffset;
    int m_width;

    QFont *m_barFont;

    RosegardenGUIDoc       *m_doc;
    Rosegarden::RulerScale *m_rulerScale;

};

#endif // _BARBUTTONS_H_
