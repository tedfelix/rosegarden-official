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

#ifndef RG_MIDIINSERTER_H
#define RG_MIDIINSERTER_H

#include "base/RealTime.h"
#include "base/TimeT.h"
#include "base/Track.h"
#include "sound/MappedInserterBase.h"
#include "sound/MidiFile.h"

#include <map>


namespace Rosegarden
{


class Composition;


/// Inserter used to generate a standard MIDI file.
/**
 * @see MidiFile::convertToMidi()
 *
 * @author Tom Breton (Tehom)
 */
class MidiInserter : public MappedInserterBase
{
public:

    MidiInserter(Composition &comp, int timingDivision, RealTime trueEnd);

    // Insert a (MidiEvent) copy of event.
    /**
     * @author Tom Breton (Tehom)
     */
    void insertCopy(const MappedEvent &event) override;

    void assignToMidiFile(MidiFile &midifile);

private:

    /// Describes and contains a track that we insert MidiEvents onto.
    /**
     * @author Tom Breton (Tehom)
     */
    struct TrackData
    {
        /// Insert and take ownership of a MidiEvent.
        /**
         * The event's time is converted from an absolute time to a time delta
         * relative to the previous time.
         *
         * @author Tom Breton (Tehom)
         */
        void insertMidiEvent(MidiEvent *event);
        /// Make and insert a tempo event.
        void insertTempo(timeT t, long tempo);
        void endTrack(timeT t);

        MidiFile::MidiTrack m_midiTrack;
        timeT m_previousTime;
    };

    typedef std::pair<TrackId, int /* channel */> TrackKey;
    typedef std::map<TrackKey, TrackData> TrackMap;

    /// Convert RealTime to timeT.
    /**
     * We don't convert time to a delta here because if we didn't end up
     * inserting the event, the new reference time that we made would be wrong.
     *
     * @author Tom Breton (Tehom)
     */
    timeT getAbsoluteTime(RealTime realtime) const;

    /// Initialize a normal track (not a conductor track)
    /**
     * @author Tom Breton (Tehom)
     */
    void initNormalTrack(TrackData &trackData, TrackId RGTrackPos) const;

    /// Get the track data for an RG track ID.
    /**
     * @author Tom Breton (Tehom)
     */
    TrackData &getTrackData(TrackId trackID, int channelNb);

    /// Get ready to receive events.
    /**
     * Assumes nothing is written to tracks yet.
     *
     * @author Tom Breton (Tehom)
     */
    void setup();

    /// Done receiving events.  Tracks will be complete when this returns.
    /**
     * @author Tom Breton (Tehom)
     */
    void finish();

    Composition &m_comp;

    // From RG track pos -> MIDI TrackData, the opposite direction
    // from MidiFile::m_trackChannelMap.
    TrackMap m_trackPosMap;

    // The conductor track, which is not part of the mapping.
    TrackData m_conductorTrack;

    // pulses per quarter note (PPQN)
    int m_timingDivision;

    bool m_finished{false};
    RealTime m_trueEnd;

    // To keep track of ramping.
    RealTime m_previousRealTime;
    timeT m_previousTime{0};
    bool m_ramping{false};

};


}

#endif
