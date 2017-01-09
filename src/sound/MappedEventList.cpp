/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QDataStream>
#include "MappedEventList.h"
#include "MappedEvent.h"
#include "base/SegmentPerformanceHelper.h"

namespace Rosegarden
{

MappedEventList::~MappedEventList()
{
    clear();
}

// copy constructor
MappedEventList::MappedEventList(const MappedEventList &mC):
    std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>()
{
    clear();

    // deep copy
    for (MappedEventList::const_iterator it = mC.begin(); it != mC.end(); ++it)
        insert(new MappedEvent(**it));

}

MappedEventList &
MappedEventList::operator=(const MappedEventList &c)
{
    if (&c == this) return *this;

    clear();

    for (MappedEventList::const_iterator it = c.begin(); it != c.end(); ++it)
        insert(new MappedEvent(**it));

    return *this;
}
    
void
MappedEventList::merge(const MappedEventList &mC)
{
    for (MappedEventList::const_iterator it = mC.begin(); it != mC.end(); ++it)
        insert(new MappedEvent(**it)); // deep copy
}

void
MappedEventList::clear()
{
    for (MappedEventListIterator it = begin(); it != end(); ++it)
        delete (*it);

    erase(begin(), end());
}



}


