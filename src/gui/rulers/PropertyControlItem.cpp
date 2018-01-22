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

#include "PropertyControlItem.h"
#include "ControlRuler.h"
#include "gui/editors/matrix/MatrixElement.h"
#include "gui/rulers/DefaultVelocityColour.h"
#include "base/ViewElement.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"

namespace Rosegarden {

PropertyControlItem::PropertyControlItem(ControlRuler *controlRuler,
        PropertyName propertyname,
        ViewElement *element,
        QPolygonF polygon)
: ControlItem(controlRuler,element->event(),polygon),
    m_element(element),
    m_propertyname(propertyname)
{
//    element->addObserver(this);
}

PropertyControlItem::~PropertyControlItem()
{
    if (m_element) {
//        m_element->removeObserver(this);
    }
}

void PropertyControlItem::update()
{
    if (!m_element) return;
    // RulerScale *rulerScale = m_controlRuler->getRulerScale();    
    double x0,x1;

    long val = 0;    
    MatrixElement *matrixelement = dynamic_cast<MatrixElement*>(m_element);
    if (matrixelement) {
        // Guarantee that matrixelement is up to date, otherwise data
        // can be out of date, velocity especially.
        matrixelement->reconfigure();
        x0 = matrixelement->getLayoutX();
        x1 = matrixelement->getWidth() + x0;
        val = matrixelement->getElementVelocity();
    } else {
        x0 = m_element->getLayoutX();
        x1 = x0 + PROPERTYCONTROLITEM_NOTATION_ITEMWIDTH;
        m_element->event()->get<Rosegarden::Int>(m_propertyname, val);
    }

    if (m_propertyname == BaseProperties::VELOCITY) {
        m_colour = DefaultVelocityColour::getInstance()->getColour(val);
    }
    
    m_y = m_controlRuler->valueToY(val);

    reconfigure(x0,x1,m_y);
}

void PropertyControlItem::setValue(float y)
{
    if (y > 1.0) y = 1.0;
    if (y < 0) y = 0;

    if (m_propertyname == BaseProperties::VELOCITY) {
        MatrixElement *matrixelement = dynamic_cast<MatrixElement*> (m_element);       
        if (matrixelement) {        
            matrixelement->reconfigure(m_controlRuler->yToValue(y));
            matrixelement->setSelected(true);
        }
        
        m_colour = DefaultVelocityColour::getInstance()->getColour(
                m_controlRuler->yToValue(y));
    }

    m_y = y;
    float x0 = boundingRect().left();
    float x1 = boundingRect().right();
    reconfigure(x0,x1,y);
}

void PropertyControlItem::reconfigure(float x0, float x1, float y)
{
    QPolygonF newpoly;
    newpoly << QPointF(x0,0) << QPointF(x0,y) << QPointF(x1,y) << QPointF(x1,0);
//    QPolygonF newpoly(QRectF(x0,0,x1-x0,y));
    this->clear();
    *this << newpoly;

    // Record the extent of the item (polygon) during update for speed
    m_xend = boundingRect().right();
    // If this is a true reconfigure (and not part of an item creation)
    // m_xstart will be positive or zero and we should move it in the
    // ControlItemMap
    if (m_xstart != -1.0 && m_xstart != boundingRect().left()) {
        m_xstart = boundingRect().left();
        m_controlRuler->moveItem(this);
    } else {
        m_xstart = boundingRect().left();
    }

//    m_controlRuler->update();
    ControlItem::reconfigure();
}

void PropertyControlItem::updateSegment()
{
    m_element->event()->set<Int>(m_propertyname,(int)(m_controlRuler->yToValue(m_y)));
}

}
