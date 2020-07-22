/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SOUNDDRIVER_H
#define RG_SOUNDDRIVER_H

#include <string>
#include <vector>
#include <list>
#include <QStringList>

#include "base/Device.h"
#include "MappedEventList.h"
#include "MappedInstrument.h"
#include "MappedDevice.h"
#include "SequencerDataBlock.h"
#include "PlayableAudioFile.h"
#include "Scavenger.h"

// For SubFormat enum
#include "RIFFAudioFile.h"


namespace Rosegarden
{

// Current recording status - whether we're monitoring anything
// or recording.
//
typedef enum
{
    RECORD_OFF,
    RECORD_ON
} RecordStatus;


// Status of a SoundDriver - whether we're got an audio and
// MIDI subsystem or not.  This is reported right up to the
// gui.
typedef unsigned int SoundDriverStatus;
enum
{
    NO_DRIVER  = 0x00,          // Nothing's OK
    AUDIO_OK   = 0x01,          // AUDIO's OK
    MIDI_OK    = 0x02,          // MIDI's OK
    VERSION_OK = 0x04           // GUI and sequencer versions match
};


// Used for MMC and MTC, not for JACK transport
//
typedef enum
{
    TRANSPORT_OFF,
    TRANSPORT_SOURCE,
    TRANSPORT_FOLLOWER
} TransportSyncStatus;


/// Pending Note Off event for the NoteOffQueue
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


/// The queue itself
/**
 * The NoteOffQueue holds a time ordered set of
 * pending MIDI NoteOffEvent objects.
 */
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
class AudioPlayQueue;

typedef std::vector<PlayableAudioFile *> PlayableAudioFileList;


/// The abstract SoundDriver
/**
 * Abstract Base Class (ABC) to support SoundDrivers, such as ALSA.
 *
 * This ABC provides the generic driver support for
 * these drivers with the Sequencer class owning an instance
 * of a sub class of this class and directing it as required
 * by RosegardenSequencer itself.
 */
class SoundDriver
{
public:
    SoundDriver(MappedStudio *studio, const std::string &name);
    virtual ~SoundDriver();

    virtual bool initialise() = 0;
    virtual void shutdown() { }

    virtual void initialisePlayback(const RealTime &position) = 0;
    virtual void stopPlayback() = 0;
    virtual void punchOut() = 0; // stop recording, continue playing
    virtual void resetPlayback(const RealTime &oldPosition, const RealTime &position) = 0;
    virtual void allNotesOff() = 0;
    
    virtual RealTime getSequencerTime() = 0;

    virtual bool getMappedEventList(MappedEventList &) = 0;

    virtual void startClocks() { }
    virtual void stopClocks() { }

    // Process some asynchronous events
    //
    virtual void processEventsOut(const MappedEventList &mC) = 0;

    // Process some scheduled events on the output queue.  The
    // slice times are here so that the driver can interleave
    // note-off events as appropriate.
    //
    virtual void processEventsOut(const MappedEventList &mC,
                                  const RealTime &sliceStart,
                                  const RealTime &sliceEnd) = 0;

    // Activate a recording state.  armedInstruments and audioFileNames
    // can be nullptr if no audio tracks recording.
    //
    virtual bool record(RecordStatus recordStatus,
                        const std::vector<InstrumentId> *armedInstruments = nullptr,
                        const std::vector<QString> *audioFileNames = nullptr) = 0;

    // Process anything that's pending
    //
    virtual void processPending() = 0;

    // Get the driver's operating sample rate
    //
    virtual unsigned int getSampleRate() const = 0;

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   QString identifier,
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

    virtual float getPluginInstancePortValue(InstrumentId id,
                                             int position,
                                             unsigned long portNumber) = 0;

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value) = 0;

