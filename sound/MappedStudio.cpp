// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2004
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

#include <iostream>
#include <dlfcn.h>

#include <qdir.h>

#include "MappedStudio.h"
#include "Sequencer.h"

#include <pthread.h> // for mutex

#define DEBUG_MAPPEDSTUDIO 1

// liblrdf - LADSPA naming library
//
#ifdef HAVE_LIBLRDF
#include "lrdf.h"
#endif // HAVE_LIBLRDF

static pthread_mutex_t _mappedObjectContainerLock =
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


// These four functions are stolen and adapted from Qt3 qvaluevector.h
//
// ** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
//
QDataStream& operator>>(QDataStream& s, Rosegarden::MappedObjectPropertyList& v)
{
    v.clear();
    Q_UINT32 c;
    s >> c;
    v.resize(c);
    for(Q_UINT32 i = 0; i < c; ++i)
    {
        Rosegarden::MappedObjectProperty t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const Rosegarden::MappedObjectPropertyList& v)
{
    s << (Q_UINT32)v.size();
    Rosegarden::MappedObjectPropertyList::const_iterator it = v.begin();
    for( ; it != v.end(); ++it )
        s << *it;
    return s;
}

QDataStream& operator>>(QDataStream& s, Rosegarden::MappedObjectValueList& v)
{
    v.clear();
    Q_UINT32 c;
    s >> c;
    v.resize(c);
    for(Q_UINT32 i = 0; i < c; ++i)
    {
        Rosegarden::MappedObjectValue t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const Rosegarden::MappedObjectValueList& v)
{
    s << (Q_UINT32)v.size();
    Rosegarden::MappedObjectValueList::const_iterator it = v.begin();
    for( ; it != v.end(); ++it )
        s << *it;
    return s;
}

namespace Rosegarden
{

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

const MappedObjectProperty MappedAudioPluginManager::Plugins = "plugins";
const MappedObjectProperty MappedAudioPluginManager::PluginIds = "pluginids";

#ifdef HAVE_LADSPA

const MappedObjectProperty MappedLADSPAPlugin::UniqueId = "uniqueId";
const MappedObjectProperty MappedLADSPAPlugin::PluginName = "pluginname";
const MappedObjectProperty MappedLADSPAPlugin::Label = "label";
const MappedObjectProperty MappedLADSPAPlugin::Author = "author";
const MappedObjectProperty MappedLADSPAPlugin::Copyright = "copyright";
const MappedObjectProperty MappedLADSPAPlugin::Category = "category";
const MappedObjectProperty MappedLADSPAPlugin::PortCount = "portcount";
const MappedObjectProperty MappedLADSPAPlugin::Ports = "ports";
const MappedObjectProperty MappedLADSPAPlugin::Bypassed = "bypassed";

const MappedObjectProperty MappedLADSPAPort::Descriptor = "descriptor";
const MappedObjectProperty MappedLADSPAPort::RangeHint = "rangehint";
const MappedObjectProperty MappedLADSPAPort::RangeLower = "rangelower";
const MappedObjectProperty MappedLADSPAPort::RangeUpper = "rangeupper";
const MappedObjectProperty MappedLADSPAPort::Value = "value";
const MappedObjectProperty MappedLADSPAPort::PortNumber = "portNumber";
const MappedObjectProperty MappedLADSPAPort::Default = "default";

#endif // HAVE_LADSPA

// --------- MappedObject ---------
//

void
MappedObject::addChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        if ((*it) == object)
            return;

    m_children.push_back(object);
}

void
MappedObject::removeChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
    {
        if ((*it) == object)
        {
            m_children.erase(it);
            return;
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
    for (; it != m_children.end(); it++)
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
    for (; it != m_children.end(); it++)
    {
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
    for (; it != children.end(); it++)
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
    for (; it != children.end(); it++) {
        (*it)->destroy();
    }

    (void)studio->clearObject(m_id);
    delete this;
}


void
MappedObject::clone(MappedObject *object)
{
    object->destroyChildren();

    // If we have children then create new versions and clone then
    //
    if (m_children.size())
    {
        MappedObject *studio = getParent();
        while (!dynamic_cast<MappedStudio*>(studio))
            studio = studio->getParent();

        std::vector<MappedObject*>::iterator it = m_children.begin();
        for (; it != m_children.end(); it++)
        {
            MappedObject *child =
                dynamic_cast<MappedStudio*>(studio)
                    ->createObject((*it)->getType(), false);
            object->addChild(child);
#ifdef DEBUG_MAPPEDSTUDIO
            std::cout << "MappedObject::clone - add child" << std::endl;
#endif
            (*it)->clone(child);
        }
    }
#ifdef DEBUG_MAPPEDSTUDIO
    else
        std::cerr << "MappedObject::clone - no children to clone" << std::endl;
#endif

}



// ------- MappedStudio -------
//

MappedStudio::MappedStudio():MappedObject(0,
                                          "MappedStudio",
                                          Studio,
                                          0,
                                          true,
                                          true),
                             m_runningObjectId(1)
{
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
MappedStudio::createObject(MappedObjectType type,
                           bool readOnly)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObject *mO = 0;

    // Ensure we've got an empty slot
    //
    while (getObjectById(m_runningObjectId))
        m_runningObjectId++;

    mO = createObject(type, m_runningObjectId, readOnly);

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type,
                           MappedObjectId id,
                           bool readOnly)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    // fail if the object already exists and it's not zero
    if (id != 0 && getObjectById(id)) {
	pthread_mutex_unlock(&_mappedObjectContainerLock);
	return 0;
    }

    MappedObject *mO = 0;

    if (type == MappedObject::AudioPluginManager)
    {
        mO = new MappedAudioPluginManager(this, id, readOnly);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(this,
                                  id,
                                  2, // channels
                                  readOnly);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioBuss)
    {
        mO = new MappedAudioBuss(this,
				 id,
				 readOnly);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioInput)
    {
        mO = new MappedAudioInput(this,
				 id,
				 readOnly);

        // push to the studio's child stack
        addChild(mO);
    }
    
#ifdef HAVE_LADSPA

    else if (type == MappedObject::LADSPAPlugin)
    {
        // create plugins under the pluginmanager if it exists
        ///
        MappedObject *mLP =
            getObjectOfType(MappedObject::AudioPluginManager);

        mO = new MappedLADSPAPlugin(mLP, id, readOnly);

        // push to the plugin manager's child stack
        mLP->addChild(mO);
    }
    else if (type == MappedObject::LADSPAPort)
    {
        // reset the parent after creation outside this method
        mO = new MappedLADSPAPort(this, id, readOnly);
    }

#endif // HAVE_LADSPA

    // Insert
    if (mO)
    {
//	std::cerr << "Adding object " << id << " to category " << type << std::endl;
        m_objects[type][id] = mO;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return mO;
}

MappedObject*
MappedStudio::getObjectOfType(MappedObjectType type)
{
    MappedObject *rv = 0;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];
    if (!category.empty()) rv = category.begin()->second;

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

std::vector<MappedObject *>
MappedStudio::getObjectsOfType(MappedObjectType type)
{
    std::vector<MappedObject *> rv;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	rv.push_back(i->second);
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

unsigned int
MappedStudio::getObjectCount(MappedObjectType type)
{
    unsigned int count = 0;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];
    count = category.size();

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return count;
}


bool
MappedStudio::destroyObject(MappedObjectId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObject *obj = getObjectById(id);

    bool rv = false;

    if (obj) {
        obj->destroy();
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::connectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
	obj1->addConnection(MappedConnectableObject::Out, mId2);
	obj2->addConnection(MappedConnectableObject::In,  mId1);
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::disconnectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
	obj1->removeConnection(MappedConnectableObject::Out, mId2);
	obj2->removeConnection(MappedConnectableObject::In,  mId1);
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::disconnectObject(MappedObjectId mId)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    MappedConnectableObject *obj =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId));
    
    if (obj) {
	while (1) {
	    MappedObjectValueList list = 
		obj->getConnections(MappedConnectableObject::In);
	    if (list.empty()) break;
	    MappedObjectId otherId = MappedObjectId(*list.begin());
	    disconnectObjects(otherId, mId);
	}
	while (1) {
	    MappedObjectValueList list = 
		obj->getConnections(MappedConnectableObject::Out);
	    if (list.empty()) break;
	    MappedObjectId otherId = MappedObjectId(*list.begin());
	    disconnectObjects(mId, otherId);
	}
    }
    
    rv = true;

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

    

// Clear down the whole studio
//
void
MappedStudio::clear()
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

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

    pthread_mutex_unlock(&_mappedObjectContainerLock);
}

bool
MappedStudio::clearObject(MappedObjectId id)
{
    bool rv = false;

    pthread_mutex_lock(&_mappedObjectContainerLock);

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

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
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

bool
MappedStudio::getProperty(const MappedObjectProperty &,
			  MappedObjectValue &)
{
    return false;
}

MappedObject*
MappedStudio::getObjectById(MappedObjectId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);
    MappedObject *rv = 0;

    for (MappedObjectMap::iterator i = m_objects.begin();
	 i != m_objects.end(); ++i) {

	MappedObjectCategory::iterator j = i->second.find(id);
	if (j != i->second.end()) {
	    rv = j->second;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedObject*
MappedStudio::getObjectByIdAndType(MappedObjectId id, MappedObjectType type)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);
    MappedObject *rv = 0;

    MappedObjectCategory &category = m_objects[type];
    MappedObjectCategory::iterator i = category.find(id);
    if (i != category.end()) {
	rv = i->second;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
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
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[object->getType()];

    bool next = false;
    MappedObject *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	if (i->second->getId() == object->getId()) next = true;
	else if (next) {
	    rv = i->second;
	    break;
	}
    }
    
    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

void
MappedStudio::setProperty(const MappedObjectProperty &property,
                          MappedObjectValue /*value*/)
{
    if (property == "")
    {
    }

}

// Remove all non read-only objects from the studio
//
void
MappedStudio::clearTemporaries()
{
    std::cerr << "MappedStudio::clearTemporaries" << std::endl;
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectId maxId = 1;

    for (MappedObjectMap::iterator i = m_objects.begin();
	 i != m_objects.end(); ++i) {

	// Temporaries are normally near the end
	
	bool done = false;

	while (!done) {

	    MappedObjectCategory::iterator j = i->second.end();
	    if (j == i->second.begin()) done = true;

	    while (j != i->second.begin()) {
		--j;
		if (j->second->isReadOnly()) {
		    if (j->second->getId() > maxId) maxId = j->second->getId();
		    if (j == i->second.begin()) done = true;
		} else {
		    // This calls back extensively on the studio, among other
		    // things invalidating j and possibly some other iterators
		    // on the same container -- which is why we start again
		    // with the loop as soon as we've done this.
		    j->second->destroy();
		    break;
		}
	    }
	}
    }

    m_runningObjectId = maxId + 1;
    pthread_mutex_unlock(&_mappedObjectContainerLock);
}

MappedAudioFader *
MappedStudio::getAudioFader(Rosegarden::InstrumentId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioFader];
    MappedAudioFader *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioFader *fader = dynamic_cast<MappedAudioFader *>(i->second);
	if (fader && (fader->getInstrument() == id)) {
	    rv = fader;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedAudioBuss *
MappedStudio::getAudioBuss(int bussNumber)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioBuss];
    MappedAudioBuss *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioBuss *buss = dynamic_cast<MappedAudioBuss *>(i->second);
	if (buss && (buss->getBussId() == bussNumber)) {
	    rv = buss;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedAudioInput *
MappedStudio::getAudioInput(int inputNumber)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioInput];
    MappedAudioInput *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioInput *input = dynamic_cast<MappedAudioInput *>(i->second);
	if (input && (input->getInputNumber() == inputNumber)) {
	    rv = input;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedObject*
MappedStudio::getPluginInstance(Rosegarden::InstrumentId id,
                                int position)
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));

    if (pm)
        return pm->getPluginInstance(id, position);

    return 0;
}

#ifdef HAVE_LADSPA

const LADSPA_Descriptor*
MappedStudio::createPluginDescriptor(unsigned long uniqueId)
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedStudio::createPluginInstance of id = "
              << uniqueId << std::endl;
#endif

    // Get the pluginmanager
    //
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedStudio::createPluginInstance - "
                  << "getting plugin descriptor" << std::endl;
#endif
        return pm->getPluginDescriptor(uniqueId);
    }
    else
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedStudio::createPluginInstance - "
                  << "plugin manager not found" << std::endl;
#endif
        return 0;
    }
}

