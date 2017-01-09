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


#include <QGraphicsItem>
#include "NotationElement.h"
#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "base/Profiler.h"

#include <iostream>

namespace Rosegarden
{

static const int NotationElementData = 1;

NotationElement::NotationElement(Event *event) :
    ViewElement(event),
    m_recentlyRegenerated(false),
    m_isColliding(false),
    m_item(0),
    m_extraItems(0)
{
    //     NOTATION_DEBUG << "new NotationElement "
    //                          << this << " wrapping " << event << endl;
}

NotationElement::~NotationElement()
{
    removeItem();
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
NotationElement::getSceneX()
{
    if (m_item)
        return m_item->x();
    else {
        std::cerr << "ERROR: No scene item for this notation element:";
        event()->dump(std::cerr);
        throw NoGraphicsItem("No scene item for notation element of type " +
                             event()->getType(), __FILE__, __LINE__);
    }
}

double
NotationElement::getSceneY()
{
    if (m_item)
        return m_item->y();
    else {
        std::cerr << "ERROR: No scene item for this notation element:";
        event()->dump(std::cerr);
        throw NoGraphicsItem("No scene item for notation element of type " +
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
NotationElement::setItem(QGraphicsItem *e, double sceneX, double sceneY)
{
    Profiler p("NotationElement::setItem");
    removeItem();
    e->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    e->setData(NotationElementData, QVariant::fromValue((void *)this));
    e->setPos(sceneX, sceneY);
    m_recentlyRegenerated = true;
    m_item = e;
}

void
NotationElement::addItem(QGraphicsItem *e, double sceneX, double sceneY)
{
    Profiler p("NotationElement::addItem");

    if (!m_item) {
        std::cerr << "ERROR: Attempt to add extra scene item to element without main scene item:";
        event()->dump(std::cerr);
        throw NoGraphicsItem("No scene item for notation element of type " +
                             event()->getType(), __FILE__, __LINE__);
    }
    if (!m_extraItems) {
        m_extraItems = new ItemList;
    }
    e->setData(NotationElementData, QVariant::fromValue((void *)this));
    e->setPos(sceneX, sceneY);
    m_extraItems->push_back(e);
}

void
NotationElement::removeItem()
{
    Profiler p("NotationElement::removeItem");

    m_recentlyRegenerated = false;

//    NOTATION_DEBUG << "NotationElement::removeItem";

    delete m_item;
    m_item = 0;

    if (m_extraItems) {

        for (ItemList::iterator i = m_extraItems->begin();
             i != m_extraItems->end(); ++i) delete *i;
        m_extraItems->clear();

        delete m_extraItems;
        m_extraItems = 0;
    }
}

void
NotationElement::reposition(double sceneX, double sceneY)
{
    Profiler p("NotationElement::reposition");

    if (!m_item) return;
    if (sceneX == m_item->x() && sceneY == m_item->y()) return;

    m_recentlyRegenerated = false;

    double dx = sceneX - m_item->x();
    double dy = sceneY - m_item->y();
    m_item->setPos(sceneX, sceneY);

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
    return m_item ? m_item->isSelected() : false;
}

void
NotationElement::setSelected(bool selected)
{
    m_recentlyRegenerated = false;
    if (m_item) m_item->setSelected(selected);
}

NotationElement *
NotationElement::getNotationElement(QGraphicsItem *item)
{
    QVariant v = item->data(NotationElementData);
    if (v.isNull()) return 0;
    return (NotationElement *)v.value<void *>();
}

}
