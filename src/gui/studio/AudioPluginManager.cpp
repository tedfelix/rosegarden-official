/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "AudioPluginManager.h"

#include "misc/Debug.h"
#include "AudioPluginClipboard.h"
#include "AudioPlugin.h"
#include "base/AudioPluginInstance.h"
#include "gui/application/RosegardenApplication.h"
#include "sound/PluginFactory.h"
#include "sound/PluginIdentifier.h"
#include <qcstring.h>
#include <qdatastream.h>
#include <qmutex.h>
#include <qstring.h>
#include <qthread.h>


namespace Rosegarden
{

AudioPluginManager::AudioPluginManager() :
            m_sampleRate(0),
            m_enumerator(this)
{
//    std::cerr << "AudioPluginManager[" << this << "]::AudioPluginManager - "
//    	      << "trace is ";
//    std::cerr << kdBacktrace() << std::endl;


    // fetch from sequencer
    fetchSampleRate();

    // Clear the plugin clipboard
    //
    m_pluginClipboard.m_pluginNumber = -1;
    m_pluginClipboard.m_program = "";
    m_pluginClipboard.m_controlValues.clear();
    
    m_enumerator.start();
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

    RG_DEBUG << "\n\nAudioPluginManager::Enumerator::run()\n\n" << endl;

    if (!rgapp->noSequencerMode()) {
        // We only waste the time looking for plugins here if we
        // know we're actually going to be able to use them.
        PluginFactory::enumerateAllPlugins(rawPlugins);
    }

    unsigned int i = 0;

    while (i < rawPlugins.size()) {

        QString identifier = rawPlugins[i++];
        QString name = rawPlugins[i++];
        unsigned long uniqueId = rawPlugins[i++].toLong();
        QString label = rawPlugins[i++];
        QString author = rawPlugins[i++];
        QString copyright = rawPlugins[i++];
        bool isSynth = ((rawPlugins[i++]).lower() == "true");
        bool isGrouped = ((rawPlugins[i++]).lower() == "true");
        QString category = rawPlugins[i++];
        unsigned int portCount = rawPlugins[i++].toInt();

        //	std::cerr << "PLUGIN: " << i << ": " << (identifier ? identifier : "(null)") << " unique id " << uniqueId << " / CATEGORY: \"" << (category ? category : "(null)") << "\"" << std::endl;

        AudioPlugin *aP = m_manager->addPlugin(identifier,
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

    RG_DEBUG << "\n\nAudioPluginManager::Enumerator::run() - done\n\n" << endl;
}

AudioPlugin*
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
    AudioPlugin *newPlugin = new AudioPlugin(identifier,
                             name,
                             uniqueId,
                             label,
                             author,
                             copyright,
                             isSynth,
                             isGrouped,
                             category);
    m_plugins.push_back(newPlugin);

    return newPlugin;
}

bool
AudioPluginManager::removePlugin(const QString &identifier)
{
    std::vector<AudioPlugin*>::iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getIdentifier() == identifier) {
            delete *it;
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

    PluginIterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
        names.push_back((*it)->getName());

    return names;
}

AudioPlugin*
AudioPluginManager::getPlugin(int number)
{
    awaitEnumeration();

    if (number < 0 || number > (int(m_plugins.size()) - 1))
        return 0;

    return m_plugins[number];
}

int
AudioPluginManager::getPositionByIdentifier(QString identifier)
{
    awaitEnumeration();

    int pos = 0;
    PluginIterator it = m_plugins.begin();

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

AudioPlugin*
AudioPluginManager::getPluginByIdentifier(QString identifier)
{
    awaitEnumeration();

    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getIdentifier() == identifier)
            return (*it);
    }

    it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if (PluginIdentifier::areIdentifiersSimilar((*it)->getIdentifier(), identifier))
            return (*it);
    }

    return 0;
}

AudioPlugin*
AudioPluginManager::getPluginByUniqueId(unsigned long uniqueId)
{
    awaitEnumeration();

    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it) {
        if ((*it)->getUniqueId() == uniqueId)
            return (*it);
    }

    return 0;
}

PluginIterator
AudioPluginManager::begin()
{
    awaitEnumeration();
    return m_plugins.begin();
}

PluginIterator
AudioPluginManager::end()
{
    awaitEnumeration();
    return m_plugins.end();
}

void
AudioPluginManager::awaitEnumeration()
{
    while (!m_enumerator.isDone()) {
        RG_DEBUG << "\n\nAudioPluginManager::awaitEnumeration() - waiting\n\n" << endl;
//        m_mutex.lock();
        usleep(100000);
//        m_mutex.unlock();
    }
}

void
AudioPluginManager::fetchSampleRate()
{
    QCString replyType;
    QByteArray replyData;

    if (rgapp->sequencerCall("getSampleRate()", replyType, replyData)) {

        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_sampleRate = result;
    }
}

}
