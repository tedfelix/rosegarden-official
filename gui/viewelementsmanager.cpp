
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

using Rosegarden::Track;

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

//         kdDebug(KDEBUG_AREA) << "ViewElementsManager::notationElementList() : inserting "
//                              << (*i)->getType() << " of duration "
//                              << (*i)->getDuration() << " at time " 
//                              << (*i)->getAbsoluteTime() << endl;

        m_notationElements->insert(el);
    }

    return m_notationElements;
}

void ViewElementsManager::insertNewEvents(Rosegarden::Track::iterator from,
                                          Rosegarden::Track::iterator to)
{
    bool eventHasViewElement = false;
    
    for (Track::iterator i = from; i != to; ++i) {

        if ((*i)->hasViewElement()) {
            kdDebug(KDEBUG_AREA) << "ViewElementsManager::insertNewEvents() : event "
                                 << (*i)->getType() << " at time "
                                 << (*i)->getAbsoluteTime() << " already has a ViewElement\n";

            eventHasViewElement = true;
            continue;
        }

        NotationElement *el = new NotationElement(*i);

//         kdDebug(KDEBUG_AREA) << "ViewElementsManager::notationElementList() : inserting "
//                              << (*i)->getType() << " of duration "
//                              << (*i)->getDuration() << " at time " 
//                              << (*i)->getAbsoluteTime() << endl;

        m_notationElements->insert(el);
    }

    if (eventHasViewElement) {
        // be lenient about it
        //KMessageBox::error(0, "ViewElementsManager::insertNewEvents() : tried wrapping events which already had ViewElements");
    }

}

void ViewElementsManager::insert(Rosegarden::Event* e)
{
    if (!e->hasViewElement()) {
        NotationElement *el = new NotationElement(e);
        m_notationElements->insert(el);
    }
}


void ViewElementsManager::insert(NotationElement *e)
{
//     kdDebug(KDEBUG_AREA) << "ViewElementsManager::insert("
//                          << e->event()->getType() << ")\n";

    m_notationElements->insert(e);
    m_track.insert(e->event());
}

void ViewElementsManager::erase(NotationElementList::iterator it)
{
    std::pair<Track::iterator, Track::iterator> interval
        = m_track.equal_range((*it)->event());
    // we can't use find() because events are sorted by time
    // and there could be more than one event at the same time
    // so we have to look for the actual event in the given interval

    bool foundEvent = false;

    for (Track::iterator eIter = interval.first;
         eIter != interval.second;
         ++eIter) {

        if ((*eIter) == ((*it)->event())) {
            kdDebug(KDEBUG_AREA) << "ViewElementsManager::erase() : Found Event : "
                                 << (*it) << endl;

            //delete *eIter; - the track will delete it
            m_track.erase(eIter);
            foundEvent = true;
            break;
        }

    }
    
    if (foundEvent) {
        // delete in all ViewElement lists
        m_notationElements->erase(it);
    } else {
        kdDebug(KDEBUG_AREA) << "ViewElementsManager::erase() : couldn't find event for notation element "
                             << *(*it) << endl;
        KMessageBox::error(0, "ViewElementsManager::erase() : could't find event");
    }

}

void ViewElementsManager::erase(NotationElement* el)
{
    kdDebug(KDEBUG_AREA) << "ViewElementsManager::erase() erasing : "
                         << el << endl;
    
    Rosegarden::Event* ev = el->event();
    m_notationElements->erase(el); // this will delete el
    m_track.erase(ev);
}

