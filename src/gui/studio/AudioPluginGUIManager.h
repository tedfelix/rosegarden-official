/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGINGUIMANAGER_H
#define RG_AUDIOPLUGINGUIMANAGER_H

#include "gui/application/RosegardenMainWindow.h"

namespace Rosegarden
{

class AudioPluginOSCGUIManager;
class AudioPluginLV2GUIManager;
class Studio;

class AudioPluginGUIManager
{
 public:

    explicit AudioPluginGUIManager(RosegardenMainWindow *mainWindow);
    virtual ~AudioPluginGUIManager();

    void setStudio(Studio *studio);
    bool hasGUI(InstrumentId instrument, int position);
    void showGUI(InstrumentId instrument, int position);
    void stopGUI(InstrumentId instrument, int position);
    void stopAllGUIs();
    void updateProgram(InstrumentId instrument, int position);
    void updatePort(InstrumentId instrument, int position, int port);
    void updateConfiguration(InstrumentId instrument, int position,
                             QString key);

 private:
    enum PluginGUIArchitecture {UNKNOWN, OSC, LV2};
    PluginGUIArchitecture getArchitecture(InstrumentId instrument, int position);

    RosegardenMainWindow *m_mainWindow;
    Studio *m_studio;
    AudioPluginOSCGUIManager *m_oscManager;
#ifdef HAVE_LILV
    AudioPluginLV2GUIManager *m_lv2Manager;
#endif

};

}

#endif