void
MappedStudio::unloadPlugin(unsigned long uniqueId)
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
        pm->unloadPlugin(uniqueId);
}

void
MappedStudio::unloadAllPluginLibraries()
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
        pm->unloadAllPluginLibraries();
}


void
MappedStudio::setPluginInstancePort(InstrumentId id,
                                    int position,
                                    unsigned long portNumber,
                                    LADSPA_Data value)
{
    //std::cout << "PORT NUMBER = " << portNumber << std::endl;

    if (m_sequencer)
    {
        m_sequencer->
            setPluginInstancePortValue(id, position, portNumber, value);
    }
} 




#endif // HAVE_LADSPA

// -------------- MappedConnectableObject -----------------
//
//
MappedConnectableObject::MappedConnectableObject(MappedObject *parent,
                                     const std::string &name,
                                     MappedObjectType type,
                                     MappedObjectId id,
                                     bool readOnly):
    MappedObject(parent,
		 name,
		 type,
		 id,
		 readOnly)
{
}

MappedConnectableObject::~MappedConnectableObject()
{
}

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
	    return;
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
	    return;
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
                                   MappedObjectValue channels,
                                   bool readOnly):
    MappedConnectableObject(parent,
                      "MappedAudioFader",
                      AudioFader,
                      id,
                      readOnly),
    m_level(0.0), // dB
    m_recordLevel(0.0),
    m_instrumentId(0),
    m_pan(0),
    m_channels(channels),
    m_inputChannel(0)
{
}

