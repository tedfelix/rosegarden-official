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

#ifndef RG_MIDIINSERTER_H
#define RG_MIDIINSERTER_H

#include "base/RealTime.h"
#include "sound/MappedInserterBase.h"
#include "sound/MidiFile.h"

namespace Rosegarden
{

/// Inserter used to generate a standard MIDI file.
/**
 * @see MidiFile::convertToMidi()
 *
 * @author Tom Breton (Tehom)
 */
class MidiInserter : public MappedInserterBase
{
    // @class MidiInserter::TrackData describes and contains a track
    // that we insert MidiEvents onto.
    // @author Tom Breton (Tehom)
    struct TrackData
    {
        // Insert and take ownership of a MidiEvent.  The event's time
        // is converted from an absolute time to a time delta relative
        // to the previous time.
        void insertMidiEvent(MidiEvent *event);
        // Make and insert a tempo event.
        void insertTempo(timeT t, long tempo);
        void endTrack(timeT t);
        MidiFile::MidiTrack m_midiTrack;
        timeT     m_previousTime;
    };

    typedef std::pair<TrackId, int> TrackKey;
    typedef std::map<TrackKey, TrackData> TrackMap;
    typedef TrackMap::iterator TrackIterator;

 public:
    MidiInserter(Composition &composition, int timingDivision, RealTime trueEnd);

    void insertCopy(const MappedEvent &evt) override;

    void assignToMidiFile(MidiFile &midifile);
        
 private:

    // Get the absolute time of evt
    timeT getAbsoluteTime(RealTime time);

    // Initialize a normal track, ie not a conductor track.
    void initNormalTrack(TrackData &track, TrackId RGTrackPos);

    // Get the relevant MIDI track data for a rosegarden track
    // position, including the track itself.
    TrackData &getTrackData(TrackId RGTrackPos, int channelNb);

    // Get ready to receive events.  Assumes nothing is written to
    // tracks yet.
    void setup();

    // Done receiving events.  Tracks will be complete when this
    // returns.
    void finish();
 
    Composition   &m_comp;
    // From RG track pos -> MIDI TrackData, the opposite direction
    // from m_trackChannelMap.
    TrackMap       m_trackPosMap;

    // The conductor track, which is not part of the mapping.
    TrackData      m_conductorTrack;
    int            m_timingDivision;   // pulses per quarter note
    bool           m_finished;
    RealTime       m_trueEnd;

    // To keep track of ramping.
    RealTime       m_previousRealTime;
    timeT          m_previousTime;
    bool           m_ramping;
    
    /* Static constants */

    static const timeT crotchetDuration;
    
};
}

#endif /* ifndef RG_MIDIINSERTER_H */
