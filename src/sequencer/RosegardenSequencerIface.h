/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENSEQUENCERIFACE_H
#define RG_ROSEGARDENSEQUENCERIFACE_H

#include "gui/application/TransportStatus.h"

#include "base/Event.h"
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"
#include "base/Instrument.h"
#include "sound/MappedDevice.h"
#include "sound/MappedStudio.h"
#include "sound/MappedCommon.h"
#include "sound/MappedBufMetaIterator.h"

namespace Rosegarden {

/// UNUSED
/**
 * RosegardenSequencer used to derive from this class, but there is no
 * other use of RosegardenSequencerIface in the rest of the system.
 */
class RosegardenSequencerIface
{
public:
    /// Required or derived dtors won't be called through base pointers.
    virtual ~RosegardenSequencerIface()  { }

    // close the sequencer
    //
    virtual void quit() = 0;

    // play from a given time with given parameters
    //
    virtual bool play(const RealTime &position,
                      const RealTime &readAhead,
                      const RealTime &audioMix,
                      const RealTime &audioRead,
                      const RealTime &audioWrite,
                      long smallFileSize) = 0;

    // record from a given time with given parameters
    //
    virtual bool record(const RealTime &position,
                        const RealTime &readAhead,
                        const RealTime &audioMix,
                        const RealTime &audioRead,
                        const RealTime &audioWrite,
                        long smallFileSize,
                        long recordMode) = 0;

    // stop the sequencer
    //
    virtual void stop() = 0;

    // punch out from recording to playback
    //
    virtual bool punchOut() = 0;
    
    // Set the sequencer to a given time
    //
    virtual void jumpTo(const RealTime &) = 0;

    // Set a loop on the sequencer
    //
    virtual void setLoop(const RealTime &start,
                         const RealTime &end) = 0;

    // Get the status of the Sequencer
    //
    virtual unsigned int getSoundDriverStatus(const QString &guiVersion) = 0;

    // Add and delete audio files on the Sequencer
    //
    virtual bool addAudioFile(const QString &fileName, int id) = 0;
    virtual bool removeAudioFile(int id) = 0;
    virtual void clearAllAudioFiles() = 0;

    // Single set function as the MappedInstrument is so lightweight.
    // Any mods on the GUI are sent only through this method.
    //
    virtual void setMappedInstrument(int type,
                                     unsigned int id) = 0;

    // The proper implementation
    //
    virtual void processMappedEvent(MappedEvent mE) = 0;

    // Return device id following last existing one -- you can treat
    // this as "number of devices" but there might be some holes if
    // devices were deleted, which you will recognise because
    // getMappedDevice(id) will return a device with id NO_DEVICE
    //
/*!DEVPUSH
    virtual unsigned int getDevices() = 0;

    // Return device by number
    //
    virtual MappedDevice getMappedDevice(unsigned int id) = 0;
*/
    // Query whether the driver implements device reconnection.
    // Returns a non-zero value if the addDevice, removeDevice,
    // getConnections, getConnection and setConnection methods
    // may be used with devices of the given type.
    //
    virtual int canReconnect(Device::DeviceType deviceType) = 0;
    
    // Create a device of the given type and direction (corresponding
    // to MidiDevice::DeviceDirection enum) and return its id.
    // The device will have no connection by default.  Direction is
    // currently ignored for non-MIDI devices.
    // Do not use this unless canReconnect(type) returned true.
    //
    virtual bool addDevice(Device::DeviceType type,
                           DeviceId id,
                           InstrumentId baseInstrumentId,
                           MidiDevice::DeviceDirection direction) = 0;

    // Remove the device of the given id.
    // Ignored if driver does not permit changing the number of devices
    // (i.e. if canReconnect(type) would return false when given the
    // type of the supplied device).
    //
    virtual void removeDevice(unsigned int id) = 0;

    // Remove all of the devices (of types that can be added or removed).
    // Ignored if driver does not permit changing the number of devices
    //
    virtual void removeAllDevices() = 0;

    // Rename the given device.
    // Ignored if the driver does not permit this operation.
    //
    virtual void renameDevice(unsigned int id, QString name) = 0;

    // Return the number of permissible connections for a device of
    // the given type and direction (corresponding to MidiDevice::
    // DeviceDirection enum).  Direction is ignored for non-MIDI devices.
    // Returns zero if devices of this type are non-reconnectable
    // (i.e. if canReconnect(type) would return false).
    //
    virtual unsigned int getConnections(Device::DeviceType type,
                                        MidiDevice::DeviceDirection direction) = 0;

    // Return one of the set of permissible connections for a device of
    // the given type and direction (corresponding to MidiDevice::
    // DeviceDirection enum).  Direction is ignored for non-MIDI devices.
    // Returns the empty string for invalid parameters.
    // 
    virtual QString getConnection(Device::DeviceType type,
                                  MidiDevice::DeviceDirection direction,
                                  unsigned int connectionNo) = 0;

