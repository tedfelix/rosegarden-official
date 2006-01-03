// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>
#include <alsa/seq.h>

#include "AlsaDriver.h"
#include "AlsaPort.h"
#include "MappedInstrument.h"
#include "Midi.h"
#include "MappedStudio.h"
#include "rosestrings.h"
#include "MappedCommon.h"
#include "MappedEvent.h"
#include "Audit.h"
#include "AudioPlayQueue.h"
#include "ExternalTransport.h"

#include <qregexp.h>
#include <pthread.h>


//#define DEBUG_ALSA 1
//#define DEBUG_PROCESS_MIDI_OUT 1
//#define MTC_DEBUG 1

// This driver implements MIDI in and out via the ALSA (www.alsa-project.org)
// sequencer interface.

using std::cerr;
using std::endl;

static size_t _debug_jack_frame_count = 0;

#define AUTO_TIMER_NAME "(auto)"


namespace Rosegarden
{

#define FAILURE_REPORT_COUNT 256
static MappedEvent::FailureCode _failureReports[FAILURE_REPORT_COUNT];
static int _failureReportWriteIndex = 0;
static int _failureReportReadIndex = 0;

AlsaDriver::AlsaDriver(MappedStudio *studio):
    SoundDriver(studio, std::string("alsa-lib version ") +
                        std::string(SND_LIB_VERSION_STR)),
    m_client(-1),
    m_inputPort(-1),
    m_syncOutputPort(-1),
    m_controllerPort(-1),
    m_queue(-1),
    m_maxClients(-1),
    m_maxPorts(-1),
    m_maxQueues(-1),
    m_midiInputPortConnected(false),
    m_midiSyncAutoConnect(false),
    m_alsaPlayStartTime(0, 0),
    m_alsaRecordStartTime(0, 0),
    m_loopStartTime(0, 0),
    m_loopEndTime(0, 0),
    m_looping(false),
    m_haveShutdown(false)
#ifdef HAVE_LIBJACK
    ,m_jackDriver(0)
#endif
    ,m_queueRunning(false)
    ,m_portCheckNeeded(false),
    m_needJackStart(NeedNoJackStart),
    m_doTimerChecks(false),
    m_firstTimerCheck(true),
    m_timerRatio(0),
    m_timerRatioCalculated(false)

{
    Audit audit;
    audit << "Rosegarden " << VERSION << " - AlsaDriver - " 
                 << m_name << std::endl;
}

AlsaDriver::~AlsaDriver()
{
    if (!m_haveShutdown) {
	std::cerr << "WARNING: AlsaDriver::shutdown() was not called before destructor, calling now" << std::endl;
	shutdown();
    }
}

int
AlsaDriver::checkAlsaError(int rc, const char *
#ifdef DEBUG_ALSA
			   message
#endif
    )
{
#ifdef DEBUG_ALSA
    if (rc < 0) 
    {
	std::cerr << "AlsaDriver::"
		  << message
		  << ": " << rc
		  << " (" << snd_strerror(rc) << ")" 
		  << std::endl;
    }
#endif
    return rc;
}

void
AlsaDriver::shutdown()
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::~AlsaDriver - shutting down" << std::endl;
#endif

#ifdef HAVE_LIBJACK
    delete m_jackDriver;
    m_jackDriver = 0;
#endif

    if (m_midiHandle)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closing MIDI client" << std::endl;
#endif

        checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, 0), "shutdown(): stopping queue");
	checkAlsaError(snd_seq_drain_output(m_midiHandle), "shutdown(): drain output");
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - stopped queue" << std::endl;
#endif
        snd_seq_close(m_midiHandle);
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closed MIDI handle" << std::endl;
#endif
        m_midiHandle = 0;
    }

    m_haveShutdown = true;
}

void
AlsaDriver::setLoop(const RealTime &loopStart, const RealTime &loopEnd)
{
    m_loopStartTime = loopStart;
    m_loopEndTime = loopEnd;

    // currently we use this simple test for looping - it might need
    // to get more sophisticated in the future.
    //
    if (m_loopStartTime != m_loopEndTime)
        m_looping = true;
    else
        m_looping = false;
}

void
AlsaDriver::getSystemInfo()
{
    int err;
    snd_seq_system_info_t *sysinfo;

    snd_seq_system_info_alloca(&sysinfo);

    if ((err = snd_seq_system_info(m_midiHandle, sysinfo)) < 0) {
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
	m_maxQueues = 0;
	m_maxClients = 0;
	m_maxPorts = 0;
	return;
    }

    m_maxQueues = snd_seq_system_info_get_queues(sysinfo); 
    m_maxClients = snd_seq_system_info_get_clients(sysinfo);
    m_maxPorts = snd_seq_system_info_get_ports(sysinfo);
}

void
AlsaDriver::showQueueStatus(int queue)
{
    int err, idx, min, max;
    snd_seq_queue_status_t *status;

    snd_seq_queue_status_alloca(&status);
    min = queue < 0 ? 0 : queue;
    max = queue < 0 ? m_maxQueues : queue + 1;

    for (idx = min; idx < max; ++idx)
    {
        if ((err = snd_seq_get_queue_status(m_midiHandle, idx, status))<0) {

            if (err == -ENOENT)
                continue;

            std::cerr << "Client " << idx << " info error: "
                      << snd_strerror(err) << std::endl;

	    reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
	    return;
        }

#ifdef DEBUG_ALSA
        std::cerr << "Queue " << snd_seq_queue_status_get_queue(status)
                  << std::endl;

        std::cerr << "Tick       = "
                  << snd_seq_queue_status_get_tick_time(status)
                  << std::endl;

        std::cerr << "Realtime   = "
                  << snd_seq_queue_status_get_real_time(status)->tv_sec
                  << "."
                  << snd_seq_queue_status_get_real_time(status)->tv_nsec
                  << std::endl;

        std::cerr << "Flags      = 0x"
                  << snd_seq_queue_status_get_status(status)
                  << std::endl;
#endif
    }

}


void
AlsaDriver::generateTimerList()
{
    // Enumerate the available timers

    snd_timer_t *timerHandle;

    snd_timer_id_t *timerId;
    snd_timer_info_t *timerInfo;

    snd_timer_id_alloca(&timerId);
    snd_timer_info_alloca(&timerInfo);

    snd_timer_query_t *timerQuery;
    char timerName[64];

    m_timers.clear();

    if (snd_timer_query_open(&timerQuery, "hw", 0) >= 0) {

	snd_timer_id_set_class(timerId, SND_TIMER_CLASS_NONE);

	while (1) {

	    if (snd_timer_query_next_device(timerQuery, timerId) < 0) break;
	    if (snd_timer_id_get_class(timerId) < 0) break;

	    AlsaTimerInfo info = {
		snd_timer_id_get_class(timerId),
		snd_timer_id_get_sclass(timerId),
		snd_timer_id_get_card(timerId),
		snd_timer_id_get_device(timerId),
		snd_timer_id_get_subdevice(timerId),
		"",
		0
	    };

	    if (info.card < 0) info.card = 0;
	    if (info.device < 0) info.device = 0;
	    if (info.subdevice < 0) info.subdevice = 0;

//	    std::cerr << "got timer: class " << info.clas << std::endl;

	    sprintf(timerName, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i",
		    info.clas, info.sclas, info.card, info.device, info.subdevice);

	    if (snd_timer_open(&timerHandle, timerName, SND_TIMER_OPEN_NONBLOCK) < 0) {
		std::cerr << "Failed to open timer: " << timerName << std::endl;
		continue;
	    }

	    if (snd_timer_info(timerHandle, timerInfo) < 0) continue;

	    info.name = snd_timer_info_get_name(timerInfo);
	    info.resolution = snd_timer_info_get_resolution(timerInfo);
	    snd_timer_close(timerHandle);

//	    std::cerr << "adding timer: " << info.name << std::endl;

	    m_timers.push_back(info);
	}

	snd_timer_query_close(timerQuery);
    }
}


std::string
AlsaDriver::getAutoTimer(bool &wantTimerChecks)
{
    Audit audit;
    
    // Look for the apparent best-choice timer.

    if (m_timers.empty()) return "";

    // The system RTC timer ought to be good, but it doesn't look like
    // a very safe choice -- we've seen some system lockups apparently
    // connected with use of this timer on 2.6 kernels.  So we avoid
    // using that as an auto option.

    // Looks like our most reliable options for timers are, in order:
    // 
    // 1. System timer if at 1000Hz, with timer checks (i.e. automatic
    //    drift correction against PCM frame count).  Only available
    //    when JACK is running.
    // 
    // 2. PCM playback timer currently in use by JACK (no drift, but
    //    suffers from jitter).
    // 
    // 3. System timer if at 1000Hz.
    // 
    // 4. System RTC timer.
    // 
    // 5. System timer.

    // As of Linux kernel 2.6.13 (?) the default system timer
    // resolution has been reduced from 1000Hz to 250Hz, giving us
    // only 4ms accuracy instead of 1ms.  This may be better than the
    // 10ms available from the stock 2.4 kernel, but it's not enough
    // for really solid MIDI timing.  If JACK is running at 44.1 or
    // 48KHz with a buffer size less than 256 frames, then the PCM
    // timer will give us less jitter.  Even at 256 frames, it may be
    // preferable in practice just because it's simpler.

    // However, we can't safely choose the PCM timer over the system
    // timer unless the latter has really awful resolution, because we
    // don't know for certain which PCM JACK is using.  We guess at
    // hw:0 for the moment, which gives us a stuck timer problem if
    // it's actually using something else.  So if the system timer
    // runs at 250Hz, we really have to choose it anyway and just give
    // a warning.

    wantTimerChecks = false; // for most options

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {

	// use system timer with timer checks if at 1000Hz or more

	for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
	     i != m_timers.end(); ++i) {
	    if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
	    if (i->clas == SND_TIMER_CLASS_GLOBAL) {
		if (i->device == SND_TIMER_GLOBAL_SYSTEM) {
		    long hz = 1000000000 / i->resolution;
		    if (hz > 240) {
			if (hz < 750) {
			    audit << "System timer is only " << hz << "Hz, sending a warning" << std::endl;
			    reportFailure(Rosegarden::MappedEvent::WarningImpreciseTimer);
			}
			wantTimerChecks = true;
		        return i->name;
		    }
		}
	    }
	}

	// look for the first PCM playback timer; that's all we know
	// about for now (until JACK becomes able to tell us which PCM
	// it's on)

	for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
	     i != m_timers.end(); ++i) {
	    if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
	    if (i->clas == SND_TIMER_CLASS_PCM) return i->name;
	}
    }
#endif

    // look for a system timer with a frequency of 1000Hz or more

    for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
	 i != m_timers.end(); ++i) {
	if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
	if (i->clas == SND_TIMER_CLASS_GLOBAL) {
	    if (i->device == SND_TIMER_GLOBAL_SYSTEM) {
		long hz = 1000000000 / i->resolution;
		if (hz > 240) {
		    if (hz < 750) {
			audit << "System timer is only " << hz << "Hz, sending a warning" << std::endl;
			reportFailure(Rosegarden::MappedEvent::WarningImpreciseTimer);
		    }
		    return i->name;
		}
	    }
	}
    }

    // look for the system RTC timer if available

/* No, this doesn't seem to be safe with some popular kernels

    for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
	 i != m_timers.end(); ++i) {
	if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
	if (i->clas == SND_TIMER_CLASS_GLOBAL) {
	    if (i->device == SND_TIMER_GLOBAL_RTC) return i->name;
	}
    }
*/

    // next look for slow, unpopular 100Hz 2.4 system timer

    for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
	 i != m_timers.end(); ++i) {
	if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
	if (i->clas == SND_TIMER_CLASS_GLOBAL) {
	    if (i->device == SND_TIMER_GLOBAL_SYSTEM) {
		audit << "Using low-resolution system timer, sending a warning" << std::endl;
		reportFailure(Rosegarden::MappedEvent::WarningImpreciseTimer);
		return i->name;
	    }
	}
    }

    // falling back to something that almost certainly won't work,
    // if for any reason all of the above failed

    return m_timers.begin()->name;
}



void
AlsaDriver::generatePortList(AlsaPortList *newPorts)
{
    Audit audit;
    AlsaPortList alsaPorts;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    audit << std::endl << "  ALSA Client information:"
		 << std::endl << std::endl;

    // Get only the client ports we're interested in and store them
    // for sorting and then device creation.
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0) continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0)
        {
	    int client = snd_seq_port_info_get_client(pinfo);
	    int port = snd_seq_port_info_get_port(pinfo);
	    unsigned int clientType = snd_seq_client_info_get_type(cinfo);
	    unsigned int portType = snd_seq_port_info_get_type(pinfo);
	    unsigned int capability = snd_seq_port_info_get_capability(pinfo);


            if ((((capability & writeCap) == writeCap) ||
                 ((capability & readCap)  == readCap)) &&
                ((capability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0))
            {
                audit << "    "
			     << client << ","
			     << port << " - ("
			     << snd_seq_client_info_get_name(cinfo) << ", "
			     << snd_seq_port_info_get_name(pinfo) << ")";

                PortDirection direction;

                if ((capability & SND_SEQ_PORT_CAP_DUPLEX) ||
                    ((capability & SND_SEQ_PORT_CAP_WRITE) &&
                     (capability & SND_SEQ_PORT_CAP_READ)))
                {
                    direction = Duplex;
                    audit << "\t\t\t(DUPLEX)";
                }
                else if (capability & SND_SEQ_PORT_CAP_WRITE)
                {
                    direction = WriteOnly;
                    audit << "\t\t(WRITE ONLY)";
                }
                else
                {
                    direction = ReadOnly;
                    audit << "\t\t(READ ONLY)";
                }

		audit << " [ctype " << clientType << ", ptype " << portType << ", cap " << capability << "]";

                // Generate a unique name using the client id
                //
		char portId[40];
		sprintf(portId, "%d:%d ", client, port);

                std::string fullClientName = 
                    std::string(snd_seq_client_info_get_name(cinfo));

                std::string fullPortName = 
                    std::string(snd_seq_port_info_get_name(pinfo));

		std::string name;

		// If the first part of the client name is the same as the 
                // start of the port name, just use the port name.  otherwise
                // concatenate.
                //
                int firstSpace = fullClientName.find(" ");

                // If no space is found then we try to match the whole string
                //
                if (firstSpace < 0) firstSpace = fullClientName.length();

                if (firstSpace > 0 &&
		    int(fullPortName.length()) >= firstSpace &&
		    fullPortName.substr(0, firstSpace) ==
		    fullClientName.substr(0, firstSpace)) {
		    name = portId + fullPortName;
		} else {
		    name = portId + fullClientName + ": " + fullPortName;
		}

                // Sanity check for length
                //
                if (name.length() > 35) name = portId + fullPortName;

		if (direction == WriteOnly) {
		    name += " (write)";
		} else if (direction == ReadOnly) {
		    name += " (read)";
		} else if (direction == Duplex) {
		    name += " (duplex)";
		}

                AlsaPortDescription *portDescription = 
                    new AlsaPortDescription(
                            Instrument::Midi,
                            name,
                            client,
			    port,
			    clientType,
			    portType,
			    capability,
                            direction);

		if (newPorts &&
		    (getPortName(ClientPortPair(client, port)) == "")) {
		    newPorts->push_back(portDescription);
		}

                alsaPorts.push_back(portDescription);

                audit << std::endl;
            }
        }
    }

    audit << std::endl;

    // Ok now sort by duplexicity
    //
    std::sort(alsaPorts.begin(), alsaPorts.end(), AlsaPortCmp());
    m_alsaPorts = alsaPorts;
}


void
AlsaDriver::generateInstruments()
{
    // Reset these before each Instrument hunt
    //
    int audioCount = 0;
    getAudioInstrumentNumbers(m_audioRunningId, audioCount);
    m_midiRunningId = MidiInstrumentBase;

    // Clear these
    //
    m_instruments.clear();
    m_devices.clear();
    m_devicePortMap.clear();
    m_suspendedPortMap.clear();

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); it++)
    {
	if ((*it)->m_client == m_client) {
	    std::cerr << "(Ignoring own port " << (*it)->m_client
		      << ":" << (*it)->m_port << ")" << std::endl;
	    continue;
	} else if ((*it)->m_client == 0) {
	    std::cerr << "(Ignoring system port " << (*it)->m_client
		      << ":" << (*it)->m_port << ")" << std::endl;
	    continue;
	}

	if ((*it)->isWriteable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Play);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                ;
#endif
	    } else {
		addInstrumentsForDevice(device);
		m_devices.push_back(device);
	    }
	}
	if ((*it)->isReadable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Record);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create record device" << std::endl;
#else
                ;
#endif
	    } else {
		m_devices.push_back(device);
	    }
	}
    }

