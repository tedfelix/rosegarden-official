// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <list>

#include "Device.h"
#include "MappedComposition.h"
#include "MappedInstrument.h"
#include "MappedDevice.h"
#include "SequencerDataBlock.h"
#include "PlayableAudioFile.h"

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


// The NoteOffQueue holds a time ordered set of
// pending MIDI NOTE OFF events.
//
class NoteOffEvent
{
public:
    NoteOffEvent() {;}
    NoteOffEvent(const RealTime &realTime,
                 unsigned int pitch,
                 MidiByte channel,
                 InstrumentId instrument):
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
    RealTime getRealTime() const { return m_realTime; }

    MidiByte getPitch() const { return m_pitch; }
    MidiByte getChannel() const { return m_channel; }
    InstrumentId getInstrument() const { return m_instrument; }

private:
    RealTime     m_realTime;
    MidiByte     m_pitch;
    MidiByte     m_channel;
    InstrumentId m_instrument;

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
class ExternalTransport;

typedef std::vector<PlayableAudioFile *> PlayableAudioFileList;

// The abstract SoundDriver
//
//
class SoundDriver
{
public:
    SoundDriver(MappedStudio *studio, const std::string &name);
    virtual ~SoundDriver();

    virtual void initialise() = 0;
    virtual void shutdown() { }

    virtual void initialisePlayback(const RealTime &position) = 0;
    virtual void stopPlayback() = 0;
    virtual void resetPlayback(const RealTime &position) = 0;
    virtual void allNotesOff() = 0;
    virtual void processNotesOff(const RealTime &time) = 0;
    
    virtual RealTime getSequencerTime() = 0;

    virtual MappedComposition *getMappedComposition() = 0;

    virtual void startClocks() { }
    virtual void stopClocks() { }

    virtual void processEventsOut(const MappedComposition &mC,
                                  bool now) = 0;

    // Activate a recording state
    //
    virtual bool record(RecordStatus recordStatus) = 0;

    // Process anything that's pending
    //
    virtual void processPending() = 0;

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

    virtual void sleep(const RealTime &rt);

    // Send the MIDI clock
    //
    virtual void sendMidiClock() = 0;

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

    // Are we counting?  (Default implementation is always counting,
    // but if a subclass implements stopClocks/startClocks then that
    // won't always be the case.)
    //
    virtual bool areClocksRunning() const { return true; }

    RealTime getStartPosition() const { return m_playStartPosition; }
    RecordStatus getRecordStatus() const { return m_recordStatus; }

    // Return a MappedDevice full of the Instrument mappings
    // that the driver has discovered.  The gui can then use
    // this list (complete with names) to generate its proper
    // Instruments under the MidiDevice and AudioDevice.
    //
    MappedDevice getMappedDevice(DeviceId id);

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
    virtual void setPlausibleConnection(DeviceId id, QString c) { setConnection(id, c); }

    virtual unsigned int getTimers() { return 0; }
    virtual QString getTimer(unsigned int n) { return ""; }
    virtual QString getCurrentTimer() { return ""; }
    virtual void setCurrentTimer(QString) { }

    virtual void getAudioInstrumentNumbers(InstrumentId &, int &) = 0;

    // Return the whole audio play queue
    //
    PlayableAudioFileList getAudioPlayQueue();

    // Just non-defunct queue members
    //
    PlayableAudioFileList getAudioPlayQueueNotDefunct();

    // Just non-defunct queue members on a particular audio instrument
    //
    PlayableAudioFileList getAudioPlayQueuePerInstrument(InstrumentId);

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
    void rationalisePlayingAudio(const std::vector<MappedEvent> &list,
				 const RealTime &playtime);
                    
    // Recording filename
    //
    void setRecordingFilename(const std::string &file)
        {  m_recordingFilename = file; }


    // Audio monitoring InstrumentId
    //
    InstrumentId getAudioMonitoringInstrument()
        { return m_audioMonitoringInstrument; }

    void setAudioMonitoringInstrument(InstrumentId id)
        { m_audioMonitoringInstrument = id; }

    // Latencies
    //
    virtual RealTime getAudioPlayLatency() { return RealTime(0, 0); }
    virtual RealTime getAudioRecordLatency() { return RealTime(0, 0); }

