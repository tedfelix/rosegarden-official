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

#define RG_MODULE_STRING "[JackDriver]"

#include "JackDriver.h"
#include "AlsaDriver.h"
#include "MappedStudio.h"
#include "AudioProcess.h"
#include "base/Profiler.h"
#include "base/AudioLevel.h"
#include "Audit.h"
#include "PluginFactory.h"
#include "SequencerDataBlock.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"

#include <QSettings>
#include <QtGlobal>

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

//#define DEBUG_JACK_DRIVER 1
//#define DEBUG_JACK_TRANSPORT 1
//#define DEBUG_JACK_PROCESS 1
//#define DEBUG_JACK_XRUN 1

namespace Rosegarden
{

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
static unsigned long framesThisPlay = 0;
static RealTime startTime;
#endif

JackDriver::JackDriver(AlsaDriver *alsaDriver) :
        m_client(nullptr),
        m_bufferSize(0),
        m_sampleRate(0),
        m_tempOutBuffer(nullptr),
        m_jackTransportEnabled(false),
        m_jackTransportSource(false),
        m_waiting(false),
        m_waitingState(JackTransportStopped),
        m_waitingToken(0),
        m_ignoreProcessTransportCount(0),
        m_bussMixer(nullptr),
        m_instrumentMixer(nullptr),
        m_fileReader(nullptr),
        m_fileWriter(nullptr),
        m_alsaDriver(alsaDriver),
        m_masterLevel(1.0),
        m_directMasterAudioInstruments(0L),
        m_directMasterSynthInstruments(0L),
        m_haveAsyncAudioEvent(false),
        m_kickedOutAt(0),
        m_framesProcessed(0),
        m_ok(false)
{
    Q_ASSERT(sizeof(sample_t) == sizeof(float));
    initialise();
}

JackDriver::~JackDriver()
{
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor";
#endif

    m_ok = false; // prevent any more work in process()

    if (m_client) {
#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "dtor: deactivating JACK client";
#endif

        if (jack_deactivate(m_client)) {
            RG_WARNING << "dtor: WARNING: deactivation failed";
        }
    }

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: terminating buss mixer";
#endif

    AudioBussMixer *bussMixer = m_bussMixer;
    m_bussMixer = nullptr;
    if (bussMixer)
        bussMixer->terminate();

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: terminating instrument mixer";
#endif

    AudioInstrumentMixer *instrumentMixer = m_instrumentMixer;
    m_instrumentMixer = nullptr;
    if (instrumentMixer) {
        instrumentMixer->terminate();
        instrumentMixer->destroyAllPlugins();
    }

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: terminating file reader";
#endif

    AudioFileReader *reader = m_fileReader;
    m_fileReader = nullptr;
    if (reader)
        reader->terminate();

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: terminating file writer";
#endif

    AudioFileWriter *writer = m_fileWriter;
    m_fileWriter = nullptr;
    if (writer)
        writer->terminate();

    if (m_client) {

#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "dtor: tearing down JACK client";
#endif

        for (size_t i = 0; i < m_inputPorts.size(); ++i) {
#ifdef DEBUG_JACK_DRIVER
            RG_DEBUG << "dtor: unregistering input " << i;
#endif

            if (jack_port_unregister(m_client, m_inputPorts[i])) {
                RG_WARNING << "dtor: WARNING: can't unregister input port " << i + 1;
            }
        }

        for (size_t i = 0; i < m_outputSubmasters.size(); ++i) {
#ifdef DEBUG_JACK_DRIVER
            RG_DEBUG << "dtor: unregistering output sub " << i;
#endif

            if (jack_port_unregister(m_client, m_outputSubmasters[i])) {
                RG_WARNING << "dtor: WARNING: can't unregister output submaster " << i + 1;
            }
        }

        for (size_t i = 0; i < m_outputMonitors.size(); ++i) {
#ifdef DEBUG_JACK_DRIVER
            RG_DEBUG << "dtor: unregistering output mon " << i;
#endif

            if (jack_port_unregister(m_client, m_outputMonitors[i])) {
                RG_WARNING << "dtor: WARNING: can't unregister output monitor " << i + 1;
            }
        }

        for (size_t i = 0; i < m_outputMasters.size(); ++i) {
#ifdef DEBUG_JACK_DRIVER
            RG_DEBUG << "dtor: unregistering output master " << i;
#endif

            if (jack_port_unregister(m_client, m_outputMasters[i])) {
                RG_WARNING << "dtor: WARNING: can't unregister output master " << i + 1;
            }
        }

#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "dtor: closing client";
#endif

        jack_client_close(m_client);
        RG_DEBUG << "dtor: done";
        m_client = nullptr;
    }

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: deleting mixers etc";
#endif

    delete bussMixer;
    delete instrumentMixer;
    delete reader;
    delete writer;

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "dtor: exiting";
#endif
}

void
JackDriver::initialise(bool reinitialise)
{
    m_ok = false;

    // Create a log that the user can easily see through the preferences
    // even in a release build.
    AUDIT << '\n';
    AUDIT << "===============================================================\n";
    AUDIT << "JackDriver::initialise() begin...\n";
    RG_DEBUG << "initialise() begin...";

    std::string jackClientName = "rosegarden";

    // set up JackOpenOptions per user config
    QSettings settings;
    settings.beginGroup(SequencerOptionsConfigGroup);
    bool autoStartJack = settings.value("autostartjack", true).toBool();
    settings.endGroup();
    // default is to auto start JACK; use JackNullOption
    jack_options_t jackOptions = JackNullOption;
    if (!autoStartJack) jackOptions = JackNoStartServer;

    // attempt connection to JACK server
    //
    if ((m_client = jack_client_open(jackClientName.c_str(), jackOptions, nullptr)) == nullptr) {
        RG_WARNING << "initialise() - JACK server not running";
        RG_WARNING << "  Attempt to start JACK server was " << (jackOptions & JackNoStartServer ? "NOT " : "") << "made per user config";
        // Also send to user log.
        AUDIT << "JACK server not running\n";
        AUDIT << "  Attempt to start JACK server was " << (jackOptions & JackNoStartServer ? "NOT " : "") << "made per user config\n";
        return ;
    }

    InstrumentId instrumentBase;
    int instrumentCount;
    m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);
    for (InstrumentId id = instrumentBase;
            id < instrumentBase + instrumentCount; ++id) {
        // prefill so that we can refer to the map without a lock (as
        // the number of instruments won't change)
        m_recordInputs[id] = RecordInputDesc(1000, -1, 0.0);
    }

    // set callbacks
    //
    jack_set_process_callback(m_client, jackProcessStatic, this);
    jack_set_buffer_size_callback(m_client, jackBufferSize, this);
    jack_set_sample_rate_callback(m_client, jackSampleRate, this);
    jack_on_shutdown(m_client, jackShutdown, this);
    jack_set_xrun_callback(m_client, jackXRun, this);
    jack_set_sync_callback(m_client, jackSyncCallback, this);

    // get and report the sample rate and buffer size
    //
    m_sampleRate = jack_get_sample_rate(m_client);
    m_bufferSize = jack_get_buffer_size(m_client);

    RG_DEBUG << "initialise() - JACK sample rate = " << m_sampleRate << "Hz, buffer size = " << m_bufferSize;
    AUDIT << "JACK sample rate = " << m_sampleRate << "Hz, buffer size = " << m_bufferSize << '\n';

    PluginFactory::setSampleRate(m_sampleRate);

    // Get the initial buffer size before we activate the client
    //

    if (!reinitialise) {

        // create processing buffer(s)
        //
        m_tempOutBuffer = new sample_t[m_bufferSize];

        RG_DEBUG << "initialise() - creating disk thread...";
        AUDIT << "Creating audio file thread...\n";

        m_fileReader = new AudioFileReader(m_alsaDriver, m_sampleRate);
        m_fileWriter = new AudioFileWriter(m_alsaDriver, m_sampleRate);
        m_instrumentMixer = new AudioInstrumentMixer
                            (m_alsaDriver, m_fileReader, m_sampleRate, m_bufferSize);
        m_bussMixer = new AudioBussMixer
                      (m_alsaDriver, m_instrumentMixer, m_sampleRate, m_bufferSize);
        m_instrumentMixer->setBussMixer(m_bussMixer);

        // We run the file reader whatever, but we only run the other
        // threads (instrument mixer, buss mixer, file writer) when we
        // actually need them.  (See updateAudioData and createRecordFile.)
        m_fileReader->run();
    }

    // Create and connect the default numbers of ports.  We always create
    // one stereo pair each of master and monitor outs, and then we create
    // record ins, fader outs and submaster outs according to the user's
    // preferences.  Since we don't know the user's preferences yet, we'll
    // start by creating one pair of record ins and no fader or submaster
    // outs.
    //
    m_outputMasters.clear();
    m_outputMonitors.clear();
    m_outputSubmasters.clear();
    m_outputInstruments.clear();
    m_inputPorts.clear();

