// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <qcanvas.h>
#include <algorithm>

#include "notationelement.h"
#include "notationproperties.h"
#include "rosestrings.h"
#include "rosedebug.h"

#include "NotationTypes.h"
#include "BaseProperties.h"
#include "Event.h"
#include "Quantizer.h"

#include <kmessagebox.h>

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::timeT;

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

Rosegarden::timeT
NotationElement::getViewAbsoluteTime() const
{
    return event()->getNotationAbsoluteTime();
}

Rosegarden::timeT
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
    return event()->has(Rosegarden::BaseProperties::BEAMED_GROUP_TUPLET_BASE);
}

bool
NotationElement::isGrace() const
{
    return event()->has(Rosegarden::BaseProperties::IS_GRACE_NOTE) &&
	event()->get<Bool>(Rosegarden::BaseProperties::IS_GRACE_NOTE);
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
	     i != m_extraItems->end(); ++i) delete *i;
	m_extraItems->clear();

	delete m_extraItems;
	m_extraItems = 0;
    }
}

void
NotationElement::reposition(double canvasX, double canvasY)
{
    m_recentlyRegenerated = false;
    if (!m_canvasItem) return;

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
    return m_canvasItem ?  m_canvasItem->selected() : false;
}

void
NotationElement::setSelected(bool selected)
{
    m_recentlyRegenerated = false;
    if (m_canvasItem) m_canvasItem->setSelected(selected);
}

