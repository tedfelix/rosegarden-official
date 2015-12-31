/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_MAPPED_EVENT_LIST_H
#define RG_MAPPED_EVENT_LIST_H


#include "base/Composition.h"
#include "MappedEvent.h"
#include <set>
#include <QDataStream>

namespace Rosegarden
{

/**
 * MappedEventList is used with MappedEvent to create a sequence
 * of MIDI ready events ready for playing.
 *
 * Note that MappedEventList is a list of MappedEvents, not a mapped
 * list of events.  Although a MappedEvent is a fixed-size object for
 * use in contexts like MappedEventBuffer which contain fixed-size sets of
 * fixed-size objects to be read and written with minimal locking,
 * MappedEventList is a normal container with nothing fixed about it;
 * it's just the container that happens to be used in sequencer
 * threads when a set of MappedEvents is called for.
 */
class MappedEventList : public std::multiset<MappedEvent *,
                                             MappedEvent::MappedEventCmp>
{
public:
    MappedEventList() { }
    MappedEventList(const MappedEventList &mC);

    void merge(const MappedEventList &mC);

    MappedEventList &operator=(const MappedEventList &mC);

    ~MappedEventList();

    // Clear out
    void clear();
};

typedef std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>::iterator
    MappedEventListIterator;

}

#endif
