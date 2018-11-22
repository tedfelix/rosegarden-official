/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MappedStudio]"


#include "MappedStudio.h"
#include "SoundDriver.h"
#include "PluginFactory.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <pthread.h> // for mutex

// #define DEBUG_MAPPEDSTUDIO 1

namespace Rosegarden
{

static pthread_mutex_t mappedObjectContainerLock;

#ifdef DEBUG_MAPPEDSTUDIO
static int approxLockCount = 0;
#endif

static inline void getLock(const char *file, int line)
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cerr << "Acquiring MappedStudio container lock at " << file << ":" << line << ": count " << approxLockCount++ << std::endl;
#else
    (void)file; (void)line;
#endif

    pthread_mutex_lock(&mappedObjectContainerLock);
}

static inline void releaseLock(const char *file, int line)
{
    pthread_mutex_unlock(&mappedObjectContainerLock);
#ifdef DEBUG_MAPPEDSTUDIO

    std::cerr << "Released container lock at " << file << ":" << line << ": count " << --approxLockCount << std::endl;
#else
    (void)file; (void)line;
#endif
}

#define GET_LOCK getLock(__FILE__,__LINE__)
#define RELEASE_LOCK releaseLock(__FILE__,__LINE__)

// These stream functions are stolen and adapted from Qt3 QVector
//
// ** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
//
QDataStream& operator>>(QDataStream& s, MappedObjectIdList& v)
{
    v.clear();
    quint32 c;
    s >> c;
    v.resize(c);
    for (quint32 i = 0; i < c; ++i) {
        MappedObjectId t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const MappedObjectIdList& v)
{
    s << (quint32)v.size();
    MappedObjectIdList::const_iterator it = v.begin();
    for ( ; it != v.end(); ++it )
        s << *it;
    return s;
}

QDataStream& operator>>(QDataStream& s, MappedObjectPropertyList& v)
{
    v.clear();
    quint32 c;
    s >> c;
    v.resize(c);
    for (quint32 i = 0; i < c; ++i) {
        MappedObjectProperty t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const MappedObjectPropertyList& v)
{
    s << (quint32)v.size();
    MappedObjectPropertyList::const_iterator it = v.begin();
    for ( ; it != v.end(); ++it )
        s << *it;
    return s;
}

QDataStream& operator>>(QDataStream& s, MappedObjectValueList& v)
{
    v.clear();
    quint32 c;
    s >> c;
    v.resize(c);
    for (quint32 i = 0; i < c; ++i) {
        MappedObjectValue t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const MappedObjectValueList& v)
{
    s << (quint32)v.size();
    MappedObjectValueList::const_iterator it = v.begin();
    for ( ; it != v.end(); ++it )
        s << *it;
    return s;
}

// Define our object properties - these can be queried and set.
//

// General things
//
const MappedObjectProperty MappedObject::Name = "name";
const MappedObjectProperty MappedObject::Instrument = "instrument";
const MappedObjectProperty MappedObject::Position = "position";

const MappedObjectProperty MappedConnectableObject::ConnectionsIn = "connectionsIn";
const MappedObjectProperty MappedConnectableObject::ConnectionsOut = "connectionsOut";

const MappedObjectProperty MappedAudioFader::Channels = "channels";
const MappedObjectProperty MappedAudioFader::FaderLevel = "faderLevel";
const MappedObjectProperty MappedAudioFader::FaderRecordLevel = "faderRecordLevel";
const MappedObjectProperty MappedAudioFader::Pan = "pan";
const MappedObjectProperty MappedAudioFader::InputChannel = "inputChannel";

const MappedObjectProperty MappedAudioBuss::BussId = "bussId";
const MappedObjectProperty MappedAudioBuss::Level = "level";
const MappedObjectProperty MappedAudioBuss::Pan = "pan";

const MappedObjectProperty MappedAudioInput::InputNumber = "inputNumber";

const MappedObjectProperty MappedPluginSlot::Identifier = "identifier";
const MappedObjectProperty MappedPluginSlot::PluginName = "pluginname";
const MappedObjectProperty MappedPluginSlot::Label = "label";
const MappedObjectProperty MappedPluginSlot::Author = "author";
const MappedObjectProperty MappedPluginSlot::Copyright = "copyright";
const MappedObjectProperty MappedPluginSlot::Category = "category";
const MappedObjectProperty MappedPluginSlot::PortCount = "portcount";
const MappedObjectProperty MappedPluginSlot::Ports = "ports";
const MappedObjectProperty MappedPluginSlot::Instrument = "instrument";
const MappedObjectProperty MappedPluginSlot::Position = "position";
const MappedObjectProperty MappedPluginSlot::Bypassed = "bypassed";
const MappedObjectProperty MappedPluginSlot::Programs = "programs";
const MappedObjectProperty MappedPluginSlot::Program = "program";
const MappedObjectProperty MappedPluginSlot::Configuration = "configuration";

const MappedObjectProperty MappedPluginPort::PortNumber = "portnumber";
const MappedObjectProperty MappedPluginPort::Name = "name";
const MappedObjectProperty MappedPluginPort::Minimum = "minimum";
const MappedObjectProperty MappedPluginPort::Maximum = "maximum";
const MappedObjectProperty MappedPluginPort::Default = "default";
const MappedObjectProperty MappedPluginPort::DisplayHint = "displayhint";
const MappedObjectProperty MappedPluginPort::Value = "value";

// --------- MappedObject ---------
//

void
MappedObject::addChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); ++it)
        if ((*it) == object)
            return ;

    m_children.push_back(object);
}

