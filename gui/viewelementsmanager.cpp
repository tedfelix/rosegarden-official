
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
using Rosegarden::Event;

ViewElementsManager::ViewElementsManager(Track &t)
    : m_track(t),
      m_notationElements(0)
{
    // empty
}

ViewElementsManager::~ViewElementsManager()
{
    if (m_notationElements) m_track.removeObserver(this);
}

NotationElementList*
ViewElementsManager::getNotationElementList()
{
    return getNotationElementList(m_track.begin(), m_track.end());
}

NotationElementList*
ViewElementsManager::getNotationElementList(Track::iterator from,
                                            Track::iterator to)
{
    if (m_notationElements) return m_notationElements;

    m_notationElements = new NotationElementList;

    for (Track::iterator i = from; i != to; ++i) {
        NotationElement *el = new NotationElement(*i);
        m_notationElements->insert(el);
    }

    m_track.addObserver(this);
    return m_notationElements;
}

void ViewElementsManager::insert(Rosegarden::Event* e,
                                 bool insertInTrack)
{
    if (insertInTrack) {
        m_track.insert(e);
        // and let the eventAdded callback do the wrapping
    } else {
	if (!e->hasViewElement()) {
	    NotationElement *el = new NotationElement(e);
	    m_notationElements->insert(el);
	} else {
	    KMessageBox::error(0, "ViewElementsManager::insert() : trying to insert an Event which already has a ViewElement");
	}
    }
}

void ViewElementsManager::erase(NotationElementList::iterator it,
                                bool eraseFromTrack)
{
    if (eraseFromTrack) {
        m_track.eraseSingle((*it)->event());
        // and let the eventRemoved callback do the unwrapping
    } else {
        m_notationElements->erase(it);
    }
}


NotationElementList::iterator ViewElementsManager::findEvent(Event *e)
{
    NotationElement dummy(e);
    std::pair<NotationElementList::iterator, NotationElementList::iterator>
        r = m_notationElements->equal_range(&dummy);

    for (NotationElementList::iterator i = r.first; i != r.second; ++i) {
        if ((*i)->event() == e) {
            return i;
        }
    }

    return m_notationElements->end();
}


void ViewElementsManager::eventAdded(Track *t, Event *e)
{
    assert(t == &m_track);
    kdDebug(KDEBUG_AREA) 
	<< "ViewElementsManager::eventAdded: at time " << e->getAbsoluteTime()
	<< endl;

    // If it isn't already wrapped, wrap it.  The already-wrapped test
    // is rather slow, and if all goes according to plan we should
    // eventually be able to lose it.

    if (findEvent(e) == m_notationElements->end()) {

        kdDebug(KDEBUG_AREA) << "We haven't got it: wrapping it now" << endl;
        NotationElement *el = new NotationElement(e);
        m_notationElements->insert(el);

    } else {
        kdDebug(KDEBUG_AREA) << "We already have it" << endl;
    }
}

void ViewElementsManager::eventRemoved(Track *t, Event *e)
{
    assert(t == &m_track);
    kdDebug(KDEBUG_AREA) 
	<< "ViewElementsManager::eventRemoved: from time "
	<< e->getAbsoluteTime() << endl;

    // If we have it, lose it

    NotationElementList::iterator i = findEvent(e);
    if (i == m_notationElements->end()) {

        kdDebug(KDEBUG_AREA) << "We haven't got it" << endl;

    } else {
        kdDebug(KDEBUG_AREA) << "We have this one, losing it" << endl;
        m_notationElements->erase(i);
        return;
    }


}

void ViewElementsManager::trackDeleted(Track *t)
{
    assert(t == &m_track);
    kdDebug(KDEBUG_AREA) << "ViewElementsManager::trackDeleted!" << endl;
}
    
