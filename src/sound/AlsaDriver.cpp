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

#ifdef __GNUG__
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

#define RG_MODULE_STRING "[AlsaDriver]"

#include "misc/Debug.h"
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#include "rosegarden-version.h"

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>
#include <alsa/seq.h>

#include "AlsaDriver.h"
#include "AlsaPort.h"
#include "ExternalTransport.h"
#include "MappedInstrument.h"
#include "Midi.h"
#include "MappedStudio.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "MappedCommon.h"
#include "MappedEvent.h"
#include "Audit.h"
#include "AudioPlayQueue.h"
#include "ExternalTransport.h"

#include <QMutex>
#include <QRegExp>
#include <QSettings>
#include <QTime>

#include <pthread.h>
#include <math.h>
#include <unistd.h>


// #define DEBUG_ALSA 1
// #define DEBUG_PROCESS_MIDI_OUT 1
//#define DEBUG_PROCESS_SOFT_SYNTH_OUT 1
//#define MTC_DEBUG 1

// This driver implements MIDI in and out via the ALSA (www.alsa-project.org)
// sequencer interface.

#define AUTO_TIMER_NAME "(auto)"
#define LOCKED QMutexLocker rg_alsa_locker(&m_mutex)

// Rosegarden does not handle note-off velocity.  The MIDI spec recommends
// using 64 in that case.  One user has reported problems with 0 which
// was being used previously.  See Bug #1426.
#define NOTE_OFF_VELOCITY 64

namespace Rosegarden
{

#ifdef HAVE_LIBJACK
static size_t debug_jack_frame_count = 0;
#endif

#define FAILURE_REPORT_COUNT 256
static MappedEvent::FailureCode failureReports[FAILURE_REPORT_COUNT];
static int failureReportWriteIndex = 0;
static int failureReportReadIndex = 0;

namespace {
    enum ClientClass { Internal, Hardware, Software };

    ClientClass getClass(int clientId)
    {
        // From https://alsa.opensrc.org/Aconnect
        //   0..63: for internal use (0 = system, 63 = OSS sequencer emulation)
        //   64..127: device drivers (up to 8 for each card)
        //   128..?: user applications

        if (clientId < 64)
            return Internal;
        if (clientId < 128)
            return Hardware;
        return Software;
    }
}

AlsaDriver::AlsaDriver(MappedStudio *studio):
    SoundDriver(studio,
                std::string("[ALSA library version ") +
                std::string(SND_LIB_VERSION_STR) + 
                std::string(", module version ") +
                getAlsaModuleVersionString() + 
                std::string(", kernel version ") +
                getKernelVersionString() +
                "]"),
    m_midiHandle(nullptr),
    m_client( -1),
    m_inputPort( -1),
    m_syncOutputPort( -1),
    m_controllerPort( -1),
    m_queue( -1),
    m_maxClients( -1),
    m_maxPorts( -1),
    m_maxQueues( -1),
    m_midiInputPortConnected(false),
    m_midiSyncAutoConnect(false),
    m_alsaPlayStartTime(0, 0),
    m_alsaRecordStartTime(0, 0),
    m_loopStartTime(0, 0),
    m_loopEndTime(0, 0),
    m_eat_mtc(0),
    m_looping(false),
    m_haveShutdown(false)
#ifdef HAVE_LIBJACK
    , m_jackDriver(nullptr)
#endif
    , m_queueRunning(false)
    , m_portCheckNeeded(false),
    m_needJackStart(NeedNoJackStart),
    m_doTimerChecks(false),
    m_firstTimerCheck(true),
    m_timerRatio(0),
    m_timerRatioCalculated(false),
    m_debug(false)
{
    // Create a log that the user can easily see through the preferences
    // even in a release build.
    AUDIT << "Rosegarden " << VERSION << " - AlsaDriver " << m_name << '\n';
    RG_DEBUG << "ctor: Rosegarden " << VERSION << " - AlsaDriver " << m_name;

    m_pendSysExcMap = new DeviceEventMap();

#ifndef NDEBUG
    // Debugging Mode
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    const QString debugAlsaDriver = "debug_AlsaDriver";
    m_debug = settings.value(debugAlsaDriver, false).toBool();
    // Write it to the file to make it easier to find.
    settings.setValue(debugAlsaDriver, m_debug);
    settings.endGroup();
#endif
}

AlsaDriver::~AlsaDriver()
{
    if (!m_haveShutdown) {
        RG_WARNING << "dtor: WARNING: AlsaDriver::shutdown() was not called before destructor, calling now";
        shutdown();
    }

    // Flush incomplete system exclusive events and delete the map.
    clearPendSysExcMap();

    delete m_pendSysExcMap;
}

int
AlsaDriver::checkAlsaError(int rc, const char *
#ifdef DEBUG_ALSA
                           message
#endif
                           )
{
#ifdef DEBUG_ALSA
    if (rc < 0) {
        RG_WARNING << "AlsaDriver::"
                   << message
                   << ": " << rc
                   << " (" << snd_strerror(rc) << ")";
    }
#endif
    return rc;
}

void
AlsaDriver::shutdown()
{
    RG_DEBUG << "shutdown(): shutting down...";

    if (m_midiHandle) {
        processNotesOff(getAlsaTime(), true, true);
    }

#ifdef HAVE_LIBJACK
    delete m_jackDriver;
    m_jackDriver = nullptr;
#endif

    if (m_midiHandle) {

        RG_DEBUG << "shutdown(): stopping queue...";

        checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, nullptr), "shutdown(): stopping queue");
        checkAlsaError(snd_seq_drain_output(m_midiHandle), "shutdown(): drain output");

        RG_DEBUG << "shutdown(): closing MIDI handle...";

        snd_seq_close(m_midiHandle);

        m_midiHandle = nullptr;
    }

    DataBlockRepository::clear();

    clearDevices();

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
        RG_WARNING << "getSystemInfo(): Error: " << snd_strerror(err);
        reportFailure(MappedEvent::FailureALSACallFailed);
        m_maxQueues = 0;
        m_maxClients = 0;
        m_maxPorts = 0;
        return ;
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

    for (idx = min; idx < max; ++idx) {
        if ((err = snd_seq_get_queue_status(m_midiHandle, idx, status)) < 0) {

            if (err == -ENOENT)
                continue;

            RG_WARNING << "showQueueStatus(): Client " << idx << " info error: " << snd_strerror(err);

            reportFailure(MappedEvent::FailureALSACallFailed);
            return ;
        }

#ifdef DEBUG_ALSA
        RG_DEBUG << "showQueueStatus(): Queue " << snd_seq_queue_status_get_queue(status);
        RG_DEBUG << "showQueueStatus(): Tick       = " << snd_seq_queue_status_get_tick_time(status);
        RG_DEBUG << "showQueueStatus(): Realtime   = " << snd_seq_queue_status_get_real_time(status)->tv_sec << "." << snd_seq_queue_status_get_real_time(status)->tv_nsec;
        RG_DEBUG << "showQueueStatus(): Flags      = 0x" << snd_seq_queue_status_get_status(status);
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

            if (snd_timer_query_next_device(timerQuery, timerId) < 0)
                break;
            if (snd_timer_id_get_class(timerId) < 0)
                break;

            AlsaTimerInfo info = {
                snd_timer_id_get_class(timerId),
                snd_timer_id_get_sclass(timerId),
                snd_timer_id_get_card(timerId),
                snd_timer_id_get_device(timerId),
                snd_timer_id_get_subdevice(timerId),
                "",
                0
            };

            if (info.card < 0)
                info.card = 0;
            if (info.device < 0)
                info.device = 0;
            if (info.subdevice < 0)
                info.subdevice = 0;

            //RG_DEBUG << "generateTimerList(): got timer: class " << info.clas;

            sprintf(timerName, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i",
                    info.clas, info.sclas, info.card, info.device, info.subdevice);

            if (snd_timer_open(&timerHandle, timerName, SND_TIMER_OPEN_NONBLOCK) < 0) {
                RG_WARNING << "generateTimerList(): Failed to open timer: " << timerName;
                continue;
            }

            if (snd_timer_info(timerHandle, timerInfo) < 0)
                continue;

            info.name = snd_timer_info_get_name(timerInfo);
            info.resolution = snd_timer_info_get_resolution(timerInfo);
            snd_timer_close(timerHandle);

            //RG_DEBUG << "generateTimerList(): adding timer: " << info.name;

            m_timers.push_back(info);
        }

        snd_timer_query_close(timerQuery);
    }
}


std::string
AlsaDriver::getAutoTimer(bool &wantTimerChecks)
{
    // Look for the apparent best-choice timer.

    if (m_timers.empty())
        return "";

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

    bool pcmTimerAccepted = false;
    wantTimerChecks = false; // for most options

    bool rtcCouldBeOK = false;

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
        wantTimerChecks = true;
        pcmTimerAccepted = true;
    }
#endif

    // look for a high frequency system timer

    for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
         i != m_timers.end(); ++i) {
        if (i->sclas != SND_TIMER_SCLASS_NONE)
            continue;
        if (i->clas == SND_TIMER_CLASS_GLOBAL) {
            if (i->device == SND_TIMER_GLOBAL_SYSTEM) {
                long hz = 1000000000 / i->resolution;
                if (hz >= 750) {
                    return i->name;
                }
            }
        }
    }

    // Look for the system RTC timer if available.  This has been
    // known to hang some real-time kernels, but reports suggest that
    // recent kernels are OK.  Avoid if the kernel is older than
    // 2.6.20 or the ALSA driver is older than 1.0.14.

    if (versionIsAtLeast(getAlsaModuleVersionString(),
                         1, 0, 14) &&
        versionIsAtLeast(getKernelVersionString(),
                         2, 6, 20)) {

        rtcCouldBeOK = true;

        for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
             i != m_timers.end(); ++i) {
            if (i->sclas != SND_TIMER_SCLASS_NONE) continue;
            if (i->clas == SND_TIMER_CLASS_GLOBAL) {
                if (i->device == SND_TIMER_GLOBAL_RTC) {
                    return i->name;
                }
            }
        }
    }

    // look for the first PCM playback timer; that's all we know about
    // for now (until JACK becomes able to tell us which PCM it's on)

    if (pcmTimerAccepted) {

        for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
             i != m_timers.end(); ++i) {
            if (i->sclas != SND_TIMER_SCLASS_NONE)
                continue;
            if (i->clas == SND_TIMER_CLASS_PCM) {
                if (i->resolution != 0) {
                    long hz = 1000000000 / i->resolution;
                    if (hz >= 750) {
                        wantTimerChecks = false; // pointless with PCM timer
                        return i->name;
                    } else {
                        AUDIT << "PCM timer: inadequate resolution " << i->resolution << '\n';
                        RG_DEBUG << "getAutoTimer(): PCM timer: inadequate resolution " << i->resolution;
                    }
                }
            }
        }
    }

    // next look for slow, unpopular 100Hz (2.4) or 250Hz (2.6) system timer

    for (std::vector<AlsaTimerInfo>::iterator i = m_timers.begin();
         i != m_timers.end(); ++i) {
        if (i->sclas != SND_TIMER_SCLASS_NONE)
            continue;
        if (i->clas == SND_TIMER_CLASS_GLOBAL) {
            if (i->device == SND_TIMER_GLOBAL_SYSTEM) {
                AUDIT << "Using low-resolution system timer, sending a warning" << '\n';
                RG_DEBUG << "getAutoTimer(): Using low-resolution system timer, sending a warning";
                if (rtcCouldBeOK) {
                    reportFailure(MappedEvent::WarningImpreciseTimerTryRTC);
                } else {
                    reportFailure(MappedEvent::WarningImpreciseTimer);
                }
                return i->name;
            }
        }
    }

    // falling back to something that almost certainly won't work,
    // if for any reason all of the above failed

    return m_timers.begin()->name;
}



/* generatePortList: called from initialiseMidi and
 * checkForNewClients.  This just polls ALSA ports and should continue
 * to be called regularly. */

void
AlsaDriver::generatePortList(AlsaPortVector *newPorts)
{
    AlsaPortVector alsaPorts;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int client;
    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    AUDIT << '\n';
    AUDIT << "  ALSA Client information:\n";
    AUDIT << '\n';
    RG_DEBUG << "generatePortList(): ALSA Client information:";

    // Get only the client ports we're interested in and store them
    // for sorting and then device creation.
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0) {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0)
            continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0) {

            int client = snd_seq_port_info_get_client(pinfo);
            int port = snd_seq_port_info_get_port(pinfo);
            unsigned int clientType = snd_seq_client_info_get_type(cinfo);
            unsigned int portType = snd_seq_port_info_get_type(pinfo);
            unsigned int capability = snd_seq_port_info_get_capability(pinfo);

            if ((((capability & writeCap) == writeCap) ||
                 ((capability & readCap) == readCap)) &&
                ((capability & SND_SEQ_PORT_CAP_NO_EXPORT) == 0)) {
                AUDIT << "    "
                      << client << ","
                      << port << " - ("
                      << snd_seq_client_info_get_name(cinfo) << ", "
                      << snd_seq_port_info_get_name(pinfo) << ")";
                RG_DEBUG << "    "
                      << client << ","
                      << port << " - ("
                      << snd_seq_client_info_get_name(cinfo) << ", "
                      << snd_seq_port_info_get_name(pinfo) << ")";

                PortDirection direction;

                if ((capability & SND_SEQ_PORT_CAP_DUPLEX) ||
                    ((capability & SND_SEQ_PORT_CAP_WRITE) &&
                     (capability & SND_SEQ_PORT_CAP_READ))) {
                    direction = Duplex;
                    AUDIT << "\t\t\t(DUPLEX)";
                    RG_DEBUG << "        (DUPLEX)";
                } else if (capability & SND_SEQ_PORT_CAP_WRITE) {
                    direction = WriteOnly;
                    AUDIT << "\t\t(WRITE ONLY)";
                    RG_DEBUG << "        (WRITE ONLY)";
                } else {
                    direction = ReadOnly;
                    AUDIT << "\t\t(READ ONLY)";
                    RG_DEBUG << "        (READ ONLY)";
                }

                AUDIT << " [ctype " << clientType << ", ptype " << portType << ", cap " << capability << "]";
                RG_DEBUG << "        [ctype " << clientType << ", ptype " << portType << ", cap " << capability << "]";

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
                if (firstSpace < 0)
                    firstSpace = int(fullClientName.length());

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
                if (name.length() > 35)
                    name = portId + fullPortName;

                if (direction == WriteOnly) {
                    name += " (write)";
                } else if (direction == ReadOnly) {
                    name += " (read)";
                } else if (direction == Duplex) {
                    name += " (duplex)";
                }

                QSharedPointer<AlsaPortDescription> portDescription(
                    new AlsaPortDescription(
                                            Instrument::Midi,
                                            name,
                                            client,
                                            port,
                                            clientType,
                                            portType,
                                            capability,
                                            direction));

                if (newPorts &&
                    (getPortName(ClientPortPair(client, port)) == "")) {
                    newPorts->push_back(portDescription);
                }

                alsaPorts.push_back(portDescription);

                AUDIT << '\n';
            }
        }
    }

    AUDIT << '\n';

    // Ok now sort by duplexicity
    //
    std::sort(alsaPorts.begin(), alsaPorts.end(), AlsaPortCmp());
    m_alsaPorts = alsaPorts;
}


