// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
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

class Sequencer;


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
    static const MappedObjectProperty Name;
    static const MappedObjectProperty Instrument;
    static const MappedObjectProperty Position;

    // The object we can create
    //
    typedef enum
    {
        Studio,
        AudioFader,          // connectable fader - interfaces with devices
        AudioBuss,           // connectable buss - inferfaces with faders
        AudioPluginManager,
        LADSPAPlugin,
        LADSPAPort
   
    } MappedObjectType;

    MappedObject(MappedObject *parent,
                 const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id,
                 bool readOnly):
        m_type(type),
        m_id(id), 
        m_name(name),
        m_parent(parent),
        m_static(false), 
        m_readOnly(readOnly) {;}

    MappedObject(MappedObject *parent, 
                 const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id,
                 bool readOnly,
                 bool s):
        m_type(type),
        m_id(id),
        m_name(name),
        m_parent(parent),
        m_static(s),
        m_readOnly(readOnly) {;}

    virtual ~MappedObject() {;}

    MappedObjectId getId() { return m_id; }
    MappedObjectType getType() { return m_type; }

    std::string getName() { return m_name; }
    void setName(const std::string &name) { m_name= name; }

    // Get and set properties
    //
    virtual MappedObjectPropertyList
        getPropertyList(const MappedObjectProperty &property) = 0;

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) = 0;

    // Accepts a MappedObject* which to clone the current object into.
    // All children of the passed MappedObject are free'd and new children
    // created as part of the clone process.  This operation turns readonly
    // objects into real objects.
    //
    // For the moment we allow this method to be overwritten so that
    // in the case of child (end of line) classes we can implement
    // attribute copying easily.
    //
    virtual void clone(MappedObject *object);

    // Ownership
    //
    MappedObject* getParent() { return m_parent; }
    void setParent(MappedObject *parent) { m_parent = parent; }

    // Get a list of child ids - get a list of a certain type
    //
    MappedObjectPropertyList getChildren();
    MappedObjectPropertyList getChildren(MappedObjectType type);

    // Child management
    //
    void addChild(MappedObject *mO);
    void removeChild(MappedObject *mO);


    // Destruction
    //
    void destroy();
    void destroyChildren();

    std::vector<MappedObject*> getChildObjects() { return m_children; }

    bool isReadOnly() const { return m_readOnly; }

protected:

    MappedObjectType m_type;
    MappedObjectId   m_id;
    std::string      m_name;

    MappedObject                *m_parent;
    std::vector<MappedObject*>   m_children;

private:

    bool             m_static;
    bool             m_readOnly;

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
    MappedObject* createObject(MappedObjectType type,
                               bool readOnly);

    // And create an object with a specified id
    //
    MappedObject* createObject(MappedObjectType type, 
                               MappedObjectId id,
                               bool readOnly);

    // Connect an Instrument to a MappedStudioObject
    //
    bool connectInstrument(InstrumentId iId, MappedObjectId mId);
    bool connectObjects(MappedObjectId mId1, MappedObjectId mId2);

    // Destroy a MappedObject by ID
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

    // Clear the temporary elements leaving all read-only (system)
    // elements in place.
    //
    void clearTemporaries();

    // Clear a MappedObject reference from the Studio
    //
    bool clearObject(MappedObjectId id);

    // Property list
    //
    virtual MappedObjectPropertyList getPropertyList(
            const MappedObjectProperty &property);

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value);

    // Get an audio fader for an InstrumentId
    //
    MappedObject* getAudioFader(Rosegarden::InstrumentId id);

    MappedObject* getPluginInstance(Rosegarden::InstrumentId id,
                                    int position);

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


    // Set the sequencer object so that we can do things like
    // initialise plugins etc.
    //
    Rosegarden::Sequencer* getSequencer() { return m_sequencer; }
    void setSequencer(Rosegarden::Sequencer *sequencer)
        { m_sequencer = sequencer; }

#ifdef HAVE_LADSPA

    // Create a plugin instance
    //
    const LADSPA_Descriptor* createPluginDescriptor(unsigned long uniqueId);

    // Remove a plugin instance - and potentially unload the library
    //
    void unloadPlugin(unsigned long uniqueId);

    // Unload all the plugin libraries
    //
    void unloadAllPluginLibraries();

    // Modify a port
    //
    void setPluginInstancePort(InstrumentId id,
                               int position,
                               unsigned long portNumber,
                               LADSPA_Data value);

#endif // HAVE_LADSPA

protected:

private:

    // We give everything we create a unique MappedObjectId for
    // this session.  So store the running total in here.
    //
    MappedObjectId             m_runningObjectId;

    // All of our mapped (virtual) studio resides in this vector as
    // well as having all their parent/child relationships.  Because
    // some things are just blobs with no connections we need to
    // maintain both - don't forget about this.
    //
    std::vector<MappedObject*> m_objects;

    // Sequencer object
    //
    Rosegarden::Sequencer     *m_sequencer;
};


