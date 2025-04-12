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

#define RG_MODULE_STRING "[MidiInserter]"
#define RG_NO_DEBUG_PRINT 1

#include "MidiInserter.h"

#include "MidiEvent.h"

#include "base/Composition.h"
#include "base/MidiTypes.h"
#include "misc/Debug.h"
#include "sound/MidiFile.h"
#include "sound/MappedEvent.h"

//#include <QtGlobal>

#include <string>

//#define MIDI_DEBUG


namespace Rosegarden
{


// *********************************************************************
// MidiInserter::TrackData
// *********************************************************************

void
MidiInserter::TrackData::insertMidiEvent(MidiEvent *event)
{
    const timeT absoluteTime = event->getTime();

    timeT delta = absoluteTime - m_previousTime;
    if (delta < 0)
        delta = 0;
    else
        m_previousTime = absoluteTime;

    event->setTime(delta);

#ifdef MIDI_DEBUG
    RG_DEBUG << "Converting absoluteTime" << (int)absoluteTime << "to delta" << (int)delta;
#endif

    m_midiTrack.push_back(event);
}

void
MidiInserter::TrackData::endTrack(timeT time)
{
    // Safe even if it is too early in timeT because insertMidiEvent()
    // fixes it.
    insertMidiEvent(new MidiEvent(
            time,
            MIDI_FILE_META_EVENT,
            MIDI_END_OF_TRACK,
            ""));
}

void
MidiInserter::TrackData::insertTempo(timeT time, long tempo)
{
    const double qpm = Composition::getTempoQpm(tempo);
    const long tempoValue = long(60000000.0 / qpm + 0.01);

    std::string tempoString;
    tempoString += MidiByte(tempoValue >> 16 & 0xFF);
    tempoString += MidiByte(tempoValue >> 8 & 0xFF);
    tempoString += MidiByte(tempoValue & 0xFF);

    insertMidiEvent(new MidiEvent(
            time,
            MIDI_FILE_META_EVENT,
            MIDI_SET_TEMPO,
            tempoString));
}

// *********************************************************************
// MidiInserter
// *********************************************************************

static const timeT crotchetDuration = Note(Note::Crotchet).getDuration();

MidiInserter::MidiInserter(
        Composition &comp, int timingDivision, RealTime trueEnd) :
    m_comp(comp),
    m_timingDivision(timingDivision),
    m_trueEnd(trueEnd)
{
    setup();
}

timeT
MidiInserter::getAbsoluteTime(RealTime realtime) const
{
    const timeT time = m_comp.getElapsedTimeForRealTime(realtime);

    RG_DEBUG << "getAbsoluteTime():" << realtime << time;

    const timeT retVal = (time * m_timingDivision) / crotchetDuration;

#ifdef MIDI_DEBUG
    RG_DEBUG << "Converting RealTime" << realtime
             << "to timeT" << retVal
             << "intermediate" << time;
#endif

    return retVal;
}

void
MidiInserter::initNormalTrack(TrackData &trackData, TrackId RGTrackPos) const
{
    // Adapted from MidiFile.cpp

    const Track *track = m_comp.getTrackById(RGTrackPos);

    trackData.m_previousTime = 0;
    trackData.insertMidiEvent(new MidiEvent(
            0,  // time
            MIDI_FILE_META_EVENT,  // eventCode
            MIDI_TRACK_NAME,  // metaEventCode
            track->getLabel()));  // metaMessage
}

MidiInserter::TrackData &
MidiInserter::getTrackData(TrackId trackID, int channelNb)
{
#ifdef MIDI_DEBUG
    RG_DEBUG << "Getting track " << trackID;
#endif

    // Some events like TimeSig and Tempo have invalid trackId and
    // should be written on the conductor track.
    if (trackID == NoTrack)
        return m_conductorTrack;

    // Otherwise we're looking it up.
    const TrackKey key = TrackKey(trackID, channelNb);
    // If we are starting a new track, initialize it.
    if (m_trackPosMap.find(key) == m_trackPosMap.end())
         initNormalTrack(m_trackPosMap[key], trackID);

    return m_trackPosMap[key];
}

void
MidiInserter::setup()
{
    // Adapted from MidiFile.cpp

    m_conductorTrack.m_previousTime = 0;

    // Insert the Rosegarden Signature Track here and any relevant
    // file META information - this will get written out just like
    // any other MIDI track.
    //
    m_conductorTrack.insertMidiEvent(new MidiEvent(
            0,  // time
            MIDI_FILE_META_EVENT,
            MIDI_COPYRIGHT_NOTICE,
            m_comp.getCopyrightNote()));

    m_conductorTrack.insertMidiEvent(new MidiEvent(
            0,  // time
            MIDI_FILE_META_EVENT,
            MIDI_CUE_POINT,
            "Created by Rosegarden"));

    m_conductorTrack.insertMidiEvent(new MidiEvent(
            0,  // time
            MIDI_FILE_META_EVENT,
            MIDI_CUE_POINT,
            "http://www.rosegardenmusic.com/"));
}

void
MidiInserter::finish()
{
    if (m_finished)
        return;

    const timeT endOfComp = getAbsoluteTime(m_trueEnd);

    m_conductorTrack.endTrack(endOfComp);

    // For each track, end the track.
    for (TrackMap::iterator i = m_trackPosMap.begin();
         i != m_trackPosMap.end();
         ++i) {
        i->second.endTrack(endOfComp);
    }

    m_finished = true;
}

void
MidiInserter::insertCopy(const MappedEvent &event)
{
    // Adapted from MidiFile.cpp

    if (m_finished)
        return;

    const MidiByte midiChannel = event.getRecordedChannel();
    TrackData &trackData = getTrackData(event.getTrackId(), midiChannel);
    const timeT midiEventAbsoluteTime = getAbsoluteTime(event.getEventTime());

#ifdef BUG1627
    // to avoid negative times here we subtract the start time
    timeT start = m_comp.getStartMarker();
    start = (start * m_timingDivision) / crotchetDuration;
    midiEventAbsoluteTime -= start;
#endif

    // If we are ramping, calculate a previous tempo that would get us
    // to this event at this time and pre-insert it, unless this
    // event's time is the same as last.
    if (m_ramping  &&  midiEventAbsoluteTime != m_previousTime) {
        const RealTime diffReal = event.getEventTime() - m_previousRealTime;
        // We undo the scaling getAbsoluteTime does.
        const timeT diffTime =
            (midiEventAbsoluteTime - m_previousTime) *
            crotchetDuration /
            m_timingDivision;

        const tempoT bridgingTempo =
            Composition::timeRatioToTempo(diffReal, diffTime, -1);

        trackData.insertTempo(m_previousTime, bridgingTempo);
        m_previousRealTime = event.getEventTime();
        m_previousTime = midiEventAbsoluteTime;
    }
#ifdef MIDI_DEBUG
    RG_DEBUG << "Inserting an event for channel " << (int)midiChannel + 1;
#endif

    try {
        switch (event.getType()) {
        case MappedEvent::Tempo:
            {
                m_ramping = (event.getData1() > 0);

                // TempoSegmentMapper::mapATempo() puts the tempo in the
                // instrument ID.
                const tempoT tempo = event.getInstrumentId();

                RG_DEBUG << "insertCopy tempo" << event.getEventTime() << midiEventAbsoluteTime << tempo;

                trackData.insertTempo(midiEventAbsoluteTime, tempo);

                break;
            }

        case MappedEvent::TimeSignature:
            {
                const int numerator = event.getData1();
                const int denominator = event.getData2();
                const timeT beatDuration =
                    TimeSignature(numerator, denominator).getBeatDuration();

                std::string timeSigString;
                timeSigString += MidiByte(numerator);

                // Work out how many powers of two are in the denominator.
                // Log base 2.
                int denPowerOf2 = 0;
                {
                    int denominatorCopy = denominator;
                    while (denominatorCopy >>= 1) {
                        ++denPowerOf2;
                    }
                }

                timeSigString += MidiByte(denPowerOf2);

                // The third byte is the number of MIDI clocks per beat.
                // There are 24 clocks per quarter-note (the MIDI clock
                // is tempo-independent and is not related to the timebase).
                const int cpb = 24 * beatDuration / crotchetDuration;
                timeSigString += MidiByte(cpb);

                // And the fourth byte is always 8, for us (it expresses
                // the number of notated 32nd-notes in a MIDI quarter-note,
                // for applications that may want to notate and perform
                // in different units)
                timeSigString += MidiByte(8);

                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_FILE_META_EVENT,
                        MIDI_TIME_SIGNATURE,
                        timeSigString));

                break;
            }

        case MappedEvent::MidiController:

            trackData.insertMidiEvent(new MidiEvent(
                    midiEventAbsoluteTime,
                    MIDI_CTRL_CHANGE | midiChannel,
                    event.getData1(),
                    event.getData2()));

            break;

        case MappedEvent::MidiProgramChange:

            trackData.insertMidiEvent(new MidiEvent(
                    midiEventAbsoluteTime,
                    MIDI_PROG_CHANGE | midiChannel,
                    event.getData1()));

            break;

        case MappedEvent::MidiNote:
        case MappedEvent::MidiNoteOneShot:
            {
                const MidiByte pitch = event.getData1();
                const MidiByte midiVelocity = event.getData2();

                RG_DEBUG << "insertCopy note" << event.getEventTime() << midiEventAbsoluteTime << pitch << midiVelocity;

                if (event.getType() == MappedEvent::MidiNote  &&
                    midiVelocity == 0) {
                    // It's actually a NOTE_OFF.
                    // "MIDI devices that can generate Note Off
                    // messages, but don't implement velocity
                    // features, will transmit Note Off messages
                    // with a preset velocity of 64"
                    trackData.insertMidiEvent(new MidiEvent(
                            midiEventAbsoluteTime,
                            MIDI_NOTE_OFF | midiChannel,
                            pitch,
                            64));
                } else {
                    // It's a NOTE_ON.
                    trackData.insertMidiEvent(new MidiEvent(
                            midiEventAbsoluteTime,
                            MIDI_NOTE_ON | midiChannel,
                            pitch,
                            midiVelocity));
                }

                break;
            }

        case MappedEvent::MidiPitchBend:

            trackData.insertMidiEvent(new MidiEvent(
                    midiEventAbsoluteTime,
                    MIDI_PITCH_BEND | midiChannel,
                    event.getData2(),
                    event.getData1()));

            break;

        case MappedEvent::MidiSystemMessage:
            {
                std::string data = DataBlockRepository::getInstance()->
                        getDataBlockForEvent(&event);

                // No EOX, add it.
                if (MidiByte(data[data.length() - 1]) != MIDI_END_OF_EXCLUSIVE)
                    data += (char)MIDI_END_OF_EXCLUSIVE;

                // construct plain SYSEX event
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_SYSTEM_EXCLUSIVE,
                        data));

