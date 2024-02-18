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
#include "sound/LV2Utils.h"
#include "sound/LV2Worker.h"
#include "misc/Strings.h"

#include <QTimer>


namespace Rosegarden
{


AudioPluginLV2GUIManager::AudioPluginLV2GUIManager(RosegardenMainWindow *mainWindow) :
        m_mainWindow(mainWindow),
        m_studio(nullptr),
        m_instrument(0),
        m_position(0)
{
    m_worker = new LV2Worker;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->registerWorker(m_worker);
}

AudioPluginLV2GUIManager::~AudioPluginLV2GUIManager()
{
    // Delete all the AudioPluginLV2GUI instances.
    stopAllGUIs();
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->unRegisterWorker();
    delete m_worker;
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
    // check that the plugin is still there
    LV2Utils* lv2utils = LV2Utils::getInstance();
    const LV2PluginInstance* inst =
        lv2utils->getPluginInstance(instrument, position);
    if (inst == nullptr) {
        RG_DEBUG << "showGui - no instance";
        return;
    }
    AudioPluginLV2GUI* gui = getInstance(instrument, position);
    if (gui->hasGUI()) {
        gui->showGui();
    }
}

void
AudioPluginLV2GUIManager::stopGUI(InstrumentId instrument, int position)
{
    RG_DEBUG << "stopGUI: " << instrument << "," << position;

    // Let slotStopGUIDelayed() know what we are closing.
    m_instrument = instrument;
    m_position = position;

    QTimer::singleShot(0, this, &AudioPluginLV2GUIManager::slotStopGUIDelayed);
}

bool AudioPluginLV2GUIManager::canEditConnections(InstrumentId instrument,
                                                  int position) const
{
    // we can edit the connections if there are 2 or more
    PluginPort::ConnectionList clist;
    getConnections(instrument, position, clist);
    if (clist.size() > 1) return true;
    return false;
}

void
AudioPluginLV2GUIManager::getConnections
(InstrumentId instrument,
 int position,
 PluginPort::ConnectionList& clist) const
{
    // this routine must work even if we have no gui
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->getConnections(instrument, position, clist);
}

void AudioPluginLV2GUIManager::setConnections
(InstrumentId instrument,
 int position,
 const PluginPort::ConnectionList& clist)
{
    // this routine must work even if we have no gui
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->setConnections(instrument, position, clist);
}

void
AudioPluginLV2GUIManager::slotStopGUIDelayed()
{
    RG_DEBUG << "stopGUIDelayed: " << m_instrument << "," << m_position;
    if (m_guis.find(m_instrument) != m_guis.end() &&
        m_guis[m_instrument].find(m_position) != m_guis[m_instrument].end()) {
        delete m_guis[m_instrument][m_position];
        m_guis[m_instrument].erase(m_position);
        if (m_guis[m_instrument].empty())
            m_guis.erase(m_instrument);
    }
}

void
AudioPluginLV2GUIManager::stopAllGUIs()
{
    RG_DEBUG << "stopallGUIs()";

    for (GUIMap::const_iterator i = m_guis.begin(); i != m_guis.end(); ++i) {
        for (IntGUIMap::const_iterator j = i->second.begin(); j != i->second.end();
             ++j) {
            AudioPluginLV2GUI* gui = j->second;
            delete gui;
        }
    }
    m_guis.clear();
}

void
AudioPluginLV2GUIManager::updatePort(InstrumentId instrument, int position,
                                     int port)
{
    RG_DEBUG << "updatePort(" << instrument << "," << position <<
        "," << port << ")";

    if (m_guis.find(instrument) == m_guis.end() ||
        m_guis[instrument].find(position) == m_guis[instrument].end())
        return ;

    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance)
        return ;

    PluginPortInstance *porti = pluginInstance->getPort(port);
    if (!porti)
        return ;

    RG_DEBUG << "updatePort(" << instrument << "," << position << "," << port << "): value " << porti->value;

    m_guis[instrument][position]->updatePortValue(port, porti->value);
}

AudioPluginLV2GUI*
AudioPluginLV2GUIManager::getInstance(InstrumentId instrument, int position)
{
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return nullptr;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return nullptr;

    QString id = strtoqstr(pluginInstance->getIdentifier());

    RG_DEBUG << "getInstance" << instrument << position << id;

    bool makeInstance = false;

    // See if we can find the instance.

    GUIMap::const_iterator it1 = m_guis.find(instrument);
    if (it1 == m_guis.end()) {
        makeInstance = true;
    } else {
        const IntGUIMap &pmap = (*it1).second;
        IntGUIMap::const_iterator it2 = pmap.find(position);
        if (it2 == pmap.end()) {
            makeInstance = true;
        } else {
            const AudioPluginLV2GUI* instance = (*it2).second;
            QString instanceId = instance->getId();
            if (id != instanceId) {
                RG_DEBUG << "getInstance" << instanceId << "->" << id;
                delete instance;
                makeInstance = true;
            }
        }
    }

    // Not found?  Make it.
    if (makeInstance) {
        AudioPluginLV2GUI* newInstance = new AudioPluginLV2GUI(pluginInstance,
                                                               m_mainWindow,
                                                               instrument,
                                                               position,
                                                               this);
        m_guis[instrument][position] = newInstance;
        RG_DEBUG << "create LV2GUI" << newInstance->getId();
    }

    return m_guis[instrument][position];
}


}