    if (!createMainOutputs()) { // one stereo pair master, one pair monitor
        RG_WARNING << "initialise() - failed to create main outputs!";
        AUDIT << "WARNING: failed to create main outputs!\n";
        return ;
    }

    if (!createRecordInputs(1)) {
        RG_WARNING << "initialise() - failed to create record inputs!";
        AUDIT << "WARNING: failed to create record inputs!\n";
        return ;
    }

    if (jack_activate(m_client)) {
        RG_WARNING << "initialise() - client activation failed";
        AUDIT << "WARNING: client activation failed\n";
        return ;
    }

    // Now set up the default connections, if configured to do so
    settings.beginGroup(SequencerOptionsConfigGroup);
    bool connectDefaultOutputs = settings.value("connect_default_jack_outputs", true).toBool();
    bool connectDefaultInputs = settings.value("connect_default_jack_inputs", true).toBool();
    settings.endGroup();

    const char **ports = jack_get_ports(m_client, nullptr, nullptr,
            JackPortIsPhysical | JackPortIsInput);
    
    if (connectDefaultOutputs) {

        std::string playback_1, playback_2;

        if (ports) {
            if (ports[0])
                playback_1 = std::string(ports[0]);
            if (ports[1])
                playback_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++)
                ;
            RG_DEBUG << "initialise() - found " << i << " JACK physical outputs";
            AUDIT << "Found " << i << " JACK physical outputs\n";

            jack_free(ports);

        } else {
            RG_WARNING << "initialise() - no JACK physical outputs found";
            AUDIT << "WARNING: no JACK physical outputs found\n";
        }

        if (playback_1 != "") {
            RG_DEBUG << "initialise() - connecting from " << "\"" << jack_port_name(m_outputMasters[0]) << "\" to \"" << playback_1.c_str() << "\"";
            AUDIT << "connecting from " << "\"" << jack_port_name(m_outputMasters[0]) << "\" to \"" << playback_1.c_str() << "\"\n";

            // connect our client up to the ALSA ports - first left output
            //
            if (jack_connect(m_client, jack_port_name(m_outputMasters[0]),
                             playback_1.c_str())) {
                RG_WARNING << "initialise() - cannot connect to JACK output port";
                AUDIT << "WARNING: cannot connect to JACK output port\n";
                return ;
            }

            /*
                    // ??? monitors?
                    if (jack_connect(m_client, jack_port_name(m_outputMonitors[0]),
                                     playback_1.c_str()))
                    {
                        RG_WARNING << "initialise() - cannot connect to JACK output port";
                        audit << "WARNING: cannot connect to JACK output port\n";
                        return;
                    }
            */
        }

        if (playback_2 != "") {
            RG_DEBUG << "initialise() - connecting from " << "\"" << jack_port_name(m_outputMasters[1]) << "\" to \"" << playback_2.c_str() << "\"";
            AUDIT << "WARNING: connecting from " << "\"" << jack_port_name(m_outputMasters[1]) << "\" to \"" << playback_2.c_str() << "\"\n";

            if (jack_connect(m_client, jack_port_name(m_outputMasters[1]),
                             playback_2.c_str())) {
                RG_WARNING << "initialise() - cannot connect to JACK output port";
                AUDIT << "WARNING: cannot connect to JACK output port\n";
            }

            /*
                    // ??? monitors?
                    if (jack_connect(m_client, jack_port_name(m_outputMonitors[1]),
                                     playback_2.c_str()))
                    {
                        RG_WARNING << "initialise() - cannot connect to JACK output port";
                        audit << "WARNING: cannot connect to JACK output port\n";
                    }
            */
        }

    }

    if (connectDefaultInputs) {

        std::string capture_1, capture_2;

        ports =
            jack_get_ports(m_client, nullptr, nullptr,
                           JackPortIsPhysical | JackPortIsOutput);

        if (ports) {
            if (ports[0])
                capture_1 = std::string(ports[0]);
            if (ports[1])
                capture_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++)
                ;
            RG_DEBUG << "initialise() - found " << i << " JACK physical inputs";
            AUDIT << "found " << i << " JACK physical inputs\n";

            jack_free(ports);

        } else {
            RG_WARNING << "initialise() - no JACK physical inputs found";
            AUDIT << "WARNING: no JACK physical inputs found\n";
        }

        if (capture_1 != "") {

            RG_DEBUG << "initialise() - connecting from " << "\"" << capture_1.c_str() << "\" to \"" << jack_port_name(m_inputPorts[0]) << "\"";
            AUDIT << "connecting from " << "\"" << capture_1.c_str() << "\" to \"" << jack_port_name(m_inputPorts[0]) << "\"\n";

            if (jack_connect(m_client, capture_1.c_str(),
                             jack_port_name(m_inputPorts[0]))) {
                RG_WARNING << "initialise() - cannot connect to JACK input port";
                AUDIT << "WARNING: cannot connect to JACK input port\n";
            }
        }

        if (capture_2 != "") {

            RG_DEBUG << "initialise() - connecting from " << "\"" << capture_2.c_str() << "\" to \"" << jack_port_name(m_inputPorts[1]) << "\"";
            AUDIT << "connecting from " << "\"" << capture_2.c_str() << "\" to \"" << jack_port_name(m_inputPorts[1]) << "\"\n";

            if (jack_connect(m_client, capture_2.c_str(),
                             jack_port_name(m_inputPorts[1]))) {
                RG_WARNING << "initialise() - cannot connect to JACK input port";
                AUDIT << "WARNING: cannot connect to JACK input port\n";
            }
        }
    }

    RG_DEBUG << "initialise() - initialised JACK audio subsystem";
    AUDIT << "initialised JACK audio subsystem\n";

    m_ok = true;
}