void
MappedObject::removeChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); ++it) {
        if ((*it) == object) {
            m_children.erase(it);
            return ;
        }
    }
}

// Return all child ids
//
MappedObjectPropertyList
MappedObject::getChildren()
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); ++it)
        list.push_back(QString("%1").arg((*it)->getId()));

    return list;
}


// Return all child ids of a certain type
//
MappedObjectPropertyList
MappedObject::getChildren(MappedObjectType type)
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); ++it) {
        if ((*it)->getType() == type)
            list.push_back(QString("%1").arg((*it)->getId()));
    }

    return list;
}

void
MappedObject::destroyChildren()
{
    // remove references from the studio as well as from the object
    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio*>(studioObject))
        studioObject = studioObject->getParent();

    // see note in destroy() below

    std::vector<MappedObject *> children = m_children;
    m_children.clear();

    std::vector<MappedObject *>::iterator it = children.begin();
    for (; it != children.end(); ++it)
        (*it)->destroy(); // remove from studio and destroy
}

// Destroy this object and remove it from the studio and
// do the same for all its children.
//
void
MappedObject::destroy()
{
    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio*>(studioObject))
        studioObject = studioObject->getParent();

    MappedStudio *studio = dynamic_cast<MappedStudio*>(studioObject);

    // The destroy method on each child calls studio->clearObject,
    // which calls back on the parent (in this case us) to remove the
    // child.  (That's necessary for the case of destroying a plugin,
    // where we need to remove it from its plugin manager -- etc.)  So
    // we don't want to be iterating over m_children here, as it will
    // change from under us.

    std::vector<MappedObject *> children = m_children;
    m_children.clear();

    std::vector<MappedObject *>::iterator it = children.begin();
    for (; it != children.end(); ++it) {
        (*it)->destroy();
    }

    (void)studio->clearObject(m_id);
    delete this;
}


// ------- MappedStudio -------
//

MappedStudio::MappedStudio() :
        MappedObject(nullptr,
                     "MappedStudio",
                     Studio,
                     0),
        m_runningObjectId(1)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef HAVE_PTHREAD_MUTEX_RECURSIVE

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
#endif

    pthread_mutex_init(&mappedObjectContainerLock, &attr);
}

MappedStudio::~MappedStudio()
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedStudio::~MappedStudio" << std::endl;
#endif

    clear();
}


// Object factory
//
MappedObject*
MappedStudio::createObject(MappedObjectType type)
{
    GET_LOCK;

    MappedObject *mO = nullptr;

    // Ensure we've got an empty slot
    //
    while (getObjectById(m_runningObjectId)) m_runningObjectId++;

    mO = createObject(type, m_runningObjectId);

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    RELEASE_LOCK;
    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type,
                           MappedObjectId id)
{
    GET_LOCK;

    // fail if the object already exists and it's not zero
    if (id != 0 && getObjectById(id)) {
        RELEASE_LOCK;
        return nullptr;
    }

    MappedObject *mO = nullptr;

    if (type == MappedObject::AudioFader) {
        mO = new MappedAudioFader(this,
                                  id,
                                  2); // channels

        // push to the studio's child stack
        addChild(mO);
    } else if (type == MappedObject::AudioBuss) {
        mO = new MappedAudioBuss(this,
                                 id);

        // push to the studio's child stack
        addChild(mO);
    } else if (type == MappedObject::AudioInput) {
        mO = new MappedAudioInput(this,
                                  id);

        // push to the studio's child stack
        addChild(mO);
    } else if (type == MappedObject::PluginSlot) {
        mO = new MappedPluginSlot(this,
                                  id);
        addChild(mO);
    } else if (type == MappedObject::PluginPort) {
        mO = new MappedPluginPort(this,
                                  id);
        // reset the port's parent after creation outside this method
    }

    // Insert
    if (mO) {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "Adding object " << id << " to category " << type << std::endl;
#endif

        m_objects[type][id] = mO;
    }

    RELEASE_LOCK;

    return mO;
}

