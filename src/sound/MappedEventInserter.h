/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDEVENTINSERTER_H
#define RG_MAPPEDEVENTINSERTER_H

#include "MappedInserterBase.h"

namespace Rosegarden
{

class MappedEventList;

/// Inserts MappedEvent objects into a MappedEventList.
/**
 * This is primarily used by RosegardenSequencer::getSlice() during playback
 * to generate a MappedEventList to send off to ALSA.
 *
 * ??? This inside-out thinking hurts my brain.  Can we instead just send
 *     a MappedEventList & to whoever needs to insert things, and let them
 *     call a MappedEventList::insertCopy()?
 */
class MappedEventInserter : public MappedInserterBase
{
public:
    explicit MappedEventInserter(MappedEventList &list) :
        m_list(list)
    { }

    /// Inserts an event into the MappedEventList (m_list).
    void insertCopy(const MappedEvent &evt) override;

private:
    MappedEventList &m_list;
};

}

#endif /* ifndef RG_MAPPEDEVENTINSERTER_H */
