// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "audiopluginoscgui.h"
#include <qtimer.h>

#ifdef HAVE_LIBLO

#include <lo/lo.h>
#include <iostream>

#include "PluginIdentifier.h"
#include "AudioPluginInstance.h"
#include "MappedCommon.h"
#include "Exception.h"

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardengui.h"

#include <kprocess.h>
#include <klocale.h>
#include <qdir.h>
#include <qfileinfo.h>

using Rosegarden::Instrument;
using Rosegarden::InstrumentId;
using Rosegarden::AudioPluginInstance;


OSCMessage::~OSCMessage()
{
    clearArgs();
}

void
OSCMessage::clearArgs()
{
    while (!m_args.empty()) {
	free(m_args[0].second);
	m_args.erase(m_args.begin());
    }
}

void
OSCMessage::addArg(char type, lo_arg *arg)
{
    lo_arg *newarg = 0;

    if (type == 's') {

	size_t sz = strlen((char *)arg) + 1;
	if (sz < sizeof(lo_arg)) sz = sizeof(lo_arg);
	newarg = (lo_arg *)malloc(sz);
	strcpy((char *)newarg, (char *)arg);

    } else {

	newarg = (lo_arg *)malloc(sizeof(lo_arg));
	memcpy((char *)newarg, (char *)arg, sizeof(lo_arg));
    }

    m_args.push_back(OSCArg(type, newarg));
}

size_t
OSCMessage::getArgCount() const
{
    return m_args.size();
}

const lo_arg *
OSCMessage::getArg(size_t i, char &type) const
{
    type = m_args[i].first;
    return m_args[i].second;
}


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
    message->clearArgs();

    int arg = 0;
    while (types && arg < argc && types[arg]) {
	message->addArg(types[arg], argv[arg]);
	++arg;
    }

    manager->postMessage(message);
    return 0;
}
    
AudioPluginOSCGUIManager::AudioPluginOSCGUIManager(RosegardenGUIApp *app) :
    m_app(app),
    m_studio(0),
    m_oscBuffer(1023)
{
    m_serverThread = lo_server_thread_new(NULL, osc_error);

    lo_server_thread_add_method(m_serverThread, NULL, NULL,
				osc_message_handler, this);

    lo_server_thread_start(m_serverThread);

    RG_DEBUG << "AudioPluginOSCGUIManager: Base OSC URL is "
	     << lo_server_thread_get_url(m_serverThread) << endl;

    m_dispatchTimer = new TimerCallbackAssistant(20, timerCallback, this);
}

AudioPluginOSCGUIManager::~AudioPluginOSCGUIManager()
{
    delete m_dispatchTimer;

    //!!! kill all plugin guis?
    //!!! arse -- there is no lo_server_thread_terminate
}

void
AudioPluginOSCGUIManager::startGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: " << instrument << "," << position
	     << endl;

    if (m_guis.find(instrument) != m_guis.end() &&
	m_guis[instrument].find(position) != m_guis[instrument].end()) {
	RG_DEBUG << "stopping GUI first" << endl;
	stopGUI(instrument, position);
    }

    // check the label
    Instrument *i = m_studio->getInstrumentById(instrument);
    if (!i) {
	RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: no such instrument as "
		 << instrument << endl;
	return;
    }

    AudioPluginInstance *pluginInstance = i->getPlugin(position);
    if (!pluginInstance) {
	RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: no plugin at position "
		 << position << " for instrument " << instrument << endl;
	return;
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

    } catch (Rosegarden::Exception e) {

	RG_DEBUG << "AudioPluginOSCGUIManager::startGUI: failed to start GUI: "
		 << e.getMessage() << endl;
    }
}

void
AudioPluginOSCGUIManager::stopGUI(InstrumentId instrument, int position)
{
    if (m_guis.find(instrument) != m_guis.end() &&
	m_guis[instrument].find(position) != m_guis[instrument].end()) {
	delete m_guis[instrument][position];
	m_guis[instrument].erase(position);
	if (m_guis[instrument].empty()) m_guis.erase(instrument);
    }
}

void
AudioPluginOSCGUIManager::postMessage(OSCMessage *message)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::postMessage" << endl;
    m_oscBuffer.write(&message, 1);
}

