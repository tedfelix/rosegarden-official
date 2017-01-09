/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPluginOSCGUIManager]"

#include "AudioPluginOSCGUIManager.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "AudioPluginOSCGUI.h"
#include "base/AudioPluginInstance.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Studio.h"
#include "gui/application/RosegardenMainWindow.h"
#include "OSCMessage.h"
#include "sound/MappedEvent.h"
#include "sound/PluginIdentifier.h"
#include "StudioControl.h"
#include "TimerCallbackAssistant.h"

#include <QString>

#include <lo/lo.h>
#include <unistd.h>

namespace Rosegarden
{

static void osc_error(int num, const char *msg, const char *path)
{
    std::cerr << "Rosegarden: ERROR: liblo server error " << num
	      << " in path " << path << ": " << msg << std::endl;
}

static int osc_message_handler(const char *path, const char *types, lo_arg **argv,
			       int argc, lo_message, void *user_data)
{
    AudioPluginOSCGUIManager *manager = (AudioPluginOSCGUIManager *)user_data;

    InstrumentId instrument;
    int position;
    QString method;

    if (!manager->parseOSCPath(path, instrument, position, method)) {
	return 1;
    }

    OSCMessage *message = new OSCMessage();
    message->setTarget(instrument);
    message->setTargetData(position);
    message->setMethod(qstrtostr(method));

    int arg = 0;
    while (types && arg < argc && types[arg]) {
	message->addArg(types[arg], argv[arg]);
	++arg;
    }

    manager->postMessage(message);
    return 0;
}

AudioPluginOSCGUIManager::AudioPluginOSCGUIManager(RosegardenMainWindow *mainWindow) :
        m_mainWindow(mainWindow),
        m_studio(0),
        m_haveOSCThread(false),
        m_oscBuffer(1023),
        m_dispatchTimer(0)
{}

AudioPluginOSCGUIManager::~AudioPluginOSCGUIManager()
{
    delete m_dispatchTimer;

    for (TargetGUIMap::iterator i = m_guis.begin(); i != m_guis.end(); ++i) {
        for (IntGUIMap::iterator j = i->second.begin(); j != i->second.end();
             ++j) {
            delete j->second;
        }
    }
    m_guis.clear();

    if (m_haveOSCThread)
        lo_server_thread_stop(m_serverThread);
}

void
AudioPluginOSCGUIManager::checkOSCThread()
{
    if (m_haveOSCThread)
        return ;

    m_serverThread = lo_server_thread_new(NULL, osc_error);

    lo_server_thread_add_method(m_serverThread, NULL, NULL,
                                osc_message_handler, this);

    lo_server_thread_start(m_serverThread);

    RG_DEBUG << "AudioPluginOSCGUIManager: Base OSC URL is "
    << lo_server_thread_get_url(m_serverThread) << endl;

    m_dispatchTimer = new TimerCallbackAssistant(20, timerCallback, this);

    m_haveOSCThread = true;
}

bool
AudioPluginOSCGUIManager::hasGUI(InstrumentId instrument, int position)
{
    PluginContainer *container = 0;
    container = m_studio->getContainerById(instrument);
    if (!container) return false;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return false;

    try {
        QString filePath = AudioPluginOSCGUI::getGUIFilePath
                           (strtoqstr(pluginInstance->getIdentifier()));
        return ( !filePath.isEmpty() );
    } catch (Exception e) { // that's OK
        return false;
    }
}

void
AudioPluginOSCGUIManager::startGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: " << instrument << "," << position
    << endl;

    checkOSCThread();

    if (m_guis.find(instrument) != m_guis.end() &&
            m_guis[instrument].find(position) != m_guis[instrument].end()) {
        RG_DEBUG << "stopping GUI first";
        stopGUI(instrument, position);
    }

    // check the label
    PluginContainer *container = 0;
    container = m_studio->getContainerById(instrument);
    if (!container) {
        RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: no such instrument or buss as "
                 << instrument << endl;
        return;
    }

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) {
        RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: no plugin at position "
        << position << " for instrument " << instrument << endl;
        return ;
    }

    try {
        AudioPluginOSCGUI *gui =
            new AudioPluginOSCGUI(pluginInstance,
                                  getOSCUrl(instrument,
                                            position,
                                            strtoqstr(pluginInstance->getIdentifier())),
                                  getFriendlyName(instrument,
                                                  position,
                                                  strtoqstr(pluginInstance->getIdentifier())));
        m_guis[instrument][position] = gui;

    } catch (Exception e) {

        RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: failed to start GUI: "
        << e.getMessage() << endl;
    }
}

