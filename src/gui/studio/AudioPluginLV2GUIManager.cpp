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
#include "gui/application/RosegardenMainWindow.h"

#include <QTimer>


namespace Rosegarden
{


AudioPluginLV2GUIManager::AudioPluginLV2GUIManager(RosegardenMainWindow *mainWindow) :
        m_mainWindow(mainWindow),
        m_studio(nullptr),
        m_instrument(0),
        m_position(0)
{
    // Make sure these are created by now.
    // Not sure we need them at this point, but we used to do the
    // equivalent here.
    LV2Utils::getInstance();
    LV2Worker::getInstance();
}

AudioPluginLV2GUIManager::~AudioPluginLV2GUIManager()
{
    // Delete all the AudioPluginLV2GUI instances.
    stopAllGUIs();

    LV2Worker::getInstance()->stop();
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

bool AudioPluginLV2GUIManager::hasParameters(InstrumentId instrument,
                                             int position) const
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    return lv2utils->hasParameters(instrument, position);
}

void AudioPluginLV2GUIManager::getParameters
(InstrumentId instrument,
 int position,
 AudioPluginInstance::PluginParameters& params)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->getParameters(instrument, position, params);
}

void AudioPluginLV2GUIManager::updatePluginParameter
(InstrumentId instrument,
 int position,
 const QString& paramId,
 const AudioPluginInstance::PluginParameter& param)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->updatePluginParameter(instrument, position, paramId, param);
}

void AudioPluginLV2GUIManager::getPresets
(InstrumentId instrument,
 int position,
 AudioPluginInstance::PluginPresetList& presets)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->getPresets(instrument, position, presets);
}

void AudioPluginLV2GUIManager::setPreset
(InstrumentId instrument, int position, const QString& uri)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->setPreset(instrument, position, uri);
    updateControls(instrument, position);
}

void AudioPluginLV2GUIManager::loadPreset
(InstrumentId instrument, int position, const QString& file)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->loadPreset(instrument, position, file);
    updateControls(instrument, position);
}

void AudioPluginLV2GUIManager::savePreset
(InstrumentId instrument, int position, const QString& file)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->savePreset(instrument, position, file);
}

bool AudioPluginLV2GUIManager::canEditConnections(InstrumentId instrument,
                                                  int position) const
{
    // we can edit the connections if there is more than one audio
    // input or more than one audio output
    PluginPort::ConnectionList clist;
    getConnections(instrument, position, clist);
    int numInput = 0;
    int numOutput = 0;
    for (const PluginPort::Connection &connection : clist) {
        if (connection.isAudio) {
            if (connection.isOutput) ++numOutput;
            else ++numInput;
        }
    }
    if (numInput > 1 || numOutput > 1) return true;
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

void AudioPluginLV2GUIManager::updateControls(InstrumentId instrument,
                                              int position)
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LV2Utils::PortValues controlInValues;
    lv2utils->getControlInValues(instrument,
                                 position,
                                 controlInValues);
    // Update for each control in value
    for (const auto &pair : controlInValues) {
        int portIndex = pair.first;
        float value = pair.second;
        m_mainWindow->slotChangePluginPort(instrument,
                                           position,
                                           portIndex,
                                           value);
    }

}

}
