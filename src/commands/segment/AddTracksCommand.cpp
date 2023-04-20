/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AddTracksCommand]"

#include "AddTracksCommand.h"

#include "base/Composition.h"
#include "misc/Debug.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"

#include <algorithm>


namespace Rosegarden
{


AddTracksCommand::AddTracksCommand(InstrumentId instrumentId,
                                   int trackPosition) :
    NamedCommand(tr("Add Tracks...")),
    m_numberOfTracks(1),
    m_instrumentIdList{instrumentId},
    m_trackPosition(trackPosition),
    m_detached(false)
{
}

AddTracksCommand::AddTracksCommand(unsigned int numberOfTracks,
                                   InstrumentId instrumentId,
                                   int trackPosition) :
    NamedCommand(tr("Add Tracks...")),
    m_numberOfTracks(numberOfTracks),
    m_instrumentIdList{instrumentId},
    m_trackPosition(trackPosition),
    m_detached(false)
{
}

AddTracksCommand::AddTracksCommand(unsigned int numberOfTracks,
                                   InstrumentIdList instrumentIdList,
                                   int trackPosition) :
    NamedCommand(tr("Add Tracks...")),
    m_numberOfTracks(numberOfTracks),
    m_instrumentIdList(instrumentIdList),
    m_trackPosition(trackPosition),
    m_detached(false)
{
}

AddTracksCommand::~AddTracksCommand()
{
    // If the Tracks are no longer in the Composition, we'll have
    // to delete them ourselves.
    if (m_detached) {
        for (size_t i = 0; i < m_newTracks.size(); ++i) {
            delete m_newTracks[i];
        }
    }
}

void AddTracksCommand::execute()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;

    Composition &composition = document->getComposition();

    // If m_newTracks are not part of the Composition, we've been undone
    // and this is a redo.
    if (m_detached) {

        // Re-attach tracks (redo)

        // Keep a list for Composition notification.
        std::vector<TrackId> trackIds;

        // For each of the new Tracks, re-add them to the Composition.
        for (size_t i = 0; i < m_newTracks.size(); ++i) {
            composition.addTrack(m_newTracks[i]);
            trackIds.push_back(m_newTracks[i]->getId());
        }

        // For each Track that needs to be moved down...
        for (TrackPositionMap::const_iterator i = m_oldPositions.begin();
             i != m_oldPositions.end();
             ++i) {
            const TrackId trackId = i->first;
            const int trackPosition = i->second;

            Track *track = composition.getTrackById(trackId);
            if (!track)
                continue;

            // Move the Track down to make room for the new tracks.
            track->setPosition(trackPosition + m_numberOfTracks);
        }

        composition.notifyTracksAdded(trackIds);

        // Switch back to "done" mode.
        m_detached = false;

        return;
    }

    // Zero-based position of the bottom Track on the UI.
    int bottomTrackPosition = composition.getNbTracks() - 1;

    // Make sure m_trackPosition is within limits.

    if (m_trackPosition == -1)
        m_trackPosition = bottomTrackPosition + 1;
    if (m_trackPosition < 0)
        m_trackPosition = 0;
    if (m_trackPosition > bottomTrackPosition + 1)
        m_trackPosition = bottomTrackPosition + 1;

    // Adjust the track positions

    // For each Track in the Composition
    for (Composition::trackcontainer::value_type &trackPair :
         composition.getTracks()) {
        const TrackId trackId = trackPair.first;
        Track *track = trackPair.second;

        const int trackPosition = track->getPosition();

        // If this Track is at or past the insertion point.
        if (trackPosition >= m_trackPosition) {
            // Store the original Track position.
            m_oldPositions[trackId] = trackPosition;
            // Move the Track position down to make room for the new Tracks.
            track->setPosition(trackPosition + m_numberOfTracks);
        }
    }

    // Add the tracks

    // Keep a list for Composition notification.
    std::vector<TrackId> trackIds;

    // For each Track to add...
    for (size_t i = 0; i < m_numberOfTracks; ++i) {

        TrackId trackId = composition.getNewTrackId();
        // Create the Track
        Track *track = new Track(trackId);

        track->setPosition(m_trackPosition + i);
        const size_t instrumentListIndex =
                std::min(i, m_instrumentIdList.size() - 1);
        const InstrumentId instrumentId =
                m_instrumentIdList[instrumentListIndex];
        track->setInstrument(instrumentId);

        // Add it to the Composition.
        composition.addTrack(track);

        m_newTracks.push_back(track);
        trackIds.push_back(trackId);

        // Send channel setup in case it hasn't been sent for this instrument.
        Instrument *instrument =
                document->getStudio().getInstrumentById(instrumentId);
        if (instrument)
            instrument->sendChannelSetup();
    }

    composition.notifyTracksAdded(trackIds);

}

void AddTracksCommand::unexecute()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;

    Composition &composition = document->getComposition();

    // Keep a list for Composition notification.
    std::vector<TrackId> trackIds;

    // For each new Track, detach it from the Composition.
    for (size_t i = 0; i < m_newTracks.size(); ++i) {
        composition.detachTrack(m_newTracks[i]);
        trackIds.push_back(m_newTracks[i]->getId());
    }

    // For each Track that was moved, put it back.
    for (const TrackPositionMap::value_type &trackPositionPair :
         m_oldPositions) {
        const TrackId trackId = trackPositionPair.first;
        const int trackPosition = trackPositionPair.second;

        Track *track = composition.getTrackById(trackId);
        if (track)
            track->setPosition(trackPosition);
    }

    composition.notifyTracksDeleted(trackIds);

    m_detached = true;
}


}
