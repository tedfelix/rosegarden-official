/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDEVENT_H
#define RG_MAPPEDEVENT_H

#include <QDataStream>

#include "base/RealTime.h"
#include "base/Track.h"
#include "base/Event.h"


namespace Rosegarden
{
class MappedEvent;

/// Used for storing data blocks for SysEx messages.
/**
 *  @see MappedEvent::m_dataBlockId
 */
class DataBlockRepository
{
public:
    friend class MappedEvent;
    typedef unsigned long blockid;

    static DataBlockRepository* getInstance();
    static std::string getDataBlockForEvent(const MappedEvent*);
    static void setDataBlockForEvent(MappedEvent*, const std::string&,
                                     bool extend = false);
    /**
     * Clear all block files
     */
    static void clear();
    // unused static bool hasDataBlock(blockid);

protected:
    DataBlockRepository();

    static std::string getDataBlock(blockid);

    static blockid registerDataBlock(const std::string&);
    // unused static void unregisterDataBlock(blockid);

    static void registerDataBlockForEvent(const std::string&, MappedEvent*);
    // unused static void unregisterDataBlockForEvent(MappedEvent*);


    //--------------- Data members ---------------------------------

    static DataBlockRepository* m_instance;
};

/// A MIDI event that is ready for playback
/**
 *  Here, the term "Mapped" refers to the conversion of an Event in a Segment
 *  to something (a MappedEvent) that is closer to what is needed to send
 *  to ALSA for playback.
 *
 *  Used as a transformation stage between Composition, Events and output
 *  at the Sequencer this class and MidiComposition eliminate the notion
 *  of the Segment and Track for ease of Event access.  The MappedEvents
 *  are ready for playing or routing through an Instrument or Effects
 *  boxes.
 *
 *  MappedEvents can also represent instructions for playback of audio
 *  samples - if the m_type is Audio then the sequencer will attempt to
 *  map the Pitch (m_data1) to the audio id.  Note that this limits us
 *  to 256 audio files in the Composition unless we use a different
 *  parameter for storing these IDs.
 *
 *  The MappedEvent/Instrument relationship is interesting - we don't
 *  want to duplicate the entire Instrument at the Sequencer level as
 *  it'd be messy and unnecessary.  Instead we use a MappedInstrument
 *  which is just a very cut down Sequencer-side version of an Instrument.
 *
 *  Some of these Events are unidirectional, some are bidirectional -
 *  that is they only have a meaning in one direction (they are still
 *  legal at either end).  They are broadcast in both directions using
 *  the "getSequencerSlice" and "processAsync/Recorded" interfaces on
 *  which the control messages can piggyback and eventually stripped out.
 */
class MappedEvent
{
public:
    typedef enum : unsigned
    {
        InvalidMappedEvent = 0,

        // MIDI events.
        // Use individual bits for bitmasking/filtering.
        // See MidiFilterDialog's ctor.
        MidiNote            = 1 << 0,
        MidiProgramChange   = 1 << 1,
        MidiKeyPressure     = 1 << 2,
        MidiChannelPressure = 1 << 3,
        MidiPitchBend       = 1 << 4,
        MidiController      = 1 << 5,
        MidiSystemMessage   = 1 << 6,

        // The remaining values do not need bitmask/filtering support.
        // We start at bit 26 which gives us 25 bits we
        // can use if we need more event types for filtering.  This
        // gives us 127 possible values for the event types that do
        // not need filtering.

        MidiNoteOneShot          = 1 << 25,  // doesn't need NOTE OFFs

        // Sent from the gui to play an audio file.
        // ??? This is used for a comparison to detect MIDI event types.
        //     That needs to be done in a safer way.  See
        //     SequenceManager::processAsynchronousMidi() and
        //     AlsaDriver::processMidiOut().
        Audio                    = 2 << 25,
        // Sent from gui to cancel playing an audio file
        AudioCancel              = 3 << 25,
        // Sent to the gui with audio level on Instrument
        AudioLevel               = 4 << 25,
        // Sent to the gui to inform an audio file stopped
        AudioStopped             = 5 << 25,
        // The gui is clear to generate a preview for a new audio file
        AudioGeneratePreview     = 6 << 25,

        // Update Instruments - new ALSA client detected
        SystemUpdateInstruments  = 7 << 25,
        // Set RG as JACK source/follower
        SystemJackTransport      = 8 << 25,
        // Set RG as MMC source/follower
        SystemMMCTransport       = 9 << 25,
        // Set System Messages and MIDI Clock
        SystemMIDIClock          = 10 << 25,
        // Set Metronome device
        SystemMetronomeDevice    = 11 << 25,
        // Set Audio inputs/outputs: data1 num inputs, data2 num submasters
        SystemAudioPortCounts    = 12 << 25,
        // Set whether we create various Audio ports (data1 is an AudioOutMask)
        SystemAudioPorts         = 13 << 25,
        // Some failure has occurred: data1 contains FailureCode
        SystemFailure            = 14 << 25,

        // Time sig. event (from time sig. composition reference segment)
        TimeSignature            = 15 << 25,
        // Tempo event (from tempo composition reference segment)
        Tempo                    = 16 << 25,

        // Panic function
        Panic                    = 17 << 25,

        // Set RG as MTC source/follower
        SystemMTCTransport       = 18 << 25,
        // Auto-connect sync outputs
        SystemMIDISyncAuto       = 19 << 25,
        // File format used for audio recording (data1 is 0=PCM,1=float)
        SystemAudioFileFormat    = 20 << 25,

        // Marker (for MIDI export, not sound)
        Marker                   = 21 << 25,
        // Text (for MIDI export, not sound)
        Text                     = 22 << 25,
        // Key signature (for MIDI export, not sound)
        KeySignature             = 23 << 25

    } MappedEventType;