MappedObject*
MappedStudio::getObjectOfType(MappedObjectType type)
{
    MappedObject *rv = nullptr;

    GET_LOCK;

    MappedObjectCategory &category = m_objects[type];
    if (!category.empty())
        rv = category.begin()->second;

    RELEASE_LOCK;

    return rv;
}

std::vector<MappedObject *>
MappedStudio::getObjectsOfType(MappedObjectType type)
{
    std::vector<MappedObject *> rv;

    GET_LOCK;

    MappedObjectCategory &category = m_objects[type];

    for (MappedObjectCategory::iterator i = category.begin();
            i != category.end(); ++i) {
        rv.push_back(i->second);
    }

    RELEASE_LOCK;

    return rv;
}

unsigned int
MappedStudio::getObjectCount(MappedObjectType type)
{
    unsigned int count = 0;

    GET_LOCK;

    MappedObjectCategory &category = m_objects[type];
    count = (unsigned int)category.size();

    RELEASE_LOCK;

    return count;
}


bool
MappedStudio::destroyObject(MappedObjectId id)
{
    GET_LOCK;

    MappedObject *obj = getObjectById(id);

    bool rv = false;

    if (obj) {
        obj->destroy();
        rv = true;
    }

    RELEASE_LOCK;

    return rv;
}

bool
MappedStudio::connectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    GET_LOCK;

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
        dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
        dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
        obj1->addConnection(MappedConnectableObject::Out, mId2);
        obj2->addConnection(MappedConnectableObject::In, mId1);
        rv = true;
    }

    RELEASE_LOCK;

    return rv;
}

bool
MappedStudio::disconnectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    GET_LOCK;

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
        dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
        dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
        obj1->removeConnection(MappedConnectableObject::Out, mId2);
        obj2->removeConnection(MappedConnectableObject::In, mId1);
        rv = true;
    }

    RELEASE_LOCK;

    return rv;
}

bool
MappedStudio::disconnectObject(MappedObjectId mId)
{
    GET_LOCK;

    bool rv = false;

    MappedConnectableObject *obj =
        dynamic_cast<MappedConnectableObject *>(getObjectById(mId));

    if (obj) {
        while (1) {
            MappedObjectValueList list =
                obj->getConnections(MappedConnectableObject::In);
            if (list.empty())
                break;
            MappedObjectId otherId = MappedObjectId(*list.begin());
            disconnectObjects(otherId, mId);
        }
        while (1) {
            MappedObjectValueList list =
                obj->getConnections(MappedConnectableObject::Out);
            if (list.empty())
                break;
            MappedObjectId otherId = MappedObjectId(*list.begin());
            disconnectObjects(mId, otherId);
        }
    }

    rv = true;

    RELEASE_LOCK;

    return rv;
}



// Clear down the whole studio
//
void
MappedStudio::clear()
{
    GET_LOCK;

    for (MappedObjectMap::iterator i = m_objects.begin();
            i != m_objects.end(); ++i) {

        for (MappedObjectCategory::iterator j = i->second.begin();
                j != i->second.end(); ++j) {

            delete j->second;
        }
    }

    m_objects.clear();

    // reset running object id
    m_runningObjectId = 1;

    RELEASE_LOCK;
}

bool
MappedStudio::clearObject(MappedObjectId id)
{
    bool rv = false;

    GET_LOCK;

    for (MappedObjectMap::iterator i = m_objects.begin();
            i != m_objects.end(); ++i) {

        MappedObjectCategory::iterator j = i->second.find(id);
        if (j != i->second.end()) {
            // if the object has a parent other than the studio,
            // persuade that parent to abandon it
            MappedObject *parent = j->second->getParent();
            if (parent && !dynamic_cast<MappedStudio *>(parent)) {
                parent->removeChild(j->second);
            }

            i->second.erase(j);
            rv = true;
            break;
        }
    }

    RELEASE_LOCK;

    return rv;
}


MappedObjectPropertyList
MappedStudio::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        // something
    }

    return list;
}

bool
MappedStudio::getProperty(const MappedObjectProperty &,
                          MappedObjectValue &)
{
    return false;
}

MappedObject*
MappedStudio::getObjectById(MappedObjectId id)
{
    GET_LOCK;
    MappedObject *rv = nullptr;

    for (MappedObjectMap::iterator i = m_objects.begin();
            i != m_objects.end(); ++i) {

        MappedObjectCategory::iterator j = i->second.find(id);
        if (j != i->second.end()) {
            rv = j->second;
            break;
        }
    }

    RELEASE_LOCK;
    return rv;
}

