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

#ifndef _AUDIO_PLUGIN_OSC_GUI_H_
#define _AUDIO_PLUGIN_OSC_GUI_H_

#include "config.h"

// The configure check for DSSI also tests for liblo.
#ifdef HAVE_DSSI

#include <lo/lo.h>
#include <string>
#include <vector>
#include <map>
#include <qobject.h>
#include "RingBuffer.h"

namespace Rosegarden {
    class AudioPluginInstance;
}
class AudioPluginOSCGUI;
class KProcess;


class OSCMessage
{
public:
    OSCMessage() { }
    ~OSCMessage();

    void setTarget(const int &target) { m_target = target; }
    int getTarget() const { return m_target; }

    void setMethod(const std::string &method) { m_method = method; }
    std::string getMethod() const { return m_method; }

    void clearArgs();
    void addArg(char type, lo_arg *arg);

    size_t getArgCount() const;
    const lo_arg *getArg(size_t i, char &type) const;

private:
    int m_target;
    std::string m_method;
    typedef std::pair<char, lo_arg *> OSCArg;
    std::vector<OSCArg> m_args;
};


class AudioPluginOSCGUIManager : public QObject
{
    Q_OBJECT

public:
    AudioPluginOSCGUIManager();
    virtual ~AudioPluginOSCGUIManager();

    void postMessage(OSCMessage *message); // I take over ownership of message

protected slots:
    void slotDispatch();

protected:
    lo_server_thread m_serverThread;
    Rosegarden::RingBuffer<OSCMessage *> m_oscBuffer;
    typedef std::map<int, AudioPluginOSCGUI *> TargetGUIMap;
    TargetGUIMap m_guis;
    QTimer *m_dispatchTimer;
};


class AudioPluginOSCGUI
{
public:
    AudioPluginOSCGUI(Rosegarden::AudioPluginInstance *instance,
		      QString oscUrl);
    virtual ~AudioPluginOSCGUI();

    void acceptFromGUI(OSCMessage *message); // I take over ownership of message

protected:
    Rosegarden::AudioPluginInstance *m_instance;
    KProcess *m_gui;
    QString m_oscUrl;
};
    

#endif

#endif
