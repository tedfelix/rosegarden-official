// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
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


MappedStudio::MappedStudio():MappedObject("MappedStudio",
                                          Studio,
                                          0),
                             m_runningObjectId(1)
{
}

MappedStudio::~MappedStudio()
{
    clear();
}


// Object factory
// 
MappedObject*
MappedStudio::createObject(MappedObjectType type)
{
    MappedObject *mO = 0;

    if (type == MappedObject::AudioPluginManager)
    {
        // Check for existence and if it does exists already
        // we can't create a new one.
        // 
        if ((mO = getObjectOfType(type)))
            return 0;

        mO = new MappedAudioPluginManager(m_runningObjectId);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(m_runningObjectId,
                                  2); // channels
    }

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

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

MappedObject*
MappedStudio::getObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;

    for (it = m_objects.begin(); it != m_objects.end(); ++it)
        if ((*it)->getId() == id)
            return (*it);

    return 0;
}




MappedObjectParameter
MappedAudioFader::getLevel()
{
    return m_level;
}

void
MappedAudioFader::setLevel(MappedObjectParameter param)
{
    m_level = param;
}

QDataStream&
operator>>(QDataStream &dS, MappedStudio *mS)
{
    // not implemented
    return dS;
}


QDataStream&
operator<<(QDataStream &dS, MappedStudio *mS)
{
    // not implemented
    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedStudio &mS)
{
    // not implemented
    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedStudio &mS)
{
    // not implemented
    return dS;
}


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



}