MappedAudioFader::~MappedAudioFader()
{
}


MappedObjectPropertyList 
MappedAudioFader::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioFader::FaderLevel);
        list.push_back(MappedAudioFader::FaderRecordLevel);
        list.push_back(MappedObject::Instrument);
        list.push_back(MappedAudioFader::Pan);
        list.push_back(MappedAudioFader::Channels);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    }
    else if (property == MappedObject::Instrument)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_instrumentId));
    }
    else if (property == MappedAudioFader::FaderLevel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    }
    else if (property == MappedAudioFader::FaderRecordLevel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_recordLevel));
    }
    else if (property == MappedAudioFader::Channels)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_channels));
    }
    else if (property == MappedAudioFader::InputChannel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_inputChannel));
    }
    else if (property == MappedAudioFader::Pan)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_pan));
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsIn.begin();

        for( ; it != m_connectionsIn.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsOut.begin();

        for( ; it != m_connectionsOut.end(); ++it)
        {
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
    if (property == MappedAudioFader::FaderLevel)
    {
        m_level = value;
    }
    else if (property == MappedObject::Instrument)
    {
        m_instrumentId = Rosegarden::InstrumentId(value);
    }
    else if (property ==  MappedAudioFader::FaderRecordLevel)
    {
        m_recordLevel = value;
    }
    else if (property ==  MappedAudioFader::Channels)
    {
        m_channels = value;
    }
    else if (property ==  MappedAudioFader::InputChannel)
    {
        m_inputChannel = value;
    }
    else if (property ==  MappedAudioFader::Pan)
    {
        m_pan = value;
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    }
    else
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioFader::setProperty - "
                  << "unsupported property" << std::endl;
#endif
        return;
    }

    /*
    std::cout << "MappedAudioFader::setProperty - "
              << property << " = " << value << std::endl;
              */


}

// ---------------- MappedAudioBuss -------------------
//
//
MappedAudioBuss::MappedAudioBuss(MappedObject *parent,
                                 MappedObjectId id,
                                 bool readOnly):
    MappedConnectableObject(parent,
                      "MappedAudioBuss",
                      AudioBuss,
                      id,
                      readOnly),
    m_bussId(0),
    m_level(0),
    m_pan(0)
{
}

MappedAudioBuss::~MappedAudioBuss()
{
}

MappedObjectPropertyList
MappedAudioBuss::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioBuss::BussId);
        list.push_back(MappedAudioBuss::Level);
        list.push_back(MappedAudioBuss::Pan);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    }
    else if (property == BussId)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_bussId));
    }
    else if (property == Level)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsIn.begin();

        for( ; it != m_connectionsIn.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsOut.begin();

        for( ; it != m_connectionsOut.end(); ++it)
        {
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
    if (property == MappedAudioBuss::BussId)
    {
        m_bussId = value;
    }
    else if (property == MappedAudioBuss::Level)
    {
        m_level = value;
    }
    else if (property == MappedAudioBuss::Pan)
    {
        m_pan = value;
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    }
    else
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioBuss::setProperty - "
                  << "unsupported property" << std::endl;
#endif
        return;
    }
}

std::vector<InstrumentId>
MappedAudioBuss::getInstruments()
{
    std::vector<InstrumentId> rv;

    pthread_mutex_lock(&_mappedObjectContainerLock);

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
    
    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}
    

// ---------------- MappedAudioInput -------------------
//
//
MappedAudioInput::MappedAudioInput(MappedObject *parent,
				   MappedObjectId id,
				   bool readOnly):
    MappedConnectableObject(parent,
			    "MappedAudioInput",
			    AudioInput,
			    id,
			    readOnly)
{
}

MappedAudioInput::~MappedAudioInput()
{
}

MappedObjectPropertyList
MappedAudioInput::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioInput::InputNumber);
    }
    else if (property == InputNumber)
    {
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
    return;
}


// ----------------- MappedAudioPluginManager -----------------
//
MappedAudioPluginManager::MappedAudioPluginManager(
        MappedObject *parent,
        MappedObjectId id,
        bool readOnly)
    :MappedObject(parent,
                 "MappedAudioPluginManager",
                  AudioPluginManager,
                  id,
                  readOnly)
{
#ifdef HAVE_LADSPA
    getenvLADSPAPath();
#endif // HAVE_LADSPA
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
        list.push_back(MappedAudioPluginManager::PluginIds);
    }