    virtual QStringList getPluginInstancePrograms(InstrumentId id,
                                                  int position) = 0;

    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position) = 0;

    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position,
                                             int bank,
                                             int program) = 0;

    virtual unsigned long getPluginInstanceProgram(InstrumentId id,
                                                   int position,
                                                   QString name) = 0;
    
    virtual void setPluginInstanceProgram(InstrumentId id,
                                          int position,
                                          QString program) = 0;

    virtual QString configurePlugin(InstrumentId id,
                                    int position,
                                    QString key,
                                    QString value) = 0;

    virtual void setAudioBussLevels(int bussId,
                                    float dB,
                                    float pan) = 0;

    virtual void setAudioInstrumentLevels(InstrumentId id,
                                          float dB,
                                          float pan) = 0;

    // Poll for new clients (for new Devices/Instruments)
    //
    virtual bool checkForNewClients() = 0;

    // Set a loop position at the driver (used for transport)
    //
    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd)
        = 0;

    virtual void sleep(const RealTime &rt);

    virtual QString getStatusLog() = 0;

    // Mapped Instruments
    //
    void setMappedInstrument(MappedInstrument *mI);
    MappedInstrument* getMappedInstrument(InstrumentId id);

    // Return the current status of the driver
    //
    unsigned int getStatus() const { return m_driverStatus; }

    // Are we playing?
    //
    bool isPlaying() const { return m_playing; }

    // Are we counting?  By default a subclass probably wants to
    // return true, if it doesn't know better.
    //
    virtual bool areClocksRunning() const = 0;

    RealTime getStartPosition() const { return m_playStartPosition; }
    RecordStatus getRecordStatus() const { return m_recordStatus; }
/*!DEVPUSH
    // Return a MappedDevice full of the Instrument mappings
    // that the driver has discovered.  The gui can then use
    // this list (complete with names) to generate its proper
    // Instruments under the MidiDevice and AudioDevice.
    //
    MappedDevice getMappedDevice(DeviceId id);

    // Return the number of devices we've found
    //
    unsigned int getDevices();
*/
    virtual bool canReconnect(Device::DeviceType) { return false; }

    virtual bool addDevice(Device::DeviceType,
                           DeviceId,
                           InstrumentId,
                           MidiDevice::DeviceDirection) {
        return false;
    }
    virtual void removeDevice(DeviceId) { }
    virtual void removeAllDevices() { }
    virtual void renameDevice(DeviceId, QString) { }

    virtual unsigned int getConnections(Device::DeviceType,
                                        MidiDevice::DeviceDirection) { return 0; }
    virtual QString getConnection(Device::DeviceType,
                                  MidiDevice::DeviceDirection,
                                  unsigned int) { return ""; }
    virtual QString getConnection(DeviceId) { return ""; }
    virtual void setConnection(DeviceId, QString) { }
    virtual void setPlausibleConnection(DeviceId id, QString c, bool) { setConnection(id, c); }
    virtual void connectSomething() { }

    virtual unsigned int getTimers() { return 0; }
    virtual QString getTimer(unsigned int) { return ""; }
    virtual QString getCurrentTimer() { return ""; }
    virtual void setCurrentTimer(QString) { }

    virtual void getAudioInstrumentNumbers(InstrumentId &, int &) = 0;
    virtual void getSoftSynthInstrumentNumbers(InstrumentId &, int &) = 0;

    // Plugin management -- SoundDrivers should maintain a plugin
    // scavenger which the audio process code can use for defunct
    // plugins.  Ownership of plugin is passed to the SoundDriver.
    //
    virtual void claimUnwantedPlugin(void *plugin) = 0;

    // This causes all scavenged plugins to be destroyed.  It
    // should only be called in non-RT contexts.
    //
    virtual void scavengePlugins() = 0;

    // Handle audio file references
    //
    void clearAudioFiles();
    bool addAudioFile(const QString &fileName, unsigned int id);
    bool removeAudioFile(unsigned int id);
                    
    void initialiseAudioQueue(const std::vector<MappedEvent> &audioEvents);
    void clearAudioQueue();
    const AudioPlayQueue *getAudioQueue() const;

    RIFFAudioFile::SubFormat getAudioRecFileFormat() const { return m_audioRecFileFormat; }


    // Latencies
    //
    virtual RealTime getAudioPlayLatency() { return RealTime::zeroTime; }
    virtual RealTime getAudioRecordLatency() { return RealTime::zeroTime; }
    virtual RealTime getInstrumentPlayLatency(InstrumentId) { return RealTime::zeroTime; }
    virtual RealTime getMaximumPlayLatency() { return RealTime::zeroTime; }

    // Buffer sizes
    //
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

    // ??? Always true.
    //void setLowLatencyMode(bool ll) { m_lowLatencyMode = ll; }
    bool getLowLatencyMode() const { return m_lowLatencyMode; }

    // Cancel the playback of an audio file - either by instrument and audio file id
    // or by audio segment id.
    //
    void cancelAudioFile(MappedEvent *mE);

    // Studio linkage
    //
    MappedStudio* getMappedStudio() { return m_studio; }
    void setMappedStudio(MappedStudio *studio) { m_studio = studio; }

    // Modify MIDI record device
    //
    void setMidiRecordDevice(DeviceId id) { m_midiRecordDevice = id; }
    DeviceId getMIDIRecordDevice() const { return m_midiRecordDevice; }

    // MIDI Realtime Sync setting
    //
    TransportSyncStatus getMIDISyncStatus() const { return m_midiSyncStatus; }
    void setMIDISyncStatus(TransportSyncStatus status) { m_midiSyncStatus = status; }

    // MMC source/follower setting
    //
    TransportSyncStatus getMMCStatus() const { return m_mmcStatus; }
    void setMMCStatus(TransportSyncStatus status) { m_mmcStatus = status; }

    // MTC source/follower setting
    //
    TransportSyncStatus getMTCStatus() const { return m_mtcStatus; }
    void setMTCStatus(TransportSyncStatus status) { m_mtcStatus = status; }

    // MMC Id
    //
    int getMMCId() const { return ((int)(m_mmcId)); }
    void setMMCId(int id) { m_mmcId = (MidiByte)(id); }

    // Set MIDI clock interval - allow redefinition above to ensure
    // we handle this reset correctly.
    //
    virtual void setMIDIClockInterval(RealTime interval) 
        { m_midiClockInterval = interval; }

    ExternalTransport *getExternalTransportControl() const {
        return m_externalTransport;
    }
    void setExternalTransportControl(ExternalTransport *transport) {
        m_externalTransport = transport;
    }

    // Do any bits and bobs of work that need to be done continuously
    // (this is called repeatedly whether playing or not).
    //
    virtual void runTasks() { }

    // Report a failure back to the GUI - ideally.  Default does nothing.
    //
    virtual void reportFailure(MappedEvent::FailureCode) { }