QString
AudioPluginOSCGUIManager::getOSCUrl(InstrumentId instrument, int position,
				    QString identifier)
{
    // OSC URL will be of the form
    //   osc.udp://localhost:54343/plugin/dssi/<instrument>/<position>/<label>
    // where <position> will be "synth" for synth plugins

    QString type, soName, label;
    Rosegarden::PluginIdentifier::parseIdentifier(identifier, type, soName, label);

    QString baseUrl = lo_server_thread_get_url(m_serverThread);
    if (!baseUrl.endsWith("/")) baseUrl += '/';

    QString url = QString("%1%2/%3/%4/%5/%6")
	.arg(baseUrl)
	.arg("plugin")
	.arg(type)
	.arg(instrument);

    if (position == Instrument::SYNTH_PLUGIN_POSITION) {
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
    RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath(" << path << ")" << endl;
    if (!m_studio) return false;

    QString pluginStr("/plugin/");

    if (path.startsWith("//")) {
	path = path.right(path.length()-1);
    }

    if (!path.startsWith(pluginStr)) {
	RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: malformed path "
		 << path << endl;
	return;
    }

    path = path.right(path.length() - pluginStr.length());

    QString type = path.section('/', 0, 0);
    QString instrumentStr = path.section('/', 1, 1);
    QString positionStr = path.section('/', 2, 2);
    QString label = path.section('/', 3, -2);
    method = path.section('/', -1, -1);

    if (!instrumentStr || !positionStr) {
	RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no instrument or position in " << path << endl;
	return false;
    }

    instrument = instrumentStr.toUInt();

    if (positionStr == "synth") {
	position = Instrument::SYNTH_PLUGIN_POSITION;
    } else {
	position = positionStr.toInt();
    }

    // check the label
    Instrument *i = m_studio->getInstrumentById(instrument);
    if (!i) {
	RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no such instrument as "
		 << instrument << " in path " << path << endl;
	return false;
    }

    AudioPluginInstance *pluginInstance = i->getPlugin(position);
    if (!pluginInstance) {
	RG_DEBUG << "AudioPluginOSCGUIManager::parseOSCPath: no plugin at position "
		 << position << " for instrument " << instrument << " in path "
		 << path << endl;
	return false;
    }

    QString identifier = strtoqstr(pluginInstance->getIdentifier());
    QString iType, iSoName, iLabel;
    Rosegarden::PluginIdentifier::parseIdentifier(identifier, iType, iSoName, iLabel);
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
    Instrument *i = m_studio->getInstrumentById(instrument);
    if (!i) return i18n("Rosegarden Plugin");
    else {
	if (position == Instrument::SYNTH_PLUGIN_POSITION) {
	    return i18n("Rosegarden: %1").arg(strtoqstr(i->getPresentationName()));
	} else {
	    return i18n("Rosegarden: %1: %2").arg(strtoqstr(i->getPresentationName()))
		.arg(i18n("Plugin slot %1").arg(position));
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
    if (!m_studio) return;

    while (m_oscBuffer.getReadSpace() > 0) {

	OSCMessage *message = 0;
	m_oscBuffer.read(&message, 1);

	int instrument = message->getTarget();
	int position = message->getTargetData();

	Instrument *i = m_studio->getInstrumentById(instrument);
	if (!i) continue;

	AudioPluginInstance *pluginInstance = i->getPlugin(position);
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

	// These generally call back on the RosegardenGUIApp.  We'd
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

	    m_app->slotPluginPortChanged(instrument, position, port, value);

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

	    //!!! locate correct program

//	    emit pluginPortChanged(instrument, position, port, value);

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
	    //!!! and update
	    
	    OSCMessage *showMessage = new OSCMessage;
	    showMessage->setMethod("show");
	    showMessage->clearArgs();
	    gui->sendToGUI(showMessage);

	} else if (method == "configure") {

	} else if (method == "midi") {

	} else if (method == "exiting") {

	} else {

	    RG_DEBUG << "AudioPluginOSCGUIManager: unknown method " << method << endl;
	}


/*
	if (m_guis.find(target) == m_guis.end()) {

	    std::cerr << "Rosegarden: WARNING: AudioPluginOSCGUIManager::slotDispatch: unknown target " << target << std::endl;
	    delete message;

	} else {
	    m_guis[target]->acceptFromGUI(message);
	}
*/

    done:
	delete message;
    }
}

AudioPluginOSCGUI::AudioPluginOSCGUI(Rosegarden::AudioPluginInstance *instance,
				     QString serverURL, QString friendlyName) :
    m_gui(0),
    m_address(0),
    m_basePath(""),
    m_serverUrl(serverURL)
{
    QString identifier = strtoqstr(instance->getIdentifier());

    QString type, soName, label;
    Rosegarden::PluginIdentifier::parseIdentifier(identifier, type, soName, label);

    QFileInfo soInfo(soName);
    if (soInfo.isRelative()) {
	//!!!
	RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Unable to deal with relative .so path " << soName << " yet" << endl;
	throw Rosegarden::Exception("Can't deal with relative .soname");
    }

    QDir dir(soInfo.dir());
    if (!dir.cd(soInfo.baseName(TRUE))) {
	RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: No GUI subdir for plugin .so " << soName << endl;
	throw Rosegarden::Exception("No GUI subdir available");
    }

    const QFileInfoList *list = dir.entryInfoList();

    for (QFileInfoList::Iterator i = list->begin(); i != list->end(); ++i) {

	if ((*i)->isFile() && (*i)->isExecutable() &&
	    (*i)->fileName().left(label.length()) == label) { // that'll do
	    
	    // arguments: osc url, dll name, label, instance tag

	    m_gui = new KProcess();

	    *m_gui << (*i)->filePath()
		   << m_serverUrl
		   << soInfo.fileName()
		   << label
		   << friendlyName;

	    RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Starting process "
		     << (*i)->filePath() << " " << m_serverUrl << " "
		     << soInfo.fileName() << " " << label << " " << friendlyName << endl;

	    if (!m_gui->start(KProcess::NotifyOnExit, KProcess::NoCommunication)) {
		RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Couldn't start process " << (*i)->filePath() << endl;
		delete m_gui;
		m_gui = 0;
	    } else {
		return;
	    }
	}
    }

    throw Rosegarden::Exception("Failed to start any GUI");
}

AudioPluginOSCGUI::~AudioPluginOSCGUI()
{
    //!!! tell GUI to exit
}

void
AudioPluginOSCGUI::setGUIUrl(QString url)
{
    if (m_address) lo_address_free(m_address);

    char *host = lo_url_get_hostname(url);
    char *port = lo_url_get_port(url);
    m_address = lo_address_new(host, port);
    free(host);
    free(port);

    m_basePath = lo_url_get_path(url);
}

void
AudioPluginOSCGUI::acceptFromGUI(OSCMessage *message)
{
    RG_DEBUG << "AudioPluginOSCGUI::acceptFromGUI" << endl;


    delete message;
}

void
AudioPluginOSCGUI::sendToGUI(OSCMessage *message)
{
    if (!m_address) {
	RG_DEBUG << "AudioPluginOSCGUI::sendToGUI: no address available for GUI"
		 << endl;
	delete message;
	return;
    }

    QString argSpec = "";
    const lo_arg *arg[5];

    size_t count = message->getArgCount();

    if (count > 5) {
	RG_DEBUG << "AudioPluginOSCGUI::sendToGUI: message has too many args ("
		 << count << ", max is 5)" << endl;
	delete message;
	return;
    }

    for (size_t i = 0; i < count; ++i) {
	char type;
	arg[i] = message->getArg(i, type);
	argSpec += type;
    }

    QString path = m_basePath + "/" + strtoqstr(message->getMethod());

    switch (count) {

    case 0:
	lo_send(m_address, path, argSpec);
	break;

    case 1:
	lo_send(m_address, path, argSpec, arg[0]);
	break;

    case 2:
	lo_send(m_address, path, argSpec, arg[0], arg[1]);
	break;

    case 3:
	lo_send(m_address, path, argSpec, arg[0], arg[1], arg[2]);
	break;

    case 4:
	lo_send(m_address, path, argSpec, arg[0], arg[1], arg[2], arg[3]);
	break;

    case 5:
	lo_send(m_address, path, argSpec, arg[0], arg[1], arg[2], arg[3], arg[4]);
	break;
    }

    delete message;
}


#endif

TimerCallbackAssistant::TimerCallbackAssistant(int ms, void (*callback)(void *data),
					       void *data) :
    m_callback(callback),
    m_data(data)
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotCallback()));
    timer->start(ms, FALSE);
}

TimerCallbackAssistant::~TimerCallbackAssistant()
{
    // nothing -- the QTimer is deleted automatically by its parent QObject (me)
}

void
TimerCallbackAssistant::slotCallback()
{
    m_callback(m_data);
}