void
AlsaDriver::generateFixedInstruments()
{
    // Create a number of soft synth Instruments
    //
    MappedInstrument *instr;
    char number[100];
    InstrumentId first;
    int count;
    getSoftSynthInstrumentNumbers(first, count);

    // soft-synth device takes id to match first soft-synth instrument
    // number, for easy identification & consistency with GUI
    DeviceId ssiDeviceId = first;

    for (int i = 0; i < count; ++i) {
        sprintf(number, " #%d", i + 1);
        std::string name = QObject::tr("Synth plugin").toStdString() + std::string(number);
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

    // Create a number of audio Instruments - these are just
    // logical Instruments anyway and so we can create as
    // many as we like and then use them for Tracks.
    //
    // Note that unlike in earlier versions of Rosegarden, we always
    // have exactly one soft synth device and one audio device (even
    // if audio output is not actually working, the device is still
    // present).
    //
    std::string audioName;
    getAudioInstrumentNumbers(first, count);

    // audio device takes id to match first audio instrument
    // number, for easy identification & consistency with GUI
    DeviceId audioDeviceId = first;

    for (int i = 0; i < count; ++i) {
        sprintf(number, " #%d", i + 1);
        audioName = QObject::tr("Audio").toStdString() + std::string(number);
        instr = new MappedInstrument(Instrument::Audio,
                                     i,
                                     first + i,
                                     audioName,
                                     audioDeviceId);
        m_instruments.push_back(instr);
    
        // Create a fader with a matching id - this is the starting
        // point for all audio faders.
        //
        m_studio->createObject(MappedObject::AudioFader, first + i);
    }

    // Create audio device
    //
    device =
        new MappedDevice(audioDeviceId,
                         Device::Audio,
                         "Audio",
                         "Audio connection");
    m_devices.push_back(device);
}

MappedDevice *
AlsaDriver::createMidiDevice(DeviceId deviceId,
                             MidiDevice::DeviceDirection reqDirection)
{
    std::string connectionName = "";
    const char *deviceName = "unnamed";

    if (reqDirection == MidiDevice::Play) {

        QString portName = QString("out %1 - %2")
            .arg(m_outputPorts.size() + 1)
            .arg(deviceName);

        int outputPort = checkAlsaError(snd_seq_create_simple_port
                                        (m_midiHandle,
                                         portName.toLocal8Bit(),
                                         SND_SEQ_PORT_CAP_READ |
                                         SND_SEQ_PORT_CAP_SUBS_READ,
                                         SND_SEQ_PORT_TYPE_APPLICATION |
                                         SND_SEQ_PORT_TYPE_SOFTWARE |
                                         SND_SEQ_PORT_TYPE_MIDI_GENERIC),
                                        "createMidiDevice - can't create output port");

        if (outputPort >= 0) {

            RG_DEBUG << "createMidiDevice(): CREATED OUTPUT PORT " << outputPort << ":" << portName << " for device " << deviceId;

            m_outputPorts[deviceId] = outputPort;
        }
    }

    MappedDevice *device = new MappedDevice(deviceId,
                                            Device::Midi,
                                            deviceName,
                                            connectionName);
    device->setDirection(reqDirection);
    return device;
}

void
AlsaDriver::addInstrumentsForDevice(MappedDevice *device, InstrumentId base)
{
    std::string channelName;
    char number[100];

    for (int channel = 0; channel < 16; ++channel) {

        // name is just number, derive rest from device at gui
        sprintf(number, "#%d", channel + 1);
        channelName = std::string(number);

        if (channel == 9) channelName = std::string("#10[D]");

        MappedInstrument *instr = new MappedInstrument
            (Instrument::Midi, channel, base++, channelName, device->getId());
        m_instruments.push_back(instr);
    }
}

bool
AlsaDriver::canReconnect(Device::DeviceType type)
{
    return (type == Device::Midi);
}

void
AlsaDriver::clearDevices()
{
    for (size_t i = 0; i < m_instruments.size(); ++i) {
        delete m_instruments[i];
    }
    m_instruments.clear();

    for (size_t i = 0; i < m_devices.size(); ++i) {
        delete m_devices[i];
    }
    m_devices.clear();

    m_devicePortMap.clear();
}

bool
AlsaDriver::addDevice(Device::DeviceType type,
                      DeviceId deviceId,
                      InstrumentId baseInstrumentId,
                      MidiDevice::DeviceDirection direction)
{
    RG_DEBUG << "addDevice(" << type << "," << direction << ")";

    if (type == Device::Midi) {

        MappedDevice *device = createMidiDevice(deviceId, direction);
        if (!device) {
            RG_WARNING << "addDevice(): WARNING: Device creation failed, type: " << type << " deviceId: " << deviceId << " baseInstrumentId: " << baseInstrumentId << " direction: " << direction;
        } else {
            addInstrumentsForDevice(device, baseInstrumentId);
            m_devices.push_back(device);

            if (direction == MidiDevice::Record) {
                setRecordDevice(device->getId(), true);
            }

            return true;
        }
    }

    return false;
}

void
AlsaDriver::removeDevice(DeviceId id)
{
    DeviceIntMap::iterator i1 = m_outputPorts.find(id);
    if (i1 == m_outputPorts.end()) {
        RG_WARNING << "removeDevice(): WARNING: Cannot find device " << id << " in port map";
        return ;
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
}

void
AlsaDriver::removeAllDevices()
{
    while (!m_outputPorts.empty()) {
        checkAlsaError(snd_seq_delete_port(m_midiHandle,
                                           m_outputPorts.begin()->second),
                       "removeAllDevices");
        m_outputPorts.erase(m_outputPorts.begin());
    }

    clearDevices();
}

void
AlsaDriver::renameDevice(DeviceId id, QString name)
{
    DeviceIntMap::iterator i = m_outputPorts.find(id);
    if (i == m_outputPorts.end()) {
        RG_WARNING << "renameDevice(): WARNING: Cannot find device " << id << " in port map";
        return ;
    }

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_port_info(m_midiHandle, i->second, pinfo);

    QString oldName = snd_seq_port_info_get_name(pinfo);
    int sep = oldName.indexOf(" - ");

    QString newName;
    if (sep < 0) {
        newName = oldName + " - " + name;
    } else {
        newName = oldName.left(sep + 3) + name;
    }

    snd_seq_port_info_set_name(pinfo, newName.toLocal8Bit().data());
    checkAlsaError(snd_seq_set_port_info(m_midiHandle, i->second, pinfo),
                   "renameDevice");

    for (size_t i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i]->getId() == id) {
            m_devices[i]->setName(qstrtostr(newName));
            break;
        }
    }

    RG_DEBUG << "renameDevice(): Renamed " << m_client << ":" << i->second << " to " << name;
}

ClientPortPair
AlsaDriver::getPortByName(std::string name)
{
    AUDIT << "AlsaDriver::getPortByName(\"" << name << "\")\n";
    RG_DEBUG << "getPortByName(" << name << ")";

    // For each ALSA port...
    for (size_t i = 0; i < m_alsaPorts.size(); ++i) {
        //AUDIT << "  Comparing\n";
        //AUDIT << "    \"" << name << "\" with\n";
        //AUDIT << "    \"" << m_alsaPorts[i]->m_name << "\"\n";
        //RG_DEBUG << "  Comparing" << name << "with";
        //RG_DEBUG << "           " << m_alsaPorts[i]->m_name;

        // ??? This comparison is unrealistic.  Client numbers can
        //     change.  We should probably remove the client number from
        //     the front of each before doing the comparison.

        if (m_alsaPorts[i]->m_name == name)
            return ClientPortPair(m_alsaPorts[i]->m_client,
                                  m_alsaPorts[i]->m_port);
    }

    return ClientPortPair(-1, -1);
}

std::string
AlsaDriver::getPortName(ClientPortPair port)
{
    for (size_t i = 0; i < m_alsaPorts.size(); ++i) {
        if (m_alsaPorts[i]->m_client == port.client &&
            m_alsaPorts[i]->m_port == port.port) {
            return m_alsaPorts[i]->m_name;
        }
    }
    return "";
}

unsigned int
AlsaDriver::getConnections(Device::DeviceType type,
                           MidiDevice::DeviceDirection direction)
{
    if (type != Device::Midi)
        return 0;

    int count = 0;
    for (size_t j = 0; j < m_alsaPorts.size(); ++j) {
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
    if (type != Device::Midi)
        return "";

    AlsaPortVector tempList;
    for (size_t j = 0; j < m_alsaPorts.size(); ++j) {
        if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
            (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
            tempList.push_back(m_alsaPorts[j]);
        }
    }

    if (connectionNo < (unsigned int)tempList.size()) {
        return strtoqstr(tempList[connectionNo]->m_name);
    }

    return "";
}

QString
AlsaDriver::getConnection(DeviceId id)
{
    if (m_devicePortMap.find(id) == m_devicePortMap.end()) return "";
    const ClientPortPair &pair = m_devicePortMap[id];
    return getPortName(pair).c_str();
}

void
AlsaDriver::setConnectionToDevice(MappedDevice &device, QString connection)
{
    ClientPortPair pair( -1, -1);
    if (connection != "") {
        pair = getPortByName(qstrtostr(connection));
    }
    setConnectionToDevice(device, connection, pair);
}

void
AlsaDriver::setConnectionToDevice(MappedDevice &device, QString connection,
                                  const ClientPortPair &pair)
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "setConnectionToDevice(): connection " << connection;
#endif

    if (device.getDirection() == MidiDevice::Record) {
        // disconnect first
        setRecordDevice(device.getId(), false);
    }

    m_devicePortMap[device.getId()] = pair;

    QString prevConnection = strtoqstr(device.getConnection());
    device.setConnection(qstrtostr(connection));

    if (device.getDirection() == MidiDevice::Play) {

        DeviceIntMap::iterator j = m_outputPorts.find(device.getId());

        if (j != m_outputPorts.end()) {

            if (prevConnection != "") {
                ClientPortPair prevPair = getPortByName(qstrtostr(prevConnection));
                if (prevPair.client >= 0 && prevPair.port >= 0) {

                    RG_DEBUG << "setConnectionToDevice(): Disconnecting my port " << j->second << " from " << prevPair.client << ":" << prevPair.port << " on reconnection";
                    snd_seq_disconnect_to(m_midiHandle,
                                          j->second,
                                          prevPair.client,
                                          prevPair.port);

                    if (m_midiSyncAutoConnect) {
                        bool foundElsewhere = false;
                        for (MappedDeviceList::iterator k = m_devices.begin();
                             k != m_devices.end(); ++k) {
                            if ((*k)->getId() != device.getId()) {
                                if ((*k)->getConnection() ==
                                    qstrtostr(prevConnection)) {
                                    foundElsewhere = true;
                                    break;
                                }
                            }
                        }
                        if (!foundElsewhere) {
                            snd_seq_disconnect_to(m_midiHandle,
                                                  m_syncOutputPort,
                                                  pair.client,
                                                  pair.port);
                        }
                    }
                }
            }

            if (pair.client >= 0  &&  pair.port >= 0) {
                RG_DEBUG << "setConnectionToDevice(): Connecting my port " << j->second << " to " << pair.client << ":" << pair.port << " on reconnection";
                snd_seq_connect_to(m_midiHandle,
                                   j->second,
                                   pair.client,
                                   pair.port);
                if (m_midiSyncAutoConnect) {
                    snd_seq_connect_to(m_midiHandle,
                                       m_syncOutputPort,
                                       pair.client,
                                       pair.port);
                }
            }
        }
    } else { // record device: reconnect

        setRecordDevice(device.getId(), true);
    }
}

void
AlsaDriver::setConnection(DeviceId id, QString connection)
{
    ClientPortPair port(getPortByName(qstrtostr(connection)));

#ifdef DEBUG_ALSA
    RG_DEBUG << "setConnection(" << id << "," << connection << ")";
#endif

    if ((connection == "") || (port.client != -1 && port.port != -1)) {

#ifdef DEBUG_ALSA
        if (connection == "") {
            RG_DEBUG << "setConnection(): empty connection, disconnecting";
        } else {
            RG_DEBUG << "setConnection(): found port";
        }
#endif

        for (size_t i = 0; i < m_devices.size(); ++i) {

            if (m_devices[i]->getId() == id) {
#ifdef DEBUG_ALSA
                RG_DEBUG << "setConnection(): and found device -- connecting";
#endif

                setConnectionToDevice(*m_devices[i], connection, port);
                return;
            }
        }
    }
#ifdef DEBUG_ALSA
    RG_DEBUG << "setConnection(): port or device not found";
#endif
}

void
AlsaDriver::setPlausibleConnection(
        DeviceId deviceId, QString idealConnection, bool recordDevice)
{
    AUDIT << "----------\n";
    AUDIT << "AlsaDriver::setPlausibleConnection()\n";
    AUDIT << "  Connection like \"" << idealConnection << "\" requested for device " << deviceId << '\n';
    RG_DEBUG << "----------";
    RG_DEBUG << "setPlausibleConnection()";
    RG_DEBUG << "  Connection like" << idealConnection << "requested for device " << deviceId;

    if (idealConnection != "") {

        ClientPortPair port(getPortByName(qstrtostr(idealConnection)));

        AUDIT << "AlsaDriver::setPlausibleConnection(): getPortByName(\"" << idealConnection << "\") returned " << port.client << ":" << port.port << '\n';
        RG_DEBUG << "setPlausibleConnection(): getPortByName(" << idealConnection << ") returned " << port.client << ":" << port.port;

        // If a client and port were found...
        if (port.client != -1  &&  port.port != -1) {

            // Find the device and make the connection.
            // ??? We should separate connecting from the port matching.
            //     Rename this routine to findPlausibleConnection() and have
            //     it return what we need to make the connection.  Probably
            //     the port.  Maybe findPlausiblePort()?  This would make it
            //     easier to unit test this process.
            for (size_t i = 0; i < m_devices.size(); ++i) {
                if (m_devices[i]->getId() == deviceId) {
                    setConnectionToDevice(*m_devices[i], idealConnection, port);
                    break;
                }
            }

            AUDIT << "AlsaDriver::setPlausibleConnection(): exact match available\n";
            RG_DEBUG << "setPlausibleConnection(): exact match available";

            return;

        }

    }

    // Inexact Matching

    AUDIT << "AlsaDriver::setPlausibleConnection(): Performing inexact matching...\n";
    RG_DEBUG << "setPlausibleConnection(): Performing inexact matching...";

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
    //
    // If the idealConnection string is empty, then we should pick the
    // first available connection that looks "nice and safe"; in
    // practice this means the first software device (don't
    // auto-connect to hardware unless there's utterly nothing else).

    int client = -1;
    int portNo = -1;
    QString portName;

    if (idealConnection != "") {

        // Extract the client, the number prior to the first colon.
        int colon = idealConnection.indexOf(":");
        if (colon >= 0)
            client = idealConnection.leftRef(colon).toInt();

        // If the client number was found...
        if (client > 0) {
            // Extract the port, the number after the first colon.
            QString remainder = idealConnection.mid(colon + 1);
            int space = remainder.indexOf(" ");
            if (space >= 0)
                portNo = remainder.leftRef(space).toInt();
        }

        // Extract the port name.

        // Port name starts after the first space.
        int firstSpace = idealConnection.indexOf(" ");
        // Port name ends at the first character that is not a word character
        // (\w), hyphen, or a space.  This strips off the "(write)" and
        // anything else.
        // ??? Would it make more sense to just search for a left paren?
        int endOfText =
                idealConnection.indexOf(QRegExp("[^\\w- ]"), firstSpace + 1);

        // No end point found?  Just take the whole thing.
        if (endOfText < 2)
            portName = idealConnection.mid(firstSpace + 1);
        else  // End point found, take only up to the end point.
            portName = idealConnection.mid(firstSpace + 1, endOfText - firstSpace - 2);
    }

    AUDIT << "AlsaDriver::setPlausibleConnection()\n";
    AUDIT << "  client: " << client << '\n';
    AUDIT << "  portNo: " << portNo << '\n';
    AUDIT << "  portName: " << portName << '\n';
    RG_DEBUG << "setPlausibleConnection()";
    RG_DEBUG << "  client: " << client;
    RG_DEBUG << "  portNo: " << portNo;
    RG_DEBUG << "  portName: " << portName;

    // If we are connecting to the system client, bail.
    // ??? This should never happen.
    if (client == 0) {
        RG_WARNING << "setPlausibleConnection(): WARNING: Cannot connect to the system client.";
        return;
    }

    QSharedPointer<AlsaPortDescription> viableHardwarePort;
    QSharedPointer<AlsaPortDescription> viableSoftwarePort;

    // Try to find one viable hardware and one viable software port, if
    // possible.  Use software preferentially.  Iterate through everything
    // until we've exhausted all possibilities for collecting one of each,
    // then sort it out afterwards.

    // ??? Combinations are interesting, but I think we can do better.
    //     If we can do two passes, we can look for the best matches, then
    //     connect them in order from best to worst.  Focus on the port names
    //     and port numbers.  Do record/playback separately.  Use Levenshtein
    //     Distance for port name fuzzy comparison.

    // Check whether the port is used...
    for (int testUsed = 1; testUsed >= 0; --testUsed) {

        // Check whether the client:port numbers match...
        for (int testNumbers = 1; testNumbers >= 0; --testNumbers) {

            // Check whether the port name matches...
            for (int testName = 1; testName >= 0; --testName) {

                // For each ALSA port...
                for (size_t i = 0; i < m_alsaPorts.size(); ++i) {

                    QSharedPointer<AlsaPortDescription> port = m_alsaPorts[i];

                    // If system port, never use.
                    // ??? I bet we never see this because it is already
                    //     filtered out of m_alsaPorts.
                    if (port->m_client == 0) {
                        RG_DEBUG << "setPlausibleConnection(): Found client 0.  Skipping.";
                        continue;
                    }

                    // If there was a client in the idealConnection and the
                    // client classes do not match, try the next.
                    if (client != -1  &&
                        getClass(port->m_client) != getClass(client))
                        continue;

                    // If we're looking for a record device and this port
                    // isn't readable, skip it.
                    // This logic is tacked onto a
                    // function originally written only to consider playback
                    // devices, but I think this will work.  If we skip
                    // play-only devices here, the rest of the logic should
                    // net us a software record device preferentially,
                    // falling back on hardware.  That should catch, e.g.
                    // VMPK first.  The user wouldn't need VMPK if they had
                    // a working hardware keyboard, so this seems sound.
                    if (recordDevice  &&  !port->isReadable())
                        continue;

                    if (testNumbers) {
                        if (port->m_port != portNo)
                            continue;
                    }

                    // If we're testing the name, and the name we are looking
                    // for isn't in the ALSA port name, skip.
                    if (testName  &&  portName != ""  &&
                        !strtoqstr(port->m_name).contains(portName))
                        continue;

                    // If we are testing for used, and we aren't looking
                    // for a record device...
                    // a little hack here...  if this is a record
                    // device, we don't really care if it's already
                    // used or not (it might be used by a play
                    // device, and if more than one record device
                    // uses the same port, it doesn't have any
                    // particularly dire consequences)
                    if (testUsed  &&  !recordDevice) {
                        if (portInUse(port->m_client, port->m_port))
                            continue;
                    }

                    // Don't connect to any of our own ports per
                    // default!  Note that our client name is set
                    // to "rosegarden" in initialiseMidi -- this
                    // string is not translated, and we'd have to
                    // change this if ever it were.  We don't have
                    // a capital R, but let's omit the R from the
                    // test just in case...
                    // ??? This appears to be unnecessary.  The rosegarden
                    //     ports never appear in m_alsaPorts.
                    if (idealConnection == ""  &&
                        strtoqstr(port->m_name).contains("osegarden")) {
                        RG_DEBUG << "setPlausibleConnection(): Found rosegarden port:" << port->m_name;
                        continue;
                    }

                    // OK, this port will do.

                    // If hardware or internal port...
                    if (getClass(port->m_client) == Hardware  ||
                        getClass(port->m_client) == Internal) {
                        // If we don't have a hardware port yet...
                        if (!viableHardwarePort) {
                            // we already filter out all play-only ports if
                            // recordDevice is true, so we only need special
                            // handling if it's false
                            if ((!recordDevice  &&  port->isWriteable())  ||
                                recordDevice) {
                                viableHardwarePort = port;

                                AUDIT << "AlsaDriver::setPlausibleConnection(): found hardware port: \"" << viableHardwarePort->m_name << "\"\n";
                                RG_DEBUG << "setPlausibleConnection(): found hardware port:" << viableHardwarePort->m_name;
                            }
                        }
                    } else {  // Software port
                        // If we don't have a software port yet...
                        if (!viableSoftwarePort) {
                            if ((!recordDevice  &&  port->isWriteable())  ||
                                recordDevice) {
                                viableSoftwarePort = port;

                                AUDIT << "AlsaDriver::setPlausibleConnection(): found software port: \"" << viableSoftwarePort->m_name << "\"\n";
                                RG_DEBUG << "setPlausibleConnection(): found software port:" << viableSoftwarePort->m_name;
                            }
                        }
                    }
                }
            }
        }
    }

    // If we found a viable software port, use it.  If we didn't find a viable
    // software port, but did find a viable hardware port, use it. 
    QSharedPointer<AlsaPortDescription> port;

    if (viableSoftwarePort)
        port = viableSoftwarePort;
    else if (viableHardwarePort)
        port = viableHardwarePort;

    if (port) {
        for (size_t j = 0; j < m_devices.size(); ++j) {

            if (m_devices[j]->getId() == deviceId) {
                setConnectionToDevice(*m_devices[j],
                                      strtoqstr(port->m_name),
                                      ClientPortPair(port->m_client, port->m_port));

                // in this case we don't request a device resync,
                // because this is only invoked at times such as
                // file load when the GUI is well aware that the
                // whole situation is in upheaval anyway

                return ;
            }
        }
    } else {
        AUDIT << "AlsaDriver::setPlausibleConnection(): nothing suitable available\n";
        RG_DEBUG << "setPlausibleConnection(): nothing suitable available";
    }
}