bool
JackDriver::createMainOutputs()
{
    if (!m_client)
        return false;

    jack_port_t *port = jack_port_register
                        (m_client, "master out L",
                         JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;
    m_outputMasters.push_back(port);

    port = jack_port_register
           (m_client, "master out R",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;
    m_outputMasters.push_back(port);

    port = jack_port_register
           (m_client, "record monitor out L",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;
    m_outputMonitors.push_back(port);

    port = jack_port_register
           (m_client, "record monitor out R",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    if (!port)
        return false;
    m_outputMonitors.push_back(port);

    return true;
}

bool
JackDriver::createFaderOutputs(int audioPairs, int synthPairs)
{
    if (!m_client)
        return false;

    int pairs = audioPairs + synthPairs;
    int pairsNow = int(m_outputInstruments.size()) / 2;
    if (pairs == pairsNow)
        return true;

    for (int i = pairsNow; i < pairs; ++i) {

        QString portName;

        if (i < audioPairs) {
            portName = QString("audio fader %1 out L").arg(i+1);
        } else {
            portName = QString("synth fader %1 out L").arg(i - audioPairs + 1);
        }

        jack_port_t *port;

        port = jack_port_register(m_client,
                                  portName.toLocal8Bit().constData(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsOutput,
                                  0);
        if (!port)
            return false;
        m_outputInstruments.push_back(port);

        if (i < audioPairs) {
            portName = QString("audio fader %1 out R").arg(i+1);
        } else {
            portName = QString("synth fader %1 out R").arg(i - audioPairs + 1);
        }

        port = jack_port_register(m_client,
                                  portName.toLocal8Bit().constData(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsOutput,
                                  0);
        if (!port)
            return false;
        m_outputInstruments.push_back(port);
    }

    while ((int)m_outputInstruments.size() > pairs * 2) {
        std::vector<jack_port_t *>::iterator itr = m_outputInstruments.end();
        --itr;
        jack_port_unregister(m_client, *itr);
        m_outputInstruments.erase(itr);
    }

    return true;
}

bool
JackDriver::createSubmasterOutputs(int pairs)
{
    if (!m_client)
        return false;

    int pairsNow = int(m_outputSubmasters.size()) / 2;
    if (pairs == pairsNow)
        return true;

    for (int i = pairsNow; i < pairs; ++i) {

        QString name;
        jack_port_t *port;

        name = QString("submaster %d out L").arg(i + 1);
        port = jack_port_register(m_client,
                                  name.toLocal8Bit(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsOutput,
                                  0);
        if (!port)
            return false;
        m_outputSubmasters.push_back(port);

        name = QString("submaster %d out R").arg(i + 1);
        port = jack_port_register(m_client,
                                  name.toLocal8Bit(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsOutput,
                                  0);
        if (!port)
            return false;
        m_outputSubmasters.push_back(port);
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
        std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
        --itr;
        jack_port_unregister(m_client, *itr);
        m_outputSubmasters.erase(itr);
    }

    return true;
}

bool
JackDriver::createRecordInputs(int pairs)
{
    if (!m_client)
        return false;

    int pairsNow = int(m_inputPorts.size()) / 2;
    if (pairs == pairsNow)
        return true;

    for (int i = pairsNow; i < pairs; ++i) {

        QString name;
        jack_port_t *port;

        name = QString("record in %1 L").arg(i + 1);
        port = jack_port_register(m_client,
                                  name.toLocal8Bit(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsInput,
                                  0);
        if (!port)
            return false;
        m_inputPorts.push_back(port);

        name = QString("record in %1 R").arg(i + 1);
        port = jack_port_register(m_client,
                                  name.toLocal8Bit(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  JackPortIsInput,
                                  0);
        if (!port)
            return false;
        m_inputPorts.push_back(port);
    }

    while ((int)m_outputSubmasters.size() > pairs * 2) {
        std::vector<jack_port_t *>::iterator itr = m_outputSubmasters.end();
        --itr;
        jack_port_unregister(m_client, *itr);
        m_outputSubmasters.erase(itr);
    }

    return true;
}


void
JackDriver::setAudioPorts(bool faderOuts, bool submasterOuts)
{
    if (!m_client)
        return ;

    // Create a log that the user can easily see through the preferences
    // even in a release build.
    AUDIT << '\n';
    AUDIT << "===============================================================\n";
    AUDIT << "JackDriver::setAudioPorts() begin...\n";

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "setAudioPorts(" << faderOuts << "," << submasterOuts << ")";
#endif

    if (!m_client) {
        RG_WARNING << "setAudioPorts(" << faderOuts << "," << submasterOuts << "): no client yet";
        AUDIT << "WARNING: setAudioPorts(" << faderOuts << "," << submasterOuts << "): no client yet\n";
        return ;
    }

    if (faderOuts) {
        InstrumentId instrumentBase;
        int audioInstruments;
        int synthInstruments;
        m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, audioInstruments);
        m_alsaDriver->getSoftSynthInstrumentNumbers(instrumentBase, synthInstruments);
        if (!createFaderOutputs(audioInstruments, synthInstruments)) {
            m_ok = false;
            RG_WARNING << "setAudioPorts(): Failed to create fader outs!";
            AUDIT << "WARNING: Failed to create fader outs!\n";
            return ;
        }
    } else {
        createFaderOutputs(0, 0);
    }

    if (submasterOuts) {

        // one fewer than returned here, because the master has a buss object too
        int count = m_alsaDriver->getMappedStudio()->getObjectCount(
                MappedObject::AudioBuss);
        if (count == 0) {
            RG_WARNING << "setAudioPorts(): Mapped studio contains no master buss!  Probably a symptom of a serious error";
            AUDIT << "WARNING: Mapped studio contains no master buss!  Probably a symptom of a serious error\n";
        } else {
            count = count - 1;
        }
        if (!createSubmasterOutputs(count)) {
            m_ok = false;
            RG_WARNING << "setAudioPorts(): Failed to create submaster outs!";
            AUDIT << "WARNING(): Failed to create submaster outs!\n";
            return ;
        }

    } else {
        createSubmasterOutputs(0);
    }
}

RealTime
JackDriver::getAudioPlayLatency() const
{
    if (!m_client)
        return RealTime::zeroTime;

#if 0
    // ??? DEPRECATED
    //     jack_port_get_total_latency() is deprecated.  Replace with
    //     jack_port_get_latency_range().

    jack_nframes_t latency =
        jack_port_get_total_latency(m_client, m_outputMasters[0]);

    return RealTime::frame2RealTime(latency, m_sampleRate);
#else
    jack_latency_range_t latencyRange;
    jack_port_get_latency_range(
            m_outputMasters[0], JackPlaybackLatency, &latencyRange);

    // min and max appear to be the same, so we'll just go with max.
    // ??? Might be better to average them?
    return RealTime::frame2RealTime(latencyRange.max, m_sampleRate);
#endif
}

RealTime
JackDriver::getAudioRecordLatency() const
{
    if (!m_client)
        return RealTime::zeroTime;

#if 0
    // ??? DEPRECATED
    //     jack_port_get_total_latency() is deprecated.  Replace with
    //     jack_port_get_latency_range().

    jack_nframes_t latency =
        jack_port_get_total_latency(m_client, m_inputPorts[0]);

    return RealTime::frame2RealTime(latency, m_sampleRate);
#else
    jack_latency_range_t latencyRange;
    jack_port_get_latency_range(
            m_inputPorts[0], JackCaptureLatency, &latencyRange);

    // min and max appear to be the same, so we'll just go with max.
    // ??? Might be better to average them?
    return RealTime::frame2RealTime(latencyRange.max, m_sampleRate);
#endif
}

RealTime
JackDriver::getInstrumentPlayLatency(InstrumentId id) const
{
    if (m_instrumentLatencies.find(id) == m_instrumentLatencies.end()) {
        return RealTime::zeroTime;
    } else {
        return m_instrumentLatencies.find(id)->second;
    }
}

RealTime
JackDriver::getMaximumPlayLatency() const
{
    return m_maxInstrumentLatency;
}

int
JackDriver::jackProcessStatic(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);
    if (inst)
        return inst->jackProcess(nframes);
    else
        return 0;
}

int
JackDriver::jackProcess(jack_nframes_t nframes)
{
    if (!m_ok || !m_client) {
#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcess(): not OK";
#endif

        return 0;
    }

    if (!m_bussMixer) {
#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcess(): no buss mixer";
#endif

        return jackProcessEmpty(nframes);
    }

    // synchronize MIDI and audio by adjusting MIDI playback rate
    if (m_alsaDriver->areClocksRunning()) {
        m_alsaDriver->checkTimerSync(m_framesProcessed);
    } else {
        m_alsaDriver->checkTimerSync(0);
    }

    bool lowLatencyMode = m_alsaDriver->getLowLatencyMode();
    bool clocksRunning = m_alsaDriver->areClocksRunning();
    bool playing = m_alsaDriver->isPlaying();
    bool asyncAudio = m_haveAsyncAudioEvent;

#ifdef DEBUG_JACK_PROCESS
    Profiler profiler("jackProcess", true);
#else
  #ifdef DEBUG_JACK_XRUN
    Profiler profiler("jackProcess", false);
  #endif
#endif

    if (lowLatencyMode) {
        if (clocksRunning) {
            if (playing || asyncAudio) {
                if (m_instrumentMixer->tryLock() == 0) {
                    m_instrumentMixer->kick(false);
                    m_instrumentMixer->releaseLock();
                    //#ifdef DEBUG_JACK_PROCESS
                } else {
                    RG_WARNING << "jackProcess(): WARNING: no instrument mixer lock available";
                    //#endif
                }
                if (m_bussMixer->getBussCount() > 0) {
                    if (m_bussMixer->tryLock() == 0) {
                        m_bussMixer->kick(false, false);
                        m_bussMixer->releaseLock();
                        //#ifdef DEBUG_JACK_PROCESS
                    } else {
                        RG_WARNING << "jackProcess(): WARNING: no buss mixer lock available";
                        //#endif
                    }
                }
            }
        }
    }

    if (jack_cpu_load(m_client) > 97.0) {
        reportFailure(MappedEvent::FailureCPUOverload);
        return jackProcessEmpty(nframes);
    }

#ifdef DEBUG_JACK_PROCESS
    Profiler profiler2("jackProcess post mix", true);
#else
  #ifdef DEBUG_JACK_XRUN
    Profiler profiler2("jackProcess post mix", false);
  #endif
#endif

    jack_position_t position;
    jack_transport_state_t state = JackTransportRolling;
    bool doneRecord = false;

    int ignoreCount = m_ignoreProcessTransportCount;
    if (ignoreCount > 0)
        --m_ignoreProcessTransportCount;

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_alsaDriver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    if (m_jackTransportEnabled) {
        state = jack_transport_query(m_client, &position);

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcess(): JACK transport state is " << state;
#endif

        if (state == JackTransportStopped) {
            if (playing && clocksRunning && !m_waiting) {
                RosegardenSequencer *sequencer =
                        RosegardenSequencer::getInstance();
                if (sequencer) {
#ifdef DEBUG_JACK_TRANSPORT
                    RG_DEBUG << "jackProcess(): JACK transport stopped externally at " << position.frame;
#endif

                    m_waitingToken = sequencer->transportJump(
                            RosegardenSequencer::TransportStopAtTime,
                            RealTime::frame2RealTime(position.frame,
                                                     position.frame_rate));
                }
            } else if (clocksRunning) {
                if (!asyncAudio) {
#ifdef DEBUG_JACK_PROCESS
                    RG_DEBUG << "jackProcess(): no interesting async events";
#endif
                    // do this before record monitor, otherwise we lose monitor out
                    jackProcessEmpty(nframes);
                }

                // for monitoring:
                int rv = 0;
                for (InstrumentId id = audioInstrumentBase;
                        id < audioInstrumentBase + audioInstruments; ++id) {
                    int irv = jackProcessRecord(id, nframes, nullptr, nullptr, clocksRunning);
                    if (irv != 0)
                        rv = irv;
                }
                doneRecord = true;

                if (!asyncAudio) {
                    return rv;
                }

            } else {
                return jackProcessEmpty(nframes);
            }
        } else if (state == JackTransportStarting) {
            return jackProcessEmpty(nframes);
        } else if (state != JackTransportRolling) {
            RG_WARNING << "jackProcess(): WARNING: unexpected JACK transport state " << state;
        }
    }

    if (state == JackTransportRolling) { // also covers not-on-transport case
        if (m_waiting) {
            if (ignoreCount > 0) {
#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "jackProcess(): transport rolling, but we're ignoring it (count = " << ignoreCount << ")";
#endif
            } else {
#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "jackProcess(): transport rolling, telling ALSA driver to go!";
#endif

                m_alsaDriver->startClocksApproved();
                m_waiting = false;
            }
        }

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcess(): (rolling or not on JACK transport)";
#endif

        if (!clocksRunning) {
#ifdef DEBUG_JACK_PROCESS
            RG_DEBUG << "jackProcess(): clocks stopped";
#endif

            return jackProcessEmpty(nframes);

        } else if (!playing) {
#ifdef DEBUG_JACK_PROCESS
            RG_DEBUG << "jackProcess(): not playing";
#endif

            if (!asyncAudio) {
#ifdef DEBUG_JACK_PROCESS
                RG_DEBUG << "jackProcess(): no interesting async events";
#endif
                // do this before record monitor, otherwise we lose monitor out
                jackProcessEmpty(nframes);
            }

            // for monitoring:
            int rv = 0;
            for (InstrumentId id = audioInstrumentBase;
                    id < audioInstrumentBase + audioInstruments; ++id) {
                int irv = jackProcessRecord(id, nframes, nullptr, nullptr, clocksRunning);
                if (irv != 0)
                    rv = irv;
            }
            doneRecord = true;

            if (!asyncAudio) {
                return rv;
            }
        }
    }

#ifdef DEBUG_JACK_PROCESS
    Profiler profiler3("jackProcess post transport", true);
#else
  #ifdef DEBUG_JACK_XRUN
    Profiler profiler3("jackProcess post transport", false);
  #endif
#endif

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_alsaDriver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    // We always have the master out

    sample_t *master[2] = {
        static_cast<sample_t *>(jack_port_get_buffer(m_outputMasters[0], nframes)),
        static_cast<sample_t *>(jack_port_get_buffer(m_outputMasters[1], nframes))
    };

    memset(master[0], 0, nframes * sizeof(sample_t));
    memset(master[1], 0, nframes * sizeof(sample_t));

    // Reset monitor outs (if present) here prior to mixing

    if (!m_outputMonitors.empty()) {
        sample_t *buffer =
            static_cast<sample_t *>(jack_port_get_buffer(m_outputMonitors[0], nframes));
        if (buffer)
            memset(buffer, 0, nframes * sizeof(sample_t));
    }

    if (m_outputMonitors.size() > 1) {
        sample_t *buffer =
            static_cast<sample_t *>(jack_port_get_buffer(m_outputMonitors[1], nframes));
        if (buffer)
            memset(buffer, 0, nframes * sizeof(sample_t));
    }

    int bussCount = m_bussMixer->getBussCount();

    // If we have any busses, then we just mix from them (but we still
    // need to keep ourselves up to date by reading and monitoring the
    // instruments).  If we have no busses, mix direct from instruments.

    for (int buss = 0; buss < bussCount; ++buss) {

        sample_t *submaster[2] = { nullptr, nullptr };
        sample_t peak[2] = { 0.0, 0.0 };

        if ((int)m_outputSubmasters.size() > buss * 2 + 1) {
            submaster[0] =
                static_cast<sample_t *>(jack_port_get_buffer(m_outputSubmasters[buss * 2], nframes));
            submaster[1] =
                static_cast<sample_t *>(jack_port_get_buffer(m_outputSubmasters[buss * 2 + 1], nframes));
        }

        if (!submaster[0])
            submaster[0] = m_tempOutBuffer;
        if (!submaster[1])
            submaster[1] = m_tempOutBuffer;

        for (int ch = 0; ch < 2; ++ch) {

            RingBuffer<AudioBussMixer::sample_t> *rb =
                m_bussMixer->getRingBuffer(buss, ch);

            if (!rb || m_bussMixer->isBussDormant(buss)) {
                if (rb)
                    rb->skip(nframes);
                if (submaster[ch])
                    memset(submaster[ch], 0, nframes * sizeof(sample_t));
            } else {
                size_t actual = rb->read(submaster[ch], nframes);
                if (actual < nframes) {
                    reportFailure(MappedEvent::FailureBussMixUnderrun);
                }
                for (size_t i = 0; i < nframes; ++i) {
                    sample_t sample = submaster[ch][i];
                    if (sample > peak[ch])
                        peak[ch] = sample;
                    master[ch][i] += sample;
                }
            }
        }

        LevelInfo info;
        info.level = AudioLevel::multiplier_to_fader(
                peak[0], 127, AudioLevel::LongFader);
        info.levelRight = AudioLevel::multiplier_to_fader(
                peak[1], 127, AudioLevel::LongFader);

        SequencerDataBlock::getInstance()->setSubmasterLevel(buss, info);

        for (InstrumentId id = audioInstrumentBase;
                id < audioInstrumentBase + audioInstruments; ++id) {
            if (buss + 1 == m_recordInputs[id].input) {
                jackProcessRecord(id, nframes, submaster[0], submaster[1], clocksRunning);
            }
        }
    }

#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "jackProcess(): have " << audioInstruments << " audio and " << synthInstruments << " synth instruments and " << bussCount << " busses";
#endif

    bool allInstrumentsDormant = true;
    static RealTime dormantTime = RealTime::zeroTime;

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        if (m_instrumentMixer->isInstrumentEmpty(id))
            continue;

        sample_t *instrument[2] = { nullptr, nullptr };
        sample_t peak[2] = { 0.0, 0.0 };

        if (int(m_outputInstruments.size()) > i * 2 + 1) {
            instrument[0] =
                static_cast<sample_t *>(jack_port_get_buffer(m_outputInstruments[i * 2], nframes));
            instrument[1] =
                static_cast<sample_t *>(jack_port_get_buffer(m_outputInstruments[i * 2 + 1], nframes));
        }

        if (!instrument[0])
            instrument[0] = m_tempOutBuffer;
        if (!instrument[1])
            instrument[1] = m_tempOutBuffer;

        for (int ch = 0; ch < 2; ++ch) {

            // We always need to read from an instrument's ring buffer
            // to keep the instrument moving along, as well as for
            // monitoring.  If the instrument is connected straight to
            // the master, then we also need to mix from it.  (We have
            // that information cached courtesy of updateAudioData.)

            bool directToMaster = false;
            if (i < audioInstruments) {
                directToMaster = (m_directMasterAudioInstruments & (1 << i));
            } else {
                directToMaster = (m_directMasterSynthInstruments &
                                  (1 << (i - audioInstruments)));
            }

#ifdef DEBUG_JACK_PROCESS
            if (id == 1000 || id == 10000) {
                RG_DEBUG << "jackProcess(): instrument id " << id << ", base " << audioInstrumentBase << ", direct masters " << m_directMasterAudioInstruments << ": " << directToMaster;
            }
#endif

            RingBuffer<AudioInstrumentMixer::sample_t, 2> *rb =
                m_instrumentMixer->getRingBuffer(id, ch);

            if (!rb || m_instrumentMixer->isInstrumentDormant(id)) {
#ifdef DEBUG_JACK_PROCESS
                if (id == 1000 || id == 10000) {
                    if (rb) {
                        RG_DEBUG << "jackProcess(): instrument " << id << " dormant";
                    } else {
                        RG_DEBUG << "jackProcess(): instrument " << id << " has no ring buffer for channel " << ch;
                    }
                }
#endif
                if (rb)
                    rb->skip(nframes);
                if (instrument[ch])
                    memset(instrument[ch], 0, nframes * sizeof(sample_t));

            } else {

                allInstrumentsDormant = false;

                size_t actual = rb->read(instrument[ch], nframes);

#ifdef DEBUG_JACK_PROCESS

                if (id == 1000) {
                    RG_DEBUG << "jackProcess(): read " << actual << " of " << nframes << " frames for instrument " << id << " channel " << ch;
                }
#endif

                if (actual < nframes) {
                    RG_WARNING << "jackProcess(): WARNING: read " << actual << " of " << nframes << " frames for " << id << " ch " << ch << " (pl " << playing << ", cl " << clocksRunning << ", aa " << asyncAudio << ")";
                    reportFailure(MappedEvent::FailureMixUnderrun);
                }

                for (size_t f = 0; f < nframes; ++f) {
                    sample_t sample = instrument[ch][f];
                    if (sample > peak[ch])
                        peak[ch] = sample;
                    if (directToMaster)
                        master[ch][f] += sample;
                }
            }

            // If the instrument is connected straight to master we
            // also need to skip() on the buss mixer's reader for it,
            // otherwise it'll block because the buss mixer isn't
            // needing to read it.

            if (rb && directToMaster) {
                rb->skip(nframes, 1); // 1 is the buss mixer's reader (magic)
            }
        }

        LevelInfo info;
        info.level = AudioLevel::multiplier_to_fader(
                peak[0], 127, AudioLevel::LongFader);
        info.levelRight = AudioLevel::multiplier_to_fader(
                peak[1], 127, AudioLevel::LongFader);

        SequencerDataBlock::getInstance()->setInstrumentLevel(id, info);
    }

    if (asyncAudio) {
        if (!allInstrumentsDormant) {
            dormantTime = RealTime::zeroTime;
        } else {
            dormantTime = dormantTime +
                          RealTime::frame2RealTime(m_bufferSize, m_sampleRate);
            if (dormantTime > RealTime(10, 0)) {
                RG_WARNING << "jackProcess(): WARNING: dormantTime = " << dormantTime << ", resetting m_haveAsyncAudioEvent";
                m_haveAsyncAudioEvent = false;
            }
        }
    }

    // Get master fader levels.  There's no pan on the master.
    float gain = AudioLevel::dB_to_multiplier(m_masterLevel);
    float masterPeak[2] = { 0.0, 0.0 };

    for (int ch = 0; ch < 2; ++ch) {
        for (size_t i = 0; i < nframes; ++i) {
            sample_t sample = master[ch][i] * gain;
            if (sample > masterPeak[ch])
                masterPeak[ch] = sample;
            master[ch][i] = sample;
        }
    }

    LevelInfo info;
    info.level = AudioLevel::multiplier_to_fader(
            masterPeak[0], 127, AudioLevel::LongFader);
    info.levelRight = AudioLevel::multiplier_to_fader(
            masterPeak[1], 127, AudioLevel::LongFader);

    SequencerDataBlock::getInstance()->setMasterLevel(info);

    for (InstrumentId id = audioInstrumentBase;
            id < audioInstrumentBase + audioInstruments; ++id) {
        if (m_recordInputs[id].input == 0) {
            jackProcessRecord(id, nframes, master[0], master[1], clocksRunning);
        } else if (m_recordInputs[id].input < 1000) { // buss, already done
            // nothing
        } else if (!doneRecord) {
            jackProcessRecord(id, nframes, nullptr, nullptr, clocksRunning);
        }
    }

    if (playing) {
        if (!lowLatencyMode) {
            if (m_bussMixer->getBussCount() == 0) {
                m_instrumentMixer->signal();
            } else {
                m_bussMixer->signal();
            }
        }
    }

    m_framesProcessed += nframes;

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
    framesThisPlay += nframes; //!!!
#endif

#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "jackProcess(): " << nframes << " frames, " << framesThisPlay << " this play, " << m_framesProcessed << " total";
#endif

    return 0;
}

int
JackDriver::jackProcessEmpty(jack_nframes_t nframes)
{
    sample_t *buffer;

#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "jackProcessEmpty() begin";
#endif

    buffer = static_cast<sample_t *>
             (jack_port_get_buffer(m_outputMasters[0], nframes));
    if (buffer)
        memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
             (jack_port_get_buffer(m_outputMasters[1], nframes));
    if (buffer)
        memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
             (jack_port_get_buffer(m_outputMonitors[0], nframes));
    if (buffer)
        memset(buffer, 0, nframes * sizeof(sample_t));

    buffer = static_cast<sample_t *>
             (jack_port_get_buffer(m_outputMonitors[1], nframes));
    if (buffer)
        memset(buffer, 0, nframes * sizeof(sample_t));

    for (size_t i = 0; i < m_outputSubmasters.size(); ++i) {
        buffer = static_cast<sample_t *>
                 (jack_port_get_buffer(m_outputSubmasters[i], nframes));
        if (buffer)
            memset(buffer, 0, nframes * sizeof(sample_t));
    }

    for (size_t i = 0; i < m_outputInstruments.size(); ++i) {
        buffer = static_cast<sample_t *>
                 (jack_port_get_buffer(m_outputInstruments[i], nframes));
        if (buffer)
            memset(buffer, 0, nframes * sizeof(sample_t));
    }

    m_framesProcessed += nframes;

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))

    framesThisPlay += nframes;
#endif
#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "jackProcess(): " << nframes << " frames, " << framesThisPlay << " this play, " << m_framesProcessed << " total";
#endif

    return 0;
}

int
JackDriver::jackProcessRecord(InstrumentId id,
                              jack_nframes_t nframes,
                              sample_t *sourceBufferLeft,
                              sample_t *sourceBufferRight,
                              bool clocksRunning)
{
#ifdef DEBUG_JACK_PROCESS
    Profiler profiler("jackProcessRecord", true);
#else
#ifdef DEBUG_JACK_XRUN

    Profiler profiler("jackProcessRecord", false);
#endif
#endif

    bool wroteSomething = false;
    sample_t peakLeft = 0.0, peakRight = 0.0;

#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "jackProcessRecord(" << id << "): clocksRunning " << clocksRunning;
#endif

    // Get input buffers
    //
    sample_t *inputBufferLeft = nullptr, *inputBufferRight = nullptr;

    int recInput = m_recordInputs[id].input;

    int channel = m_recordInputs[id].channel;
    int channels = (channel == -1 ? 2 : 1);
    if (channels == 2)
        channel = 0;

    float level = m_recordInputs[id].level;

    if (sourceBufferLeft) {

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcessRecord(" << id << "): buss input provided";
#endif

        inputBufferLeft = sourceBufferLeft;
        if (sourceBufferRight)
            inputBufferRight = sourceBufferRight;

    } else if (recInput < 1000) {

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcessRecord(" << id << "): no known input";
#endif

        return 0;

    } else {

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcessRecord(" << id << "): record input " << recInput;
#endif

        int input = recInput - 1000;

        int port = input * channels + channel;
        int portRight = input * channels + 1;

        if (port < int(m_inputPorts.size())) {
            inputBufferLeft = static_cast<sample_t*>
                              (jack_port_get_buffer(m_inputPorts[port], nframes));
        }

        if (channels == 2 && portRight < int(m_inputPorts.size())) {
            inputBufferRight = static_cast<sample_t*>
                               (jack_port_get_buffer(m_inputPorts[portRight], nframes));
        }
    }

    float gain = AudioLevel::dB_to_multiplier(level);

    if (m_alsaDriver->getRecordStatus() == RECORD_ON &&
        clocksRunning &&
        m_fileWriter->haveRecordFileOpen(id)) {

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcessRecord(" << id << "): recording";
#endif

        memset(m_tempOutBuffer, 0, nframes * sizeof(sample_t));

        if (inputBufferLeft) {
            for (size_t i = 0; i < nframes; ++i) {
                sample_t sample = inputBufferLeft[i] * gain;
                if (sample > peakLeft)
                    peakLeft = sample;
                m_tempOutBuffer[i] = sample;
            }

            if (!m_outputMonitors.empty()) {
                sample_t *buf =
                    static_cast<sample_t *>
                    (jack_port_get_buffer(m_outputMonitors[0], nframes));
                if (buf) {
                    for (size_t i = 0; i < nframes; ++i) {
                        buf[i] += m_tempOutBuffer[i];
                    }
                }
            }

            m_fileWriter->write(id, m_tempOutBuffer, 0, nframes);
        }

        if (channels == 2) {

            if (inputBufferRight) {
                for (size_t i = 0; i < nframes; ++i) {
                    sample_t sample = inputBufferRight[i] * gain;
                    if (sample > peakRight)
                        peakRight = sample;
                    m_tempOutBuffer[i] = sample;
                }
                if (m_outputMonitors.size() > 1) {
                    sample_t *buf =
                        static_cast<sample_t *>
                        (jack_port_get_buffer(m_outputMonitors[1], nframes));
                    if (buf) {
                        for (size_t i = 0; i < nframes; ++i) {
                            buf[i] += m_tempOutBuffer[i];
                        }
                    }
                }
            }

            m_fileWriter->write(id, m_tempOutBuffer, 1, nframes);
        }

        wroteSomething = true;

    } else {

        // want peak levels and monitors anyway, even if not recording

#ifdef DEBUG_JACK_PROCESS
        RG_DEBUG << "jackProcessRecord(" << id << "): monitoring only";
#endif

        if (inputBufferLeft) {

            sample_t *buf = nullptr;
            if (!m_outputMonitors.empty()) {
                buf = static_cast<sample_t *>
                    (jack_port_get_buffer(m_outputMonitors[0], nframes));
            }

            for (size_t i = 0; i < nframes; ++i) {
                sample_t sample = inputBufferLeft[i] * gain;
                if (sample > peakLeft)
                    peakLeft = sample;
                if (buf)
                    buf[i] = sample;
            }

            if (channels == 2 && inputBufferRight) {

                buf = nullptr;
                if (m_outputMonitors.size() > 1) {
                    buf = static_cast<sample_t *>
                        (jack_port_get_buffer(m_outputMonitors[1], nframes));
                }

                for (size_t i = 0; i < nframes; ++i) {
                    sample_t sample = inputBufferRight[i] * gain;
                    if (sample > peakRight)
                        peakRight = sample;
                    if (buf)
                        buf[i] = sample;
                }
            }
        }
    }

    if (channels < 2)
        peakRight = peakLeft;

    LevelInfo info;
    info.level = AudioLevel::multiplier_to_fader
            (peakLeft, 127, AudioLevel::LongFader);
    info.levelRight = AudioLevel::multiplier_to_fader
            (peakRight, 127, AudioLevel::LongFader);
    SequencerDataBlock::getInstance()->setInstrumentRecordLevel(id, info);

    if (wroteSomething) {
        m_fileWriter->signal();
    }

    return 0;
}


int
JackDriver::jackSyncCallback(jack_transport_state_t state,
                             jack_position_t *position,
                             void *arg)
{
    JackDriver *inst = (JackDriver *)arg;
    if (!inst)
        return true; // or rather, return "huh?"

    inst->m_alsaDriver->checkTimerSync(0); // reset, as not processing

    if (!inst->m_jackTransportEnabled)
        return true; // ignore

    RosegardenSequencer *sequencer =
            RosegardenSequencer::getInstance();
    if (!sequencer)
        return true;

#ifdef DEBUG_JACK_TRANSPORT

    RG_DEBUG << "jackSyncCallback(): state " << state << " [" << (state == 0 ? "stopped" : state == 1 ? "rolling" : state == 2 ? "looping" : state == 3 ? "starting" : "unknown") << "], frame " << position->frame << ", waiting " << inst->m_waiting << ", playing " << inst->m_alsaDriver->isPlaying();
    RG_DEBUG << "jackSyncCallback(): m_waitingState " << inst->m_waitingState << ", unique_1 " << position->unique_1 << ", unique_2 " << position->unique_2;
    RG_DEBUG << "jackSyncCallback(): rate " << position->frame_rate << ", bar " << position->bar << ", beat " << position->beat << ", tick " << position->tick << ", bpm " << position->beats_per_minute;

#endif

    RosegardenSequencer::TransportRequest request =
            RosegardenSequencer::TransportNoChange;

    if (inst->m_alsaDriver->isPlaying()) {

        if (state == JackTransportStarting) {
            request = RosegardenSequencer::TransportJumpToTime;
        } else if (state == JackTransportStopped) {
            request = RosegardenSequencer::TransportStop;
        }

    } else {

        if (state == JackTransportStarting) {
            request = RosegardenSequencer::TransportStartAtTime;
        } else if (state == JackTransportStopped) {
            request = RosegardenSequencer::TransportNoChange;
        }
    }

    if (!inst->m_waiting || inst->m_waitingState != state) {

        if (request == RosegardenSequencer::TransportJumpToTime ||
                request == RosegardenSequencer::TransportStartAtTime) {

            RealTime rt = RealTime::frame2RealTime(position->frame,
                                                   position->frame_rate);

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): Requesting jump to " << rt;
#endif

            inst->m_waitingToken = sequencer->transportJump(request, rt);

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): My token is " << inst->m_waitingToken;
#endif

        } else if (request == RosegardenSequencer::TransportStop) {

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): Requesting state change to " << request;
#endif

            inst->m_waitingToken = sequencer->transportChange(request);

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): My token is " << inst->m_waitingToken;
#endif

        } else if (request == RosegardenSequencer::TransportNoChange) {

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): Requesting no state change!";
#endif

            inst->m_waitingToken = sequencer->transportChange(request);

#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): My token is " << inst->m_waitingToken;
#endif

        }

        inst->m_waiting = true;
        inst->m_waitingState = state;

#ifdef DEBUG_JACK_TRANSPORT
        RG_DEBUG << "jackSyncCallback(): Setting waiting to " << inst->m_waiting << " and waiting state to " << inst->m_waitingState << " (request was " << request << ")";
#endif

        return 0;

    } else {

        if (sequencer->isTransportSyncComplete(inst->m_waitingToken)) {
#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): Sync complete";
#endif

            return 1;
        } else {
#ifdef DEBUG_JACK_TRANSPORT
            RG_DEBUG << "jackSyncCallback(): Sync not complete";
#endif

            return 0;
        }
    }
}