MappedObject*
MappedStudio::getObjectByIdAndType(MappedObjectId id, MappedObjectType type)
{
    GET_LOCK;
    MappedObject *rv = nullptr;

    MappedObjectCategory &category = m_objects[type];
    MappedObjectCategory::iterator i = category.find(id);
    if (i != category.end()) {
        rv = i->second;
    }

    RELEASE_LOCK;
    return rv;
}

MappedObject*
MappedStudio::getFirst(MappedObjectType type)
{
    return getObjectOfType(type);
}

MappedObject*
MappedStudio::getNext(MappedObject *object)
{
    GET_LOCK;

    MappedObjectCategory &category = m_objects[object->getType()];

    bool next = false;
    MappedObject *rv = nullptr;

    for (MappedObjectCategory::iterator i = category.begin();
            i != category.end(); ++i) {
        if (i->second->getId() == object->getId())
            next = true;
        else if (next) {
            rv = i->second;
            break;
        }
    }

    RELEASE_LOCK;
    return rv;
}

void
MappedStudio::setProperty(const MappedObjectProperty &property,
                          MappedObjectValue /*value*/)
{
    if (property == "") {}

}

MappedAudioFader *
MappedStudio::getAudioFader(InstrumentId id)
{
    GET_LOCK;

    MappedObjectCategory &category = m_objects[AudioFader];
    MappedAudioFader *rv = nullptr;

    for (MappedObjectCategory::iterator i = category.begin();
            i != category.end(); ++i) {
        MappedAudioFader *fader = dynamic_cast<MappedAudioFader *>(i->second);
        if (fader && (fader->getInstrument() == id)) {
            rv = fader;
            break;
        }
    }

    RELEASE_LOCK;
    return rv;
}

MappedAudioBuss *
MappedStudio::getAudioBuss(int bussNumber)
{
    GET_LOCK;

    MappedObjectCategory &category = m_objects[AudioBuss];
    MappedAudioBuss *rv = nullptr;

    for (MappedObjectCategory::iterator i = category.begin();
            i != category.end(); ++i) {
        MappedAudioBuss *buss = dynamic_cast<MappedAudioBuss *>(i->second);
        if (buss && (buss->getBussId() == bussNumber)) {
            rv = buss;
            break;
        }
    }

    RELEASE_LOCK;
    return rv;
}

MappedAudioInput *
MappedStudio::getAudioInput(int inputNumber)
{
    GET_LOCK;

    MappedObjectCategory &category = m_objects[AudioInput];
    MappedAudioInput *rv = nullptr;

    for (MappedObjectCategory::iterator i = category.begin();
            i != category.end(); ++i) {
        MappedAudioInput *input = dynamic_cast<MappedAudioInput *>(i->second);
        if (input && (input->getInputNumber() == inputNumber)) {
            rv = input;
            break;
        }
    }

    RELEASE_LOCK;
    return rv;
}


// -------------- MappedConnectableObject -----------------
//
//
MappedConnectableObject::MappedConnectableObject(MappedObject *parent,
        const std::string &name,
        MappedObjectType type,
        MappedObjectId id):
        MappedObject(parent,
                     name,
                     type,
                     id)
{}

MappedConnectableObject::~MappedConnectableObject()
{}

void
MappedConnectableObject::setConnections(ConnectionDirection dir,
                                        MappedObjectValueList conns)
{
    if (dir == In)
        m_connectionsIn = conns;
    else
        m_connectionsOut = conns;
}

void
MappedConnectableObject::addConnection(ConnectionDirection dir,
                                       MappedObjectId id)
{
    MappedObjectValueList &list =
        (dir == In ? m_connectionsIn : m_connectionsOut);

    for (MappedObjectValueList::iterator i = list.begin(); i != list.end(); ++i) {
        if (*i == id) {
            return ;
        }
    }

    list.push_back(MappedObjectValue(id));
}

void
MappedConnectableObject::removeConnection(ConnectionDirection dir,
        MappedObjectId id)
{
    MappedObjectValueList &list =
        (dir == In ? m_connectionsIn : m_connectionsOut);

    for (MappedObjectValueList::iterator i = list.begin(); i != list.end(); ++i) {
        if (*i == id) {
            list.erase(i);
            return ;
        }
    }
}

MappedObjectValueList
MappedConnectableObject::getConnections(ConnectionDirection dir)
{
    if (dir == In)
        return m_connectionsIn;
    else
        return m_connectionsOut;
}