void
AlsaDriver::connectSomething()
{
    // Called after document load, if there are devices in the document but none
    // of them has managed to get itself connected to anything.  Tries to find
    // something suitable to connect one play, and one record device to, and
    // connects it.  If nothing very appropriate beckons, leaves unconnected.

    MappedDevice *toConnect = nullptr;
    MappedDevice *toConnectRecord = nullptr;

    // Check whether anything is connected.

    // For each device
    for (size_t i = 0; i < m_devices.size(); ++i) {
        MappedDevice *device = m_devices[i];

        // If something is connected, bail.
        // ??? We need a getPairForDevice(DeviceId) so we can code this as:
        //     if (getPairForDevice(device->getId()) != ClientPortPair(-1,-1))
        if (m_devicePortMap.find(device->getId()) != m_devicePortMap.end()  &&
            m_devicePortMap[device->getId()] != ClientPortPair()  &&
            m_devicePortMap[device->getId()] != ClientPortPair(-1,-1)) {
            return;
        }

        if (device->getDirection() == MidiDevice::Play  &&  !toConnect)
            toConnect = device;

        if (device->getDirection() == MidiDevice::Record  &&  !toConnectRecord)
            toConnectRecord = device;
    }            

    // If the studio was absolutely empty, we'll make it to here with
    // toConnect and toConnectRecord still null, so in that case we'll
    // simply move along without doing anything.

    if (toConnect)
        setPlausibleConnection(toConnect->getId(), "");

    if (toConnectRecord) {
        bool recordDevice = true;
        setPlausibleConnection(toConnectRecord->getId(), "", recordDevice);
    }
}

void
AlsaDriver::checkTimerSync(size_t frames)
{
    if (!m_doTimerChecks)
        return ;

#ifdef HAVE_LIBJACK

    if (!m_jackDriver || !m_queueRunning || frames == 0 ||
        (getMTCStatus() == TRANSPORT_FOLLOWER)) {
        m_firstTimerCheck = true;
        return ;
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
        return ;
    }

    RealTime jackDiff = RealTime::frame2RealTime
        (nowJackFrames - startJackFrames,
         m_jackDriver->getSampleRate());

    RealTime alsaDiff = nowAlsaTime - startAlsaTime;

    if (alsaDiff > RealTime(10, 0)) {

#ifdef DEBUG_ALSA
        if (!m_playing) {
            RG_DEBUG << "checkTimerSync(): ALSA:" << startAlsaTime << "\t->" << nowAlsaTime << "\nJACK: " << startJackFrames << "\t\t-> " << nowJackFrames;
            RG_DEBUG << "checkTimerSync(): ALSA diff:  " << alsaDiff << "\nJACK diff:  " << jackDiff;
        }
#endif

        double ratio = (jackDiff - alsaDiff) / alsaDiff;

        if (fabs(ratio) > 0.1) {
#ifdef DEBUG_ALSA
            if (!m_playing) {
                RG_DEBUG << "checkTimerSync(): Ignoring excessive ratio " << ratio << ", hoping for a more likely result next time";
            }
#endif

        } else if (fabs(ratio) > 0.000001) {

#ifdef DEBUG_ALSA
            if (alsaDiff > RealTime::zeroTime && jackDiff > RealTime::zeroTime) {
                if (!m_playing) {
                    if (jackDiff < alsaDiff) {
                        RG_DEBUG << "checkTimerSync(): <<<< ALSA timer is faster by " << 100.0 * ((alsaDiff - jackDiff) / alsaDiff) << "% (1/" << int(1.0 / ratio) << ")";
                    } else {
                        RG_DEBUG << "checkTimerSync(): >>>> JACK timer is faster by " << 100.0 * ((jackDiff - alsaDiff) / alsaDiff) << "% (1/" << int(1.0 / ratio) << ")";
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
    return (unsigned int)m_timers.size() + 1; // one extra for auto
}

QString
AlsaDriver::getTimer(unsigned int n)
{
    if (n == 0)
        return AUTO_TIMER_NAME;
    else
        return strtoqstr(m_timers[n -1].name);
}

QString
AlsaDriver::getCurrentTimer()
{
    return strtoqstr(m_currentTimer);
}

void
AlsaDriver::setCurrentTimer(QString timer)
{
    QSettings settings;
    bool skip = settings.value("ALSA/SkipSetCurrentTimer", false).toBool();
    // Write back out so we can find it.
    settings.setValue("ALSA/SkipSetCurrentTimer", skip);
    if (skip)
        return;

    if (timer == getCurrentTimer())
        return ;

    RG_DEBUG << "setCurrentTimer(" << timer << ")";

    std::string name(qstrtostr(timer));

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
    checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, nullptr), "setCurrentTimer(): stopping queue");
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

    for (size_t i = 0; i < m_timers.size(); ++i) {
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
                AUDIT << "    Current timer set to \"" << name << "\" with timer checks\n";
                RG_DEBUG << "setCurrentTimer(): Current timer set to \"" << name << "\" with timer checks";
            } else {
                AUDIT << "    Current timer set to \"" << name << "\"\n";
                RG_DEBUG << "setCurrentTimer(): Current timer set to \"" << name << "\"";
            }

            if (m_timers[i].clas == SND_TIMER_CLASS_GLOBAL &&
                m_timers[i].device == SND_TIMER_GLOBAL_SYSTEM) {
                long hz = 1000000000 / m_timers[i].resolution;
                if (hz < 900) {
                    AUDIT << "    WARNING: using system timer with only " << hz << "Hz resolution!\n";
                    RG_WARNING << "setCurrentTimer(): WARNING: using system timer with only " << hz << "Hz resolution!";
                }
            }

            break;
        }
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver)
        m_jackDriver->prebufferAudio();
#endif

    checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, nullptr), "checkAlsaError(): continue queue");
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "setCurrentTimer(): draining output to continue queue");
    m_queueRunning = true;

    m_firstTimerCheck = true;
}

bool
AlsaDriver::initialise()
{
    bool result = true;

    initialiseAudio();
    result = initialiseMidi();

    return result;
}



// Set up queue, client and port
//
bool
AlsaDriver::initialiseMidi()
{
    // Create a non-blocking handle.
    //
    if (snd_seq_open(&m_midiHandle,
                     "default",
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0) {
        AUDIT << "AlsaDriver::initialiseMidi() - "
              << "couldn't open sequencer - " << snd_strerror(errno)
              << " - perhaps you need to modprobe snd-seq-midi.\n";
        RG_WARNING << "initialiseMidi(): WARNING: couldn't open sequencer - "
                   << snd_strerror(errno)
                   << " - perhaps you need to modprobe snd-seq-midi.";
        reportFailure(MappedEvent::FailureALSACallFailed);
        return false;
    }

    // Set the client name.  Note that we depend on knowing this name
    // elsewhere, e.g. in setPlausibleConnection below.  If it is ever
    // changed, we may have to check for other occurrences
    // 
    snd_seq_set_client_name(m_midiHandle, "rosegarden");

    if ((m_client = snd_seq_client_id(m_midiHandle)) < 0) {
        RG_WARNING << "initialiseMidi(): WARNING: Can't create client";
        return false;
    }

    // Create a queue
    //
    if ((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
                                             "Rosegarden queue")) < 0) {
        RG_WARNING << "initialiseMidi(): WARNING: Can't allocate queue";
        return false;
    }

    // Create the input port
    //
    snd_seq_port_info_t *pinfo;

    snd_seq_port_info_alloca(&pinfo);
    snd_seq_port_info_set_capability(pinfo,
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE );
    snd_seq_port_info_set_type(pinfo, SND_SEQ_PORT_TYPE_APPLICATION |
                               SND_SEQ_PORT_TYPE_SOFTWARE |
                               SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    snd_seq_port_info_set_midi_channels(pinfo, 16);
    /* we want to know when the events got delivered to us */
    snd_seq_port_info_set_timestamping(pinfo, 1);
    snd_seq_port_info_set_timestamp_real(pinfo, 1);
    snd_seq_port_info_set_timestamp_queue(pinfo, m_queue);
    snd_seq_port_info_set_name(pinfo, "record in");

    if (checkAlsaError(snd_seq_create_port(m_midiHandle, pinfo),
                       "initialiseMidi - can't create input port") < 0)
        return false;
    m_inputPort = snd_seq_port_info_get_port(pinfo);

    // Subscribe the input port to the ALSA Announce port
    // to receive notifications when clients, ports and subscriptions change
    snd_seq_connect_from( m_midiHandle, m_inputPort,
                          SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE );

    m_midiInputPortConnected = true;

#define POOL_DEBUG 0

#if POOL_DEBUG
    snd_seq_client_pool_t *poolInfo;
    snd_seq_client_pool_malloc(&poolInfo);
    snd_seq_get_client_pool(m_midiHandle, poolInfo);
    RG_DEBUG << "initialiseMidi() before setting pool sizes...";
    RG_DEBUG << "  input pool: " << snd_seq_client_pool_get_input_pool(poolInfo);
    RG_DEBUG << "  output pool: " << snd_seq_client_pool_get_output_pool(poolInfo);
    RG_DEBUG << "  output room: " << snd_seq_client_pool_get_output_room(poolInfo);
#endif

    // Increase memory pool size.
    // Normally these are 200, 500, and 0 respectively.

    // valgrind shows errors here.  They appear to be harmless.

    if (checkAlsaError(snd_seq_set_client_pool_input(m_midiHandle, 2000),
                       "AlsaDriver::initialiseMidi(): can't set input pool size") < 0)
        return false;

    if (checkAlsaError(snd_seq_set_client_pool_output(m_midiHandle, 2000),
                       "AlsaDriver::initialiseMidi(): can't set output pool size") < 0)
        return false;

    if (checkAlsaError(snd_seq_set_client_pool_output_room(m_midiHandle, 2000),
                       "AlsaDriver::initialiseMidi(): can't set output pool room") < 0)
        return false;

#if POOL_DEBUG
    snd_seq_get_client_pool(m_midiHandle, poolInfo);
    RG_DEBUG << "initialiseMidi() after setting pool sizes...";
    RG_DEBUG << "  input pool: " << snd_seq_client_pool_get_input_pool(poolInfo);
    RG_DEBUG << "  output pool: " << snd_seq_client_pool_get_output_pool(poolInfo);
    RG_DEBUG << "  output room: " << snd_seq_client_pool_get_output_room(poolInfo);
    snd_seq_client_pool_free(poolInfo);
#endif

    // Create sync output now as well
    m_syncOutputPort = checkAlsaError(snd_seq_create_simple_port
                                      (m_midiHandle,
                                       "sync out",
                                       SND_SEQ_PORT_CAP_READ |
                                       SND_SEQ_PORT_CAP_SUBS_READ,
                                       SND_SEQ_PORT_TYPE_APPLICATION |
                                       SND_SEQ_PORT_TYPE_SOFTWARE |
                                       SND_SEQ_PORT_TYPE_MIDI_GENERIC),
                                      "initialiseMidi - can't create sync output port");

    // and port for hardware controller
    m_controllerPort = checkAlsaError(snd_seq_create_simple_port
                                      (m_midiHandle,
                                       "external controller",
                                       SND_SEQ_PORT_CAP_READ |
                                       SND_SEQ_PORT_CAP_WRITE |
                                       SND_SEQ_PORT_CAP_SUBS_READ |
                                       SND_SEQ_PORT_CAP_SUBS_WRITE,
                                       SND_SEQ_PORT_TYPE_APPLICATION |
                                       SND_SEQ_PORT_TYPE_SOFTWARE |
                                       SND_SEQ_PORT_TYPE_MIDI_GENERIC),
                                      "initialiseMidi - can't create controller port");

    getSystemInfo();

    generatePortList();
    generateFixedInstruments();

    // Modify status with MIDI success
    //
    m_driverStatus |= MIDI_OK;

    generateTimerList();
    setCurrentTimer(AUTO_TIMER_NAME);

    // Start the timer
    if (checkAlsaError(snd_seq_start_queue(m_midiHandle, m_queue, nullptr),
                       "initialiseMidi(): couldn't start queue") < 0) {
        reportFailure(MappedEvent::FailureALSACallFailed);
        return false;
    }

    m_queueRunning = true;

    // process anything pending
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "initialiseMidi(): couldn't drain output");

    AUDIT << "AlsaDriver::initialiseMidi() -  initialised MIDI subsystem\n\n";
    RG_DEBUG << "initialiseMidi() - initialised MIDI subsystem";

    return true;
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
        m_jackDriver = nullptr;
    }
#endif
}

int
AlsaDriver::songPositionPointer(const RealTime &time)
{
    // The SPP is the MIDI Beat upon which to start the song.
    // Songs are always assumed to start on a MIDI Beat of 0.  Each MIDI
    // Beat spans 6 MIDI Clocks.  In other words, each MIDI Beat is a 16th
    // note (since there are 24 MIDI Clocks in a quarter note).
    // See the MIDI spec section 2, page 27.

    // 24 MIDI clocks per quarter note.
    // ??? This does not work if there are tempo changes.  See comments
    //     on m_midiClockInterval.
    const int midiClocks = lround(time / m_midiClockInterval);

    // Convert to MIDI beats.  Round down to the previous MIDI beat to avoid
    // skipping anything.
    return midiClocks / 6;
}

