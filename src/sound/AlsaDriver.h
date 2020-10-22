/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ALSADRIVER_H
#define RG_ALSADRIVER_H

#ifdef HAVE_ALSA

#include "SoundDriver.h"
#include "NoteOffEvent.h"
#include "base/Instrument.h"
#include "base/Device.h"
#include "AlsaPort.h"
#include "MappedEventList.h"
#include "Scavenger.h"
#include "RunnablePluginInstance.h"

#ifdef HAVE_LIBJACK
#include "JackDriver.h"
#endif

#include <alsa/asoundlib.h> // ALSA

#include <QMutex>
#include <QSharedPointer>

#include <vector>
#include <set>
#include <map>
#include <string>

namespace Rosegarden
{


/// Specialisation of SoundDriver to support ALSA (http://www.alsa-project.org)
class AlsaDriver : public SoundDriver
{
public:
    AlsaDriver(MappedStudio *studio);
    ~AlsaDriver() override;

    // shutdown everything that's currently open
    void shutdown() override;

    bool initialise() override;
    void initialisePlayback(const RealTime &position) override;
    void stopPlayback() override;
    void punchOut() override;
    void resetPlayback(const RealTime &oldPosition, const RealTime &position) override;
    void allNotesOff();

    RealTime getSequencerTime() override;

    /// Get all pending input events from ALSA as a MappedEventList.
    /**
     * Called by RosegardenSequencer::processRecordedMidi() when recording and
     * RosegardenSequencer::processAsynchronousEvents() when playing or
     * stopped.
     *
     * These events are processed by RosegardenDocument::insertRecordedMidi()
     * in the GUI thread.
     */
    bool getMappedEventList(MappedEventList &mappedEventList) override;
    
    bool record(RecordStatus recordStatus,
                const std::vector<InstrumentId> &armedInstruments,
                const std::vector<QString> &audioFileNames) override;

    void startClocks() override;
    virtual void startClocksApproved(); // called by JACK driver in sync mode
    void stopClocks() override;
    bool areClocksRunning() const override { return m_queueRunning; }

    /// Send both MIDI and audio events out, unqueued
    /**
     * This version sends MIDI data to ALSA for transmission via MIDI
     * immediately.  (I assume audio events are also sent immediately.)
     */
    void processEventsOut(const MappedEventList &mC) override;
    /// Send both MIDI and audio events out, queued
    /**
     * Used by RosegardenSequencer::keepPlaying() to send events out
     * during playback.
     */
    void processEventsOut(const MappedEventList &mC,
                                  const RealTime &sliceStart,
                                  const RealTime &sliceEnd) override;

    // Return the sample rate
    //
    unsigned int getSampleRate() const override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getSampleRate();
        else return 0;
#else
        return 0;
#endif
    }

    // Define here to catch this being reset
    //
    void setMIDIClockInterval(RealTime interval) override;

    // initialise subsystems
    //
    bool initialiseMidi();
    void initialiseAudio();

    // Some stuff to help us debug this
    //
    void getSystemInfo();
    void showQueueStatus(int queue);

    // Process pending
    //
    void processPending() override;

    RealTime getAudioPlayLatency() override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getAudioPlayLatency();
#endif
        return RealTime::zeroTime;
    }

    RealTime getAudioRecordLatency() override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getAudioRecordLatency();
#endif
        return RealTime::zeroTime;
    }

    RealTime getInstrumentPlayLatency(InstrumentId id) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getInstrumentPlayLatency(id);
#else
        Q_UNUSED(id);
#endif
        return RealTime::zeroTime;
    }

    RealTime getMaximumPlayLatency() override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getMaximumPlayLatency();
#endif
        return RealTime::zeroTime;
    }
        

    // Plugin instance management
    //
    void setPluginInstance(InstrumentId id,
                                   QString identifier,
                                   int position) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstance(id, identifier, position);
#else
        Q_UNUSED(id);
        Q_UNUSED(identifier);
        Q_UNUSED(position);
#endif
    }

    void removePluginInstance(InstrumentId id, int position) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->removePluginInstance(id, position);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
#endif
    }

    void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstancePortValue(id, position, portNumber, value);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(portNumber);
        Q_UNUSED(value);