#ifdef HAVE_DSSI
    // Create a number of soft synth Instruments
    //
    {
    MappedInstrument *instr;
    char number[100];
    InstrumentId first;
    int count;
    getSoftSynthInstrumentNumbers(first, count);

    DeviceId ssiDeviceId = getSpareDeviceId();

    if (m_driverStatus & AUDIO_OK)
    {
        for (int i = 0; i < count; ++i)
        {
            sprintf(number, " #%d", i + 1);
            std::string name = "Synth plugin" + std::string(number);
            instr = new MappedInstrument(Instrument::SoftSynth,
                                         i,
                                         first + i,
                                         name,
                                         ssiDeviceId);
            m_instruments.push_back(instr);

            m_studio->createObject(MappedObject::AudioFader,
                                   first + i);
        }

        MappedDevice *device =
                        new MappedDevice(ssiDeviceId,
                                         Device::SoftSynth,
                                         "Synth plugin",
                                         "Soft synth connection");
        m_devices.push_back(device);
    }
    }
#endif

#ifdef HAVE_LIBJACK

    // Create a number of audio Instruments - these are just
    // logical Instruments anyway and so we can create as 
    // many as we like and then use them as Tracks.
    //
    {
    MappedInstrument *instr;
    char number[100];
    std::string audioName;

    DeviceId audioDeviceId = getSpareDeviceId();

    if (m_driverStatus & AUDIO_OK)
    {
        for (int channel = 0; channel < audioCount; ++channel)
        {
            sprintf(number, " #%d", channel + 1);
            audioName = "Audio" + std::string(number);
            instr = new MappedInstrument(Instrument::Audio,
                                         channel,
                                         m_audioRunningId,
                                         audioName,
                                         audioDeviceId);
            m_instruments.push_back(instr);

            // Create a fader with a matching id - this is the starting
            // point for all audio faders.
            //
            m_studio->createObject(MappedObject::AudioFader,
                                   m_audioRunningId);

            /*
            std::cerr  << "AlsaDriver::generateInstruments - "
                       << "added audio fader (id=" << m_audioRunningId
                       << ")" << std::endl;
                       */
    
            m_audioRunningId++;
        }

        // Create audio device
        //
        MappedDevice *device =
                        new MappedDevice(audioDeviceId,
                                         Device::Audio,
                                         "Audio",
                                         "Audio connection");
        m_devices.push_back(device);
    }
    }
#endif

}

MappedDevice *
AlsaDriver::createMidiDevice(AlsaPortDescription *port,
			     MidiDevice::DeviceDirection reqDirection)
{
    char deviceName[100];
    std::string connectionName("");
    Audit audit;

    static int unknownCounter;

    static int counters[3][2]; // [system/hardware/software][out/in]
    const int SYSTEM = 0, HARDWARE = 1, SOFTWARE = 2;
    static const char *firstNames[4][2] = {
	{ "MIDI output system device", "MIDI input system device" },
	{ "MIDI external device", "MIDI hardware input device" },
	{ "MIDI software device", "MIDI software input" }
    };
    static const char *countedNames[4][2] = {
	{ "MIDI output system device %d", "MIDI input system device %d" },
	{ "MIDI external device %d", "MIDI hardware input device %d" },
	{ "MIDI software device %d", "MIDI software input %d" }
    };

    static int specificCounters[2];
    static const char *specificNames[2] = {
	"MIDI soundcard synth", "MIDI soft synth",
    };
    static const char *specificCountedNames[2] = {
	"MIDI soundcard synth %d", "MIDI soft synth %d",
    };

    DeviceId deviceId = getSpareDeviceId();

    if (port) {

	if (reqDirection == MidiDevice::Record && !port->isReadable())  return 0;
	if (reqDirection == MidiDevice::Play   && !port->isWriteable()) return 0;

	int category = (port->m_client <  64 ? SYSTEM :
			port->m_client < 128 ? HARDWARE : SOFTWARE);

	bool haveName = false;

	if (category != SYSTEM && reqDirection == MidiDevice::Play) {

	    // No way to query whether a port is a MIDI synth, as
	    // PORT_TYPE_SYNTH actually indicates something different
	    // (ability to do direct wavetable synthesis -- nothing
	    // to do with MIDI).  But we assume GM/GS/XG/MT32 devices
	    // are synths.

	    bool isSynth = (port->m_portType &
			    (SND_SEQ_PORT_TYPE_MIDI_GM |
			     SND_SEQ_PORT_TYPE_MIDI_GS |
			     SND_SEQ_PORT_TYPE_MIDI_XG |
			     SND_SEQ_PORT_TYPE_MIDI_MT32));

	    // Because we can't discover through the API whether a
	    // port is a synth, we are instead reduced to this
	    // disgusting hack.  (At least we should make this
	    // configurable!)

	    if (!isSynth &&
		(port->m_name.find("ynth") < port->m_name.length())) isSynth = true;
	    if (!isSynth &&
		(port->m_name.find("nstrument") < port->m_name.length())) isSynth = true;
	    if (!isSynth &&
		(port->m_name.find("VSTi") < port->m_name.length())) isSynth = true;
	    
	    if (category == SYSTEM) isSynth = false;

	    if (isSynth) {
		int clientType = (category == SOFTWARE) ? 1 : 0;
		if (specificCounters[clientType] == 0) {
		    sprintf(deviceName, specificNames[clientType]);
		    ++specificCounters[clientType];
		} else {
		    sprintf(deviceName,
			    specificCountedNames[clientType],
			    ++specificCounters[clientType]);
		}
		haveName = true;
	    }
	}

	if (!haveName) {

	    if (counters[category][reqDirection] == 0) {
		sprintf(deviceName, firstNames[category][reqDirection]);
		++counters[category][reqDirection];
	    } else {
		sprintf(deviceName,
			countedNames[category][reqDirection],
			++counters[category][reqDirection]);
	    }
	}

	m_devicePortMap[deviceId] = ClientPortPair(port->m_client,
						   port->m_port);

	connectionName = port->m_name;

	audit << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode for connection " << connectionName
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;

    } else {

	sprintf(deviceName, "Anonymous MIDI device %d", ++unknownCounter);

	audit << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode -- no connection available "
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;
    }

    if (reqDirection == MidiDevice::Play) {

	QString portName;

	if (QString(deviceName).startsWith("Anonymous MIDI device ")) {
	    portName = QString("out %1")
		.arg(m_outputPorts.size() + 1);
	} else {
	    portName = QString("out %1 - %2")
		.arg(m_outputPorts.size() + 1)
		.arg(deviceName);
	}

	int outputPort = checkAlsaError(snd_seq_create_simple_port
					(m_midiHandle,
					 portName,
					 SND_SEQ_PORT_CAP_READ |
					 SND_SEQ_PORT_CAP_SUBS_READ,
					 SND_SEQ_PORT_TYPE_APPLICATION), 
					"createMidiDevice - can't create output port");

	if (outputPort >= 0) {

	    std::cerr << "CREATED OUTPUT PORT " << outputPort << ":" << portName << " for device " << deviceId << std::endl;

	    m_outputPorts[deviceId] = outputPort;
	
	    if (port) {
		std::cerr << "Connecting my port " << outputPort << " to " << port->m_client << ":" << port->m_port << " on initialisation" << std::endl;
		snd_seq_connect_to(m_midiHandle,
				   outputPort,
				   port->m_client,
				   port->m_port);
		if (m_midiSyncAutoConnect) {
		    snd_seq_connect_to(m_midiHandle,
				       m_syncOutputPort,
				       port->m_client,
				       port->m_port);
		}
		std::cerr << "done" << std::endl;
	    }
	}
    }

    MappedDevice *device = new MappedDevice(deviceId,
					    Device::Midi,
					    deviceName,
					    connectionName);
    device->setDirection(reqDirection);
    return device;
}

DeviceId
AlsaDriver::getSpareDeviceId()
{
    std::set<DeviceId> ids;
    for (unsigned int i = 0; i < m_devices.size(); ++i) {
	ids.insert(m_devices[i]->getId());
    }

    DeviceId id = 0;
    while (ids.find(id) != ids.end()) ++id;
    return id;
}

void
AlsaDriver::addInstrumentsForDevice(MappedDevice *device)
{
    std::string channelName;
    char number[100];

    for (int channel = 0; channel < 16; ++channel)
    {
	// Create MappedInstrument for export to GUI
	//
	// name is just number, derive rest from device at gui
	sprintf(number, "#%d", channel + 1);
	channelName = std::string(number);
	
	if (channel == 9) channelName = std::string("#10[D]");
	MappedInstrument *instr = new MappedInstrument(Instrument::Midi,
						       channel,
						       m_midiRunningId++,
						       channelName,
						       device->getId());
	m_instruments.push_back(instr);
    }
}
    

bool
AlsaDriver::canReconnect(Device::DeviceType type)
{
    return (type == Device::Midi);
}

DeviceId
AlsaDriver::addDevice(Device::DeviceType type,
		      MidiDevice::DeviceDirection direction)
{
    if (type == Device::Midi) {

	MappedDevice *device = createMidiDevice(0, direction);
	if (!device) {
#ifdef DEBUG_ALSA
	    std::cerr << "WARNING: Device creation failed" << std::endl;
#else
            ;
#endif
	} else {
	    addInstrumentsForDevice(device);
	    m_devices.push_back(device);

	    MappedEvent *mE =
		new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				0, 0);
	    insertMappedEventForReturn(mE);

	    return device->getId();
	}
    }

    return Device::NO_DEVICE;
}

void
AlsaDriver::removeDevice(DeviceId id)
{
    DeviceIntMap::iterator i1 = m_outputPorts.find(id);
    if (i1 == m_outputPorts.end()) {
	std::cerr << "WARNING: AlsaDriver::removeDevice: Cannot find device "
		  << id << " in port map" << std::endl;
	return;
    }
    checkAlsaError( snd_seq_delete_port(m_midiHandle, i1->second),
		    "removeDevice");
    m_outputPorts.erase(i1);
	
    for (MappedDeviceList::iterator i = m_devices.end();
	 i != m_devices.begin(); ) {
	
	--i;

	if ((*i)->getId() == id) {
	    delete *i;
	    m_devices.erase(i);
	}
    }

    for (MappedInstrumentList::iterator i = m_instruments.end();
	 i != m_instruments.begin(); ) {

	--i;
	
	if ((*i)->getDevice() == id) {
	    delete *i;
	    m_instruments.erase(i);
	}
    }

    MappedEvent *mE =
	new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			0, 0);
    insertMappedEventForReturn(mE);
}

void
AlsaDriver::renameDevice(DeviceId id, QString name)
{
    DeviceIntMap::iterator i = m_outputPorts.find(id);
    if (i == m_outputPorts.end()) {
	std::cerr << "WARNING: AlsaDriver::renameDevice: Cannot find device "
		  << id << " in port map" << std::endl;
	return;
    }

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_port_info(m_midiHandle, i->second, pinfo);

    QString oldName = snd_seq_port_info_get_name(pinfo);
    int sep = oldName.find(" - ");

    QString newName;
    
    if (name.startsWith("Anonymous MIDI device ")) {
	if (sep < 0) sep = 0;
	newName = oldName.left(sep);
    } else if (sep < 0) {
	newName = oldName + " - " + name;
    } else {
	newName = oldName.left(sep + 3) + name;
    }	

    snd_seq_port_info_set_name(pinfo, newName.data());
    checkAlsaError(snd_seq_set_port_info(m_midiHandle, i->second, pinfo),
		   "renameDevice");

    for (unsigned int i = 0; i < m_devices.size(); ++i) {
	if (m_devices[i]->getId() == id) {
	    m_devices[i]->setName(newName.data());
	    break;
	}
    }

    std::cerr << "Renamed " << m_client << ":" << i->second << " to " << name << std::endl;
}

ClientPortPair
AlsaDriver::getPortByName(std::string name)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_name == name) {
	    return ClientPortPair(m_alsaPorts[i]->m_client,
				  m_alsaPorts[i]->m_port);
	}
    }
    return ClientPortPair(-1, -1);
}

std::string
AlsaDriver::getPortName(ClientPortPair port)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_client == port.first &&
	    m_alsaPorts[i]->m_port == port.second) {
	    return m_alsaPorts[i]->m_name;
	}
    }
    return "";
}
    

unsigned int
AlsaDriver::getConnections(Device::DeviceType type,
			   MidiDevice::DeviceDirection direction)
{
    if (type != Device::Midi) return 0;

    int count = 0;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    ++count;
	}
    }
    
    return count;
}

QString
AlsaDriver::getConnection(Device::DeviceType type,
			  MidiDevice::DeviceDirection direction,
			  unsigned int connectionNo)
{
    if (type != Device::Midi) return "";
    
    AlsaPortList tempList;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    tempList.push_back(m_alsaPorts[j]);
	}
    }
    
    if (connectionNo < tempList.size()) {
	return tempList[connectionNo]->m_name.c_str();
    }

    return "";
}

void
AlsaDriver::setConnectionToDevice(MappedDevice &device, QString connection)
{
    ClientPortPair pair(-1,-1);
    if (connection && connection != "") {
	pair = getPortByName(connection.data());
    }
    setConnectionToDevice(device, connection, pair);
}

void
AlsaDriver::setConnectionToDevice(MappedDevice &device, QString connection,
				  const ClientPortPair &pair)
{
    QString prevConnection = device.getConnection();
    device.setConnection(connection.data());
    
    if (device.getDirection() == Rosegarden::MidiDevice::Play) {

	DeviceIntMap::iterator j = m_outputPorts.find(device.getId());

	if (j != m_outputPorts.end()) {

	    if (prevConnection != "") {
		ClientPortPair prevPair = getPortByName(prevConnection);
		if (prevPair.first >= 0 && prevPair.second >= 0) {

	std::cerr << "Disconnecting my port " << j->second << " from " << prevPair.first << ":" << prevPair.second << " on reconnection" << std::endl;
		    snd_seq_disconnect_to(m_midiHandle,
					  j->second,
					  prevPair.first,
					  prevPair.second);

		    if (m_midiSyncAutoConnect) {
			bool foundElsewhere = false;
			for (MappedDeviceList::iterator k = m_devices.begin();
			     k != m_devices.end(); ++k) {
			    if ((*k)->getId() != device.getId()) {
				if ((*k)->getConnection() == prevConnection) {
				    foundElsewhere = true;
				    break;
				}
			    }
			}
			if (!foundElsewhere) {
			    snd_seq_disconnect_to(m_midiHandle,
						  m_syncOutputPort,
						  pair.first,
						  pair.second);
			}			
		    }
		}
	    }

	    if (pair.first >= 0 && pair.second >= 0) {
	std::cerr << "Connecting my port " << j->second << " to " << pair.first << ":" << pair.second << " on reconnection" << std::endl;
		snd_seq_connect_to(m_midiHandle,
				   j->second,
				   pair.first,
				   pair.second);
		if (m_midiSyncAutoConnect) {
		    snd_seq_connect_to(m_midiHandle,
				       m_syncOutputPort,
				       pair.first,
				       pair.second);
		}
	    }
	}
    }
}

void
AlsaDriver::setConnection(DeviceId id, QString connection)
{
    Audit audit;
    ClientPortPair port(getPortByName(connection.data()));

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		setConnectionToDevice(*m_devices[i], connection, port);

		MappedEvent *mE =
		    new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				    0, 0);
		insertMappedEventForReturn(mE);

		break;
	    }
	}
    }
}

void
AlsaDriver::setPlausibleConnection(DeviceId id, QString idealConnection)
{
    Audit audit;
    ClientPortPair port(getPortByName(idealConnection.data()));

    audit << "AlsaDriver::setPlausibleConnection: connection like "
		 << idealConnection << " requested for device " << id << std::endl;

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		setConnectionToDevice(*m_devices[i], idealConnection, port);
		break;
	    }
	}

	audit << "AlsaDriver::setPlausibleConnection: exact match available"
		     << std::endl;
	return;
    }

    // What we want is a connection that:
    // 
    //  * is in the right "class" (the 0-63/64-127/128+ range of client id)
    //  * has at least some text in common
    //  * is not yet in use for any device.
    // 
    // To do this, we exploit our privileged position as part of AlsaDriver
    // and use our knowledge of how connection strings are made (see
    // AlsaDriver::generatePortList above) to pick out the relevant parts
    // of the requested string.
    
    int client = 0;
    int colon = idealConnection.find(":");
    if (colon >= 0) client = idealConnection.left(colon).toInt();

    int firstSpace = idealConnection.find(" ");
    int endOfText  = idealConnection.find(QRegExp("[^\\w ]"), firstSpace);

    QString text;
    if (endOfText < 2) {
	text = idealConnection.mid(firstSpace + 1);
    } else {
	text = idealConnection.mid(firstSpace + 1, endOfText - firstSpace - 2);
    }

    for (int testName = 1; testName >= 0; --testName) {

	for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {

	    AlsaPortDescription *port = m_alsaPorts[i];

	    if (client > 0 && (port->m_client / 64 != client / 64)) continue;
	    
	    if (testName && text != "" &&
		!QString(port->m_name.c_str()).contains(text)) continue;
	    
	    bool used = false;
	    for (DevicePortMap::iterator dpmi = m_devicePortMap.begin();
		 dpmi != m_devicePortMap.end(); ++dpmi) {
		if (dpmi->second.first  == port->m_client &&
		    dpmi->second.second == port->m_port) {
		    used = true;
		    break;
		}
	    }
	    if (used) continue;

	    // OK, this one will do

	    audit << "AlsaDriver::setPlausibleConnection: fuzzy match "
			 << port->m_name << " available" << std::endl;

	    m_devicePortMap[id] = ClientPortPair(port->m_client, port->m_port);

	    for (unsigned int i = 0; i < m_devices.size(); ++i) {

		if (m_devices[i]->getId() == id) {
		    setConnectionToDevice(*m_devices[i], port->m_name, m_devicePortMap[id]);
		    
		    // in this case we don't request a device resync,
		    // because this is only invoked at times such as
		    // file load when the GUI is well aware that the
		    // whole situation is in upheaval anyway
		    
		    return;
		}
	    }
	}
    }

    audit << "AlsaDriver::setPlausibleConnection: nothing suitable available"
		 << std::endl;
}