void
AudioPluginOSCGUIManager::showGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::showGUI: " << instrument << "," << position
    << endl;

    if (m_guis.find(instrument) != m_guis.end() &&
        m_guis[instrument].find(position) != m_guis[instrument].end()) {
        m_guis[instrument][position]->show();
    } else {
        startGUI(instrument, position);
    }
}

void
AudioPluginOSCGUIManager::stopGUI(InstrumentId instrument, int position)
{
    if (m_guis.find(instrument) != m_guis.end() &&
        m_guis[instrument].find(position) != m_guis[instrument].end()) {
        delete m_guis[instrument][position];
        m_guis[instrument].erase(position);
        if (m_guis[instrument].empty())
            m_guis.erase(instrument);
    }
}

void
AudioPluginOSCGUIManager::stopAllGUIs()
{
    while (!m_guis.empty()) {
        while (!m_guis.begin()->second.empty()) {
            delete (m_guis.begin()->second.begin()->second);
            m_guis.begin()->second.erase(m_guis.begin()->second.begin());
        }
        m_guis.erase(m_guis.begin());
    }
}

void
AudioPluginOSCGUIManager::postMessage(OSCMessage *message)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::postMessage";
    m_oscBuffer.write(&message, 1);
}

void
AudioPluginOSCGUIManager::updateProgram(InstrumentId instrument, int position)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::updateProgram(" << instrument << ","
    << position << ")" << endl;

    if (m_guis.find(instrument) == m_guis.end() ||
        m_guis[instrument].find(position) == m_guis[instrument].end())
        return ;

    PluginContainer *container = 0;
    container = m_studio->getContainerById(instrument);
    if (!container) return;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return;

    unsigned long rv = StudioControl::getPluginProgram
                       (pluginInstance->getMappedId(),
                        strtoqstr(pluginInstance->getProgram()));

    int bank = rv >> 16;
    int program = rv - (bank << 16);

    RG_DEBUG << "AudioPluginOSCGUIManager::updateProgram(" << instrument << ","
    << position << "): rv " << rv << ", bank " << bank << ", program " << program << endl;

    m_guis[instrument][position]->sendProgram(bank, program);
}

void
AudioPluginOSCGUIManager::updatePort(InstrumentId instrument, int position,
                                     int port)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::updatePort(" << instrument << ","
    << position << "," << port << ")" << endl;

    if (m_guis.find(instrument) == m_guis.end() ||
        m_guis[instrument].find(position) == m_guis[instrument].end())
        return ;

    PluginContainer *container = 0;
    container = m_studio->getContainerById(instrument);
    if (!container) return;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance)
        return ;

    PluginPortInstance *porti = pluginInstance->getPort(port);
    if (!porti)
        return ;

    RG_DEBUG << "AudioPluginOSCGUIManager::updatePort(" << instrument << ","
    << position << "," << port << "): value " << porti->value << endl;

    m_guis[instrument][position]->sendPortValue(port, porti->value);
}

void
AudioPluginOSCGUIManager::updateConfiguration(InstrumentId instrument, int position,
        QString key)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::updateConfiguration(" << instrument << ","
    << position << "," << key << ")" << endl;

    if (m_guis.find(instrument) == m_guis.end() ||
            m_guis[instrument].find(position) == m_guis[instrument].end())
        return ;

    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return;

    QString value = strtoqstr(pluginInstance->getConfigurationValue(qstrtostr(key)));

    RG_DEBUG << "AudioPluginOSCGUIManager::updatePort(" << instrument << ","
    << position << "," << key << "): value " << value << endl;

    m_guis[instrument][position]->sendConfiguration(key, value);
}

QString
AudioPluginOSCGUIManager::getOSCUrl(InstrumentId instrument, int position,
                                    QString identifier)
{
    // OSC URL will be of the form
    //   osc.udp://localhost:54343/plugin/dssi/<instrument>/<position>/<label>
    // where <position> will be "synth" for synth plugins

    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);

    QString baseUrl = lo_server_thread_get_url(m_serverThread);
    if (!baseUrl.endsWith("/"))
        baseUrl += '/';

    QString url = QString("%1%2/%3/%4/%5/%6")
                  .arg(baseUrl)
                  .arg("plugin")
                  .arg(type)
                  .arg(instrument);

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
        url = url.arg("synth");
    } else {
        url = url.arg(position);
    }

    url = url.arg(label);

    return url;
}