#endif
    }

    float getPluginInstancePortValue(InstrumentId id,
                                             int position,
                                             unsigned long portNumber) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstancePortValue(id, position, portNumber);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(portNumber);
#endif
        return 0;
    }

    void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstanceBypass(id, position, value);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(value);
#endif
    }

    QStringList getPluginInstancePrograms(InstrumentId id,
                                                  int position) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstancePrograms(id, position);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
#endif
        return QStringList();
    }

    QString getPluginInstanceProgram(InstrumentId id,
                                             int position) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
#endif
        return QString();
    }

    QString getPluginInstanceProgram(InstrumentId id,
                                             int position,
                                             int bank,
                                             int program) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position, bank, program);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(bank);
        Q_UNUSED(program);
#endif
        return QString();
    }

    unsigned long getPluginInstanceProgram(InstrumentId id,
                                                   int position,
                                                   QString name) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position, name);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(name);
#endif
        return 0;
    }
    
    void setPluginInstanceProgram(InstrumentId id,
                                          int position,
                                          QString program) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstanceProgram(id, position, program);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(program);
#endif
    }

    QString configurePlugin(InstrumentId id,
                                    int position,
                                    QString key,
                                    QString value) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->configurePlugin(id, position, key, value);
#else
        Q_UNUSED(id);
        Q_UNUSED(position);
        Q_UNUSED(key);
        Q_UNUSED(value);
#endif
        return QString();
    }

    void setAudioBussLevels(int bussId,
                                    float dB,
                                    float pan) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setAudioBussLevels(bussId, dB, pan);
#else
        Q_UNUSED(bussId);
        Q_UNUSED(dB);
        Q_UNUSED(pan);
#endif
    }

    void setAudioInstrumentLevels(InstrumentId instrument,
                                          float dB,
                                          float pan) override {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setAudioInstrumentLevels(instrument, dB, pan);
#else
        Q_UNUSED(instrument);
        Q_UNUSED(dB);
        Q_UNUSED(pan);
#endif
    }

    void claimUnwantedPlugin(void *plugin) override;
    void scavengePlugins() override;

    /// Update Ports and Connections
    /**
     * Updates m_alsaPorts and m_devicePortMap to match the ports and
     * connections that are out there.
     *
     * ??? rename: updatePortsAndConnections() or just update()?
     */
    void checkForNewClients() override;

    void setLoop(const RealTime &loopStart, const RealTime &loopEnd) override;

    void sleep(const RealTime &) override;

    // ----------------------- End of Virtuals ----------------------

    // Create and send an MMC command
    //
    void sendMMC(MidiByte deviceId,
                 MidiByte instruction,
                 bool isCommand,
                 const std::string &data);

    // Check whether the given event is an MMC command we need to act on
    // (and if so act on it)
    //
    bool testForMMCSysex(const snd_seq_event_t *event);

    // Create and enqueue a batch of MTC quarter-frame events
    //
    void insertMTCQFrames(RealTime sliceStart, RealTime sliceEnd);

    // Create and enqueue an MTC full-frame system exclusive event
    //
    void insertMTCFullFrame(RealTime time);

    // Parse and accept an incoming MTC quarter-frame event
    //
    void handleMTCQFrame(unsigned int data_byte, RealTime the_time);

    // Check whether the given event is an MTC sysex we need to act on
    // (and if so act on it)
    //
    bool testForMTCSysex(const snd_seq_event_t *event);

    // Adjust the ALSA clock skew for MTC lock
    //
    void tweakSkewForMTC(int factor);

    // Recalibrate internal MTC factors
    //
    void calibrateMTC();

    // Send a System message straight away
    //
    void sendSystemDirect(MidiByte command, int *arg);

    // Scheduled system message with arguments
    //
    void sendSystemQueued(MidiByte command,
                          const std::string &args,
                          const RealTime &time);

    // Set the record device
    //
    void setRecordDevice(DeviceId id, bool connectAction);
    void unsetRecordDevices();

    bool addDevice(Device::DeviceType type,
                           DeviceId id,
                           InstrumentId baseInstrumentId,
                           MidiDevice::DeviceDirection direction) override;
    void removeDevice(DeviceId id) override;
    void removeAllDevices() override;
    void renameDevice(DeviceId id, QString name) override;

    // Get available connections per device
    // 
    unsigned int getConnections(Device::DeviceType type,
                                        MidiDevice::DeviceDirection direction) override;
    QString getConnection(Device::DeviceType type,
                                  MidiDevice::DeviceDirection direction,
                                  unsigned int connectionNo) override;
    QString getConnection(DeviceId id) override;
    void setConnection(DeviceId deviceId, QString connection) override;
    void setPlausibleConnection(DeviceId deviceId,
                                QString idealConnection,
                                bool recordDevice = false) override;
    void connectSomething() override;

    unsigned int getTimers() override;
    QString getTimer(unsigned int) override;
    QString getCurrentTimer() override;
    void setCurrentTimer(QString) override;
 
    void getAudioInstrumentNumbers(InstrumentId &audioInstrumentBase,
                                           int &audioInstrumentCount) override {
        audioInstrumentBase = AudioInstrumentBase;
#ifdef HAVE_LIBJACK
        audioInstrumentCount = AudioInstrumentCount;
#else
        audioInstrumentCount = 0;
#endif
    }
 
    void getSoftSynthInstrumentNumbers(InstrumentId &ssInstrumentBase,
                                               int &ssInstrumentCount) override {
        ssInstrumentBase = SoftSynthInstrumentBase;
        ssInstrumentCount = SoftSynthInstrumentCount;
    }

    QString getStatusLog() override;

    // To be called regularly from JACK driver when idle
    void checkTimerSync(size_t frames);

    void runTasks() override;

    // Report a failure back to the GUI
    //
    void reportFailure(MappedEvent::FailureCode code) override;

