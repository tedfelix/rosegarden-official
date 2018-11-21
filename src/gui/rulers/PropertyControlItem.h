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

#ifndef RG_PROPERTYCONTROLITEM_H
#define RG_PROPERTYCONTROLITEM_H

#include "base/ViewElement.h"
#include "gui/rulers/ControlItem.h"

#define PROPERTYCONTROLITEM_NOTATION_ITEMWIDTH 20

namespace Rosegarden {

class ControlRuler;
class PropertyName;
//class MatrixElement;

class PropertyControlItem : public ControlItem
{
public:
    PropertyControlItem(ControlRuler* controlRuler,
                PropertyName propertyname,
                ViewElement* element,
                QPolygonF polygon);

    ~PropertyControlItem() override;

    void update() override;
    ViewElement* getElement() { return m_element; }

    void setValue(float y) override;
    void updateSegment() override;

protected:
    using ControlItem::reconfigure;
    void reconfigure(float x0,float x1,float y);

    //--------------- Data members ---------------------------------
    ViewElement* m_element;
    PropertyName m_propertyname;
};

}

#endif