void
AlsaDriver::checkTimerSync(size_t frames)
{
    if (!m_doTimerChecks) return;

#ifdef HAVE_LIBJACK
    if (!m_jackDriver || !m_queueRunning || frames == 0 ||
	(getMTCStatus() == TRANSPORT_SLAVE)) {
	m_firstTimerCheck = true;
	return;
    }

    static RealTime startAlsaTime;
    static size_t startJackFrames = 0;
    static size_t lastJackFrames = 0;

    size_t nowJackFrames = m_jackDriver->getFramesProcessed();
    RealTime nowAlsaTime = getAlsaTime();
    
    if (m_firstTimerCheck ||
	(nowJackFrames <= lastJackFrames) ||
	(nowAlsaTime <= startAlsaTime)) {

	startAlsaTime = nowAlsaTime;
	startJackFrames = nowJackFrames;
	lastJackFrames = nowJackFrames;

	m_firstTimerCheck = false;
	return;
    }

    RealTime jackDiff = RealTime::frame2RealTime
	(nowJackFrames - startJackFrames,
	 m_jackDriver->getSampleRate());
    
    RealTime alsaDiff = nowAlsaTime - startAlsaTime;

    if (alsaDiff > RealTime(10, 0)) {

#ifdef DEBUG_ALSA
        if (!m_playing) {
	    std::cout << "\nALSA:" << startAlsaTime << "\t->" << nowAlsaTime << "\nJACK: " << startJackFrames << "\t\t-> " << nowJackFrames << std::endl;
	    std::cout << "ALSA diff:  " << alsaDiff << "\nJACK diff:  " << jackDiff << std::endl;
	}
#endif

	double ratio = (jackDiff - alsaDiff) / alsaDiff;

	if (fabs(ratio) > 0.1) {
#ifdef DEBUG_ALSA
	    if (!m_playing) {
		std::cout << "Ignoring excessive ratio " << ratio
			  << ", hoping for a more likely result next time"
			  << std::endl;
	    }
#endif
	} else if (fabs(ratio) > 0.000001) {

#ifdef DEBUG_ALSA
	    if (alsaDiff > RealTime::zeroTime && jackDiff > RealTime::zeroTime) {
	        if (!m_playing) {
		    if (jackDiff < alsaDiff) {
  		        std::cout << "<<<< ALSA timer is faster by " << 100.0 * ((alsaDiff - jackDiff) / alsaDiff) << "% (1/" << int(1.0/ratio) << ")" << std::endl;
		    } else {
		        std::cout << ">>>> JACK timer is faster by " << 100.0 * ((jackDiff - alsaDiff) / alsaDiff) << "% (1/" << int(1.0/ratio) << ")" << std::endl;
		    }
		}
	    }
#endif

	    m_timerRatio = ratio;
	    m_timerRatioCalculated = true;
	}

	m_firstTimerCheck = true;
    }
#endif
}
    

unsigned int
AlsaDriver::getTimers()
{
    return m_timers.size() + 1; // one extra for auto
}

QString
AlsaDriver::getTimer(unsigned int n)
{
    if (n == 0) return AUTO_TIMER_NAME;
    else return m_timers[n-1].name.c_str();
}

QString
AlsaDriver::getCurrentTimer()
{
    return m_currentTimer.c_str();
}

void
AlsaDriver::setCurrentTimer(QString timer)
{
    Audit audit;

    if (timer == getCurrentTimer()) return;

    std::cerr << "AlsaDriver::setCurrentTimer(" << timer << ")" << std::endl;

    std::string name(timer.data());

    if (name == AUTO_TIMER_NAME) {
	name = getAutoTimer(m_doTimerChecks);
    } else {
        m_doTimerChecks = false;
    }
    m_timerRatioCalculated = false;
   
    // Stop and restart the queue around the timer change.  We don't
    // call stopClocks/startClocks here because they do the wrong
    // thing if we're currently playing and on the JACK transport.

    m_queueRunning = false;
    checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, NULL), "setCurrentTimer(): stopping queue");
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "setCurrentTimer(): draining output to stop queue");
    
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_real_time_t z = { 0, 0 };
    snd_seq_ev_set_queue_pos_real(&event, m_queue, &z);
    snd_seq_ev_set_direct(&event);
    checkAlsaError(snd_seq_control_queue(m_midiHandle, m_queue, SND_SEQ_EVENT_SETPOS_TIME,
					  0, &event), "setCurrentTimer(): control queue");
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "setCurrentTimer(): draining output to control queue");
    m_alsaPlayStartTime = RealTime::zeroTime;

    for (unsigned int i = 0; i < m_timers.size(); ++i) {
	if (m_timers[i].name == name) {

	    snd_seq_queue_timer_t *timer;
	    snd_timer_id_t *timerid;

	    snd_seq_queue_timer_alloca(&timer);
	    snd_seq_get_queue_timer(m_midiHandle, m_queue, timer);

	    snd_timer_id_alloca(&timerid);
	    snd_timer_id_set_class(timerid, m_timers[i].clas);
	    snd_timer_id_set_sclass(timerid, m_timers[i].sclas);
	    snd_timer_id_set_card(timerid, m_timers[i].card);
	    snd_timer_id_set_device(timerid, m_timers[i].device);
	    snd_timer_id_set_subdevice(timerid, m_timers[i].subdevice);

	    snd_seq_queue_timer_set_id(timer, timerid);
	    snd_seq_set_queue_timer(m_midiHandle, m_queue, timer);

	    if (m_doTimerChecks) {
		audit << "    Current timer set to \"" << name << "\" with timer checks"
		      << std::endl;
	    } else {
		audit << "    Current timer set to \"" << name << "\""
		      << std::endl;
	    }		

	    if (m_timers[i].clas == SND_TIMER_CLASS_GLOBAL &&
		m_timers[i].device == SND_TIMER_GLOBAL_SYSTEM) {
		long hz = 1000000000 / m_timers[i].resolution;
		if (hz < 900) {
		    audit << "    WARNING: using system timer with only "
			  << hz << "Hz resolution!" << std::endl;
		}
	    }

	    break;
	}
    }
    
#ifdef HAVE_LIBJACK
    if (m_jackDriver) m_jackDriver->prebufferAudio();
#endif

    checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, NULL), "checkAlsaError(): continue queue");
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "setCurrentTimer(): draining output to continue queue");
    m_queueRunning = true;

    m_firstTimerCheck = true;
}

void
AlsaDriver::initialise()
{
    initialiseAudio();
    initialiseMidi();
}



// Set up queue, client and port
//
void
AlsaDriver::initialiseMidi()
{ 
    Audit audit;

    // Create a non-blocking handle.
    //
    if (snd_seq_open(&m_midiHandle,
                     "default",
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0) {
        audit << "AlsaDriver::initialiseMidi - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
	return;
    }

    snd_seq_set_client_name(m_midiHandle, "rosegarden");

    if ((m_client = snd_seq_client_id(m_midiHandle)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't create client"
                  << std::endl;
#endif
        return;
    }

    // Create a queue
    //
    if ((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
					     "Rosegarden queue")) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't allocate queue"
                  << std::endl;
#endif
        return;
    }

    // Create the input port
    //
    snd_seq_port_info_t *pinfo;

    snd_seq_port_info_alloca(&pinfo);
    snd_seq_port_info_set_capability(pinfo,
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE );
    snd_seq_port_info_set_type(pinfo, SND_SEQ_PORT_TYPE_APPLICATION);
    snd_seq_port_info_set_midi_channels(pinfo, 16);
    /* we want to know when the events got delivered to us */
    snd_seq_port_info_set_timestamping(pinfo, 1);
    snd_seq_port_info_set_timestamp_real(pinfo, 1);
    snd_seq_port_info_set_timestamp_queue(pinfo, m_queue);
    snd_seq_port_info_set_name(pinfo, "record in");

    if (checkAlsaError(snd_seq_create_port(m_midiHandle, pinfo), 
                       "initialiseMidi - can't create input port") < 0)
        return;
    m_inputPort = snd_seq_port_info_get_port(pinfo);

    // Subscribe the input port to the ALSA Announce port
    // to receive notifications when clients, ports and subscriptions change
    snd_seq_connect_from( m_midiHandle, m_inputPort, 
			  SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE );
    
    m_midiInputPortConnected = true;

    // Set the input queue size
    //
    if (snd_seq_set_client_pool_output(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_input(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_output_room(m_midiHandle, 2000) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't modify pool parameters"
                  << std::endl;
#endif
        return;
    }

    // Create sync output now as well
    m_syncOutputPort = checkAlsaError(snd_seq_create_simple_port
				      (m_midiHandle,
				       "sync out",
				       SND_SEQ_PORT_CAP_READ |
				       SND_SEQ_PORT_CAP_SUBS_READ,
				       SND_SEQ_PORT_TYPE_APPLICATION), 
				      "initialiseMidi - can't create sync output port");

    // and port for hardware controller
    m_controllerPort = checkAlsaError(snd_seq_create_simple_port
				      (m_midiHandle,
				       "external controller",
				       SND_SEQ_PORT_CAP_READ |
				       SND_SEQ_PORT_CAP_WRITE |
				       SND_SEQ_PORT_CAP_SUBS_READ |
				       SND_SEQ_PORT_CAP_SUBS_WRITE,
				       SND_SEQ_PORT_TYPE_APPLICATION), 
				      "initialiseMidi - can't create controller port");

    getSystemInfo();

    generatePortList();
    generateInstruments();

    // Modify status with MIDI success
    //
    m_driverStatus |= MIDI_OK;

    generateTimerList();
    setCurrentTimer(AUTO_TIMER_NAME);

    // Start the timer
    if (checkAlsaError(snd_seq_start_queue(m_midiHandle, m_queue, NULL), 
                       "initialiseMidi(): couldn't start queue") < 0) {
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
	return;
    }

    m_queueRunning = true;

    // process anything pending
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "initialiseMidi(): couldn't drain output");

    audit << "AlsaDriver::initialiseMidi -  initialised MIDI subsystem"
              << std::endl << std::endl;
}

// We don't even attempt to use ALSA audio.  We just use JACK instead.
// See comment at the top of this file and jackProcess() for further
// information on how we use this.
//
void
AlsaDriver::initialiseAudio()
{
#ifdef HAVE_LIBJACK
    m_jackDriver = new JackDriver(this);

    if (m_jackDriver->isOK()) {
	m_driverStatus |= AUDIO_OK;
    } else {
	delete m_jackDriver;
	m_jackDriver = 0;
    }
#endif
}

void
AlsaDriver::initialisePlayback(const RealTime &position)
{
#ifdef DEBUG_ALSA
    std::cerr << "\n\nAlsaDriver - initialisePlayback" << std::endl;
#endif

    // now that we restart the queue at each play, the origin is always zero
    m_alsaPlayStartTime = RealTime::zeroTime;
    m_playStartPosition = position;

    m_startPlayback = true;

    m_mtcFirstTime = -1;
    m_mtcSigmaE = 0;
    m_mtcSigmaC = 0;

    if (getMMCStatus() == TRANSPORT_MASTER) {
        sendMMC(127, MIDI_MMC_PLAY, true, "");
    }

    if (getMTCStatus() == TRANSPORT_MASTER) {
	insertMTCFullFrame(position);
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	m_needJackStart = NeedJackStart;
    }
#endif
}


void
AlsaDriver::stopPlayback()
{
#ifdef DEBUG_ALSA
    std::cerr << "\n\nAlsaDriver - stopPlayback" << std::endl;
#endif

    if (getMMCStatus() == TRANSPORT_MASTER)
    {
        sendMMC(127, MIDI_MMC_STOP, true, "");
    }

    allNotesOff();
    m_playing = false;

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	m_jackDriver->stopTransport();
	m_needJackStart = NeedNoJackStart;
    }
#endif
    
    // Flush the output and input queues
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_INPUT|
                                              SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    // Send system stop to duplex MIDI devices
    //
    if (m_midiClockEnabled) sendSystemDirect(SND_SEQ_EVENT_STOP, "");

    // send sounds-off to all play devices
    //
    for (MappedDeviceList::iterator i = m_devices.begin(); i != m_devices.end(); ++i) {
	if ((*i)->getDirection() == Rosegarden::MidiDevice::Play) {
	    sendDeviceController((*i)->getId(),
				 MIDI_CONTROLLER_SUSTAIN, 0);
	    sendDeviceController((*i)->getId(),
				 MIDI_CONTROLLER_ALL_NOTES_OFF, 0);
	}
    }

    punchOut();

    stopClocks(); // Resets ALSA timer to zero

    clearAudioQueue();

    startClocksApproved(); // restarts ALSA timer without starting JACK transport
}

void
AlsaDriver::punchOut()
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::punchOut" << std::endl;
#endif

#ifdef HAVE_LIBJACK
    // Close any recording file
    if (m_recordStatus == RECORD_ON)
    {
	for (InstrumentSet::const_iterator i = m_recordingInstruments.begin();
	     i != m_recordingInstruments.end(); ++i) {

	    Rosegarden::InstrumentId id = *i;

	    if (id >= Rosegarden::AudioInstrumentBase &&
		id <  Rosegarden::MidiInstrumentBase) {
		
		AudioFileId auid = 0;
		if (m_jackDriver && m_jackDriver->closeRecordFile(id, auid)) {

#ifdef DEBUG_ALSA
		    std::cerr << "AlsaDriver::stopPlayback: sending back to GUI for instrument " << id << std::endl;
#endif

		    // Create event to return to gui to say that we've
		    // completed an audio file and we can generate a
		    // preview for it now.
		    //
		    // nasty hack -- don't have right audio id here, and
		    // the sequencer will wipe out the instrument id and
		    // replace it with currently-selected one in gui --
		    // so use audio id slot to pass back instrument id
		    // and handle accordingly in gui
		    try
		    {
			MappedEvent *mE =
			    new MappedEvent(id,
					    MappedEvent::AudioGeneratePreview,
					    id % 256,
					    id / 256);
			
			// send completion event
			insertMappedEventForReturn(mE);
		    }
		    catch(...) {;}
		}
	    }
	}
    }
#endif

    // Change recorded state if any set
    //
    if (m_recordStatus == RECORD_ON) m_recordStatus = RECORD_OFF;

    m_recordingInstruments.clear();
}

void
AlsaDriver::resetPlayback(const RealTime &oldPosition, const RealTime &position)
{
#ifdef DEBUG_ALSA
    std::cerr << "\n\nAlsaDriver - resetPlayback(" << position << ")" << std::endl;
#endif

    m_playStartPosition = position;
    m_alsaPlayStartTime = getAlsaTime();

    // Reset note offs to correct positions
    //
    RealTime jump = position - oldPosition;

    // modify the note offs that exist as they're relative to the
    // playStartPosition terms.
    //
    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                                i != m_noteOffQueue.end(); ++i)
    {
        // if we're fast forwarding then we bring the note off closer
	if (jump >= RealTime::zeroTime)
        {
            (*i)->setRealTime((*i)->getRealTime() - jump /* + modifyNoteOff */);
        }
        else // we're rewinding - kill the note immediately
        {
            (*i)->setRealTime(m_playStartPosition);
        }
    }

    // Ensure we clear down output queue on reset - in the case of
    // MIDI clock where we might have a long queue of events already
    // posted.
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);
    //snd_seq_remove_events_set_event_type(info, 
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    if (getMTCStatus() == TRANSPORT_MASTER) {
	m_mtcFirstTime = -1;
	m_mtcSigmaE = 0;
	m_mtcSigmaC = 0;
	insertMTCFullFrame(position);
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	m_needJackStart = NeedJackReposition;
    }
#endif
}

