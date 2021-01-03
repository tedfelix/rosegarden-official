/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPluginManager]"

#include "AudioPluginManager.h"

#include "misc/Debug.h"
#include "AudioPluginClipboard.h"
#include "AudioPlugin.h"
#include "base/AudioPluginInstance.h"
#include "sequencer/RosegardenSequencer.h"
#include "sound/PluginFactory.h"
#include "sound/PluginIdentifier.h"

#include <QByteArray>
#include <QDataStream>
#include <QMutex>
#include <QString>
#include <QThread>

#include <unistd.h>



namespace Rosegarden
{

AudioPluginManager::AudioPluginManager(bool enableSound) :
    m_sampleRate(0),
    m_enumerator(this),
    m_enableSound(enableSound)
{
    //RG_DEBUG << "ctor";

    // Clear the plugin clipboard
    //
    m_pluginClipboard.m_pluginNumber = -1;
    m_pluginClipboard.m_program = "";
    m_pluginClipboard.m_controlValues.clear();
    
    m_enumerator.start();

    //RG_DEBUG << "ctor done";
}

AudioPluginManager::Enumerator::Enumerator(AudioPluginManager *manager) :
        m_manager(manager),
        m_done(false)
{}

void
AudioPluginManager::Enumerator::run()
{
    QMutexLocker locker(&(m_manager->m_mutex));
    MappedObjectPropertyList rawPlugins;

    //RG_DEBUG << "Enumerator::run()...";

    if (m_manager->m_enableSound) {
        //RG_DEBUG << "  sound was enabled, enumerating plugins...";

        // We only waste the time looking for plugins here if we
        // know we're actually going to be able to use them.
        PluginFactory::enumerateAllPlugins(rawPlugins);
    }

    size_t i = 0;

    while (i < rawPlugins.size()) {

        QString identifier = rawPlugins[i++];
        QString name = rawPlugins[i++];
        unsigned long uniqueId = rawPlugins[i++].toLong();
        QString label = rawPlugins[i++];
        QString author = rawPlugins[i++];
        QString copyright = rawPlugins[i++];
        bool isSynth = ((rawPlugins[i++]).toLower() == "true");
        bool isGrouped = ((rawPlugins[i++]).toLower() == "true");
        QString category = rawPlugins[i++];
        unsigned int portCount = rawPlugins[i++].toInt();

        //RG_DEBUG << "PLUGIN: " << i << ": " << (identifier != "" ? identifier : "(null)") << " unique id " << uniqueId << " / CATEGORY: \"" << (category != "" ? category : "(null)") << "\"";
        //RG_DEBUG << "Enumerator::run(): Plugin name:" << name;

        QSharedPointer<AudioPlugin> aP = m_manager->addPlugin(
                                               identifier,
                                               name,
                                               uniqueId,
                                               label,
                                               author,
                                               copyright,
                                               isSynth,
                                               isGrouped,
                                               category);

        for (unsigned int j = 0; j < portCount; j++) {

            int number = rawPlugins[i++].toInt();
            name = rawPlugins[i++];
            PluginPort::PortType type =
                PluginPort::PortType(rawPlugins[i++].toInt());
            PluginPort::PortDisplayHint hint =
                PluginPort::PortDisplayHint(rawPlugins[i++].toInt());
            PortData lowerBound = rawPlugins[i++].toFloat();
            PortData upperBound = rawPlugins[i++].toFloat();
            PortData defaultValue = rawPlugins[i++].toFloat();

            aP->addPort(number,
                        name,
                        type,
                        hint,
                        lowerBound,
                        upperBound,
                        defaultValue);
        }
    }

    m_done = true;

    //RG_DEBUG << "Enumerator::run() - done";
}

QSharedPointer<AudioPlugin>
AudioPluginManager::addPlugin(const QString &identifier,
                              const QString &name,
                              unsigned long uniqueId,
                              const QString &label,
                              const QString &author,
                              const QString &copyright,
                              bool isSynth,
                              bool isGrouped,
                              const QString &category)
{
    QSharedPointer<AudioPlugin> newPlugin(
             new AudioPlugin(identifier,
                             name,
                             uniqueId,
                             label,
                             author,
                             copyright,
                             isSynth,
                             isGrouped,
                             category));
    m_plugins.push_back(newPlugin);

    return newPlugin;
}

bool
AudioPluginManager::removePlugin(const QString &identifier)
{
    iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getIdentifier() == identifier) {
            m_plugins.erase(it);
            return true;
        }
    }

    return false;
}

std::vector<QString>
AudioPluginManager::getPluginNames()
{
    awaitEnumeration();

    std::vector<QString> names;

    iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
        names.push_back((*it)->getName());

    return names;
}

QSharedPointer<AudioPlugin>
AudioPluginManager::getPlugin(int number)
{
    awaitEnumeration();

    if (number < 0 || number > (int(m_plugins.size()) - 1))
        return {};

    return m_plugins[number];
}

int
AudioPluginManager::getPositionByIdentifier(QString identifier)
{
    awaitEnumeration();

    int pos = 0;
    iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getIdentifier() == identifier)
            return pos;

        pos++;
    }

    pos = 0;
    it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if (PluginIdentifier::areIdentifiersSimilar((*it)->getIdentifier(), identifier))
            return pos;

        pos++;
    }

    return -1;
}

QSharedPointer<AudioPlugin>
AudioPluginManager::getPluginByIdentifier(QString identifier)
{
    awaitEnumeration();

    iterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getIdentifier() == identifier)
            return (*it);
    }

    it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if (PluginIdentifier::areIdentifiersSimilar((*it)->getIdentifier(), identifier))
            return (*it);
    }

    return {};
}

QSharedPointer<AudioPlugin>
AudioPluginManager::getPluginByUniqueId(unsigned long uniqueId)
{
    awaitEnumeration();

    iterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getUniqueId() == uniqueId)
            return (*it);
    }

    return {};
}

AudioPluginManager::iterator
AudioPluginManager::begin()
{
    awaitEnumeration();
    return m_plugins.begin();
}

AudioPluginManager::iterator
AudioPluginManager::end()
{
    awaitEnumeration();
    return m_plugins.end();
}

void
AudioPluginManager::awaitEnumeration()
{
    // TODO: this should use a QSemaphore instead
    while (!m_enumerator.isDone()) {
        RG_DEBUG << "\n\nAudioPluginManager::awaitEnumeration() - waiting\n\n";
        usleep(100000); // 100 ms
    }
}

unsigned int
AudioPluginManager::getSampleRate() const
{
    if (m_sampleRate == 0) {
        m_sampleRate = RosegardenSequencer::getInstance()->getSampleRate();
    }
    return m_sampleRate;
}

}