#ifdef HAVE_LADSPA

    else if (property == MappedAudioPluginManager::PluginIds)
    {
        // get the list of plugin ids
        //
        MappedStudio *studio = dynamic_cast<MappedStudio*>(m_parent);

        if (studio)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (studio->getFirst(MappedObject::LADSPAPlugin));

            while (plugin)
            {
                list.push_back(MappedObjectProperty
                            ("%1").arg(plugin->getId()));
                plugin = dynamic_cast<MappedLADSPAPlugin*>
                            (studio->getNext(plugin));
            }
        }
    }
    else if (property == MappedAudioPluginManager::Plugins)
    {

        // get a full list of plugin descriptions
        //
        MappedStudio *studio = dynamic_cast<MappedStudio*>(m_parent);

        if (studio)
        {

            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (studio->getFirst(MappedObject::LADSPAPlugin));

            while (plugin)
            {
                list.push_back(MappedObjectProperty
                            ("%1").arg(plugin->getId()));

                list.push_back(MappedObjectProperty
                        (plugin->getPluginName().c_str()));

                list.push_back(MappedObjectProperty
                        ("%1").arg(plugin->getUniqueId()));

                list.push_back(MappedObjectProperty
                        (plugin->getLabel().c_str()));

                list.push_back(MappedObjectProperty
                        (plugin->getAuthor().c_str()));

                list.push_back(MappedObjectProperty
                        (plugin->getCopyright().c_str()));

                list.push_back(MappedObjectProperty
                        (plugin->getCategory().c_str()));

                list.push_back(MappedObjectProperty
                        ("%1").arg(plugin->getPortCount()));

                std::vector<MappedObject*> children =
                    plugin->getChildObjects();

                for (unsigned int i = 0; i < children.size(); i++)
                {
                    MappedLADSPAPort *port =
                        dynamic_cast<MappedLADSPAPort*>(children[i]);

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getId()));

                    list.push_back(MappedObjectProperty
                            (port->getPortName().c_str()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getDescriptor()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getDisplayHint()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getMinimum()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getMaximum()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getDefault()));
                }

                plugin = dynamic_cast<MappedLADSPAPlugin*>
                            (studio->getNext(plugin));
            }
        }
    }

#endif // HAVE_LADSPA

    return list;
}

bool
MappedAudioPluginManager::getProperty(const MappedObjectProperty &,
				      MappedObjectValue &)
{
    return false; // have no scalar properties
}


#ifdef HAVE_LIBLRDF
static bool useLRDF = false;
static std::map<unsigned long, std::string> lrdfTaxonomy;
static void
generateTaxonomy(std::string uri, std::string base)
{
    lrdf_uris *uris = lrdf_get_instances(uri.c_str());

    if (uris != NULL) {
	for (int i = 0; i < uris->count; ++i) {
	    lrdfTaxonomy[lrdf_get_uid(uris->items[i])] = base;
	}
	lrdf_free_uris(uris);
    }

    uris = lrdf_get_subclasses(uri.c_str());

    if (uris != NULL) {
	for (int i = 0; i < uris->count; ++i) {
	    char *label = lrdf_get_label(uris->items[i]);
	    generateTaxonomy(uris->items[i],
			     base + (base.length() > 0 ? " > " : "") + label);
	}
	lrdf_free_uris(uris);
    }
}
#endif

void
MappedAudioPluginManager::discoverPlugins(MappedStudio *studio)
{
    clearPlugins(studio);

#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedAudioPluginManager::discoverPlugins - "
	      << "discovering plugins; path is ";
    for (std::vector<std::string>::iterator i = m_path.begin();
	 i != m_path.end(); ++i) {
	std::cout << "[" << *i << "] ";
    }
    std::cout << std::endl;
#endif

#ifdef HAVE_LIBLRDF
    // Initialise liblrdf and read the description files 
    //
    lrdf_init();

    std::vector<QString> lrdfPaths;

    lrdfPaths.push_back("/usr/local/share/ladspa/rdf");
    lrdfPaths.push_back("/usr/share/ladspa/rdf");

    for (std::vector<std::string>::iterator i = m_path.begin();
	 i != m_path.end(); ++i) {
	lrdfPaths.push_back(QString(i->c_str()) + "/rdf");
    }

    bool haveSomething = false;

    for (size_t i = 0; i < lrdfPaths.size(); ++i) {
	QDir dir(lrdfPaths[i], "*.rdf;*.rdfs");
	for (unsigned int j = 0; j < dir.count(); ++j) {
	    if (!lrdf_read_file(QString("file:" + lrdfPaths[i] + "/" + dir[j]).data())) {
#ifdef DEBUG_MAPPEDSTUDIO
		std::cerr << "MappedAudioPluginManager: read RDF file " << (lrdfPaths[i] + "/" + dir[j]) << std::endl;
#endif
		haveSomething = true;
	    }
	}
    }
    if (!haveSomething) {
#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedAudioPluginManager::discoverPlugins - "
	          << "can't find plugin description files" << std::endl;
#endif
	useLRDF = false;
    } else {
	useLRDF = true;
	generateTaxonomy(LADSPA_BASE "Plugin", "");
    }
#endif // HAVE_LIBLRDF

    for (std::vector<std::string>::iterator i = m_path.begin();
	 i != m_path.end(); ++i) {

	QDir pluginDir(QString(i->c_str()), "*.so");

	for (unsigned int j = 0; j < pluginDir.count(); ++j) {
	    enumeratePlugin
		(studio,
		 *i + std::string("/") + std::string(pluginDir[j].data()));
	}
    }

#ifdef HAVE_LIBLRDF
    // Cleanup after the RDF library
    //
    lrdf_cleanup();
#endif // HAVE_LIBLRDF

    
}


// It is only later, after they've gone,
// I realize they have delivered a letter.
// It's a letter from my wife.  "What are you doing
// there?" my wife asks.  "Are you drinking?"
// I study the postmark for hours.  Then it, too, begins to fade.
// I hope someday to forget all this.
//
void
MappedAudioPluginManager::clearPlugins(MappedStudio *studio)
{
    MappedObject *object;
    while ((object = studio->getObjectOfType(MappedObject::LADSPAPlugin)))
    {
        studio->destroyObject(object->getId());
    }
}


#ifdef HAVE_LADSPA

void
MappedAudioPluginManager::getenvLADSPAPath()
{
    std::string path;

    char *cpath = getenv("LADSPA_PATH");
    if (cpath) path = cpath;

    if (path == "") {
	char *home = getenv("HOME");
	if (home) {
	    path = std::string(home) +
		"/.ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa";
	} else {
	    path = "/usr/local/lib/ladspa:/usr/lib/ladspa";
	}
    }

    std::string::size_type index = 0, newindex = 0;

    while ((newindex = path.find(':', index)) >= 0 && newindex < path.size()) {
	m_path.push_back(path.substr(index, newindex - index));
	index = newindex + 1;
    }
    
    m_path.push_back(path.substr(index));
}


