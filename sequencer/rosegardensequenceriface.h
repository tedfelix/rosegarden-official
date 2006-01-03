// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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

#ifndef _ROSEGARDENSEQUENCERIFACE_H_
#define _ROSEGARDENSEQUENCERIFACE_H_

#include <dcopobject.h>
// #include <qvaluevector.h>
// #include <qpair.h>

#include "rosegardendcop.h"
#include "Event.h"
#include "MappedComposition.h"
#include "MappedEvent.h"
#include "Instrument.h"
#include "MappedDevice.h"
#include "MappedRealTime.h"
#include "MappedStudio.h"
#include "MappedCommon.h"

class RosegardenSequencerIface : virtual public DCOPObject
{
    K_DCOP
public:
    k_dcop:

    // close the sequencer
    //
    virtual void quit() = 0;

    // play from a given time with given parameters
    //
    virtual int play(long timeSec,
                     long timeNsec,
                     long readAheadSec,
                     long readAheadNsec,
                     long audioMixSec,
                     long audioMixNsec,
                     long audioReadSec,
                     long audioReadNsec,
                     long audioWriteSec,
                     long audioWriteNsec,
		     long smallFileSize) = 0;

    // record from a given time with given parameters
    //
    virtual int record(long timeSec,
                       long timeNsec,
                       long readAheadSec,
                       long readAheadNsec,
		       long audioMixSec,
		       long audioMixNsec,
		       long audioReadSec,
		       long audioReadNsec,
		       long audioWriteSec,
		       long audioWriteNsec,
		       long smallFileSize,
                       long recordMode) = 0;

    // stop the sequencer
    //
    virtual ASYNC stop() = 0;

    // Set the sequencer to a given time
    //
    virtual void jumpTo(long posSec, long posNsec) = 0;

    // Set a loop on the sequencer
    //
    virtual void setLoop(long loopStartSec,
                         long loopStartNsec,
                         long loopEndSec,
                         long loopEndNsec) = 0;

    // Get the status of the Sequencer
    //
    virtual unsigned int getSoundDriverStatus() = 0;

    // Add and delete audio files on the Sequencer
    //
    virtual int addAudioFile(const QString &fileName, int id) = 0;
    virtual int removeAudioFile(int id) = 0;
    virtual void clearAllAudioFiles() = 0;

    // Single set function as the MappedInstrument is so lightweight.
    // Any mods on the GUI are sent only through this method.
    //
    virtual void setMappedInstrument(int type,
                                     unsigned char channel,
                                     unsigned int id) = 0;

    // The GUI can use this method to process an immediate selection
    // of MappedEvents (Program Changes, SysExs, async Events etc).
    //
    virtual void processSequencerSlice(Rosegarden::MappedComposition mC) = 0;


    // Horrible ugly ugly ugly interface for single MappedEvents
    // just until we implement the proper MappedEvent interface
    //
    virtual void processMappedEvent(unsigned int id,
                                    int type,
                                    unsigned char pitch,
                                    unsigned char velocity,
                                    long absTimeSec,
                                    long absTimeNsec,
                                    long durationSec,
                                    long durationNsec,
                                    long audioStartMarkerSec,
                                    long audioStartMarkerNsec) = 0;

    // The proper implementation
    //
    virtual void processMappedEvent(Rosegarden::MappedEvent mE) = 0;

    // Return device id following last existing one -- you can treat
    // this as "number of devices" but there might be some holes if
    // devices were deleted, which you will recognise because
    // getMappedDevice(id) will return a device with id NO_DEVICE
    //
    virtual unsigned int getDevices() = 0;

    // Return device by number
    //
    virtual Rosegarden::MappedDevice getMappedDevice(unsigned int id) = 0;

    // Query whether the driver implements device reconnection.
    // Returns a non-zero value if the addDevice, removeDevice,
    // getConnections, getConnection and setConnection methods
    // may be used with devices of the given type.
    //
    virtual int canReconnect(int deviceType) = 0;
    
    // Create a device of the given type and direction (corresponding
    // to MidiDevice::DeviceDirection enum) and return its id.
    // The device will have no connection by default.  Direction is
    // currently ignored for non-MIDI devices.
    // Do not use this unless canReconnect(type) returned true.
    //
    virtual unsigned int addDevice(int type, unsigned int direction) = 0;

    // Remove the device of the given id.
    // Ignored if driver does not permit changing the number of devices
    // (i.e. if canReconnect(type) would return false when given the
    // type of the supplied device).
    //
    virtual void removeDevice(unsigned int id) = 0;

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
    virtual unsigned int getConnections(int type, unsigned int direction) = 0;

    // Return one of the set of permissible connections for a device of
    // the given type and direction (corresponding to MidiDevice::
    // DeviceDirection enum).  Direction is ignored for non-MIDI devices.
    // Returns the empty string for invalid parameters.
    // 
    virtual QString getConnection(int type,
				  unsigned int direction,
				  unsigned int connectionNo) = 0;

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
    virtual Rosegarden::MappedRealTime getAudioPlayLatency() = 0;
    virtual Rosegarden::MappedRealTime getAudioRecordLatency() = 0;

    // Set a property on a MappedObject
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   float value) = 0;

    // Set many properties on many MappedObjects
    //
    virtual void setMappedProperties(const Rosegarden::MappedObjectIdList &ids,
				     const Rosegarden::MappedObjectPropertyList &properties,
				     const Rosegarden::MappedObjectValueList &values) = 0;

    // Set a string property on a MappedObject
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   const QString &value) = 0;

    // Set a MappedObject to a property list
    //
    virtual void setMappedPropertyList(
                               int id,
                               const QString &property,
                               const Rosegarden::MappedObjectPropertyList &values) = 0;

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

    // Destroy an object (returns a bool but for KDE2 DCOP compat we
    // use an int of course).
    //
    virtual int destroyMappedObject(int id) = 0;

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
    virtual void setQuarterNoteLength(long timeSec, long timeNsec) = 0;

    // Return a (potentially lengthy) human-readable status log
    //
    virtual QString getStatusLog() = 0;

    // Debug stuff, to check MmappedSegment::iterator
    virtual void dumpFirstSegment() = 0;

    /// Remap a segment while playing
    virtual void remapSegment(const QString& filename, size_t newSize) = 0;

    /// Add a segment while playing
    virtual void addSegment(const QString& filename) = 0;

    /// Delete a segment while playing
    virtual void deleteSegment(const QString& filename) = 0;

    /// Close all mmapped segments
    virtual void closeAllSegments() = 0;

    /** Update mute (etc) statuses while playing. The sequencer handles
	this automatically (with no need for this call) for MIDI events,
	but it needs to be prodded when an already-playing audio segment
	drops in or out.
    */
    virtual void remapTracks() = 0;
};

#endif // _ROSEGARDENSEQUENCERIFACE_H_