                break;
            }

        case MappedEvent::MidiChannelPressure:

            trackData.insertMidiEvent(new MidiEvent(
                    midiEventAbsoluteTime,
                    MIDI_CHNL_AFTERTOUCH | midiChannel,
                    event.getData1()));

            break;

        case MappedEvent::MidiKeyPressure:

            trackData.insertMidiEvent(new MidiEvent(
                    midiEventAbsoluteTime,
                    MIDI_POLY_AFTERTOUCH | midiChannel,
                    event.getData1(),
                    event.getData2()));

            break;

        case MappedEvent::MidiRPN:
            {
                // Create the set of CCs required for the RPN.

                const int rpn = event.getNumber();

                // BnH 65H <RPN MSB>
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x65,
                        (rpn >> 7) & 0x7F));

                // BnH 64H <RPN LSB>
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x64,
                        rpn & 0x7F));

                const int value = event.getValue();

                // BnH 06H <Value MSB>  (Data Entry MSB)
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x06,
                        (value >> 7) & 0x7F));

                // BnH 26H <Value LSB>  (Data Entry LSB)
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x26,
                        value & 0x7F));

                break;
            }

        case MappedEvent::MidiNRPN:
            {
                // Create the set of CCs required for the NRPN.

                const int nrpn = event.getNumber();

                // BnH 65H <RPN MSB>
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x63,
                        (nrpn >> 7) & 0x7F));

                // BnH 64H <RPN LSB>
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x62,
                        nrpn & 0x7F));

                const int value = event.getValue();

                // BnH 06H <Value MSB>  (Data Entry MSB)
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x06,
                        (value >> 7) & 0x7F));

                // BnH 26H <Value LSB>  (Data Entry LSB)
                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_CTRL_CHANGE | midiChannel,
                        0x26,
                        value & 0x7F));

                break;
            }

        case MappedEvent::Marker:
            {
                const std::string metaMessage =
                        DataBlockRepository::getInstance()->
                                getDataBlockForEvent(&event);

                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_FILE_META_EVENT,
                        MIDI_TEXT_MARKER,
                        metaMessage));

                break;
            }

        case MappedEvent::Text:
            {
                const MidiByte midiTextType = event.getData1();

                const std::string metaMessage =
                        DataBlockRepository::getInstance()->
                                getDataBlockForEvent(&event);

                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_FILE_META_EVENT,
                        midiTextType,
                        metaMessage));

                break;
            }

        case MappedEvent::KeySignature:
            {
                std::string metaMessage;
                metaMessage += MidiByte(event.getData1());
                metaMessage += MidiByte(event.getData2());

                trackData.insertMidiEvent(new MidiEvent(
                        midiEventAbsoluteTime,
                        MIDI_FILE_META_EVENT,
                        MIDI_KEY_SIGNATURE,
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
MidiInserter::assignToMidiFile(MidiFile &midifile)
{
    finish();

    midifile.clearMidiComposition();

    // We leave out fields that write doesn't look at.
    midifile.m_numberOfTracks = m_trackPosMap.size() + 1;
    midifile.m_timingDivision = m_timingDivision;
    midifile.m_format = MidiFile::MIDI_SIMULTANEOUS_TRACK_FILE;

    midifile.m_midiComposition[0] = m_conductorTrack.m_midiTrack;
    unsigned int index = 0;
    for (TrackMap::iterator i = m_trackPosMap.begin();
         i != m_trackPosMap.end();
         ++i, ++index) {
        midifile.m_midiComposition[index + 1] = i->second.m_midiTrack;
    }
}


}