void 
AlsaDriver::setMIDIClockInterval(RealTime interval)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::setMIDIClockInterval(" << interval << ")" << endl;
#endif

    // Reset the value
    //
    SoundDriver::setMIDIClockInterval(interval);

    // Return if the clock isn't enabled
    //
    if (!m_midiClockEnabled) return;

    if (false)  // don't remove any events quite yet
    {

    // Remove all queued events (although we should filter this
    // down to just the clock events.
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);

    //if (snd_seq_type_check(SND_SEQ_EVENT_CLOCK, SND_SEQ_EVFLG_CONTROL))
    //snd_seq_remove_events_set_event_type(info, 
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events_set_event_type(info, SND_SEQ_EVFLG_CONTROL);
    std::cout << "AlsaDriver::setMIDIClockInterval - "
              << "MIDI CLOCK TYPE IS CONTROL" << std::endl;
    snd_seq_remove_events(m_midiHandle, info);
    }

}




void
AlsaDriver::allNotesOff()
{
    snd_seq_event_t event;
    ClientPortPair outputDevice;
    RealTime offTime;

    // drop any pending notes
    snd_seq_drop_output_buffer(m_midiHandle);
    snd_seq_drop_output(m_midiHandle);

    // prepare the event
    snd_seq_ev_clear(&event);
    offTime = getAlsaTime();

    for (NoteOffQueue::iterator it = m_noteOffQueue.begin();
                                it != m_noteOffQueue.end(); ++it)
    {
        // Set destination according to connection for instrument
        //
	outputDevice = getPairForMappedInstrument((*it)->getInstrument());
	if (outputDevice.first < 0 || outputDevice.second < 0) continue;

	snd_seq_ev_set_subs(&event);

	// Set source according to port for device
	//
	int src = getOutputPortForMappedInstrument((*it)->getInstrument());
	if (src < 0) continue;
	snd_seq_ev_set_source(&event, src);

        snd_seq_ev_set_noteoff(&event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               127);

        //snd_seq_event_output(m_midiHandle, &event);
        int error = snd_seq_event_output_direct(m_midiHandle, &event);

        if (error < 0)
        {
#ifdef DEBUG_ALSA
	    std::cerr << "AlsaDriver::allNotesOff - "
                      << "can't send event" << std::endl;
#endif
        }

        delete(*it);
    }
    
    m_noteOffQueue.erase(m_noteOffQueue.begin(), m_noteOffQueue.end());

    /*
    std::cerr << "AlsaDriver::allNotesOff - "
              << " queue size = " << m_noteOffQueue.size() << std::endl;
              */

    // flush
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "allNotesOff(): draining");
}

void
AlsaDriver::processNotesOff(const RealTime &time, bool now)
{
    if (m_noteOffQueue.empty()) return;

    snd_seq_event_t event;

    ClientPortPair outputDevice;
    RealTime offTime;

    // prepare the event
    snd_seq_ev_clear(&event);

#ifdef DEBUG_PROCESS_MIDI_OUT
    std::cerr << "AlsaDriver::processNotesOff(" << time << ")" << std::endl;
#endif

    while (m_noteOffQueue.begin() != m_noteOffQueue.end() &&
	   (*m_noteOffQueue.begin())->getRealTime() < time) {

	NoteOffEvent *ev = *m_noteOffQueue.begin();

#ifdef DEBUG_PROCESS_MIDI_OUT
	std::cerr << "AlsaDriver::processNotesOff(" << time << "): found event at " << ev->getRealTime() << ", instr " << ev->getInstrument() << ", channel " << int(ev->getChannel()) << ", pitch " << int(ev->getPitch()) << std::endl;
#endif

	bool isSoftSynth = (ev->getInstrument() >= SoftSynthInstrumentBase);

        offTime = ev->getRealTime();

        snd_seq_real_time_t alsaOffTime = { offTime.sec,
                                            offTime.nsec };

	if (!isSoftSynth) {

	    snd_seq_ev_set_subs(&event);

	    // Set source according to instrument
	    //
	    int src = getOutputPortForMappedInstrument(ev->getInstrument());
	    if (src < 0) {
		delete ev;
		m_noteOffQueue.erase(m_noteOffQueue.begin());
		continue;
	    }

	    snd_seq_ev_set_source(&event, src);
	    snd_seq_ev_schedule_real(&event, m_queue, 0, &alsaOffTime);

	} else {
	    
	    event.time.time = alsaOffTime;
	}

        snd_seq_ev_set_noteoff(&event,
                               ev->getChannel(),
                               ev->getPitch(),
                               127);

	// send note off
	if (isSoftSynth) {
	    processSoftSynthEventOut(ev->getInstrument(), &event, now);
	} else {
	    snd_seq_event_output(m_midiHandle, &event);
	}

	delete ev;
        m_noteOffQueue.erase(m_noteOffQueue.begin());
    }

    // We don't flush the queue here, as this is called nested from
    // processMidiOut, which does the flushing

#ifdef DEBUG_PROCESS_MIDI_OUT
    std::cerr << "AlsaDriver::processNotesOff - "
	      << " queue size now: " << m_noteOffQueue.size() << std::endl;
#endif
}

// Get the queue time and convert it to RealTime for the gui
// to use.
//
RealTime
AlsaDriver::getSequencerTime()
{
    RealTime t(0, 0);

    t = getAlsaTime() + m_playStartPosition - m_alsaPlayStartTime;

//    std::cerr << "AlsaDriver::getSequencerTime: alsa time is "
//	      << getAlsaTime() << ", start time is " << m_alsaPlayStartTime << ", play start position is " << m_playStartPosition << endl;

    return t;
}

// Gets the time of the ALSA queue
//
RealTime
AlsaDriver::getAlsaTime()
{
    RealTime sequencerTime(0, 0);

    snd_seq_queue_status_t *status;
    snd_seq_queue_status_alloca(&status);

    if (snd_seq_get_queue_status(m_midiHandle, m_queue, status) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::getAlsaTime - can't get queue status"
                  << std::endl;
#endif
        return sequencerTime;
    }

    sequencerTime.sec = snd_seq_queue_status_get_real_time(status)->tv_sec;
    sequencerTime.nsec = snd_seq_queue_status_get_real_time(status)->tv_nsec;

//    std::cerr << "AlsaDriver::getAlsaTime: alsa time is " << sequencerTime << std::endl;

    return sequencerTime;
}


// Get all pending input events and turn them into a MappedComposition.
//
//
MappedComposition*
AlsaDriver::getMappedComposition()
{
    m_recordComposition.clear();

    while (_failureReportReadIndex != _failureReportWriteIndex) {
	MappedEvent::FailureCode code = _failureReports[_failureReportReadIndex];
//	std::cerr << "AlsaDriver::reportFailure(" << code << ")" << std::endl;
	Rosegarden::MappedEvent *mE = new Rosegarden::MappedEvent
	    (0, Rosegarden::MappedEvent::SystemFailure, code, 0);
	m_returnComposition.insert(mE);
	_failureReportReadIndex =
	    (_failureReportReadIndex + 1) % FAILURE_REPORT_COUNT;
    }

    if (!m_returnComposition.empty()) {
	for (MappedComposition::iterator i = m_returnComposition.begin();
	     i != m_returnComposition.end(); ++i) {
	    m_recordComposition.insert(new MappedEvent(**i));
	}
	m_returnComposition.clear();
    }

    // If the input port hasn't connected we shouldn't poll it
    //
    if (m_midiInputPortConnected == false)
    {
        return &m_recordComposition;
    }

    RealTime eventTime(0, 0);

    snd_seq_event_t *event;

    while (snd_seq_event_input(m_midiHandle, &event) > 0)
    {
        unsigned int channel = (unsigned int)event->data.note.channel;
        unsigned int chanNoteKey = ( channel << 8 ) +
                                   (unsigned int) event->data.note.note;
                             
	bool fromController = false;
	if (event->dest.client == m_client &&
	    event->dest.port == m_controllerPort) {
#ifdef DEBUG_ALSA
	    std::cerr << "Received an external controller event" << std::endl;
#endif
	    fromController = true;
	} else {
#ifdef DEBUG_ALSA
	    std::cerr << "Received non-controller event (dest=" << (int)event->dest.client << ":" << (int)event->dest.port << ", controller is " << (int)m_client << ":" << (int)m_controllerPort << ")" << std::endl;
#endif
	}

	unsigned int deviceId = Rosegarden::Device::NO_DEVICE;

	if (fromController) {
	    deviceId = Rosegarden::Device::CONTROL_DEVICE;
	} else {
	    for (MappedDeviceList::iterator i = m_devices.begin();
		 i != m_devices.end(); ++i) 
	    {
		ClientPortPair pair(m_devicePortMap[(*i)->getId()]);
		if (((*i)->getDirection() == MidiDevice::Record) &&
		    ( pair.first == event->source.client ) &&
		    ( pair.second == event->source.port ))
		{
		    deviceId = (*i)->getId();
		    break;
		}
	    }
	}
	
        eventTime.sec = event->time.time.tv_sec;
        eventTime.nsec = event->time.time.tv_nsec;
        eventTime = eventTime - m_alsaRecordStartTime + m_playStartPosition;

        switch(event->type)
        {
            case SND_SEQ_EVENT_NOTE:
            case SND_SEQ_EVENT_NOTEON:
		if (fromController) continue;
                if (event->data.note.velocity > 0)
                {
		    MappedEvent *mE = new MappedEvent();
                    mE->setPitch(event->data.note.note);
                    mE->setVelocity(event->data.note.velocity);
                    mE->setEventTime(eventTime);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);

                    // Negative duration - we need to hear the NOTE ON
                    // so we must insert it now with a negative duration
                    // and pick and mix against the following NOTE OFF
                    // when we create the recorded segment.
                    //
                    mE->setDuration(RealTime(-1, 0));

                    // Create a copy of this when we insert the NOTE ON -
                    // keeping a copy alive on the m_noteOnMap.
                    //
                    // We shake out the two NOTE Ons after we've recorded
                    // them.
                    //
                    m_recordComposition.insert(new MappedEvent(mE));
                    m_noteOnMap[deviceId][chanNoteKey] = mE;

                    break;
                }

            case SND_SEQ_EVENT_NOTEOFF:
		if (fromController) continue;
                if (m_noteOnMap[deviceId][chanNoteKey] != 0)
                {
                    // Set duration correctly on the NOTE OFF
                    //
                    MappedEvent *mE = m_noteOnMap[deviceId][chanNoteKey];
                    RealTime duration = eventTime - mE->getEventTime();

                    if (duration < RealTime::zeroTime) break;

                    // Velocity 0 - NOTE OFF.  Set duration correctly
                    // for recovery later.
                    //
                    mE->setVelocity(0);
                    mE->setDuration(duration);

                    // force shut off of note
                    m_recordComposition.insert(mE);

                    // reset the reference
                    //
                    m_noteOnMap[deviceId][chanNoteKey] = 0;
                    
                }
                break;

            case SND_SEQ_EVENT_KEYPRESS:
                {
		    if (fromController) continue;

                    // Fix for 632964 by Pedro Lopez-Cabanillas (20030523)
                    //
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiKeyPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.note.note);
                    mE->setData2(event->data.note.velocity);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiController);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.param);
                    mE->setData2(event->data.control.value);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_PGMCHANGE:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiProgramChange);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);
                    m_recordComposition.insert(mE);

                }
                break;

            case SND_SEQ_EVENT_PITCHBEND:
                {
		    if (fromController) continue;

                    // Fix for 711889 by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value + 8192;
                    int d1 = (s >> 7) & 0x7f; // data1 = MSB
                    int d2 = s & 0x7f; // data2 = LSB
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiPitchBend);
                    mE->setEventTime(eventTime);
                    mE->setData1(d1);
                    mE->setData2(d2);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CHANPRESS:
                {
		    if (fromController) continue;

                    // Fixed by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value & 0x7f;
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiChannelPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(s);
                    mE->setRecordedChannel(channel);
                    mE->setRecordedDevice(deviceId);
                    m_recordComposition.insert(mE);
                }
               break;

            case SND_SEQ_EVENT_SYSEX:

		if (fromController) continue;

		if (!testForMTCSysex(event) &&
		    !testForMMCSysex(event)) {

                   // Bundle up the data into a block on the MappedEvent
                   //
                   std::string data;
                   char *ptr = (char*)(event->data.ext.ptr);
                   for (unsigned int i = 0; i < event->data.ext.len; ++i)
                       data += *(ptr++);

#ifdef DEBUG_ALSA
                   if ((MidiByte)(data[1]) == MIDI_SYSEX_RT)
                   {
                       std::cerr << "REALTIME SYSEX" << endl;
                       for (unsigned int ii = 0; ii < data.length(); ++ii) {
                           printf("B %d = %02x\n", ii, data[ii]);
                       }
                   }
#endif

                   MappedEvent *mE = new MappedEvent();
                   mE->setType(MappedEvent::MidiSystemMessage);
                   mE->setData1(Rosegarden::MIDI_SYSTEM_EXCLUSIVE);
                   mE->setRecordedDevice(deviceId);
                   // chop off SYX and EOX bytes from data block
                   // Fix for 674731 by Pedro Lopez-Cabanillas (20030601)
                   DataBlockRepository::setDataBlockForEvent(mE, data.substr(1, data.length() - 2));
                   mE->setEventTime(eventTime);
                   m_recordComposition.insert(mE);
               }
               break;


            case SND_SEQ_EVENT_SENSING: // MIDI device is still there
		break;

            case SND_SEQ_EVENT_QFRAME:
		if (fromController) continue;
		if (getMTCStatus() == TRANSPORT_SLAVE) {
		    handleMTCQFrame(event->data.control.value, eventTime);
		}
		break;

            case SND_SEQ_EVENT_CLOCK:
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got realtime MIDI clock" << std::endl;

               break;

            case SND_SEQ_EVENT_START:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "START" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_CONTINUE:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "CONTINUE" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_STOP:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "STOP" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_SONGPOS:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "SONG POSITION" << std::endl;
#endif
               break;

               // these cases are handled by checkForNewClients
               //
            case SND_SEQ_EVENT_CLIENT_START: 
            case SND_SEQ_EVENT_CLIENT_EXIT: 
            case SND_SEQ_EVENT_CLIENT_CHANGE: 
            case SND_SEQ_EVENT_PORT_START: 
            case SND_SEQ_EVENT_PORT_EXIT: 
            case SND_SEQ_EVENT_PORT_CHANGE: 
            case SND_SEQ_EVENT_PORT_SUBSCRIBED:
            case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		m_portCheckNeeded = true;
#ifdef DEBUG_ALSA		
		std::cerr << "AlsaDriver::getMappedComposition - "
			  << "got announce event (" 
			  << int(event->type) << ")" << std::endl;
#endif			  
		break;
            case SND_SEQ_EVENT_TICK:
            default:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got unhandled MIDI event type from ALSA sequencer"
                         << "(" << int(event->type) << ")" << std::endl;
#endif
               break;


        }
    }

    if (getMTCStatus() == TRANSPORT_SLAVE && isPlaying()) {
#ifdef MTC_DEBUG
	std::cerr << "seq time is " << getSequencerTime() << ", last MTC receive "
		  << m_mtcLastReceive << ", first time " << m_mtcFirstTime << std::endl;
#endif
	if (m_mtcFirstTime == 0) { // have received _some_ MTC quarter-frame info
	    RealTime seqTime = getSequencerTime();
	    if (m_mtcLastReceive < seqTime &&
		seqTime - m_mtcLastReceive > RealTime(0, 500000000L)) {
		ExternalTransport *transport = getExternalTransportControl();
		if (transport) {
		    transport->transportJump(ExternalTransport::TransportStopAtTime,
					     m_mtcLastEncoded);
		}
	    }
	}
    }

    return &m_recordComposition;
}

static int lock_count = 0;