    typedef enum
    {
        // These values are OR'd to produce the data2 field in a
        // SystemAudioPorts event.
        FaderOuts                = 1 << 0,
        SubmasterOuts            = 1 << 1

    } MappedEventAudioOutMask;

    typedef enum
    {
        // JACK is having some xruns - warn the user maybe
        FailureXRuns             = 0,
        // JACK has died or kicked us out
        FailureJackDied          = 1,
        // Audio subsystem failed to read from disc fast enough
        FailureDiscUnderrun      = 2,
        // Audio subsystem failed to write to disc fast enough
        FailureDiscOverrun       = 3,
        // Audio subsystem failed to mix busses fast enough
        FailureBussMixUnderrun   = 4,
        // Audio subsystem failed to mix instruments fast enough
        FailureMixUnderrun       = 5,
        // Using a timer that has too low a resolution (e.g. 100Hz system timer)
        WarningImpreciseTimer    = 6,
        // Too much CPU time spent in audio processing -- risk of xruns and lockup
        FailureCPUOverload       = 7,
        // JACK kicked us out, but we've reconnected
        FailureJackRestart       = 8,
        // JACK kicked us out, and now the reconnection has failed
        FailureJackRestartFailed = 9,
        // A necessary ALSA call has returned an error code
        FailureALSACallFailed    = 10,
        // Using a timer that has too low a resolution, but RTC might work
        WarningImpreciseTimerTryRTC = 11
    } FailureCode;

    MappedEvent()  { }

    // Another Internal constructor from Events
    MappedEvent(InstrumentId id,
                const Event &e,
                const RealTime &eventTime,
                const RealTime &duration);

    // A general MappedEvent constructor for any MappedEvent type
    //
    MappedEvent(InstrumentId instrumentId,
                MappedEventType type,
                MidiByte pitch,
                MidiByte velocity,
                const RealTime &absTime,
                const RealTime &duration,
                const RealTime &audioStartMarker) :
        m_instrument(instrumentId),
        m_type(type),
        m_data1(pitch),
        m_data2(velocity),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(audioStartMarker)
    { }

    /// Construct an Audio MappedEvent.
    MappedEvent(InstrumentId instrumentId,
                unsigned short audioFileID,
                const RealTime &eventTime,
                const RealTime &duration,
                const RealTime &audioStartMarker) :
         m_instrument(instrumentId),
         m_type(Audio),
         m_data1(audioFileID % 256),  // LSB
         m_data2(audioFileID / 256),  // MSB
         m_eventTime(eventTime),
         m_duration(duration),
         m_audioStartMarker(audioStartMarker)
    { }

    /// Construct a MIDI event with data1 and data2 parameters.
    MappedEvent(InstrumentId instrumentId,
                MappedEventType type,
                MidiByte data1,
                MidiByte data2) :
         m_instrument(instrumentId),
         m_type(type),
         m_data1(data1),
         m_data2(data2)
    { }

    /// Construct a MIDI event with a data1 parameter.
    MappedEvent(InstrumentId instrumentId,
                MappedEventType type,
                MidiByte data1) :
        m_instrument(instrumentId),
        m_type(type),
        m_data1(data1)
    { }

    MappedEvent(InstrumentId instrumentId,
                MappedEventType type) :
        m_instrument(instrumentId),
        m_type(type)
    { }

    bool isValid() const  { return m_type != InvalidMappedEvent; }

    // Event time
    void setEventTime(const RealTime &eventTime)  { m_eventTime = eventTime; }
    RealTime getEventTime() const  { return m_eventTime; }

    // Duration
    void setDuration(const RealTime &duration)  { m_duration = duration; }
    RealTime getDuration() const  { return m_duration; }

    // Instrument ID
    void setInstrumentId(InstrumentId id)  { m_instrument = id; }
    InstrumentId getInstrumentId() const  { return m_instrument; }

    // Track
    void setTrackId(TrackId id) { m_trackId = id; }
    TrackId getTrackId() const { return m_trackId; }

