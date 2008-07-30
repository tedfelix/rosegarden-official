/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qcanvas.h>

namespace Rosegarden {

class ControlRuler;
class ElementAdapter;
        
class ControlItem : public QCanvasRectangle
{
public:
    ControlItem(ControlRuler* controlRuler,
                ElementAdapter* adapter,
                int x, int width = DefaultWidth);

    ~ControlItem();
    
    virtual void setValue(long);
    int getValue() const { return m_value; }

    void setWidth(int w)  { setSize(w, height()); }
    void setHeight(int h) { setSize(width(), h); }
    int getHeight()       { return size().height(); }

    virtual void draw(QPainter &painter);

    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);
    virtual void handleMouseWheel(QWheelEvent *e);

    virtual void setSelected(bool yes);

    /// recompute height according to represented value prior to a canvas repaint
    virtual void updateFromValue();

    /// update value according to height after a user edit
    virtual void updateValue();

    ElementAdapter* getElementAdapter() { return m_elementAdapter; }

protected:

    //--------------- Data members ---------------------------------

    long m_value;
    bool m_handlingMouseMove;

    ControlRuler* m_controlRuler;
    ElementAdapter* m_elementAdapter;

    static const unsigned int BorderThickness;
    static const unsigned int DefaultWidth;
};

}
