
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

#include <qcanvas.h>
#include <algorithm>

#include "viewelementsmanager.h"
#include "notationelement.h"
#include "notationproperties.h"

#ifndef NDEBUG
#include "rosedebug.h"
#endif

#include "NotationTypes.h"
#include "Event.h"

#include <kmessagebox.h>

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;

NotationElement::NotationElement(Event *event)
    : ViewElement(event),
      m_x(0),
      m_y(0),
      m_canvasItem(0)
{
}

NotationElement::~NotationElement()
{
//     kdDebug(KDEBUG_AREA) << "~NotationElement()\n";

    delete m_canvasItem;
}

double
NotationElement::getEffectiveX() throw (NoCanvasItem)
{
    if (m_canvasItem)
        return m_canvasItem->x();
    else
        throw NoCanvasItem();
}

double
NotationElement::getEffectiveY() throw (NoCanvasItem)
{
    if (m_canvasItem)
        return m_canvasItem->y();
    else
        throw NoCanvasItem();
}

void
NotationElement::setNote(Note note)
{
    event()->setDuration(note.getDuration());
    event()->setMaybe<Int>(Rosegarden::Note::NoteType, note.getNoteType());
    event()->setMaybe<Int>(Rosegarden::Note::NoteDots, note.getDots());
}

Note
NotationElement::getNote() const
{
    Note::Type ntype = event()->get<Int>(Rosegarden::Note::NoteType);
    long dots = 0;;
    event()->get<Int>(Rosegarden::Note::NoteDots, dots);
    return Note(ntype, dots);
}


bool
NotationElement::isRest() const
{
    return event()->isa("rest");
}

bool
NotationElement::isNote() const
{
    return event()->isa(Note::EventType);
}

void
NotationElement::setCanvasItem(QCanvasItem *e, double dxoffset, double dyoffset)
{
    delete m_canvasItem;
    m_canvasItem = e;
    e->move(m_x + dxoffset, m_y + dyoffset);
}

//////////////////////////////////////////////////////////////////////

NotationElementList::~NotationElementList()
{
    for(iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
}

void
NotationElementList::erase(NotationElementList::iterator pos)
{
    kdDebug(KDEBUG_AREA) << "NotationElementList::erase() : delete "
                         << *pos << endl;

    delete *pos;
    multiset<NotationElement*, NotationElementCmp>::erase(pos);
}

void
NotationElementList::erase(NotationElementList::iterator from,
                           NotationElementList::iterator to)
{
    for (NotationElementList::iterator i = from;
         i != to; ++i)
        delete *i;

    multiset<NotationElement*, NotationElementCmp>::erase(from, to);
}

void
NotationElementList::eraseSingle(NotationElement* el)
{
    iterator elPos = findSingle(el);

    if (elPos != end()) {
        
        erase(elPos);

    } else {
        
        kdDebug(KDEBUG_AREA) << "NotationElementList::eraseSingle() : couldn't find element "
                             << *el << endl;
        KMessageBox::error(0, "NotationElementList::erase() : could't find element");
    }

}

NotationElementList::iterator
NotationElementList::findPrevious(const std::string &type, iterator i)

{
    //!!! what to return on failure? I think probably
    // end(), as begin() could be a success case
    if (i == begin()) return end();
    --i;
    for (;;) {
        if ((*i)->event()->isa(type)) return i;
        if (i == begin()) return end();
        --i;
    }
}

NotationElementList::iterator
NotationElementList::findNext(const std::string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(type); ++i);
    return i;
}

NotationElementList::iterator
NotationElementList::findSingle(NotationElement* el)
{
    iterator res = end();

    std::pair<iterator, iterator> interval = equal_range(el);

    bool foundIt = false;
    
    for(iterator i = interval.first; i != interval.second; ++i) {
        if (*i == el) {
            res = i;
            break;
        }
    }

    return res;
}


#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream &dbg, NotationElement &e)
{
    dbg << "NotationElement - x : " << e.getLayoutX() << " - y : " << e.getLayoutY()
        << endl << *e.event() << endl;

//     e.event()->dump(cout);

    return dbg;
}

kdbgstream& operator<<(kdbgstream &dbg, NotationElementList &l)
{
    for(NotationElementList::const_iterator i = l.begin();
        i != l.end(); ++i) {
        dbg << *(*i) << endl;
    }

    return dbg;
}

#endif

