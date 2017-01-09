/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDINSERTERSORT_H
#define RG_MAPPEDINSERTERSORT_H

#include "sound/MappedInserterBase.h"
#include "sound/MappedEvent.h"
#include <list>

namespace Rosegarden
{

/// Sorts MappedEvent objects.
/**
 * SortingInserter is a pseudo-inserter that accepts events,
 * sorts them (as fetchEvents does not wholly do) and (via insertSorted())
 * re-inserts them somewhere, sorted.
 *
 * This is used when generating a standard MIDI file.  See
 * MidiFile::convertToMidi().
 *
 * @author Tom Breton (Tehom)
 */
class SortingInserter : public MappedInserterBase
{
    /// Comparison functor for std::list::sort()
    struct MappedEventCmp
    {
        bool operator()(const MappedEvent &a, const MappedEvent &b) const {
            // ??? This functor may not be needed given that operator<
            //     is defined for MappedEvent.  std::list::sort() without
            //     arguments will use MappedEvent::operator<().
            return a < b;
        }
    };
    
public:
    /// Sorts the events and copies them to an inserter.
    /**
     * Call this after inserting events via insertCopy() to get the
     * sorted events out.
     *
     * rename: exportSorted()?  extractSorted()?  Something a little more
     *         "output-oriented" seems like it might be easier to understand.
     */
    void insertSorted(MappedInserterBase &exporter);

private:
    /// Inserts an event into a list in preparation for sorting.
    /**
     * See insertSorted() which sorts the list and extracts the events
     * from the list in sorted order.
     */
    virtual void insertCopy(const MappedEvent &evt);

    // NB, this is not the same as MappedEventList which is actually a
    // std::multiset.
    std::list<MappedEvent> m_list;
};

}

#endif /* ifndef RG_MAPPEDINSERTERSORT_H */
