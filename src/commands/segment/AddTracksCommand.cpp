/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
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


namespace Rosegarden
{


AddTracksCommand::AddTracksCommand(Composition *composition,
                                   unsigned int numberOfTracks,
                                   InstrumentId instrumentId,
                                   int trackPosition):
    NamedCommand(tr("Add Tracks...")),
    m_composition(composition),
    m_numberOfTracks(numberOfTracks),
    m_instrumentId(instrumentId),
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
    // Re-attach tracks (redo)
    //
    if (m_detached) {

        std::vector<TrackId> trackIds;

        for (size_t i = 0; i < m_newTracks.size(); i++) {
            m_composition->addTrack(m_newTracks[i]);
            trackIds.push_back(m_newTracks[i]->getId());
        }

        // Adjust the track positions.
        for (TrackPositionMap::iterator i = m_oldPositions.begin();
             i != m_oldPositions.end(); ++i) {

            int newPosition = i->second + m_numberOfTracks;
            Track *track = m_composition->getTrackById(i->first);
            if (track) track->setPosition(newPosition);
        }

        m_composition->notifyTracksAdded(trackIds);

        m_detached = false;

        return;
    }

    // Adjust the track positions

    int highPosition = 0;

    for (Composition::trackiterator it = m_composition->getTracks().begin();
         it != m_composition->getTracks().end(); ++it) {

        int pos = it->second->getPosition();

        if (pos > highPosition) {
            highPosition = pos;
        }
    }

    if (m_trackPosition == -1) m_trackPosition = highPosition + 1;
    if (m_trackPosition < 0) m_trackPosition = 0;
    if (m_trackPosition > highPosition + 1) m_trackPosition = highPosition + 1;

    for (Composition::trackiterator it = m_composition->getTracks().begin();
         it != m_composition->getTracks().end(); ++it) {

        int pos = it->second->getPosition();

        if (pos >= m_trackPosition) {
            m_oldPositions[it->first] = pos;
            it->second->setPosition(pos + m_numberOfTracks);
        }
    }

    // Add the tracks

    std::vector<TrackId> trackIds;

    for (unsigned int i = 0; i < m_numberOfTracks; ++i) {

        TrackId trackId = m_composition->getNewTrackId();
        Track *track = new Track(trackId);

        track->setPosition(m_trackPosition + i);
        track->setInstrument(m_instrumentId);

        m_composition->addTrack(track);
        trackIds.push_back(trackId);
        m_newTracks.push_back(track);
    }

    m_composition->notifyTracksAdded(trackIds);

    // Send channel setup in case it hasn't been sent for this instrument.
    RosegardenDocument *document = RosegardenMainWindow::self()->getDocument();
    Instrument *instrument =
            document->getStudio().getInstrumentById(m_instrumentId);
    if (instrument)
        instrument->sendChannelSetup();
}

void AddTracksCommand::unexecute()
{
    std::vector<TrackId> trackIds;

    // Detach the tracks
    for (size_t i = 0; i < m_newTracks.size(); i++) {
        m_composition->detachTrack(m_newTracks[i]);
        trackIds.push_back(m_newTracks[i]->getId());
    }

    // Adjust the positions
    for (TrackPositionMap::iterator i = m_oldPositions.begin();
         i != m_oldPositions.end(); ++i) {

        Track *track = m_composition->getTrackById(i->first);
        if (track) track->setPosition(i->second);
    }

    m_composition->notifyTracksDeleted(trackIds);

    m_detached = true;
}

}
