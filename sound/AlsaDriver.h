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

// Specialisation of SoundDriver to support ALSA (http://www.alsa-project.org)
//
//
#ifndef _ALSADRIVER_H_
#define _ALSADRIVER_H_

#include <vector>
#include <set>

#include "config.h"

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h> // ALSA

#include "SoundDriver.h"
#include "Instrument.h"
#include "Device.h"
#include "AlsaPort.h"

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
    virtual void resetPlayback(const RealTime &position);
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);

    virtual RealTime getSequencerTime();

    virtual MappedComposition *getMappedComposition();
    
    virtual bool record(RecordStatus recordStatus);

    virtual void startClocks();
    virtual void startClocksApproved(); // called by JACK driver in sync mode
    virtual void stopClocks();
    virtual bool areClocksRunning() { return m_queueRunning; }

    virtual void processEventsOut(const MappedComposition &mC,
                                  bool now);

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


    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   unsigned long pluginId,
                                   int position) {
#ifdef HAVE_LIBJACK
	if (m_jackDriver) m_jackDriver->setPluginInstance(id, pluginId, position);
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

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value) {
#ifdef HAVE_LIBJACK
	if (m_jackDriver) m_jackDriver->setPluginInstanceBypass(id, position, value);
#endif
    }

    virtual bool checkForNewClients();

    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd);

    // Send the MIDI clock
    //
    virtual void sendMidiClock();

    virtual void sleep(const RealTime &);

    // ----------------------- End of Virtuals ----------------------

    // Create and send an MMC command
    //
    void sendMMC(MidiByte deviceId,
                 MidiByte instruction,
                 bool isCommand,
                 const std::string &data);

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
    void setRecordDevice(DeviceId id);
    void unsetRecordDevices();

    virtual bool canReconnect(Device::DeviceType type);

    virtual DeviceId addDevice(Device::DeviceType type,
			       MidiDevice::DeviceDirection direction);
    virtual void removeDevice(DeviceId id);

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

    virtual QString getStatusLog();

protected:
    typedef std::vector<AlsaPortDescription *> AlsaPortList;

    ClientPortPair getFirstDestination(bool duplex);
    ClientPortPair getPairForMappedInstrument(InstrumentId id);

    /**
     * Bring m_alsaPorts up-to-date; if newPorts is non-null, also
     * return the new ports (not previously in m_alsaPorts) through it
     */
    virtual void generatePortList(AlsaPortList *newPorts = 0);
    virtual void generateInstruments();

    virtual void generateTimerList();

    void addInstrumentsForDevice(MappedDevice *device);
    MappedDevice *createMidiDevice(AlsaPortDescription *,
				   MidiDevice::DeviceDirection);

    virtual void processMidiOut(const MappedComposition &mC,
                                bool now);

    virtual void processAudioQueue(bool /* now */) { }

private:
    RealTime getAlsaTime();

    // Locally convenient to control our devices
    //
    void sendDeviceController(const ClientPortPair &device,
                              MidiByte byte1,
                              MidiByte byte2);

    AlsaPortList m_alsaPorts;

    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_client;
    int                          m_port;
    int                          m_queue;
    int                          m_maxClients;
    int                          m_maxPorts;
    int                          m_maxQueues;

    NoteOffQueue                 m_noteOffQueue;

    // Because this can fail even if the driver's up (if
    // another service is using the port say)
    //
    bool                         m_midiInputPortConnected;

    RealTime                     m_alsaPlayStartTime;
    RealTime                     m_alsaRecordStartTime;

    RealTime                     m_loopStartTime;
    RealTime                     m_loopEndTime;
    bool                         m_looping;

    bool                         m_haveShutdown;

#ifdef HAVE_LIBJACK
    JackDriver *m_jackDriver;
#endif

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
    };
    std::vector<AlsaTimerInfo> m_timers;
    std::string m_currentTimer;

    bool m_queueRunning;
};

}

#endif // HAVE_ALSA

#endif // _ALSADRIVER_H_

