/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiInserter]"
#define RG_NO_DEBUG_PRINT 1

#include "MidiInserter.h"

#include "MidiEvent.h"
#include "base/Composition.h"
#include "base/MidiTypes.h"
#include "misc/Debug.h"
#include "sound/MidiFile.h"
#include "sound/MappedEvent.h"

#include <QtGlobal>

#include <string>

#define MIDI_DEBUG 1

namespace Rosegarden
{
    /*** TrackData ***/

// Insert and take ownership of a MidiEvent.  The event's time is
// converted from an absolute time to a time delta relative to the
// previous time.
// @author Tom Breton (Tehom)
void
MidiInserter::TrackData::
insertMidiEvent(MidiEvent *event)
{
    timeT absoluteTime = event->getTime();
    timeT delta        = absoluteTime - m_previousTime;
    if (delta < 0)
        { delta = 0; }
    else
        { m_previousTime = absoluteTime; }
    event->setTime(delta);
#ifdef MIDI_DEBUG
    RG_DEBUG << "Converting absoluteTime" << (int)absoluteTime
             << "to delta" << (int)delta
             << endl;
#endif
    m_midiTrack.push_back(event);
}

void
MidiInserter::TrackData::
endTrack(timeT t)
{
    // Safe even if t is too early in timeT because insertMidiEvent
    // fixes it.
    insertMidiEvent
        (new MidiEvent(t, MIDI_FILE_META_EVENT,
                       MIDI_END_OF_TRACK, ""));
}

void
MidiInserter::TrackData::
insertTempo(timeT t, long tempo)
{
    double qpm = Composition::getTempoQpm(tempo);
    long tempoValue = long(60000000.0 / qpm + 0.01);

    std::string tempoString;
    tempoString += (MidiByte) ( tempoValue >> 16 & 0xFF );
    tempoString += (MidiByte) ( tempoValue >> 8 & 0xFF );
    tempoString += (MidiByte) ( tempoValue & 0xFF );


    insertMidiEvent
        (new MidiEvent(t,
                       MIDI_FILE_META_EVENT,
                       MIDI_SET_TEMPO,
                       tempoString));
}

    /*** MidiInserter ***/
const timeT MidiInserter::crotchetDuration =
    Note(Note::Crotchet).getDuration();

MidiInserter::
MidiInserter(Composition &composition, int timingDivision, RealTime trueEnd) :
    m_comp(composition),
    m_timingDivision(timingDivision),
    m_finished(false),
    m_trueEnd(trueEnd),
    m_previousRealTime(RealTime::zeroTime),
    m_previousTime(0),
    m_ramping(false)
{ setup(); }

// Get the absolute RG time of evt.  We don't convert time to a delta
// here because if we didn't end up inserting the event, the new
// reference time that we made would be wrong.
// @author Tom Breton (Tehom)
timeT
MidiInserter::
getAbsoluteTime(RealTime realtime)
{
    timeT time = m_comp.getElapsedTimeForRealTime(realtime);
    timeT retVal = (time * m_timingDivision) / crotchetDuration;
#ifdef MIDI_DEBUG
    RG_DEBUG << "Converting RealTime" << realtime
             << "to timeT" << retVal
             << "intermediate" << time
             << endl;
#endif

    return retVal;
}

// Initialize a normal track (not a conductor track)
// @author Tom Breton (Tehom)
// Adapted from MidiFile.cpp
void
MidiInserter::
initNormalTrack(TrackData &trackData, TrackId RGTrackPos)
{
    Track *track = m_comp.getTrackById(RGTrackPos);
    trackData.m_previousTime = 0;
    trackData.
        insertMidiEvent
        (new MidiEvent(0,
                       MIDI_FILE_META_EVENT,
                       MIDI_TRACK_NAME,
                       track->getLabel()));
}

// Return the respective track data, creating it if needed.
// @author Tom Breton (Tehom)
MidiInserter::TrackData &
MidiInserter::
getTrackData(TrackId RGTrackPos, int channelNb)
{
#ifdef MIDI_DEBUG
    RG_DEBUG << "Getting track " << (int)RGTrackPos;
#endif
    // Some events like TimeSig and Tempo have invalid trackId and
    // should be written on the conductor track.
    if (RGTrackPos == NO_TRACK)
        { return m_conductorTrack; }
    // Otherwise we're looking it up.
    TrackKey key = TrackKey(RGTrackPos, channelNb);
    // If we are starting a new track, initialize it.
   if (m_trackPosMap.find(key) == m_trackPosMap.end()) {
         initNormalTrack(m_trackPosMap[key], RGTrackPos);
    }
    return m_trackPosMap[key];
}

// Get ready to receive events.  Assumes nothing is written to
// tracks yet.
// @author Tom Breton (Tehom)
// Adapted from MidiFile.cpp
void
MidiInserter::
setup()
{
    m_conductorTrack.m_previousTime = 0;
    
    // Insert the Rosegarden Signature Track here and any relevant
    // file META information - this will get written out just like
    // any other MIDI track.
    //
    m_conductorTrack.
        insertMidiEvent
        (new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_COPYRIGHT_NOTICE,
                       m_comp.getCopyrightNote()));

