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


#include "PasteSegmentsCommand.h"

#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "base/Track.h"
#include "gui/application/RosegardenMainWindow.h"
#include "document/RosegardenDocument.h"

#include <QString>


namespace Rosegarden
{

PasteSegmentsCommand::PasteSegmentsCommand(Composition *composition,
        Clipboard *clipboard,
        timeT pasteTime,
        TrackId baseTrack,
        bool useExactTracks) :
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_clipboard(new Clipboard(*clipboard)),
        m_pasteTime(pasteTime),
        m_baseTrack(baseTrack),
        m_exactTracks(useExactTracks),
        m_detached(false),
        m_oldEndTime(m_composition->getEndMarker())
{
    // nothing else
}

PasteSegmentsCommand::~PasteSegmentsCommand()
{
    if (m_detached) {
        for (size_t i = 0; i < m_addedSegments.size(); ++i) {
            delete m_addedSegments[i];
        }
    }

    delete m_clipboard;
}

void
PasteSegmentsCommand::execute()
{
    if (!m_addedSegments.empty()) {
        // been here before
        for (size_t i = 0; i < m_addedSegments.size(); ++i) {
            m_composition->addSegment(m_addedSegments[i]);
        }
        return ;
    }

    if (m_clipboard->isEmpty())
        return ;

    // We want to paste such that the earliest Segment starts at
    // m_pasteTime and the others start at the same times relative to
    // that as they did before.  Likewise for track, unless
    // m_exactTracks is set.

    timeT earliestStartTime = m_clipboard->getBaseTime();
    timeT latestEndTime = 0;
    int lowestTrackPos = -1;

    // For each segment in the clipboard, compute the lowest track
    // position and the latest end time.
    for (Clipboard::iterator i = m_clipboard->begin();
            i != m_clipboard->end(); ++i) {

        int trackPos = m_composition->getTrackPositionById((*i)->getTrack());
        if (trackPos >= 0 &&
                (lowestTrackPos < 0 || trackPos < lowestTrackPos)) {
            lowestTrackPos = trackPos;
        }

        if ((*i)->getEndMarkerTime() > latestEndTime) {
            latestEndTime = (*i)->getEndMarkerTime();
        }
    }

    if (m_exactTracks)
        lowestTrackPos = 0;
    if (lowestTrackPos < 0)
        lowestTrackPos = 0;
    timeT offset = m_pasteTime - earliestStartTime;
    int baseTrackPos = m_composition->getTrackPositionById(m_baseTrack);
    int trackOffset = baseTrackPos - lowestTrackPos;

    // For each segment in the clipboard, paste it into the composition
    for (Clipboard::iterator i = m_clipboard->begin();
            i != m_clipboard->end(); ++i) {
 
        // If the segment is an audio segment
        if ((*i)->getType() == Segment::Audio)
            // If the segment's file ID isn't in the audio file manager
            if (!RosegardenMainWindow::self()->getDocument()->
                getAudioFileManager().fileExists((*i)->getAudioFileId())) {
                // Skip pasting it.
                continue;
            }

        int newTrackPos = trackOffset +
                          m_composition->getTrackPositionById((*i)->getTrack());

        Track *track = m_composition->getTrackByPosition(newTrackPos);

        if (!track) {
            newTrackPos = 0;
            track = m_composition->getTrackByPosition(newTrackPos);
        }

        TrackId newTrackId = track->getId();

        Segment *segment = (**i).clone();
        segment->setStartTime(segment->getStartTime() + offset);
        segment->setTrack(newTrackId);
        m_composition->addSegment(segment);
        if (m_clipboard->isPartial()) {
            segment->normalizeRests(segment->getStartTime(),
                                    segment->getEndMarkerTime());
        }
        m_addedSegments.push_back(segment);
    }

    timeT pasteEndTime = m_pasteTime + (latestEndTime - earliestStartTime);

    m_composition->setPosition(pasteEndTime);

    if (m_composition->autoExpandEnabled()) {
        // If the composition needs expanding, do so...
        if (pasteEndTime > m_composition->getEndMarker())
            m_composition->setEndMarker(
                    m_composition->getBarEndForTime(pasteEndTime));
    }

    m_detached = false;
}

void
PasteSegmentsCommand::unexecute()
{
    for (size_t i = 0; i < m_addedSegments.size(); ++i) {
        m_composition->detachSegment(m_addedSegments[i]);
    }
    m_detached = true;

    // Restore the original composition end in case it was changed
    m_composition->setEndMarker(m_oldEndTime);
}

}
