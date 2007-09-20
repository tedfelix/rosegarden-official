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


#include "MatrixElement.h"
//#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "generalgui/GUIPalette.h"
#include "generalgui/DefaultVelocityColour.h"
#include <QBrush>
#include <QColor>


namespace Rosegarden
{

MatrixElement::MatrixElement(Event *event) :
        ViewElement(event)
{
    //     MATRIX_DEBUG << "new MatrixElement "
    //                          << this << " wrapping " << event << endl;
    setPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    colorFromVelocity();
}

MatrixElement::~MatrixElement()
{
    //     MATRIX_DEBUG << "MatrixElement " << this << "::~MatrixElement() wrapping "
    //                          << event() << endl;

}

bool MatrixElement::isNote() const
{
    return event()->isa(Note::EventType);
}


bool MatrixElement::getVisibleRectangle(QRectF &rectangle)
{
    if (isVisible()) {
        rectangle = rect();
        return true;
    }
    return false;
}

QVariant MatrixElement::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedChange && scene()) {
        // set brush color according to selected state or velocity
        //
        if (value.toBool() == true) {
            setBrush(QBrush(GUIPalette::getColour(GUIPalette::SelectedElement)));
        } else {
            colorFromVelocity();
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void MatrixElement::colorFromVelocity()
{
    // Get velocity for colouring
    //
    using Rosegarden::BaseProperties::VELOCITY;
    long velocity = 127;
    if (event()->has(VELOCITY))
        event()->get
        <Int>(VELOCITY, velocity);

    setBrush(QBrush(DefaultVelocityColour::getInstance()->getColour(velocity)));    
}

}
