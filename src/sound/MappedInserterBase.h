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

#ifndef RG_MAPPEDINSERTERBASE_H
#define RG_MAPPEDINSERTERBASE_H

namespace Rosegarden
{
class MappedEvent;

/// Base class for the polymorphic event inserters.
/**
 * There are three derivers:
 *
 *   - MappedEventInserter for playback
 *   - SortingInserter for sorting events when generating standard MIDI files
 *   - MidiInserter for generating standard MIDI files
 *
 * See each of the above for more details.
 *
 * Polymorphism in this case lets us use the same code to drive playback
 * and MIDI file generation.
 */
class MappedInserterBase
{
public:
    // Make sure the proper derived dtor gets called.
    virtual ~MappedInserterBase()  { }

    /// Derivers override this to provide more specific insertion behavior.
    virtual void insertCopy(const MappedEvent &evt) = 0;
};

}

#endif /* ifndef RG_MAPPEDINSERTERBASE_H */
