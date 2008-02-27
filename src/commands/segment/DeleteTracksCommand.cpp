/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "DeleteTracksCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <qstring.h>


namespace Rosegarden
{

DeleteTracksCommand::DeleteTracksCommand(Composition *composition,
        std::vector<TrackId> tracks):
        KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_tracks(tracks),
        m_detached(false)
{}

DeleteTracksCommand::~DeleteTracksCommand()
{
    if (m_detached) {
        for (unsigned int i = 0; i < m_oldTracks.size(); ++i)
            delete m_oldTracks[i];

        for (unsigned int i = 0; i < m_oldSegments.size(); ++i)
            delete m_oldSegments[i];

        m_oldTracks.clear();
        m_oldSegments.clear();
    }
}

void DeleteTracksCommand::execute()
{
    Track *track = 0;
    const Composition::segmentcontainer &segments =
        m_composition->getSegments();

    //cout << "DeleteTracksCommand::execute()" << endl;

    m_oldSegments.clear();
    m_oldTracks.clear();

    // Remap positions and track numbers
    //

    Composition::trackiterator tit;
    Composition::trackcontainer
    &tracks = m_composition->getTracks();

    for (unsigned int i = 0; i < m_tracks.size(); ++i) {
        // detach segments and store tracks somewhere
        track = m_composition->getTrackById(m_tracks[i]);

        if (track) {
            // detach all segments for that track
            //
            for (Composition::segmentcontainer::const_iterator
                    it = segments.begin();
                    it != segments.end(); ++it) {
                if ((*it)->getTrack() == m_tracks[i]) {
                    m_oldSegments.push_back(*it);
                    m_composition->detachSegment(*it);
                }
            }

            // store old tracks
            m_oldTracks.push_back(track);
            if (m_composition->detachTrack(track) == false) {
                RG_DEBUG << "DeleteTracksCommand::execute - can't detach track" << endl;
            }
        }
    }

    std::vector<Track*>::iterator otIt;
    for (otIt = m_oldTracks.begin(); otIt != m_oldTracks.end(); ++otIt) {
        for (tit = tracks.begin(); tit != tracks.end(); ++tit) {
            if ((*tit).second->getPosition() > (*otIt)->getPosition()) {
                // If the track we've removed was after the current
                // track then decrement the track position.
                //
                int newPosition = (*tit).second->getPosition() - 1;

                (*tit).second->setPosition(newPosition);

            }
        }
    }

    m_detached = true;
}

void DeleteTracksCommand::unexecute()
{
    // Add the tracks and the segments back in
    //

    // Remap positions and track numbers
    //
    Composition::trackcontainer
    &tracks = m_composition->getTracks();
    Composition::trackiterator tit;

    std::vector<Track*>::iterator otIt;
    for (otIt = m_oldTracks.begin(); otIt != m_oldTracks.end(); ++otIt) {
        // From the back we shuffle out the tracks to allow the new
        // (old) track some space to come back in.
        //
        tit = tracks.end();
        while (true) {
            --tit;

            if ((*tit).second->getPosition() >= (*otIt)->getPosition()) {
                // If the track we're adding has position after the
                // current track then increment position.
                //
                int newPosition = (*tit).second->getPosition() + 1;

                (*tit).second->setPosition(newPosition);
            }

            if (tit == tracks.begin())
                break;
        }

        m_composition->addTrack(*otIt);
    }

    for (unsigned int i = 0; i < m_oldSegments.size(); ++i)
        m_composition->addSegment(m_oldSegments[i]);

    m_detached = false;
}

}
