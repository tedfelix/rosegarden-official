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
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "base/Studio.h"

#include <QApplication>

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
    m_compositionExpanded(false),
    m_undone(false)
{

}

MergeFileCommand::~MergeFileCommand()
{
    // If the Tracks/Segments are no longer in the Composition, we'll have
    // to delete them ourselves.
    if (m_undone) {
        for (size_t i = 0; i < m_newTracks.size(); ++i) {
            delete m_newTracks[i];
        }
        for (size_t i = 0; i < m_newSegments.size(); ++i) {
            delete m_newSegments[i];
        }
    }
}

// Find the first MIDI Device's first Instrument ID.
// ??? Specialized, but a candidate for promotion to Studio.
InstrumentId getFirstMidiInstrumentId(const Studio &destStudio)
{
    const DeviceList *deviceList = destStudio.getDevices();

    // For each Device in the Studio
    for (const Device *device : *deviceList) {
        // if this is not a MIDI device, try the next.
        if (device->getType() != Device::Midi)
            continue;
        // Skip input devices.
        if (device->isInput())
            continue;

        // Get the Device's Instruments.
        InstrumentList instruments = device->getPresentationInstruments();
        if (instruments.empty())
            return MidiInstrumentBase;

        // Return the Device's first Instrument ID
        return instruments[0]->getId();
    }

    return MidiInstrumentBase;
}

// Copy the type from the srcTrack to the destTrack.
void copyType(const Studio &srcStudio, const Track *srcTrack,
              const Studio &destStudio, Track *destTrack)
{
    const Instrument *instrument = srcStudio.getInstrumentFor(srcTrack);
    if (!instrument)
        return;

    switch (instrument->getType()) {
        case Instrument::Audio: {
            const Device *audioDevice = destStudio.getAudioDevice();
            const InstrumentId instrumentId =
                    audioDevice->getPresentationInstruments()[0]->getId();
            destTrack->setInstrument(instrumentId);
            break;
        }
        case Instrument::SoftSynth: {
            const Device *synthDevice = destStudio.getSoftSynthDevice();
            const InstrumentId instrumentId =
                    synthDevice->getPresentationInstruments()[0]->getId();
            destTrack->setInstrument(instrumentId);
            break;
        }
        case Instrument::InvalidInstrument:
        case Instrument::Midi:
        default: {
            // Use the first MIDI Device's first Instrument ID.
            destTrack->setInstrument(getFirstMidiInstrumentId(destStudio));
            break;
        }
    }
}

void MergeFileCommand::execute()
{
    // If we are redoing...
    if (m_undone) {
        redo();
        // Switch back to "done" mode.
        m_undone = false;
        return;
    }

    RosegardenDocument *destDoc = RosegardenDocument::currentDocument;
    if (!destDoc)
        return;

    Composition &destComp = destDoc->getComposition();

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

    // Keep a list for Composition notification.
    std::vector<TrackId> trackIds;

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
        trackIds.push_back(destTrackId);

        // Create the Track
        Track *destTrack = new Track(destTrackId);
        destTrack->setPosition(destMaxTrackPos + 1 + srcTrackPosition);
        // Copy the Track label
        destTrack->setLabel(srcTrack->getLabel());

        // Copy the Track Instrument type
        copyType(m_sourceDocument->getStudio(), srcTrack,
                 destDoc->getStudio(), destTrack);

        // Add it to the Composition.
        destComp.addTrack(destTrack);

        // For undo.
        m_newTracks.push_back(destTrack);
    }

    // ??? Too early?
    destComp.notifyTracksAdded(trackIds);

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

        // Save for redo.
        m_newSegments.push_back(segment);

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
        // For each time signature in the source document
        for (int i = 0;
             i < srcComp.getTimeSignatureCount();
             ++i) {
            std::pair<timeT, TimeSignature> ts =
                    srcComp.getTimeSignatureChange(i);

            const timeT targetTime = ts.first + time0;

            // Check for clobber
            const int clobberNum = destComp.getTimeSignatureNumberAt(ts.first);
            std::pair<timeT, TimeSignature> clobberTS =
                    destComp.getTimeSignatureChange(clobberNum);
            // If there is already a time signature at this exact time
            if (clobberTS.first == targetTime) {
                // It will be clobbered.  Add to clobbered time signature list.
                m_clobberedTimeSignatures[clobberTS.first] = clobberTS.second;
            }

            destComp.addTimeSignature(targetTime, ts.second);

            // For undo.
            m_newTimeSignatures[targetTime] = ts.second;
        }

        // Copy tempos from the merge source.
        // For each tempo change in the source document
        for (int i = 0;
             i < srcComp.getTempoChangeCount();
             ++i) {
            std::pair<timeT, tempoT> t = srcComp.getTempoChange(i);

            const timeT targetTime = t.first + time0;

            // Check for clobber
            const int clobberNum = destComp.getTempoChangeNumberAt(t.first);
            std::pair<timeT, tempoT> clobberedTempoChange =
                    destComp.getTempoChange(clobberNum);
            // If there is already a tempo change at this exact time
            if (clobberedTempoChange.first == targetTime) {
                // It will be clobbered.  Add to clobbered tempo change list.
                m_clobberedTempoChanges[clobberedTempoChange.first] =
                        clobberedTempoChange.second;
            }

            destComp.addTempoAtTime(targetTime, t.second);

            // For undo.
            m_newTempoChanges[targetTime] = t.second;
        }
    }

    // If the Composition needs to be expanded, expand it.
    if (maxEndTime > destComp.getEndMarker()) {
        // For undo.  Capture before we modify.
        m_oldCompositionEnd = destComp.getEndMarker();

        // Expand the Composition.
        // ??? Check against ChangeCompositionLengthCommand.  Is there anything
        //     else that needs doing?
        destComp.setEndMarker(maxEndTime);

        m_compositionExpanded = true;
        m_newCompositionEnd = maxEndTime;
    }

    // Make sure the center of the action is visible.
    // ??? Or just make the last Track visible?
    //emit makeTrackVisible(destMaxTrackPos + 1 + srcNrTracks/2 + 1);

    // This doesn't live past the first execution of the command.
    // ??? Can we move all of RosegardenMainWindow::mergeFile()
    //     and RosegardenDocument::mergeDocument() into here?
    //     Then we can move the source document read into here.
    //     I think the answer is "no".  The issue is that both the
    //     file selection dialog and FileMergeDialog offer Cancel buttons.
    //     And there's no way to cancel a command from within its execute().
    //     At least I don't think there is.  I think we'll stick to this.
    m_sourceDocument = nullptr;
}