bool
AudioPluginOSCGUIManager::parseOSCPath(QString path, InstrumentId &instrument,
                                       int &position, QString &method)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath(" << path << ")";
    if (!m_studio)
        return false;

    QString pluginStr("/plugin/");

    if (path.startsWith("//")) {
        path = path.right(path.length() - 1);
    }

    if (!path.startsWith(pluginStr)) {
        RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: malformed path "
        << path << endl;
        return false;
    }

    path = path.right(path.length() - pluginStr.length());

    QString type = path.section('/', 0, 0);
    QString instrumentStr = path.section('/', 1, 1);
    QString positionStr = path.section('/', 2, 2);
    QString label = path.section('/', 3, -2);
    method = path.section('/', -1, -1);

    if (instrumentStr.isEmpty() || positionStr.isEmpty() ) {
        RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no instrument or position in " << path;
        return false;
    }

    instrument = instrumentStr.toUInt();

    if (positionStr == "synth") {
        position = Instrument::SYNTH_PLUGIN_POSITION;
    } else {
        position = positionStr.toInt();
    }

    // check the label
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) {
        RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no such instrument or buss as "
        << instrument << " in path " << path << endl;
        return false;
    }

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) {
        RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no plugin at position "
        << position << " for instrument " << instrument << " in path "
        << path << endl;
        return false;
    }

    QString identifier = strtoqstr(pluginInstance->getIdentifier());
    QString iType, iSoName, iLabel;
    PluginIdentifier::parseIdentifier(identifier, iType, iSoName, iLabel);
    if (iLabel != label) {
        RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: wrong label for plugin"
        << " at position " << position << " for instrument " << instrument
        << " in path " << path << " (actual label is " << iLabel
        << ")" << endl;
        return false;
    }

    RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: good path " << path
    << ", got mapped id " << pluginInstance->getMappedId() << endl;

    return true;
}

QString
AudioPluginOSCGUIManager::getFriendlyName(InstrumentId instrument, int position,
        QString)
{
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container)
        return tr("Rosegarden Plugin");
    else {
        if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
            return tr("Rosegarden: %1").arg(strtoqstr(container->getAlias()));
        } else {
            return tr("Rosegarden: %1: %2").arg(strtoqstr(container->getAlias()))
                    .arg(tr("Plugin slot %1").arg(position + 1));
        }
    }
}

void
AudioPluginOSCGUIManager::timerCallback(void *data)
{
    AudioPluginOSCGUIManager *manager = (AudioPluginOSCGUIManager *)data;
    manager->dispatch();
}

void
AudioPluginOSCGUIManager::dispatch()
{
    if (!m_studio)
        return ;

    while (m_oscBuffer.getReadSpace() > 0) {

        OSCMessage *message = 0;
        m_oscBuffer.read(&message, 1);

        int instrument = message->getTarget();
        int position = message->getTargetData();

        PluginContainer *container = m_studio->getContainerById(instrument);
        if (!container) continue;

        AudioPluginInstance *pluginInstance = container->getPlugin(position);
        if (!pluginInstance) continue;

        AudioPluginOSCGUI *gui = 0;

        if (m_guis.find(instrument) == m_guis.end()) {
            RG_DEBUG << "AudioPluginOSCGUIManager: no GUI for instrument "
            << instrument << endl;
        } else if (m_guis[instrument].find(position) == m_guis[instrument].end()) {
            RG_DEBUG << "AudioPluginOSCGUIManager: no GUI for instrument "
            << instrument << ", position " << position << endl;
        } else {
            gui = m_guis[instrument][position];
        }

        std::string method = message->getMethod();

        char type;
        const lo_arg *arg;

        // These generally call back on the RosegardenMainWindow.  We'd
        // like to emit signals, but making AudioPluginOSCGUIManager a
        // QObject is problematic if it's only conditionally compiled.

        if (method == "control") {

            if (message->getArgCount() != 2) {
                RG_DEBUG << "AudioPluginOSCGUIManager: wrong number of args ("
                << message->getArgCount() << ") for control method"
                << endl;
                goto done;
            }
            if (!(arg = message->getArg(0, type)) || type != 'i') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get port number"
                << endl;
                goto done;
            }
            int port = arg->i;
            if (!(arg = message->getArg(1, type)) || type != 'f') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get port value"
                << endl;
                goto done;
            }
            float value = arg->f;

            RG_DEBUG << "AudioPluginOSCGUIManager: setting port " << port
            << " to value " << value << endl;

            m_mainWindow->slotChangePluginPort(instrument, position, port, value);

        } else if (method == "program") {

            if (message->getArgCount() != 2) {
                RG_DEBUG << "AudioPluginOSCGUIManager: wrong number of args ("
                << message->getArgCount() << ") for program method"
                << endl;
                goto done;
            }
            if (!(arg = message->getArg(0, type)) || type != 'i') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get bank number"
                << endl;
                goto done;
            }
            int bank = arg->i;
            if (!(arg = message->getArg(1, type)) || type != 'i') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get program number"
                << endl;
                goto done;
            }
            int program = arg->i;

            QString programName = StudioControl::getPluginProgram
                                  (pluginInstance->getMappedId(), bank, program);

            m_mainWindow->slotChangePluginProgram(instrument, position, programName);

        } else if (method == "update") {

            if (message->getArgCount() != 1) {
                RG_DEBUG << "AudioPluginOSCGUIManager: wrong number of args ("
                << message->getArgCount() << ") for update method"
                << endl;
                goto done;
            }
            if (!(arg = message->getArg(0, type)) || type != 's') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get GUI URL"
                << endl;
                goto done;
            }
            QString url = &arg->s;

            if (!gui) {
                RG_DEBUG << "AudioPluginOSCGUIManager: no GUI for update method"
                << endl;
                goto done;
            }

            gui->setGUIUrl(url);

            for (AudioPluginInstance::ConfigMap::const_iterator i =
                        pluginInstance->getConfiguration().begin();
                    i != pluginInstance->getConfiguration().end(); ++i) {

                QString key = strtoqstr(i->first);
                QString value = strtoqstr(i->second);

#ifdef DSSI_PROJECT_DIRECTORY_KEY

                if (key == PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY) {
                    key = DSSI_PROJECT_DIRECTORY_KEY;
                }
#endif

                RG_DEBUG << "update: configuration: " << key << " -> "
                << value << endl;

                gui->sendConfiguration(key, value);
            }

            unsigned long rv = StudioControl::getPluginProgram
                               (pluginInstance->getMappedId(), strtoqstr(pluginInstance->getProgram()));

            int bank = rv >> 16;
            int program = rv - (bank << 16);
            gui->sendProgram(bank, program);

            int controlCount = 0;
            for (PortInstanceIterator i = pluginInstance->begin();
                    i != pluginInstance->end(); ++i) {
                gui->sendPortValue((*i)->number, (*i)->value);
                /* Avoid overloading the GUI if there are lots and lots of ports */
                if (++controlCount % 50 == 0)
                    usleep(300000);
            }

            gui->show();

        } else if (method == "configure") {

            if (message->getArgCount() != 2) {
                RG_DEBUG << "AudioPluginOSCGUIManager: wrong number of args ("
                << message->getArgCount() << ") for configure method"
                << endl;
                goto done;
            }

            if (!(arg = message->getArg(0, type)) || type != 's') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get configure key"
                << endl;
                goto done;
            }
            QString key = &arg->s;

            if (!(arg = message->getArg(1, type)) || type != 's') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get configure value"
                << endl;
                goto done;
            }
            QString value = &arg->s;

