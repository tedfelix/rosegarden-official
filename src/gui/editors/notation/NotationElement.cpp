/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[NotationElement]"
#define RG_NO_DEBUG_PRINT 1

#include <QGraphicsItem>
#include "NotationElement.h"
#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "base/Profiler.h"

namespace Rosegarden
{

static const int NotationElementData = 1;

NotationElement::NotationElement(Event *event, Segment *segment) :
    ViewElement(event),
    m_airX(0),
    m_airWidth(0),
    m_recentlyRegenerated(false),
    m_isColliding(false),
    m_item(nullptr),
    m_extraItems(nullptr),
    m_highlight(true),
    m_segment(segment)
{
    //RG_DEBUG << "ctor: " << this << " wrapping " << event;
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
        RG_WARNING << "getSceneX(): ERROR: No scene item for this notation element:";
        RG_WARNING << event();

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
        RG_WARNING << "getSceneY(): ERROR: No scene item for this notation element:";
        RG_WARNING << event();

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
    if (m_highlight) {
        m_item->setOpacity(1.0);
    } else {
        m_item->setOpacity(NONHIGHLIGHTOPACITY);
    }
}

void
NotationElement::addItem(QGraphicsItem *e, double sceneX, double sceneY)
{
    Profiler p("NotationElement::addItem");

    if (!m_item) {
        RG_WARNING << "addItem(): ERROR: Attempt to add extra scene item to element without main scene item:";
        RG_WARNING << event();

        throw NoGraphicsItem("No scene item for notation element of type " +
                             event()->getType(), __FILE__, __LINE__);
    }
    if (!m_extraItems) {
        m_extraItems = new ItemList;
    }
    e->setData(NotationElementData, QVariant::fromValue((void *)this));
    e->setPos(sceneX, sceneY);
    m_extraItems->push_back(e);
    setHighlight(m_highlight);
}

void
NotationElement::removeItem()
{
    Profiler p("NotationElement::removeItem");

    m_recentlyRegenerated = false;

    //RG_DEBUG << "removeItem()";

    delete m_item;
    m_item = nullptr;

    if (m_extraItems) {

        for (ItemList::iterator i = m_extraItems->begin();
             i != m_extraItems->end(); ++i) delete *i;
        m_extraItems->clear();

        delete m_extraItems;
        m_extraItems = nullptr;
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
    if (v.isNull()) return nullptr;
    return static_cast<NotationElement *>(v.value<void *>());
}

void NotationElement::setHighlight(bool highlight)
{
    //RG_DEBUG << "setHighlight" << highlight << m_highlight << m_item;
    if (highlight == m_highlight) return;
    m_highlight = highlight;
    if (! m_item) return;
    //RG_DEBUG << "set item opacity" << highlight << *event();
    if (highlight) {
        m_item->setOpacity(1.0);
    } else {
        m_item->setOpacity(NONHIGHLIGHTOPACITY);
    }
    if (m_extraItems) {
        for (ItemList::iterator i = m_extraItems->begin();
             i != m_extraItems->end(); ++i) {
            if (highlight) {
                (*i)->setOpacity(1.0);
            } else {
                (*i)->setOpacity(NONHIGHLIGHTOPACITY);
            }
        }
    }
}

}
