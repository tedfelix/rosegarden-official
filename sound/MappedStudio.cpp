// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.2
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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

#include <dlfcn.h>

#include <qdir.h>

#include "MappedStudio.h"

namespace Rosegarden
{

const MappedObjectProperty MappedObject::FaderLevel = "faderLevel";
const MappedObjectProperty MappedAudioPluginManager::Plugins = "plugins";

MappedStudio::MappedStudio():MappedObject(0,
                                          "MappedStudio",
                                          Studio,
                                          0),
                             m_runningObjectId(1)
{
}

MappedStudio::~MappedStudio()
{
    std::cout << "MappedStudio::~MappedStudio" << std::endl;
    clear();
}


// Object factory
// 
MappedObject*
MappedStudio::createObject(MappedObjectType type)
{
    MappedObject *mO = 0;

    // Ensure we've got an empty slot
    //
    while(getObject(m_runningObjectId))
        m_runningObjectId++;

    mO = createObject(type, m_runningObjectId);

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type, MappedObjectId id)
{
    // fail if the object already exists and it's not zero
    if (id != 0 && getObject(id)) return 0;

    MappedObject *mO = 0;

    if (type == MappedObject::AudioPluginManager)
    {
        mO = new MappedAudioPluginManager(this, id);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(this,
                                  id,
                                  2); // channels
    }
    else if (type == MappedObject::AudioPluginLADSPA)
    {
        // create plugins under the pluginmanager if it exists
        ///
        MappedObject *mAPM =
            getObjectOfType(MappedObject::AudioPluginManager);

        mO = new MappedLADSPAPlugin(mAPM, id);
    }

    // Insert
    if (mO)
    {
        m_objects.push_back(mO);
    }

    return mO;
}

MappedObject*
MappedStudio::getObjectOfType(MappedObjectType type)
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
        if ((*it)->getType() == type)
            return (*it);
    return 0;
}



bool
MappedStudio::destroyObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            delete (*it);
            m_objects.erase(it);
            return true;
        }
    }
   
    return false;
}

bool
MappedStudio::connectInstrument(InstrumentId /*iId*/, MappedObjectId /*msId*/)
{
    return true;
}

bool
MappedStudio::connectObjects(MappedObjectId /*mId1*/, MappedObjectId /*mId2*/)
{
    return true;
}

// Clear down the whole studio
//
void
MappedStudio::clear()
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
        delete (*it);

    m_objects.erase(m_objects.begin(), m_objects.end());

}

MappedObjectPropertyList
MappedStudio::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        // something
    }

    return list;
}


MappedObject*
MappedStudio::getObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;

    for (it = m_objects.begin(); it != m_objects.end(); it++)
        if ((*it)->getId() == id)
            return (*it);

    return 0;
}

MappedObject*
MappedStudio::getFirst(MappedObjectType type)
{
    return getObjectOfType(type);
}

MappedObject*
MappedStudio::getNext(MappedObject *object)
{
    MappedObjectType type = Studio;

    std::vector<MappedObject*>::iterator it = m_objects.begin();

    while (it != m_objects.end())
    {
        if (object->getId() == (*it)->getId())
        {
            type = (*it)->getType();
            it++;
            break;
        }
        it++;
    }

    for (; it != m_objects.end(); it++)
    {
        if (type == (*it)->getType())
            return (*it);
    }

    return 0;
}



MappedObjectValue
MappedAudioFader::getLevel()
{
    return m_level;
}

void
MappedAudioFader::setLevel(MappedObjectValue param)
{
    m_level = param;
}

MappedObjectPropertyList 
MappedAudioFader::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        // etc

    }

    return list;
}

/*
QDataStream&
operator>>(QDataStream &dS, MappedStudio *mS)
{
    // not implemented
    mS->clear();

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, MappedStudio *mS)
{
    dS << mS->getObjects()->size();

    for (unsigned int i = 0; i < mS->getObjects()->size(); i++)
    {
        //dS << m_objects[i]->getId();
        //dS << m_objects[i]->getType();
    }

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedStudio &mS)
{
    mS.clear();

    unsigned int size = 0;
    dS >> size;
    
    //for (unsigned int i = 0; i < size; i++)

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedStudio &mS)
{
    dS << mS.getObjects()->size();

    for (unsigned int i = 0; i < mS.getObjects()->size(); i++)
    {
        //dS << m_objects[i].getId();
        //dS << m_objects[i].getType();
    }

    return dS;
}
*/


MappedAudioPluginManager::MappedAudioPluginManager(
        MappedObject *parent,
        MappedObjectId id)
    :MappedObject(parent,
                 "MappedAudioPluginManager",
                  AudioPluginManager,
                  id,
                  true)
{
}