const LADSPA_Descriptor*
MappedAudioPluginManager::getDescriptorFromHandle(unsigned long uniqueId,
                                                  void *pluginHandle)
{
    LADSPA_Descriptor_Function descrFn = 0;
    descrFn = (LADSPA_Descriptor_Function)
                    dlsym(pluginHandle, "ladspa_descriptor");
    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;

        int index = 0;

        do
        {
            descriptor = descrFn(index);
                                                                                            if (descriptor)
            {
                if (descriptor->UniqueID == uniqueId)
                    return descriptor;
            }

            index++;

        } while (descriptor);
    }

    return 0;
}


// The LADSPAPluginHandles vector holds all the libraries we've loaded
// and their handles for re-use by plugins from the same library.  When
// we've got no plugins left in use from a library we can automatically
// unload it.
//
const LADSPA_Descriptor*
MappedAudioPluginManager::getPluginDescriptor(unsigned long uniqueId)
{
    // Find the plugin
    //
    MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(getReadOnlyPlugin(uniqueId));

    if (plugin == 0)
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioPluginManager::getPluginDescriptor - "
                  << "can't find read-only plugin" << std::endl;
#endif
        return 0;
    }

    // Check if we have the handle
    //
    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
    {
        if (it->first == plugin->getLibraryName())
            return getDescriptorFromHandle(uniqueId, it->second);
    }
    

    // Now create the handle and store it
    //
    void *pluginHandle = dlopen(plugin->getLibraryName().c_str(), RTLD_LAZY);

    std::pair<std::string, void*> pluginHandlePair(plugin->getLibraryName(),
                                                   pluginHandle);
    m_pluginHandles.push_back(pluginHandlePair);

    // now generate the descriptor
    return getDescriptorFromHandle(uniqueId, pluginHandle);
}


// Potentially unload the library if all plugins within it are
// not being used.
//
void
MappedAudioPluginManager::unloadPlugin(unsigned long uniqueId)
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedAudioPluginManager::unloadPlugin - "
              << "unloading plugin " << uniqueId << std::endl;
#endif

    pthread_mutex_lock(&_mappedObjectContainerLock);

    // Find the plugin
    //
    MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(getReadOnlyPlugin(uniqueId));

    if (plugin == 0)
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioPluginManager::unloadPlugin - "
                  << "can't find plugin to unload" << std::endl;
#endif
	pthread_mutex_unlock(&_mappedObjectContainerLock);
        return;
    }

    void *pluginHandle = 0;

    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
    {
        if (it->first == plugin->getLibraryName())
        {
            // library is loaded (as it should be)
            pluginHandle = it->second;
            break;
        }
    }
    
    if (pluginHandle == 0)
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedAudioPluginManager::unloadPlugin - "
                  << "can't find plugin library to unload" << std::endl;
#endif
	pthread_mutex_unlock(&_mappedObjectContainerLock);
        return;
    }

    // check if any of these plugins are still loaded
    //
    std::vector<unsigned long> list = getPluginsInLibrary(pluginHandle);
    std::vector<unsigned long>::iterator pIt = list.begin();

    std::cout << list.size() << " plugins in library" << std::endl;

    for (; pIt != list.end(); pIt++)
    {
	std::cout << "inspecting one of 'em" << std::endl;

        if (getPluginInstance(*pIt, false)) {
	    std::cout << "still in use, returning" << std::endl;

	    pthread_mutex_unlock(&_mappedObjectContainerLock);
            return;
	}
    }

#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedAudioPluginManager::unloadPlugin - "
              << "unloading library \"" 
              << plugin->getLibraryName() << "\"" << std::endl;
#endif

    dlclose(pluginHandle);
    m_pluginHandles.erase(it);
    pthread_mutex_unlock(&_mappedObjectContainerLock);
}

void
MappedAudioPluginManager::unloadAllPluginLibraries()
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedAudioPluginManager::unloadAllPluginLibraries - "
              << "unloading " << m_pluginHandles.size() << " libraries"
              << std::endl;
#endif

    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
        dlclose(it->second);

    m_pluginHandles.erase(m_pluginHandles.begin(), m_pluginHandles.end());
}

std::vector<unsigned long>
MappedAudioPluginManager::getPluginsInLibrary(void *pluginHandle)
{
    std::vector<unsigned long> list;
    LADSPA_Descriptor_Function descrFn = 0;
    descrFn = (LADSPA_Descriptor_Function)
                    dlsym(pluginHandle, "ladspa_descriptor");
    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;
        int index = 0;

        do
        {
            descriptor = descrFn(index);

            if (descriptor)
            {
                list.push_back(descriptor->UniqueID);
            }
            index++;
        } while(descriptor);
    }

    return list;
}

#endif // HAVE_LADSPA


void
MappedAudioPluginManager::enumeratePlugin(MappedStudio *studio,
                                          const std::string& path)
{
#ifdef HAVE_LADSPA
    LADSPA_Descriptor_Function descrFn = 0;
    void *pluginHandle = 0;

    pluginHandle = dlopen(path.c_str(), RTLD_LAZY);

//    std::cout << "Opening " << path << std::endl;

    if (!pluginHandle) {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedAudioPluginManager::enumeratePlugin : couldn't dlopen "
                  << path << " - " << dlerror() << std::endl;
#endif
        return;
    }

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
		std::cout << "Found plugin; label is " << descriptor->Label
			  << "; " << (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties) ? "is" : "is not") << " hard RT capable" << std::endl;

		std::string category = "";
		
#ifdef HAVE_LIBLRDF
		char *def_uri = 0;
		lrdf_defaults *defs = 0;
		
		if (useLRDF) {
		    category = lrdfTaxonomy[descriptor->UniqueID];
#ifdef DEBUG_MAPPEDSTUDIO
		    std::cout << "Plugin id is " << descriptor->UniqueID
			      << ", category is \""
			      << lrdfTaxonomy[descriptor->UniqueID]
			      << "\", name is " << descriptor->Name
			      << ", label is " << descriptor->Label
			      << std::endl;
#endif
		    def_uri = lrdf_get_default_uri(descriptor->UniqueID);
		    if (def_uri) {
			defs = lrdf_get_setting_values(def_uri);
			std::cout << "Have " << defs->count << " default settings" << std::endl;
		    }
		}
