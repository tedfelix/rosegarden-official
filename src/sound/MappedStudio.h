/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2018 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDSTUDIO_H
#define RG_MAPPEDSTUDIO_H

#include <map>
#include <string>
#include <vector>
#include <QDataStream>
#include <QString>

#include "MappedCommon.h"
#include "base/Instrument.h"
#include "base/Device.h"

#include "base/AudioPluginInstance.h" // for PluginPort::PortDisplayHint //!!!???


// A sequencer-side representation of certain elements in the
// gui that enables us to control outgoing or incoming audio
// and MIDI with run-time only persistence.  Placeholders for
// our Studio elements on the sequencer.

namespace Rosegarden
{

class SoundDriver;


// Types are in MappedCommon.h
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
        AudioInput,          // connectable record input
        PluginSlot,
        PluginPort

    } MappedObjectType;

    MappedObject(MappedObject *parent,
                 const std::string &name,
                 MappedObjectType type,
                 MappedObjectId id):
        m_type(type),
        m_id(id), 
        m_name(name),
        m_parent(parent) {;}

    virtual ~MappedObject() {;}

    MappedObjectId getId() { return m_id; }
    MappedObjectType getType() { return m_type; }

    std::string getName() { return m_name; }
    void setName(const std::string &name) { m_name= name; }

    // Get and set properties
    //
    virtual MappedObjectPropertyList
        getPropertyList(const MappedObjectProperty &property) = 0;

    virtual bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) = 0;

    // Only relevant to objects that have string properties
    // 
    virtual bool getStringProperty(const MappedObjectProperty &/* property */,
                                   QString &/* value */) { return false; }

    virtual void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) = 0;

    // Only relevant to objects that have string properties
    // 
    virtual void setStringProperty(const MappedObjectProperty &/* property */,
                                   QString /* value */) { }

    // Only relevant to objects that have list properties
    //
    virtual void setPropertyList(const MappedObjectProperty &/* property */,
                                 const MappedObjectPropertyList &/* values */) { }

    // Ownership
    //
    MappedObject* getParent() { return m_parent; }
    const MappedObject* getParent() const { return m_parent; }
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

protected:

    MappedObjectType m_type;
    MappedObjectId   m_id;
    std::string      m_name;

    MappedObject                *m_parent;
    std::vector<MappedObject*>   m_children;
};


class MappedAudioFader;
class MappedAudioBuss;
class MappedAudioInput;

// Works as a factory and virtual plug-board for all our other
// objects whether they be MIDI or audio.
//
//
//
class MappedStudio : public MappedObject
{
public:
    MappedStudio();
    ~MappedStudio() override;

    // Create a new slider of a certain type for a certain
    // type of device.
    //
    MappedObject* createObject(MappedObjectType type);

    // And create an object with a specified id
    //
    MappedObject* createObject(MappedObjectType type, 
                               MappedObjectId id);

    bool connectObjects(MappedObjectId mId1, MappedObjectId mId2);
    bool disconnectObjects(MappedObjectId mId1, MappedObjectId mId2);
    bool disconnectObject(MappedObjectId mId);

    // Destroy a MappedObject by ID
    //
    bool destroyObject(MappedObjectId id);

    // Get an object by ID only
    //
    MappedObject* getObjectById(MappedObjectId);

    // Get an object by ID and type.  (Returns 0 if the ID does not
    // exist or exists but is not of the correct type.)  This is
    // faster than getObjectById if you know the type already.
    //
    MappedObject* getObjectByIdAndType(MappedObjectId, MappedObjectType);

    // Get an arbitrary object of a given type - to see if any exist
    //
    MappedObject* getObjectOfType(MappedObjectType type);

    // Find out how many objects there are of a certain type
    //
    unsigned int getObjectCount(MappedObjectType type);

    // iterators
    MappedObject* getFirst(MappedObjectType type);
    MappedObject* getNext(MappedObject *object);

    std::vector<MappedObject *> getObjectsOfType(MappedObjectType type);

    // Empty the studio of everything
    //
    void clear();

