// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef QCANVASGROUPABLEITEM_H
#define QCANVASGROUPABLEITEM_H

#include <Q3Canvas>
#include <Q3CanvasEllipse>
#include <Q3CanvasItem>
#include <Q3CanvasItemList>
#include <Q3CanvasLine>
#include <Q3CanvasPixmapArray>
#include <Q3CanvasPolygonalItem>
#include <Q3CanvasRectangle>
#include <Q3CanvasSprite>
#include <Q3CanvasText>
#include <Q3CanvasView>
#include <Q3Canvas>

class QCanvasItemGroup;

/**
 * This class is meant to be inherited by Q3CanvasItem children to make
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
    QCanvasGroupableItem(Q3CanvasItem*, QCanvasItemGroup*,
                         bool withRelativeCoords = false);

    virtual ~QCanvasGroupableItem();

    /// Returns the QCanvasItemGroup this groupable item belongs to
    QCanvasItemGroup* group() { return m_group; }

    /// Returns the QCanvasItemGroup this groupable item belongs to
    const QCanvasItemGroup* group() const { return m_group; }

    /// Returns the Q3CanvasItem which this groupable item wraps
    Q3CanvasItem *item() { return m_item; }

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
    Q3CanvasItem*      m_item;

};


/**
 * This class implements Q3CanvasItem groups
 *
 * An item group will keep its items in a fixed relative position when
 * moved, just like in a drawing program where you can "bind" several
 * items together so that they'll behave as a single item.
 *
 * Proper behavior requires collaboration from the Q3CanvasView,
 * though. When about to move an item, the Q3CanvasView object should
 * first check if it's not a groupable item, and if so fetch its
 * QCanvasItemGroup and move it instead.
 */
class QCanvasItemGroup : public Q3CanvasItem
{
public: 
    QCanvasItemGroup(Q3Canvas *);
    virtual ~QCanvasItemGroup();

    virtual void moveBy(double dx, double dy);
    virtual void advance(int stage);
    virtual bool collidesWith(const Q3CanvasItem*) const;
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
    virtual void addItem(Q3CanvasItem *);

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
    virtual void addItemWithRelativeCoords(Q3CanvasItem *);

    /**
     * Remove the specified item from the group
     */
    virtual void removeItem(Q3CanvasItem*);

private:
    virtual bool collidesWith(const Q3CanvasSprite*,
                              const Q3CanvasPolygonalItem*,
                              const Q3CanvasRectangle*,
                              const Q3CanvasEllipse*,
                              const Q3CanvasText* ) const;

protected:
    //--------------- Data members ---------------------------------

    Q3CanvasItemList m_items;
};


/**
 * A Q3CanvasLine which can be put in a QCanvasGroup
 */
class QCanvasLineGroupable : public Q3CanvasLine, public QCanvasGroupableItem
{
public: 
    QCanvasLineGroupable(Q3Canvas *c, QCanvasItemGroup *g);
};

/**
 * A Q3CanvasRectangle which can be put in a QCanvasGroup
 */
class QCanvasRectangleGroupable : public Q3CanvasRectangle, public QCanvasGroupableItem
{
public: 
    QCanvasRectangleGroupable(Q3Canvas *c, QCanvasItemGroup *g);
};

/**
 * A Q3CanvasText which can be put in a QCanvasGroup
 */
class QCanvasTextGroupable : public Q3CanvasText, public QCanvasGroupableItem
{
public: 
    QCanvasTextGroupable(Q3Canvas *c, QCanvasItemGroup *g);
    QCanvasTextGroupable(const QString&, Q3Canvas *c, QCanvasItemGroup *g);
};

/**
 * A Q3CanvasSprite that can be put in a QCanvasGroup
 */
class QCanvasSpriteGroupable : public Q3CanvasSprite, public QCanvasGroupableItem
{
public:
    QCanvasSpriteGroupable(Q3CanvasPixmapArray*,
                           Q3Canvas*,
                           QCanvasItemGroup*);
};

#endif
