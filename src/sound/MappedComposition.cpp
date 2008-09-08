/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QDataStream>
#include "MappedComposition.h"
#include "MappedEvent.h"
#include "SegmentPerformanceHelper.h"
#include <iostream>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

MappedComposition::~MappedComposition()
{
    clear();
}

// copy constructor
MappedComposition::MappedComposition(const MappedComposition &mC):
        std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>()
{
    clear();

    // deep copy
    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); it++)
        insert(new MappedEvent(**it));

}

MappedComposition &
MappedComposition::operator=(const MappedComposition &c)
{
    if (&c == this) return *this;

    clear();

    for (MappedComposition::const_iterator it = c.begin(); it != c.end(); it++)
        insert(new MappedEvent(**it));

    return *this;
}
    
void
MappedComposition::merge(const MappedComposition &mC)
{
    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); it++)
        insert(new MappedEvent(**it)); // deep copy
}

void
MappedComposition::clear()
{
    for (MappedCompositionIterator it = begin(); it != end(); it++)
        delete (*it);

    erase(begin(), end());
}



}