void
AlsaDriver::initialisePlayback(const RealTime &position)
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "initialisePlayback() begin...";
#endif

    // now that we restart the queue at each play, the origin is always zero
    m_alsaPlayStartTime = RealTime::zeroTime;
    m_playStartPosition = position;

    m_startPlayback = true;

    m_mtcFirstTime = -1;
    m_mtcSigmaE = 0;
    m_mtcSigmaC = 0;

    if (getMMCStatus() == TRANSPORT_SOURCE) {
        sendMMC(127, MIDI_MMC_PLAY, true, "");
        m_eat_mtc = 0;
    }

    // If MIDI Sync is enabled then adjust for the MIDI Clock to
    // synchronise the sequencer with the clock.
    //
    if (getMIDISyncStatus() == TRANSPORT_SOURCE) {

        // Note: CakeWalk PA 9.03 doesn't like this.  This causes it to go out
        //       of "Waiting for MIDI sync" mode, rendering MIDI sync
        //       impossible.
        sendSystemDirect(SND_SEQ_EVENT_STOP, nullptr);

        // Send the Song Position Pointer for MIDI CLOCK positioning

        // Get time from current alsa time to start of alsa timing,
        // add the initial starting point and convert to SPP.
        // ??? It doesn't seem appropriate for ALSA time to figure into
        //     the SPP computation.  SPP is relative only to Composition
        //     time.

        // Note: CakeWalk PA 9.03 does not adhere to the MIDI spec.  It treats
        //       SPP 1 as the beginning of the song.  The spec says 0 is the
        //       beginning.

        int spp = songPositionPointer(
                getAlsaTime() - m_alsaPlayStartTime + m_playStartPosition);
        sendSystemDirect(SND_SEQ_EVENT_SONGPOS, &spp);

        // ??? The computed SPP might be significantly different from
        //     m_playStartPosition.  This will then knock us out of sync.
        //     We need to adjust m_playStartPosition to match the SPP
        //     exactly.  See Bug #1101.
        //
        //       m_playStartPosition = spp * 6 * m_midiClockInterval;
        //
        //     It's probably not safe to mess with m_playStartPosition
        //     here.  We probably need to work our way up the call chain
        //     to a safer place.
        //
        //     The workaround is to make sure you start playback on a
        //     beat.  Hold down Ctrl while positioning the playback
        //     position pointer.

        if (m_playStartPosition == RealTime::zeroTime)
            sendSystemDirect(SND_SEQ_EVENT_START, nullptr);
        else
            sendSystemDirect(SND_SEQ_EVENT_CONTINUE, nullptr);

        // A short delay is required to give the follower time to get
        // ready for the first clock.  The MIDI Spec (section 2, page 30)
        // recommends at least 1ms.  We'll go with 2ms to be safe.
        // Without this delay, follower sequencers will typically be
        // behind by one MIDI clock.

        // Shift the ALSA clock to get a 2ms delay before starting playback.
        m_alsaPlayStartTime.sec = 0;
        m_alsaPlayStartTime.nsec = 2000000;
    }

    // Since the MTC message is queued, it needs to know that
    // m_alsaPlayStartTime may have been modified by MIDI sync above.
    // So, we handle it after MIDI sync.

    if (getMTCStatus() == TRANSPORT_SOURCE) {
        // Queue up the MTC message.
        insertMTCFullFrame(position);
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
        m_needJackStart = NeedJackStart;
    }
#endif

    // Erase recent noteoffs.  There shouldn't be any, but let's be
    // extra careful.
    m_recentNoteOffs.clear();
}


void
AlsaDriver::stopPlayback()
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "stopPlayback() begin...";
#endif

    if (getMIDISyncStatus() == TRANSPORT_SOURCE) {
        sendSystemDirect(SND_SEQ_EVENT_STOP, nullptr);
    }

    if (getMMCStatus() == TRANSPORT_SOURCE) {
        sendMMC(127, MIDI_MMC_STOP, true, "");
        //<VN> need to throw away the next MTC event
        m_eat_mtc = 3;
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
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_INPUT |
                                        SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    // send sounds-off to all play devices
    //
    for (MappedDeviceList::iterator i = m_devices.begin(); i != m_devices.end(); ++i) {
        if ((*i)->getDirection() == MidiDevice::Play) {
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
    RG_DEBUG << "punchOut() begin...";
#endif

    // Flush any incomplete System Exclusive received from ALSA devices
    clearPendSysExcMap();

#ifdef HAVE_LIBJACK
    // Close any recording file
    if (m_recordStatus == RECORD_ON) {
        for (InstrumentSet::const_iterator i = m_recordingInstruments.begin();
             i != m_recordingInstruments.end(); ++i) {

            InstrumentId id = *i;

            if (id >= AudioInstrumentBase &&
                id < MidiInstrumentBase) {

                AudioFileId auid = 0;
                if (m_jackDriver && m_jackDriver->closeRecordFile(id, auid)) {

#ifdef DEBUG_ALSA
                    RG_DEBUG << "punchOut(): sending back to GUI for instrument " << id;
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
                    try {
                        MappedEvent *mE =
                            new MappedEvent(id,
                                            MappedEvent::AudioGeneratePreview,
                                            id % 256,
                                            id / 256);

                        // send completion event
                        insertMappedEventForReturn(mE);
                    } catch (...) {
                        ;
                    }
                }
            }
        }
    }
#endif

    // Change recorded state if any set
    //
    if (m_recordStatus == RECORD_ON)
        m_recordStatus = RECORD_OFF;

    m_recordingInstruments.clear();
}

void
AlsaDriver::resetPlayback(const RealTime &oldPosition, const RealTime &position)
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "resetPlayback(" << oldPosition << "," << position << ")";
#endif

    if (getMMCStatus() == TRANSPORT_SOURCE) {
        unsigned char t_sec = (unsigned char) position.sec % 60;
        unsigned char t_min = (unsigned char) (position.sec / 60) % 60;
        unsigned char t_hrs = (unsigned char) (position.sec / 3600);
#define STUPID_BROKEN_EQUIPMENT
#ifdef STUPID_BROKEN_EQUIPMENT
        // Some recorders assume you are talking in 30fps...
        unsigned char t_frm = (unsigned char) (position.nsec / 33333333U);
        unsigned char t_sbf = (unsigned char) ((position.nsec / 333333U) % 100U);
#else
        // We always send at 25fps, it's the easiest to avoid rounding problems
        unsigned char t_frm = (unsigned char) (position.nsec / 40000000U);
        unsigned char t_sbf = (unsigned char) ((position.nsec / 400000U) % 100U);
#endif

        RG_DEBUG << "resetPlayback(): Jump using MMC LOCATE to" << position;
        RG_DEBUG << "resetPlayback():   which is " << int(t_hrs) << ":" << int(t_min) << ":" << int(t_sec) << "." << int(t_frm) << "." << int(t_sbf);
        unsigned char locateDataArr[7] = {
            0x06,
            0x01,
            (unsigned char)(0x60 + t_hrs),    // (30fps flag) + hh
            t_min,         // mm
            t_sec,         // ss
            t_frm,         // frames
            t_sbf        // subframes
        };

        sendMMC(127, MIDI_MMC_LOCATE, true, std::string((const char *) locateDataArr, 7));
    }

    RealTime formerStartPosition = m_playStartPosition;

    m_playStartPosition = position;
    m_alsaPlayStartTime = getAlsaTime();

    // Reset note offs to correct positions
    //
    RealTime jump = position - oldPosition;

#ifdef DEBUG_PROCESS_MIDI_OUT
    RG_DEBUG << "resetPlayback(): Currently " << m_noteOffQueue.size() << " in note off queue";
#endif

    // modify the note offs that exist as they're relative to the
    // playStartPosition terms.
    //
    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
         i != m_noteOffQueue.end(); ++i) {

        // if we're fast forwarding then we bring the note off closer
        if (jump >= RealTime::zeroTime) {

            RealTime endTime = formerStartPosition + (*i)->getRealTime();

#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "resetPlayback(): Forward jump of " << jump << ": adjusting note off from "
                      << (*i)->getRealTime() << " (absolute " << endTime
                      << ") to:";
#endif
            (*i)->setRealTime(endTime - position);
#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "resetPlayback():     " << (*i)->getRealTime();
#endif
        } else // we're rewinding - kill the note immediately
            {
#ifdef DEBUG_PROCESS_MIDI_OUT
                RG_DEBUG << "resetPlayback(): Rewind by " << jump << ": setting note off to zero";
#endif
                (*i)->setRealTime(RealTime::zeroTime);
            }
    }

    pushRecentNoteOffs();
    processNotesOff(getAlsaTime(), true);
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "resetPlayback(): draining");

    // Ensure we clear down output queue on reset - in the case of
    // MIDI clock where we might have a long queue of events already
    // posted.
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    if (getMTCStatus() == TRANSPORT_SOURCE) {
        m_mtcFirstTime = -1;
        m_mtcSigmaE = 0;
        m_mtcSigmaC = 0;
        insertMTCFullFrame(position);
    }

#ifdef HAVE_LIBJACK
    if (m_jackDriver) {
        m_jackDriver->clearSynthPluginEvents();
        m_needJackStart = NeedJackReposition;
    }
#endif
}

void
AlsaDriver::setMIDIClockInterval(RealTime interval)
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "setMIDIClockInterval(" << interval << ")";
#endif

    if (m_midiClockInterval == interval) return;

    // Reset the value
    //
    SoundDriver::setMIDIClockInterval(interval);

    // Return if the clock isn't enabled
    //
    if (!m_midiClockEnabled)
        return ;

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
            RG_DEBUG << "setMIDIClockInterval(): MIDI CLOCK TYPE IS CONTROL";
            snd_seq_remove_events(m_midiHandle, info);
        }

}

void
AlsaDriver::clearPendSysExcMap()
{
    // Flush incomplete system exclusive events and delete the map.
    if (!m_pendSysExcMap->empty()) {
        RG_WARNING << "clearPendSysExcMap(): WARNING: Erasing "
                   << m_pendSysExcMap->size() << " incomplete system exclusive message(s). ";
        DeviceEventMap::iterator pendIt = m_pendSysExcMap->begin();
        for(; pendIt != m_pendSysExcMap->end(); ++pendIt) {
            delete pendIt->second.first;
            m_pendSysExcMap->erase(pendIt->first);
        }
    }
}

void
AlsaDriver::pushRecentNoteOffs()
{
#ifdef DEBUG_PROCESS_MIDI_OUT
    RG_DEBUG << "pushRecentNoteOffs(): have " << m_recentNoteOffs.size() << " in queue";
#endif

    for (NoteOffQueue::iterator i = m_recentNoteOffs.begin();
         i != m_recentNoteOffs.end(); ++i) {
        (*i)->setRealTime(RealTime::zeroTime);
        m_noteOffQueue.insert(*i);
    }

    m_recentNoteOffs.clear();
}

// Remove recent noteoffs that are before time t
void
AlsaDriver::cropRecentNoteOffs(const RealTime &t)
{
    while (!m_recentNoteOffs.empty()) {
        NoteOffEvent *ev = *m_recentNoteOffs.begin();
#ifdef DEBUG_PROCESS_MIDI_OUT
        RG_DEBUG << "cropRecentNoteOffs(): " << ev->getRealTime() << " vs " << t;
#endif
        if (ev->getRealTime() >= t) break;
        delete ev;
        m_recentNoteOffs.erase(m_recentNoteOffs.begin());
    }
}

void
AlsaDriver::weedRecentNoteOffs(unsigned int pitch, MidiByte channel,
                               InstrumentId instrument)
{
    for (NoteOffQueue::iterator i = m_recentNoteOffs.begin();
         i != m_recentNoteOffs.end(); ++i) {
        if ((*i)->getPitch() == pitch &&
            (*i)->getChannel() == channel && 
            (*i)->getInstrument() == instrument) {
#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "weedRecentNoteOffs(): deleting one";
#endif
            delete *i;
            m_recentNoteOffs.erase(i);
            break;
        }
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
         it != m_noteOffQueue.end(); ++it) {
        // Set destination according to connection for instrument
        //
        outputDevice = getPairForMappedInstrument((*it)->getInstrument());
        if (outputDevice.client < 0  ||  outputDevice.port < 0)
            continue;

        snd_seq_ev_set_subs(&event);

        // Set source according to port for device
        //
        int src = getOutputPortForMappedInstrument((*it)->getInstrument());
        if (src < 0)
            continue;
        snd_seq_ev_set_source(&event, src);

        snd_seq_ev_set_noteoff(&event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               NOTE_OFF_VELOCITY);

        //snd_seq_event_output(m_midiHandle, &event);
        int error = snd_seq_event_output_direct(m_midiHandle, &event);

        if (error < 0) {
#ifdef DEBUG_ALSA
            RG_WARNING << "allNotesOff() - can't send event";
#endif

        }

        delete(*it);
    }

    m_noteOffQueue.erase(m_noteOffQueue.begin(), m_noteOffQueue.end());

    //RG_DEBUG << "allNotesOff() - queue size = " << m_noteOffQueue.size();

    // flush
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "allNotesOff(): draining");
}

void
AlsaDriver::processNotesOff(const RealTime &time, bool now, bool everything)
{
    if (m_noteOffQueue.empty()) {
        return;
    }

    snd_seq_event_t alsaEvent;

    // prepare the event
    snd_seq_ev_clear(&alsaEvent);

    RealTime alsaTime = getAlsaTime();

#ifdef DEBUG_PROCESS_MIDI_OUT
    RG_DEBUG << "processNotesOff(" << time << "): alsaTime = " << alsaTime << ", now = " << now;
#endif

    // For each note-off event in the note-off queue
    while (m_noteOffQueue.begin() != m_noteOffQueue.end()) {

        NoteOffEvent *noteOff = *m_noteOffQueue.begin();

        if (noteOff->getRealTime() > time) {
#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "processNotesOff(): Note off time " << noteOff->getRealTime() << " is beyond current time " << time;
#endif
            if (!everything) break;
        }

#ifdef DEBUG_PROCESS_MIDI_OUT
        RG_DEBUG << "processNotesOff(" << time << "): found event at " << noteOff->getRealTime() << ", instr " << noteOff->getInstrument() << ", channel " << int(noteOff->getChannel()) << ", pitch " << int(noteOff->getPitch());
#endif

        RealTime offTime = noteOff->getRealTime();
        if (offTime < RealTime::zeroTime) offTime = RealTime::zeroTime;
        bool scheduled = (offTime > alsaTime) && !now;
        if (!scheduled) offTime = RealTime::zeroTime;

        snd_seq_real_time_t alsaOffTime = { (unsigned int)offTime.sec,
                                            (unsigned int)offTime.nsec };

        snd_seq_ev_set_noteoff(&alsaEvent,
                               noteOff->getChannel(),
                               noteOff->getPitch(),
                               NOTE_OFF_VELOCITY);

        bool isSoftSynth = (noteOff->getInstrument() >= SoftSynthInstrumentBase);

        if (!isSoftSynth) {

            snd_seq_ev_set_subs(&alsaEvent);

            // Set source according to instrument
            //
            int src = getOutputPortForMappedInstrument(noteOff->getInstrument());
            if (src < 0) {
                RG_WARNING << "processNotesOff(): WARNING: Note off has no output port (instr = " << noteOff->getInstrument() << ")";
                delete noteOff;
                m_noteOffQueue.erase(m_noteOffQueue.begin());
                continue;
            }

            snd_seq_ev_set_source(&alsaEvent, src);

            snd_seq_ev_set_subs(&alsaEvent);

            snd_seq_ev_schedule_real(&alsaEvent, m_queue, 0, &alsaOffTime);

            if (scheduled) {
                snd_seq_event_output(m_midiHandle, &alsaEvent);
            } else {
                snd_seq_event_output_direct(m_midiHandle, &alsaEvent);
            }

        } else {

            alsaEvent.time.time = alsaOffTime;

            processSoftSynthEventOut(noteOff->getInstrument(), &alsaEvent, now);
        }

        if (!now) {
            m_recentNoteOffs.insert(noteOff);
        } else {
            delete noteOff;
        }
        m_noteOffQueue.erase(m_noteOffQueue.begin());
    }

    // We don't flush the queue here, as this is called nested from
    // processMidiOut, which does the flushing

#ifdef DEBUG_PROCESS_MIDI_OUT
    RG_DEBUG << "processNotesOff() - queue size now: " << m_noteOffQueue.size();
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

    //RG_DEBUG << "getSequencerTime(): alsa time is " << getAlsaTime() << ", start time is " << m_alsaPlayStartTime << ", play start position is " << m_playStartPosition;

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

    if (snd_seq_get_queue_status(m_midiHandle, m_queue, status) < 0) {
#ifdef DEBUG_ALSA
        RG_DEBUG << "getAlsaTime() - can't get queue status";
#endif
        return sequencerTime;
    }

    sequencerTime.sec = snd_seq_queue_status_get_real_time(status)->tv_sec;
    sequencerTime.nsec = snd_seq_queue_status_get_real_time(status)->tv_nsec;

    //RG_DEBUG << "getAlsaTime(): alsa time is " << sequencerTime;

    return sequencerTime;
}


