
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
    event()->set<Int>(P_NOTE_TYPE, note.getType());
    event()->set<Bool>(P_NOTE_DOTTED, note.isDotted());
}

Note
NotationElement::getNote() const
{
    Note::Type ntype = event()->get<Int>(P_NOTE_TYPE);
    bool isDotted = false;
    event()->get<Bool>(P_NOTE_DOTTED, isDotted);
    
    return Note(ntype, isDotted);
}


bool
NotationElement::isRest() const
{
    //???
    return event()->isa("core", "rest");
//    return event()->type() == "rest";
}

bool
NotationElement::isNote() const
{
    return event()->isa(Note::EventPackage, Note::EventType);
//    return event()->type() == "note";
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
    delete *pos;
    multiset<NotationElement*, NotationElementCmp>::erase(pos);
}

NotationElementList::iterator
NotationElementList::findPrevious(const string &package,
                                  const string &type, iterator i)

{
    //!!! what to return on failure? I think probably
    // end(), as begin() could be a success case
    if (i == begin()) return end();
    --i;
    for (;;) {
        if ((*i)->event()->isa(package, type)) return i;
        if (i == begin()) return end();
        --i;
    }
}

NotationElementList::iterator
NotationElementList::findNext(const string &package,
                              const string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(package, type); ++i);
    return i;
}

class PitchGreater {
public:
    bool operator()(const NotationElementList::iterator &a,
                    const NotationElementList::iterator &b) {
        try {
            return ((*a)->event()->get<Int>("pitch") <
                    (*b)->event()->get<Int>("pitch"));
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "Bad karma: PitchGreater failed to find one or both pitches" << endl;
            return false;
        }
    }
};

vector<NotationElementList::iterator>
NotationElementList::findSucceedingChordElements(iterator i,
                                                 iterator &inext,
                                                 Event::timeT &maxDuration,
                                                 bool quantized) 
{
    // I think we're trying to please rather too many different people
    // in this method

    vector<NotationElementList::iterator> v;
    Event::timeT myTime = (*i)->getAbsoluteTime();
    long d;
    maxDuration = 0;

    // A chord can only contain notes; anything else having the same
    // time as this is probably a bogoid of some nature.  We succeed
    // trivially for the first element, if there is one & it's a note
    while (i != end() && (*i)->isNote() && (*i)->getAbsoluteTime() == myTime) {

        if (quantized) {
            bool done = (*i)->event()->get<Int>(P_QUANTIZED_DURATION, d);
            if (!done) {
                m_quantizer.quantize((*i)->event());
                d = (*i)->event()->get<Int>(P_QUANTIZED_DURATION);
            }
        } else {
            d = (*i)->event()->getDuration();
        }

        if (d > maxDuration) maxDuration = d;
        v.push_back(i);
        inext = i;
        ++i;
    }

    if (v.size() > 1) {
        stable_sort(v.begin(), v.end(), PitchGreater());
    }

    kdDebug(KDEBUG_AREA) << "NotationElementList::findSucceedingChordElements: pitches are:" << endl;
    for (unsigned int i = 0; i < v.size(); ++i) {
        kdDebug(KDEBUG_AREA) << ((*v[i])->event()->get<Int>("pitch")) <<endl;
    }

    return v;
}

bool NotationElementList::hasSucceedingChordElements(iterator i)
{
    // find out whether findSucceedingChordElements would return >1 elt
    int c = 0;
    Event::timeT myTime = (*i)->getAbsoluteTime();
    while (i != end() && (*i)->isNote() && (*i)->getAbsoluteTime() == myTime) {
        if (++c > 1) return true;
        ++i;
    }
    return false;
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