protected:
    // Helper functions to be implemented by subclasses
    //
    virtual void processMidiOut(const MappedEventList &mC,
                                const RealTime &sliceStart,
                                const RealTime &sliceEnd) = 0;
    virtual void generateFixedInstruments() = 0;

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
    NoteOffQueue                                m_noteOffQueue;

    // This is our driver's own list of MappedInstruments and MappedDevices.  
    // These are uncoupled at this level - the Instruments and Devices float
    // free and only index each other - the Devices hold information only like
    // name, id and if the device is duplex capable.
    //
    typedef std::vector<MappedInstrument*> MappedInstrumentList;
    MappedInstrumentList m_instruments;

    typedef std::vector<MappedDevice *> MappedDeviceList;
    MappedDeviceList m_devices;
    MappedDevice *findDevice(DeviceId deviceId);

    DeviceId                                    m_midiRecordDevice;

    RecordStatus                                m_recordStatus;


    InstrumentId                                m_midiRunningId;
    InstrumentId                                m_audioRunningId;

    // Subclass _MUST_ scavenge this regularly:
    Scavenger<AudioPlayQueue>                   m_audioQueueScavenger;
    AudioPlayQueue                             *m_audioQueue;

    // A list of AudioFiles that we can play.
    //
    std::vector<AudioFile*>                     m_audioFiles;

    RealTime m_audioMixBufferLength;
    RealTime m_audioReadBufferLength;
    RealTime m_audioWriteBufferLength;
    int m_smallFileSize;
    bool m_lowLatencyMode;

    RIFFAudioFile::SubFormat m_audioRecFileFormat;
    
    // Virtual studio hook
    //
    MappedStudio                *m_studio;
    
    // Controller to make externally originated transport requests on
    //
    ExternalTransport           *m_externalTransport;

    // MMC and MTC status and ID
    //
    TransportSyncStatus          m_midiSyncStatus;
    TransportSyncStatus          m_mmcStatus;
    TransportSyncStatus          m_mtcStatus;
    MidiByte                     m_mmcId;      // device id

    /// Whether we are sending MIDI Clocks (transport master).
    /**
     * ??? This is basically (m_midiSyncStatus == TRANSPORT_MASTER).  It is
     *     likely redundant and m_midiSyncStatus can be used instead.
     */
    bool                         m_midiClockEnabled;
    /// 24 MIDI clocks per quarter note.  MIDI Spec section 2, page 30.
    /**
     * If the Composition has tempo changes, this single interval is
     * insufficient.  We should instead compute SPP based on bar/beat/pulse
     * from the Composition.  Since the GUI and sequencer are separated,
     * the bar/beat/pulse values would need to be pushed in at play and
     * record time.  See RosegardenSequencer::m_songPosition.
     */
    RealTime                     m_midiClockInterval;
};

}

#endif // RG_SOUNDDRIVER_H