// A connectable AudioObject that provides a connection framework
// for MappedAudioFader and MappedAudioBuss (for example).  An
// asbtract base class.
//
// n input connections and m output connections - subclasses
// can do the cleverness if n != m
//
//

class MappedAudioObject : public MappedObject
{
public:
    static const MappedObjectProperty ConnectionsIn;
    static const MappedObjectProperty ConnectionsOut;
    static const MappedObjectProperty Channels;

    typedef enum
    {
        In,
        Out
    } AudioConnection;

    MappedAudioObject(MappedObject *parent,
                      const std::string &name,
                      MappedObjectType type,
                      MappedObjectId id,
                      MappedObjectValue channels,
                      bool readOnly);

    ~MappedAudioObject();

    // Get and set connections - these operate on normal MappedObject
    // properties but we provide these methods for convenience.
    //
    void setConnections(AudioConnection dir,
                        MappedObjectPropertyList conns);

    MappedObjectPropertyList getConnections (AudioConnection dir);

protected:

    // Which audio connections we have
    //
    MappedObjectPropertyList      m_connectionsIn;
    MappedObjectPropertyList      m_connectionsOut;

    // How many channels we carry
    //
    MappedObjectValue             m_channels;
};

// Audio fader
//
class MappedAudioFader : public MappedAudioObject
{
public:

    // properties
    //
    static const MappedObjectProperty FaderLevel;
    static const MappedObjectProperty FaderRecordLevel;
    static const MappedObjectProperty Pan;

    MappedAudioFader(MappedObject *parent,
                     MappedObjectId id,
                     MappedObjectValue channels = 2, // sterep default
                     bool readOnly = false);
    ~MappedAudioFader();

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value);

protected:

    MappedObjectValue             m_level;
    MappedObjectValue             m_recordLevel;
    Rosegarden::InstrumentId      m_instrumentId;

    // Are the plugins for this instrument to be bypassed?
    // (while the plugins don't reside here we use this as
    // the central point of contact)
    //
    bool                          m_bypassed; // we surely don't need this?

    // Stereo pan (-1.0 to +1.0)
    //
    MappedObjectValue             m_pan;

};

class MappedAudioBuss : public MappedAudioObject
{
public:

    MappedAudioBuss(MappedObject *parent,
                    MappedObjectId id,
                    MappedObjectValue channels = 2,  // stereo default
                    bool readOnly = false);
    ~MappedAudioBuss();

};

#ifdef HAVE_LADSPA
class MappedLADSPAPlugin : public MappedObject, public LADSPAPlugin
{
public:
    // properties
    static const MappedObjectProperty UniqueId;
    static const MappedObjectProperty PluginName;
    static const MappedObjectProperty Label;
    static const MappedObjectProperty Author;
    static const MappedObjectProperty Copyright;
    static const MappedObjectProperty PortCount;
    static const MappedObjectProperty Ports;
    static const MappedObjectProperty Bypassed;

    MappedLADSPAPlugin(MappedObject *parent,
                       MappedObjectId id,
                       bool readOnly):
        MappedObject(parent,
                     "MappedLADSPAPlugin",
                     MappedObject::LADSPAPlugin,
                     id,
                     readOnly),
                     m_instrument(0),
                     m_position(0),
                     m_bypassed(false) {;}

    MappedLADSPAPlugin(MappedObject *parent,
                       MappedObjectId id):
        MappedObject(parent,
                     "MappedLADSPAPlugin",
                     MappedObject::LADSPAPlugin,
                     id,
                     false),
                     m_instrument(0),
                     m_position(0),
                     m_bypassed(false) {;}

    ~MappedLADSPAPlugin();

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value);

    unsigned long getUniqueId() const { return m_uniqueId;}
    std::string getLabel() const { return m_label; }
    std::string getAuthor() const { return m_author; }
    std::string getCopyright() const { return m_copyright; }
    unsigned long getPortCount() const { return m_portCount; }

    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }
    int getPosition() const { return m_position; }

    // populate this object with descriptor information
    void populate(const LADSPA_Descriptor *descriptor);

    // redefine
    virtual void clone(MappedObject *object);

    // Cheat - we want to use a thin interface to ports from
    // the gui - this sets port values on port number.
    //
    void setPort(unsigned long portNumber, float value);

    bool isBypassed() const { return m_bypassed; }

protected:

    unsigned long             m_uniqueId;
    std::string               m_label;
    std::string               m_author;
    std::string               m_copyright;
    unsigned long             m_portCount;

    Rosegarden::InstrumentId  m_instrument;
    int                       m_position;
    bool                      m_bypassed;

};