#endif // HAVE_LIBLRDF
		
		if (category == "" && descriptor->Name != 0) {
		    std::string name = descriptor->Name;
		    if (name.length() > 4 &&
			name.substr(name.length() - 4) == " VST")
			category = "VSTs";
		}
		
		MappedLADSPAPlugin *plugin =
		    dynamic_cast<MappedLADSPAPlugin*>
		    (studio->createObject
		     (MappedObject::LADSPAPlugin, true)); // RO
		
		plugin->setLibraryName(path);
		plugin->populate(descriptor, category);

		int controlPortNumber = 1;
		
		for (unsigned long i = 0; i < descriptor->PortCount; i++)
		{
		    MappedLADSPAPort *port = 
			dynamic_cast<MappedLADSPAPort*>
			(studio->createObject
			 (MappedObject::LADSPAPort, true)); // read-only
		    
		    // tie up relationships
		    plugin->addChild(port);
		    port->setParent(plugin);
		    
		    port->setPortName(descriptor->PortNames[i]);
		    port->setDescriptor(descriptor->PortDescriptors[i]);
		    
		    port->setRangeHint(
			descriptor->PortRangeHints[i].HintDescriptor,
			descriptor->PortRangeHints[i].LowerBound,
			descriptor->PortRangeHints[i].UpperBound);
		    
		    port->setPortNumber(i);

		    if (LADSPA_IS_PORT_CONTROL(port->getDescriptor())) {

			// apply default

#ifdef HAVE_LIBLRDF
			if (def_uri)
			{
			    for (int j = 0; j < defs->count; j++)
			    {
				if (defs->items[j].pid == controlPortNumber)
				{
				    std::cout << "Default for this port (" << defs->items[j].pid << ", " << defs->items[j].label << ") is " << defs->items[j].value << "; applying this to port number " << i << " with name " << port->getPortName() << std::endl;
				    port->setDefault(defs->items[j].value);
				}
			    }
			}
#endif // HAVE_LIBLRDF

			++controlPortNumber;
		    }
		}
#ifdef HAVE_LIBLRDF
		if (defs) {
		    lrdf_free_setting_values(defs);
		    defs = 0;
		}
#endif // HAVE_LIBLRDF

                index++;
            }
        }
        while(descriptor);

    } else {
	std::cerr << "PluginManager::loadPlugin: " << path
		  << " is not a LADSPA plugin object" << std::endl;
    }

    if(dlclose(pluginHandle) != 0)
    {
#ifdef DEBUG_MAPPEDSTUDIO

        std::cerr << "PluginManager::loadPlugin - can't unload plugin"
                  << std::endl;
#endif
        return;
    }

#endif // HAVE_LADSPA

}

void
MappedAudioPluginManager::setProperty(const MappedObjectProperty &property,
                                      MappedObjectValue /*value*/)
{
    if (property == "")
    {
    }
}

MappedObject*
MappedAudioPluginManager::getReadOnlyPlugin(unsigned long uniqueId)
{
    return getPluginInstance(uniqueId, true);
}

MappedObject*
MappedAudioPluginManager::getPluginInstance(unsigned long uniqueId,
                                            bool readOnly)
{
#ifdef HAVE_LADSPA
    pthread_mutex_lock(&_mappedObjectContainerLock);

    std::vector<MappedObject*>::iterator it = m_children.begin();

    for(; it != m_children.end(); it++)
    {
        if ((*it)->getType() == LADSPAPlugin &&
            (*it)->isReadOnly() == readOnly)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(*it);

            if (plugin->getUniqueId() == uniqueId) {
		pthread_mutex_unlock(&_mappedObjectContainerLock);
                return *it;
	    }
        }
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

#endif // HAVE_LADSPA

    return 0;
}

MappedObject*
MappedAudioPluginManager::getPluginInstance(InstrumentId instrument,
                                            int position)
{
#ifdef HAVE_LADSPA
    std::vector<MappedObject*>::iterator it = m_children.begin();

    for(; it != m_children.end(); it++)
    {
        if ((*it)->getType() == LADSPAPlugin)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(*it);

            if(plugin->getInstrument() == instrument &&
               plugin->getPosition() == position)
                return *it;
        }
    }

#endif // HAVE_LADSPA

    return 0;
}





//  ------------------ MappedLADSPAPlugin ------------------
//
//

#ifdef HAVE_LADSPA

MappedLADSPAPlugin::~MappedLADSPAPlugin()
{
//    std::cout << "MappedLADSPAPlugin::~MappedLADSPAPlugin (" << this << ")" << std::endl;

    /*
    MappedStudio *studio =
        dynamic_cast<MappedStudio*>(getParent()->getParent());

    if (studio)
    {
        Rosegarden::Sequencer *seq = studio->getSequencer();
        cout << "HERE" << endl;
        if (seq)
            seq->removePluginInstance(m_instrument, m_position);
    }
    */
}


void
MappedLADSPAPlugin::populate(const LADSPA_Descriptor *descriptor,
			     std::string category)
{
    if (descriptor)
    {
        m_uniqueId = descriptor->UniqueID;
        m_label = descriptor->Label;
        m_author = descriptor->Maker;
        m_copyright = descriptor->Copyright;
        m_portCount = descriptor->PortCount;
        m_pluginName = descriptor->Name;
    }
   
    m_category = category;

}


