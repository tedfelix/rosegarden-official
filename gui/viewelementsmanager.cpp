/***************************************************************************
                          viewelementsmanager.cpp  -  description
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

#include "viewelementsmanager.h"
#include "notationelement.h"

ViewElementsManager::ViewElementsManager()
{
}

ViewElementsManager::~ViewElementsManager()
{
}

NotationElementList*
ViewElementsManager::notationElementList(EventList::iterator from,
                                         EventList::iterator to)
{
    NotationElementList *res = new NotationElementList;
    
    for (EventList::iterator i = from; i != to; ++i) {
//         res->push_back(new NotationElement(*i));
        res->insert(new NotationElement(*i));
    }

    return res;
}