// ------------ MappedAudioFader ----------------
//
MappedAudioFader::MappedAudioFader(MappedObject *parent,
                                   MappedObjectId id,
                                   MappedObjectValue channels):
        MappedConnectableObject(parent,
                                "MappedAudioFader",
                                AudioFader,
                                id),
        m_level(0.0),  // dB
        m_recordLevel(0.0),
        m_instrumentId(0),
        m_pan(0),
        m_channels(channels),
        m_inputChannel(0)
{}

MappedAudioFader::~MappedAudioFader()
{}


MappedObjectPropertyList
MappedAudioFader::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        list.push_back(MappedAudioFader::FaderLevel);
        list.push_back(MappedAudioFader::FaderRecordLevel);
        list.push_back(MappedObject::Instrument);
        list.push_back(MappedAudioFader::Pan);
        list.push_back(MappedAudioFader::Channels);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    } else if (property == MappedObject::Instrument) {
        list.push_back(MappedObjectProperty("%1").arg(m_instrumentId));
    } else if (property == MappedAudioFader::FaderLevel) {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    } else if (property == MappedAudioFader::FaderRecordLevel) {
        list.push_back(MappedObjectProperty("%1").arg(m_recordLevel));
    } else if (property == MappedAudioFader::Channels) {
        list.push_back(MappedObjectProperty("%1").arg(m_channels));
    } else if (property == MappedAudioFader::InputChannel) {
        list.push_back(MappedObjectProperty("%1").arg(m_inputChannel));
    } else if (property == MappedAudioFader::Pan) {
        list.push_back(MappedObjectProperty("%1").arg(m_pan));
    } else if (property == MappedConnectableObject::ConnectionsIn) {
        MappedObjectValueList::const_iterator
        it = m_connectionsIn.begin();

        for ( ; it != m_connectionsIn.end(); ++it) {
            list.push_back(QString("%1").arg(*it));
        }
    } else if (property == MappedConnectableObject::ConnectionsOut) {
        MappedObjectValueList::const_iterator
        it = m_connectionsOut.begin();

        for ( ; it != m_connectionsOut.end(); ++it) {
            list.push_back(QString("%1").arg(*it));
        }
    }

    return list;
}

bool
MappedAudioFader::getProperty(const MappedObjectProperty &property,
                              MappedObjectValue &value)
{
    if (property == FaderLevel) {
        value = m_level;
    } else if (property == Instrument) {
        value = m_instrumentId;
    } else if (property == FaderRecordLevel) {
        value = m_recordLevel;
    } else if (property == Channels) {
        value = m_channels;
    } else if (property == InputChannel) {
        value = m_inputChannel;
    } else if (property == Pan) {
        value = m_pan;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioFader::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

void
MappedAudioFader::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    bool updateLevels = false;

    if (property == MappedAudioFader::FaderLevel) {
        m_level = value;
        updateLevels = true;
    } else if (property == MappedObject::Instrument) {
        m_instrumentId = InstrumentId(value);
        updateLevels = true;
    } else if (property == MappedAudioFader::FaderRecordLevel) {
        m_recordLevel = value;
    } else if (property == MappedAudioFader::Channels) {
        m_channels = value;
    } else if (property == MappedAudioFader::InputChannel) {
        m_inputChannel = value;
    } else if (property == MappedAudioFader::Pan) {
        m_pan = value;
        updateLevels = true;
    } else if (property == MappedConnectableObject::ConnectionsIn) {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    } else if (property == MappedConnectableObject::ConnectionsOut) {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioFader::setProperty - "
        << "unsupported property" << std::endl;
#endif

        return ;
    }

    /*
    std::cout << "MappedAudioFader::setProperty - "
              << property << " = " << value << std::endl;
              */

    if (updateLevels) {
        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            studio->getSoundDriver()->setAudioInstrumentLevels
            (m_instrumentId, m_level, m_pan);
        }
    }
}

// ---------------- MappedAudioBuss -------------------
//
//
MappedAudioBuss::MappedAudioBuss(MappedObject *parent,
                                 MappedObjectId id) :
        MappedConnectableObject(parent,
                                "MappedAudioBuss",
                                AudioBuss,
                                id),
        m_bussId(0),
        m_level(0),
        m_pan(0)
{}

MappedAudioBuss::~MappedAudioBuss()
{}

MappedObjectPropertyList
MappedAudioBuss::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        list.push_back(MappedAudioBuss::BussId);
        list.push_back(MappedAudioBuss::Level);
        list.push_back(MappedAudioBuss::Pan);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    } else if (property == BussId) {
        list.push_back(MappedObjectProperty("%1").arg(m_bussId));
    } else if (property == Level) {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    } else if (property == MappedConnectableObject::ConnectionsIn) {
        MappedObjectValueList::const_iterator
        it = m_connectionsIn.begin();

        for ( ; it != m_connectionsIn.end(); ++it) {
            list.push_back(QString("%1").arg(*it));
        }
    } else if (property == MappedConnectableObject::ConnectionsOut) {
        MappedObjectValueList::const_iterator
        it = m_connectionsOut.begin();

        for ( ; it != m_connectionsOut.end(); ++it) {
            list.push_back(QString("%1").arg(*it));
        }
    }

    return list;
}