// Get all pending input events and turn them into a MappedEventList.
//
//
bool
AlsaDriver::getMappedEventList(MappedEventList &mappedEventList)
{
    while (failureReportReadIndex != failureReportWriteIndex) {
        MappedEvent::FailureCode code = failureReports[failureReportReadIndex];
        //RG_DEBUG << "getMappedEventList(): failure code: " << code;
        MappedEvent *mE = new MappedEvent
            (0, MappedEvent::SystemFailure, code, 0);
        m_returnComposition.insert(mE);
        failureReportReadIndex =
            (failureReportReadIndex + 1) % FAILURE_REPORT_COUNT;
    }

    if (!m_returnComposition.empty()) {
        for (MappedEventList::iterator i = m_returnComposition.begin();
             i != m_returnComposition.end(); ++i) {
            mappedEventList.insert(new MappedEvent(**i));
        }
        m_returnComposition.clear();
    }

    // If the input port hasn't connected we shouldn't poll it
    //
    //    if (m_midiInputPortConnected == false) {
    //        return true;
    //    }

    RealTime eventTime(0, 0);

    //RG_DEBUG << "getMappedEventList(): looking for events";

    snd_seq_event_t *event;

    // The ALSA documentation indicates that snd_seq_event_input() "returns
    // the byte size of remaining events on the input buffer if an event is
    // successfully received."  This is not true.  snd_seq_event_input()
    // typically returns 1.  Not sure if this is "success" or the number of
    // events read, or something else.  But the point is that although this
    // code appears to be wrong per the ALSA docs, it is actually correct.

    // While there's an event available...
    while (snd_seq_event_input(m_midiHandle, &event) > 0) {
        //RG_DEBUG << "getMappedEventList(): found something";

        unsigned int channel = (unsigned int)event->data.note.channel;
        unsigned int chanNoteKey = ( channel << 8 ) +
            (unsigned int) event->data.note.note;
#ifdef DEBUG_ALSA
        RG_DEBUG << "getMappedEventList(): Got note " << chanNoteKey
                 << " on channel " << channel;
#endif

        bool fromController = false;

        if (event->dest.client == m_client &&
            event->dest.port == m_controllerPort) {
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList(): Received an external controller event";
#endif

            fromController = true;
        }

        unsigned int deviceId = Device::NO_DEVICE;

        if (fromController) {
            deviceId = Device::CONTROL_DEVICE;
        } else {
            for (MappedDeviceList::iterator i = m_devices.begin();
                 i != m_devices.end(); ++i) {
                ClientPortPair pair(m_devicePortMap[(*i)->getId()]);
                if (((*i)->getDirection() == MidiDevice::Record) &&
                    ( pair.client == event->source.client ) &&
                    ( pair.port == event->source.port )) {
                    deviceId = (*i)->getId();
                    break;
                }
            }
        }

        eventTime.sec = event->time.time.tv_sec;
        eventTime.nsec = event->time.time.tv_nsec;
        eventTime = eventTime - m_alsaRecordStartTime + m_playStartPosition;

#ifdef DEBUG_ALSA
        if (!fromController) {
            RG_DEBUG << "getMappedEventList(): Received normal event: type " << int(event->type) << ", chan " << channel << ", note " << int(event->data.note.note) << ", time " << eventTime;
        }
#endif

        switch (event->type) {
        case SND_SEQ_EVENT_NOTE:
        case SND_SEQ_EVENT_NOTEON:
            //RG_DEBUG << "AD::gMEL()  NOTEON channel:" << channel << " pitch:" << event->data.note.note << " velocity:" << event->data.note.velocity;

            if (fromController)
                continue;
            if (event->data.note.velocity > 0) {
                MappedEvent *mE = new MappedEvent();
                mE->setType(MappedEvent::MidiNote);
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
                mE->setDuration(RealTime( -1, 0));

                // Create a copy of this when we insert the NOTE ON -
                // keeping a copy alive on the m_noteOnMap.
                //
                // We shake out the two NOTE Ons after we've recorded
                // them.
                //
                mappedEventList.insert(new MappedEvent(mE));
                m_noteOnMap[deviceId].insert(std::pair<unsigned int, MappedEvent*>(chanNoteKey, mE));

                break;
            }

            // fall-through
            // NOTEON with velocity 0 is treated as a NOTEOFF

        case SND_SEQ_EVENT_NOTEOFF: {
            //RG_DEBUG << "AD::gMEL()  NOTEOFF channel:" << channel << " pitch:" << event->data.note.note;

            if (fromController)
                continue;

            // Check the note on map for any note on events to close.
            // find() prevents inadvertently adding an entry to the map.
            // Since this is commented out, that must not have been an
            // issue.
            //NoteOnMap::iterator noteOnMapIt = m_noteOnMap.find(deviceId);
            ChannelNoteOnMap::iterator noteOnIt = m_noteOnMap[deviceId].find(chanNoteKey);

            // If a corresponding note on was found
            if (noteOnIt != m_noteOnMap[deviceId].end()) {

                // Work with the MappedEvent in the map.  We will transform
                // it into a note off and insert it into the mapped event
                // list.
                MappedEvent *mE = noteOnIt->second;

                // Compute correct duration for the NOTE OFF
                RealTime duration = eventTime - mE->getEventTime();

#ifdef DEBUG_ALSA
                RG_DEBUG << "getMappedEventList(): NOTE OFF: found NOTE ON at " << mE->getEventTime();
#endif

                // Fix zero duration record bug.
                if (duration <= RealTime::zeroTime) {
                    duration = RealTime::fromMilliseconds(1);

                    // ??? It seems odd that we only set the event time for
                    //     the note off in this case.  Otherwise it gets the
                    //     event time of the matching note on.  That seems
                    //     pretty misleading.  I guess a note-off's event time
                    //     plus duration is its event time.  But if we see the
                    //     duration is one millisecond, the eventTime will be
                    //     the actual note-off event time.
                    mE->setEventTime(eventTime);
                }

                // Transform the note-on in the map to a note-off by setting
                // the velocity to 0.
                mE->setVelocity(0);

                // Set duration correctly for recovery later.
                mE->setDuration(duration);

                // Insert this note-off into the mapped event list.
                mappedEventList.insert(mE);

                // reset the reference
                // Remove the MappedEvent from the note on map.
                m_noteOnMap[deviceId].erase(noteOnIt);

            }
        }
            break;

        case SND_SEQ_EVENT_KEYPRESS: {
            if (fromController)
                continue;

            // Fix for 632964 by Pedro Lopez-Cabanillas (20030523)
            //
            MappedEvent *mE = new MappedEvent();
            mE->setType(MappedEvent::MidiKeyPressure);
            mE->setEventTime(eventTime);
            mE->setData1(event->data.note.note);
            mE->setData2(event->data.note.velocity);
            mE->setRecordedChannel(channel);
            mE->setRecordedDevice(deviceId);
            mappedEventList.insert(mE);
        }
            break;

        case SND_SEQ_EVENT_CONTROLLER: {
            MappedEvent *mE = new MappedEvent();
            mE->setType(MappedEvent::MidiController);
            mE->setEventTime(eventTime);
            mE->setData1(event->data.control.param);
            mE->setData2(event->data.control.value);
            mE->setRecordedChannel(channel);
            mE->setRecordedDevice(deviceId);
            mappedEventList.insert(mE);
        }
            break;

        case SND_SEQ_EVENT_PGMCHANGE: {
            MappedEvent *mE = new MappedEvent();
            mE->setType(MappedEvent::MidiProgramChange);
            mE->setEventTime(eventTime);
            mE->setData1(event->data.control.value);
            mE->setRecordedChannel(channel);
            mE->setRecordedDevice(deviceId);
            mappedEventList.insert(mE);

        }
            break;

        case SND_SEQ_EVENT_PITCHBEND: {
            if (fromController)
                continue;

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
            mappedEventList.insert(mE);
        }
            break;

        case SND_SEQ_EVENT_CHANPRESS: {
            if (fromController)
                continue;

            // Fixed by Pedro Lopez-Cabanillas (20030523)
            //
            int s = event->data.control.value & 0x7f;
            MappedEvent *mE = new MappedEvent();
            mE->setType(MappedEvent::MidiChannelPressure);
            mE->setEventTime(eventTime);
            mE->setData1(s);
            mE->setRecordedChannel(channel);
            mE->setRecordedDevice(deviceId);
            mappedEventList.insert(mE);
        }
            break;

        case SND_SEQ_EVENT_SYSEX:

            if (fromController)
                continue;

            if (!testForMTCSysex(event) &&
                !testForMMCSysex(event)) {

                // Bundle up the data into a block on the MappedEvent
                //
                std::string data;
                char *ptr = (char*)(event->data.ext.ptr);
                for (unsigned int i = 0; i < event->data.ext.len; ++i)
                    data += *(ptr++);

#ifdef DEBUG_ALSA

                if ((MidiByte)(data[1]) == MIDI_SYSEX_RT) {
                    RG_DEBUG << "getMappedEventList(): REALTIME SYSEX";
                    for (unsigned int ii = 0; ii < event->data.ext.len; ++ii) {
                        printf("B %d = %02x\n", ii, ((char*)(event->data.ext.ptr))[ii]);
                    }
                } else {
                    RG_DEBUG << "getMappedEventList(): NON-REALTIME SYSEX";
                    for (unsigned int ii = 0; ii < event->data.ext.len; ++ii) {
                        printf("B %d = %02x\n", ii, ((char*)(event->data.ext.ptr))[ii]);
                    }
                }
#endif

                // Thank you to Christoph Eckert for pointing out via
                // Pedro Lopez-Cabanillas aseqmm code that we need to pool
                // alsa system exclusive messages since they may be broken
                // across several ALSA messages.
            
                // Unfortunately, pooling these messages get very complicated
                // since it creates many corner cases during this realtime
                // activity that may involve possible bad data transmissions.
            
                bool beginNewMessage = false;
                if (data.length() > 0) {
                    // Check if at start of MIDI message
                    if (MidiByte(data.at(0)) == MIDI_SYSTEM_EXCLUSIVE) {
                        data.erase(0,1); // Skip (SYX). RG doesn't use it.
                        beginNewMessage = true;
                    }
                }

                std::string sysExcData; 
                MappedEvent *sysExcEvent = nullptr;

                // Check to see if there are any pending System Exclusive Messages
                if (!m_pendSysExcMap->empty()) {
                    // Check our map to see if we have a pending operations for
                    // the current deviceId.
                    DeviceEventMap::iterator pendIt = m_pendSysExcMap->find(deviceId);
                
                    if (pendIt != m_pendSysExcMap->end()) {
                        sysExcEvent = pendIt->second.first;
                        sysExcData = pendIt->second.second;
                    
                        // Be optimistic that we won't have to re-add this afterwards.
                        // Also makes keeping track of this easier.
                        m_pendSysExcMap->erase(pendIt);
                    }
                }
            
                bool createNewEvent = false;
                if (!sysExcEvent) {
                    // Did not find a pending (unfinished) System Exclusive message.
                    // Create a new event.
                    createNewEvent = true;
                
                    if (!beginNewMessage) {
                        RG_WARNING << "getMappedEventList(): WARNING: New ALSA message arrived with incorrect MIDI System Exclusive start byte.";
                        RG_WARNING << "getMappedEventList():          This is probably a bad transmission.";
                    }
                } else {
                    // We found a pending (unfinished) System Exclusive message.

                    // Check if at start of MIDI message
                    if (!beginNewMessage) {
                        // Prepend pooled events to the current message data

                        if (sysExcData.size() > 0) {
                            data.insert(0, sysExcData);
                        }
                    } else {
                        // This is the start of a new message but have
                        // pending (incomplete) messages already.
                        createNewEvent = true;
                    
                        // Decide how to handle previous (incomplete) message
                        if (sysExcData.size() > 0) {
                            RG_WARNING << "getMappedEventList(): WARNING: Sending an incomplete ALSA message to the composition.";
                            RG_WARNING << "getMappedEventList():          This is probably a bad transmission.";

                            // Push previous (incomplete) message to mapped event list
                            DataBlockRepository::setDataBlockForEvent(sysExcEvent, sysExcData);
                            mappedEventList.insert(sysExcEvent);
                        } else {
                            // Previous message has no meaningful data.
                            RG_WARNING << "getMappedEventList(): WARNING: Discarding meaningless incomplete ALSA message";

                            delete sysExcEvent;
                        }
                    }
                }
                            
                if (createNewEvent) {
                    // Still need a current event to work with.  Create it.
                    sysExcEvent = new MappedEvent();
                    sysExcEvent->setType(MappedEvent::MidiSystemMessage);
                    sysExcEvent->setData1(MIDI_SYSTEM_EXCLUSIVE);
                    sysExcEvent->setRecordedDevice(deviceId);
                    sysExcEvent->setEventTime(eventTime);
                }

                // We need to check to see if this event completes the
                // System Exclusive event.
            
                bool pushOnMap = false;
                if (!data.empty()) {
                    int lastChar = data.size() - 1;
                
                    // Check to see if we are at the end of a message.
                    if (MidiByte(data.at(lastChar)) == MIDI_END_OF_EXCLUSIVE) {
                        // Remove (EOX). RG doesn't use it. 
                        data.erase(lastChar);

                        // Push message to mapped event list
                        DataBlockRepository::setDataBlockForEvent(sysExcEvent, data);
                        mappedEventList.insert(sysExcEvent);
                    } else {

                        pushOnMap = true;
                    }
                } else {
                    // Data is empty.  Anyway we got here we need to put it back
                    // in the pending map.  This will resolve itself elsewhere.
                    // But if we are here, this is probably and error.

                    RG_WARNING << "getMappedEventList(): WARNING: ALSA message arrived with no useful System Exclusive data bytes";
                    RG_WARNING << "getMappedEventList():          This is probably a bad transmission";

                    pushOnMap = true;
                }
            
                if (pushOnMap) {
                    // Put the unfinished event back in the pending map.
                    m_pendSysExcMap->insert(std::make_pair(deviceId,
                                                           std::make_pair(sysExcEvent, data)));

                    if (beginNewMessage) { 
                        RG_DEBUG << "getMappedEventList(): Encountered long System Exclusive Message (pooling message until transmission complete)";
                    }
                }
            }
            break;


        case SND_SEQ_EVENT_SENSING:  // MIDI device is still there
            break;

        case SND_SEQ_EVENT_QFRAME:
            if (fromController)
                continue;
            if (getMTCStatus() == TRANSPORT_FOLLOWER) {
                handleMTCQFrame(event->data.control.value, eventTime);
            }
            break;

        case SND_SEQ_EVENT_CLOCK:
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - got realtime MIDI clock";
#endif
            break;

        case SND_SEQ_EVENT_START:
            if ((getMIDISyncStatus() == TRANSPORT_FOLLOWER) && !isPlaying()) {
                ExternalTransport *transport = getExternalTransportControl();
                if (transport) {
                    transport->transportJump(ExternalTransport::TransportStopAtTime,
                                             RealTime::zeroTime);
                    transport->transportChange(ExternalTransport::TransportStart);
                }
            }
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - START";
#endif
            break;

        case SND_SEQ_EVENT_CONTINUE:
            if ((getMIDISyncStatus() == TRANSPORT_FOLLOWER) && !isPlaying()) {
                ExternalTransport *transport = getExternalTransportControl();
                if (transport) {
                    transport->transportChange(ExternalTransport::TransportPlay);
                }
            }
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - CONTINUE";
#endif
            break;

        case SND_SEQ_EVENT_STOP:
            if ((getMIDISyncStatus() == TRANSPORT_FOLLOWER) && isPlaying()) {
                ExternalTransport *transport = getExternalTransportControl();
                if (transport) {
                    transport->transportChange(ExternalTransport::TransportStop);
                }
            }
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - STOP";
#endif
            break;

        case SND_SEQ_EVENT_SONGPOS:
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - SONG POSITION";
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
            RG_DEBUG << "getMappedEventList() - got announce event (" << int(event->type) << ")";
#endif

            break;
        case SND_SEQ_EVENT_TICK:
        default:
#ifdef DEBUG_ALSA
            RG_DEBUG << "getMappedEventList() - got unhandled MIDI event type from ALSA sequencer" << "(" << int(event->type) << ")";
#endif

            break;


        }
    }

    if (getMTCStatus() == TRANSPORT_FOLLOWER && isPlaying()) {
#ifdef MTC_DEBUG
        RG_DEBUG << "getMappedEventList(): seq time is " << getSequencerTime() << ", last MTC receive "
                 << m_mtcLastReceive << ", first time " << m_mtcFirstTime;
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
    return true;
}

// This should probably be a non-static private member.
static int lock_count = 0;

void
AlsaDriver::handleMTCQFrame(unsigned int data_byte, RealTime the_time)
{
    if (getMTCStatus() != TRANSPORT_FOLLOWER)
        return ;

    switch (data_byte & 0xF0) {
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

    case 0x70: {
        m_mtcHours |= (data_byte & 0x01) << 4;
        m_mtcSMPTEType = (data_byte & 0x06) >> 1;

        int fps = 30;
        if (m_mtcSMPTEType == 0)
            fps = 24;
        else if (m_mtcSMPTEType == 1)
            fps = 25;

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
            RG_DEBUG << "handleMTCQFrame(): RG MTC: Tstamp " << m_mtcEncodedTime << " Received @ " << m_mtcReceiveTime;
#endif

            calibrateMTC();

            RealTime t_diff = m_mtcEncodedTime - m_mtcReceiveTime;
#ifdef MTC_DEBUG
            RG_DEBUG << "handleMTCQFrame(): Diff: " << t_diff;
#endif

            /* -ve diff means ALSA time ahead of MTC time */

            if (t_diff.sec > 0) {
                tweakSkewForMTC(60000);
            } else if (t_diff.sec < 0) {
                tweakSkewForMTC( -60000);
            } else {
                /* "small" diff - use adaptive technique */
                tweakSkewForMTC(t_diff.nsec / 1400);
                if ((t_diff.nsec / 1000000) == 0) {
                    if (++lock_count == 3) {
                        printf("Got a lock @ %02d:%02d:%02d.%02d (type %d)\n",
                               m_mtcHours,
                               m_mtcMinutes,
                               m_mtcSeconds,
                               m_mtcFrames,
                               m_mtcSMPTEType);
                    }
                } else {
                    lock_count = 0;
                }
            }

        } else if (m_eat_mtc > 0) {
#ifdef MTC_DEBUG
            RG_DEBUG << "handleMTCQFrame(): MTC: Received quarter frame just after issuing MMC stop - ignore it";
#endif

            --m_eat_mtc;
        } else {
            /* If we're not playing, we should be. */
#ifdef MTC_DEBUG
            RG_DEBUG << "handleMTCQFrame(): MTC: Received quarter frame while not playing - starting now";
#endif

            ExternalTransport *transport = getExternalTransportControl();
            if (transport) {
                tweakSkewForMTC(0);  /* JPM - reset it on start of playback, to be sure */
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
    snd_seq_real_time_t atime =
        { (unsigned int)time.sec, (unsigned int)time.nsec };

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
        return ;
    }

    // We send at 25fps, it's the easiest to avoid rounding problems
    RealTime twoFrames(0, 80000000U);
    RealTime quarterFrame(0, 10000000U);
    int fps = 25;

#ifdef MTC_DEBUG
    RG_DEBUG << "insertMTCQFrames(" << sliceStart << ","
             << sliceEnd << "): first time " << m_mtcFirstTime;
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
        RG_DEBUG << "insertMTCQFrames(): Sending MTC quarter frame at " << t;
#endif

        unsigned char c = (type << 4);

        switch (type) {
        case 0:
            c += ((unsigned char)m_mtcFrames & 0x0f);
            break;
        case 1:
            c += (((unsigned char)m_mtcFrames & 0xf0) >> 4);
            break;
        case 2:
            c += ((unsigned char)m_mtcSeconds & 0x0f);
            break;
        case 3:
            c += (((unsigned char)m_mtcSeconds & 0xf0) >> 4);
            break;
        case 4:
            c += ((unsigned char)m_mtcMinutes & 0x0f);
            break;
        case 5:
            c += (((unsigned char)m_mtcMinutes & 0xf0) >> 4);
            break;
        case 6:
            c += ((unsigned char)m_mtcHours & 0x0f);
            break;
        case 7:  // hours high nibble + smpte type
            c += (m_mtcHours >> 4) & 0x01;
            c += (1 << 1); // type 1 indicates 25fps
            break;
        }

        RealTime scheduleTime = t + m_alsaPlayStartTime - m_playStartPosition;
        snd_seq_real_time_t atime =
            { (unsigned int)scheduleTime.sec,
              (unsigned int)scheduleTime.nsec };

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
    if (getMTCStatus() != TRANSPORT_FOLLOWER)
        return false;

    // At this point, and possibly for the foreseeable future, the only
    // sysex we're interested in is full-frame transport location

#ifdef MTC_DEBUG
    RG_DEBUG << "testForMTCSysex(): MTC: testing sysex of length " << event->data.ext.len << ":";
    for (int i = 0; i < event->data.ext.len; ++i) {
        RG_DEBUG << "testForMTCSysex():     " << (int)*((unsigned char *)event->data.ext.ptr + i);
    }
#endif

    if (event->data.ext.len != 10)
        return false;

    unsigned char *ptr = (unsigned char *)(event->data.ext.ptr);

    if (*ptr++ != MIDI_SYSTEM_EXCLUSIVE)
        return false;
    if (*ptr++ != MIDI_SYSEX_RT)
        return false;
    if (*ptr++ > 127)
        return false;

    // 01 01 for MTC full frame

    if (*ptr++ != 1)
        return false;
    if (*ptr++ != 1)
        return false;

    int htype = *ptr++;
    int min = *ptr++;
    int sec = *ptr++;
    int frame = *ptr++;

    if (*ptr != MIDI_END_OF_EXCLUSIVE)
        return false;

    int hour = (htype & 0x1f);
    int type = (htype & 0xe0) >> 5;

    m_mtcFrames = frame;
    m_mtcSeconds = sec;
    m_mtcMinutes = min;
    m_mtcHours = hour;
    m_mtcSMPTEType = type;

    int fps = 30;
    if (m_mtcSMPTEType == 0)
        fps = 24;
    else if (m_mtcSMPTEType == 1)
        fps = 25;

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
    RG_DEBUG << "testForMTCSysex(): MTC: MTC sysex found (frame type " << type << "), jumping to " << m_mtcEncodedTime;
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
        return ;
    else if (m_mtcFirstTime > 0) {
        --m_mtcFirstTime;
        m_mtcSigmaC = 0;
        m_mtcSigmaE = 0;
    } else {
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
/*
JPM: If CalibrateMTC malfunctions (which tends to happen if the timecode
restarts a lot) then 'bias_factor' will be left in the range of 1.8 billion
and the sequencer engine will be unusable until the program is quit and
restarted.  Reset it to a sane default when called with factor of 0
*/

    if (factor == 0) {
        bias_factor = 0;
    }

    if (factor > 50000) {
        factor = 50000;
    } else if (factor < -50000) {
        factor = -50000;
    } else if (factor == last_factor) {
        return ;
    } else {
        if (m_mtcFirstTime == -1)
            m_mtcFirstTime = 5;
    }
    last_factor = factor;

    snd_seq_queue_tempo_t *q_ptr;
    snd_seq_queue_tempo_alloca(&q_ptr);

    snd_seq_get_queue_tempo( m_midiHandle, m_queue, q_ptr);

    unsigned int t_skew = snd_seq_queue_tempo_get_skew(q_ptr);
#ifdef MTC_DEBUG
    RG_DEBUG << "tweakSkewForMTC(): RG MTC: skew: " << t_skew;
#endif

    t_skew = 0x10000 + factor + bias_factor;

#ifdef MTC_DEBUG
    RG_DEBUG << "tweakSkewForMTC():     changed to " << factor << "+" << bias_factor;
#endif

    snd_seq_queue_tempo_set_skew(q_ptr, t_skew);
    snd_seq_set_queue_tempo( m_midiHandle, m_queue, q_ptr);

    m_mtcSkew = t_skew;
}

bool
AlsaDriver::testForMMCSysex(const snd_seq_event_t *event)
{
    if (getMMCStatus() != TRANSPORT_FOLLOWER)
        return false;

    if (event->data.ext.len != 6)
        return false;

    unsigned char *ptr = (unsigned char *)(event->data.ext.ptr);

    if (*ptr++ != MIDI_SYSTEM_EXCLUSIVE)
        return false;
    if (*ptr++ != MIDI_SYSEX_RT)
        return false;
    if (*ptr++ > 127)
        return false;
    if (*ptr++ != MIDI_SYSEX_RT_COMMAND)
        return false;

    int instruction = *ptr++;

    if (*ptr != MIDI_END_OF_EXCLUSIVE)
        return false;

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
AlsaDriver::processMidiOut(const MappedEventList &mC,
                           const RealTime &sliceStart,
                           const RealTime &sliceEnd)
{
    LOCKED;

    // special case for unqueued events
    bool now = (sliceStart == RealTime::zeroTime && sliceEnd == RealTime::zeroTime);

#ifdef DEBUG_PROCESS_MIDI_OUT
    RG_DEBUG << "processMidiOut(" << sliceStart << "," << sliceEnd << "), " << mC.size() << " events, now is " << now;
#endif

    if (!now) {
        // This 0.5 sec is arbitrary, but it must be larger than the
        // sequencer's read-ahead
        RealTime diff = RealTime::fromSeconds(0.5);
        RealTime cutoff = sliceStart - diff;
        cropRecentNoteOffs(cutoff - m_playStartPosition + m_alsaPlayStartTime);
    }

    // These won't change in this slice
    //
    if ((mC.begin() != mC.end())) {
        SequencerDataBlock::getInstance()->setVisual(*mC.begin());
    }

    // A pointer to this is extracted from it and placed in "event".
    // We need this to stay alive so that the pointer continues to
    // be valid until "event" is finally used.  It might be possible to
    // move this to a smaller scope, but this loop is really big and
    // hard to follow.
    std::string sysExData;

    // NB the MappedEventList is implicitly ordered by time (std::multiset)

    // For each incoming mapped event
    // ??? "i" is a bit hard to follow in this huge 400-line loop.  How about
    //     we dereference it at the top and never use "(*i)" again:
    //       const MappedEvent *mappedEvent = (*i);
    //     Might shave off a CPU cycle or two as a bonus.
    for (MappedEventList::const_iterator i = mC.begin(); i != mC.end(); ++i) {
        // Skip all non-MIDI events.
        if ((*i)->getType() >= MappedEvent::Audio)
            continue;

        if ((*i)->getType() == MappedEvent::MidiNote &&
            (*i)->getDuration() == RealTime::zeroTime &&
            (*i)->getVelocity() == 0) {
            // NOTE OFF with duration zero is scheduled from the
            // internal segment mapper and we don't use that message
            // in realtime otherwise it is a duplicate.
            // When we receive a NOTE OFF message from MIDI input, the
            // duration is never zero but at least 1 msec (see the case
            // for SND_SEQ_EVENT_NOTEOFF in getMappedEventList).
            continue;
        }

        bool debug = throttledDebug();
        if (debug) {
            RG_DEBUG << "processMidiOut(): for each event...";
            QString eventType = "unknown";
            switch ((*i)->getType()) {
                case MappedEvent::MidiNote: eventType = "MidiNote"; break;
                case MappedEvent::MidiNoteOneShot: eventType = "MidiNoteOneShot"; break;
                case MappedEvent::MidiController: eventType = "MidiController"; break;
                default: break;
            }
            RG_DEBUG << "processMidiOut():   MappedEvent Event Type: " << (*i)->getType() << " (" << eventType << ")";
        }

        // ??? rename: alsaEvent
        snd_seq_event_t event;
        snd_seq_ev_clear(&event);
    
        bool isControllerOut = ((*i)->getRecordedDevice() ==
                                Device::CONTROL_DEVICE);

        bool isSoftSynth = (!isControllerOut &&
                            ((*i)->getInstrument() >= SoftSynthInstrumentBase));

        RealTime outputTime = (*i)->getEventTime() - m_playStartPosition +
            m_alsaPlayStartTime;

        if (now && !m_playing && m_queueRunning) {
            // stop queue to ensure exact timing and make sure the
            // event gets through right now
#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "processMidiOut(): stopping queue for now-event";
#endif

            checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, nullptr), "processMidiOut(): stop queue");
            checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
        }

        RealTime alsaTimeNow = getAlsaTime();

        if (now) {
            if (!m_playing) {
                outputTime = alsaTimeNow;
            } else if (outputTime < alsaTimeNow) {
                // This isn't really necessary as ALSA will immediately
                // send out events that are prior to the current time.
                // And that's what we want anyway.
                outputTime = alsaTimeNow;
            }
        }

#ifdef DEBUG_PROCESS_MIDI_OUT
        RG_DEBUG << "processMidiOut[" << now << "]: event is at " << outputTime << " (" << outputTime - alsaTimeNow << " ahead of queue time), type " << int((*i)->getType()) << ", duration " << (*i)->getDuration();
#endif

        if (!m_queueRunning && outputTime < alsaTimeNow) {
            RealTime adjust = alsaTimeNow - outputTime;
            if ((*i)->getDuration() > RealTime::zeroTime) {
                if ((*i)->getDuration() <= adjust) {
#ifdef DEBUG_PROCESS_MIDI_OUT
                    RG_DEBUG << "processMidiOut[" << now << "]: too late for this event, abandoning it";
#endif

                    continue;
                } else {
#ifdef DEBUG_PROCESS_MIDI_OUT
                    RG_DEBUG << "processMidiOut[" << now << "]: pushing event forward and reducing duration by " << adjust;
#endif

                    (*i)->setDuration((*i)->getDuration() - adjust);
                }
            } else {
#ifdef DEBUG_PROCESS_MIDI_OUT
                RG_DEBUG << "processMidiOut[" << now << "]: pushing zero-duration event forward by " << adjust;
#endif

            }
            outputTime = alsaTimeNow;
        }

        processNotesOff(outputTime, now);

#if defined(DEBUG_PROCESS_MIDI_OUT)  &&  defined(HAVE_LIBJACK)
        if (m_jackDriver) {
            size_t frameCount = m_jackDriver->getFramesProcessed();
            size_t elapsed = frameCount - debug_jack_frame_count;
            RealTime rt = RealTime::frame2RealTime(elapsed, m_jackDriver->getSampleRate());
            rt = rt - getAlsaTime();
            RG_DEBUG << "processMidiOut[" << now << "]: JACK time is " << rt << " ahead of ALSA time";
        }
#endif

        // Second and nanoseconds for ALSA
        //
        snd_seq_real_time_t time =
            { (unsigned int)outputTime.sec, (unsigned int)outputTime.nsec };

        if (!isSoftSynth) {

#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "processMidiOut[" << now << "]: instrument " << (*i)->getInstrument();
            RG_DEBUG << "processMidiOut():     pitch: " << (int)(*i)->getPitch() << ", velocity " << (int)(*i)->getVelocity() << ", duration " << (*i)->getDuration();
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

        MappedInstrument *instrument = getMappedInstrument((*i)->getInstrument());

        // set the stop time for Note Off
        //
        RealTime outputStopTime = outputTime + (*i)->getDuration()
            - RealTime(0, 1); // notch it back 1nsec just to ensure
        // correct ordering against any other
        // note-ons at the same nominal time
        bool needNoteOff = false;

        MidiByte channel = 0;

        if (isControllerOut) {
            channel = (*i)->getRecordedChannel();
#ifdef DEBUG_ALSA
            RG_DEBUG << "processMidiOut() - Event of type " << (int)((*i)->getType()) << " (data1 " << (int)(*i)->getData1() << ", data2 " << (int)(*i)->getData2() << ") for external controller channel " << (int)channel;
#endif
        } else if (instrument != nullptr) {
            channel = (*i)->getRecordedChannel();
#ifdef DEBUG_ALSA
            RG_DEBUG << "processMidiOut() - Non-controller Event of type " << (int)((*i)->getType()) << " (data1 " << (int)(*i)->getData1() << ", data2 " << (int)(*i)->getData2() << ") for channel " << (int)(*i)->getRecordedChannel();
#endif
        } else {
#ifdef DEBUG_ALSA
            RG_DEBUG << "processMidiOut() - No instrument for event of type "
                      << (int)(*i)->getType() << " at " << (*i)->getEventTime();
#endif
            channel = 0;
        }

        // channel is a MidiByte which is unsigned.  This will never be true.
        //if (channel < 0) { continue; }

        switch ((*i)->getType()) {

        case MappedEvent::MidiNote:
            if ((*i)->getVelocity() == 0) {
                snd_seq_ev_set_noteoff(&event,
                                       channel,
                                       (*i)->getPitch(),
                                       NOTE_OFF_VELOCITY);
                break;
            }

            // !!! FALLTHROUGH
            //
            // MidiNote behaves exactly like MidiNoteOneShot, except that
            // a velocity of 0 results in a note-off instead of a note-on.
            //
            // Why "OneShot" is used to differentiate is unclear.  Renaming
            // may be in order.  A duration of RealTime(-1,0) results in
            // the sending of only a note-on and no note-off.  This seems
            // more like a kind of "one-shot" than treating a 0 velocity
            // in a special way.

        case MappedEvent::MidiNoteOneShot:
            snd_seq_ev_set_noteon(&event,
                                  channel,
                                  (*i)->getPitch(),
                                  (*i)->getVelocity());

            // NOTE ON from MIDI input is scheduled with duration -1
            // and we don't use the NOTE OFF stack for MIDI input.
            if ((*i)->getDuration() > RealTime(-1, 0)) {
                needNoteOff = true;
            }

            if (!isSoftSynth) {
                LevelInfo info;
                info.level = (*i)->getVelocity();
                info.levelRight = 0;
                SequencerDataBlock::getInstance()->setInstrumentLevel
                    ((*i)->getInstrument(), info);
            }

            weedRecentNoteOffs((*i)->getPitch(), channel, (*i)->getInstrument());
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

        case MappedEvent::MidiPitchBend: {
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

        case MappedEvent::MidiSystemMessage: {
            switch ((*i)->getData1()) {
            case MIDI_SYSTEM_EXCLUSIVE: {
                char out[2];
                sprintf(out, "%c", MIDI_SYSTEM_EXCLUSIVE);
                sysExData = out;

                sysExData += DataBlockRepository::getDataBlockForEvent((*i));

                sprintf(out, "%c", MIDI_END_OF_EXCLUSIVE);
                sysExData += out;

                // Note: sysExData needs to stay around until this event
                //   is actually sent.  event has a pointer to its contents.
                snd_seq_ev_set_sysex(&event,
                                     sysExData.length(),
                                     (char*)(sysExData.c_str()));
            }
                break;

            case MIDI_TIMING_CLOCK: {
                RealTime rt =
                    RealTime(time.tv_sec, time.tv_nsec);

                //RG_DEBUG << "processMidiOut() - " << "send clock @ " << rt;

                sendSystemQueued(SND_SEQ_EVENT_CLOCK, "", rt);

                continue;

            }
                break;

            default:
                RG_WARNING << "processMidiOut(): WARNING: unrecognised system message";
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

            // These types do nothing here, so go on to the
            // next iteration.
        case MappedEvent::Audio:
        case MappedEvent::AudioCancel:
        case MappedEvent::AudioLevel:
        case MappedEvent::AudioStopped:
        case MappedEvent::SystemUpdateInstruments:
        case MappedEvent::SystemJackTransport:  //???
        case MappedEvent::SystemMMCTransport:
        case MappedEvent::SystemMIDIClock:
        case MappedEvent::SystemMIDISyncAuto:
        case MappedEvent::AudioGeneratePreview:
        case MappedEvent::Marker:
        case MappedEvent::Panic:
        case MappedEvent::SystemAudioFileFormat:
        case MappedEvent::SystemAudioPortCounts:
        case MappedEvent::SystemAudioPorts:
        case MappedEvent::SystemFailure:
        case MappedEvent::SystemMetronomeDevice:
        case MappedEvent::SystemMTCTransport:
        case MappedEvent::TimeSignature:
        case MappedEvent::Tempo:
        case MappedEvent::Text:
             continue;

        default:
        case MappedEvent::InvalidMappedEvent:
#ifdef DEBUG_ALSA
            RG_DEBUG << "processMidiOut() - skipping unrecognised or invalid MappedEvent type";
#endif

            continue;
        }

        if (debug) {
            QString eventType = "unknown";
            switch (event.type) {
                case SND_SEQ_EVENT_NOTEON: eventType = "SND_SEQ_EVENT_NOTEON"; break;
                case SND_SEQ_EVENT_NOTEOFF: eventType = "SND_SEQ_EVENT_NOTEOFF"; break;
                case SND_SEQ_EVENT_CONTROLLER: eventType = "SND_SEQ_EVENT_CONTROLLER"; break;
                default: break;
            }
            RG_DEBUG << "  ALSA event type: " << event.type << " (" << eventType << ")";
        }

        if (isSoftSynth) {
            if (debug)
                RG_DEBUG << "  Calling processSoftSynthEventOut()...";

            processSoftSynthEventOut((*i)->getInstrument(), &event, now);

        } else {
            if (debug)
                RG_DEBUG << "  Calling snd_seq_event_output()...";

            int rc = snd_seq_event_output(m_midiHandle, &event);
            checkAlsaError(rc, "processMidiOut(): output queued");

            if (debug)
                RG_DEBUG << "  snd_seq_event_output() rc:" << rc;

            if (now) {
                if (m_queueRunning && !m_playing) {
                    // restart queue
#ifdef DEBUG_PROCESS_MIDI_OUT
                    RG_DEBUG << "processMidiOut(): restarting queue after now-event";
#endif

                    checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, nullptr), "processMidiOut(): continue queue");
                }
                checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
            }
        }

        // Add note to note off stack
        //
        if (needNoteOff) {
            NoteOffEvent *noteOffEvent =
                new NoteOffEvent(outputStopTime,  // already calculated
                                 (*i)->getPitch(),
                                 channel,
                                 (*i)->getInstrument());

#ifdef DEBUG_ALSA
            RG_DEBUG << "processMidiOut(): Adding NOTE OFF at " << outputStopTime;
#endif

            m_noteOffQueue.insert(noteOffEvent);
        }
    }  // for each event

    processNotesOff(sliceEnd - m_playStartPosition + m_alsaPlayStartTime, now);

    if (getMTCStatus() == TRANSPORT_SOURCE) {
        insertMTCQFrames(sliceStart, sliceEnd);
    }

    if (m_queueRunning) {

        if (now && !m_playing) {
            // just to be sure
#ifdef DEBUG_PROCESS_MIDI_OUT
            RG_DEBUG << "processMidiOut(): restarting queue after all now-events";
#endif

            checkAlsaError(snd_seq_continue_queue(m_midiHandle, m_queue, nullptr), "processMidiOut(): continue queue");
        }

#ifdef DEBUG_PROCESS_MIDI_OUT 
        //RG_DEBUG << "processMidiOut(): m_queueRunning " << m_queueRunning << ", now " << now;
#endif
        checkAlsaError(snd_seq_drain_output(m_midiHandle), "processMidiOut(): draining");
    }
}

void
AlsaDriver::processSoftSynthEventOut(InstrumentId id, const snd_seq_event_t *ev, bool now)
{
#ifdef DEBUG_PROCESS_SOFT_SYNTH_OUT
    RG_DEBUG << "processSoftSynthEventOut(): instrument " << id << ", now " << now;
#endif

#ifdef HAVE_LIBJACK

    if (!m_jackDriver)
        return ;
    RunnablePluginInstance *synthPlugin = m_jackDriver->getSynthPlugin(id);

    if (synthPlugin) {

        RealTime t(ev->time.time.tv_sec, ev->time.time.tv_nsec);

        if (now)
            t = RealTime::zeroTime;
        else
            t = t + m_playStartPosition - m_alsaPlayStartTime;

#ifdef DEBUG_PROCESS_SOFT_SYNTH_OUT
        RG_DEBUG << "processSoftSynthEventOut(): event time " << t;
#endif

        synthPlugin->sendEvent(t, ev);

        if (now) {
#ifdef DEBUG_PROCESS_SOFT_SYNTH_OUT
            RG_DEBUG << "processSoftSynthEventOut(): setting haveAsyncAudioEvent";
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

#ifdef DEBUG_ALSA
    RG_DEBUG << "startClocks() begin...";
#endif

    if (m_needJackStart) {
#ifdef DEBUG_ALSA
        RG_DEBUG << "startClocks: Need JACK start (m_playing = " << m_playing << ")";
#endif

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
#ifdef DEBUG_ALSA
                RG_DEBUG << "startClocks(): playing, prebuffer audio";
#endif

                m_jackDriver->prebufferAudio();
            } else {
#ifdef DEBUG_ALSA
                RG_DEBUG << "startClocks(): prepare audio only";
#endif

                m_jackDriver->prepareAudio();
            }
            bool rv;
            if (m_needJackStart == NeedJackReposition) {
                rv = m_jackDriver->relocateTransport();
            } else {
                rv = m_jackDriver->startTransport();
                if (!rv) {
#ifdef DEBUG_ALSA
                    RG_DEBUG << "startClocks(): Waiting for startClocksApproved";
#endif
                    // need to wait for transport sync
                    debug_jack_frame_count = m_jackDriver->getFramesProcessed();
                    return ;
                }
            }
        }
    }
#endif

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, nullptr)) < 0) {
        RG_WARNING << "startClocks(): WARNING: Couldn't start queue - " << snd_strerror(result);
        reportFailure(MappedEvent::FailureALSACallFailed);
    }

#ifdef DEBUG_ALSA
    RG_DEBUG << "startClocks(): started clocks";
#endif

    m_queueRunning = true;

#ifdef HAVE_LIBJACK

    if (m_jackDriver) {
        debug_jack_frame_count = m_jackDriver->getFramesProcessed();
    }
#endif

    // process pending MIDI events
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "startClocks(): draining");
}

void
AlsaDriver::startClocksApproved()
{
    LOCKED;
#ifdef DEBUG_ALSA
    RG_DEBUG << "startClocksApproved() begin...";
#endif

    //!!!
    m_needJackStart = NeedNoJackStart;
    startClocks();
    return ;

    int result;

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, nullptr)) < 0) {
        RG_WARNING << "startClocksApproved(): WARNING: Couldn't start queue - " << snd_strerror(result);
        reportFailure(MappedEvent::FailureALSACallFailed);
    }

    m_queueRunning = true;

    // process pending MIDI events
    checkAlsaError(snd_seq_drain_output(m_midiHandle), "startClocksApproved(): draining");
}

