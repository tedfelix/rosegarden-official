// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "qcanvasgroupableitem.h"
#include "rosedebug.h"

QCanvasGroupableItem::QCanvasGroupableItem(QCanvasItem *i,
                                           QCanvasItemGroup *g,
                                           bool withRelativeCoords)
    : m_group(g),
      m_item(i)
{
//     kdDebug(KDEBUG_AREA) << "QCanvasGroupableItem() - this : " << this
//                          << " - group : " << g
//                          << " - item : " << i << endl;

    if (withRelativeCoords)
        group()->addItemWithRelativeCoords(item());
    else
        group()->addItem(item());
}

QCanvasGroupableItem::~QCanvasGroupableItem()
{
//     kdDebug(KDEBUG_AREA) << "~QCanvasGroupableItem() - this : " << this
//                          << " - group : " << group()
//                          << " - item : " << item() << endl;

    // remove from the item group if we're still attached to one
    if (group())
        group()->removeItem(item());
}

void
QCanvasGroupableItem::relativeMoveBy(double dx, double dy)
{
    m_item->moveBy(dx + m_group->x(),
                   dy + m_group->y());
}

void
QCanvasGroupableItem::detach()
{
    m_group = 0;
}

//////////////////////////////////////////////////////////////////////

QCanvasItemGroup::QCanvasItemGroup(QCanvas *c)
    : QCanvasItem(c)
{
//     kdDebug(KDEBUG_AREA) << "QCanvasItemGroup() - this : " << this << endl;
}

QCanvasItemGroup::~QCanvasItemGroup()
{
//     kdDebug(KDEBUG_AREA) << "~QCanvasItemGroup() - this : " << this << endl;

    // Tell all our items that we're being destroyed
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i) {
        QCanvasGroupableItem *t = dynamic_cast<QCanvasGroupableItem*>(*i);
        if (t) t->detach();
    }
}

void
QCanvasItemGroup::moveBy(double dx, double dy)
{
    QCanvasItem::moveBy(dx, dy); // move ourselves

    QCanvasItemList::Iterator i; // move group items
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->moveBy(dx, dy);
}

void
QCanvasItemGroup::advance(int stage)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->advance(stage);
}

bool
QCanvasItemGroup::collidesWith(const QCanvasItem *item) const
{
    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        if ((*i)->collidesWith(item)) return true;

    return false;
}

void
QCanvasItemGroup::draw(QPainter&)
{
    // There isn't anything to do - all the items will be drawn
    // seperately by the canvas anyway. However the function has to be
    // implemented because it's an abstract virtual in QCanvasItem.

//     QCanvasItemList::Iterator i;
//     for(i = m_items.begin(); i != m_items.end(); ++i)
//         (*i)->draw(p);
}

void
QCanvasItemGroup::setVisible(bool yes)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setSelected(bool yes)
{
    QCanvasItem::setSelected(yes);

    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setEnabled(bool yes)
{
    QCanvasItem::setEnabled(yes);

    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setEnabled(yes);
}

void
QCanvasItemGroup::setActive(bool yes)
{
    QCanvasItem::setActive(yes);

    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setActive(yes);
}

int
QCanvasItemGroup::rtti() const
{
    return 10002;
}

QRect
QCanvasItemGroup::boundingRect() const
{
    QRect r;

    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRect());

    return r;
}

QRect
QCanvasItemGroup::boundingRectAdvanced() const
{
    QRect r;

    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRectAdvanced());

    return r;
}

bool
QCanvasItemGroup::collidesWith(const QCanvasSprite *s,
                               const QCanvasPolygonalItem *p,
                               const QCanvasRectangle *r,
                               const QCanvasEllipse *e,
                               const QCanvasText *t) const
{
    if (s) return collidesWith(s);
    else if(p) return collidesWith(p);
    else if(r) return collidesWith(r);
    else if(e) return collidesWith(e);
    else if(t) return collidesWith(t);

    return false;

}

void
QCanvasItemGroup::addItem(QCanvasItem *i)
{
    m_items.append(i);
}

void
QCanvasItemGroup::addItemWithRelativeCoords(QCanvasItem *i)
{
    i->moveBy(x(), y());
    addItem(i);
}

void
QCanvasItemGroup::removeItem(QCanvasItem *i)
{
//     kdDebug(KDEBUG_AREA) << "QCanvasItemGroup::removeItem() - this : "
//                          << this << " - item : " << i << endl;
    m_items.remove(i);
}

//////////////////////////////////////////////////////////////////////


QCanvasLineGroupable::QCanvasLineGroupable(QCanvas *c,
                                           QCanvasItemGroup *g)
    : QCanvasLine(c),
      QCanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasRectangleGroupable::QCanvasRectangleGroupable(QCanvas *c,
                                                     QCanvasItemGroup *g)
    : QCanvasRectangle(c),
      QCanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasTextGroupable::QCanvasTextGroupable(const QString& label,
                                           QCanvas *c,
                                           QCanvasItemGroup *g)
    : QCanvasText(label, c),
      QCanvasGroupableItem(this, g)
{}

QCanvasTextGroupable::QCanvasTextGroupable(QCanvas *c,
                                           QCanvasItemGroup *g)
    : QCanvasText(c),
      QCanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasSpriteGroupable::QCanvasSpriteGroupable(QCanvasPixmapArray *pa,
                                               QCanvas *c,
                                               QCanvasItemGroup *g)
    : QCanvasSprite(pa, c),
      QCanvasGroupableItem(this, g)
{}