    // Clear a MappedObject reference from the Studio
    //
    bool clearObject(MappedObjectId id);

    // Property list
    //
    MappedObjectPropertyList getPropertyList(
            const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    // Get an audio fader for an InstrumentId.  Convenience function.
    //
    MappedAudioFader *getAudioFader(InstrumentId id);
    MappedAudioBuss *getAudioBuss(int bussNumber); // not buss no., not object id
    MappedAudioInput *getAudioInput(int inputNumber); // likewise

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


    // Set the driver object so that we can do things like
    // initialise plugins etc.
    //
    SoundDriver* getSoundDriver() { return m_soundDriver; }
    const SoundDriver* getSoundDriver() const { return m_soundDriver; }
    void setSoundDriver(SoundDriver *driver) { m_soundDriver = driver; }

protected:

private:

    // We give everything we create a unique MappedObjectId for
    // this session.  So store the running total in here.
    //
    MappedObjectId             m_runningObjectId;

    // All of our mapped (virtual) studio resides in this container as
    // well as having all their parent/child relationships.  Because
    // some things are just blobs with no connections we need to
    // maintain both - don't forget about this.
    //
    // Note that object IDs are globally unique, not just unique within
    // a category.
    //
    typedef std::map<MappedObjectId, MappedObject *> MappedObjectCategory;
    typedef std::map<MappedObjectType, MappedObjectCategory> MappedObjectMap;
    MappedObjectMap m_objects;
    
    // Driver object
    //
    SoundDriver *m_soundDriver;
};


// A connectable AudioObject that provides a connection framework
// for MappedAudioFader and MappedAudioBuss (for example).  An
// abstract base class.
//
// n input connections and m output connections - subclasses
// can do the cleverness if n != m
//

class MappedConnectableObject : public MappedObject
{
public:
    static const MappedObjectProperty ConnectionsIn;
    static const MappedObjectProperty ConnectionsOut;

    typedef enum
    {
        In,
        Out
    } ConnectionDirection;

    MappedConnectableObject(MappedObject *parent,
                            const std::string &name,
                            MappedObjectType type,
                            MappedObjectId id);

    ~MappedConnectableObject() override;

    void setConnections(ConnectionDirection dir,
                        MappedObjectValueList conns);

    void addConnection(ConnectionDirection dir, MappedObjectId id);
    void removeConnection(ConnectionDirection dir, MappedObjectId id);

    MappedObjectValueList getConnections (ConnectionDirection dir);

protected:

    // Which audio connections we have
    //
    MappedObjectValueList      m_connectionsIn;
    MappedObjectValueList      m_connectionsOut;
};

// Audio fader
//
class MappedAudioFader : public MappedConnectableObject
{
public:
    static const MappedObjectProperty Channels;

    // properties
    //
    static const MappedObjectProperty FaderLevel;
    static const MappedObjectProperty FaderRecordLevel;
    static const MappedObjectProperty Pan;
    static const MappedObjectProperty InputChannel;

    MappedAudioFader(MappedObject *parent,
                     MappedObjectId id,
                     MappedObjectValue channels = 2); // stereo default
    ~MappedAudioFader() override;

    MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    InstrumentId getInstrument() const { return m_instrumentId; }

protected:

    MappedObjectValue             m_level;
    MappedObjectValue             m_recordLevel;
    InstrumentId      m_instrumentId;

    // Stereo pan (-1.0 to +1.0)
    //
    MappedObjectValue             m_pan;

    // How many channels we carry
    //
    MappedObjectValue             m_channels;

    // If we have an input, which channel we take from it (if we are
    // a mono fader at least)
    //
    MappedObjectValue             m_inputChannel;
};

class MappedAudioBuss : public MappedConnectableObject
{
public:
    // A buss is much simpler than an instrument fader.  It's always
    // stereo, and just has a level and pan associated with it.  The
    // level may be a submaster fader level or a send mix level, it
    // depends on what the purpose of the buss is.  At the moment we
    // just have a 1-1 relationship between busses and submasters, and
    // no send channels.

    static const MappedObjectProperty BussId;
    static const MappedObjectProperty Pan;
    static const MappedObjectProperty Level;

