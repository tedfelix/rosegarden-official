/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[DeleteTracksCommand]"

#include "DeleteTracksCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include <QString>


namespace Rosegarden
{

DeleteTracksCommand::DeleteTracksCommand(Composition *composition,
        std::vector<TrackId> tracks):
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_tracks(tracks),
        m_detached(false)
{}

DeleteTracksCommand::~DeleteTracksCommand()
{
    if (m_detached) {
        for (size_t i = 0; i < m_oldTracks.size(); ++i)
            delete m_oldTracks[i];

        for (size_t i = 0; i < m_oldSegments.size(); ++i)
            delete m_oldSegments[i];

        m_oldTracks.clear();
        m_oldSegments.clear();
    }
}

void DeleteTracksCommand::execute()
{
    // Clear out the undo info.
    m_oldSegments.clear();
    m_oldTracks.clear();

    // Alias for readability
    const SegmentMultiSet &segments = m_composition->getSegments();

    // Remove the tracks and their segments.

    // For each track we are deleting.
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        TrackId trackId = m_tracks[i];
        Track *track = m_composition->getTrackById(trackId);

        if (track) {
            // ??? The following segment removal code will never find any
            //     segments to remove.  All clients of this class make sure
            //     the segments are removed before the tracks are removed.
            //     This code should be removed and replaced with a check to
            //     make sure that the segments have been removed before
            //     removing the tracks.  Segment removal is a bit tricky
            //     for audio segments.  We should limit the kludge to
            //     SegmentEraseCommand.

            // For each segment in the composition.
            for (SegmentMultiSet::const_iterator j = segments.begin();
                 j != segments.end();
                 /* incremented inside */) {
                // Increment before use.  Otherwise detachSegment() will
                // invalidate our iterator.
                SegmentMultiSet::const_iterator k = j++;

                // If this segment is on the track we are deleting
                if ((*k)->getTrack() == trackId) {
                    // Save the segment for undo.
                    m_oldSegments.push_back(*k);
                    // Remove the segment from the composition.
                    m_composition->detachSegment(*k);
                }
            }

            // Save the track for undo.
            m_oldTracks.push_back(track);
            // Remove the track from the composition.
            if (m_composition->detachTrack(track) == false) {
                RG_DEBUG << "DeleteTracksCommand::execute - can't detach track";
            }
        }
    }

    // Adjust the track position numbers to remove any gaps.

    Composition::trackcontainer &tracks = m_composition->getTracks();

    // For each deleted track
    for (std::vector<Track*>::iterator oldTrackIter = m_oldTracks.begin();
         oldTrackIter != m_oldTracks.end();
         ++oldTrackIter) {
        // For each track left in the composition
        for (Composition::trackiterator compTrackIter = tracks.begin();
             compTrackIter != tracks.end();
             ++compTrackIter) {
            // If the composition track was after the deleted track
            if (compTrackIter->second->getPosition() >
                    (*oldTrackIter)->getPosition()) {
                // Decrement the composition track's position to close up
                // the gap in the position numbers.
                int newPosition = compTrackIter->second->getPosition() - 1;
                compTrackIter->second->setPosition(newPosition);
            }
        }
    }

    m_composition->notifyTracksDeleted(m_tracks);

    m_detached = true;
}

void DeleteTracksCommand::unexecute()
{
    // Add the tracks and the segments back in.

    std::vector<TrackId> trackIds;

    // Alias for readability.
    Composition::trackcontainer &tracks = m_composition->getTracks();

    // For each track we need to add back in 
    for (std::vector<Track*>::iterator oldTrackIter = m_oldTracks.begin(); 
         oldTrackIter != m_oldTracks.end(); 
         ++oldTrackIter) {

        // From the back we shift the track positions in the composition
        // to allow the new (old) track some space to come back in.

        Composition::trackiterator compTrackIter = tracks.end();
        while (true) {
            --compTrackIter;

            // If the composition track's position is after or the same as
            // the position of the track we are adding
            if ((*compTrackIter).second->getPosition() >= 
                    (*oldTrackIter)->getPosition()) {
                // Increment the composition track's position to make room
                int newPosition = (*compTrackIter).second->getPosition() + 1;
                (*compTrackIter).second->setPosition(newPosition);
            }

            if (compTrackIter == tracks.begin())
                break;
        }

        // Add the new (old) track back in.
        m_composition->addTrack(*oldTrackIter);
        trackIds.push_back((*oldTrackIter)->getId());
    }

    // Add the old segments back in.
    // ??? This is not good enough for audio segments.  See
    //     SegmentEraseCommand::unexecute().
    for (size_t i = 0; i < m_oldSegments.size(); ++i)
        m_composition->addSegment(m_oldSegments[i]);

    m_composition->notifyTracksAdded(trackIds);

    m_detached = false;
}

}

