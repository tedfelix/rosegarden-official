// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#ifndef QCANVASGROUPABLEITEM_H
#define QCANVASGROUPABLEITEM_H

#include <qcanvas.h>

/**
 * This class is meant to be inherited by QCanvasItem children to make
 * them groupable.
 *
 * On destruction, the item will remove itself from the group it's
 * attached to.
 *
 * @see QCanvasSpriteGroupable
 * @see QCanvasLineGroupable
 */
class QCanvasGroupableItem
{
    friend class QCanvasItemGroup;

public:

    /**
     * Create a groupable item, e.g. put the item in the specified
     * QCanvasItemGroup. If withRelativeCoords is true, the item's
     * position will be translated so that it's coordinates are
     * relative to those of the item group.
     *
     * @see QCanvasItemGroup#addItemWithRelativeCoords()
     */
    QCanvasGroupableItem(QCanvasItem*, QCanvasItemGroup*,
                         bool withRelativeCoords = false);

    virtual ~QCanvasGroupableItem();

    /// Returns the QCanvasItemGroup this groupable item belongs to
    QCanvasItemGroup* group() { return m_group; }

    /// Returns the QCanvasItemGroup this groupable item belongs to
    const QCanvasItemGroup* group() const { return m_group; }

    /// Returns the QCanvasItem which this groupable item wraps
    QCanvasItem *item() { return m_item; }

    /**
     * Same as moveBy(), except that the move is done relative to the
     * item group's coordinates
     */
    virtual void relativeMoveBy(double dx, double dy);

protected:
    /**
     * Detach item from the item group - called by QCanvasItemGroup only
     *
     * Set m_group to 0, so that on destruction the item won't try to
     * remove itself from the group
     */
    void detach();
    
private:
    //--------------- Data members ---------------------------------

    QCanvasItemGroup* m_group;
    QCanvasItem*      m_item;

};


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
     * For example, suppose you have a QCanvasItemGroup whose coords
     * are 10,10. If you call addItemWithRelativeCoords() with an item
     * whose coords are 5,5, the item is moved so that its coords
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
    //--------------- Data members ---------------------------------

    QCanvasItemList m_items;
};


/**
 * A QCanvasLine which can be put in a QCanvasGroup
 */
class QCanvasLineGroupable : public QCanvasLine, public QCanvasGroupableItem
{
public: 
    QCanvasLineGroupable(QCanvas *c, QCanvasItemGroup *g);
};

/**
 * A QCanvasRectangle which can be put in a QCanvasGroup
 */
class QCanvasRectangleGroupable : public QCanvasRectangle, public QCanvasGroupableItem
{
public: 
    QCanvasRectangleGroupable(QCanvas *c, QCanvasItemGroup *g);
};

/**
 * A QCanvasText which can be put in a QCanvasGroup
 */
class QCanvasTextGroupable : public QCanvasText, public QCanvasGroupableItem
{
public: 
    QCanvasTextGroupable(QCanvas *c, QCanvasItemGroup *g);
    QCanvasTextGroupable(const QString&, QCanvas *c, QCanvasItemGroup *g);
};

/**
 * A QCanvasSprite that can be put in a QCanvasGroup
 */
class QCanvasSpriteGroupable : public QCanvasSprite, public QCanvasGroupableItem
{
public:
    QCanvasSpriteGroupable(QCanvasPixmapArray*,
                           QCanvas*,
                           QCanvasItemGroup*);
};

#endif
