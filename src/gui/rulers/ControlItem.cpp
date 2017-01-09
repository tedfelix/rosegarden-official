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

#define RG_MODULE_STRING "[ControlItem]"

#include "ControlItem.h"
#include "ControlRuler.h"
#include "ElementAdapter.h"
//#include "misc/Debug.h"

namespace Rosegarden {

const unsigned int ControlItem::BorderThickness = 1;
const unsigned int ControlItem::DefaultWidth    = 20;

ControlItem::ControlItem(ControlRuler *controlRuler,
        Event *event,
        QPolygonF polygon)
: QPolygonF(polygon),
    m_xstart(-1.0),
    m_y(0),
    m_selected(false),
    m_controlRuler(controlRuler),
//    m_elementAdapter(elementAdapter),
    m_event(event)
{
}

ControlItem::~ControlItem()
{
//    delete m_elementAdapter;
}

void ControlItem::setValue(float v)
{
//     RG_DEBUG << "ControlItem::setValue(" << v << ") x = " << x();

    m_y = v;
}

void ControlItem::updateSegment()
{
}

void ControlItem::updateFromValue()
{
}

void ControlItem::draw(QPainter &/* painter */)
{
}

void ControlItem::handleMouseButtonPress(QMouseEvent*)
{
}

void ControlItem::handleMouseButtonRelease(QMouseEvent*)
{
}

void ControlItem::handleMouseMove(QMouseEvent*, int /*deltaX*/, int /* deltaY */)
{
}

void ControlItem::handleMouseWheel(QWheelEvent *)
{
}

void ControlItem::setSelected(bool s)
{
    m_selected = s;
}

void ControlItem::update()
{
}

void ControlItem::reconfigure()
{
    if (m_xstart != m_lastxstart) {
        m_controlRuler->moveItem(this);
    }
    
    m_lastxstart = m_xstart;
}

void ControlItem::setX(int /* x */)
{
}

void ControlItem::setWidth(int /* width */)
{
}

}
