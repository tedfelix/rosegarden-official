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

#include <string> 

#include <qdatastream.h>

#include "MappedCommon.h"
#include "Instrument.h"
#include "Device.h"
#include "Plugins.h"

#ifndef _MAPPEDSTUDIO_H_
#define _MAPPEDSTUDIO_H_


// A sequencer-side representation of certain elements in the
// gui that enables us to control outgoing or incoming audio
// and MIDI with run-time only persistence.  Placeholders for
// our Studio elements on the sequencer.
//
//

namespace Rosegarden
{

//
// Types are in MappedCommon.h
//

// Every MappedStudio object derives from this class - if an
// object is static then you're only allowed one per Studio
// (as in the case of a PluginManager).
//
class MappedObject
{
public:

    // Some common properties
    //
    static const MappedObjectProperty FaderLevel;

    // The object we can create
    //
    typedef enum
    {
        Studio,
        AudioFader,
        AudioPluginManager,
        LADSPAPlugin,
        LADSPAPort
   
    } MappedObjectType;

    MappedObject(MappedObject *parent,
                 const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id):
        m_type(type),
        m_id(id), 
        m_static(false), 
        m_name(name),
        m_parent(parent) {;}

    MappedObject(MappedObject *parent, 
                 const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id,
                 bool s):
        m_type(type),
        m_id(id),
        m_static(s),
        m_name(name),
        m_parent(parent) {;}

    virtual ~MappedObject() {;}

    MappedObjectId getId() { return m_id; }
    MappedObjectType getType() { return m_type; }

    std::string getName() { return m_name; }
    void setName(const std::string &name) { m_name= name; }

    virtual MappedObjectPropertyList
        getPropertyList(const MappedObjectProperty &property) = 0;

    // Ownership
    //
    MappedObject* getParent() { return m_parent; }
    void setParent(MappedObject *parent) { m_parent = parent; }

protected:

    MappedObjectType m_type;
    MappedObjectId   m_id;
    bool             m_static;
    std::string      m_name;

    MappedObject    *m_parent;
};


// Works as a factory and virtual plug-board for all our other
// objects whether they be MIDI or audio.
//
//
//
class MappedStudio : public MappedObject
{
public:
    MappedStudio();
    ~MappedStudio();

    // Create a new slider of a certain type for a certain
    // type of device.
    //
    MappedObject* createObject(MappedObjectType type);

    // And create an object with a specified id
    //
    MappedObject* createObject(MappedObjectType type, MappedObjectId id);

    // Connect an Instrument to a MappedStudioObject
    //
    bool connectInstrument(InstrumentId iId, MappedObjectId mId);
    bool connectObjects(MappedObjectId mId1, MappedObjectId mId2);

    // Destroy a MappedObject
    //
    bool destroyObject(MappedObjectId id);

    // Get an object
    //
    MappedObject* getObject(MappedObjectId);

    // Get an object of a certain type - to see if any exist
    //
    MappedObject* getObjectOfType(MappedObjectType type);

    // iterators
    MappedObject* getFirst(MappedObjectType type);
    MappedObject* getNext(MappedObject *object);

    // Empty the studio of everything
    //
    void clear();

    // Property list
    //
    virtual MappedObjectPropertyList getPropertyList(
            const MappedObjectProperty &property);

    // Return the object vector
    //
    //std::vector<MappedObject*>* getObjects() const { return &m_objects; }

    // DCOP streaming
    //
    /* dunno if we need this
    friend QDataStream& operator>>(QDataStream &dS, MappedStudio *mS);
    friend QDataStream& operator<<(QDataStream &dS, MappedStudio *mS);
    friend QDataStream& operator>>(QDataStream &dS, MappedStudio &mS);
    friend QDataStream& operator<<(QDataStream &dS, const MappedStudio &mS);
    */

protected:

private:

    // We give everything we create a unique MappedObjectId for
    // this session.  So store the running total in here.
    //
    MappedObjectId             m_runningObjectId;

    // All of our mapped (virtual) studio resides in this vector -
    // probably eventually want to make this more efficient.
    //
    //
    std::vector<MappedObject*> m_objects;
};



// Audio fader
//
class MappedAudioFader : public MappedObject
{
public:
    MappedAudioFader(MappedObject *parent,
                     MappedObjectId id,
                     MappedObjectValue channels):
        MappedObject(parent,
                     "MappedAudioFader",
                     AudioFader,
                     id),
                     m_level(80), // assume 100 is max for the moment
                     m_channels(channels) {;}

    ~MappedAudioFader() {;}

    // level
    MappedObjectValue getLevel();
    void setLevel(MappedObjectValue param);

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

protected:

    MappedObjectValue m_level;
    MappedObjectValue m_channels;

};

#ifdef HAVE_LADSPA
class MappedLADSPAPlugin : public MappedObject, public LADSPAPlugin
{
public:
    MappedLADSPAPlugin(MappedObject *parent, MappedObjectId id):
        MappedObject(parent,
                     "MappedLADSPAPlugin",
                     MappedObject::LADSPAPlugin,
                     id) {;}

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

protected:
};

class MappedLADSPAPort : public MappedObject
{
public:
    MappedLADSPAPort(MappedObject *parent, MappedObjectId id);

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

    void setPortName(const std::string &name) { m_portName = name; }
    std::string getPortName() const { return m_portName; }

    void setRangeHint(const LADSPA_PortRangeHint &pRH)
        { m_portRangeHint = pRH; }
    LADSPA_PortRangeHint getRangeHint() const { return m_portRangeHint; }

    void setDescriptor(const LADSPA_PortDescriptor &pD)
        { m_portDescriptor = pD; }
    LADSPA_PortDescriptor getDescriptor() const { return m_portDescriptor; }

protected:
    std::string           m_portName;
    LADSPA_PortRangeHint  m_portRangeHint;
    LADSPA_PortDescriptor m_portDescriptor;

};

#endif


// MappedPluginManager locates and lists plugins and
// provides an interface for plugging them into the
// faders/Instruments.
//
//
class MappedAudioPluginManager : public MappedObject
{
public:
    static const MappedObjectProperty Plugins;


    MappedAudioPluginManager(MappedObject *parent, MappedObjectId id);
    ~MappedAudioPluginManager();

    // Property list
    //
    virtual MappedObjectPropertyList getPropertyList(
            const MappedObjectProperty &property);

    // Get a list of plugins and create MappedObjects out of them
    //
    void discoverPlugins(MappedStudio *studio);
    void clearPlugins(MappedStudio *studio);

#ifdef HAVE_LADSPA

    // Just some LADSPA paths 
    //
    void getenvLADSPAPath();
    std::string getLADSPAPath() { return m_path; }
    void setLADSPAPath(const std::string &path);
    void addLADSPAPath(const std::string &path);

#endif


protected:
    // Help discover plugins
    //
    void enumeratePlugin(MappedStudio *studio, const std::string& path);

    std::string m_path;
};

}

#endif // _MAPPEDSTUDIO_H_
