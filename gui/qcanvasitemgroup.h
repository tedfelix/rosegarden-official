// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef QCANVASITEMGROUP_H
#define QCANVASITEMGROUP_H

#include <qcanvas.h>

/**
 * This class implements QCanvasItem groups
 *
 * An item group will keep its items in a fixed relative position when
 * moved, just like in a drawing program where you can "bind" several
 * items together so that they'll behave as a single item.
 *
 * Proper behavior requires collaboration from the QCanvasView,
 * though. When about to move an item, the QCanvasView object should
 * first check if it's not a groupable item, and if so fetch its
 * QCanvasItemGroup and move it instead.
 */
class QCanvasItemGroup : public QCanvasItem
{
public: 
    QCanvasItemGroup(QCanvas *);
    virtual ~QCanvasItemGroup();

    virtual void moveBy(double dx, double dy);
    virtual void advance(int stage);
    virtual bool collidesWith(const QCanvasItem*) const;
    virtual void draw(QPainter&);
    virtual void setVisible(bool yes);
    virtual void setSelected(bool yes);
    virtual void setEnabled(bool yes);
    virtual void setActive(bool yes);
    virtual int rtti() const;
    virtual QRect boundingRect() const;
    virtual QRect boundingRectAdvanced() const;

    /**
     * Add a new item to this group.
     *
     * The item's coordinates are kept as is.
     *
     * @see addItemWithRelativeCoords()
     */
    virtual void addItem(QCanvasItem *);

    /**
     * Add a new item to this group.
     *
     * The item's coordinates are considered relative to the group.
     * For example, suppose you have a QCanvasItemGroup which coords
     * are 10,10. If you call addItemWithRelativeCoords() with an item
     * which coords are 5,5, the item is moved so that it's coords
     * will be 5,5 relative to the group (e.g. 15,15).
     *
     * @see addItem()
     */
    virtual void addItemWithRelativeCoords(QCanvasItem *);

    /**
     * Remove the specified item from the group
     */
    virtual void removeItem(QCanvasItem*);

private:
    virtual bool collidesWith(const QCanvasSprite*,
                              const QCanvasPolygonalItem*,
                              const QCanvasRectangle*,
                              const QCanvasEllipse*,
                              const QCanvasText* ) const;

protected:
    QCanvasItemList m_items;
};

#endif
