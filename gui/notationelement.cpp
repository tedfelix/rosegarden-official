// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
      m_x(0),
      m_y(0),
      m_recentlyRegenerated(false),
      m_canvasItem(0)
{
//     NOTATION_DEBUG << "new NotationElement "
//                          << this << " wrapping " << event << endl;
}

NotationElement::~NotationElement()
{
//     NOTATION_DEBUG << "NotationElement " << this << "::~NotationElement() wrapping "
//                          << event() << endl;

    delete m_canvasItem;
}

Rosegarden::timeT
NotationElement::getViewAbsoluteTime() const
{
    //!!! nasty, this is for test purposes only
    static Rosegarden::NotationQuantizer *q = 0;
    if (!q) q = new Rosegarden::NotationQuantizer();
    return q->getQuantizedAbsoluteTime(event());
}

Rosegarden::timeT
NotationElement::getViewDuration() const
{
    //!!! nasty, this is for test purposes only
    static Rosegarden::NotationQuantizer *q = 0;
    if (!q) q = new Rosegarden::NotationQuantizer();
    return q->getQuantizedDuration(event());
}

double
NotationElement::getCanvasX() 
{
    if (m_canvasItem)
        return m_canvasItem->x();
    else
        throw NoCanvasItem("No canvas item for notation element of type " +
			   event()->getType(), __FILE__, __LINE__);
}

double
NotationElement::getCanvasY()
{
    if (m_canvasItem)
        return m_canvasItem->y();
    else
        throw NoCanvasItem("No canvas item for notation element of type " +
			   event()->getType(), __FILE__, __LINE__);
}

bool
NotationElement::isRest() const
{
    return event()->isa("rest");
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
NotationElement::setCanvasItem(QCanvasItem *e, double dxoffset, double dyoffset)
{
    m_recentlyRegenerated = true;
    delete m_canvasItem;
    m_canvasItem = e;
    e->move(m_x + dxoffset, m_y + dyoffset);
}

void
NotationElement::removeCanvasItem()
{
    m_recentlyRegenerated = false;
    delete m_canvasItem;
    m_canvasItem = 0;
}

void
NotationElement::reposition(double dxoffset, double dyoffset)
{
    m_recentlyRegenerated = false;
    if (!m_canvasItem) return;
    m_canvasItem->move(m_x + dxoffset, m_y + dyoffset);
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