    // Return the current connection for the given device, or the
    // empty string if the driver does not permit reconnections or the
    // device is not connected.
    //
    virtual QString getConnection(DeviceId id) = 0;

    // Reconnect a particular device.
    // Ignored if driver does not permit reconnections or the connection
    // is not one of the permissible set for that device.
    // 
    virtual void setConnection(unsigned int deviceId, QString connection) = 0;

    // Reconnect a device to a particular connection or to the closest
    // thing to that connection currently available (using some heuristic).
    // Ignored if driver does not permit reconnections.
    // 
    virtual void setPlausibleConnection(unsigned int deviceId,
                                        QString idealConnection) = 0;

    // Ensure that at least one playback device is connected to
    // something, if there is at least one very obvious candidate to
    // connect it to
    //
    virtual void connectSomething() = 0;

    // Return the number of different timers we are capable of
    // sychronising against.  This may return 0 if the driver has no
    // ability to change the current timer.
    //
    virtual unsigned int getTimers() = 0;

    // Return the name of a timer from the available set (where
    // n is between 0 and the return value from getTimers() - 1).
    //
    virtual QString getTimer(unsigned int n) = 0;

    // Return the name of the timer we are currently synchronising
    // against.
    // 
    virtual QString getCurrentTimer() = 0;

    // Set the timer we are currently synchronising against.
    // Invalid arguments are simply ignored.
    //
    virtual void setCurrentTimer(QString timer) = 0;

    virtual void setLowLatencyMode(bool lowLatMode) = 0;

    // Fetch audio play latencies
    //
    virtual RealTime getAudioPlayLatency() = 0;
    virtual RealTime getAudioRecordLatency() = 0;

    // Set a property on a MappedObject
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   float value) = 0;

    // Set many properties on many MappedObjects
    //
    virtual void setMappedProperties(const MappedObjectIdList &ids,
                                     const MappedObjectPropertyList &properties,
                                     const MappedObjectValueList &values) = 0;

    // Set a string property on a MappedObject
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   const QString &value) = 0;

    // Set a MappedObject to a property list
    //
    virtual QString setMappedPropertyList(int id,
                                          const QString &property,
                                          const MappedObjectPropertyList &values) = 0;

    // Get a mapped object id for a object type
    //
    virtual int getMappedObjectId(int type) = 0;

    // Get a list of properties of a certain type from an object
    //
    virtual std::vector<QString> getPropertyList(int id,
                                                 const QString &property) = 0;

    // Get a list of available plugins
    //
    virtual std::vector<QString> getPluginInformation() = 0;

    // Nasty hack: program name/number mappings are one thing that
    // mapped object properties can't cope with
    //
    virtual QString getPluginProgram(int mappedId, int bank, int program) = 0;

    // Nastier hack: return value is bank << 16 + program
    //
    virtual unsigned long getPluginProgram(int mappedId, const QString &name) = 0;
    
    // Cheat - we can't use a call (getPropertyList) during playback
    // so we use this method to set port N on plugin X.
    //
    virtual void setMappedPort(int pluginIn,
                               unsigned long id,
                               float value) = 0;

    virtual float getMappedPort(int pluginIn,
                                unsigned long id) = 0;

    // Create a (transient, writeable) object
    //
    virtual int createMappedObject(int type) = 0;

    // Destroy an object
    //
    virtual bool destroyMappedObject(int id) = 0;

    // Connect two objects
    //
    virtual void connectMappedObjects(int id1, int id2) = 0;
    
    // Disconnect two objects
    //
    virtual void disconnectMappedObjects(int id1, int id2) = 0;

    // Disconnect an object from everything
    //
    virtual void disconnectMappedObject(int id) = 0;

    // Driver sample rate
    //
    virtual unsigned int getSampleRate() const = 0;

    // Initialise/Reinitialise the studio back down to read only objects
    // and set to defaults.
    //
    virtual void clearStudio() = 0;

    // Allow the GUI to tell the sequence the duration of a quarter
    // note when the TEMPO changes - this is to allow the sequencer
    // to generate MIDI clock (at 24 PPQN).
    //
    virtual void setQuarterNoteLength(RealTime) = 0;

    // Return a (potentially lengthy) human-readable status log
    //
    virtual QString getStatusLog() = 0;

    // Debug stuff, to check MmappedSegment::iterator
    virtual void dumpFirstSegment() = 0;

    /// A segment has been modified
    virtual void segmentModified(MappedEventBuffer *) = 0;

    /// Add a segment
    virtual void segmentAdded(MappedEventBuffer *) = 0;

    /// Delete a segment
    virtual void segmentAboutToBeDeleted(MappedEventBuffer *) = 0;

    /// Close all mapped segments
    virtual void compositionAboutToBeDeleted() = 0;

    /** Update mute (etc) statuses while playing. The sequencer handles
        this automatically (with no need for this call) for MIDI events,
        but it needs to be prodded when an already-playing audio segment
        drops in or out.
    */
    virtual void remapTracks() = 0;
};

}

#endif // RG_ROSEGARDENSEQUENCERIFACE_H
