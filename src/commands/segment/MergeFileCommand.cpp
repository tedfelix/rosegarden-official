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

#define RG_MODULE_STRING "[MergeFileCommand]"

#include "MergeFileCommand.h"

#include "base/Composition.h"
#include "misc/Debug.h"
#include "document/RosegardenDocument.h"
//#include "gui/application/RosegardenMainWindow.h"
//#include "base/Studio.h"

#include <algorithm>


namespace Rosegarden
{


MergeFileCommand::MergeFileCommand(RosegardenDocument *srcDoc,
                                   bool mergeAtEnd,
                                   bool mergeTimesAndTempos) :
    NamedCommand(tr("Merge File")),
    m_sourceDocument(srcDoc),
    m_mergeAtEnd(mergeAtEnd),
    m_mergeTimesAndTempos(mergeTimesAndTempos),
    m_undone(false)
{

}

MergeFileCommand::~MergeFileCommand()
{
    // If the Tracks are no longer in the Composition, we'll have
    // to delete them ourselves.
    if (m_undone) {
        for (size_t i = 0; i < m_newTracks.size(); ++i) {
            delete m_newTracks[i];
        }
    }
}

void MergeFileCommand::execute()
{
    RosegardenDocument *destDoc = RosegardenDocument::currentDocument;
    if (!destDoc)
        return;

    Composition &destComp = destDoc->getComposition();

    // If we are redoing...
    if (m_undone) {

        // ...

        // Switch back to "done" mode.
        m_undone = false;

        return;
    }

    // First time through...

    // Merging from the merge source (m_sourceDocument) into the merge
    // destination (destDoc).

    Composition &srcComp = m_sourceDocument->getComposition();

    // Destination start time.
    // ??? We should allow for "at cursor position" as well.
    timeT time0 = 0;
    if (m_mergeAtEnd)
        time0 = destComp.getBarEndForTime(destComp.getDuration() - 1);

    const int srcNrTracks = srcComp.getNbTracks();
    const int destMaxTrackPos = destComp.getNbTracks() - 1;

    // For each Track in the source Composition, create a new Track in the
    // destination Composition.
    for (int srcTrackPosition = 0;
         srcTrackPosition < srcNrTracks;
         ++srcTrackPosition)
    {
        const Track *srcTrack = srcComp.getTrackByPosition(srcTrackPosition);
        if (!srcTrack)
            continue;

        const TrackId destTrackId = destComp.getNewTrackId();

        // Create the Track
        Track *destTrack = new Track(destTrackId);
        destTrack->setPosition(destMaxTrackPos + 1 + srcTrackPosition);
        // Copy the Track label
        destTrack->setLabel(srcTrack->getLabel());
        // Copy the Track instrument type
        // ??? Definitely not just MidiInstrumentBase.
        //     AudioInstrumentBase and SoftSynthInstrumentBase too.
        //     We probably need to be smarter and search for a valid
        //     Device and use its first InstrumentId.
        destTrack->setInstrument(MidiInstrumentBase);

        // Add it to the Composition.
        destComp.addTrack(destTrack);

        // For undo.
        m_newTracks.push_back(destTrack);
    }

    // Destination.
    // ??? I don't think this is how this version will work.
    //const TrackId firstNewTrackId = destComp.getNewTrackId();
    // Keep track of the max end time so we can expand the composition
    // if needed.
    timeT maxEndTime = 0;

    // For each Segment in the source composition (srcComp), move it to the
    // destination composition (destComp).
    for (Composition::iterator i = srcComp.begin(), j = i;
         i != srcComp.end();
         i = j) {

        // Move to the next so that we can detach the current.
        ++j;
        Segment *segment = *i;

        Track *srcTrack = srcComp.getTrackById(segment->getTrack());
        if (!srcTrack)
            continue;

        const int destTrackPosition =
                destMaxTrackPos + 1 + srcTrack->getPosition();

        Track *destTrack = destComp.getTrackByPosition(destTrackPosition);
        if (!destTrack)
            continue;

        const TrackId destTrackId = destTrack->getId();

        // Move the Segment from the srcDoc to the dest (this).
        // Ok to detach as we've got j pointing to the next Segment.
        srcComp.detachSegment(segment);
        segment->setTrack(destTrackId);
        destComp.addSegment(segment);

        // Update maxEndTime
        timeT segmentEndTime = segment->getEndMarkerTime();
        if (m_mergeAtEnd) {
            segment->setStartTime(segment->getStartTime() + time0);
            segmentEndTime += time0;
        }
        if (segmentEndTime > maxEndTime)
            maxEndTime = segmentEndTime;
    }

    // Merge in time signatures and tempos from the merge source
    if (m_mergeTimesAndTempos) {
        // Copy time signatures from the merge source.
        for (int i = 0;
             i < srcComp.getTimeSignatureCount();
             ++i) {
            std::pair<timeT, TimeSignature> ts =
                    srcComp.getTimeSignatureChange(i);
            destComp.addTimeSignature(ts.first + time0, ts.second);
        }
        // Copy tempos from the merge source.
        for (int i = 0;
             i < srcComp.getTempoChangeCount();
             ++i) {
            std::pair<timeT, tempoT> t = srcComp.getTempoChange(i);
            destComp.addTempoAtTime(t.first + time0, t.second);
        }
    }

    // If the Composition needs to be expanded, expand it.
    if (maxEndTime > destComp.getEndMarker())
        destComp.setEndMarker(maxEndTime);

    // Make sure the center of the action is visible.
    // ???
    //emit makeTrackVisible(destMaxTrackPos + 1 + srcNrTracks/2 + 1);

    // This doesn't live past the first execution of the command.
    m_sourceDocument = nullptr;
}

void MergeFileCommand::unexecute()
{
    m_undone = true;
}


}
