
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

#ifndef RG_ADDTRACKSCOMMAND_H
#define RG_ADDTRACKSCOMMAND_H

#include "document/Command.h"  // for NamedCommand
#include "base/Instrument.h"
#include "base/Track.h"

#include <QCoreApplication>  // for Q_DECLARE_TR_FUNCTIONS()

#include <vector>
#include <map>


namespace Rosegarden
{


class AddTracksCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddTracksCommand)

public:
    /// Add one Track
    AddTracksCommand(InstrumentId instrumentId,
                     int trackPosition); // -1 -> at end
    /// Add multiple Tracks
    AddTracksCommand(unsigned int numberOfTracks,
                     InstrumentId instrumentId,
                     int trackPosition); // -1 -> at end
    typedef std::vector<InstrumentId> InstrumentIdList;
    /// Add multiple Tracks with a list of Instrument IDs to use.
    AddTracksCommand(unsigned int numberOfTracks,
                     InstrumentIdList instrumentIdList,
                     int trackPosition); // -1 -> at end

    ~AddTracksCommand() override;

    void execute() override;
    void unexecute() override;

private:
    /// Number of Tracks being added.
    unsigned int m_numberOfTracks;
    /// Instruments to use for each new Track.
    InstrumentIdList m_instrumentIdList;
    /// Where to insert the new Tracks.
    int m_trackPosition;

    /// Tracks created by this command.
    std::vector<Track *> m_newTracks;

    typedef std::map<TrackId, int /*position*/> TrackPositionMap;
    /// Track positions prior to the add.
    TrackPositionMap m_oldPositions;

    /// Tracks in m_newTracks are no longer in the Composition (we've been undone).
    /**
     * ??? Seems like a concept that many commands might find useful.  Why
     *     isn't there a Command::m_undone?
     */
    bool m_detached;
};


}

#endif
