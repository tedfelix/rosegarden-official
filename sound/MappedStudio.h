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

#include "Instrument.h"
#include "Device.h"
#include "PluginManager.h"

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


typedef unsigned int MappedObjectId;
typedef QString      MappedObjectProperty;
typedef int          MappedObjectValue;

// Every MappedStudio object derives from this class - if an
// object is static then you're only allowed one per Studio
// (as in the case of a PluginManager).
//
class MappedObject
{
public:

    // The object we can create
    //
    typedef enum
    {
        Studio,
        AudioFader,
        AudioPluginManager
    
    } MappedObjectType;

    MappedObject(const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id):
        m_type(type), m_id(id), m_static(false), m_name(name) {;}

    MappedObject(const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id,
                 bool s):
        m_type(type), m_id(id), m_static(s), m_name(name) {;}

    virtual ~MappedObject() {;}

    MappedObjectId getId() { return m_id; }
    MappedObjectType getType() { return m_type; }

    std::string getName() { return m_name; }
    void setName(const std::string &name) { m_name= name; }

protected:

    MappedObjectType m_type;
    MappedObjectId   m_id;
    bool             m_static;
    std::string      m_name;
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

    // Destroy a MappedStudio item
    //
    bool destroyItem(MappedObjectId id);

    // Get an object
    //
    MappedObject* getObject(MappedObjectId);

    // Get an object of a certain type - to see if any exist
    //
    MappedObject* getObjectOfType(MappedObjectType type);

    // Empty the studio of everything
    //
    void clear();

    // DCOP streaming
    //
    friend QDataStream& operator>>(QDataStream &dS, MappedStudio *mS);
    friend QDataStream& operator<<(QDataStream &dS, MappedStudio *mS);
    friend QDataStream& operator>>(QDataStream &dS, MappedStudio &mS);
    friend QDataStream& operator<<(QDataStream &dS, const MappedStudio &mS);

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
    MappedAudioFader(MappedObjectId id, MappedObjectValue channels):
        MappedObject("MappedAudioFader",
                     AudioFader,
                     id),
                     m_level(80), // assume 100 is max for the moment
                     m_channels(channels) {;}

    ~MappedAudioFader() {;}

    // level
    MappedObjectValue getLevel();
    void setLevel(MappedObjectValue param);

protected:

    MappedObjectValue m_level;
    MappedObjectValue m_channels;

};

// MappedPluginManager locates and lists plugins and
// provides an interface for plugging them into the
// faders/Instruments.
//
//
class MappedAudioPluginManager : public MappedObject, public PluginManager
{
public:
    MappedAudioPluginManager(MappedObjectId id);
    ~MappedAudioPluginManager();

protected:

};


}

#endif // _MAPPEDSTUDIO_H_
