// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.2
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include <string>
#include <vector>

#include "Instrument.h"
#include "Device.h"
#include "MappedComposition.h"
#include "MappedInstrument.h"
#include "RealTime.h"
#include "AudioFile.h"
#include "MappedDevice.h"

// Abstract base to support SoundDrivers such as aRts and ALSA.
//
// This base class provides the generic driver support for
// these drivers with the Sequencer class owning an instance
// of a sub class of this class and directing it and required
// by the rosegardensequencer itself.
//
//

#ifndef _SOUNDDRIVER_H_
#define _SOUNDDRIVER_H_

namespace Rosegarden
{

// Current recording status - whether we're monitoring anything
// or recording.
//
typedef enum
{
    ASYNCHRONOUS_MIDI,
    ASYNCHRONOUS_AUDIO,
    RECORD_MIDI,
    RECORD_AUDIO
} RecordStatus;


// Status of a SoundDriver - whether we're got an audio and
// MIDI subsystem or not.  This is reported right up to the
// gui.
//
typedef enum
{
    NO_DRIVER = 0x00,          // Nothing's OK
    AUDIO_OK  = 0x01,          // AUDIO's OK
    MIDI_OK   = 0x02           // MIDI's OK
} SoundDriverStatus;


// PlayableAudioFile is queued on the m_audioPlayQueue and
// played by processAudioQueue() in Sequencer.  State changes
// through playback and it's finally discarded when done.
//
// To enable us to have multiple file handles on real AudioFile
// we store the file handle with this class.
//
//
class PlayableAudioFile
{
public:

    typedef enum
    {
        IDLE,
        PLAYING,
        DEFUNCT
    } PlayStatus;


    PlayableAudioFile(InstrumentId instrumentId,
                      AudioFile *audioFile,
                      const RealTime &startTime,
                      const RealTime &startIndex,
                      const RealTime &duration);

    ~PlayableAudioFile();

    void setStatus(const PlayStatus &status) { m_status = status; }
    PlayStatus getStatus() const { return m_status; }

    void setStartTime(const RealTime &time) { m_startTime = time; }
    RealTime getStartTime() const { return m_startTime; }

    void setDuration(const RealTime &time) { m_duration = time; }
    RealTime getEndTime() const { return m_startTime + m_duration; }

    void setStartIndex(const RealTime &time) { m_startIndex = time; }
    RealTime getStartIndex() const { return m_startIndex; }

    // Get audio file for interrogation
    //
    AudioFile* getAudioFile() { return m_audioFile; }

    // Get instrument ID - we need to be able to map back
    // at the GUI.
    //
    InstrumentId getInstrument() { return m_instrumentId; }

    // Reaches through to AudioFile interface using our local file handle
    //
    //
    bool scanTo(const RealTime &time);
    std::string getSampleFrames(unsigned int frames);
    std::string getSampleFrameSlice(const RealTime &time);

    // Two important numbers also reaching through to AudioFile
    //
    unsigned int getChannels();
    unsigned int getBitsPerSample();
    unsigned int getSampleRate();
    unsigned int getBytesPerSample();


private:
    RealTime              m_startTime;
    RealTime              m_startIndex;
    RealTime              m_duration;
    PlayStatus            m_status;

    // Performance file handle - must open non-blocking to
    // allow other potential PlayableAudioFiles access to
    // the same file.
    //
    std::ifstream        *m_file;

    // AudioFile handle
    //
    AudioFile            *m_audioFile;

    // Originating Instrument Id
    //
    InstrumentId          m_instrumentId;

};

// The NoteOffQueue holds a time ordered set of
// pending MIDI NOTE OFF events.
//
class NoteOffEvent
{
public:
    NoteOffEvent() {;}
    NoteOffEvent(const Rosegarden::RealTime &realTime,
                 unsigned int pitch,
                 Rosegarden::MidiByte channel,
                 Rosegarden::InstrumentId instrument):
        m_realTime(realTime),
        m_pitch(pitch),
        m_channel(channel),
        m_instrument(instrument) {;}
    ~NoteOffEvent() {;}

    struct NoteOffEventCmp
    {
        bool operator()(NoteOffEvent *nO1, NoteOffEvent *nO2)
        {
            return nO1->getRealTime() < nO2->getRealTime();
        }
    };

    void setRealTime(const RealTime &time) { m_realTime = time; }
    Rosegarden::RealTime getRealTime() const { return m_realTime; }

    Rosegarden::MidiByte getPitch() const { return m_pitch; }
    Rosegarden::MidiByte getChannel() const { return m_channel; }
    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }

private:
    Rosegarden::RealTime     m_realTime;
    Rosegarden::MidiByte     m_pitch;
    Rosegarden::MidiByte     m_channel;
    Rosegarden::InstrumentId m_instrument;

};


// The queue itself
//
class NoteOffQueue : public std::multiset<NoteOffEvent *,
                     NoteOffEvent::NoteOffEventCmp>
{
public:
    NoteOffQueue() {;}
    ~NoteOffQueue() {;}
private:
};


class MappedStudio;

// The abstract SoundDriver
//
//
class SoundDriver
{
public:
    SoundDriver(MappedStudio *studio, const std::string &name);
    virtual ~SoundDriver();

    virtual void initialiseMidi() = 0;
    virtual void initialiseAudio() = 0;
    virtual void initialisePlayback(const RealTime &position) = 0;
    virtual void stopPlayback() = 0;
    virtual void resetPlayback(const RealTime &position,
                               const RealTime &latency) = 0;
    virtual void allNotesOff() = 0;
    virtual void processNotesOff(const RealTime &time) = 0;
    
