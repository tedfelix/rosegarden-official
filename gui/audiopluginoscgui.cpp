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

#include "rosestrings.h"
#include "rosedebug.h"

#include <kprocess.h>
#include <qdir.h>
#include <qfileinfo.h>


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


#define PLUGIN_PREFIX "/plugin/"

static void osc_error(int num, const char *msg, const char *path)
{
    std::cerr << "Rosegarden: ERROR: liblo server error " << num
	      << " in path " << path << ": " << msg << std::endl;
}

static int osc_message_handler(const char *path, const char *types, lo_arg **argv,
			       int argc, lo_message, void *user_data)
{
    AudioPluginOSCGUIManager *manager = (AudioPluginOSCGUIManager *)user_data;

    if (strncmp(path, PLUGIN_PREFIX, strlen(PLUGIN_PREFIX))) {
	std::cerr << "Rosegarden: ERROR: unexpected OSC path " << path << std::endl;
	return 1;
    }
	
    char *scooter = (char *)(path + strlen(PLUGIN_PREFIX));
    
    int id = (int)strtol(scooter, &scooter, 0);
    if (id == 0) {
	std::cerr << "Rosegarden: ERROR: malformed plugin id in " << path << std::endl;
	return 1;
    }

    if (!scooter || (*scooter != '/') || !*++scooter) {
	std::cerr << "Rosegarden: ERROR: malformed method in " << path << std::endl;
	return 1;
    }

    std::string method = scooter;

    OSCMessage *message = new OSCMessage();
    message->setTarget(id);
    message->setMethod(method);
    message->clearArgs();

    int arg = 0;
    while (types && arg < argc && types[arg]) {
	message->addArg(types[arg], argv[arg]);
    }

    manager->postMessage(message);
    return 0;
}
    
AudioPluginOSCGUIManager::AudioPluginOSCGUIManager() :
    m_oscBuffer(1023)
{
    m_serverThread = lo_server_thread_new(NULL, osc_error);

    // OSC URL will be of the form localhost:54343/plugin/<mapped id>/method

    lo_server_thread_add_method(m_serverThread, NULL, NULL,
				osc_message_handler, this);

    lo_server_thread_start(m_serverThread);

    m_dispatchTimer = new TimerCallbackAssistant(20, timerCallback, this);
}

AudioPluginOSCGUIManager::~AudioPluginOSCGUIManager()
{
    delete m_dispatchTimer;

    //!!! kill all plugin guis?
    //!!! arse -- there is no lo_server_thread_terminate
}

void
AudioPluginOSCGUIManager::postMessage(OSCMessage *message)
{
    RG_DEBUG << "AudioPluginOSCGUIManager::postMessage" << endl;
    m_oscBuffer.write(&message, 1);
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
    while (m_oscBuffer.getReadSpace() > 0) {

	OSCMessage *message;
	m_oscBuffer.read(&message, 1);

	int target = message->getTarget();

	if (m_guis.find(target) == m_guis.end()) {

	    std::cerr << "Rosegarden: WARNING: AudioPluginOSCGUIManager::slotDispatch: unknown target " << target << std::endl;
	    delete message;

	} else {
	    m_guis[target]->acceptFromGUI(message);
	}
    }
}

AudioPluginOSCGUI::AudioPluginOSCGUI(Rosegarden::AudioPluginInstance *instance,
				     QString oscUrl) :
    m_instance(instance),
    m_gui(0),
    m_oscUrl(oscUrl)
{
    QString identifier = strtoqstr(instance->getIdentifier());

    QString type, soName, label;
    Rosegarden::PluginIdentifier::parseIdentifier(identifier, type, soName, label);

    QFileInfo soInfo(soName);
    if (soInfo.isRelative()) {
	//!!!
	RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Unable to deal with relative .so path " << soName << " yet" << endl;
	return;
    }

    QDir dir(soInfo.dir());
    if (!dir.cd(soInfo.baseName(TRUE))) {
	RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: No GUI subdir for plugin .so " << soName << endl;
	return;
    }

    const QFileInfoList *list = dir.entryInfoList();

    for (QFileInfoList::Iterator i = list->begin(); i != list->end(); ++i) {

	if ((*i)->isFile() && (*i)->isExecutable() &&
	    (*i)->fileName().left(label.length()) == label) { // that'll do
	    
	    // arguments: osc url, dll name, label, instance tag

	    m_gui = new KProcess();

	    *m_gui << (*i)->filePath()
		   << m_oscUrl
		   << soInfo.fileName()
		   << label
		   << "blah"; //!!!

	    if (!m_gui->start(KProcess::NotifyOnExit, KProcess::NoCommunication)) {
		RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Couldn't start process " << (*i)->filePath() << endl;
		delete m_gui;
		m_gui = 0;
	    } else {
		return;
	    }
	}
    }
}

AudioPluginOSCGUI::~AudioPluginOSCGUI()
{
}

void
AudioPluginOSCGUI::acceptFromGUI(OSCMessage *message)
{
    RG_DEBUG << "AudioPluginOSCGUI::acceptFromGUI" << endl;


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


