/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#ifdef HAVE_LILV
#include "gui/studio/AudioPluginLV2GUIManager.h"
#endif


namespace Rosegarden
{


AudioPluginGUIManager::AudioPluginGUIManager(RosegardenMainWindow *mainWindow) :
    m_studio(nullptr),
    m_oscManager(new AudioPluginOSCGUIManager(mainWindow))
#ifdef HAVE_LILV
    ,
    m_lv2Manager(new AudioPluginLV2GUIManager(mainWindow))
#endif
{
}

AudioPluginGUIManager::~AudioPluginGUIManager()
{
    delete m_oscManager;
#ifdef HAVE_LILV
    delete m_lv2Manager;
#endif
}

void AudioPluginGUIManager::setStudio(Studio *studio)
{
    m_studio = studio;
    m_oscManager->setStudio(studio);
#ifdef HAVE_LILV
    m_lv2Manager->setStudio(studio);
#endif
}

bool AudioPluginGUIManager::hasGUI(InstrumentId instrument, int position)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    RG_DEBUG << "hasGui" << instrument << position << arch;
    // ??? Switch on type.  Most of these functions are switch on type
    //     delegation functions.  Would polymorphism make more sense?
    switch(arch) {
    case OSC:
        return m_oscManager->hasGUI(instrument, position);
    case LV2:
#ifdef HAVE_LILV
        return m_lv2Manager->hasGUI(instrument, position);
#else
        return false;
#endif
    case UNKNOWN:
        return false;
    }
    return false;
}

void AudioPluginGUIManager::showGUI(InstrumentId instrument, int position)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->showGUI(instrument, position);
        break;
    case LV2:
#ifdef HAVE_LILV
        m_lv2Manager->showGUI(instrument, position);
#endif
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::stopGUI(InstrumentId instrument, int position)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->stopGUI(instrument, position);
        break;
    case LV2:
#ifdef HAVE_LILV
        m_lv2Manager->stopGUI(instrument, position);
#endif
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::stopAllGUIs()
{
    m_oscManager->stopAllGUIs();
#ifdef HAVE_LILV
    m_lv2Manager->stopAllGUIs();
#endif
}

    bool AudioPluginGUIManager::hasParameters(InstrumentId instrument,
                                        int position) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return false;
#ifdef HAVE_LILV
    return m_lv2Manager->hasParameters(instrument, position);
#else
    return false;
#endif
}

void AudioPluginGUIManager::getParameters
(InstrumentId instrument,
 int position,
 AudioPluginInstance::PluginParameters& params) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->getParameters(instrument, position, params);
#else
    return;
#endif
}

void AudioPluginGUIManager::updatePluginParameter
(InstrumentId instrument,
 int position,
 const QString& paramId,
 const AudioPluginInstance::PluginParameter& param) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->updatePluginParameter(instrument, position, paramId, param);
#else
    return;
#endif
}

bool AudioPluginGUIManager::canUsePresets(InstrumentId instrument,
                                          int position) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // lv2 plugins can handle presets
    if (arch == LV2) return true;
    return false;
}

void AudioPluginGUIManager::getPresets
(InstrumentId instrument,
 int position,
 AudioPluginInstance::PluginPresetList& presets) const
{
    presets.clear();
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // lv2 plugins can handle presets
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->getPresets(instrument, position, presets);
#endif
}

void AudioPluginGUIManager::setPreset
(InstrumentId instrument, int position, const QString& uri) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // lv2 plugins can handle presets
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->setPreset(instrument, position, uri);
#endif
}

void AudioPluginGUIManager::loadPreset
(InstrumentId instrument, int position, const QString& file) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // lv2 plugins can handle presets
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->loadPreset(instrument, position, file);
#endif
}

void AudioPluginGUIManager::savePreset
(InstrumentId instrument, int position, const QString& file) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // lv2 plugins can handle presets
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->savePreset(instrument, position, file);
#endif
}

bool AudioPluginGUIManager::canEditConnections(InstrumentId instrument,
                                               int position) const
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return false;
#ifdef HAVE_LILV
    return m_lv2Manager->canEditConnections(instrument, position);
#else
    return false;
#endif
}

void AudioPluginGUIManager::getConnections
(InstrumentId instrument,
 int position,
 PluginPort::ConnectionList& clist) const
{
    clist.connections.clear();
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return;
#ifdef HAVE_LILV
    m_lv2Manager->getConnections(instrument, position, clist);
#endif
}

void AudioPluginGUIManager::setConnections
(InstrumentId instrument,
 int position,
 const PluginPort::ConnectionList& clist) const
{
#ifdef HAVE_LILV
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only lv2
    if (arch != LV2) return;
    m_lv2Manager->setConnections(instrument, position, clist);
#else
    // Address compiler warnings.
    (void)instrument;
    (void)position;
    (void)clist;
#endif
}

void AudioPluginGUIManager::updateProgram(InstrumentId instrument, int position)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only osc
    if (arch != OSC) return;
    m_oscManager->updateProgram(instrument, position);
}

void AudioPluginGUIManager::updatePort(InstrumentId instrument, int position,
                                       int port)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    switch(arch) {
    case OSC:
        m_oscManager->updatePort(instrument, position, port);
        break;
    case LV2:
#ifdef HAVE_LILV
        m_lv2Manager->updatePort(instrument, position, port);
#endif
        break;
    case UNKNOWN:
        break;
    }
}

void AudioPluginGUIManager::updateConfiguration(InstrumentId instrument,
                                                int position, QString key)
{
    PluginGUIArchitecture arch = getArchitecture(instrument, position);
    // only osc
    if (arch != OSC) return;
    m_oscManager->updateConfiguration(instrument, position, key);
}

AudioPluginGUIManager::PluginGUIArchitecture
AudioPluginGUIManager::getArchitecture
(InstrumentId instrument, int position) const
{
    if (!m_studio) return UNKNOWN;
    PluginContainer *container = m_studio->getContainerById(instrument);
    if (!container) return UNKNOWN;

    AudioPluginInstance *pluginInstance = container->getPlugin(position);
    if (!pluginInstance) return UNKNOWN;

    QString id = strtoqstr(pluginInstance->getIdentifier());
    QString iType, iSoName, iLabel, arch;
    PluginIdentifier::parseIdentifier(id, iType, iSoName, iLabel, arch);
    RG_DEBUG << "arch:" << arch;
    if (arch == "ladspa" || arch == "dssi") return OSC;
    if (arch == "lv2") return LV2;
    return UNKNOWN;
}


}
