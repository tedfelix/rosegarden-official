
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