MappedObjectPropertyList
MappedLADSPAPlugin::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;
    
    if (property == "")
    {
        // our LADSPA properties
        list.push_back(UniqueId);
        list.push_back(PluginName);
        list.push_back(Label);
        list.push_back(Author);
        list.push_back(Copyright);
        list.push_back(Category);
        list.push_back(PortCount);
        list.push_back(Bypassed);
    }
    else
    {
        if (property == MappedLADSPAPlugin::UniqueId)
            list.push_back(MappedObjectProperty("%1").arg(m_uniqueId));
        else if (property == MappedLADSPAPlugin::Label)
            list.push_back(MappedObjectProperty(m_label.c_str()));
        else if (property == MappedLADSPAPlugin::PluginName)
            list.push_back(MappedObjectProperty(m_pluginName.c_str()));
        else if (property == MappedLADSPAPlugin::Author)
            list.push_back(MappedObjectProperty(m_author.c_str()));
        else if (property == MappedLADSPAPlugin::Copyright)
            list.push_back(MappedObjectProperty(m_copyright.c_str()));
        else if (property == MappedLADSPAPlugin::Category)
            list.push_back(MappedObjectProperty(m_category.c_str()));
        else if (property == MappedLADSPAPlugin::PortCount)
            list.push_back(MappedObjectProperty("%1").arg(m_portCount));
        else if (property == MappedObject::Instrument)
            list.push_back(MappedObjectProperty("%1").arg(m_instrument));
        else if (property == MappedObject::Position)
            list.push_back(MappedObjectProperty("%1").arg(m_position));
        else if (property == MappedLADSPAPlugin::Bypassed)
            list.push_back(MappedObjectProperty("%1").arg(m_bypassed));
        else if (property == MappedLADSPAPlugin::Ports)
        {
            // list the port object ids
            /*
            MappedObjectPropertyList list = 
                getChildren(MappedObject::LADSPAPort);
            return list;
            cout << "GOT " << list.size() << " CHILDREN" << endl;
            */

            return getChildren(MappedObject::LADSPAPort);

        }
    }

    return list;
}


bool
MappedLADSPAPlugin::getProperty(const MappedObjectProperty &property,
				MappedObjectValue &value)
{
    if (property == UniqueId) {
	value = m_uniqueId;
    } else if (property == PortCount) {
	value = m_portCount;
    } else if (property == Instrument) {
	value = m_instrument;
    } else if (property == Position) {
	value = m_position;
    } else if (property == Bypassed) {
	value = m_bypassed;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedLADSPAPlugin::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}


void
MappedLADSPAPlugin::setProperty(const MappedObjectProperty &property,
                                MappedObjectValue value)
{

    if (property == MappedLADSPAPlugin::UniqueId)
    {
        // we've already got a plugin of this type
        //
        if (m_uniqueId == ((unsigned long)value))
            return;

        m_uniqueId = (unsigned long)value;

        // Get the studio and the sequencer
        //
        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent()->getParent());
        Rosegarden::Sequencer *seq = studio->getSequencer();

        // shut down and remove the plugin instance we have running
        seq->removePluginInstance(m_instrument, m_position);

        // manufacture the ports for this plugin type
        MappedAudioPluginManager *pluginManager =
            dynamic_cast<MappedAudioPluginManager*>(getParent());

        MappedLADSPAPlugin *roPlugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (pluginManager->getReadOnlyPlugin(((unsigned long)value)));

        if (roPlugin == 0)
        {
#ifdef DEBUG_MAPPEDSTUDIO
            std::cerr << "MappedLADSPAPlugin::setProperty - "
                      << "can't get read-only copy to clone" << std::endl;
#endif
            return;
        }

        // clone the ports
        //
        roPlugin->clone(this);

        // now create the new instance
        seq->setPluginInstance(m_instrument,
                               ((unsigned long)(value)),
                               m_position);

    }
    else if (property == MappedLADSPAPlugin::Instrument)
    {
        m_instrument = Rosegarden::InstrumentId(value);
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedLADSPAPlugin::setProperty - "
                  << "setting instrument id to " << m_instrument << std::endl;
#endif
    }
    else if (property == Rosegarden::MappedObject::Position)
    {
        m_position = int(value);
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedLADSPAPlugin::setProperty - "
                  << "setting position to " << m_position << std::endl;
#endif

    }
    else if (property == Rosegarden::MappedLADSPAPlugin::Bypassed)
    {
        m_bypassed = bool(value);
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedLADSPAPlugin::setProperty - "
                  << "setting bypassed to " << m_bypassed << std::endl;
#endif

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent()->getParent());

        studio->getSequencer()->setPluginInstanceBypass(m_instrument,
                                                        m_position,
                                                        m_bypassed);
    }
}

void
MappedLADSPAPlugin::clone(MappedObject *object)
{
    object->destroyChildren();

    // If we have children then create new versions and clone then
    //
    if (m_children.size())
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cout << "MappedLADSPAPlugin::clone - cloning "
                  << m_children.size() << " children" << std::endl;
#endif

        MappedObject *studio = getParent();
        while(!(dynamic_cast<MappedStudio*>(studio)))
            studio = studio->getParent();

        std::vector<MappedObject*>::iterator it = m_children.begin();
        for (; it != m_children.end(); it++)
        {
            MappedObject *child =
                dynamic_cast<MappedStudio*>(studio)
                    ->createObject(LADSPAPort, false);
            object->addChild(child);
            child->setParent(object);

            (*it)->clone(child);
        }
    }
#ifdef DEBUG_MAPPEDSTUDIO
    else
        std::cerr << "MappedLADSPAPlugin::clone - " 
                  << "no children to clone" << std::endl;
#endif
}

void
MappedLADSPAPlugin::setPort(unsigned long portNumber, float value)
{
    std::vector<MappedObject*> ports = getChildObjects();
    std::vector<MappedObject*>::iterator it = ports.begin();
    MappedLADSPAPort *port = 0;

    for (; it != ports.end(); it++)
    {
        port = dynamic_cast<MappedLADSPAPort*>(*it);

        if (port && port->getPortNumber() == portNumber)
            port->setProperty(MappedLADSPAPort::Value, value);
    }
}


#endif // HAVE_LADSPA


// ------------------ MappedLADSPAPort --------------------
//
//

#ifdef HAVE_LADSPA

MappedLADSPAPort::MappedLADSPAPort(MappedObject *parent,
                                   MappedObjectId id,
                                   bool readOnly):
    MappedObject(parent, "MappedAudioPluginPort", LADSPAPort, id, readOnly),
    m_value(0.0),
    m_default(0.0),
    m_portNumber(0)
{
}

MappedObjectPropertyList
MappedLADSPAPort::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back("name");
        list.push_back("descriptor");
        list.push_back("rangehint");
        list.push_back("rangelower");
        list.push_back("rangeupper");
	list.push_back("default");
    }
    else if (property == MappedLADSPAPort::Default)
    {
	list.push_back(QString("%1").arg(m_default));
    }
    else 
    {
        if (property == MappedLADSPAPort::Value)
        {
#ifdef DEBUG_MAPPEDSTUDIO
            std::cout << "MappedLADSPAPort::MappedLADSPAPort - "
                      << "value = " << m_value << std::endl;
#endif
            list.push_back(MappedObjectProperty("%1").arg(m_value));
        }
    }


    return list;
}