MappedAudioPluginManager::~MappedAudioPluginManager()
{
}


// If we pass no argument then return the list of plugins
//
MappedObjectPropertyList
MappedAudioPluginManager::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioPluginManager::Plugins);
        /*
        PluginIterator it;
        for (it = m_plugins.begin(); it != m_plugins.end(); it++)
            list.push_back(MappedObjectProperty((*it)->getName().c_str()));
            */

    }
    else if (property == MappedAudioPluginManager::Plugins)
    {
        MappedStudio *studio = dynamic_cast<MappedStudio*>(m_parent);

        if (studio)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (studio->getFirst(AudioPluginLADSPA));

            while (plugin)
            {
                list.push_back(MappedObjectProperty
                            (plugin->getPluginName().c_str()));
                plugin = dynamic_cast<MappedLADSPAPlugin*>
                            (studio->getNext(plugin));
            }
        }
    }


    return list;
}

void
MappedAudioPluginManager::discoverPlugins(MappedStudio *studio)
{
    QDir dir(QString(m_path.c_str()), "*.so");
    clearPlugins(studio);

    for (unsigned int i = 0; i < dir.count(); i++ )
        enumeratePlugin(studio,
                m_path + std::string("/") + std::string(dir[i].data()));
    
}

void
MappedAudioPluginManager::clearPlugins(MappedStudio *studio)
{
    MappedObject *object;
    while ((object = studio->getObjectOfType(AudioPluginLADSPA)))
    {
        studio->destroyObject(object->getId());
    }
}


void
MappedAudioPluginManager::getenvLADSPAPath()
{
    char *path = getenv("LADSPA_PATH");

    if (!path) m_path = "";
    else m_path = std::string(path);

    // try a default value
    if (m_path == "")
        m_path = "/usr/lib/ladspa";

}

void
MappedAudioPluginManager::setLADSPAPath(const std::string &path)
{
    m_path = path;
}


void
MappedAudioPluginManager::addLADSPAPath(const std::string &path)
{
    m_path += path;
}

void
MappedAudioPluginManager::enumeratePlugin(MappedStudio *studio,
                                          const std::string& path)
{
#ifdef HAVE_LADSPA
    LADSPA_Descriptor_Function descrFn = 0;
    void *pluginHandle = 0;

    pluginHandle = dlopen(path.c_str(), RTLD_LAZY);

    descrFn = (LADSPA_Descriptor_Function)dlsym(pluginHandle,
                                                "ladspa_descriptor");

    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;

        int index = 0;

        do
        {
            descriptor = descrFn(index);

            if (descriptor)
            {
                // The sequencer is only interested in plugins that 
                // will be able to run in real time.
                //
                if (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
                {
                    MappedLADSPAPlugin *plugin =
                        dynamic_cast<MappedLADSPAPlugin*>
                            (studio->createObject(AudioPluginLADSPA));

                    plugin->setLibraryName(path);
                    plugin->setPluginName(descriptor->Name);
                    
                    //plugin->setDescriptor(descriptor);
                }
                index++;
            }
        }
        while(descriptor);
    }

    if(dlclose(pluginHandle) != 0)
        std::cerr << "PluginManager::loadPlugin - can't unload plugin"
                  << std::endl;

#endif // HAVE_LADSPA
}

#ifdef HAVE_LADSPA
MappedObjectPropertyList
MappedLADSPAPlugin::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;
    
    if (property == "")
    {
        // our LADSPA properties
        list.push_back(MappedObjectProperty("id"));
        list.push_back(MappedObjectProperty("label"));
        list.push_back(MappedObjectProperty("name"));
        list.push_back(MappedObjectProperty("author"));
        list.push_back(MappedObjectProperty("copyright"));
        list.push_back(MappedObjectProperty("numberofports"));
    }
    else if (m_descriptor)
    {
        if (property == "id")
            list.push_back(MappedObjectProperty("%1").arg(m_descriptor->UniqueID));
        else if (property == "label")
            list.push_back(MappedObjectProperty(m_descriptor->Label));
        else if (property == "name")
            list.push_back(MappedObjectProperty(m_descriptor->Name));
        else if (property == "author")
            list.push_back(MappedObjectProperty(m_descriptor->Maker));
        else if (property == "copyright")
            list.push_back(MappedObjectProperty(m_descriptor->Copyright));
        else if (property == "numberofports")
            list.push_back(MappedObjectProperty("%1").arg(m_descriptor->PortCount));
    }

    return list;

}
#endif // HAVE_LADSPA




}


