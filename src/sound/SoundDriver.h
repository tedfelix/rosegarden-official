/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SOUNDDRIVER_H
#define RG_SOUNDDRIVER_H

#include "base/Device.h"
#include "base/Instrument.h"  // For InstrumentId...
#include "base/MidiProgram.h"  // For MidiByte...
#include "base/RealTime.h"
#include "MappedEvent.h"
#include "MappedInstrument.h"
#include "MappedDevice.h"
#include "Scavenger.h"
#include "AudioPlayQueue.h"

#include "RIFFAudioFile.h"  // For SubFormat enum

#include <QString>
#include <QStringList>

#include <set>
#include <string>
#include <vector>


namespace Rosegarden
{


// Current recording status - whether we're monitoring anything
// or recording.
enum RecordStatus  { RECORD_OFF, RECORD_ON };

typedef unsigned SoundDriverStatus;
enum
{
    NO_DRIVER = 0x00,  // Nothing's OK
    AUDIO_OK  = 0x01,
    MIDI_OK   = 0x02
};

/// Used for MMC and MTC, not for JACK transport
enum TransportSyncStatus
{
    TRANSPORT_OFF,
    TRANSPORT_SOURCE,
    TRANSPORT_FOLLOWER
};


class RosegardenSequencer;
class MappedEventList;
class MappedStudio;


/// Abstract Base Class (ABC) to support sound drivers, such as ALSA.
/**
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

    void setAudioBufferSizes(RealTime mix, RealTime read, RealTime write,
                             int smallFileSize) {
        m_audioMixBufferLength = mix;
        m_audioReadBufferLength = read;
        m_audioWriteBufferLength = write;
        m_smallFileSize = smallFileSize;
    }

    /// Store a local copy at construction time.
    /**
     * This lets us avoid calling RosegardenSequencer::getInstance() which
     * uses a mutex.
     */
    void setSequencer(RosegardenSequencer *sequencer) {
        m_sequencer = sequencer;
    }
    /// Use instead of RosegardenSequencer::getInstance() to avoid mutex.
    /**
     * JackDriver uses this for transport requests.
     */
    RosegardenSequencer *getSequencer() const  { return m_sequencer; }

    virtual bool initialise()  { return true; }
    virtual void shutdown()  { }

    SoundDriverStatus getStatus() const  { return m_driverStatus; }

    virtual void initialisePlayback(const RealTime & /*position*/)  { }
    virtual void stopPlayback()  { }
    /// stop recording, continue playing
    virtual void punchOut()  { }
    virtual void resetPlayback(const RealTime & /*oldPosition*/,
                               const RealTime & /*position*/)  { }
    
    virtual RealTime getSequencerTime()  { return RealTime(0, 0); }

    virtual bool getMappedEventList(MappedEventList &)  { return true; }

    virtual void startClocks() { }
    virtual void stopClocks() { }

    // Process some asynchronous events
    //
    virtual void processEventsOut(const MappedEventList & /*mC*/)  { }

    // Process some scheduled events on the output queue.  The
    // slice times are here so that the driver can interleave
    // note-off events as appropriate.
    //
    virtual void processEventsOut(const MappedEventList & /*mC*/,
                                  const RealTime & /*sliceStart*/,
                                  const RealTime & /*sliceEnd*/)  { }

    virtual bool record(
            RecordStatus /*recordStatus*/,
            const std::vector<InstrumentId> & /*armedInstruments*/,
            const std::vector<QString> & /*audioFileNames*/)
        { return false; }

    virtual void processPending()  { }

    // Get the driver's operating sample rate
    virtual unsigned int getSampleRate() const  { return 0; }

    // Plugin instance management

    virtual void setPluginInstance(InstrumentId /*id*/,
                                   QString /*identifier*/,
                                   int /*position*/)  { }

    virtual void removePluginInstance(InstrumentId /*id*/,
                                      int /*position*/)  { }

    virtual void setPluginInstancePortValue(InstrumentId /*id*/,
                                            int /*position*/,
                                            unsigned long /*portNumber*/,
                                            float /*value*/)  { }

    virtual float getPluginInstancePortValue(InstrumentId /*id*/,
                                             int /*position*/,
                                             unsigned long /*portNumber*/)
        { return 0; }

    virtual void setPluginInstanceBypass(InstrumentId /*id*/,
                                         int /*position*/,
                                         bool /*value*/)  { }

    virtual QStringList getPluginInstancePrograms(InstrumentId /*id*/,
                                                  int /*position*/)
        { return QStringList(); }

    virtual QString getPluginInstanceProgram(InstrumentId /*id*/,
                                             int /*position*/)
        { return QString(); }

    virtual QString getPluginInstanceProgram(InstrumentId /*id*/,
                                             int /*position*/,
                                             int /*bank*/,
                                             int /*program*/)
        { return QString(); }

    virtual unsigned long getPluginInstanceProgram(InstrumentId /*id*/,
                                                   int /*position*/,
                                                   QString /*name*/)
        { return 0; }
    
    virtual void setPluginInstanceProgram(InstrumentId /*id*/,
                                          int /*position*/,
                                          QString /*program*/)  { }

