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

#define RG_MODULE_STRING "[AudioPluginLV2GUIManager]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginLV2GUIManager.h"

#include "base/Studio.h"
#include "base/AudioPluginInstance.h"
#include "gui/studio/AudioPluginLV2GUI.h"

namespace Rosegarden
{

AudioPluginLV2GUIManager::AudioPluginLV2GUIManager(RosegardenMainWindow *mainWindow) :
        m_mainWindow(mainWindow),
        m_studio(nullptr)
{
    m_world = lilv_world_new();
    lilv_world_load_all(m_world);
}

AudioPluginLV2GUIManager::~AudioPluginLV2GUIManager()
{
    stopAllGUIs();
    for (auto i = m_guis.begin(); i != m_guis.end(); ++i) {
        for (auto j = i->second.begin(); j != i->second.end();
             ++j) {
            delete j->second;
        }
    }
    m_guis.clear();
    lilv_world_free(m_world);
}

void
AudioPluginLV2GUIManager::setStudio(Studio *studio)
{
    m_studio = studio;
}

bool
AudioPluginLV2GUIManager::hasGUI(InstrumentId instrument, int position)
{
    AudioPluginLV2GUI* gui = getInstance(instrument, position);

    return gui->hasGUI();
}

void
AudioPluginLV2GUIManager::showGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "showGUI(): " << instrument << "," << position;
    AudioPluginLV2GUI* gui = getInstance(instrument, position);
    if (gui->hasGUI()) {
        gui->show();
    }
}

void
AudioPluginLV2GUIManager::stopGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "stopGUI(): " << instrument << "," << position;
    AudioPluginLV2GUI* gui = getInstance(instrument, position);
    if (gui->hasGUI()) {
        gui->hide();
    }
}

void
AudioPluginLV2GUIManager::stopAllGUIs()
{
    for (auto i = m_guis.begin(); i != m_guis.end(); ++i) {
        for (auto j = i->second.begin(); j != i->second.end();
             ++j) {
            AudioPluginLV2GUI* gui = j->second;
            gui->hide();
        }
    }
}

void
AudioPluginLV2GUIManager::updateProgram(InstrumentId instrument, int position)
{
    RG_DEBUG << "updateProgram(" << instrument << "," << position << ")";
}

void
AudioPluginLV2GUIManager::updatePort(InstrumentId instrument, int position,
                                     int port)
{
    RG_DEBUG << "updatePort(" << instrument << "," << position << "," << port << ")";
}

void
AudioPluginLV2GUIManager::updateConfiguration(InstrumentId instrument, int position,
        QString key)
{
    RG_DEBUG << "updateConfiguration(" << instrument << "," << position << "," << key << ")";
}

AudioPluginLV2GUI*
AudioPluginLV2GUIManager::getInstance(InstrumentId instrument, int position)
{
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return nullptr;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return nullptr;

    QString Id = strtoqstr(pluginInstance->getIdentifier());

    RG_DEBUG << "getInstance" << instrument << position << Id;

    bool makeInstance = false;

    auto it1 = m_guis.find(instrument);
    if (it1 == m_guis.end()) {
        makeInstance = true;
    } else {
        IntGUIMap& pmap = (*it1).second;
        auto it2 = pmap.find(position);
        if (it2 == pmap.end()) {
            makeInstance = true;
        } else {
            AudioPluginLV2GUI* instance = (*it2).second;
            QString instanceId = instance->getId();
            if (Id != instanceId) {
                RG_DEBUG << "getInstance" << instanceId << "->" << Id;
                delete(instance);
                makeInstance = true;
            }
        }
    }
    if (makeInstance) {
        AudioPluginLV2GUI* newInstance = new AudioPluginLV2GUI(pluginInstance,
                                                               m_world,
                                                               m_mainWindow,
                                                               instrument,
                                                               position);
        m_guis[instrument][position] = newInstance;
        RG_DEBUG << "create LV2GUI" << newInstance->getId();
    }

    return m_guis[instrument][position];
}

}