bool
JackDriver::relocateTransportInternal(bool alsoStart)
{
    if (!m_client)
        return true;

#ifdef DEBUG_JACK_TRANSPORT
    const char *fn = (alsoStart ?
                      "JackDriver::startTransport()" :
                      "JackDriver::relocateTransport()");
#endif

#ifdef DEBUG_JACK_TRANSPORT
    RG_DEBUG << "relocateTransportInternal(): called by " << fn;
#else
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "relocateTransportInternal()";
#endif
#endif

    // m_waiting is true if we are waiting for the JACK transport
    // to finish a change of state.

    if (m_jackTransportEnabled) {

        // If on the transport, we never return true here -- instead
        // the JACK process calls startClocksApproved() to signal to
        // the ALSA driver that it's time to go.  But we do use this
        // to manage our JACK transport state requests.

        // Where did this request come from?  Are we just responding
        // to an external sync?

        RosegardenSequencer *sequencer =
                RosegardenSequencer::getInstance();

        if (sequencer) {
            if (sequencer->isTransportSyncComplete(m_waitingToken)) {

                // Nope, this came from Rosegarden

#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "relocateTransportInternal(): called by " << fn << ": asking JACK transport to start, setting wait state";
#endif

                m_waiting = true;
                m_waitingState = JackTransportStarting;

                long frame = RealTime::realTime2Frame
                             (m_alsaDriver->getSequencerTime(), m_sampleRate);

                if (frame < 0) {
                    // JACK Transport doesn't support preroll and
                    // can't set transport position to before zero
                    // (frame count is unsigned), so there's no very
                    // satisfactory fix for what to do for count-in
                    // bars.  Let's just start at zero instead.
                    jack_transport_locate(m_client, 0);
                } else {
                    jack_transport_locate(m_client, frame);
                }

                if (alsoStart) {
                    jack_transport_start(m_client);
                    m_ignoreProcessTransportCount = 1;
                } else {
                    m_ignoreProcessTransportCount = 2;
                }
            } else {
#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "relocateTransportInternal(): called by " << fn << ": waiting already";
#endif

            }
        }
        return false;
    }

#if (defined(DEBUG_JACK_DRIVER) || defined(DEBUG_JACK_PROCESS) || defined(DEBUG_JACK_TRANSPORT))
    framesThisPlay = 0; //!!!
    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    startTime = RealTime(tv.tv_sec, tv.tv_usec * 1000); //!!!
#endif
#ifdef DEBUG_JACK_TRANSPORT

    RG_DEBUG << "relocateTransportInternal(): called by " << fn << ": not on JACK transport, accepting right away";
#endif

    return true;
}