    MappedAudioBuss(MappedObject *parent,
                    MappedObjectId id);
    ~MappedAudioBuss() override;

    MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    MappedObjectValue getBussId() { return m_bussId; }

    // super-convenience function: retrieve the ids of the instruments
    // connected to this buss
    std::vector<InstrumentId> getInstruments();

protected:
    int m_bussId;
    MappedObjectValue m_level;
    MappedObjectValue m_pan;
};

class MappedAudioInput : public MappedConnectableObject
{
public:
    // An input is simpler still -- no properties at all, apart from
    // the input number, otherwise just the connections

    static const MappedObjectProperty InputNumber;

    MappedAudioInput(MappedObject *parent,
                     MappedObjectId id);
    ~MappedAudioInput() override;

    MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    MappedObjectValue getInputNumber() { return m_inputNumber; }

protected:
    MappedObjectValue m_inputNumber;
};

class MappedPluginSlot : public MappedObject
{
public:
    static const MappedObjectProperty Identifier;
    static const MappedObjectProperty PluginName;
    static const MappedObjectProperty Label;
    static const MappedObjectProperty Author;
    static const MappedObjectProperty Copyright;
    static const MappedObjectProperty Category;
    static const MappedObjectProperty PortCount;
    static const MappedObjectProperty Ports;
    static const MappedObjectProperty Program;
    static const MappedObjectProperty Programs; // list property
    static const MappedObjectProperty Instrument;
    static const MappedObjectProperty Position;
    static const MappedObjectProperty Bypassed;
    static const MappedObjectProperty Configuration; // list property

    MappedPluginSlot(MappedObject *parent, MappedObjectId id);
    ~MappedPluginSlot() override;

    MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    bool getStringProperty(const MappedObjectProperty &property,
                                   QString &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    void setStringProperty(const MappedObjectProperty &property,
                                   QString value) override;

    void setPropertyList(const MappedObjectProperty &,
                                 const MappedObjectPropertyList &) override;

    void  setPort(unsigned long portNumber, float value);
    float getPort(unsigned long portNumber);

    InstrumentId getInstrument() const { return m_instrument; }
    int getPosition() const { return m_position; }

    QString getProgram(int bank, int program);
    unsigned long getProgram(QString name); // rv is bank << 16 + program

protected:
    QString                   m_identifier;

    QString                   m_name;
    QString                   m_label;
    QString                   m_author;
    QString                   m_copyright;
    QString                   m_category;
    unsigned long             m_portCount;

    InstrumentId              m_instrument;
    int                       m_position;
    bool                      m_bypassed;

    std::map<QString, QString> m_configuration;
};

class MappedPluginPort : public MappedObject
{
public:
    static const MappedObjectProperty PortNumber;
    static const MappedObjectProperty Name;
    static const MappedObjectProperty Minimum;
    static const MappedObjectProperty Maximum;
    static const MappedObjectProperty Default;
    static const MappedObjectProperty DisplayHint;
    static const MappedObjectProperty Value;

    MappedPluginPort(MappedObject *parent, MappedObjectId id);
    ~MappedPluginPort() override;

    MappedObjectPropertyList getPropertyList(
                        const MappedObjectProperty &property) override;

    bool getProperty(const MappedObjectProperty &property,
                             MappedObjectValue &value) override;

    bool getStringProperty(const MappedObjectProperty &property,
                                   QString &value) override;

    void setProperty(const MappedObjectProperty &property,
                             MappedObjectValue value) override;

    void setStringProperty(const MappedObjectProperty &property,
                                   QString value) override;

    void setValue(MappedObjectValue value);
    MappedObjectValue getValue() const;

    int getPortNumber() const { return m_portNumber; }

protected:
    int                     m_portNumber;
    QString                 m_name;
    MappedObjectValue       m_minimum;
    MappedObjectValue       m_maximum;
    MappedObjectValue       m_default;
    PluginPort::PortDisplayHint m_displayHint;

};

    
}

#endif // RG_MAPPEDSTUDIO_H
