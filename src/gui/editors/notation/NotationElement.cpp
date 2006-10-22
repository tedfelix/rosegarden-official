/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "NotationElement.h"
#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"

#include <qcanvas.h>

namespace Rosegarden
{

NotationElement::NotationElement(Event *event)
        : ViewElement(event),
        m_recentlyRegenerated(false),
        m_canvasItem(0),
        m_extraItems(0)
{
    //     NOTATION_DEBUG << "new NotationElement "
    //                          << this << " wrapping " << event << endl;
}

NotationElement::~NotationElement()
{
    removeCanvasItem();
}

timeT
NotationElement::getViewAbsoluteTime() const
{
    return event()->getNotationAbsoluteTime();
}

timeT
NotationElement::getViewDuration() const
{
    return event()->getNotationDuration();
}

double
NotationElement::getCanvasX()
{
    if (m_canvasItem)
        return m_canvasItem->x();
    else {
        std::cerr << "ERROR: No canvas item for this notation element:";
        event()->dump(std::cerr);
        throw NoCanvasItem("No canvas item for notation element of type " +
                           event()->getType(), __FILE__, __LINE__);
    }
}

double
NotationElement::getCanvasY()
{
    if (m_canvasItem)
        return m_canvasItem->y();
    else {
        std::cerr << "ERROR: No canvas item for this notation element:";
        event()->dump(std::cerr);
        throw NoCanvasItem("No canvas item for notation element of type " +
                           event()->getType(), __FILE__, __LINE__);
    }
}

bool
NotationElement::isRest() const
{
    return event()->isa(Note::EventRestType);
}

bool
NotationElement::isNote() const
{
    return event()->isa(Note::EventType);
}

bool
NotationElement::isTuplet() const
{
    return event()->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE);
}

bool
NotationElement::isGrace() const
{
    return event()->has(BaseProperties::IS_GRACE_NOTE) &&
           event()->get
           <Bool>(BaseProperties::IS_GRACE_NOTE);
}

void
NotationElement::setCanvasItem(QCanvasItem *e, double canvasX, double canvasY)
{
    removeCanvasItem();
    m_recentlyRegenerated = true;
    m_canvasItem = e;
    e->move(canvasX, canvasY);
}

void
NotationElement::addCanvasItem(QCanvasItem *e, double canvasX, double canvasY)
{
    if (!m_canvasItem) {
        std::cerr << "ERROR: Attempt to add extra canvas item to element without main canvas item:";
        event()->dump(std::cerr);
        throw NoCanvasItem("No canvas item for notation element of type " +
                           event()->getType(), __FILE__, __LINE__);
    }
    if (!m_extraItems) {
        m_extraItems = new ItemList;
    }
    e->move(canvasX, canvasY);
    m_extraItems->push_back(e);
}

void
NotationElement::removeCanvasItem()
{
    m_recentlyRegenerated = false;

    delete m_canvasItem;
    m_canvasItem = 0;

    if (m_extraItems) {

        for (ItemList::iterator i = m_extraItems->begin();
                i != m_extraItems->end(); ++i)
            delete *i;
        m_extraItems->clear();

        delete m_extraItems;
        m_extraItems = 0;
    }
}

void
NotationElement::reposition(double canvasX, double canvasY)
{
    m_recentlyRegenerated = false;
    if (!m_canvasItem)
        return ;

    double dx = canvasX - m_canvasItem->x();
    double dy = canvasY - m_canvasItem->y();
    m_canvasItem->move(canvasX, canvasY);

    if (m_extraItems) {
        for (ItemList::iterator i = m_extraItems->begin();
                i != m_extraItems->end(); ++i) {
            (*i)->moveBy(dx, dy);
        }
    }
}

bool
NotationElement::isSelected()
{
    return m_canvasItem ? m_canvasItem->selected() : false;
}

void
NotationElement::setSelected(bool selected)
{
    m_recentlyRegenerated = false;
    if (m_canvasItem)
        m_canvasItem->setSelected(selected);
}

}