    virtual RealTime getSequencerTime() = 0;

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency) = 0;

    virtual void processEventsOut(const MappedComposition &mC,
                                  const Rosegarden::RealTime &playLatency,
                                  bool now) = 0;

    // Activate a recording state
    //
    virtual void record(const RecordStatus& recordStatus) = 0;

    // Process anything that's pending
    //
    virtual void processPending(const RealTime &playLatency) = 0;

    // Mapped Instruments
    //
    void setMappedInstrument(MappedInstrument *mI);
    MappedInstrument* getMappedInstrument(InstrumentId id);

    // Return the current status of the driver
    //
    unsigned int getStatus() const { return m_driverStatus; }

    // Queue an audio file for playback
    //
    void queueAudio(PlayableAudioFile *audioFile);

    // Are we playing?
    //
    bool isPlaying() const { return m_playing; }

    RealTime getStartPosition() const { return m_playStartPosition; }
    RecordStatus getRecordStatus() const { return m_recordStatus; }

    // Return a MappedDevice full of the Instrument mappings
    // that the driver has discovered.  The gui can then use
    // this list (complete with names) to generate its proper
    // Instruments under the MidiDevice and AudioDevice.
    //
    Rosegarden::MappedDevice getMappedDevice(DeviceId id);

    // Return the number of devices we've found
    //
    unsigned int getDevices();

    // Return the audio play queue
    //
    std::vector<PlayableAudioFile*>& getAudioPlayQueue()
        { return m_audioPlayQueue; }

    // Clear the queue
    //
    void clearAudioPlayQueue();

    // Handle audio file references
    //
    void clearAudioFiles();
    bool addAudioFile(const std::string &fileName, unsigned int id);
    bool removeAudioFile(unsigned int id);
                    
    // Queue up an audio sample for playing - we use the
    // m_audioPlayThreadQueue to make sure all queue add
    // operations are thread-safe.
    //
    bool queueAudio(InstrumentId instrumentId,
                    AudioFileId audioFileId,
                    const RealTime &absoluteTime,
                    const RealTime &audioStartMarker,
                    const RealTime &duration,
                    const RealTime &playLatency);

    // Move the pending thread queue across to the real queue
    // at a safe point in time (when another thread isn't
    // accessing the vector.
    //
    void pushPlayableAudioQueue();

    // Recording filename
    //
    void setRecordingFilename(const std::string &file)
        {  m_recordingFilename = file; }


    // Audio monitoring InstrumentId
    //
    Rosegarden::InstrumentId getAudioMonitoringInstrument()
        { return m_audioMonitoringInstrument; }

    void setAudioMonitoringInstrument(Rosegarden::InstrumentId id)
        { m_audioMonitoringInstrument = id; }

    // Latencies
    //
    void setAudioPlayLatency(const RealTime &l) { m_audioPlayLatency = l; }
    RealTime getAudioPlayLatency() { return  m_audioPlayLatency; }

    void setAudioRecordLatency(const RealTime &l) { m_audioRecordLatency = l; }
    RealTime getAudioRecordLatency() { return m_audioRecordLatency; }

    // Cancel the playback of an audio file
    //
    void cancelAudioFile(InstrumentId instrumentId, AudioFileId audioFileId);

    // Studio linkage
    //
    MappedStudio* getMappedStudio() { return m_studio; }
    void setMappedStudio(MappedStudio *studio) { m_studio = studio; }

protected:
    // Helper functions to be implemented by subclasses
    //
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now) = 0;
    virtual void processAudioQueue(const RealTime &playLatency,
                                   bool now) = 0;
    virtual void generateInstruments() = 0;

    // Audio
    //
    AudioFile* getAudioFile(unsigned int id);

    std::string                                 m_name;
    unsigned int                                m_driverStatus;
    Rosegarden::RealTime                        m_playStartPosition;
    bool                                        m_startPlayback;
    bool                                        m_playing;

    // MIDI Note-off handling
    //
    std::map<unsigned int, MappedEvent*>        m_noteOnMap;
    NoteOffQueue                                m_noteOffQueue;

    // This is our driver's own list of MappedInstruments.
    //
    std::vector<MappedInstrument*>              m_instruments;

    // List of device names by DeviceId
    //
    std::vector<std::string>                    m_deviceName;

    MappedComposition                           m_recordComposition;
    RecordStatus                                m_recordStatus;


    InstrumentId                                m_midiRunningId;
    InstrumentId                                m_audioRunningId;
    DeviceId                                    m_deviceRunningId;

    // Audio files - both real and the playing abstraction
    //
    std::vector<PlayableAudioFile*>             m_audioPlayQueue;

    // The Thread queue is used to make sure we don't invalidate
    // iterators by writing to the audioPlayQueue when another
    // (JACK) thread is reading from it.  We use the thread queue
    // as temporary storage and move it along at the end of the
    // JACK process callback.
    //
    std::vector<PlayableAudioFile*>             m_audioPlayThreadQueue;
    std::vector<AudioFile*>                     m_audioFiles;

    // Filename we should record to
    std::string                                 m_recordingFilename;

    Rosegarden::InstrumentId                    m_audioMonitoringInstrument;

    // Audio latencies
    //
    RealTime                     m_audioPlayLatency;
    RealTime                     m_audioRecordLatency;

    // Virtual studio hook
    //
    MappedStudio                *m_studio;

};

}

#endif // _SOUNDDRIVER_H_

