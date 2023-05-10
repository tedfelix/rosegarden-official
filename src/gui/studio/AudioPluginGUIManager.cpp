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

#define RG_MODULE_STRING "[AudioPluginGUIManager]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginGUIManager.h"

#include "gui/studio/AudioPluginOSCGUIManager.h"
#include "base/Studio.h"
#include "base/AudioPluginInstance.h"
#include "sound/PluginIdentifier.h"

namespace Rosegarden
{

AudioPluginGUIManager::AudioPluginGUIManager(RosegardenMainWindow *mainWindow) :
    m_mainWindow(mainWindow),
    m_oscManager(new AudioPluginOSCGUIManager(mainWindow))
{}

AudioPluginGUIManager::~AudioPluginGUIManager()
{
}

void AudioPluginGUIManager::setStudio(Studio *studio)
{
    m_studio = studio;
    m_oscManager->setStudio(studio);
}

bool AudioPluginGUIManager::hasGUI(InstrumentId instrument, int position)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    RG_DEBUG << "hasGui" << instrument << position << arch;
    switch(arch) {
    case OSC:
        return m_oscManager->hasGUI(instrument, position);
    case LV2:
        //todo
        return false;
    case UNKNOWN:
        return false;
    }
    return false;
}

void AudioPluginGUIManager::showGUI(InstrumentId instrument, int position)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->showGUI(instrument, position);
        break;
    case LV2:
        //todo
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::stopGUI(InstrumentId instrument, int position)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->stopGUI(instrument, position);
        break;
    case LV2:
        //todo
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::stopAllGUIs()
{
    m_oscManager->stopAllGUIs();
    // lv2 todo
}

void AudioPluginGUIManager::updateProgram(InstrumentId instrument, int position)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->updateProgram(instrument, position);
        break;
    case LV2:
        //todo
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::updatePort(InstrumentId instrument, int position,
                                       int port)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->updatePort(instrument, position, port);
        break;
    case LV2:
        //todo
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::updateConfiguration(InstrumentId instrument,
                                                int position, QString key)
{
    PluginArchitecture arch = getArtchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->updateConfiguration(instrument, position, key);
        break;
    case LV2:
        //todo
        break;
    case UNKNOWN:
        break;
    }
}

AudioPluginGUIManager::PluginArchitecture
AudioPluginGUIManager::getArtchitecture(InstrumentId instrument, int position)
{
    if (!m_studio) return UNKNOWN;
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return UNKNOWN;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return UNKNOWN;

    QString id = strtoqstr(pluginInstance->getIdentifier());
    QString iType, iSoName, iLabel;
    PluginIdentifier::parseIdentifier(id, iType, iSoName, iLabel);
    RG_DEBUG << "iType:" << iType;
    if (iType == "ladspa" || iType == "dssi") return OSC;
    if (iType == "lv2") return LV2;
    return UNKNOWN;
}

}