    m_conductorTrack.
        insertMidiEvent
        (new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_CUE_POINT,
                       "Created by Rosegarden"));

    m_conductorTrack.
        insertMidiEvent
        (new MidiEvent(0, MIDI_FILE_META_EVENT, MIDI_CUE_POINT,
                       "http://www.rosegardenmusic.com/"));
}

// Done receiving events.  Tracks will be complete when this returns.
// @author Tom Breton (Tehom)
void
MidiInserter::
finish()
{
    if(m_finished) { return; }
    timeT endOfComp = getAbsoluteTime(m_trueEnd);
    m_conductorTrack.endTrack(endOfComp);
    for (TrackIterator i = m_trackPosMap.begin();
         i != m_trackPosMap.end();
         ++i) {
        i->second.endTrack(endOfComp);
    }
    m_finished = true;
}

// Insert a (MidiEvent) copy of evt.
// @author Tom Breton (Tehom)
// Adapted from MidiFile.cpp
void
MidiInserter::
insertCopy(const MappedEvent &evt)
{
    Q_ASSERT(!m_finished);

    MidiByte   midiChannel = evt.getRecordedChannel();
    TrackData& trackData   = getTrackData(evt.getTrackId(), midiChannel);
    timeT      midiEventAbsoluteTime = getAbsoluteTime(evt.getEventTime());

    // If we are ramping, calculate a previous tempo that would get us
    // to this event at this time and pre-insert it, unless this
    // event's time is the same as last.
    if (m_ramping && (midiEventAbsoluteTime != m_previousTime)) {
        RealTime diffReal = evt.getEventTime()    - m_previousRealTime;
        // We undo the scaling getAbsoluteTime does.
        timeT    diffTime =
            (midiEventAbsoluteTime - m_previousTime) *
            crotchetDuration /
            m_timingDivision;

        tempoT bridgingTempo =
            Composition::timeRatioToTempo(diffReal, diffTime, -1);

        trackData.insertTempo(m_previousTime, bridgingTempo);
        m_previousRealTime = evt.getEventTime();
        m_previousTime     = midiEventAbsoluteTime;
    }
#ifdef MIDI_DEBUG
    RG_DEBUG << "Inserting an event for channel " << (int)midiChannel + 1;
#endif

    try {
        switch (evt.getType())
            {
            case MappedEvent::Tempo:
                {
                    m_ramping = (evt.getData1() > 0) ? true : false;
                    // Yes, we fetch it from "instrument" because
                    // that's what TempoSegmentMapper puts it in.
                    tempoT tempo = evt.getInstrument();
                    trackData.insertTempo(midiEventAbsoluteTime, tempo);
                    break;
                }
            case MappedEvent::TimeSignature:
                {
                    int numerator   = evt.getData1();
                    int denominator = evt.getData2();
                    timeT beatDuration =
                        TimeSignature(numerator, denominator).
                        getBeatDuration();

                    std::string timeSigString;
                    timeSigString += (MidiByte) numerator;
                    int denPowerOf2 = 0;

                    // Work out how many powers of two are in the denominator
                    //
                    {
                        int denominatorCopy = denominator;
                        while (denominatorCopy >>= 1)
                            { denPowerOf2++; }
                    }

                    timeSigString += (MidiByte) denPowerOf2;
                
                    // The third byte is the number of MIDI clocks per beat.
                    // There are 24 clocks per quarter-note (the MIDI clock
                    // is tempo-independent and is not related to the timebase).
                    //
                    int cpb = 24 * beatDuration / crotchetDuration;
                    timeSigString += (MidiByte) cpb;

                    // And the fourth byte is always 8, for us (it expresses
                    // the number of notated 32nd-notes in a MIDI quarter-note,
                    // for applications that may want to notate and perform
                    // in different units)
                    //
                    timeSigString += (MidiByte) 8;

                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_FILE_META_EVENT,
                                       MIDI_TIME_SIGNATURE,
                                       timeSigString));

                    break;
                }
            case MappedEvent::MidiController:
                {
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_CTRL_CHANGE | midiChannel,
                                       evt.getData1(), evt.getData2()));

                    break;
                }
            case MappedEvent::MidiProgramChange:
                {
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_PROG_CHANGE | midiChannel,
                                       evt.getData1()));
                    break;
                }

            case MappedEvent::MidiNote:
            case MappedEvent::MidiNoteOneShot:
                {
                    MidiByte pitch         = evt.getData1();
                    MidiByte midiVelocity  = evt.getData2();

                    if ((evt.getType() == MappedEvent::MidiNote) &&
                        (midiVelocity == 0)) {
                        // It's actually a NOTE_OFF.
                        // "MIDI devices that can generate Note Off
                        // messages, but don't implement velocity
                        // features, will transmit Note Off messages
                        // with a preset velocity of 64"
                        trackData.
                            insertMidiEvent
                            (new MidiEvent(midiEventAbsoluteTime,
                                           MIDI_NOTE_OFF | midiChannel,
                                           pitch,
                                           64));
                    } else {
                        // It's a NOTE_ON.
                        trackData.
                            insertMidiEvent
                            (new MidiEvent(midiEventAbsoluteTime,
                                           MIDI_NOTE_ON | midiChannel,
                                           pitch,
                                           midiVelocity));
                    }
                    break;
                }
            case MappedEvent::MidiPitchBend:
                {
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_PITCH_BEND | midiChannel,
                                       evt.getData2(), evt.getData1()));
                    break;
                }

            case MappedEvent::MidiSystemMessage:
                {
                    std::string data = 
                        DataBlockRepository::getInstance()->
                        getDataBlockForEvent(&evt);

                    // check for closing EOX and add one if none found
                    //
                    if (MidiByte(data[data.length() - 1]) != MIDI_END_OF_EXCLUSIVE) {
                        data += (char)MIDI_END_OF_EXCLUSIVE;
                    }

                    // construct plain SYSEX event
                    //
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_SYSTEM_EXCLUSIVE,
                                       data));

                    break;
                }

            case MappedEvent::MidiChannelPressure:
                {
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_CHNL_AFTERTOUCH | midiChannel,
                                       evt.getData1()));

                    break;
                }
            case MappedEvent::MidiKeyPressure:
                {
                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_POLY_AFTERTOUCH | midiChannel,
                                       evt.getData1(), evt.getData2()));

                    break;
                }

            case MappedEvent::Marker:
                {
                    std::string metaMessage = 
                        DataBlockRepository::getInstance()->
                        getDataBlockForEvent(&evt);

                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_FILE_META_EVENT,
                                       MIDI_TEXT_MARKER,
                                       metaMessage));

                    break;
                }

            case MappedEvent::Text:
                {
                    MidiByte midiTextType = evt.getData1();

                    std::string metaMessage = 
                        DataBlockRepository::getInstance()->
                        getDataBlockForEvent(&evt);

                    trackData.
                        insertMidiEvent
                        (new MidiEvent(midiEventAbsoluteTime,
                                       MIDI_FILE_META_EVENT,
                                       midiTextType,
                                       metaMessage));
                    break;
                }

            
                // Pacify compiler warnings about missed cases.
            case MappedEvent::InvalidMappedEvent:
            case MappedEvent::Audio:
            case MappedEvent::AudioCancel:
            case MappedEvent::AudioLevel:
            case MappedEvent::AudioStopped:
            case MappedEvent::AudioGeneratePreview:
            case MappedEvent::SystemUpdateInstruments:
            case MappedEvent::SystemJackTransport:
            case MappedEvent::SystemMMCTransport:
            case MappedEvent::SystemMIDIClock:
            case MappedEvent::SystemMetronomeDevice:
            case MappedEvent::SystemAudioPortCounts:
            case MappedEvent::SystemAudioPorts:
            case MappedEvent::SystemFailure:
            case MappedEvent::Panic:
            case MappedEvent::SystemMTCTransport:
            case MappedEvent::SystemMIDISyncAuto:
            case MappedEvent::SystemAudioFileFormat:
            default:
                break;
            }

    } catch (const Event::NoData &d) {
#ifdef MIDI_DEBUG
        RG_DEBUG << "Caught Event::NoData at " << midiEventAbsoluteTime << ", message is:" << d.getMessage();
#endif

    } catch (const Event::BadType &b) {
#ifdef MIDI_DEBUG
        RG_DEBUG << "Caught Event::BadType at " << midiEventAbsoluteTime << ", message is:" << b.getMessage();
#endif

    } catch (const SystemExclusive::BadEncoding &e) {
#ifdef MIDI_DEBUG
        RG_DEBUG << "Caught bad SysEx encoding at " << midiEventAbsoluteTime;
#endif

    }
}
void
MidiInserter::
assignToMidiFile(MidiFile &midifile)
{
    finish();

    midifile.clearMidiComposition();

    // We leave out fields that write doesn't look at.
    //
    midifile.m_numberOfTracks = m_trackPosMap.size() + 1;
    midifile.m_timingDivision = m_timingDivision;
    midifile.m_format         = MidiFile::MIDI_SIMULTANEOUS_TRACK_FILE;

    midifile.m_midiComposition[0] = m_conductorTrack.m_midiTrack;
    unsigned int index = 0;
    for (TrackIterator i = m_trackPosMap.begin();
         i != m_trackPosMap.end();
         ++i, ++index) {
        midifile.m_midiComposition[index + 1] =
            i->second.m_midiTrack;
    }
}

}