bool
MappedAudioBuss::getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value)
{
    if (property == BussId) {
        value = m_bussId;
    } else if (property == Level) {
        value = m_level;
    } else if (property == Pan) {
        value = m_pan;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioBuss::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

void
MappedAudioBuss::setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value)
{
    bool updateLevels = false;

    if (property == MappedAudioBuss::BussId) {
        m_bussId = (int)value;
        updateLevels = true;
    } else if (property == MappedAudioBuss::Level) {
        m_level = value;
        updateLevels = true;
    } else if (property == MappedAudioBuss::Pan) {
        m_pan = value;
        updateLevels = true;
    } else if (property == MappedConnectableObject::ConnectionsIn) {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    } else if (property == MappedConnectableObject::ConnectionsOut) {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioBuss::setProperty - "
        << "unsupported property" << std::endl;
#endif

        return ;
    }

    if (updateLevels) {
        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            studio->getSoundDriver()->setAudioBussLevels
            (m_bussId, m_level, m_pan);
        }
    }
}

std::vector<InstrumentId>
MappedAudioBuss::getInstruments()
{
    std::vector<InstrumentId> rv;

    GET_LOCK;

    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio *>(studioObject))
        studioObject = studioObject->getParent();

    std::vector<MappedObject *> objects =
        static_cast<MappedStudio *>(studioObject)->
        getObjectsOfType(MappedObject::AudioFader);

    for (std::vector<MappedObject *>::iterator i = objects.begin();
            i != objects.end(); ++i) {
        MappedAudioFader *fader = dynamic_cast<MappedAudioFader *>(*i);
        if (fader) {
            MappedObjectValueList connections = fader->getConnections
                                                (MappedConnectableObject::Out);
            if (!connections.empty() && (*connections.begin() == getId())) {
                rv.push_back(fader->getInstrument());
            }
        }
    }

    RELEASE_LOCK;

    return rv;
}


// ---------------- MappedAudioInput -------------------
//
//
MappedAudioInput::MappedAudioInput(MappedObject *parent,
                                   MappedObjectId id) :
        MappedConnectableObject(parent,
                                "MappedAudioInput",
                                AudioInput,
                                id)
{}

MappedAudioInput::~MappedAudioInput()
{}

MappedObjectPropertyList
MappedAudioInput::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        list.push_back(MappedAudioInput::InputNumber);
    } else if (property == InputNumber) {
        list.push_back(MappedObjectProperty("%1").arg(m_inputNumber));
    }

    return list;
}

bool
MappedAudioInput::getProperty(const MappedObjectProperty &property,
                              MappedObjectValue &value)
{
    if (property == InputNumber) {
        value = m_inputNumber;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioInput::getProperty - "
        << "no properties available" << std::endl;
#endif

    }
    return false;
}

void
MappedAudioInput::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == InputNumber) {
        m_inputNumber = value;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioInput::setProperty - "
        << "no properties available" << std::endl;
#endif

    }
    return ;
}


MappedPluginSlot::MappedPluginSlot(MappedObject *parent, MappedObjectId id) :
        MappedObject(parent, "MappedPluginSlot", PluginSlot, id)
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cerr << "MappedPluginSlot::MappedPluginSlot: id = " << id << std::endl;
#endif
}

MappedPluginSlot::~MappedPluginSlot()
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cerr << "MappedPluginSlot::~MappedPluginSlot: id = " << getId() << ", identifier = " << m_identifier << std::endl;
#endif

    if (m_identifier != "") {

        // shut down and remove the plugin instance we have running

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            SoundDriver *drv = studio->getSoundDriver();

            if (drv) {
                drv->removePluginInstance(m_instrument, m_position);
            }
        }
    }
}

MappedObjectPropertyList
MappedPluginSlot::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        list.push_back(PortCount);
        list.push_back(Instrument);
        list.push_back(Bypassed);
        list.push_back(PluginName);
        list.push_back(Label);
        list.push_back(Author);
        list.push_back(Copyright);
        list.push_back(Category);
    } else if (property == Programs) {

        // The set of available programs is dynamic -- it can change
        // while a plugin is instantiated.  So we query it on demand
        // each time.

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            QStringList programs =
                studio->getSoundDriver()->getPluginInstancePrograms(m_instrument,
                        m_position);

            for (int i = 0; i < int(programs.count()); ++i) {
                list.push_back(programs[i]);
            }
        }

    } else {
        std::cerr << "MappedPluginSlot::getPropertyList: not a list property"
        << std::endl;
    }

    return list;
}