    virtual QString configurePlugin(InstrumentId /*id*/,
                                    int /*position*/,
                                    QString /*key*/,
                                    QString /*value*/)  { return QString(); }

    virtual void setAudioBussLevels(int /*bussId*/,
                                    float /*dB*/,
                                    float /*pan*/)  { }

    virtual void setAudioInstrumentLevels(InstrumentId /*id*/,
                                          float /*dB*/,
                                          float /*pan*/)  { }

    // Poll for new clients (for new Devices/Instruments)
    //
    virtual void checkForNewClients() = 0;

    // Set a loop position at the driver (used for transport)
    //
    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd)
        = 0;

    virtual void sleep(const RealTime &rt);

    virtual QString getStatusLog() = 0;

    // Mapped Instruments
    //
    void setMappedInstrument(MappedInstrument *mI);

    bool isPlaying() const  { return m_playing; }

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

    virtual bool addDevice(Device::DeviceType,
                           DeviceId,
                           InstrumentId,
                           MidiDevice::DeviceDirection)
        { return false; }
    virtual void removeDevice(DeviceId) { }
    virtual void removeAllDevices() { }
    virtual void renameDevice(DeviceId, QString) { }

    virtual unsigned int getConnections(Device::DeviceType,
                                        MidiDevice::DeviceDirection) { return 0; }
    virtual QString getConnection(Device::DeviceType,
                                  MidiDevice::DeviceDirection,
                                  unsigned int) { return ""; }
    virtual QString getConnection(DeviceId) { return ""; }
    virtual void setConnection(
            DeviceId /* deviceId */,
            QString /* idealConnection */) { }
    virtual void setPlausibleConnection(
            DeviceId deviceId,
            QString idealConnection,
            bool /* recordDevice */)
                    { setConnection(deviceId, idealConnection); }
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
    const AudioPlayQueue *getAudioQueue() const;

    RIFFAudioFile::SubFormat getAudioRecFileFormat() const
        { return m_audioRecFileFormat; }


    // Latencies
    //
    virtual RealTime getAudioPlayLatency() { return RealTime::zeroTime; }
    virtual RealTime getAudioRecordLatency() { return RealTime::zeroTime; }
    virtual RealTime getInstrumentPlayLatency(InstrumentId) { return RealTime::zeroTime; }
    virtual RealTime getMaximumPlayLatency() { return RealTime::zeroTime; }

    // Buffer sizes
    //
    RealTime getAudioMixBufferLength() { return m_audioMixBufferLength; }
    RealTime getAudioReadBufferLength() { return m_audioReadBufferLength; }
    RealTime getAudioWriteBufferLength() { return m_audioWriteBufferLength; }

    bool getLowLatencyMode() const  { return true; }

    MappedStudio *getMappedStudio() { return m_studio; }

    // Set MIDI clock interval - allow redefinition above to ensure
    // we handle this reset correctly.
    //
    virtual void setMIDIClockInterval(RealTime interval) 
        { m_midiClockInterval = interval; }

    // Do any bits and bobs of work that need to be done continuously
    // (this is called repeatedly whether playing or not).
    //
    virtual void runTasks() { }

    // Report a failure back to the GUI - ideally.  Default does nothing.
    //
    virtual void reportFailure(MappedEvent::FailureCode) { }

protected:

    /// For transport requests.
    /*
     * Use instead of RosegardenSequencer::getInstance() to avoid mutex.
     */
    RosegardenSequencer *m_sequencer;

    /// Driver name for the audit log.
    std::string m_name;

    SoundDriverStatus m_driverStatus;

    RealTime m_playStartPosition;
    bool m_playing;

    typedef std::vector<MappedInstrument *> MappedInstrumentList;
    // This is our driver's own list of MappedInstruments and MappedDevices.
    // These are uncoupled at this level - the Instruments and Devices float
    // free and only index each other - the Devices hold information only like
    // name, id and if the device is duplex capable.
    MappedInstrumentList m_instruments;

    RecordStatus m_recordStatus;

    // Subclass _MUST_ scavenge this regularly.
    Scavenger<AudioPlayQueue> m_audioQueueScavenger;

    AudioPlayQueue *m_audioQueue;

    /// A list of AudioFile's that we can play.
    std::vector<AudioFile *> m_audioFiles;
    AudioFile *getAudioFile(unsigned int id);

    RealTime m_audioMixBufferLength;
    RealTime m_audioReadBufferLength;
    RealTime m_audioWriteBufferLength;

    int m_smallFileSize;

    RIFFAudioFile::SubFormat m_audioRecFileFormat;
    
    // Virtual studio hook
    MappedStudio *m_studio;

    /// 24 MIDI clocks per quarter note.  MIDI Spec section 2, page 30.
    /**
     * If the Composition has tempo changes, this single interval is
     * insufficient.  We should instead compute SPP based on bar/beat/pulse
     * from the Composition.  Since the GUI and sequencer are separated,
     * the bar/beat/pulse values would need to be pushed in at play and
     * record time.  See RosegardenSequencer::m_songPosition.
     */
    RealTime m_midiClockInterval;
};


}

#endif // RG_SOUNDDRIVER_H