    void setAudioBufferSizes(RealTime mix, RealTime read, RealTime write,
			     int smallFileSize) {
	m_audioMixBufferLength = mix;
	m_audioReadBufferLength = read;
	m_audioWriteBufferLength = write;
	m_smallFileSize = smallFileSize;
    }

    RealTime getAudioMixBufferLength() { return m_audioMixBufferLength; }
    RealTime getAudioReadBufferLength() { return m_audioReadBufferLength; }
    RealTime getAudioWriteBufferLength() { return m_audioWriteBufferLength; }
    int getSmallFileSize() { return m_smallFileSize; }

    // Cancel the playback of an audio file - either by instrument and audio file id
    // or by audio segment id.
    //
    void cancelAudioFile(MappedEvent *mE);

    // Studio linkage
    //
    MappedStudio* getMappedStudio() { return m_studio; }
    void setMappedStudio(MappedStudio *studio) { m_studio = studio; }

    // Arguments
    //
    void setArgs(const std::vector<std::string> &args) { m_args = args; }

    // Modify MIDI record device
    //
    void setMidiRecordDevice(DeviceId id) { m_midiRecordDevice = id; }
    DeviceId getMIDIRecordDevice() const { return m_midiRecordDevice; }

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
    void setMMCId(int id) { m_mmcId = (MidiByte)(id); }

    // Set MIDI clock interval - allow redefinition above to ensure
    // we handle this reset correctly.
    //
    virtual void setMIDIClockInterval(RealTime interval) 
        { m_midiClockInterval = interval; }

    // Get and set the mapper which may optionally be used to
    // store recording levels etc for communication back to the GUI.
    // (If a subclass wants this and finds it's not there, it should
    // simply continue without.)
    //
    SequencerDataBlock *getSequencerDataBlock() { return m_sequencerDataBlock; }
    void setSequencerDataBlock(SequencerDataBlock *d) { m_sequencerDataBlock = d; }

    ExternalTransport *getExternalTransportControl() const {
	return m_externalTransport;
    }
    void setExternalTransportControl(ExternalTransport *transport) {
	m_externalTransport = transport;
    }

    // Report a failure back to the GUI - ideally.  Default does nothing.
    //
    virtual void reportFailure(Rosegarden::MappedEvent::FailureCode code) { }

protected:
    // Helper functions to be implemented by subclasses
    //
    virtual void processMidiOut(const MappedComposition &mC,
                                bool now) = 0;
    virtual void processAudioQueue(bool now) = 0;
    virtual void generateInstruments() = 0;

    // Audio
    //
    AudioFile* getAudioFile(unsigned int id);

    std::string                                 m_name;
    unsigned int                                m_driverStatus;
    RealTime                                    m_playStartPosition;
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

    DeviceId                                    m_midiRecordDevice;

    MappedComposition                           m_recordComposition;
    MappedComposition                           m_returnComposition;
    RecordStatus                                m_recordStatus;


    InstrumentId                                m_midiRunningId;
    InstrumentId                                m_audioRunningId;

    // Audio files playing at the driver are held here - if we're
    // using threads then make sure you mutex access to this
    // vector.
    //
    std::list<PlayableAudioFile *>              m_audioPlayQueue;

    // A list of AudioFiles that we can play.
    //
    std::vector<AudioFile*>                     m_audioFiles;

    // Filename we should record to
    std::string                                 m_recordingFilename;

    InstrumentId                                m_audioMonitoringInstrument;

    RealTime m_audioMixBufferLength;
    RealTime m_audioReadBufferLength;
    RealTime m_audioWriteBufferLength;
    int m_smallFileSize;

    // Virtual studio hook
    //
    MappedStudio                *m_studio;

    // Sequencer data block for communication back to GUI
    //
    SequencerDataBlock          *m_sequencerDataBlock;
    
    // Controller to make externally originated transport requests on
    //
    ExternalTransport           *m_externalTransport;

    std::vector<std::string>     m_args;

    // MMC status and ID
    //
    bool                         m_mmcEnabled;
    bool                         m_mmcMaster;
    MidiByte                     m_mmcId;      // device id

    // MIDI clock interval
    //
    bool                         m_midiClockEnabled;
    RealTime                     m_midiClockInterval;
    RealTime                     m_midiClockSendTime;

    // MIDI Song Position pointer
    //
    long                         m_midiSongPositionPointer;

};

}

#endif // _SOUNDDRIVER_H_