protected:
    void clearDevices();

    ClientPortPair getFirstDestination(bool duplex);
    ClientPortPair getPairForMappedInstrument(InstrumentId id);
    int getOutputPortForMappedInstrument(InstrumentId id);

    /// Map of note-on events indexed by "channel note".
    /**
     * A "channel note" is a combination channel and note: (channel << 8) + note.
     */
    typedef std::multimap<unsigned int /*channelNote*/, MappedEvent *> ChannelNoteOnMap;
    /// Two-dimensional note-on map indexed by deviceID and "channel note".
    typedef std::map<unsigned int /*deviceID*/, ChannelNoteOnMap > NoteOnMap;
    /// Map of note-on events to match up with note-off's.
    /**
     * Indexed by device ID and "channelNote".
     *
     * Used by AlsaDriver::getMappedEventList().
     */
    NoteOnMap m_noteOnMap;

    typedef std::vector<QSharedPointer<AlsaPortDescription> > AlsaPortVector;

    /// Bring m_alsaPorts up-to-date.
    /**
     * Polls ALSA ports and should be called when a client or port event
     * arrives from ALSA.
     *
     * Called from initialiseMidi() and checkForNewClients().
     */
    virtual void generatePortList();
    void generateFixedInstruments();

    virtual void generateTimerList();
    virtual std::string getAutoTimer(bool &wantTimerChecks);

    void addInstrumentsForDevice(MappedDevice *device, InstrumentId base);
    MappedDevice *createMidiDevice(DeviceId deviceId,
                                   MidiDevice::DeviceDirection);

    /// Send MIDI out via ALSA.
    /**
     * For unqueued (immediate) send, specify RealTime::zeroTime for
     * sliceStart and sliceEnd.  Otherwise events will be queued for
     * future send at appropriate times.
     *
     * Used by processEventsOut() to send MIDI out via ALSA.
     */
    void processMidiOut(const MappedEventList &mC,
                        const RealTime &sliceStart,
                        const RealTime &sliceEnd);

    virtual void processSoftSynthEventOut(InstrumentId id,
                                          const snd_seq_event_t *event,
                                          bool now);

    virtual bool isRecording(AlsaPortDescription *port);

    virtual void processAudioQueue(bool /* now */) { }

