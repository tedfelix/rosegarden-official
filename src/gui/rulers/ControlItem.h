/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLITEM_H
#define RG_CONTROLITEM_H

#include <QPolygonF>
#include <QColor>
#include <map>
class QPainter;
class QMouseEvent;
class QWheelEvent;

namespace Rosegarden {

#define MIDI_CONTROL_MAX_VALUE 0x7F // Shouldnt be here

class ControlRuler;
//class ElementAdapter;
class Event;

class ControlItem : public QPolygonF
{
public:
    ControlItem(ControlRuler* controlRuler,
//                ElementAdapter* adapter,
                Event* event,
                QPolygonF polygon);

    virtual ~ControlItem();

    virtual void setValue(float);
    float y() const { return m_y; }
    QColor getColour() { return m_colour; }

    //void setWidth(int w)  { setSize(w, height()); }
    //void setHeight(int h) {
	//setSize(width(), h);
	//setZ(50.0+(h/2.0));
    //}
    //unsigned int getHeight()       { return size().height(); }

    virtual void draw(QPainter &painter);

    virtual double xStart() { return m_xstart; }
    virtual double xEnd() { return m_xend; }

    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);
    virtual void handleMouseWheel(QWheelEvent *e);

    virtual void setSelected(bool yes);
    bool isSelected() { return m_selected; }
    //    virtual void setHighlighted(bool yes) { m_highlighted=yes; update(); }
    /// recompute height according to represented value prior to a canvas repaint
    virtual void updateFromValue();

    /// update value according to height after a user edit
    virtual void updateSegment();

    virtual void update();

    virtual void setX(int);
    virtual void setWidth(int);

//    ElementAdapter* getElementAdapter() { return m_elementAdapter; }
    virtual Event* getEvent() { return m_event; }

protected:
    virtual void reconfigure();
    
    //--------------- Data members ---------------------------------

    QColor m_colour;
    double m_xstart;
    double m_xend;
    double m_lastxstart;
    float m_y;
    bool m_handlingMouseMove;
    bool m_selected;

    ControlRuler* m_controlRuler;
    Event* m_event;

    static const unsigned int BorderThickness;
    static const unsigned int DefaultWidth;
};

class ControlItemMap : public std::multimap<double, ControlItem*> {};
class ControlItemList : public std::list<ControlItem*> {};

}

#endif
