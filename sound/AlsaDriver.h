// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

// Specialisation of SoundDriver to support ALSA (http://www.alsa-project.org)
//
//
#ifndef _ALSADRIVER_H_
#define _ALSADRIVER_H_

#include <vector>
#include <set>
#include <map>

#include "config.h"

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h> // ALSA

#include "SoundDriver.h"
#include "Instrument.h"
#include "Device.h"
#include "AlsaPort.h"
#include "Scavenger.h"
#include "RunnablePluginInstance.h"

#ifdef HAVE_LIBJACK
#include "JackDriver.h"
#endif

namespace Rosegarden
{

class AlsaDriver : public SoundDriver
{
public:
    AlsaDriver(MappedStudio *studio);
    virtual ~AlsaDriver();

    // shutdown everything that's currently open
    void shutdown();

    virtual void initialise();
    virtual void initialisePlayback(const RealTime &position);
    virtual void stopPlayback();
    virtual void punchOut();
    virtual void resetPlayback(const RealTime &oldPosition, const RealTime &position);
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time, bool now);

    virtual RealTime getSequencerTime();

    virtual MappedComposition *getMappedComposition();
    
    virtual bool record(RecordStatus recordStatus,
			const std::vector<InstrumentId> *armedInstruments = 0,
			const std::vector<QString> *audioFileNames = 0);

    virtual void startClocks();
    virtual void startClocksApproved(); // called by JACK driver in sync mode
    virtual void stopClocks();
    virtual bool areClocksRunning() const { return m_queueRunning; }

    virtual void processEventsOut(const MappedComposition &mC);
    virtual void processEventsOut(const MappedComposition &mC,
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
    void initialiseMidi();
    void initialiseAudio();

    // Some stuff to help us debug this
    //
    void getSystemInfo();
    void showQueueStatus(int queue);

    // Process pending
    //
    virtual void processPending();

    // We can return audio control signals to the gui using MappedEvents.
    // Meter levels or audio file completions can go in here.
    //
    void insertMappedEventForReturn(MappedEvent *mE);

    
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
    void sendSystemDirect(MidiByte command,
                          const std::string &args);

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

    virtual DeviceId addDevice(Device::DeviceType type,
			       MidiDevice::DeviceDirection direction);
    virtual void removeDevice(DeviceId id);
    virtual void renameDevice(DeviceId id, QString name);

    // Get available connections per device
    // 
    virtual unsigned int getConnections(Device::DeviceType type,
					MidiDevice::DeviceDirection direction);
    virtual QString getConnection(Device::DeviceType type,
				  MidiDevice::DeviceDirection direction,
				  unsigned int connectionNo);
    virtual void setConnection(DeviceId deviceId, QString connection);
    virtual void setPlausibleConnection(DeviceId deviceId, QString connection);

    virtual unsigned int getTimers();
    virtual QString getTimer(unsigned int);
    virtual QString getCurrentTimer();
    virtual void setCurrentTimer(QString);
 
    virtual void getAudioInstrumentNumbers(InstrumentId &audioInstrumentBase,
					   int &audioInstrumentCount) {
	audioInstrumentBase = AudioInstrumentBase;
#ifdef HAVE_LIBJACK
	audioInstrumentCount = 16;
#else
	audioInstrumentCount = 0;
#endif
    }
 
    virtual void getSoftSynthInstrumentNumbers(InstrumentId &ssInstrumentBase,
					       int &ssInstrumentCount) {
	ssInstrumentBase = SoftSynthInstrumentBase;
#ifdef HAVE_DSSI
	ssInstrumentCount = 24;
#else
	ssInstrumentCount = 0;
#endif
    }

    virtual QString getStatusLog();

    // To be called regularly from JACK driver when idle
    void checkTimerSync(size_t frames);

    virtual void runTasks();

    // Report a failure back to the GUI
    //
    virtual void reportFailure(Rosegarden::MappedEvent::FailureCode code);

protected:
    typedef std::vector<AlsaPortDescription *> AlsaPortList;

    ClientPortPair getFirstDestination(bool duplex);
    ClientPortPair getPairForMappedInstrument(InstrumentId id);
    int getOutputPortForMappedInstrument(InstrumentId id);
    std::map<unsigned int, std::map<unsigned int, MappedEvent*> >  m_noteOnMap;

    /**
     * Bring m_alsaPorts up-to-date; if newPorts is non-null, also
     * return the new ports (not previously in m_alsaPorts) through it
     */
    virtual void generatePortList(AlsaPortList *newPorts = 0);
    virtual void generateInstruments();

    virtual void generateTimerList();
    virtual std::string getAutoTimer(bool &wantTimerChecks);

    void addInstrumentsForDevice(MappedDevice *device);
    MappedDevice *createMidiDevice(AlsaPortDescription *,
				   MidiDevice::DeviceDirection);

    virtual void processMidiOut(const MappedComposition &mC,
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
    void sendDeviceController(Rosegarden::DeviceId device,
                              MidiByte byte1,
                              MidiByte byte2);
			      
    int checkAlsaError(int rc, const char *message);

    AlsaPortList m_alsaPorts;

    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_client;

    int                          m_inputPort;
    
    typedef std::map<Rosegarden::DeviceId, int> DeviceIntMap;
    DeviceIntMap                 m_outputPorts;

    int                          m_syncOutputPort;

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

#ifdef HAVE_LIBJACK
    JackDriver *m_jackDriver;
#endif

    Scavenger<RunnablePluginInstance> m_pluginScavenger;

    //!!! -- hoist to SoundDriver w/setter?
    typedef std::set<Rosegarden::InstrumentId> InstrumentSet;
    InstrumentSet m_recordingInstruments;

    typedef std::map<DeviceId, ClientPortPair> DevicePortMap;
    DevicePortMap m_devicePortMap;

    typedef std::map<ClientPortPair, DeviceId> PortDeviceMap;
    PortDeviceMap m_suspendedPortMap;

    std::string getPortName(ClientPortPair port);
    ClientPortPair getPortByName(std::string name);

    DeviceId getSpareDeviceId();

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

    bool m_queueRunning;
    
    bool m_portCheckNeeded;

    enum { NeedNoJackStart, NeedJackReposition, NeedJackStart } m_needJackStart;
    
    bool m_doTimerChecks;
    bool m_firstTimerCheck;
    double m_timerRatio;
    bool m_timerRatioCalculated;
};

}

#endif // HAVE_ALSA

#endif // _ALSADRIVER_H_