bool
MappedPluginSlot::getProperty(const MappedObjectProperty &property,
                              MappedObjectValue &value)
{
    if (property == PortCount) {
        value = m_portCount;
    } else if (property == Instrument) {
        value = m_instrument;
    } else if (property == Position) {
        value = m_position;
    } else if (property == Bypassed) {
        value = m_bypassed;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

bool
MappedPluginSlot::getStringProperty(const MappedObjectProperty &property,
                                    QString &value)
{
    if (property == Identifier) {
        value = m_identifier;
    } else if (property == PluginName) {
        value = m_name;
    } else if (property == Label) {
        value = m_label;
    } else if (property == Author) {
        value = m_author;
    } else if (property == Copyright) {
        value = m_copyright;
    } else if (property == Category) {
        value = m_category;
    } else if (property == Program) {

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            value = studio->getSoundDriver()->getPluginInstanceProgram(m_instrument,
                    m_position);
        }
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

QString
MappedPluginSlot::getProgram(int bank, int program)
{
    MappedStudio *studio =
        dynamic_cast<MappedStudio*>(getParent());

    if (studio) {
        return
            studio->getSoundDriver()->getPluginInstanceProgram(m_instrument,
                    m_position,
                    bank,
                    program);
    }

    return QString();
}

unsigned long
MappedPluginSlot::getProgram(QString name)
{
    MappedStudio *studio =
        dynamic_cast<MappedStudio*>(getParent());

    if (studio) {
        return
            studio->getSoundDriver()->getPluginInstanceProgram(m_instrument,
                    m_position,
                    name);
    }

    return 0;
}

void
MappedPluginSlot::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == Instrument) {
        m_instrument = InstrumentId(value);
    } else if (property == PortCount) {
        m_portCount = int(value);
    } else if (property == Position) {
        m_position = int(value);
    } else if (property == Bypassed) {
        m_bypassed = bool(value);

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            studio->getSoundDriver()->setPluginInstanceBypass(m_instrument,
                    m_position,
                    m_bypassed);
        }
    }
}

void
MappedPluginSlot::setStringProperty(const MappedObjectProperty &property,
                                    QString value)
{
    RG_DEBUG << "MappedPluginSlot::setStringProperty: "
             << property << " -> " << value << endl;

    if (property == Identifier) {

        if (m_identifier == value)
            return ;

        // shut down and remove the plugin instance we have running

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            SoundDriver *drv = studio->getSoundDriver();

            if (drv) {

                // We don't call drv->removePluginInstance at this
                // point: the sequencer will deal with that when we
                // call setPluginInstance below.  If we removed the
                // instance here, we might cause the library we want
                // for the new plugin instance to be unloaded and then
                // loaded again, which is hardly the most efficient.

                m_identifier = value;

                // populate myself and my ports
                PluginFactory *factory = PluginFactory::instanceFor(m_identifier);
                if (!factory) {
                    std::cerr << "WARNING: MappedPluginSlot::setProperty(identifier): No plugin factory for identifier " << m_identifier << "!" << std::endl;
                    m_identifier = "";
                    return ;
                }

                factory->populatePluginSlot(m_identifier, *this);

                // now create the new instance
                drv->setPluginInstance(m_instrument,
                                       m_identifier,
                                       m_position);
            }
        }

        m_configuration.clear();

    } else if (property == PluginName) {
        m_name = value;
    } else if (property == Label) {
        m_label = value;
    } else if (property == Author) {
        m_author = value;
    } else if (property == Copyright) {
        m_copyright = value;
    } else if (property == Category) {
        m_category = value;
    } else if (property == Program) {

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        if (studio) {
            studio->getSoundDriver()->setPluginInstanceProgram(m_instrument,
                    m_position,
                    value);
        }
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::setProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

    }
}

void
MappedPluginSlot::setPropertyList(const MappedObjectProperty &property,
                                  const MappedObjectPropertyList &values)
{
    if (property == Configuration) {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::setPropertyList(configuration): configuration is:" << std::endl;
#endif

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

        for (MappedObjectPropertyList::const_iterator i = values.begin();
                i != values.end(); ++i) {

            QString key = *i;
            QString value = *++i;

#ifdef DEBUG_MAPPEDSTUDIO
            std::cerr << key << " = " << value << std::endl;
#endif

            if (m_configuration.find(key) != m_configuration.end() &&
                m_configuration[key] == value) {
                continue;
            }

            if (studio) {
                QString rv =
                    studio->getSoundDriver()->configurePlugin(m_instrument,
                            m_position,
                            key, value);
                if (!rv.isEmpty()) {
                    throw(rv);
                }
            }
        }

        m_configuration.clear();

        for (MappedObjectPropertyList::const_iterator i = values.begin();
             i != values.end(); ++i) {

            QString key = *i;
            QString value = *++i;

            m_configuration[key] = value;
        }
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::setPropertyList - "
        << "not a list property" << std::endl;
#endif

    }
}