void
AlsaDriver::handleMTCQFrame(unsigned int data_byte, RealTime the_time)
{
    if (getMTCStatus() != TRANSPORT_SLAVE) return;
    
    switch(data_byte & 0xF0)
    {
        /* Frame */
        case 0x00:
            /*
             * Reset everything
             */
	    m_mtcReceiveTime = the_time;
            m_mtcFrames = data_byte & 0x0f;
            m_mtcSeconds = 0;
            m_mtcMinutes = 0;
            m_mtcHours = 0;
            m_mtcSMPTEType = 0;

            break;
            
        case 0x10:
            m_mtcFrames |= (data_byte & 0x0f) << 4;
            break;
            
        /* Seconds */
        case 0x20:
            m_mtcSeconds = data_byte & 0x0f;
            break;
        case 0x30:
            m_mtcSeconds |= (data_byte & 0x0f) << 4;
            break;
            
        /* Minutes */
        case 0x40:
            m_mtcMinutes = data_byte & 0x0f;
            break;
        case 0x50:
            m_mtcMinutes |= (data_byte & 0x0f) << 4;
            break;
            
        /* Hours and SMPTE type */
        case 0x60:
            m_mtcHours = data_byte & 0x0f;
            break;

        case 0x70:
	{
            m_mtcHours |= (data_byte & 0x01) << 4;
            m_mtcSMPTEType = (data_byte & 0x06) >> 1;

	    int fps = 30;
	    if (m_mtcSMPTEType == 0) fps = 24;
	    else if (m_mtcSMPTEType == 1) fps = 25;

            /*
             * Ok, got all the bits now
             * (Assuming time is rolling forward)
             */

            /* correct for 2-frame lag */
            m_mtcFrames += 2;
            if (m_mtcFrames >= fps) {
                m_mtcFrames -= fps;
                if (++m_mtcSeconds == 60) {
                    m_mtcSeconds = 0;
                    if (++m_mtcMinutes == 60) {
                        m_mtcMinutes = 0;
                        ++m_mtcHours;
                    }
                }
            }
            
#ifdef MTC_DEBUG
            printf("RG MTC: Got a complete sequence: %02d:%02d:%02d.%02d (type %d)\n",
                    m_mtcHours,
                    m_mtcMinutes,
                    m_mtcSeconds,
                    m_mtcFrames,
                    m_mtcSMPTEType);
#endif

            /* compute encoded time */
            m_mtcEncodedTime.sec = m_mtcSeconds +
		                   m_mtcMinutes * 60 +
                                   m_mtcHours * 60 * 60;

	    switch (fps) {
	    case 24:
		m_mtcEncodedTime.nsec = (int)
		    ((125000000UL * (unsigned)m_mtcFrames) / (unsigned) 3);
		break;
	    case 25:
		m_mtcEncodedTime.nsec = (int)
		    (40000000UL * (unsigned)m_mtcFrames);
		break;
	    case 30:
	    default:
		m_mtcEncodedTime.nsec = (int)
		    ((100000000UL * (unsigned)m_mtcFrames) / (unsigned) 3);
		break;
	    }

            /*
             * We only mess with the clock if we are playing
             */
            if (m_playing) {
#ifdef MTC_DEBUG
                std::cerr << "RG MTC: Tstamp " << m_mtcEncodedTime;
                std::cerr << " Received @ " << m_mtcReceiveTime << endl;
#endif

                calibrateMTC();

                RealTime t_diff = m_mtcEncodedTime - m_mtcReceiveTime;
                std::cerr << "Diff: "<< t_diff << endl; 

                /* -ve diff means ALSA time ahead of MTC time */

                if (t_diff.sec > 0) {
                    tweakSkewForMTC(60000);
                }
                else if (t_diff.sec < 0) {
                    tweakSkewForMTC(-60000);
                }
                else {
                    /* "small" diff - use adaptive technique */
                    tweakSkewForMTC(t_diff.nsec / 1400);
			if ((t_diff.nsec /1000000) == 0) {
				if (++lock_count == 3) {
            printf("Got a lock @ %02d:%02d:%02d.%02d (type %d)\n",
                    m_mtcHours,
                    m_mtcMinutes,
                    m_mtcSeconds,
                    m_mtcFrames,
                    m_mtcSMPTEType);
				}
			}
			else {
				lock_count = 0;
			}
                }
                
            } else {
		/* If we're not playing, we should be. */
#ifdef MTC_DEBUG
		std::cerr << "MTC: Received quarter frame while not playing - starting now" << std::endl;
#endif
		ExternalTransport *transport = getExternalTransportControl();
		if (transport) {
		    transport->transportJump
			(ExternalTransport::TransportStartAtTime,
			 m_mtcEncodedTime);
		}
	    }

            break;
	}

        /* Oh dear, demented device! */
        default:
            break;
    }
}

void
AlsaDriver::insertMTCFullFrame(RealTime time)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_syncOutputPort);
    snd_seq_ev_set_subs(&event);

    m_mtcEncodedTime = time;
    m_mtcSeconds = m_mtcEncodedTime.sec % 60;
    m_mtcMinutes = (m_mtcEncodedTime.sec / 60) % 60;
    m_mtcHours = (m_mtcEncodedTime.sec / 3600);

    // We always send at 25fps, it's the easiest to avoid rounding problems
    m_mtcFrames = (unsigned)m_mtcEncodedTime.nsec / 40000000U;

    time = time + m_alsaPlayStartTime - m_playStartPosition;
    snd_seq_real_time_t atime = { time.sec, time.nsec };

    unsigned char data[10] =
	{ MIDI_SYSTEM_EXCLUSIVE,
	  MIDI_SYSEX_RT, 127, 1, 1,
	  0, 0, 0, 0,
	  MIDI_END_OF_EXCLUSIVE };
    
    data[5] = ((unsigned char)m_mtcHours & 0x1f) + (1 << 5); // 1 indicates 25fps
    data[6] = (unsigned char)m_mtcMinutes;
    data[7] = (unsigned char)m_mtcSeconds;
    data[8] = (unsigned char)m_mtcFrames;

    snd_seq_ev_schedule_real(&event, m_queue, 0, &atime);
    snd_seq_ev_set_sysex(&event, 10, data);

    checkAlsaError(snd_seq_event_output(m_midiHandle, &event),
		   "insertMTCFullFrame event send");

    if (m_queueRunning) {
	checkAlsaError(snd_seq_drain_output(m_midiHandle), "insertMTCFullFrame drain");
    }
}

void
AlsaDriver::insertMTCQFrames(RealTime sliceStart, RealTime sliceEnd)
{
    if (sliceStart == RealTime::zeroTime && sliceEnd == RealTime::zeroTime) {
	// not a real slice
	return;
    }
    
    // We send at 25fps, it's the easiest to avoid rounding problems
    RealTime twoFrames(0, 80000000U);
    RealTime quarterFrame(0, 10000000U);
    int fps = 25;

#ifdef MTC_DEBUG
    std::cout << "AlsaDriver::insertMTCQFrames(" << sliceStart << ","
	      << sliceEnd << "): first time " << m_mtcFirstTime << std::endl;
#endif

    RealTime t;

    if (m_mtcFirstTime != 0) { // first time through, reset location
	m_mtcEncodedTime = sliceStart;
	t = sliceStart;
	m_mtcFirstTime = 0;
    } else {
	t = m_mtcEncodedTime + quarterFrame;
    }

    m_mtcSeconds = m_mtcEncodedTime.sec % 60;
    m_mtcMinutes = (m_mtcEncodedTime.sec / 60) % 60;
    m_mtcHours = (m_mtcEncodedTime.sec / 3600);
    m_mtcFrames = (unsigned)m_mtcEncodedTime.nsec / 40000000U; // 25fps

    std::string bytes = " ";

    int type = 0;

    while (m_mtcEncodedTime < sliceEnd) {

	snd_seq_event_t event;
	snd_seq_ev_clear(&event);
	snd_seq_ev_set_source(&event, m_syncOutputPort);
	snd_seq_ev_set_subs(&event);

#ifdef MTC_DEBUG
	std::cout << "Sending MTC quarter frame at " << t << std::endl;
#endif

	unsigned char c = (type << 4);

	switch (type) {
	case 0: c +=  ((unsigned char)m_mtcFrames & 0x0f); break;
	case 1: c += (((unsigned char)m_mtcFrames & 0xf0) >> 4); break;
	case 2: c +=  ((unsigned char)m_mtcSeconds & 0x0f); break;
	case 3: c += (((unsigned char)m_mtcSeconds & 0xf0) >> 4); break;
	case 4: c +=  ((unsigned char)m_mtcMinutes & 0x0f); break;
	case 5: c += (((unsigned char)m_mtcMinutes & 0xf0) >> 4); break;
	case 6: c +=  ((unsigned char)m_mtcHours & 0x0f); break;
	case 7: // hours high nibble + smpte type
	    c += (m_mtcHours >> 4) & 0x01;
	    c += (1 << 1); // type 1 indicates 25fps
	    break;
	}

	RealTime scheduleTime = t + m_alsaPlayStartTime - m_playStartPosition;
	snd_seq_real_time_t atime = { scheduleTime.sec, scheduleTime.nsec };
	
	event.type = SND_SEQ_EVENT_QFRAME;
	event.data.control.value = c;

	snd_seq_ev_schedule_real(&event, m_queue, 0, &atime);

	checkAlsaError(snd_seq_event_output(m_midiHandle, &event),
		       "insertMTCQFrames sending qframe event");

	if (++type == 8) {
	    m_mtcFrames += 2;
	    if (m_mtcFrames >= fps) {
		m_mtcFrames -= fps;
		if (++m_mtcSeconds == 60) {
		    m_mtcSeconds = 0;
		    if (++m_mtcMinutes == 60) {
			m_mtcMinutes = 0;
			++m_mtcHours;
		    }
		}
	    }
	    m_mtcEncodedTime = t;
	    type = 0;
	}

	t = t + quarterFrame;
    }
}

bool
AlsaDriver::testForMTCSysex(const snd_seq_event_t *event)
{
    if (getMTCStatus() != TRANSPORT_SLAVE) return false;

    // At this point, and possibly for the foreseeable future, the only
    // sysex we're interested in is full-frame transport location

#ifdef MTC_DEBUG
    std::cerr << "MTC: testing sysex of length " << event->data.ext.len << ":" << std::endl;
    for (int i = 0; i < event->data.ext.len; ++i) {
	std::cerr << (int)*((unsigned char *)event->data.ext.ptr + i) << " ";
    }
    std::cerr << endl;
#endif

    if (event->data.ext.len != 10) return false;

    unsigned char *ptr = (unsigned char *)(event->data.ext.ptr);
    
    if (*ptr++ != MIDI_SYSTEM_EXCLUSIVE) return false;
    if (*ptr++ != MIDI_SYSEX_RT) return false;
    if (*ptr++ > 127) return false;

    // 01 01 for MTC full frame

    if (*ptr++ != 1) return false;
    if (*ptr++ != 1) return false;

    int htype = *ptr++;
    int min   = *ptr++;
    int sec   = *ptr++;
    int frame = *ptr++;

    if (*ptr != MIDI_END_OF_EXCLUSIVE) return false;

    int hour = (htype & 0x1f);
    int type = (htype & 0xe0) >> 5;

    m_mtcFrames = frame;
    m_mtcSeconds = sec;
    m_mtcMinutes = min;
    m_mtcHours = hour;
    m_mtcSMPTEType = type;

    int fps = 30;
    if (m_mtcSMPTEType == 0) fps = 24;
    else if (m_mtcSMPTEType == 1) fps = 25;

    m_mtcEncodedTime.sec = sec + min * 60 + hour * 60 * 60;
    
    switch (fps) {
    case 24:
	m_mtcEncodedTime.nsec = (int)
	    ((125000000UL * (unsigned)m_mtcFrames) / (unsigned) 3);
	break;
    case 25:
	m_mtcEncodedTime.nsec = (int)
	    (40000000UL * (unsigned)m_mtcFrames);
	break;
    case 30:
    default:
	m_mtcEncodedTime.nsec = (int)
	    ((100000000UL * (unsigned)m_mtcFrames) / (unsigned) 3);
	break;
    }

#ifdef MTC_DEBUG
    std::cerr << "MTC: MTC sysex found (frame type " << type
	      << "), jumping to " << m_mtcEncodedTime << std::endl;
#endif
    
    ExternalTransport *transport = getExternalTransportControl();
    if (transport) {
	transport->transportJump
	    (ExternalTransport::TransportJumpToTime,
	     m_mtcEncodedTime);
    }

    return true;
}

static int last_factor = 0;
static int bias_factor = 0;

void
AlsaDriver::calibrateMTC()
{
    if (m_mtcFirstTime < 0)
        return;
    else if (m_mtcFirstTime > 0)
    {
        --m_mtcFirstTime;
        m_mtcSigmaC = 0;
        m_mtcSigmaE = 0;
    }
    else
    {
        RealTime diff_e = m_mtcEncodedTime - m_mtcLastEncoded;
        RealTime diff_c = m_mtcReceiveTime - m_mtcLastReceive;

#ifdef MTC_DEBUG
        printf("RG MTC: diffs %d %d %d\n", diff_c.nsec, diff_e.nsec, m_mtcSkew);
#endif
        
        m_mtcSigmaE += ((long long int) diff_e.nsec) * m_mtcSkew;
        m_mtcSigmaC += diff_c.nsec;


        int t_bias = (m_mtcSigmaE / m_mtcSigmaC) - 0x10000;

#ifdef MTC_DEBUG
        printf("RG MTC: sigmas %lld %lld %d\n", m_mtcSigmaE, m_mtcSigmaC, t_bias);
#endif

        bias_factor = t_bias;
    }

    m_mtcLastReceive = m_mtcReceiveTime;
    m_mtcLastEncoded = m_mtcEncodedTime;

}

void
AlsaDriver::tweakSkewForMTC(int factor)
{
    if (factor > 50000) {
        factor = 50000;
    }
    else if (factor < -50000) {
        factor = -50000;
    }
    else if (factor == last_factor)
    {
        return;
    }
    else
    {
        if (m_mtcFirstTime == -1)
            m_mtcFirstTime = 5;
    }
    last_factor = factor;

    snd_seq_queue_tempo_t *q_ptr;
    snd_seq_queue_tempo_alloca(&q_ptr);

    snd_seq_get_queue_tempo( m_midiHandle, m_queue, q_ptr);

    unsigned int t_skew = snd_seq_queue_tempo_get_skew(q_ptr);
#ifdef MTC_DEBUG
    std::cerr << "RG MTC: skew: " << t_skew;
#endif

    t_skew = 0x10000 + factor + bias_factor;

#ifdef MTC_DEBUG
    std::cerr << " changed to " << factor << "+" <<bias_factor << endl;
#endif

    snd_seq_queue_tempo_set_skew(q_ptr, t_skew);
    snd_seq_set_queue_tempo( m_midiHandle, m_queue, q_ptr);

    m_mtcSkew = t_skew;
}
    
bool
AlsaDriver::testForMMCSysex(const snd_seq_event_t *event)
{
    if (getMMCStatus() != TRANSPORT_SLAVE) return false;

    if (event->data.ext.len != 6) return false;

    unsigned char *ptr = (unsigned char *)(event->data.ext.ptr);
    
    if (*ptr++ != MIDI_SYSTEM_EXCLUSIVE) return false;
    if (*ptr++ != MIDI_SYSEX_RT) return false;
    if (*ptr++ > 127) return false;
    if (*ptr++ != MIDI_SYSEX_RT_COMMAND) return false;

    int instruction = *ptr++;

    if (*ptr != MIDI_END_OF_EXCLUSIVE) return false;

    if (instruction == MIDI_MMC_PLAY ||
	instruction == MIDI_MMC_DEFERRED_PLAY) {
	ExternalTransport *transport = getExternalTransportControl();
	if (transport) {
	    transport->transportChange(ExternalTransport::TransportPlay);
	}
    } else if (instruction == MIDI_MMC_STOP) {
	ExternalTransport *transport = getExternalTransportControl();
	if (transport) {
	    transport->transportChange(ExternalTransport::TransportStop);
	}
    }

    return true;
}

void
AlsaDriver::processMidiOut(const MappedComposition &mC,
                           const RealTime &sliceStart,
			   const RealTime &sliceEnd)
{
    RealTime outputTime;
    RealTime outputStopTime;
    MappedInstrument *instrument;
    ClientPortPair outputDevice;
    MidiByte channel;
    snd_seq_event_t event;

    // special case for unqueued events
    bool now = (sliceStart == RealTime::zeroTime && sliceEnd == RealTime::zeroTime);

    // These won't change in this slice
    //
    snd_seq_ev_clear(&event);

    if ((mC.begin() != mC.end()) && getSequencerDataBlock()) {
	getSequencerDataBlock()->setVisual(*mC.begin());
    }

#ifdef DEBUG_PROCESS_MIDI_OUT
    std::cerr << "AlsaDriver::processMidiOut(" << sliceStart << "," << sliceEnd
	      << "), " << mC.size() << " events, now is " << now << std::endl;
#endif

    // NB the MappedComposition is implicitly ordered by time (std::multiset)

    for (MappedComposition::const_iterator i = mC.begin(); i != mC.end(); ++i)
    {
        if ((*i)->getType() >= MappedEvent::Audio)
            continue;

	bool isControllerOut = ((*i)->getRecordedDevice() == 
				Rosegarden::Device::CONTROL_DEVICE);
	
	bool isSoftSynth = (!isControllerOut &&
			    ((*i)->getInstrument() >= SoftSynthInstrumentBase));

        outputTime = (*i)->getEventTime() - m_playStartPosition +
                           m_alsaPlayStartTime;

	if (now && !m_playing && m_queueRunning) {
	    // stop queue to ensure exact timing and make sure the
	    // event gets through right now
#ifdef DEBUG_PROCESS_MIDI_OUT
	    std::cerr << "processMidiOut: stopping queue for now-event" << std::endl;
#endif
	    checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, NULL), "processMidiOut(): stop queue");
	    checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
	}

	RealTime alsaTimeNow = getAlsaTime();
	
	if (now) {
	    if (!m_playing) {
		outputTime = alsaTimeNow;
	    } else if (outputTime < alsaTimeNow) {
		outputTime = alsaTimeNow + RealTime(0, 10000000);
	    }
	}

