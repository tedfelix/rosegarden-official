/***************************************************************************
                          qcanvasgroupableitem.h  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QCANVASGROUPABLEITEM_H
#define QCANVASGROUPABLEITEM_H

/**
 * This class is meant to be inherited by QCanvasItem children to make
 * them groupable.

 @see QCanvasSpriteGroupable
 @see QCanvasLineGroupable

* @author Guillaume Laurent
*/

class QCanvasItemGroup;
class QCanvasItem;

class QCanvasGroupableItem {
    friend QCanvasItemGroup;

public:

    /**
     * Create a groupable item, e.g. put the item in the specified
     * QCanvasItemGroup. If withRelativeCoords is true, the item's
     * position will be translated so that it's coordinates are
     * relative to those of the item group.
     *
     * @ see QCanvasItemGroup::addItemWithRelativeCoords()
     */
    QCanvasGroupableItem(QCanvasItem*, QCanvasItemGroup*,
                         bool withRelativeCoords = false);

    virtual ~QCanvasGroupableItem();

    QCanvasItemGroup* group() { return m_group; }
    QCanvasItem*      item()  { return m_item; }

    /** same as moveBy(), except that the move is done relative to the
       item group's coordinates
    */
    virtual void relativeMoveBy(double dx, double dy);

protected:
    /**
     * detach item from the item group - called by QCanvasItemGroup only
     *
     * set m_group to 0, so that on desctruction the item won't try to
     * remove itself from the group
     */
    void detach();
    
private:
    QCanvasItemGroup* m_group;
    QCanvasItem*      m_item;

};

#endif