private:
    RealTime getAlsaTime();

    // Locally convenient to control our devices
    //
    void sendDeviceController(DeviceId device,
                              MidiByte byte1,
                              MidiByte byte2);
                              
    int checkAlsaError(int rc, const char *message);

    /// The ALSA ports.
    AlsaPortVector m_alsaPorts;
    void setFirstConnection(DeviceId deviceId, bool recordDevice);

    bool m_startPlayback;

    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_client;

    int                          m_inputPort;
    
    typedef std::map<DeviceId, int /* portNumber */> DeviceIntMap;
    // ??? The ports that are created for each output device in the
    //     Composition/Studio?
    DeviceIntMap m_outputPorts;

    int                          m_syncOutputPort;
    int                          m_externalControllerPort;

    int                          m_queue;
    int                          m_maxClients;
    int                          m_maxPorts;
    int                          m_maxQueues;

    // Because this can fail even if the driver's up (if
    // another service is using the port say)
    //
    bool                         m_midiInputPortConnected;

    bool                         m_midiSyncAutoConnect;

    RealTime                     m_alsaPlayStartTime;
    RealTime                     m_alsaRecordStartTime;

    RealTime                     m_loopStartTime;
    RealTime                     m_loopEndTime;

    // MIDI Time Code handling:

    unsigned int                 m_eat_mtc;
    // Received/emitted MTC data breakdown:
    RealTime                     m_mtcReceiveTime;
    RealTime                     m_mtcEncodedTime;
    int                          m_mtcFrames;
    int                          m_mtcSeconds;
    int                          m_mtcMinutes;
    int                          m_mtcHours;
    int                          m_mtcSMPTEType;

    // Calculated MTC factors:
    int                          m_mtcFirstTime;
    RealTime                     m_mtcLastEncoded;
    RealTime                     m_mtcLastReceive;
    long long int                m_mtcSigmaE;
    long long int                m_mtcSigmaC;
    unsigned int                 m_mtcSkew;

    bool                         m_looping;

    bool                         m_haveShutdown;

    // Track System Exclusive Event across several ALSA messages
    // ALSA may break long system exclusive messages into chunks.
    typedef std::map<unsigned int,
                     std::pair<MappedEvent *, std::string> > DeviceEventMap;
    DeviceEventMap             *m_pendSysExcMap;
    
    /**
     * Clear all accumulated incompete System Exclusive messages.
     */
    void clearPendSysExcMap();
    
#ifdef HAVE_LIBJACK
    JackDriver *m_jackDriver;
