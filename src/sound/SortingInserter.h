/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MAPPEDINSERTERSORT_H_
#define _MAPPEDINSERTERSORT_H_

#include "sound/MappedInserterBase.h"
#include "sound/MappedEvent.h"
#include <queue>

namespace Rosegarden
{

// @class SortingInserter is a pseudo-inserter that accepts events,
// sorts them (as fillCompositionWithEventsUntil does not wholly do)
// and re-inserts them somewhere, sorted.
// @author Tom Breton (Tehom)
class SortingInserter : public MappedInserterBase
{
    struct MappedEventRevCmp
    {
        bool operator()(const MappedEvent &a, const MappedEvent &b) const {
            return b < a;
        }
    };
    
 public:
  void insertSorted(MappedInserterBase &);
 private:
  virtual void insertCopy(const MappedEvent &evt);
  std::priority_queue<MappedEvent, std::deque<MappedEvent>, MappedEventRevCmp> m_queue;
};

}

#endif /* ifndef _MAPPEDINSERTERSORT_H_ */
