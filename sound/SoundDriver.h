// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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
#include "RingBuffer.h"

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
                      const RealTime &duration,
                      RingBuffer *ringBuffer = 0);

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

    // 
    std::string getSampleFrames(unsigned int frames);
    std::string getSampleFrameSlice(const RealTime &time);

    // Two important numbers also reaching through to AudioFile
    //
    unsigned int getChannels();
    unsigned int getBitsPerSample();
    unsigned int getSampleRate();
    unsigned int getBytesPerSample();

    // Using these methods we can load up the audio file with
    // an external ringbuffer to use in favour of an internally
    // generated one.  The caller is in charge of deleting the
    // old buffer if we change.
    //
    void setRingBuffer(RingBuffer *rB) { m_ringBuffer = rB; }
    RingBuffer* getRingBuffer() { return m_ringBuffer; }

    // Push a number of frames into the ringbuffer
    //
    void fillRingBuffer(int bytes);

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

    // Our i/o buffer
    //
    RingBuffer           *m_ringBuffer;

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

    virtual void initialise() = 0;

    virtual void initialisePlayback(const RealTime &position,
                                    const RealTime &playLatency) = 0;
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
    virtual bool record(RecordStatus recordStatus) = 0;

    // Process anything that's pending
    //
    virtual void processPending(const RealTime &playLatency) = 0;

    // Get the driver's operating sample rate
    //
    virtual unsigned int getSampleRate() const = 0;

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   unsigned long pluginId,
                                   int position) = 0;

    virtual void removePluginInstance(InstrumentId id,
                                      int position) = 0;

    // Clear down and remove all plugin instances
    //
    virtual void removePluginInstances() = 0;

    virtual void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value) = 0;

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value) = 0;

    // Poll for new clients (for new Devices/Instruments)
    //
    virtual bool checkForNewClients() = 0;

    // Set a loop position at the driver (used for transport)
    //
    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd)
        = 0;

    // Send the MIDI clock
    //
    virtual void sendMidiClock(const RealTime &playLatency) = 0;

    virtual QString getStatusLog() { return ""; }

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

    virtual bool canReconnect(Device::DeviceType) { return false; }

    virtual DeviceId addDevice(Device::DeviceType,
			       MidiDevice::DeviceDirection) {
	return Device::NO_DEVICE;
    }
    virtual void removeDevice(DeviceId) { }

    virtual unsigned int getConnections(Device::DeviceType,
					MidiDevice::DeviceDirection) { return 0; }
    virtual QString getConnection(Device::DeviceType,
				  MidiDevice::DeviceDirection,
				  unsigned int) { return ""; }
    virtual void setConnection(DeviceId, QString) { }

    // Return the whole audio play queue
    //
    std::vector<PlayableAudioFile*>& getAudioPlayQueue() { return m_audioPlayQueue; }

    // Just non-defunct queue members
    //
    std::vector<PlayableAudioFile*> getAudioPlayQueueNotDefunct();

    // Clear the queue
    //
    void clearAudioPlayQueue();

    // Does what it says it does
    //
    void clearDefunctFromAudioPlayQueue();

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

    // Arguments
    //
    void setArgs(const std::vector<std::string> &args) { m_args = args; }

    // Modify MIDI record device
    //
    void setMidiRecordDevice(Rosegarden::DeviceId id)
        { m_midiRecordDevice = id; }
    Rosegarden::DeviceId getMIDIRecordDevice() const 
        { return m_midiRecordDevice; }

    // Set MMC state
    //
    bool isMMCEnabled() const { return m_mmcEnabled; }
    void enableMMC(bool mmc) { m_mmcEnabled = mmc; }

    // Set MMC master/slave
    //
    bool isMMCMaster() const { return (m_mmcMaster && m_mmcEnabled); }
    void setMasterMMC(bool mmc) { m_mmcMaster = mmc; }

    // MMC Id
    //
    int getMMCId() const { return ((int)(m_mmcId)); }
    void setMMCId(int id) { m_mmcId = (Rosegarden::MidiByte)(id); }

    // Set MIDI clock interval
    //
    void setMIDIClockInterval(long usecs) { m_midiClockInterval = usecs; }

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

    // This is our driver's own list of MappedInstruments and MappedDevices.  
    // These are uncoupled at this level - the Instruments and Devices float
    // free and only index each other - the Devices hold information only like
    // name, id and if the device is duplex capable.
    //
    typedef std::vector<MappedInstrument*> MappedInstrumentList;
    MappedInstrumentList m_instruments;

    typedef std::vector<MappedDevice*> MappedDeviceList;
    MappedDeviceList m_devices;

    Rosegarden::DeviceId                        m_midiRecordDevice;

    //
    MappedComposition                           m_recordComposition;
    RecordStatus                                m_recordStatus;


    InstrumentId                                m_midiRunningId;
    InstrumentId                                m_audioRunningId;

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

    std::vector<std::string>     m_args;

    // MMC status and ID
    //
    bool                         m_mmcEnabled;
    bool                         m_mmcMaster;
    Rosegarden::MidiByte         m_mmcId;      // device id

    // MIDI clock interval - microseconds
    //
    bool                         m_midiClockEnabled;
    long                         m_midiClockInterval;
    Rosegarden::RealTime         m_midiClockSendTime;

    // MIDI Song Position pointer
    //
    long                         m_midiSongPositionPointer;

};

}

#endif // _SOUNDDRIVER_H_