bool
JackDriver::startTransport()
{
    return relocateTransportInternal(true);
}

bool
JackDriver::relocateTransport()
{

    return relocateTransportInternal(false);
}

void
JackDriver::stopTransport()
{
    if (!m_client)
        return ;

    RG_DEBUG << "stopTransport(): resetting m_haveAsyncAudioEvent";

    m_haveAsyncAudioEvent = false;

#ifdef DEBUG_JACK_TRANSPORT

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime endTime = RealTime(tv.tv_sec, tv.tv_usec * 1000); //!!!
    RG_DEBUG << "stopTransport(): frames this play: " << framesThisPlay << ", elapsed " << (endTime - startTime);
#endif

    if (m_jackTransportEnabled) {

        // Where did this request come from?  Is this a result of our
        // sync to a transport that has in fact already stopped?

        RosegardenSequencer *sequencer =
                RosegardenSequencer::getInstance();

        if (sequencer) {
            if (sequencer->isTransportSyncComplete(m_waitingToken)) {

                // No, we have no outstanding external requests; this
                // must have genuinely been requested from within
                // Rosegarden, so:

#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "stopTransport(): internal request, asking JACK transport to stop";
#endif

                jack_transport_stop(m_client);

            } else {
                // Nothing to do

#ifdef DEBUG_JACK_TRANSPORT
                RG_DEBUG << "stopTransport(): external request, JACK transport is already stopped";
#endif

            }
        }
    }

    if (m_instrumentMixer)
        m_instrumentMixer->resetAllPlugins(true); // discard events too
}


