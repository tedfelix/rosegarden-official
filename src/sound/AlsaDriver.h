/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "base/Instrument.h"
#include "base/Device.h"
#include "AlsaPort.h"
#include "Scavenger.h"
#include "RunnablePluginInstance.h"

#ifdef HAVE_LIBJACK
#include "JackDriver.h"
#endif

#include <alsa/asoundlib.h> // ALSA

#include <QMutex>

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
    virtual ~AlsaDriver();

    // shutdown everything that's currently open
    void shutdown();

    virtual bool initialise();
    virtual void initialisePlayback(const RealTime &position);
    virtual void stopPlayback();
    virtual void punchOut();
    virtual void resetPlayback(const RealTime &oldPosition, const RealTime &position);
    virtual void allNotesOff();

    virtual RealTime getSequencerTime();

    /// Get MIDI data from ALSA
    /**
     * Called by RosegardenSequencer::processRecordedMidi() when recording and
     * RosegardenSequencer::processAsynchronousEvents() when playing or
     * stopped.
     *
     * These events are processed by RosegardenDocument::insertRecordedMidi()
     * in the GUI thread.
     */
    virtual bool getMappedEventList(MappedEventList &mappedEventList);
    
    virtual bool record(RecordStatus recordStatus,
                        const std::vector<InstrumentId> *armedInstruments = 0,
                        const std::vector<QString> *audioFileNames = 0);

    virtual void startClocks();
    virtual void startClocksApproved(); // called by JACK driver in sync mode
    virtual void stopClocks();
    virtual bool areClocksRunning() const { return m_queueRunning; }

    /// Send both MIDI and audio events out, unqueued
    /**
     * This version sends MIDI data to ALSA for transmission via MIDI
     * immediately.  (I assume audio events are also sent immediately.)
     */
    virtual void processEventsOut(const MappedEventList &mC);
    /// Send both MIDI and audio events out, queued
    /**
     * Used by RosegardenSequencer::keepPlaying() to send events out
     * during playback.
     */
    virtual void processEventsOut(const MappedEventList &mC,
                                  const RealTime &sliceStart,
                                  const RealTime &sliceEnd);

    // Return the sample rate
    //
    virtual unsigned int getSampleRate() const {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getSampleRate();
        else return 0;
#else
        return 0;
#endif
    }

    // Define here to catch this being reset
    //
    virtual void setMIDIClockInterval(RealTime interval);

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
    virtual void processPending();

    virtual RealTime getAudioPlayLatency() {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getAudioPlayLatency();
#endif
        return RealTime::zeroTime;
    }

    virtual RealTime getAudioRecordLatency() {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getAudioRecordLatency();
#endif
        return RealTime::zeroTime;
    }

    virtual RealTime getInstrumentPlayLatency(InstrumentId id) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getInstrumentPlayLatency(id);
#endif
        return RealTime::zeroTime;
    }

    virtual RealTime getMaximumPlayLatency() {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getMaximumPlayLatency();
#endif
        return RealTime::zeroTime;
    }
        

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   QString identifier,
                                   int position) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstance(id, identifier, position);
#endif
    }

    virtual void removePluginInstance(InstrumentId id, int position) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->removePluginInstance(id, position);
#endif
    }

    // Remove all plugin instances
    //
    virtual void removePluginInstances() {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->removePluginInstances();
#endif
    }

    virtual void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstancePortValue(id, position, portNumber, value);
#endif
    }

    virtual float getPluginInstancePortValue(InstrumentId id,
                                             int position,
                                             unsigned long portNumber) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstancePortValue(id, position, portNumber);
#endif
        return 0;
    }

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstanceBypass(id, position, value);
#endif
    }

    virtual QStringList getPluginInstancePrograms(InstrumentId id,
                                                  int position) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstancePrograms(id, position);
#endif
        return QStringList();
    }

    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position);
#endif
        return QString();
    }

    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position,
                                             int bank,
                                             int program) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position, bank, program);
#endif
        return QString();
    }

    virtual unsigned long getPluginInstanceProgram(InstrumentId id,
                                                   int position,
                                                   QString name) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->getPluginInstanceProgram(id, position, name);
#endif
        return 0;
    }
    
    virtual void setPluginInstanceProgram(InstrumentId id,
                                          int position,
                                          QString program) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setPluginInstanceProgram(id, position, program);
#endif
    }

    virtual QString configurePlugin(InstrumentId id,
                                    int position,
                                    QString key,
                                    QString value) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) return m_jackDriver->configurePlugin(id, position, key, value);
#endif
        return QString();
    }

    virtual void setAudioBussLevels(int bussId,
                                    float dB,
                                    float pan) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setAudioBussLevels(bussId, dB, pan);
