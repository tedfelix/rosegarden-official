/***************************************************************************
                          viewelementsmanager.h  -  description
                             -------------------
    begin                : Sun Sep 10 2000
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

#ifndef VIEWELEMENTSMANAGER_H
#define VIEWELEMENTSMANAGER_H

#include "Element2.h"

class NotationElement;

typedef list<NotationElement*> NotationElementList;

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class ViewElementsManager
{
public: 
    ViewElementsManager();
    ~ViewElementsManager();

    static NotationElementList* notationElementList(EventList::iterator from,
                                                    EventList::iterator to);
    
};

#endif