// Pick up any change of buffer size
//
int
JackDriver::jackBufferSize(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "jackBufferSize() - buffer size changed to " << nframes;
#endif

    inst->m_bufferSize = nframes;

    // Recreate our temporary mix buffers to the new size
    //
    //!!! need buffer size change callbacks on plugins (so long as they
    // have internal buffers) and the mix manager, with locks acquired
    // appropriately

    delete [] inst->m_tempOutBuffer;
    inst->m_tempOutBuffer = new sample_t[inst->m_bufferSize];

    return 0;
}

// Sample rate change
//
int
JackDriver::jackSampleRate(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "jackSampleRate() - sample rate changed to " << nframes;
#endif

    inst->m_sampleRate = nframes;

    return 0;
}

void
JackDriver::jackShutdown(void *arg)
{
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "jackShutdown() - callback received - informing GUI";
#endif

#ifdef DEBUG_JACK_XRUN
    RG_DEBUG << "jackShutdown()";
    Profiles::getInstance()->dump();
#endif

    JackDriver *inst = static_cast<JackDriver*>(arg);
    inst->m_ok = false;
    inst->m_kickedOutAt = time(nullptr);
    inst->reportFailure(MappedEvent::FailureJackDied);
}

int
JackDriver::jackXRun(void *arg)
{
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "jackXRun()";
#endif

#ifdef DEBUG_JACK_XRUN
    RG_DEBUG << "jackXRun()";
    Profiles::getInstance()->dump();
#endif

    // Report to GUI
    //
    JackDriver *inst = static_cast<JackDriver*>(arg);
    inst->reportFailure(MappedEvent::FailureXRuns);

    return 0;
}


