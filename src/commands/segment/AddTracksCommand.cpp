/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QString>


namespace Rosegarden
{

AddTracksCommand::AddTracksCommand(Composition *composition,
                                   unsigned int nbTracks,
                                   InstrumentId id,
                                   int position):
    NamedCommand(getGlobalName()),
    m_composition(composition),
    m_nbNewTracks(nbTracks),
    m_instrumentId(id),
    m_position(position),
    m_detached(false)

{
}

AddTracksCommand::~AddTracksCommand()
{
    if (m_detached) {
        for (size_t i = 0; i < m_newTracks.size(); ++i)
            delete m_newTracks[i];
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

            int newPosition = i->second + m_nbNewTracks;
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

    if (m_position == -1) m_position = highPosition + 1;
    if (m_position < 0) m_position = 0;
    if (m_position > highPosition + 1) m_position = highPosition + 1;

    for (Composition::trackiterator it = m_composition->getTracks().begin();
         it != m_composition->getTracks().end(); ++it) {

        int pos = it->second->getPosition();

        if (pos >= m_position) {
            m_oldPositions[it->first] = pos;
            it->second->setPosition(pos + m_nbNewTracks);
        }
    }

    // Add the tracks

    std::vector<TrackId> trackIds;

    for (unsigned int i = 0; i < m_nbNewTracks; ++i) {

        TrackId trackId = m_composition->getNewTrackId();
        Track *track = new Track(trackId);

        track->setPosition(m_position + i);
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