#ifdef DEBUG_PROCESS_MIDI_OUT
	std::cerr << "processMidiOut[" << now << "]: event is at " << outputTime << " (" << outputTime - alsaTimeNow << " ahead of queue time), type " << int((*i)->getType()) << ", duration " << (*i)->getDuration() << std::endl;
#endif

	if (!m_queueRunning && outputTime < alsaTimeNow) {
	    RealTime adjust = alsaTimeNow - outputTime;
	    if ((*i)->getDuration() > RealTime::zeroTime) {
		if ((*i)->getDuration() <= adjust) {
#ifdef DEBUG_PROCESS_MIDI_OUT
		    std::cerr << "processMidiOut[" << now << "]: too late for this event, abandoning it" << std::endl;
#endif
		    continue;
		} else {
#ifdef DEBUG_PROCESS_MIDI_OUT
		    std::cerr << "processMidiOut[" << now << "]: pushing event forward and reducing duration by " << adjust << std::endl;
#endif
		    (*i)->setDuration((*i)->getDuration() - adjust);
		}
	    } else {
#ifdef DEBUG_PROCESS_MIDI_OUT
		std::cerr << "processMidiOut[" << now << "]: pushing zero-duration event forward by " << adjust << std::endl;
#endif
	    }
	    outputTime = alsaTimeNow;
	}

	processNotesOff(outputTime, now);

#ifdef HAVE_LIBJACK
	if (m_jackDriver) {
	    size_t frameCount = m_jackDriver->getFramesProcessed();
	    size_t elapsed = frameCount - _debug_jack_frame_count;
	    RealTime rt = RealTime::frame2RealTime(elapsed, m_jackDriver->getSampleRate());
	    rt = rt - getAlsaTime();
#ifdef DEBUG_PROCESS_MIDI_OUT
	    std::cerr << "processMidiOut[" << now << "]: JACK time is " << rt << " ahead of ALSA time" << std::endl;
#endif 
	}
#endif

        // Second and nanoseconds for ALSA
        //
        snd_seq_real_time_t time = { outputTime.sec, outputTime.nsec };

	if (!isSoftSynth) {

#ifdef DEBUG_PROCESS_MIDI_OUT
	    std::cout << "processMidiOut[" << now << "]: instrument " << (*i)->getInstrument() << std::endl;
	    std::cout << "pitch: " << (int)(*i)->getPitch() << ", velocity " << (int)(*i)->getVelocity() << ", duration " << (*i)->getDuration() << std::endl;
#endif

	    snd_seq_ev_set_subs(&event);

	    // Set source according to port for device
	    //
	    int src;

	    if (isControllerOut) {
		src = m_controllerPort;
	    } else {
		src = getOutputPortForMappedInstrument((*i)->getInstrument());
	    }

	    if (src < 0) continue;
	    snd_seq_ev_set_source(&event, src);

	    snd_seq_ev_schedule_real(&event, m_queue, 0, &time);

	} else {
	    event.time.time = time;
	}

        instrument = getMappedInstrument((*i)->getInstrument());

        // set the stop time for Note Off
        //
        outputStopTime = outputTime + (*i)->getDuration()
	    - RealTime(0, 1); // notch it back 1nsec just to ensure
			      // correct ordering against any other
			      // note-ons at the same nominal time
	bool needNoteOff = false;
 
	if (isControllerOut) {
	    channel = (*i)->getRecordedChannel();
#ifdef DEBUG_ALSA
	    std::cerr << "processMidiOut() - Event of type " << (int)((*i)->getType()) << " (data1 " << (int)(*i)->getData1() << ", data2 " << (int)(*i)->getData2() << ") for external controller channel " << (int)channel << std::endl;
#endif
	} else if (instrument != 0) {
            channel = instrument->getChannel();
	} else {
#ifdef DEBUG_ALSA
            std::cerr << "processMidiOut() - No instrument for event of type "
		      << (int)(*i)->getType() << " at " << (*i)->getEventTime()
                      << std::endl;
#endif
            channel = 0;
        }

        switch((*i)->getType())
        {
            case MappedEvent::MidiNoteOneShot:
                {
                    snd_seq_ev_set_noteon(&event,
					  channel,
					  (*i)->getPitch(),
					  (*i)->getVelocity());
		    needNoteOff = true;

		    if (!isSoftSynth && getSequencerDataBlock()) {
			Rosegarden::LevelInfo info;
			info.level = (*i)->getVelocity();
			info.levelRight = 0;
			getSequencerDataBlock()->setInstrumentLevel
			    ((*i)->getInstrument(), info);
		    }
                }
                break;

            case MappedEvent::MidiNote:
		// We always use plain NOTE ON here, not ALSA
		// time+duration notes, because we have our own NOTE
		// OFF stack (which will be augmented at the bottom of
		// this function) and we want to ensure it gets used
		// for the purposes of e.g. soft synths
                //
		if ((*i)->getVelocity() > 0)
		{
		    snd_seq_ev_set_noteon(&event,
					  channel,
					  (*i)->getPitch(),
					  (*i)->getVelocity());

		    if (!isSoftSynth && getSequencerDataBlock()) {
			Rosegarden::LevelInfo info;
			info.level = (*i)->getVelocity();
			info.levelRight = 0;
			getSequencerDataBlock()->setInstrumentLevel
			    ((*i)->getInstrument(), info);
		    }
		}
		else
		{
		    snd_seq_ev_set_noteoff(&event,
					   channel,
					   (*i)->getPitch(),
					   (*i)->getVelocity());
		}

                break;

            case MappedEvent::MidiProgramChange:
                snd_seq_ev_set_pgmchange(&event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiKeyPressure:
                snd_seq_ev_set_keypress(&event,
                                        channel,
                                        (*i)->getData1(),
                                        (*i)->getData2());
                break;

            case MappedEvent::MidiChannelPressure:
                snd_seq_ev_set_chanpress(&event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiPitchBend:
                {
                    int d1 = (int)((*i)->getData1());
                    int d2 = (int)((*i)->getData2());
                    int value = ((d1 << 7) | d2) - 8192;

                    // keep within -8192 to +8192
                    //
                    // if (value & 0x4000)
                    //    value -= 0x8000;

                    snd_seq_ev_set_pitchbend(&event,
                                             channel,
                                             value);
                }
                break;

            case MappedEvent::MidiSystemMessage:
                {
                    switch((*i)->getData1())
                    {
                        case Rosegarden::MIDI_SYSTEM_EXCLUSIVE:
                            {
                                char out[2];
                                sprintf(out, "%c", MIDI_SYSTEM_EXCLUSIVE);
                                std::string data = out;

                                data += DataBlockRepository::getDataBlockForEvent((*i));

                                sprintf(out, "%c", MIDI_END_OF_EXCLUSIVE);
                                data += out;
    
                                snd_seq_ev_set_sysex(&event,
                                                     data.length(),
                                                     (char*)(data.c_str()));
                            }
                            break;

                        case Rosegarden::MIDI_TIMING_CLOCK:
                            {
                                Rosegarden::RealTime rt =
                                    Rosegarden::RealTime(time.tv_sec, time.tv_nsec);

                                /*
                                std::cerr << "AlsaDriver::processMidiOut - "
                                          << "send clock @ " << rt << std::endl;
                                          */

                                sendSystemQueued(SND_SEQ_EVENT_CLOCK, "", rt);

                                continue;

                            }
                            break;
                             
                        default:
                            std::cerr << "AlsaDriver::processMidiOut - "
                                      << "unrecognised system message" 
                                      << std::endl;
                            break;
                    }
                }
                break;

            case MappedEvent::MidiController:
                snd_seq_ev_set_controller(&event,
                                          channel,
                                          (*i)->getData1(),
                                          (*i)->getData2());
                break;

            case MappedEvent::Audio:
            case MappedEvent::AudioCancel:
            case MappedEvent::AudioLevel:
            case MappedEvent::AudioStopped:
            case MappedEvent::SystemUpdateInstruments:
	    case MappedEvent::SystemJackTransport: //???
            case MappedEvent::SystemMMCTransport:
            case MappedEvent::SystemMIDIClock:
            case MappedEvent::SystemMIDISyncAuto:
                break;

            default:
            case MappedEvent::InvalidMappedEvent:
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processMidiOut - "
                          << "skipping unrecognised or invalid MappedEvent type"
                          << std::endl;
#endif
                continue;
        }

	if (isSoftSynth) {
	    
	    processSoftSynthEventOut((*i)->getInstrument(), &event, now);

	} else {
	    checkAlsaError(snd_seq_event_output(m_midiHandle, &event),
			   "processMidiOut(): output queued");

	    if (now) {
		if (m_queueRunning && !m_playing) {
		    // restart queue
#ifdef DEBUG_PROCESS_MIDI_OUT
		    std::cerr << "processMidiOut: restarting queue after now-event" << std::endl;
#endif
		    checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, NULL), "processMidiOut(): continue queue");
		}
		checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
	    }
	}

        // Add note to note off stack
        //
        if (needNoteOff)
        {
	    NoteOffEvent *noteOffEvent =
		new NoteOffEvent(outputStopTime, // already calculated
				 (*i)->getPitch(),
				 channel,
				 (*i)->getInstrument());

#ifdef DEBUG_ALSA
	    std::cerr << "Adding NOTE OFF at " << outputStopTime
		      << std::endl;
#endif

	    m_noteOffQueue.insert(noteOffEvent);
        }
    }

    processNotesOff(sliceEnd - m_playStartPosition + m_alsaPlayStartTime, now);

    if (getMTCStatus() == TRANSPORT_MASTER) {
	insertMTCQFrames(sliceStart, sliceEnd);
    }

    if (m_queueRunning) {

	if (now && !m_playing) {
	    // just to be sure
#ifdef DEBUG_PROCESS_MIDI_OUT
	    std::cerr << "processMidiOut: restarting queue after all now-events" << std::endl;
#endif
	    checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, NULL), "processMidiOut(): continue queue");
	}

#ifdef DEBUG_PROCESS_MIDI_OUT
//	std::cerr << "processMidiOut: m_queueRunning " << m_queueRunning
//		  << ", now " << now << std::endl;
#endif
	checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
    }
}

void
AlsaDriver::processSoftSynthEventOut(InstrumentId id, const snd_seq_event_t *ev, bool now)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver::processSoftSynthEventOut: instrument " << id << ", now " << now << std::endl;
#endif

#ifdef HAVE_LIBJACK

    if (!m_jackDriver) return;
    RunnablePluginInstance *synthPlugin = m_jackDriver->getSynthPlugin(id);

    if (synthPlugin) {

	RealTime t(ev->time.time.tv_sec, ev->time.time.tv_nsec);

	if (now) t = RealTime::zeroTime;
	else t = t + m_playStartPosition - m_alsaPlayStartTime;

#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::processSoftSynthEventOut: time " << t << std::endl;
#endif

	synthPlugin->sendEvent(t, ev);

	if (now) {
#ifdef DEBUG_ALSA
	    std::cerr << "AlsaDriver::processSoftSynthEventOut: setting haveAsyncAudioEvent" << std::endl;
#endif
	    m_jackDriver->setHaveAsyncAudioEvent();
	}
    }
#endif
}

void
AlsaDriver::startClocks()
{
    int result;

    std::cerr << "AlsaDriver::startClocks" << std::endl;

    if (m_needJackStart) {
	std::cerr << "AlsaDriver::startClocks: Need JACK start (m_playing = " << m_playing << ")" << std::endl;
    }
    
#ifdef HAVE_LIBJACK

    // New JACK transport scheme: The initialisePlayback,
    // resetPlayback and stopPlayback methods set m_needJackStart, and
    // then this method checks it and calls the appropriate JACK
    // transport start or relocate method, which calls back on
    // startClocksApproved when ready.  (Previously this method always
    // called the JACK transport start method, so we couldn't handle
    // moving the pointer when not playing, and we had to stop the
    // transport explicitly from resetPlayback when repositioning
    // during playback.)

    if (m_jackDriver) {

	// Don't need any locks on this, except for those that the
	// driver methods take and hold for themselves

	if (m_needJackStart != NeedNoJackStart) {
	    if (m_needJackStart == NeedJackStart ||
		m_playing) {
		m_jackDriver->prebufferAudio();
	    } else {
		m_jackDriver->prepareAudio();
	    }
	    bool rv;
	    if (m_needJackStart == NeedJackReposition) {
		rv = m_jackDriver->relocateTransport();
	    } else {
		rv = m_jackDriver->startTransport();
	    }
	    if (!rv) {
		std::cerr << "AlsaDriver::startClocks: Waiting for startClocksApproved" << std::endl;
		// need to wait for transport sync
		_debug_jack_frame_count = m_jackDriver->getFramesProcessed();
		return;
	    }
	}
    }
#endif

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, NULL)) < 0) {
	std::cerr << "AlsaDriver::startClocks - couldn't start queue - "
		  << snd_strerror(result)
		  << std::endl;
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
    }

    std::cerr << "AlsaDriver::startClocks: started clocks" << std::endl;

    m_queueRunning = true;

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	_debug_jack_frame_count = m_jackDriver->getFramesProcessed();
    }
#endif

    // If the clock is enabled then adjust for the MIDI Clock to 
    // synchronise the sequencer with the clock.
    //
    if (m_midiClockEnabled)
    {
        // Send the Song Position Pointer for MIDI CLOCK positioning
        //
        // Deconstruct the spp into two midi bytes which we still
        // reconstruct this side of ALSA - yes, I know it's a bit
        // of a waste of time but with no or little ALSA seq documentation
        // and this being the manner of constructing the SPP MIDI
        // message it _feels_ right.  The sendSystemDirect reconstruction
        // was worked out by guesswork.

        // Get time from current alsa time to start of alsa timing -
        // add the initial starting point and divide by the total
        // single clock length.  Divide this result by 6 for the SPP
        // position.
        //
        long spp =
          long(((getAlsaTime() - m_alsaPlayStartTime + m_playStartPosition) /
                     m_midiClockInterval) / 6.0);

        MidiByte lsb = spp & 0x7f;
        MidiByte msb = (spp >> 7) & 0x7f;
        std::string args;
        args += lsb;
        args += msb;

        // Ok now we have the new SPP - stop the transport and restart with the
        // new value.
        //
        sendSystemDirect(SND_SEQ_EVENT_STOP, "");

        sendSystemDirect(SND_SEQ_EVENT_SONGPOS, args);

        /*
        std::cout << "AlsaDriver::startClocks - "
                  << " sending song position pointer = " 
                  << getAlsaTime() - m_alsaPlayStartTime + m_playStartPosition
                  << "s, spp  = "
                  << spp 
                  << ", msb = " << int(msb)
                  << ", lsb = " << int(lsb)
                  << std::endl;
        */

        // Now send the START/CONTINUE
        //
        if (m_playStartPosition == RealTime::zeroTime)
            sendSystemQueued(SND_SEQ_EVENT_START, "",
                             m_alsaPlayStartTime);
        else
            sendSystemQueued(SND_SEQ_EVENT_CONTINUE, "",
                             m_alsaPlayStartTime);
    }

    // process pending MIDI events
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "startClocks(): draining");
}

void
AlsaDriver::startClocksApproved()
{
    std::cerr << "AlsaDriver::startClocks: startClocksApproved" << std::endl;

    //!!!
    m_needJackStart = NeedNoJackStart;
    startClocks();
    return;

    int result;

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, NULL)) < 0) {
	std::cerr << "AlsaDriver::startClocks - couldn't start queue - "
		  << snd_strerror(result)
		  << std::endl;
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
    }

    m_queueRunning = true;

    // process pending MIDI events
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "startClocksApproved(): draining");
}

void
AlsaDriver::stopClocks()
{
    std::cerr << "AlsaDriver::stopClocks" << std::endl;

    if (checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, NULL), "stopClocks(): stopping queue") < 0) {
	reportFailure(Rosegarden::MappedEvent::FailureALSACallFailed);
    }
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "stopClocks(): draining output to stop queue");

    m_queueRunning = false;

    // We used to call m_jackDriver->stop() from here, but we no
    // longer do -- it's now called from stopPlayback() so as to
    // handle repositioning during playback (when stopClocks is
    // necessary but stopPlayback and m_jackDriver->stop() are not).
    
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_real_time_t z = { 0, 0 };
    snd_seq_ev_set_queue_pos_real(&event, m_queue, &z);
    snd_seq_ev_set_direct(&event);
    checkAlsaError(snd_seq_control_queue(m_midiHandle, m_queue, SND_SEQ_EVENT_SETPOS_TIME,
					 0, &event), "stopClocks(): setting zpos to queue");
    // process that
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "stopClocks(): draining output to zpos queue");

    std::cerr << "AlsaDriver::stopClocks: ALSA time now is " << getAlsaTime() << std::endl;

    m_alsaPlayStartTime = RealTime::zeroTime;
}
	