#endif
    }

    virtual void setAudioInstrumentLevels(InstrumentId instrument,
                                          float dB,
                                          float pan) {
#ifdef HAVE_LIBJACK
        if (m_jackDriver) m_jackDriver->setAudioInstrumentLevels(instrument, dB, pan);
#endif
    }

    virtual void claimUnwantedPlugin(void *plugin);
    virtual void scavengePlugins();

    virtual bool checkForNewClients();

    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd);

    virtual void sleep(const RealTime &);

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

    virtual bool canReconnect(Device::DeviceType type);

    virtual bool addDevice(Device::DeviceType type,
                           DeviceId id,
                           InstrumentId baseInstrumentId,
                           MidiDevice::DeviceDirection direction);
    virtual void removeDevice(DeviceId id);
    virtual void removeAllDevices();
    virtual void renameDevice(DeviceId id, QString name);

    // Get available connections per device
    // 
    virtual unsigned int getConnections(Device::DeviceType type,
                                        MidiDevice::DeviceDirection direction);
    virtual QString getConnection(Device::DeviceType type,
                                  MidiDevice::DeviceDirection direction,
                                  unsigned int connectionNo);
    virtual QString getConnection(DeviceId id);
    virtual void setConnection(DeviceId deviceId, QString connection);
    virtual void setPlausibleConnection(DeviceId deviceId,
                                        QString connection,
                                        bool recordDevice = false);
    virtual void connectSomething();

    virtual unsigned int getTimers();
    virtual QString getTimer(unsigned int);
    virtual QString getCurrentTimer();
    virtual void setCurrentTimer(QString);
 
    virtual void getAudioInstrumentNumbers(InstrumentId &audioInstrumentBase,
                                           int &audioInstrumentCount) {
        audioInstrumentBase = AudioInstrumentBase;
#ifdef HAVE_LIBJACK
        audioInstrumentCount = AudioInstrumentCount;
#else
        audioInstrumentCount = 0;
#endif
    }
 
    virtual void getSoftSynthInstrumentNumbers(InstrumentId &ssInstrumentBase,
                                               int &ssInstrumentCount) {
        ssInstrumentBase = SoftSynthInstrumentBase;
        ssInstrumentCount = SoftSynthInstrumentCount;
    }

    virtual QString getStatusLog();

    // To be called regularly from JACK driver when idle
    void checkTimerSync(size_t frames);

    virtual void runTasks();

    // Report a failure back to the GUI
    //
    virtual void reportFailure(MappedEvent::FailureCode code);

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

    typedef std::vector<AlsaPortDescription *> AlsaPortList;

    /**
     * Bring m_alsaPorts up-to-date; if newPorts is non-null, also
     * return the new ports (not previously in m_alsaPorts) through it
     */
    virtual void generatePortList(AlsaPortList *newPorts = 0);
    virtual void generateFixedInstruments();

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
    virtual void processMidiOut(const MappedEventList &mC,
                                const RealTime &sliceStart,
                                const RealTime &sliceEnd);

    virtual void processSoftSynthEventOut(InstrumentId id,
                                          const snd_seq_event_t *event,
                                          bool now);

    virtual bool isRecording(AlsaPortDescription *port);

    virtual void processAudioQueue(bool /* now */) { }

    virtual void setConnectionToDevice(MappedDevice &device, QString connection);
    virtual void setConnectionToDevice(MappedDevice &device, QString connection,
                                       const ClientPortPair &pair);

private:
    RealTime getAlsaTime();

    // Locally convenient to control our devices
    //
    void sendDeviceController(DeviceId device,
                              MidiByte byte1,
                              MidiByte byte2);
                              
    int checkAlsaError(int rc, const char *message);

    AlsaPortList m_alsaPorts;

    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_client;

    int                          m_inputPort;
    
    typedef std::map<DeviceId, int> DeviceIntMap;
    DeviceIntMap                 m_outputPorts;

    int                          m_syncOutputPort;
    int                          m_controllerPort;

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

    typedef std::map<DeviceId, ClientPortPair> DevicePortMap;
    DevicePortMap m_devicePortMap;

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
    std::string m_currentTimer;

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
    
    bool m_portCheckNeeded;

    enum { NeedNoJackStart, NeedJackReposition, NeedJackStart } m_needJackStart;
    
    bool m_doTimerChecks;
    bool m_firstTimerCheck;
    double m_timerRatio;
    bool m_timerRatioCalculated;

    std::string getAlsaModuleVersionString();
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
};

}

#endif // HAVE_ALSA

#endif // RG_ALSADRIVER_H