void
MappedPluginSlot::setPort(unsigned long portNumber, float value)
{
    std::vector<MappedObject*> ports = getChildObjects();
    std::vector<MappedObject*>::iterator it = ports.begin();
    MappedPluginPort *port = nullptr;

    for (; it != ports.end(); ++it) {
        port = dynamic_cast<MappedPluginPort *>(*it);
        if (port && (unsigned long)port->getPortNumber() == portNumber) {
            port->setValue(value);
        }
    }
}

float
MappedPluginSlot::getPort(unsigned long portNumber)
{
    std::vector<MappedObject*> ports = getChildObjects();
    std::vector<MappedObject*>::iterator it = ports.begin();
    MappedPluginPort *port = nullptr;

    for (; it != ports.end(); ++it) {
        port = dynamic_cast<MappedPluginPort *>(*it);
        if (port && (unsigned long)port->getPortNumber() == portNumber) {
            return port->getValue();
        }
    }

    return 0;
}


MappedPluginPort::MappedPluginPort(MappedObject *parent, MappedObjectId id) :
        MappedObject(parent, "MappedPluginPort", PluginPort, id)
{}

MappedPluginPort::~MappedPluginPort()
{}

MappedObjectPropertyList
MappedPluginPort::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
        list.push_back(PortNumber);
        list.push_back(Minimum);
        list.push_back(Maximum);
        list.push_back(Default);
        list.push_back(DisplayHint);
        list.push_back(Value);
        list.push_back(Name);
    } else {
        std::cerr << "MappedPluginSlot::getPropertyList: not a list property"
        << std::endl;
    }

    return list;
}

bool
MappedPluginPort::getProperty(const MappedObjectProperty &property,
                              MappedObjectValue &value)
{
    if (property == PortNumber) {
        value = m_portNumber;
    } else if (property == Minimum) {
        value = m_minimum;
    } else if (property == Maximum) {
        value = m_maximum;
    } else if (property == Default) {
        value = m_default;
    } else if (property == DisplayHint) {
        value = m_displayHint;
    } else if (property == Value) {
        return getValue();
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

bool
MappedPluginPort::getStringProperty(const MappedObjectProperty &property,
                                    QString &value)
{
    if (property == Name) {
        value = m_name;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::getProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

        return false;
    }
    return true;
}

void
MappedPluginPort::setValue(MappedObjectValue value)
{
    MappedPluginSlot *slot =
        dynamic_cast<MappedPluginSlot *>(getParent());

    if (slot) {

        MappedStudio *studio =
            dynamic_cast<MappedStudio *>(slot->getParent());

        if (studio) {
            SoundDriver *drv = studio->getSoundDriver();

            if (drv) {
                drv->setPluginInstancePortValue(slot->getInstrument(),
                                                slot->getPosition(),
                                                m_portNumber, value);
            }
        }
    }
}

float
MappedPluginPort::getValue() const
{
    const MappedPluginSlot *slot =
        dynamic_cast<const MappedPluginSlot *>(getParent());

    if (slot) {

        const MappedStudio *studio =
            dynamic_cast<const MappedStudio *>(slot->getParent());

        if (studio) {
            SoundDriver *drv =
                const_cast<SoundDriver *>(studio->getSoundDriver());

            if (drv) {
                return drv->getPluginInstancePortValue(slot->getInstrument(),
                                                       slot->getPosition(),
                                                       m_portNumber);
            }
        }
    }

    return 0;
}

void
MappedPluginPort::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == PortNumber) {
        m_portNumber = int(value);
    } else if (property == Minimum) {
        m_minimum = value;
    } else if (property == Maximum) {
        m_maximum = value;
    } else if (property == Default) {
        m_default = value;
    } else if (property == DisplayHint) {
        m_displayHint = PluginPort::PortDisplayHint(value);
    } else if (property == Value) {
        setValue(value);
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::setProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

    }
}


void
MappedPluginPort::setStringProperty(const MappedObjectProperty &property,
                                    QString value)
{
    if (property == Name) {
        m_name = value;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::setProperty - "
        << "unsupported or non-scalar property" << std::endl;
#endif

    }
}


}