bool
MappedLADSPAPort::getProperty(const MappedObjectProperty &property,
			      MappedObjectValue &value)
{
    if (property == Default) {
	value = getDefault();
    } else if (property == Value) {
	value = m_value;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedLADSPAPort::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

void
MappedLADSPAPort::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == MappedLADSPAPort::Value)
    {
        /*
        std::cout << "MappedLADSPAPort::setProperty value = "
                  << value << std::endl;
                  */

        m_value = value;

        // Gather information and tell the plugin instance that we've
        // changed
        MappedObject *studio = getParent();
        while(!(dynamic_cast<MappedStudio*>(studio)))
            studio = studio->getParent();

        MappedStudio *studioObject = dynamic_cast<MappedStudio*>(studio);
        MappedLADSPAPlugin *plugin =
            dynamic_cast<MappedLADSPAPlugin*>(getParent());

        studioObject->setPluginInstancePort(plugin->getInstrument(),
                                            plugin->getPosition(),
                                            m_portNumber,
                                            value);
    }
    else if (property == MappedLADSPAPort::PortNumber)
    {
        m_portNumber =  ((unsigned long)value);
    }
    else if (property == MappedLADSPAPort::Default)
    {
	m_default = ((MappedObjectValue)value);
    }

}

void
MappedLADSPAPort::clone(MappedObject *object)
{
    // Set the port number first - see the above method for an
    // explanation of why..
    //
    object->setProperty(MappedLADSPAPort::PortNumber, m_portNumber);
    object->setProperty(MappedLADSPAPort::Value, m_value);
}

unsigned int
MappedLADSPAPort::getSampleRate() const
{
    const MappedObject *parent = getParent();
    const MappedStudio *studio = 0;

    while (parent && !(studio = dynamic_cast<const MappedStudio *>(parent)))
	parent = parent->getParent();

    // The normal case is for the studio and its sequencer to exist;
    // these 48K returns are just to be on the safe side

    if (!studio) return 48000;

    const Sequencer *seq = studio->getSequencer();
    if (!seq) return 48000;

    return seq->getSampleRate();
}

MappedObjectValue
MappedLADSPAPort::getMinimum() const
{
    LADSPA_PortRangeHintDescriptor d = m_portRangeHint.HintDescriptor;

    MappedObjectValue val = 0.0;
    
    if (LADSPA_IS_HINT_BOUNDED_BELOW(d)) {

	val = m_portRangeHint.LowerBound;

    } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
	if (m_haveDefault) {
	    val = std::min(0.0, std::min(m_portRangeHint.UpperBound,
					 m_default) - 1.0);
	} else {
	    val = std::min(0.0, m_portRangeHint.UpperBound - 1.0);
	}
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	val *= getSampleRate();
    }

    return val;
}

MappedObjectValue
MappedLADSPAPort::getMaximum() const
{
    LADSPA_PortRangeHintDescriptor d = m_portRangeHint.HintDescriptor;

    MappedObjectValue val = 0.0;
    
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
	val = m_portRangeHint.UpperBound;
    } else {
	val = getMinimum() + 1.0;
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	val *= getSampleRate();
    }

    return val;
}

MappedObjectValue
MappedLADSPAPort::getDefault() const
{
    if (m_haveDefault) {
	return m_default;
    }

    LADSPA_PortRangeHintDescriptor d = m_portRangeHint.HintDescriptor;
    LADSPA_Data min = getMinimum(), max = getMaximum();
    MappedObjectValue val = 0.0;

#ifdef DEBUG_MAPPEDSTUDIO
    std::cerr << "MappedLADSPAPort::getDefault: min is " << min
	      << ", max is " << max << std::endl;
#endif

    bool logarithmic = LADSPA_IS_HINT_LOGARITHMIC(d);
    
    if (!LADSPA_IS_HINT_HAS_DEFAULT(d)) {
#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: no default" << std::endl;
#endif
	val = min;

    } else if (LADSPA_IS_HINT_DEFAULT_MINIMUM(d)) {

#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: min default" << std::endl;
#endif
	val = min;

    } else if (LADSPA_IS_HINT_DEFAULT_LOW(d)) {

#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: low default" << std::endl;
#endif
	if (logarithmic) {
	    val = powf(10, log10(min) * 0.75 + log10(max) * 0.25);
	} else {
	    val = min * 0.75 + max * 0.25;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(d)) {

#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: middle default" << std::endl;
#endif
	if (logarithmic) {
	    val = powf(10, log10(min) * 0.5 + log10(max) * 0.5);
	} else {
	    val = min * 0.5 + max * 0.5;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(d)) {

#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: high default" << std::endl;
#endif
	if (logarithmic) {
	    val = powf(10, log10(min) * 0.25 + log10(max) * 0.75);
	} else {
	    val = min * 0.25 + max * 0.75;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(d)) {

#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedLADSPAPort::getDefault: max default" << std::endl;
#endif
	val = max;

    } else if (LADSPA_IS_HINT_DEFAULT_0(d)) {

	val = 0.0;

    } else if (LADSPA_IS_HINT_DEFAULT_1(d)) {

	val = 1.0;

    } else if (LADSPA_IS_HINT_DEFAULT_100(d)) {

	val = 100.0;

    } else if (LADSPA_IS_HINT_DEFAULT_440(d)) {

	val = 440.0;

    } else {

	val = min;
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	val *= getSampleRate();
    }

    return val;
}

PluginPort::PortDisplayHint
MappedLADSPAPort::getDisplayHint() const
{
    LADSPA_PortRangeHintDescriptor d = m_portRangeHint.HintDescriptor;
    int hint = PluginPort::NoHint;

    if (LADSPA_IS_HINT_TOGGLED(d)) hint |= PluginPort::Toggled;
    if (LADSPA_IS_HINT_INTEGER(d)) hint |= PluginPort::Integer;
    if (LADSPA_IS_HINT_LOGARITHMIC(d)) hint |= PluginPort::Logarithmic;

    return (PluginPort::PortDisplayHint)hint;
}
    

#endif // HAVE_LADSPA



}


