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

#include "MappedStudio.h"
#include "PluginManager.h"

namespace Rosegarden
{

const MappedObjectProperty MappedObject::FaderLevel = "faderLevel";
const MappedObjectProperty MappedAudioPluginManager::Plugins = "plugins";

MappedStudio::MappedStudio():MappedObject("MappedStudio",
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

    if (type == MappedObject::AudioPluginManager)
    {
        // Check and return if exists
        if ((mO = getObjectOfType(type)))
            return mO;

        mO = new MappedAudioPluginManager(m_runningObjectId);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(m_runningObjectId,
                                  2); // channels
    }

    // Insert
    if (mO)
    {
        m_objects.push_back(mO);
    }

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type, MappedObjectId id)
{
    // fail if the object already exists
    if (getObject(id)) return 0;

    MappedObject *mO = 0;

    if (type == MappedObject::AudioPluginManager)
    {
        mO = new MappedAudioPluginManager(id);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(id,
                                  2); // channels
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
    for (it = m_objects.begin(); it != m_objects.end(); ++it)
        if ((*it)->getType() == type)
            return (*it);
    return 0;
}



bool
MappedStudio::destroyItem(MappedObjectId /*id*/)
{
    return true;
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
    for (it = m_objects.begin(); it != m_objects.end(); ++it)
        delete (*it);

    m_objects.erase(m_objects.begin(), m_objects.end());

}

MappedObjectPropertyList
MappedStudio::getPropertyList(const MappedObjectProperty &property)
{
    return MappedObjectPropertyList();
}


MappedObject*
MappedStudio::getObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;

    for (it = m_objects.begin(); it != m_objects.end(); ++it)
        if ((*it)->getId() == id)
            return (*it);

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
    return MappedObjectPropertyList();
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


MappedAudioPluginManager::MappedAudioPluginManager(MappedObjectId id)
    :MappedObject("MappedAudioPluginManager",
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
        PluginIterator it;
        for (it = m_plugins.begin(); it != m_plugins.end(); it++)
            list.push_back(MappedObjectProperty((*it)->getName().c_str()));

    }

    return list;
}


MappedObjectPropertyList
MappedAudioPluginManager::getPluginProperty(
        PluginId id,
        const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;
    Plugin *plugin = getPluginForId(id);

    if (plugin)
        list = plugin->getPropertyList(property);

    return list;
}

MappedObjectPropertyList
MappedAudioPluginManager::getPortProperty(PluginId pluginId,
                                          PluginPortId portId,
                                          const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;
    return list;
}



}

