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

#include <kmessagebox.h>

#include "rosedebug.h"

#include "viewelementsmanager.h"
#include "notationelement.h"

ViewElementsManager::ViewElementsManager(Track &t)
    : m_track(t),
      m_notationElements(0)
{
}

ViewElementsManager::~ViewElementsManager()
{
}

NotationElementList*
ViewElementsManager::notationElementList(Track::iterator from,
                                         Track::iterator to)
{
    if (m_notationElements) return m_notationElements;
    
    m_notationElements = new NotationElementList;

    for (Track::iterator i = from; i != to; ++i) {
        NotationElement *el = new NotationElement(*i);
        m_notationElements->insert(el);
    }

    return m_notationElements;
}

void
ViewElementsManager::insert(NotationElement *e)
{
    m_notationElements->insert(e);
    m_track.insert(e->event());
}

void
ViewElementsManager::erase(NotationElementList::iterator it)
{
    pair<Track::iterator, Track::iterator> interval
        = m_track.equal_range((*it)->event());

    bool foundEvent = false;

    for (Track::iterator eIter = interval.first;
         eIter != interval.second;
         ++eIter) {

        if ((*eIter) == ((*it)->event())) {
            kdDebug(KDEBUG_AREA) << "ViewElementsManager::erase() : Found Event : "
                                 << *(*it) << endl;

            delete *eIter;
            m_track.erase(eIter);
            foundEvent = true;
            break;
        }

    }
    
    if (foundEvent) {
        m_notationElements->erase(it);
    } else {
        kdDebug(KDEBUG_AREA) << "ViewElementsManager::erase() : couldn't find event for notation element "
                             << *(*it) << endl;
        KMessageBox::error(0, "ViewElementsManager::erase() : could't find event");
    }

}