void MergeFileCommand::unexecute()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;

    Composition &composition = document->getComposition();

    // Remove the added Tracks from the composition.

    // Keep a list for Composition notification.
    std::vector<TrackId> trackIds;

    // For each new Track, detach it from the Composition.
    for (size_t trackIndex = 0; trackIndex < m_newTracks.size(); ++trackIndex) {
        const TrackId trackId = m_newTracks[trackIndex]->getId();

        const SegmentMultiSet &segments = composition.getSegments();

        // For each Segment in the Composition.
        for (SegmentMultiSet::const_iterator segmentIter = segments.begin();
             segmentIter != segments.end();
             /* incremented inside */) {
            // Increment before use.  Otherwise detachSegment() will
            // invalidate our iterator.
            SegmentMultiSet::const_iterator segmentIter2 = segmentIter++;

            // If this Segment is on the track we are deleting
            if ((*segmentIter2)->getTrack() == trackId) {
                // Remove the Segment from the Composition.
                composition.detachSegment(*segmentIter2);
            }
        }

        composition.detachTrack(m_newTracks[trackIndex]);
        trackIds.push_back(trackId);
    }

    composition.notifyTracksDeleted(trackIds);

    // Have to refresh the SequenceManager before continuing.
    // Otherwise it will still have the old Segments.
    RosegardenMainWindow::self()->getSequenceManager()->update();
    // Empty the queue as the above calls postEvent() instead of sendEvent().
    QApplication::processEvents();

    // Remove the time signatures
    for (const TimeSignatureMap::value_type &pair : m_newTimeSignatures) {
        const int timeSignatureNumber =
                composition.getTimeSignatureNumberAt(pair.first);
        composition.removeTimeSignature(timeSignatureNumber);
    }

    // Put back the clobbered time signatures.
    for (const TimeSignatureMap::value_type &pair : m_clobberedTimeSignatures) {
        composition.addTimeSignature(pair.first, pair.second);
    }

    // Remove the tempos
    for (const TempoChangeMap::value_type &pair : m_newTempoChanges) {
        const int tempoChangeNumber =
                composition.getTempoChangeNumberAt(pair.first);
        composition.removeTempoChange(tempoChangeNumber);
    }

    // Put back the clobbered time signatures.
    for (const TempoChangeMap::value_type &pair : m_clobberedTempoChanges) {
        composition.addTempoAtTime(pair.first, pair.second);
    }

    // Reverse any Composition expansion.
    // ??? Check against ChangeCompositionLengthCommand.  Is there anything
    //     else that needs doing?
    if (m_compositionExpanded)
        composition.setEndMarker(m_oldCompositionEnd);

    // More?


    m_undone = true;
}

void MergeFileCommand::redo()
{
    RosegardenDocument *document = RosegardenDocument::currentDocument;
    if (!document)
        return;

    Composition &composition = document->getComposition();

    // Keep a list for Composition notification.
    std::vector<TrackId> trackIds;

    // Add the Tracks.
    for (size_t trackIndex = 0; trackIndex < m_newTracks.size(); ++trackIndex) {
        composition.addTrack(m_newTracks[trackIndex]);
        trackIds.push_back(m_newTracks[trackIndex]->getId());
    }

    // ??? Too early?
    composition.notifyTracksAdded(trackIds);

    // Add the Segments.
    composition.addAllSegments(m_newSegments);

    // ??? At this point, I suspect the SequencerManager needs to be
    //     updated like we do for undo.  Otherwise the new Segments will
    //     not be aware of their tempo changes.

    // Add the time signatures.
    for (const TimeSignatureMap::value_type &pair : m_newTimeSignatures) {
        composition.addTimeSignature(pair.first, pair.second);
    }

    // Add the tempos.
    for (const TempoChangeMap::value_type &pair : m_newTempoChanges) {
        composition.addTempoAtTime(pair.first, pair.second);
    }

    // Expand the composition.
    // ??? Check against ChangeCompositionLengthCommand.  Is there anything
    //     else that needs doing?
    if (m_compositionExpanded)
        composition.setEndMarker(m_newCompositionEnd);

    // Make sure the center of the action is visible.
    // ??? Or just make the last Track visible?
    //emit makeTrackVisible(destMaxTrackPos + 1 + srcNrTracks/2 + 1);
}


}