class MappedLADSPAPort : public MappedObject
{
public:

    // properties
    //
    static const MappedObjectProperty Descriptor;
    static const MappedObjectProperty RangeHint;
    static const MappedObjectProperty RangeLower;
    static const MappedObjectProperty RangeUpper;
    static const MappedObjectProperty Value;
    static const MappedObjectProperty Default;

    static const MappedObjectProperty PortNumber;

    MappedLADSPAPort(MappedObject *parent,
                     MappedObjectId id,
                     bool readOnly);

    virtual MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property);

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value);

    void setPortName(const std::string &name) { m_portName = name; }
    std::string getPortName() const { return m_portName; }

    void setRangeHint(LADSPA_PortRangeHintDescriptor hd,
                      LADSPA_Data lowerBound,
                      LADSPA_Data upperBound)
        { m_portRangeHint.HintDescriptor = hd;
          m_portRangeHint.LowerBound = lowerBound;
          m_portRangeHint.UpperBound= upperBound; }
    LADSPA_PortRangeHint getRangeHint() const { return m_portRangeHint; }

    void setDescriptor(const LADSPA_PortDescriptor &pD)
        { m_portDescriptor = pD; }
    LADSPA_PortDescriptor getDescriptor() const { return m_portDescriptor; }
 
    void setPortNumber(unsigned long portNumber) { m_portNumber = portNumber; }
    unsigned long getPortNumber() const { return m_portNumber; }

    void setValue(MappedObjectValue value) { m_value = value; }
    MappedObjectValue getValue() const { return m_value; }

    void setDefault(MappedObjectValue value) { m_default = value; }
    MappedObjectValue getDefault() const { return m_default; }

    // redefine clone() here to copy across value only
    //
    virtual void clone(MappedObject *object);

protected:
    std::string           m_portName;
    LADSPA_PortRangeHint  m_portRangeHint;
    LADSPA_PortDescriptor m_portDescriptor;

    MappedObjectValue     m_value;
    MappedObjectValue     m_default;  // default value
    unsigned long         m_portNumber;

};

#endif // HAVE_LADSPA


// MappedPluginManager locates and lists plugins and
// provides an interface for plugging them into the
// faders/Instruments.
//
// Rather confusingly this manager holds both the class of 
// the plugin (in a read-only manner) as well as the actual
// live instances of the plugin.  While this isn't very
// clever or very efficient it works for the moment until
// we have the energy or patience to change it.
//
//

#ifdef HAVE_LADSPA
typedef std::vector<std::pair<std::string, void*> > LADSPAPluginHandles;
typedef std::vector<std::pair<std::string, void*> >::iterator LADSPAIterator;
#endif // HAVE_LADSPA

class MappedAudioPluginManager : public MappedObject
{
public:

    // public properties for the query interface
    //
    static const MappedObjectProperty Plugins;
    static const MappedObjectProperty PluginIds;

    MappedAudioPluginManager(MappedObject *parent,
                             MappedObjectId id,
                             bool readOnly);
    ~MappedAudioPluginManager();

    // Property list
    //
    virtual MappedObjectPropertyList getPropertyList(
            const MappedObjectProperty &property);

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value);

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

    // Locate and load a plugin from its uniqueId.  The caller
    // can then use the descriptor to create and control a plugin 
    // instance.
    //
    const LADSPA_Descriptor* getPluginDescriptor(unsigned long uniqueId);

    // Using this method we unload a plugin from the list of pluginHandles
    // and we check to see if we can unload the file completely.
    //
    void unloadPlugin(unsigned long uniqueId);

    // Close down all plugins
    //
    void unloadAllPluginLibraries();

    // Get all the plugin IDs from a library
    //
    std::vector<unsigned long> getPluginsInLibrary(void *pluginHandle);

#endif

    // Get the class of this plugin so we can clone() it
    //
    MappedObject* getReadOnlyPlugin(unsigned long uniqueId);

    // Get a plugin according to read-onlyness
    //
    MappedObject* getPluginInstance(unsigned long uniqueId, bool readOnly);

    // Get a plugin instance according to instrument and position
    //
    MappedObject* getPluginInstance(InstrumentId instrument, int position);

protected:
    // Help discover plugins
    //
    void enumeratePlugin(MappedStudio *studio, const std::string& path);

#ifdef HAVE_LADSPA
    const LADSPA_Descriptor* getDescriptorFromHandle(unsigned long uniqueId,
                                                     void *pluginHandle);


    // Keep a track of plugin handles
    //
    LADSPAPluginHandles m_pluginHandles;

#endif // HAVE_LADSPA

    std::string m_path;

};

}

#endif // _MAPPEDSTUDIO_H_