    // Pitch
    void setPitch(MidiByte pitch)
    {
        // Keep pitch within MIDI limits
        if (pitch > MidiMaxValue)
            m_data1 = MidiMaxValue;
        else
            m_data1 = pitch;
    }
    MidiByte getPitch() const { return m_data1; }

    // Velocity
    void setVelocity(MidiByte v) { m_data2 = v; }
    MidiByte getVelocity() const { return m_data2; }

    // data1
    void setData1(MidiByte d1)  { m_data1 = d1; }
    MidiByte getData1() const  { return m_data1; }

    // data2
    void setData2(MidiByte d2)  { m_data2 = d2; }
    MidiByte getData2() const  { return m_data2; }

    int getAudioFileID() const  { return m_data1 + 256 * m_data2; }

    // A sample doesn't have to be played from the beginning.  When
    // passing an Audio event this value may be set to indicate from
    // where in the sample it should be played.  Duration is measured
    // against total sounding length (not absolute position).
    //
    void setAudioStartMarker(const RealTime &aS)
        { m_audioStartMarker = aS; }
    RealTime getAudioStartMarker() const
        { return m_audioStartMarker; }

    MappedEventType getType() const { return m_type; }
    void setType(const MappedEventType &value) { m_type = value; }

    // Data block id
    //
    DataBlockRepository::blockid getDataBlockId() const { return m_dataBlockId; }
    void setDataBlockId(DataBlockRepository::blockid dataBlockId) { m_dataBlockId = dataBlockId; }

    /// Set data block for SysEx, markers, and text.
    /*
     * If the block doesn't exist, it is created.
     *
     * For SysEx, DO NOT include the F0/F7 SOX/EOX in the block.  Those will
     * be added by AlsaDriver::processMidiOut().
     *
     * To set up a SysEx message:
     *   - Set instrumentId to NoInstrument.
     *   - Set type to MidiSystemMessage.
     *   - Set data1 to MIDI_SYSTEM_EXCLUSIVE.
     *   - Set recorded device with the destination device.
     *   - Call setDataBlock() to add the SysEx data.
     */
    void setDataBlock(const std::string &rawData);

    // Whether the event is all done sounding at time t.
    /**
     * Zero-duration events at exactly time t are not all done, but
     * non-zeroes that end at exactly time t are.
     */
    bool EndedBefore(RealTime t) const
    {
        return
            ((getEventTime() + getDuration() <= t) &&
             (getDuration() != RealTime::zero() ||
              getEventTime() != t));
    }

    // How MappedEvents are ordered in the MappedEventList
    //
    struct MappedEventCmp
    {
        bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
        {
            return *mE1 < *mE2;
        }
    };

    friend bool operator<(const MappedEvent &a, const MappedEvent &b);

    /// The runtime Segment ID of an audio file.
    void setRuntimeSegmentId(int id)  { m_runtimeSegmentId = id; }
    int getRuntimeSegmentId() const  { return m_runtimeSegmentId; }

    bool isAutoFading() const { return m_autoFade; }
    void setAutoFade(bool value) { m_autoFade = value; }

    RealTime getFadeInTime() const { return m_fadeInTime; }
    void setFadeInTime(const RealTime &time)
            { m_fadeInTime = time; }

    RealTime getFadeOutTime() const { return m_fadeOutTime; }
    void setFadeOutTime(const RealTime &time)
            { m_fadeOutTime = time; }

    // Original event input channel as it was recorded
    //
    unsigned int getRecordedChannel() const { return m_recordedChannel; }
    void setRecordedChannel(const unsigned int channel)
            { m_recordedChannel = channel; }

    // Original event record device as it was recorded
    //
    unsigned int getRecordedDevice() const { return m_recordedDevice; }
    void setRecordedDevice(const unsigned int device) { m_recordedDevice = device; }

private:

    TrackId m_trackId{NoTrack};
    InstrumentId m_instrument{0};
    MappedEventType m_type{InvalidMappedEvent};
    MidiByte m_data1{0};
    MidiByte m_data2{0};
    RealTime m_eventTime;
    RealTime m_duration;
    RealTime m_audioStartMarker;

    // Use this when we want to store something in addition to the
    // other bytes in this type, e.g. System Exclusive.
    //
    DataBlockRepository::blockid m_dataBlockId{0};

    // Id of the segment that this (audio) event is derived from
    //
    int m_runtimeSegmentId{-1};

    // Audio autofading
    //
    bool m_autoFade{false};
    RealTime m_fadeInTime;
    RealTime m_fadeOutTime;

    // For input events, original data, stored as it was recorded.
    // For output events, channel to play on.  m_recordedDevice is not
    // used for output.
    unsigned int m_recordedChannel{0};
    unsigned int m_recordedDevice{0};

    friend QDebug operator<<(QDebug, const MappedEvent &);
};

QDebug operator<<(QDebug, const MappedEvent &);


}

#endif