void
AlsaDriver::stopClocks()
{
#ifdef DEBUG_ALSA
    RG_DEBUG << "stopClocks() begin...";
#endif

    if (checkAlsaError(snd_seq_stop_queue(m_midiHandle, m_queue, nullptr), "stopClocks(): stopping queue") < 0) {
        reportFailure(MappedEvent::FailureALSACallFailed);
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

#ifdef DEBUG_ALSA
    RG_DEBUG << "stopClocks(): ALSA time now is " << getAlsaTime();
#endif

    m_alsaPlayStartTime = RealTime::zeroTime;
}


void
AlsaDriver::processEventsOut(const MappedEventList &mC)
{
    processEventsOut(mC, RealTime::zeroTime, RealTime::zeroTime);
}

void
AlsaDriver::processEventsOut(const MappedEventList &mC,
                             const RealTime &sliceStart,
                             const RealTime &sliceEnd)
{
    // special case for unqueued events
#ifdef HAVE_LIBJACK
    const bool now = (sliceStart == RealTime::zeroTime && sliceEnd == RealTime::zeroTime);
#endif

    if (m_startPlayback) {
        m_startPlayback = false;
        // This only records whether we're playing in principle,
        // not whether the clocks are actually ticking.  Contrariwise,
        // areClocksRunning tells us whether the clocks are ticking
        // but not whether we're actually playing (the clocks go even
        // when we're not).  Check both if you want to know whether
        // we're really rolling.
        m_playing = true;

        if (getMTCStatus() == TRANSPORT_FOLLOWER) {
            tweakSkewForMTC(0);
        }
    }

    AudioFile *audioFile = nullptr;
    bool haveNewAudio = false;

    // For each incoming event, insert audio events if we find them
    for (MappedEventList::const_iterator i = mC.begin(); i != mC.end(); ++i) {
#ifdef HAVE_LIBJACK

        // Play an audio file
        //
        if ((*i)->getType() == MappedEvent::Audio) {
            if (!m_jackDriver)
                continue;

            // This is used for handling asynchronous
            // (i.e. unexpected) audio events only

            if ((*i)->getEventTime() > RealTime( -120, 0)) {
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

            if (audioFile) {
                MappedAudioFader *fader =
                    dynamic_cast<MappedAudioFader*>
                    (getMappedStudio()->getAudioFader((*i)->getInstrument()));

                if (!fader) {
                    RG_WARNING << "processEventsOut(): WARNING: No fader for audio instrument " << (*i)->getInstrument();
                    continue;
                }

                int channels = fader->getPropertyList(
                                                      MappedAudioFader::Channels)[0].toInt();

                RealTime bufferLength = getAudioReadBufferLength();
                size_t bufferFrames = (size_t)RealTime::realTime2Frame
                    (bufferLength, m_jackDriver->getSampleRate());
                if (bufferFrames % size_t(m_jackDriver->getBufferSize())) {
                    bufferFrames /= size_t(m_jackDriver->getBufferSize());
                    bufferFrames ++;
                    bufferFrames *= size_t(m_jackDriver->getBufferSize());
                }

                //#define DEBUG_PLAYING_AUDIO
#ifdef DEBUG_PLAYING_AUDIO
                RG_DEBUG << "processEventsOut(): Creating playable audio file: id " << audioFile->getId() << ", event time " << (*i)->getEventTime() << ", time now " << getAlsaTime() << ", start marker " << (*i)->getAudioStartMarker() << ", duration " << (*i)->getDuration() << ", instrument " << (*i)->getInstrument() << " channels " << channels;
                RG_DEBUG << "processEventsOut(): Read buffer length is " << bufferLength << " (" << bufferFrames << " frames)";
#endif

                PlayableAudioFile *paf = nullptr;

                try {
                    paf = new PlayableAudioFile((*i)->getInstrument(),
                                                audioFile,
                                                getSequencerTime() +
                                                (RealTime(1, 0) / 4),
                                                (*i)->getAudioStartMarker(),
                                                (*i)->getDuration(),
                                                bufferFrames,
                                                getSmallFileSize() * 1024,
                                                channels,
                                                m_jackDriver->getSampleRate());
                } catch (...) {
                    continue;
                }

                if ((*i)->isAutoFading()) {
                    paf->setAutoFade(true);
                    paf->setFadeInTime((*i)->getFadeInTime());
                    paf->setFadeOutTime((*i)->getFadeInTime());

                    //#define DEBUG_AUTOFADING
#ifdef DEBUG_AUTOFADING

                    RG_DEBUG << "processEventsOut(): PlayableAudioFile is AUTOFADING - "
                             << "in = " << (*i)->getFadeInTime()
                             << ", out = " << (*i)->getFadeOutTime();
#endif

                }
#ifdef DEBUG_AUTOFADING
                else {
                    RG_DEBUG << "processEventsOut(): PlayableAudioFile has no AUTOFADE";
                }
#endif


                // segment runtime id
                paf->setRuntimeSegmentId((*i)->getRuntimeSegmentId());

                m_audioQueue->addUnscheduled(paf);

                haveNewAudio = true;
            } else {
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Can't find audio file reference.";
                RG_DEBUG << "processEventsOut(): Try reloading the current Rosegarden file.";
#else
                ;
#endif

            }
        }

        // Cancel a playing audio file preview (this is predicated on
        // runtime segment ID and optionally start time)
        //
        if ((*i)->getType() == MappedEvent::AudioCancel) {
            cancelAudioFile(*i);
        }
#endif // HAVE_LIBJACK

        if ((*i)->getType() == MappedEvent::SystemMIDIClock) {
            switch ((int)(*i)->getData1()) {
            case 0:  // MIDI Clock and System messages: Off
                m_midiClockEnabled = false;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden MIDI CLOCK, START and STOP DISABLED";
#endif

                setMIDISyncStatus(TRANSPORT_OFF);
                break;

            case 1:  // MIDI Clock and System messages: Send MIDI Clock, Start and Stop
                m_midiClockEnabled = true;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden send MIDI CLOCK, START and STOP ENABLED";
#endif

                setMIDISyncStatus(TRANSPORT_SOURCE);
                break;

            case 2:  // MIDI Clock and System messages: Accept Start, Stop and Continue
                m_midiClockEnabled = false;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden accept START and STOP ENABLED";
#endif

                setMIDISyncStatus(TRANSPORT_FOLLOWER);
                break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemMIDISyncAuto) {
            if ((*i)->getData1()) {
                m_midiSyncAutoConnect = true;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden MIDI SYNC AUTO ENABLED";
#endif

                for (DevicePortMap::iterator dpmi = m_devicePortMap.begin();
                     dpmi != m_devicePortMap.end(); ++dpmi) {
                    snd_seq_connect_to(m_midiHandle,
                                       m_syncOutputPort,
                                       dpmi->second.client,
                                       dpmi->second.port);
                }
            } else {
                m_midiSyncAutoConnect = false;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden MIDI SYNC AUTO DISABLED";
#endif
            }
        }

#ifdef HAVE_LIBJACK
        // Set the JACK transport
        if ((*i)->getType() == MappedEvent::SystemJackTransport) {
            bool enabled = false;
            bool source = false;

            switch ((int)(*i)->getData1()) {
            case 2:
                source = true;
                enabled = true;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden to follow JACK transport and request JACK timebase master role (not yet implemented)";
#endif
                break;

            case 1:
                enabled = true;
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden to follow JACK transport";
#endif
                break;

            case 0:
            default:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden to ignore JACK transport";
#endif
                break;
            }

            if (m_jackDriver) {
                m_jackDriver->setTransportEnabled(enabled);
                m_jackDriver->setTransportSource(source);
            }
        }
#endif // HAVE_LIBJACK


        if ((*i)->getType() == MappedEvent::SystemMMCTransport) {
            switch ((int)(*i)->getData1()) {
            case 1:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden is MMC SOURCE";
#endif

                setMMCStatus(TRANSPORT_SOURCE);
                break;

            case 2:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden is MMC FOLLOWER";
#endif
                setMMCStatus(TRANSPORT_FOLLOWER);
                break;

            case 0:
            default:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden MMC Transport DISABLED";
#endif

                setMMCStatus(TRANSPORT_OFF);
                break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemMTCTransport) {
            switch ((int)(*i)->getData1()) {
            case 1:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden is MTC SOURCE";
#endif

                setMTCStatus(TRANSPORT_SOURCE);
                tweakSkewForMTC(0);
                m_mtcFirstTime = -1;
                break;

            case 2:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden is MTC FOLLOWER";
#endif

                setMTCStatus(TRANSPORT_FOLLOWER);
                m_mtcFirstTime = -1;
                break;

            case 0:
            default:
#ifdef DEBUG_ALSA
                RG_DEBUG << "processEventsOut(): Rosegarden MTC Transport DISABLED";
#endif

                setMTCStatus(TRANSPORT_OFF);
                m_mtcFirstTime = -1;
                break;
            }
        }

        //if ((*i)->getType() == MappedEvent::SystemAudioPortCounts) {
            // never actually used, I think?
        //}

        if ((*i)->getType() == MappedEvent::SystemAudioPorts) {
#ifdef HAVE_LIBJACK
            if (m_jackDriver) {
                int data = (*i)->getData1();
                m_jackDriver->setAudioPorts(data & MappedEvent::FaderOuts,
                                            data & MappedEvent::SubmasterOuts);
            }
#else
#ifdef DEBUG_ALSA
            RG_DEBUG << "processEventsOut(): MappedEvent::SystemAudioPorts - no audio subsystem";
#endif
#endif

        }

        if ((*i)->getType() == MappedEvent::SystemAudioFileFormat) {
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
                RG_DEBUG << "processEventsOut(): MappedEvent::SystemAudioFileFormat - unexpected format number " << format;
#endif

                break;
            }
#else
#ifdef DEBUG_ALSA
            RG_DEBUG << "processEventsOut(): MappedEvent::SystemAudioFileFormat - no audio subsystem";
#endif
#endif

        }

        if ((*i)->getType() == MappedEvent::Panic) {
            for (MappedDeviceList::iterator i = m_devices.begin();
                 i != m_devices.end(); ++i) {
                if ((*i)->getDirection() == MidiDevice::Play) {
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

    clearPendSysExcMap();

    if (recordStatus == RECORD_ON) {
        // start recording
        m_recordStatus = RECORD_ON;
        m_alsaRecordStartTime = RealTime::zeroTime;

        unsigned int audioCount = 0;

        if (armedInstruments) {

            for (size_t i = 0; i < armedInstruments->size(); ++i) {

                InstrumentId id = (*armedInstruments)[i];

                m_recordingInstruments.insert(id);
                if (!audioFileNames || (audioCount >= (unsigned int)audioFileNames->size())) {
                    continue;
                }

                QString fileName = (*audioFileNames)[audioCount];

                if (id >= AudioInstrumentBase &&
                    id < MidiInstrumentBase) {

                    bool good = false;

#ifdef DEBUG_ALSA
                    RG_DEBUG << "record(): Requesting new record file \"" << fileName << "\" for instrument " << id;
#endif

#ifdef HAVE_LIBJACK
                    if (m_jackDriver &&
                        m_jackDriver->openRecordFile(id, fileName)) {
                        good = true;
                    }
#endif

                    if (!good) {
                        m_recordStatus = RECORD_OFF;
                        RG_WARNING << "record(): No JACK driver, or JACK driver failed to prepare for recording audio";
                        return false;
                    }

                    ++audioCount;
                }
            }
        }
    } else
        if (recordStatus == RECORD_OFF) {
            m_recordStatus = RECORD_OFF;
        }
#ifdef DEBUG_ALSA
        else {
            RG_DEBUG << "record(): unsupported recording mode";
        }
#endif

    return true;
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair( -1, -1);
    AlsaPortVector::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it) {
        destPair.client = (*it)->m_client;
        destPair.port = (*it)->m_port;

        // If duplex port is required then choose first one
        //
        if (duplex) {
            if ((*it)->m_direction == Duplex)
                return destPair;
        } else {
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
    if (instrument) {
        DeviceId device = instrument->getDevice();
        DevicePortMap::iterator i = m_devicePortMap.find(device);
        if (i != m_devicePortMap.end()) {
            return i->second;
        }
    }
#ifdef DEBUG_ALSA
    /*
      else
      {
      RG_DEBUG << "getPairForMappedInstrument(): WARNING: couldn't find instrument for id " << id << ", falling through";
      }
    */
#endif

    return ClientPortPair( -1, -1);
}

int
AlsaDriver::getOutputPortForMappedInstrument(InstrumentId id)
{
    MappedInstrument *instrument = getMappedInstrument(id);
    if (instrument) {
        DeviceId device = instrument->getDevice();
        DeviceIntMap::iterator i = m_outputPorts.find(device);
        if (i != m_outputPorts.end()) {
            return i->second;
        }
#ifdef DEBUG_ALSA
        else {
            RG_DEBUG << "getOutputPortForMappedInstrument(): WARNING: couldn't find output port for device for instrument " << id << ", falling through";
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
    if (dimi == m_outputPorts.end())
        return ;

    snd_seq_ev_set_source(&event, dimi->second);
    snd_seq_ev_set_direct(&event);

    for (int i = 0; i < 16; i++) {
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
    RG_DEBUG << "isRecording() begin...";

    if (port->isReadable()) {

        snd_seq_query_subscribe_t *qSubs;
        snd_seq_addr_t rg_addr, sender_addr;
        snd_seq_query_subscribe_alloca(&qSubs);

        rg_addr.client = m_client;
        rg_addr.port = m_inputPort;

        snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
        snd_seq_query_subscribe_set_index(qSubs, 0);
        snd_seq_query_subscribe_set_root(qSubs, &rg_addr);

        while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0) {
            sender_addr = *snd_seq_query_subscribe_get_addr(qSubs);
            if (sender_addr.client == port->m_client &&
                sender_addr.port == port->m_port) {
                RG_DEBUG << "isRecording(): returning true";
                return true;
            }
            snd_seq_query_subscribe_set_index(qSubs,
                                              snd_seq_query_subscribe_get_index(qSubs) + 1);
        }
    }
    RG_DEBUG << "isRecording(): returning false";
    return false;
}

bool
AlsaDriver::checkForNewClients()
{
    // bool madeChange = false; 

#ifdef DEBUG_ALSA
    RG_DEBUG << "checkForNewClients() begin...";
#endif

    if (!m_portCheckNeeded) return false;

#ifdef DEBUG_ALSA
    RG_DEBUG << "checkForNewClients(): port check needed";
#endif

    AlsaPortVector newPorts;
    generatePortList(&newPorts); // updates m_alsaPorts, returns new ports as well

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
                otherEnd->client == j->second.client &&
                otherEnd->port == j->second.port) {
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
                    j->second = ClientPortPair( -1, -1);
                    setConnectionToDevice(**i, "");
                    // madeChange = true;
                }
            } else {
                for (AlsaPortVector::iterator k = m_alsaPorts.begin();
                     k != m_alsaPorts.end(); ++k) {
                    if ((*k)->m_client == firstOther.client &&
                        (*k)->m_port == firstOther.port) {
                        m_devicePortMap[(*i)->getId()] = firstOther;
                        setConnectionToDevice(**i, (*k)->m_name.c_str(), firstOther);
                        // madeChange = true;
                        break;
                    }
                }
            }
        }
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
    RG_DEBUG << "setRecordDevice(): device " << id << ", action " << connectAction;

    // Locate a suitable port
    //
    if (m_devicePortMap.find(id) == m_devicePortMap.end()) {
#ifdef DEBUG_ALSA
        RG_DEBUG << "setRecordDevice() - couldn't match device id (" << id << ") to ALSA port";
#endif

        return ;
    }

    ClientPortPair pair = m_devicePortMap[id];

    RG_DEBUG << "setRecordDevice(): port is " << pair.client << ":" << pair.port;

    snd_seq_addr_t sender, dest;
    sender.client = pair.client;
    sender.port = pair.port;

    MappedDevice *device = nullptr;

    for (MappedDeviceList::iterator i = m_devices.begin();
         i != m_devices.end(); ++i) {
        if ((*i)->getId() == id) {
            device = *i;
            if (device->getDirection() == MidiDevice::Record) {
                if (device->isRecording() && connectAction) {
#ifdef DEBUG_ALSA
                    RG_DEBUG << "setRecordDevice() - attempting to subscribe (" << id << ") already subscribed";
#endif
                    return ;
                }
                if (!device->isRecording() && !connectAction) {
#ifdef DEBUG_ALSA
                    RG_DEBUG << "setRecordDevice() - attempting to unsubscribe (" << id << ") already unsubscribed";
#endif
                    return ;
                }
            } else {
#ifdef DEBUG_ALSA
                RG_DEBUG << "setRecordDevice() - attempting to set play device (" << id << ") to record device";
#endif
                return ;
            }
            break;
        }
    }

    if (!device) return;

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
                           "setRecordDevice - failed subscription of input port") < 0) {
            // Not the end of the world if this fails but we
            // have to flag it internally.
            //
            AUDIT << "AlsaDriver::setRecordDevice() - "
                  << int(sender.client) << ":" << int(sender.port)
                  << " failed to subscribe device "
                  << id << " as record port\n";
            RG_DEBUG << "setRecordDevice() - "
                  << int(sender.client) << ":" << int(sender.port)
                  << " failed to subscribe device "
                  << id << " as record port";
        } else {
            m_midiInputPortConnected = true;
            AUDIT << "AlsaDriver::setRecordDevice() - successfully subscribed device " << id << " as record port\n";
            RG_DEBUG << "setRecordDevice() - successfully subscribed device " << id << " as record port";
            device->setRecording(true);
        }
    } else {
        if (checkAlsaError(snd_seq_unsubscribe_port(m_midiHandle, subs),
                           "setRecordDevice - failed to unsubscribe a device") == 0) {
            AUDIT << "AlsaDriver::setRecordDevice() - "
                  << "successfully unsubscribed device "
                  << id << " as record port\n";
            RG_DEBUG << "setRecordDevice() - "
                  << "successfully unsubscribed device "
                  << id << " as record port";
            device->setRecording(false);
        }
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

    // Unsubscribe any existing connections
    //
    snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(qSubs, 0);
    snd_seq_query_subscribe_set_root(qSubs, &tmp_addr);

    while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0) {
        tmp_addr = *snd_seq_query_subscribe_get_addr(qSubs);

        snd_seq_port_subscribe_t *dSubs;
        snd_seq_port_subscribe_alloca(&dSubs);

        snd_seq_addr_t dSender;
        dSender.client = tmp_addr.client;
        dSender.port = tmp_addr.port;

        snd_seq_port_subscribe_set_sender(dSubs, &dSender);
        snd_seq_port_subscribe_set_dest(dSubs, &dest);

        int error = snd_seq_unsubscribe_port(m_midiHandle, dSubs);

        if (error < 0) {
#ifdef DEBUG_ALSA
            RG_DEBUG << "unsetRecordDevices() - can't unsubscribe record port";
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
AlsaDriver::sendSystemDirect(MidiByte command, int *args)
{
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_syncOutputPort);
    snd_seq_ev_set_subs(&event);

    event.queue = SND_SEQ_QUEUE_DIRECT;

    // set the command
    event.type = command;

    // set args if we have them
    if (args) {
        event.data.control.value = *args;
    }

    int error = snd_seq_event_output_direct(m_midiHandle, &event);

    if (error < 0) {
#ifdef DEBUG_ALSA
        RG_DEBUG << "sendSystemDirect() - can't send event (" << int(command) << ")";
#endif

    }

    //    checkAlsaError(snd_seq_drain_output(m_midiHandle),
    //           "sendSystemDirect(): draining");
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

    snd_seq_real_time_t sendTime =
        { (unsigned int)time.sec, (unsigned int)time.nsec };

    // Schedule the command
    //
    event.type = command;

    snd_seq_ev_schedule_real(&event, m_queue, 0, &sendTime);

    // set args if we have them
    switch (args.length()) {
    case 1:
        event.data.control.value = args[0];
        break;

    case 2:
        event.data.control.value = int(args[0]) | (int(args[1]) << 7);
        break;

    default:  // do nothing
        break;
    }

    int error = snd_seq_event_output(m_midiHandle, &event);

    if (error < 0) {
#ifdef DEBUG_ALSA
        RG_DEBUG << "sendSystemQueued() - "
                 << "can't send event (" << int(command) << ")"
                 << " - error = (" << error << ")";
#endif

    }

    //    if (m_queueRunning) {
    //    checkAlsaError(snd_seq_drain_output(m_midiHandle), "sendSystemQueued(): draining");
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
    return strtoqstr(AUDIT.str());
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
#ifdef DEBUG_ALSA

        unsigned int t_base = snd_seq_queue_tempo_get_skew_base(q_ptr);
        if (!m_playing) {
            RG_DEBUG << "runTasks(): Skew: " << t_skew << "/" << t_base;
        }
#endif

        unsigned int newSkew = t_skew + (unsigned int)(t_skew * ratio);

        if (newSkew != t_skew) {
#ifdef DEBUG_ALSA
            if (!m_playing) {
                RG_DEBUG << "runTasks():     changed to " << newSkew;
            }
#endif
            snd_seq_queue_tempo_set_skew(q_ptr, newSkew);
            snd_seq_set_queue_tempo( m_midiHandle, m_queue, q_ptr);
        }

        m_firstTimerCheck = true;
    }

#endif
}

void
AlsaDriver::reportFailure(MappedEvent::FailureCode code)
{
    //#define REPORT_XRUNS 1
#ifndef REPORT_XRUNS
    if (code == MappedEvent::FailureXRuns ||
        code == MappedEvent::FailureDiscUnderrun ||
        code == MappedEvent::FailureBussMixUnderrun ||
        code == MappedEvent::FailureMixUnderrun) {
        return ;
    }
#endif

    // Ignore consecutive duplicates
    if (failureReportWriteIndex > 0 &&
        failureReportWriteIndex != failureReportReadIndex) {
        if (code == failureReports[failureReportWriteIndex - 1])
            return ;
    }

    failureReports[failureReportWriteIndex] = code;
    failureReportWriteIndex =
        (failureReportWriteIndex + 1) % FAILURE_REPORT_COUNT;
}

std::string
AlsaDriver::getAlsaModuleVersionString()
{
    FILE *v = fopen("/proc/asound/version", "r");

    // Examples:
    // Advanced Linux Sound Architecture Driver Version 1.0.14rc3.
    // Advanced Linux Sound Architecture Driver Version 1.0.14 (Thu May 31 09:03:25 2008 UTC).

    if (v) {
        char buf[256];
        if (fgets(buf, 256, v) == nullptr) {
            fclose(v);
            return "(unknown)"; /* check fgets result */
        }
        fclose(v);

        std::string vs(buf);
        std::string::size_type sp = vs.find_first_of('.');
        if (sp > 0 && sp != std::string::npos) {
            while (sp > 0 && isdigit(vs[sp-1])) --sp;
            vs = vs.substr(sp);
            if (vs.length() > 0 && vs[vs.length()-1] == '\n') {
                vs = vs.substr(0, vs.length()-1);
            }
            if (vs.length() > 0 && vs[vs.length()-1] == '.') {
                vs = vs.substr(0, vs.length()-1);
            }
            return vs;
        }
    }

    return "(unknown)";
}

std::string
AlsaDriver::getKernelVersionString()
{
    FILE *v = fopen("/proc/version", "r");

    if (v) {
        char buf[256];
        if (fgets(buf, 256, v) == nullptr) {
            fclose(v);
            return "(unknown)"; /* check fgets result */
        }
        fclose(v);

        std::string vs(buf);
        std::string key(" version ");
        std::string::size_type sp = vs.find(key);
        if (sp != std::string::npos) {
            vs = vs.substr(sp + key.length());
            sp = vs.find(' ');
            if (sp != std::string::npos) {
                vs = vs.substr(0, sp);
            }
            if (vs.length() > 0 && vs[vs.length()-1] == '\n') {
                vs = vs.substr(0, vs.length()-1);
            }
            return vs;
        }
    }

    return "(unknown)";
}

void
AlsaDriver::extractVersion(std::string v, int &major, int &minor, int &subminor, std::string &suffix)
{
    major = minor = subminor = 0;
    suffix = "";
    if (v == "(unknown)") return;

    std::string::size_type sp, pp;

    sp = v.find('.');
    if (sp == std::string::npos) goto done;
    major = atoi(v.substr(0, sp).c_str());
    pp = sp + 1;

    sp = v.find('.', pp);
    if (sp == std::string::npos) goto done;
    minor = atoi(v.substr(pp, sp - pp).c_str());
    pp = sp + 1;

    while (++sp < v.length() && (::isdigit(v[sp]) || v[sp] == '-')) { }
    subminor = atoi(v.substr(pp, sp - pp).c_str());

    if (sp >= v.length()) goto done;
    suffix = v.substr(sp);

done:
    RG_DEBUG << "extractVersion(): major = " << major << ", minor = " << minor << ", subminor = " << subminor << ", suffix = \"" << suffix << "\"";
}

bool
AlsaDriver::versionIsAtLeast(std::string v, int major, int minor, int subminor)
{
    int actualMajor, actualMinor, actualSubminor;
    std::string actualSuffix;

    extractVersion(v, actualMajor, actualMinor, actualSubminor, actualSuffix);

    bool ok = false;

    if (actualMajor > major) {
        ok = true;
    } else if (actualMajor == major) {
        if (actualMinor > minor) {
            ok = true;
        } else if (actualMinor == minor) {
            if (actualSubminor > subminor) {
                ok = true;
            } else if (actualSubminor == subminor) {
                if (strncmp(actualSuffix.c_str(), "rc", 2) &&
                    strncmp(actualSuffix.c_str(), "pre", 3)) {
                    ok = true;
                }
            }
        }
    }

    RG_DEBUG << "versionIsAtLeast(): is version " << v << " at least " << major << "." << minor << "." << subminor << "? " << (ok ? "yes" : "no");
    return ok;
}    

bool
AlsaDriver::throttledDebug() const
{
    if (!m_debug)
        return false;

    static bool active = true;
    static int count = 0;
    static QTime timeout;

    // if we are active
    if (active) {
        ++count;
        // if we've done too many
        if (count > 5) {
            active = false;
            timeout = QTime::currentTime().addSecs(5);
            return false;
        }
        return true;
    } else {
        // if current time > timeout
        if (QTime::currentTime() > timeout) {
            active = true;
            count = 1;
            return true;
        }
        return false;
    }

    return false;
}

bool
AlsaDriver::portInUse(int client, int port) const
{
    // If the client/port is in m_devicePortMap, then it
    // is in use.  See setConnectionToDevice().

    for (DevicePortMap::const_iterator dpmi =
             m_devicePortMap.begin();
         dpmi != m_devicePortMap.end();
         ++dpmi) {
        if (dpmi->second.client == client  &&
            dpmi->second.port == port) {
            return true;
        }
    }

    // Not found, so not in use.
    return false;
}


}


#endif // HAVE_ALSA