void
AlsaDriver::processEventsOut(const MappedComposition &mC)
{
    processEventsOut(mC, RealTime::zeroTime, RealTime::zeroTime);
}

void
AlsaDriver::processEventsOut(const MappedComposition &mC,
			     const RealTime &sliceStart,
			     const RealTime &sliceEnd)
{
    // special case for unqueued events
    bool now = (sliceStart == RealTime::zeroTime && sliceEnd == RealTime::zeroTime);

    if (m_startPlayback)
    {
        m_startPlayback = false;
	// This only records whether we're playing in principle,
	// not whether the clocks are actually ticking.  Contrariwise,
	// areClocksRunning tells us whether the clocks are ticking
	// but not whether we're actually playing (the clocks go even
	// when we're not).  Check both if you want to know whether
	// we're really rolling.
        m_playing = true;

	if (getMTCStatus() == TRANSPORT_SLAVE) {
	    tweakSkewForMTC(0);
	}
    }

    AudioFile *audioFile = 0;
    bool haveNewAudio = false;

    // insert audio events if we find them
    for (MappedComposition::const_iterator i = mC.begin(); i != mC.end(); ++i)
    {
#ifdef HAVE_LIBJACK

        // Play an audio file
        //
        if ((*i)->getType() == MappedEvent::Audio)
        {
	    if (!m_jackDriver) continue;

	    // This is used for handling asynchronous
	    // (i.e. unexpected) audio events only

	    if ((*i)->getEventTime() > RealTime(-120, 0)) {
		// Not an asynchronous event
		continue;
	    }

            // Check for existence of file - if the sequencer has died
            // and been restarted then we're not always loaded up with
            // the audio file references we should have.  In the future
            // we could make this just get the gui to reload our files
            // when (or before) this fails.
            //
            audioFile = getAudioFile((*i)->getAudioID());

            if (audioFile)
            { 
		MappedAudioFader *fader =
		    dynamic_cast<MappedAudioFader*>
		    (getMappedStudio()->getAudioFader((*i)->getInstrument()));

		if (!fader) {
		    std::cerr << "WARNING: AlsaDriver::processEventsOut: no fader for audio instrument " << (*i)->getInstrument() << std::endl;
		    continue;
		}

		unsigned int channels = fader->getPropertyList(
		    MappedAudioFader::Channels)[0].toInt();

		RealTime bufferLength = getAudioReadBufferLength();
		int bufferFrames = RealTime::realTime2Frame
		    (bufferLength, m_jackDriver->getSampleRate());
		if (bufferFrames % m_jackDriver->getBufferSize()) {
		    bufferFrames /= m_jackDriver->getBufferSize();
		    bufferFrames ++;
		    bufferFrames *= m_jackDriver->getBufferSize();
		}

//#define DEBUG_PLAYING_AUDIO
#ifdef DEBUG_PLAYING_AUDIO
		std::cout << "Creating playable audio file: id " << audioFile->getId() << ", event time " << (*i)->getEventTime() << ", time now " << getAlsaTime() << ", start marker " << (*i)->getAudioStartMarker() << ", duration " << (*i)->getDuration() << ", instrument " << (*i)->getInstrument() << " channels " << channels <<  std::endl;

		std::cout << "Read buffer length is " << bufferLength << " (" << bufferFrames << " frames)" << std::endl;
#endif

                PlayableAudioFile *paf =
                    new PlayableAudioFile((*i)->getInstrument(),
                                          audioFile,
                                          getSequencerTime() +
					  (RealTime(1, 0) / 4),
                                          (*i)->getAudioStartMarker(),
                                          (*i)->getDuration(),
					  bufferFrames,
					  getSmallFileSize() * 1024,
					  channels,
					  m_jackDriver->getSampleRate());

                if ((*i)->isAutoFading())
                {
		    paf->setAutoFade(true);
                    paf->setFadeInTime((*i)->getFadeInTime());
                    paf->setFadeOutTime((*i)->getFadeInTime());

//#define DEBUG_AUTOFADING
#ifdef DEBUG_AUTOFADING
                    std::cout << "PlayableAudioFile is AUTOFADING - "
                              << "in = " << (*i)->getFadeInTime()
                              << ", out = " << (*i)->getFadeOutTime()
                              << std::endl;
#endif
                }
#ifdef DEBUG_AUTOFADING
                else
                {
                    std::cout << "PlayableAudioFile has no AUTOFADE" 
                              << std::endl;
                }
#endif


                // segment runtime id
		paf->setRuntimeSegmentId((*i)->getRuntimeSegmentId());

		m_audioQueue->addUnscheduled(paf);

		haveNewAudio = true;
            }
            else
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "can't find audio file reference" 
                          << std::endl;

                std::cerr << "AlsaDriver::processEventsOut - "
                          << "try reloading the current Rosegarden file"
                          << std::endl;
#else 
                ;
#endif
            }
        }

        // Cancel a playing audio file preview (this is predicated on
        // runtime segment ID and optionally start time)
        //
        if ((*i)->getType() == MappedEvent::AudioCancel)
        {
            cancelAudioFile(*i);
        }

#endif // HAVE_LIBJACK

        if ((*i)->getType() == MappedEvent::SystemMIDIClock)
        {
            if ((*i)->getData1())
            {
                m_midiClockEnabled = true;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK ENABLED"
                          << std::endl;
#endif
            }
            else
            {
                m_midiClockEnabled = false;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK DISABLED"
                          << std::endl;
#endif
            }
        }

        if ((*i)->getType() == MappedEvent::SystemMIDISyncAuto)
        {
            if ((*i)->getData1())
            {
                m_midiSyncAutoConnect = true;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI SYNC AUTO ENABLED"
                          << std::endl;
#endif
		for (DevicePortMap::iterator dpmi = m_devicePortMap.begin();
		     dpmi != m_devicePortMap.end(); ++dpmi) {
		    snd_seq_connect_to(m_midiHandle,
				       m_syncOutputPort,
				       dpmi->second.first,
				       dpmi->second.second);
		}
	    }
            else
            {
                m_midiSyncAutoConnect = false;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI SYNC AUTO DISABLED"
                          << std::endl;
#endif
            }
        }

#ifdef HAVE_LIBJACK

        // Set the JACK transport 
        if ((*i)->getType() == MappedEvent::SystemJackTransport)
        {
            bool enabled = false;
            bool master = false;

            switch ((int)(*i)->getData1())
            {
                case 2:
                    master = true;
		    enabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to follow JACK transport and request JACK timebase master role (not yet implemented)"
                              << std::endl;
#endif
                    break;

                case 1:
		    enabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to follow JACK transport"
                              << std::endl;
#endif
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to ignore JACK transport"
                              << std::endl;
#endif
                    break;
            }

	    if (m_jackDriver) {
		m_jackDriver->setTransportEnabled(enabled);
		m_jackDriver->setTransportMaster(master);
	    }
        }
#endif // HAVE_LIBJACK


        if ((*i)->getType() == MappedEvent::SystemMMCTransport)
        {
            switch ((int)(*i)->getData1())
            {
                case 1:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC MASTER"
                              << std::endl;
#endif
		    setMMCStatus(TRANSPORT_MASTER);
                    break;

                case 2:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC SLAVE"
                              << std::endl;
#endif
		    setMMCStatus(TRANSPORT_SLAVE);
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden MMC Transport DISABLED"
                              << std::endl;
#endif
		    setMMCStatus(TRANSPORT_OFF);
                    break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemMTCTransport)
        {
            switch ((int)(*i)->getData1())
            {
                case 1:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MTC MASTER"
                              << std::endl;
#endif
		    setMTCStatus(TRANSPORT_MASTER);
		    tweakSkewForMTC(0);
		    m_mtcFirstTime = -1;
                    break;

                case 2:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MTC SLAVE"
                              << std::endl;
#endif
		    setMTCStatus(TRANSPORT_SLAVE);
		    m_mtcFirstTime = -1;
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden MTC Transport DISABLED"
                              << std::endl;
#endif
		    setMTCStatus(TRANSPORT_OFF);
		    m_mtcFirstTime = -1;
                    break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemRecordDevice)
        {
            DeviceId recordDevice =
               (DeviceId)((*i)->getData1());
            bool conn = (bool) ((*i)->getData2());

            // Unset connections
            //
            // unsetRecordDevices();

            // Special case to set for all record ports
            //
            if (recordDevice == Device::ALL_DEVICES)
            {
                /* set all record devices */
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "set all record devices - not implemented"
                          << std::endl;
#endif

                /*
		MappedDeviceList::iterator it = m_devices.begin();
                std::vector<int> ports;
                std::vector<int>::iterator pIt;

                for (; it != m_devices.end(); ++it)
                {
                    std::cout << "DEVICE = " << (*it)->getName() << " - DIR = "
                         << (*it)->getDirection() << endl;
                    // ignore ports we can't connect to
                    if ((*it)->getDirection() == MidiDevice::WriteOnly) continue;

                    std::cout << "PORTS = " << ports.size() << endl;
                    ports = (*it)->getPorts();
                    for (pIt = ports.begin(); pIt != ports.end(); ++pIt)
                    {
                        setRecordDevice((*it)->getClient(), *pIt);
                    }
                }
                */
            }
            else
            {
                // Otherwise just for the one device and port
                //
                setRecordDevice(recordDevice, conn);
            }
        }

        if ((*i)->getType() == MappedEvent::SystemAudioPortCounts)
        {
	    // never actually used, I think?
        }

        if ((*i)->getType() == MappedEvent::SystemAudioPorts)
        {
#ifdef HAVE_LIBJACK
	    if (m_jackDriver) {
		int data = (*i)->getData1();
		m_jackDriver->setAudioPorts(data & MappedEvent::FaderOuts,
					    data & MappedEvent::SubmasterOuts);
	    }
#else
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::processEventsOut - "
                      << "MappedEvent::SystemAudioPorts - no audio subsystem"
                      << std::endl;
#endif
#endif
        }
        
	if ((*i)->getType() == MappedEvent::SystemAudioFileFormat) 
	{
#ifdef HAVE_LIBJACK
	    int format = (*i)->getData1();
	    switch (format) {
	    case 0:
		m_audioRecFileFormat = RIFFAudioFile::PCM;
		break;
	    case 1:
		m_audioRecFileFormat = RIFFAudioFile::FLOAT;
		break;
	    default:
#ifdef DEBUG_ALSA
		std::cerr << "AlsaDriver::processEventsOut - "
			  << "MappedEvent::SystemAudioFileFormat - unexpected format number " << format
			  << std::endl;
#endif
		break;
	    }
#else
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::processEventsOut - "
                      << "MappedEvent::SystemAudioFileFormat - no audio subsystem"
                      << std::endl;
#endif
#endif
	}

	if ((*i)->getType() == MappedEvent::Panic)
        {
	    for (MappedDeviceList::iterator i = m_devices.begin();
		 i != m_devices.end(); ++i) {
		if ((*i)->getDirection() == Rosegarden::MidiDevice::Play) {
		    sendDeviceController((*i)->getId(),
					 MIDI_CONTROLLER_SUSTAIN, 0);
		    sendDeviceController((*i)->getId(),
					 MIDI_CONTROLLER_ALL_NOTES_OFF, 0);
		    sendDeviceController((*i)->getId(),
					 MIDI_CONTROLLER_RESET, 0);
		}
	    }
	}
    }

    // Process Midi and Audio
    //
    processMidiOut(mC, sliceStart, sliceEnd);

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	if (haveNewAudio) {
	    if (now) {
		m_jackDriver->prebufferAudio();
		m_jackDriver->setHaveAsyncAudioEvent();
	    }
	    if (m_queueRunning) {
		m_jackDriver->kickAudio();
	    }
	}
    }
#endif
}

bool
AlsaDriver::record(RecordStatus recordStatus,
		   const std::vector<InstrumentId> *armedInstruments,
		   const std::vector<QString> *audioFileNames)
{
    m_recordingInstruments.clear();

    if (recordStatus == RECORD_ON)
    {
        // start recording
        m_recordStatus = RECORD_ON;
	m_alsaRecordStartTime = RealTime::zeroTime;

	unsigned int audioCount = 0;

	if (armedInstruments) {

	    for (unsigned int i = 0; i < armedInstruments->size(); ++i) {

		Rosegarden::InstrumentId id = (*armedInstruments)[i];

		m_recordingInstruments.insert(id);
		if (!audioFileNames || (audioCount >= audioFileNames->size())) {
		    continue;
		}

		QString fileName = (*audioFileNames)[audioCount];

		if (id >= Rosegarden::AudioInstrumentBase &&
		    id <  Rosegarden::MidiInstrumentBase) {

		    bool good = false;

#ifdef DEBUG_ALSA
		    std::cerr << "AlsaDriver::record: Requesting new record file \"" << fileName << "\" for instrument " << id << std::endl;
#endif

#ifdef HAVE_LIBJACK
		    if (m_jackDriver &&
			m_jackDriver->openRecordFile(id, fileName.data())) {
			good = true;
		    }
#endif
		    
		    if (!good) {
			m_recordStatus = RECORD_OFF;
			std::cerr << "AlsaDriver::record: No JACK driver, or JACK driver failed to prepare for recording audio" << std::endl;
			return false;
		    }

		    ++audioCount;
		}
	    }
        }
    }
    else
    if (recordStatus == RECORD_OFF)
    {
        m_recordStatus = RECORD_OFF;
    }
#ifdef DEBUG_ALSA
    else
    {
        std::cerr << "AlsaDriver::record - unsupported recording mode"
                  << std::endl;
    }
#endif

    return true;
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair(-1, -1);
    AlsaPortList::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        destPair.first = (*it)->m_client;
        destPair.second = (*it)->m_port;

        // If duplex port is required then choose first one
        //
        if (duplex)
        {
            if ((*it)->m_direction == Duplex)
                return destPair;
        }
        else
        {
            // If duplex port isn't required then choose first
            // specifically non-duplex port (should be a synth)
            //
            if ((*it)->m_direction != Duplex)
                return destPair;
        }
    }

    return destPair;
}


// Sort through the ALSA client/port pairs for the range that
// matches the one we're querying.  If none matches then send
// back -1 for each.
//
ClientPortPair
AlsaDriver::getPairForMappedInstrument(InstrumentId id)
{
    MappedInstrument *instrument = getMappedInstrument(id);
    if (instrument)
    {
	DeviceId device = instrument->getDevice();
	DevicePortMap::iterator i = m_devicePortMap.find(device);
	if (i != m_devicePortMap.end())
	{
	    return i->second;
	}
    }
#ifdef DEBUG_ALSA
    /*
    else
    {
	cerr << "WARNING: AlsaDriver::getPairForMappedInstrument: couldn't find instrument for id " << id << ", falling through" << endl;
    }
    */
#endif

    return ClientPortPair(-1, -1);
}

int
AlsaDriver::getOutputPortForMappedInstrument(InstrumentId id)
{
    MappedInstrument *instrument = getMappedInstrument(id);
    if (instrument)
    {
	DeviceId device = instrument->getDevice();
	DeviceIntMap::iterator i = m_outputPorts.find(device);
	if (i != m_outputPorts.end()) {
	    return i->second;
	}
#ifdef DEBUG_ALSA
	else {
	    cerr << "WARNING: AlsaDriver::getOutputPortForMappedInstrument: couldn't find output port for device for instrument " << id << ", falling through" << endl;
	}
#endif
    }

    return -1;
}

// Send a direct controller to the specified port/client
//
void
AlsaDriver::sendDeviceController(DeviceId device,
                                 MidiByte controller,
                                 MidiByte value)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    
    snd_seq_ev_set_subs(&event);

    DeviceIntMap::iterator dimi = m_outputPorts.find(device);
    if (dimi == m_outputPorts.end()) return;
    
    snd_seq_ev_set_source(&event, dimi->second);
    snd_seq_ev_set_direct(&event);
    
    for (int i = 0; i < 16; i++)
    {
        snd_seq_ev_set_controller(&event,
                                  i,
                                  controller,
                                  value);
        snd_seq_event_output_direct(m_midiHandle, &event);
    }

    // we probably don't need this:
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "sendDeviceController(): draining");
}

void
AlsaDriver::processPending()
{
    if (!m_playing) {
	processNotesOff(getAlsaTime(), true);
	checkAlsaError(snd_seq_drain_output(m_midiHandle), "processPending(): draining");
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	m_jackDriver->updateAudioData();
    }
#endif

    scavengePlugins();
    m_audioQueueScavenger.scavenge();
}

void
AlsaDriver::insertMappedEventForReturn(MappedEvent *mE)
{
    // Insert the event ready for return at the next opportunity.
    // 
    m_returnComposition.insert(mE);
}