void

JackDriver::restoreIfRestorable()
{
    if (m_kickedOutAt == 0)
        return ;

    if (m_client) {
        jack_client_close(m_client);
        RG_DEBUG << "restoreIfRestorable(): closed client";
        m_client = nullptr;
    }

    time_t now = time(nullptr);

    if (now < m_kickedOutAt || now >= m_kickedOutAt + 3) {

        if (m_instrumentMixer)
            m_instrumentMixer->resetAllPlugins(true);
        RG_DEBUG << "restoreIfRestorable(): reset plugins";

        initialise(true);

        if (m_ok) {
            reportFailure(MappedEvent::FailureJackRestart);
        } else {
            reportFailure(MappedEvent::FailureJackRestartFailed);
        }

        m_kickedOutAt = 0;
    }
}

void
JackDriver::prepareAudio()
{
    if (!m_instrumentMixer)
        return ;

    // This is used when restarting clocks after repositioning, but
    // when not actually playing (yet).  We need to do things like
    // regenerating the processing buffers here.  prebufferAudio()
    // also does all of this, but rather more besides.

    m_instrumentMixer->allocateBuffers();
    m_instrumentMixer->resetAllPlugins(false);
}

void
JackDriver::prebufferAudio()
{
    if (!m_instrumentMixer)
        return ;

    // We want this to happen when repositioning during playback, and
    // stopTransport no longer happens then, so we call it from here.
    // NB. Don't want to discard events here as this is called after
    // pushing events to the soft synth queues at startup
    m_instrumentMixer->resetAllPlugins(false);

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "prebufferAudio(): sequencer time is " << m_alsaDriver->getSequencerTime();
#endif

    RealTime sliceStart = getNextSliceStart(m_alsaDriver->getSequencerTime());

    m_fileReader->fillBuffers(sliceStart);

    if (m_bussMixer->getBussCount() > 0) {
        m_bussMixer->fillBuffers(sliceStart); // also calls on m_instrumentMixer
    } else {
        m_instrumentMixer->fillBuffers(sliceStart);
    }
}

void
JackDriver::kickAudio()
{
#ifdef DEBUG_JACK_PROCESS
    RG_DEBUG << "kickAudio()";
#endif

    if (m_fileReader)
        m_fileReader->kick();
    if (m_instrumentMixer)
        m_instrumentMixer->kick();
    if (m_bussMixer)
        m_bussMixer->kick();
    if (m_fileWriter)
        m_fileWriter->kick();
}

void
JackDriver::updateAudioData()
{
    if (!m_ok || !m_client)
        return ;

#ifdef DEBUG_JACK_DRIVER 
    //RG_DEBUG << "updateAudioData() begin...";
#endif

    MappedAudioBuss *mbuss =
        m_alsaDriver->getMappedStudio()->getAudioBuss(0);

    if (mbuss) {
        float level = 0.0;
        (void)mbuss->getProperty(MappedAudioBuss::Level, level);
        m_masterLevel = level;
    }

    unsigned long directMasterAudioInstruments = 0L;
    unsigned long directMasterSynthInstruments = 0L;

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_alsaDriver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_alsaDriver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    RealTime jackLatency = getAudioPlayLatency();
    RealTime maxLatency = RealTime::zeroTime;

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

        InstrumentId id;
        if (i < audioInstruments)
            id = audioInstrumentBase + i;
        else
            id = synthInstrumentBase + (i - audioInstruments);

        MappedAudioFader *fader = m_alsaDriver->getMappedStudio()->getAudioFader(id);
        if (!fader)
            continue;

        float f = 2;
        (void)fader->getProperty(MappedAudioFader::Channels, f);
        int channels = (int)f;

        int inputChannel = -1;
        if (channels == 1) {
            float f = 0;
            (void)fader->getProperty(MappedAudioFader::InputChannel, f);
            inputChannel = (int)f;
        }

        float level = 0.0;
        (void)fader->getProperty(MappedAudioFader::FaderRecordLevel, level);

        // Like in base/Instrument.h, we use numbers < 1000 to
        // mean buss numbers and >= 1000 to mean record ins
        // when recording the record input number.

        MappedObjectValueList connections = fader->getConnections
            (MappedConnectableObject::In);
        int input = 1000;

        if (connections.empty()) {

            RG_WARNING << "updateAudioData(): WARNING: No connections in for record instrument " << (id) << " (mapped id " << fader->getId() << ")";

            // oh dear.
            input = 1000;

        } else if (*connections.begin() == mbuss->getId()) {

            input = 0;

        } else {

            MappedObject *obj = m_alsaDriver->getMappedStudio()->
                getObjectById(MappedObjectId(*connections.begin()));

            if (!obj) {

                RG_WARNING << "updateAudioData(): WARNING: No such object as " << *connections.begin();
                input = 1000;
            } else if (obj->getType() == MappedObject::AudioBuss) {
                input = (int)((MappedAudioBuss *)obj)->getBussId();
            } else if (obj->getType() == MappedObject::AudioInput) {
                input = (int)((MappedAudioInput *)obj)->getInputNumber()
                        + 1000;
            } else {
                RG_WARNING << "updateAudioData(): WARNING: Object " << *connections.begin() << " is not buss or input";
                input = 1000;
            }
        }

        if (m_recordInputs[id].input != input) {
            RG_DEBUG << "updateAudioData(): Changing record input for instrument " << id << " to " << input;
        }
        m_recordInputs[id] = RecordInputDesc(input, inputChannel, level);

        size_t pluginLatency = 0;
        bool empty = m_instrumentMixer->isInstrumentEmpty(id);

        if (!empty) {
            pluginLatency = m_instrumentMixer->getPluginLatency(id);
        }

        // If we find the object is connected to no output, or to buss
        // number 0 (the master), then we set the bit appropriately.

        connections = fader->getConnections(MappedConnectableObject::Out);

        if (connections.empty() || (*connections.begin() == mbuss->getId())) {
            if (i < audioInstruments) {
                directMasterAudioInstruments |= (1 << i);
            } else {
                directMasterSynthInstruments |= (1 << (i - audioInstruments));
            }
        } else if (!empty) {
            pluginLatency +=
                m_instrumentMixer->getPluginLatency((unsigned int) * connections.begin() - 1);
        }

        if (empty) {
            m_instrumentLatencies[id] = RealTime::zeroTime;
        } else {
            m_instrumentLatencies[id] = jackLatency +
                                        RealTime::frame2RealTime(pluginLatency, m_sampleRate);
            if (m_instrumentLatencies[id] > maxLatency) {
                maxLatency = m_instrumentLatencies[id];
            }
        }
    }

    m_maxInstrumentLatency = maxLatency;
    m_directMasterAudioInstruments = directMasterAudioInstruments;
    m_directMasterSynthInstruments = directMasterSynthInstruments;
    m_maxInstrumentLatency = maxLatency;

    int inputs = m_alsaDriver->getMappedStudio()->
                 getObjectCount(MappedObject::AudioInput);

    if (m_client) {
        // this will return with no work if the inputs are already correct:
        createRecordInputs(inputs);
    }

    m_bussMixer->updateInstrumentConnections();
    m_instrumentMixer->updateInstrumentMuteStates();

    if (m_bussMixer->getBussCount() == 0 || m_alsaDriver->getLowLatencyMode()) {
        if (m_bussMixer->running()) {
            m_bussMixer->terminate();
        }
    } else {
        if (!m_bussMixer->running()) {
            m_bussMixer->run();
        }
    }

    if (m_alsaDriver->getLowLatencyMode()) {
        if (m_instrumentMixer->running()) {
            m_instrumentMixer->terminate();
        }
    } else {
        if (!m_instrumentMixer->running()) {
            m_instrumentMixer->run();
        }
    }

