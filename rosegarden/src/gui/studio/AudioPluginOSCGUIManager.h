
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_AUDIOPLUGINOSCGUIMANAGER_H_
#define _RG_AUDIOPLUGINOSCGUIMANAGER_H_

#include "base/MidiProgram.h"
#include <map>
#include "sound/RingBuffer.h"
#include <qstring.h>




namespace Rosegarden
{

class TimerCallbackAssistant;
class Studio;
class RosegardenGUIApp;
class OSCMessage;
class AudioPluginOSCGUI;


class AudioPluginOSCGUIManager
{
public:
    AudioPluginOSCGUIManager(RosegardenGUIApp *app);
    virtual ~AudioPluginOSCGUIManager();

    void setStudio(Studio *studio) { m_studio = studio; }

    bool hasGUI(InstrumentId id, int position);
    void startGUI(InstrumentId id, int position);
    void showGUI(InstrumentId id, int position);
    void stopGUI(InstrumentId id, int position);
    void stopAllGUIs();

    void postMessage(OSCMessage *message); // I take over ownership of message
    void dispatch();

    void updateProgram(InstrumentId id, int position);
    void updatePort(InstrumentId id, int position, int port);
    void updateConfiguration(InstrumentId id, int position,
			     QString key);

    QString getOSCUrl(InstrumentId instrument, int position,
		      QString identifier);
    QString getFriendlyName(InstrumentId instrument, int position,
			    QString identifier);
    bool parseOSCPath(QString path, InstrumentId &instrument, int &position,
		      QString &method);

    static void timerCallback(void *data);
    static void guiExitedCallback(void *data);

protected:
    RosegardenGUIApp *m_app;
    Studio *m_studio;

    bool m_haveOSCThread;
    void checkOSCThread();

    lo_server_thread m_serverThread;
    RingBuffer<OSCMessage *> m_oscBuffer;

    typedef std::map<int, AudioPluginOSCGUI *> IntGUIMap;
    typedef std::map<int, IntGUIMap> TargetGUIMap;
    TargetGUIMap m_guis;

    TimerCallbackAssistant *m_dispatchTimer;
};



}

#endif