// check for recording status on any ALSA Port
//
bool
AlsaDriver::isRecording(AlsaPortDescription *port)
{
    if (port->isReadable()) {

	snd_seq_query_subscribe_t *qSubs;
	snd_seq_addr_t rg_addr, sender_addr;
	snd_seq_query_subscribe_alloca(&qSubs);

	rg_addr.client = m_client;
	rg_addr.port = m_inputPort;

	snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
	snd_seq_query_subscribe_set_index(qSubs, 0);
	snd_seq_query_subscribe_set_root(qSubs, &rg_addr);

	while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0)
	{
	    sender_addr = *snd_seq_query_subscribe_get_addr(qSubs);
	    if (sender_addr.client == port->m_client &&
	        sender_addr.port == port->m_port)
	        return true;

            snd_seq_query_subscribe_set_index(qSubs,
                snd_seq_query_subscribe_get_index(qSubs) + 1);
	}
    }	
    return false;
}

bool
AlsaDriver::checkForNewClients()
{
    Audit audit;
    bool madeChange = false;

    if (!m_portCheckNeeded)
    	return false;
    
    AlsaPortList newPorts;
    generatePortList(&newPorts); // updates m_alsaPorts, returns new ports as well

    // If any devices have connections that no longer exist,
    // clear those connections and stick them in the suspended
    // port map in case they come back online later.

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	
	ClientPortPair pair(m_devicePortMap[(*i)->getId()]);
	
	bool found = false;
	for (AlsaPortList::iterator j = m_alsaPorts.begin();
	     j != m_alsaPorts.end(); ++j) {
	    if ((*j)->m_client == pair.first &&
		(*j)->m_port == pair.second) {
		if ((*i)->getDirection() == MidiDevice::Record) {
		    bool recState = isRecording(*j);
		    if (recState != (*i)->isRecording()) {
		        madeChange = true;
			(*i)->setRecording(recState);
		    }
		} else {
			(*i)->setRecording(false);
		}
		found = true;
		break;
	    }
	}
	
	if (!found) {
	    m_suspendedPortMap[pair] = (*i)->getId();
	    m_devicePortMap[(*i)->getId()] = ClientPortPair(-1, -1);
	    setConnectionToDevice(**i, "");
	    (*i)->setRecording(false);
	    madeChange = true;
	}
    }
    
    // If we've increased the number of connections, we need
    // to assign the new connections to existing devices that
    // have none, where possible, and create new devices for
    // any left over.
    
    if (newPorts.size() > 0) {

	audit << "New ports:" << std::endl;

	for (AlsaPortList::iterator i = newPorts.begin();
	     i != newPorts.end(); ++i) {

	    if ((*i)->m_client == m_client) {
		audit << "(Ignoring own port " << (*i)->m_client << ":" << (*i)->m_port << ")" << std::endl;
		continue;
	    } else if ((*i)->m_client == 0) {
		audit << "(Ignoring system port " << (*i)->m_client << ":" << (*i)->m_port << ")" << std::endl;
		continue;
	    }

	    audit << (*i)->m_name << std::endl;

	    std::string portName = (*i)->m_name;
	    ClientPortPair portPair = ClientPortPair((*i)->m_client,
						     (*i)->m_port);

	    if (m_suspendedPortMap.find(portPair) != m_suspendedPortMap.end()) {

		DeviceId id = m_suspendedPortMap[portPair];

		audit << "(Reusing suspended device " << id << ")" << std::endl;

		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getId() == id) {
			setConnectionToDevice(**j, portName);
		    }
		}

		m_suspendedPortMap.erase(m_suspendedPortMap.find(portPair));
		m_devicePortMap[id] = portPair;
		madeChange = true;
		continue;
	    }
	    
	    bool needPlayDevice = true, needRecordDevice = true;

	    if ((*i)->isReadable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Record) {
			audit << "(Reusing record device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			setConnectionToDevice(**j, portName);
			needRecordDevice = false;
			madeChange = true;
			break;
		    }
		}
	    } else {
		needRecordDevice = false;
	    }

	    if ((*i)->isWriteable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Play) {
			audit << "(Reusing play device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			setConnectionToDevice(**j, portName);
			needPlayDevice = false;
			madeChange = true;
			break;
		    }
		}
	    } else {
		needPlayDevice = false;
	    }

	    if (needRecordDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Record);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create record device" << std::endl;
#else 
                    ;
#endif
		} else {
		    audit << "(Created new record device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		    madeChange = true;
		}
	    }

	    if (needPlayDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Play);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                    ;
#endif
		} else {
		    audit << "(Created new play device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		    madeChange = true;
		}
	    }
	}
    }

    // If one of our ports is connected to a single other port and
    // it isn't the one we thought, we should update our connection

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {

	DevicePortMap::iterator j = m_devicePortMap.find((*i)->getId());

	snd_seq_addr_t addr;
	addr.client = m_client;

	DeviceIntMap::iterator ii = m_outputPorts.find((*i)->getId());
	if (ii == m_outputPorts.end()) continue;
	addr.port = ii->second;

	snd_seq_query_subscribe_t *subs;
	snd_seq_query_subscribe_alloca(&subs);
	snd_seq_query_subscribe_set_root(subs, &addr);
	snd_seq_query_subscribe_set_index(subs, 0);
	
	bool haveOurs = false;
	int others = 0;
	ClientPortPair firstOther;

	while (!snd_seq_query_port_subscribers(m_midiHandle, subs)) {

	    const snd_seq_addr_t *otherEnd =
		snd_seq_query_subscribe_get_addr(subs);
		
	    if (!otherEnd) continue;

	    if (j != m_devicePortMap.end() &&
		otherEnd->client == j->second.first &&
		otherEnd->port == j->second.second) {
		haveOurs = true;
	    } else {
		++others;
		firstOther = ClientPortPair(otherEnd->client, otherEnd->port);
	    }

	    snd_seq_query_subscribe_set_index
		(subs, snd_seq_query_subscribe_get_index(subs) + 1);
	}

	if (haveOurs) { // leave our own connection alone, and stop worrying
	    continue;

	} else {
	    if (others == 0) {
		if (j != m_devicePortMap.end()) {
		    j->second = ClientPortPair(-1, -1);
		    setConnectionToDevice(**i, "");
		    madeChange = true;
		}
	    } else {
		for (AlsaPortList::iterator k = m_alsaPorts.begin();
		     k != m_alsaPorts.end(); ++k) {
		    if ((*k)->m_client == firstOther.first &&
			(*k)->m_port == firstOther.second) {
			m_devicePortMap[(*i)->getId()] = firstOther;
			setConnectionToDevice(**i, (*k)->m_name, firstOther);
			madeChange = true;
			break;
		    }
		}
	    }
	}
    }

    if (madeChange) {
	MappedEvent *mE =
	    new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			    0, 0);
	// send completion event
	insertMappedEventForReturn(mE);
    }
    
    m_portCheckNeeded = false;
    	
    return true;
}


// From a DeviceId get a client/port pair for connecting as the
// MIDI record device.
//
void
AlsaDriver::setRecordDevice(DeviceId id, bool connectAction)
{
    Audit audit;

    // Locate a suitable port
    //
    if (m_devicePortMap.find(id) == m_devicePortMap.end()) {
	audit << "AlsaDriver::setRecordDevice - "
	      << "couldn't match device id (" << id << ") to ALSA port"
	      << std::endl;
        return;
    }

    ClientPortPair pair = m_devicePortMap[id];

    snd_seq_addr_t sender, dest;
    sender.client = pair.first;
    sender.port = pair.second;

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	if ((*i)->getId() == id) {
	    if ((*i)->getDirection() == MidiDevice::Record) 
	    {
	    	if ((*i)->isRecording() && connectAction) 
	    	{
		    audit << "AlsaDriver::setRecordDevice - "
			  << "attempting to subscribe (" << id 
			  << ") already subscribed" << std::endl;
	    		
	    	    return;
	    	}
	    	if (!(*i)->isRecording() && !connectAction) 
	    	{
		    audit << "AlsaDriver::setRecordDevice - "
			  << "attempting to unsubscribe (" << id 
			  << ") already unsubscribed" << std::endl;
	    		
	    	    return;
	    	}
	    }
	    else
	    {
		audit << "AlsaDriver::setRecordDevice - "
		      << "attempting to set play device (" << id 
		      << ") to record device" << std::endl;
		return;
	    }
	    break;
	}
    }

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    dest.client = m_client;
    dest.port = m_inputPort;

    // Set destinations and senders
    //
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    // subscribe or unsubscribe the port
    //
    if (connectAction) {
	if (checkAlsaError(snd_seq_subscribe_port(m_midiHandle, subs), 
	    "setRecordDevice - failed subscription of input port") < 0)
	{
	    // Not the end of the world if this fails but we
	    // have to flag it internally.
	    //
	    audit << "AlsaDriver::setRecordDevice - "
		  << int(sender.client) << ":" << int(sender.port)
		  << " failed to subscribe device " 
		  << id << " as record port" << std::endl;
	}
	else
	{
	    m_midiInputPortConnected = true;
            audit << "AlsaDriver::setRecordDevice - "
		  << "successfully subscribed device "
		  << id << " as record port" << std::endl;
	}
    }
    else
    {
    	if(checkAlsaError(snd_seq_unsubscribe_port(m_midiHandle, subs),
    		"setRecordDevice - failed to unsubscribe a device") == 0)
            audit << "AlsaDriver::setRecordDevice - "
		  << "successfully unsubscribed device "
		  << id << " as record port" << std::endl;
    			
    }
}

// Clear any record device connections
//
void
AlsaDriver::unsetRecordDevices()
{
    snd_seq_addr_t dest;
    dest.client = m_client;
    dest.port = m_inputPort;

    snd_seq_query_subscribe_t *qSubs;
    snd_seq_addr_t tmp_addr;
    snd_seq_query_subscribe_alloca(&qSubs);

    tmp_addr.client = m_client;
    tmp_addr.port = m_inputPort;

    // Unsubsribe any existing connections
    //
    snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(qSubs, 0);
    snd_seq_query_subscribe_set_root(qSubs, &tmp_addr);

    while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0)
    {
        tmp_addr = *snd_seq_query_subscribe_get_addr(qSubs);

        snd_seq_port_subscribe_t *dSubs;
        snd_seq_port_subscribe_alloca(&dSubs);

        snd_seq_addr_t dSender;
        dSender.client = tmp_addr.client;
        dSender.port = tmp_addr.port;

        snd_seq_port_subscribe_set_sender(dSubs, &dSender);
        snd_seq_port_subscribe_set_dest(dSubs, &dest);

        int error = snd_seq_unsubscribe_port(m_midiHandle, dSubs);

        if (error < 0)
        {
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::unsetRecordDevices - "
                      << "can't unsubscribe record port" << std::endl;
#endif
        }

        snd_seq_query_subscribe_set_index(qSubs,
                snd_seq_query_subscribe_get_index(qSubs) + 1);
    }
}

void
AlsaDriver::sendMMC(MidiByte deviceArg,
                    MidiByte instruction,
                    bool isCommand,
                    const std::string &data)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_syncOutputPort);
    snd_seq_ev_set_subs(&event);

    unsigned char dataArr[10] =
	{ MIDI_SYSTEM_EXCLUSIVE,
	  MIDI_SYSEX_RT, deviceArg,
	  (isCommand ? MIDI_SYSEX_RT_COMMAND : MIDI_SYSEX_RT_RESPONSE),
	  instruction };

    std::string dataString = std::string((const char *)dataArr) +
	data + (char)MIDI_END_OF_EXCLUSIVE;
	
    snd_seq_ev_set_sysex(&event, dataString.length(),
			 (char *)dataString.c_str());

    event.queue = SND_SEQ_QUEUE_DIRECT;

    checkAlsaError(snd_seq_event_output_direct(m_midiHandle, &event),
		   "sendMMC event send");

    if (m_queueRunning) {
	checkAlsaError(snd_seq_drain_output(m_midiHandle), "sendMMC drain");
    }
}

// Send a system real-time message from the sync output port
//
void
AlsaDriver::sendSystemDirect(MidiByte command, const std::string &args)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_syncOutputPort);
    snd_seq_ev_set_subs(&event);

    event.queue = SND_SEQ_QUEUE_DIRECT;

    // set the command
    event.type = command;

    // set args if we have them
    switch(args.length())
    {
    case 0:
	break;
	
    case 1:
	event.data.control.value = args[0];
	break;
	
    case 2:
	event.data.control.value = int(args[0]) | (int(args[1]) << 7);
	break;
	
    default: // do nothing
	std::cerr << "AlsaDriver::sendSystemDirect - "
		  << "too many argument bytes" << std::endl;
	break;
    }

    int error = snd_seq_event_output_direct(m_midiHandle, &event);

    if (error < 0)
    {
#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::sendSystemDirect - "
		  << "can't send event (" << int(command) << ")"
		  << std::endl;
#endif
    }

//    checkAlsaError(snd_seq_drain_output(m_midiHandle),
//		   "sendSystemDirect(): draining");
}


void
AlsaDriver::sendSystemQueued(MidiByte command,
                             const std::string &args,
                             const RealTime &time)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_syncOutputPort);
    snd_seq_ev_set_subs(&event);

    snd_seq_real_time_t sendTime = { time.sec, time.nsec };

    // Schedule the command
    //
    event.type = command;

    snd_seq_ev_schedule_real(&event, m_queue, 0, &sendTime);

    // set args if we have them
    switch(args.length())
    {
    case 1:
	event.data.control.value = args[0];
	break;
	
    case 2:
	event.data.control.value = int(args[0]) | (int(args[1]) << 7);
	break;
	
    default: // do nothing
	break;
    }
    
    int error = snd_seq_event_output(m_midiHandle, &event);
    
    if (error < 0)
    {
#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::sendSystemQueued - "
		  << "can't send event (" << int(command) << ")"
		  << " - error = (" << error << ")"
		  << std::endl;
#endif
    }

//    if (m_queueRunning) {
//	checkAlsaError(snd_seq_drain_output(m_midiHandle), "sendSystemQueued(): draining");
//    }
}


void
AlsaDriver::claimUnwantedPlugin(void *plugin)
{
    m_pluginScavenger.claim((RunnablePluginInstance *)plugin);
}


void
AlsaDriver::scavengePlugins()
{
    m_pluginScavenger.scavenge();
}


QString
AlsaDriver::getStatusLog()
{
    return QString::fromUtf8(Audit::getAudit().c_str());
}


void
AlsaDriver::sleep(const RealTime &rt)
{
    int npfd = snd_seq_poll_descriptors_count(m_midiHandle, POLLIN);
    struct pollfd *pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(m_midiHandle, pfd, npfd, POLLIN);
    poll(pfd, npfd, rt.sec * 1000 + rt.msec());
}

void
AlsaDriver::runTasks()
{
#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
	if (!m_jackDriver->isOK()) {
	    m_jackDriver->restoreIfRestorable();
	}
    }

    if (m_doTimerChecks && m_timerRatioCalculated) {

	double ratio = m_timerRatio;
	m_timerRatioCalculated = false;

	snd_seq_queue_tempo_t *q_ptr;
	snd_seq_queue_tempo_alloca(&q_ptr);

	snd_seq_get_queue_tempo(m_midiHandle, m_queue, q_ptr);

	unsigned int t_skew = snd_seq_queue_tempo_get_skew(q_ptr);
	unsigned int t_base = snd_seq_queue_tempo_get_skew_base(q_ptr);
	
#ifdef DEBUG_ALSA
	if (!m_playing) {
	    std::cerr << "Skew: " << t_skew << "/" << t_base;
	}
#endif
	
	unsigned int newSkew = t_skew + (unsigned int)(t_skew * ratio);
	
	if (newSkew != t_skew) {
#ifdef DEBUG_ALSA
	    if (!m_playing) {
		std::cerr << " changed to " << newSkew << endl;
	    }
#endif
	    snd_seq_queue_tempo_set_skew(q_ptr, newSkew);
	    snd_seq_set_queue_tempo( m_midiHandle, m_queue, q_ptr);
	} else {
#ifdef DEBUG_ALSA
	    if (!m_playing) {
		std::cerr << endl;
	    }
#endif
	}

	m_firstTimerCheck = true;
    }

#endif
}

void 
AlsaDriver::reportFailure(Rosegarden::MappedEvent::FailureCode code)
{
    // Ignore consecutive duplicates
    if (_failureReportWriteIndex > 0 &&
	_failureReportWriteIndex != _failureReportReadIndex) {
	if (code == _failureReports[_failureReportWriteIndex - 1]) return;
    }

    _failureReports[_failureReportWriteIndex] = code;
    _failureReportWriteIndex =
	(_failureReportWriteIndex + 1) % FAILURE_REPORT_COUNT;
}

}


#endif // HAVE_ALSA