#ifdef DEBUG_JACK_DRIVER 
    //RG_DEBUG << "updateAudioData() end";
#endif
}

void
JackDriver::setAudioBussLevels(int bussNo, float dB, float pan)
{
    if (m_bussMixer) {
        m_bussMixer->setBussLevels(bussNo, dB, pan);
    }
}

void
JackDriver::setAudioInstrumentLevels(InstrumentId instrument, float dB, float pan)
{
    if (m_instrumentMixer) {
        m_instrumentMixer->setInstrumentLevels(instrument, dB, pan);
    }
}

RealTime
JackDriver::getNextSliceStart(const RealTime &now) const
{
    jack_nframes_t frame;
    bool neg = false;

    if (now < RealTime::zeroTime) {
        neg = true;
        frame = RealTime::realTime2Frame(RealTime::zeroTime - now, m_sampleRate);
    } else {
        frame = RealTime::realTime2Frame(now, m_sampleRate);
    }

    jack_nframes_t rounded = frame;
    rounded /= m_bufferSize;
    rounded *= m_bufferSize;

    RealTime roundrt;

    if (rounded == frame)
        roundrt = RealTime::frame2RealTime(rounded, m_sampleRate);
    else if (neg)
        roundrt = RealTime::frame2RealTime(rounded, m_sampleRate);
    else
        roundrt = RealTime::frame2RealTime(rounded + m_bufferSize, m_sampleRate);

    if (neg)
        roundrt = RealTime::zeroTime - roundrt;

    return roundrt;
}


int
JackDriver::getAudioQueueLocks()
{
    // We have to lock the mixers first, because the mixers can try to
    // lock the disk manager from within a locked section -- so if we
    // locked the disk manager first we would risk deadlock when
    // trying to acquire the instrument mixer lock

    int rv = 0;
    if (m_bussMixer) {
#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "getAudioQueueLocks(): trying to lock buss mixer";
#endif

        rv = m_bussMixer->getLock();
        if (rv)
            return rv;
    }
    if (m_instrumentMixer) {
#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "getAudioQueueLocks(): ok, now trying for instrument mixer";
#endif

        rv = m_instrumentMixer->getLock();
        if (rv)
            return rv;
    }
    if (m_fileReader) {
#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "getAudioQueueLocks(): ok, now trying for disk reader";
#endif

        rv = m_fileReader->getLock();
        if (rv)
            return rv;
    }
    if (m_fileWriter) {
#ifdef DEBUG_JACK_DRIVER
        RG_DEBUG << "getAudioQueueLocks(): ok, now trying for disk writer";
#endif

        rv = m_fileWriter->getLock();
    }
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "getAudioQueueLocks(): ok";
#endif

    return rv;
}

int
JackDriver::tryAudioQueueLocks()
{
    int rv = 0;
    if (m_bussMixer) {
        rv = m_bussMixer->tryLock();
        if (rv)
            return rv;
    }
    if (m_instrumentMixer) {
        rv = m_instrumentMixer->tryLock();
        if (rv) {
            if (m_bussMixer) {
                m_bussMixer->releaseLock();
            }
        }
    }
    if (m_fileReader) {
        rv = m_fileReader->tryLock();
        if (rv) {
            if (m_instrumentMixer) {
                m_instrumentMixer->releaseLock();
            }
            if (m_bussMixer) {
                m_bussMixer->releaseLock();
            }
        }
    }
    if (m_fileWriter) {
        rv = m_fileWriter->tryLock();
        if (rv) {
            if (m_fileReader) {
                m_fileReader->releaseLock();
            }
            if (m_instrumentMixer) {
                m_instrumentMixer->releaseLock();
            }
            if (m_bussMixer) {
                m_bussMixer->releaseLock();
            }
        }
    }
    return rv;
}

int
JackDriver::releaseAudioQueueLocks()
{
    int rv = 0;
#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "releaseAudioQueueLocks()";
#endif

    if (m_fileWriter)
        rv = m_fileWriter->releaseLock();
    if (m_fileReader)
        rv = m_fileReader->releaseLock();
    if (m_instrumentMixer)
        rv = m_instrumentMixer->releaseLock();
    if (m_bussMixer)
        rv = m_bussMixer->releaseLock();
    return rv;
}


void
JackDriver::setPluginInstance(InstrumentId id, QString identifier,
                              int position)
{
    if (m_instrumentMixer) {
        m_instrumentMixer->setPlugin(id, position, identifier);
    }
    if (!m_alsaDriver->isPlaying()) {
        prebufferAudio(); // to ensure the plugin's ringbuffers are generated
    }
}

void
JackDriver::removePluginInstance(InstrumentId id, int position)
{
    if (m_instrumentMixer)
        m_instrumentMixer->removePlugin(id, position);
}

void
JackDriver::setPluginInstancePortValue(InstrumentId id, int position,
                                       unsigned long portNumber,
                                       float value)
{
    if (m_instrumentMixer)
        m_instrumentMixer->setPluginPortValue(id, position, portNumber, value);
}

float
JackDriver::getPluginInstancePortValue(InstrumentId id, int position,
                                       unsigned long portNumber)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getPluginPortValue(id, position, portNumber);
    return 0;
}

void
JackDriver::setPluginInstanceBypass(InstrumentId id, int position, bool value)
{
    if (m_instrumentMixer)
        m_instrumentMixer->setPluginBypass(id, position, value);
}

QStringList
JackDriver::getPluginInstancePrograms(InstrumentId id, int position)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getPluginPrograms(id, position);
    return QStringList();
}

QString
JackDriver::getPluginInstanceProgram(InstrumentId id, int position)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getPluginProgram(id, position);
    return QString();
}

QString
JackDriver::getPluginInstanceProgram(InstrumentId id, int position,
                                     int bank, int program)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getPluginProgram(id, position, bank, program);
    return QString();
}

unsigned long
JackDriver::getPluginInstanceProgram(InstrumentId id, int position, QString name)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getPluginProgram(id, position, name);
    return 0;
}

void
JackDriver::setPluginInstanceProgram(InstrumentId id, int position, QString program)
{
    if (m_instrumentMixer)
        m_instrumentMixer->setPluginProgram(id, position, program);
}

QString
JackDriver::configurePlugin(InstrumentId id, int position, QString key, QString value)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->configurePlugin(id, position, key, value);
    return QString();
}

RunnablePluginInstance *
JackDriver::getSynthPlugin(InstrumentId id)
{
    if (m_instrumentMixer)
        return m_instrumentMixer->getSynthPlugin(id);
    else
        return nullptr;
}

void
JackDriver::clearSynthPluginEvents()
{
    if (!m_instrumentMixer) return;

#ifdef DEBUG_JACK_DRIVER
    RG_DEBUG << "clearSynthPluginEvents()";
#endif

    m_instrumentMixer->discardPluginEvents();
}

bool
JackDriver::openRecordFile(InstrumentId id,
                           const QString &filename)
{
    if (m_fileWriter) {
        if (!m_fileWriter->running()) {
            m_fileWriter->run();
        }
        return m_fileWriter->openRecordFile(id, filename);
    } else {
        RG_WARNING << "openRecordFile(): WARNING: No file writer available!";
        return false;
    }
}

bool
JackDriver::closeRecordFile(InstrumentId id,
                            AudioFileId &returnedId)
{
    if (m_fileWriter) {
        return m_fileWriter->closeRecordFile(id, returnedId);
        if (m_fileWriter->running() && !m_fileWriter->haveRecordFilesOpen()) {
            m_fileWriter->terminate();
        }
    } else
        return false;
}


void
JackDriver::reportFailure(MappedEvent::FailureCode code)
{
    if (m_alsaDriver)
        m_alsaDriver->reportFailure(code);
}


}

#endif // HAVE_LIBJACK
#endif // HAVE_ALSA
