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


#ifndef _MAPPEDCOMPOSITION_H_
#define _MAPPEDCOMPOSITION_H_


// MappedComposition is used with MappedEvent to create a sequence
// of MIDI ready events ready for playing.  The QDataStream operators
// are a necessary part of the DCOP transmission process allowing 
// the whole class to be serialized.  The core application is sent
// a request specifying a time slice between given start and end
// points which it fills with MappedEvents which are cut down
// (sequencer suitable) versions of the core Events.
//

#include <Composition.h>
#include "MappedEvent.h"
#include <set>
#include <QDataStream>

namespace Rosegarden
{

class MappedComposition : public std::multiset<MappedEvent *,
                          MappedEvent::MappedEventCmp>
{
public:
    MappedComposition() { }

    MappedComposition(const MappedComposition &mC);

    void merge(const MappedComposition &mC);

    MappedComposition &operator=(const MappedComposition &mC);

    ~MappedComposition();

    // Clear out
    void clear();
};

typedef std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>::iterator MappedCompositionIterator;


}

#endif // _MAPPEDCOMPOSITION_H_