#endif

    Scavenger<RunnablePluginInstance> m_pluginScavenger;

    //!!! -- hoist to SoundDriver w/setter?
    typedef std::set<InstrumentId> InstrumentSet;
    InstrumentSet m_recordingInstruments;

    typedef std::vector<MappedDevice *> MappedDeviceList;
    /// The devices in the Composition.
    MappedDeviceList m_devices;
    MappedDevice *findDevice(DeviceId deviceId);

    // ??? rename: ConnectionMap
    typedef std::map<DeviceId, ClientPortPair> DevicePortMap;
    /// Composition/Studio MappedDevice -> ALSA ClientPortPair connections.
    /**
     * ??? rename: m_connectionMap
     */
    DevicePortMap m_devicePortMap;
    void setConnectionToDevice(MappedDevice &device, QString connection);
    void setConnectionToDevice(MappedDevice &device, QString connection,
                               const ClientPortPair &pair);
    /// Return whether the client/port is in m_devicePortMap.
    bool portInUse(int client, int port) const;
    /// Is the given deviceId within m_devicePortMap connected?
    bool isConnected(DeviceId deviceId) const;

    std::string getPortName(ClientPortPair port);
    ClientPortPair getPortByName(std::string name);

    struct AlsaTimerInfo {
        int clas;
        int sclas;
        int card;
        int device;
        int subdevice;
        std::string name;
        long resolution;
    };
    std::vector<AlsaTimerInfo> m_timers;
    QString m_currentTimer;

    /// A time ordered set of pending MIDI NoteOffEvent objects.
    /**
     * This is used to turn off all notes when Stop is pressed.
     *
     * See processNotesOff().
     */
    NoteOffQueue m_noteOffQueue;

    /// Send out the note-off events in m_noteOffQueue
    /**
     * Only send out the note-offs up to "time".  Note-offs in the future
     * are scheduled to happen at the proper times.
     * If "now" is true, the events are sent immediately, even if they would
     * be in the future.  If "everything" is true, "time" is ignored and all
     * note-off events in the queue are sent.  This is used for shutdown.
     */
    void processNotesOff(const RealTime &time, bool now, bool everything = false);

    // This auxiliary queue is here as a hack, to avoid stuck notes if
    // resetting playback while a note-off is currently in the ALSA
    // queue.  When playback is reset by ffwd or rewind etc, we drop
    // all the queued events (which is generally what is desired,
    // except for note offs) and reset the queue timer (so the note
    // offs would have the wrong time stamps even if we hadn't dropped
    // them).  Thus, we need to re-send any recent note offs before
    // continuing.  This queue records which note offs have been
    // added to the ALSA queue recently.
    //
    NoteOffQueue m_recentNoteOffs;
    void pushRecentNoteOffs(); // move from recent to normal queue after reset
    void cropRecentNoteOffs(const RealTime &t); // remove old note offs
    void weedRecentNoteOffs(unsigned int pitch, MidiByte channel,
			    InstrumentId instrument); // on subsequent note on

    bool m_queueRunning;
    
    /// An ALSA client or port event was received.
    /**
     * checkForNewClients() will handle.
     */
    bool m_portCheckNeeded;

    enum { NeedNoJackStart, NeedJackReposition, NeedJackStart } m_needJackStart;
    
    bool m_doTimerChecks;
    bool m_firstTimerCheck;
    double m_timerRatio;
    bool m_timerRatioCalculated;

    std::string getAlsaVersion();
    std::string getKernelVersionString();
    void extractVersion(std::string vstr, int &major, int &minor, int &subminor, std::string &suffix);
    bool versionIsAtLeast(std::string vstr, int major, int minor, int subminor);

    QMutex m_mutex;

    /// Add an event to be returned by getMappedEventList().
    /**
     * Used by AlsaDriver::punchOut() to send an AudioGeneratePreview message
     * to the GUI.
     *
     * Old comments:
     * "We can return audio control signals to the GUI using MappedEvents.
     *  Meter levels or audio file completions can go in here."
     *
     * @see m_returnComposition
     */
    void insertMappedEventForReturn(MappedEvent *mE);

    /// Holds events to be returned by getMappedEventList().
    /**
     * Rename this to something less confusing.  "Composition" has a very
     * specific meaning in rg.  This is not a Composition object.
     * This object is a holding area for events that need to be returned
     * at a later point.  Investigate its purpose, then come up with a
     * better name.  m_mappedEventsForReturn?  m_audioGeneratePreviewEvents?
     *
     * @see insertMappedEventForReturn()
     */
    MappedEventList m_returnComposition;

    /// Debugging mode is enabled.
    /**
     * Moving debug configuration to run-time to make it a little
     * easier for users to turn on and off.  Downside is the need
     * for "if (m_debug)" which uses CPU as opposed to #if.
     *
     * To enable debugging, do a debug build and set the following
     * in the Rosegarden.conf file:
     *
     *   [General_Options]
     *   debug_AlsaDriver=true
     */
    bool m_debug;

    /// Reduce the amount of debug output.
    /**
     * Debugging real-time code is rather challenging.  It's easy to
     * end up with too much output, and the behavior will change as
     * the debugging sucks up CPU.
     *
     * This function is intended to reduce the amount of debug output
     * by only allowing output at fixed intervals, and for short
     * bursts.
     */
    bool throttledDebug() const;

    /// Compute SPP at or prior to time.
    int songPositionPointer(const RealTime &time);

    /// See handleTransportCCs().
    bool m_acceptTransportCCs;
    /// Handle CCs like 117 for play, 116 for stop, etc...
    /**
     * Returns true if handled and the CC can be discarded.
     */
    bool handleTransportCCs(unsigned controlNumber, int value);

    /// Whether we are sending MIDI Clocks (transport master).
    /**
     * ??? This is basically (m_midiSyncStatus == TRANSPORT_MASTER).  It is
     *     likely redundant and m_midiSyncStatus can be used instead.
     */
    bool                         m_midiClockEnabled;

    MappedInstrument *getMappedInstrument(InstrumentId id);

    /// Cancel the playback of an audio file.
    /**
     * Either by instrument and audio file id or by audio segment id.
     */
    void cancelAudioFile(MappedEvent *mE);

    void clearAudioQueue();

    TransportSyncStatus m_midiSyncStatus;
    TransportSyncStatus m_mmcStatus;
    TransportSyncStatus m_mtcStatus;

};


}

#endif // HAVE_ALSA

#endif // RG_ALSADRIVER_H