#ifdef DSSI_RESERVED_CONFIGURE_PREFIX

            if (key.startsWith(DSSI_RESERVED_CONFIGURE_PREFIX) ||
                    key == PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY) {
                RG_DEBUG << "AudioPluginOSCGUIManager: illegal reserved configure call from gui: " << key << " -> " << value;
                goto done;
            }
#endif

            RG_DEBUG << "AudioPluginOSCGUIManager: configure(" << key << "," << value
            << ")" << endl;

            m_mainWindow->slotChangePluginConfiguration(instrument, position,
#ifdef DSSI_GLOBAL_CONFIGURE_PREFIX
                                                 key.startsWith(DSSI_GLOBAL_CONFIGURE_PREFIX),
#else
                                                 false,
#endif
                                                 key, value);

        } else if (method == "midi") {

            if (message->getArgCount() != 1) {
                RG_DEBUG << "AudioPluginOSCGUIManager: wrong number of args ("
                << message->getArgCount() << ") for midi method"
                << endl;
                goto done;
            }
            if (!(arg = message->getArg(0, type)) || type != 'm') {
                RG_DEBUG << "AudioPluginOSCGUIManager: failed to get MIDI event"
                << endl;
                goto done;
            }

            RG_DEBUG << "AudioPluginOSCGUIManager: handling MIDI message";

            // let's only handle note on and note off

            int eventCode = arg->m[1];
            int eventType = eventCode & MIDI_MESSAGE_TYPE_MASK;
            if (eventType == MIDI_NOTE_ON ||
                    eventType == MIDI_NOTE_OFF) {
                MappedEvent ev(instrument,
                               MappedEvent::MidiNote,
                               MidiByte(arg->m[2]),
                               MidiByte(arg->m[3]),
                               RealTime::zeroTime,
                               RealTime::zeroTime,
                               RealTime::zeroTime);
                if (eventType == MIDI_NOTE_OFF)
                    ev.setVelocity(0);
                StudioControl::sendMappedEvent(ev);
            }

        } else if (method == "exiting") {

            RG_DEBUG << "AudioPluginOSCGUIManager: GUI exiting";
            stopGUI(instrument, position);
            m_mainWindow->slotPluginGUIExited(instrument, position);

        } else {

            RG_DEBUG << "AudioPluginOSCGUIManager: unknown method " << method;
        }

done:
        delete message;
    }
}

}

