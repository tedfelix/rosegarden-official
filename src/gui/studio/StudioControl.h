/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_STUDIOCONTROL_H
#define RG_STUDIOCONTROL_H

#include "base/MidiProgram.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include <QMutex>
#include <QString>


class MappedObjectValueList;
class MappedObjectIdList;


namespace Rosegarden
{

class RealTime;
class MappedInstrument;
class MappedEvent;
class MappedEventList;
class Instrument;
class ImmediateNote;

typedef std::pair<Rosegarden::MidiByte, Rosegarden::MidiByte> MidiControlPair;

class StudioControl
{
public:

    // Object management
    //
    static MappedObjectId
        createStudioObject(MappedObject::MappedObjectType type);
    static MappedObjectId
        getStudioObjectByType(MappedObject::MappedObjectType type);
    static bool destroyStudioObject(MappedObjectId id);

    // Properties
    //
    static MappedObjectPropertyList
                  getStudioObjectProperty(MappedObjectId id,
                                          const MappedObjectProperty &property);

    // Set a value to a value
    //
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        MappedObjectValue value);

    // Set many values to values
    //
    static bool setStudioObjectProperties(const MappedObjectIdList &ids,
                                          const MappedObjectPropertyList &properties,
                                          const MappedObjectValueList &values);

    // Set a value to a string 
    //
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        const QString &value);

    // Set a value to a string list.  Return value is error if any.
    //
    static QString setStudioObjectPropertyList(MappedObjectId id,
					       const MappedObjectProperty &property,
					       const MappedObjectPropertyList &values);

    static void setStudioPluginPort(MappedObjectId pluginId,
                                    unsigned long portId,
                                    MappedObjectValue value);

    static MappedObjectValue getStudioPluginPort(MappedObjectId pluginId,
                                                 unsigned long portId);

    // Get all plugin information
    //
    static MappedObjectPropertyList getPluginInformation();

    // Get program name for a given program
    // 
    static QString getPluginProgram(MappedObjectId, int bank, int program);

    // Get program numbers for a given name (rv is bank << 16 + program)
    // This is one of the nastiest hacks in the whole application
    // 
    static unsigned long getPluginProgram(MappedObjectId, QString name);

    // Connection
    //
    static void connectStudioObjects(MappedObjectId id1,
                                     MappedObjectId id2);
    static void disconnectStudioObjects(MappedObjectId id1,
                                        MappedObjectId id2);
    static void disconnectStudioObject(MappedObjectId id);

    // Send controllers and other one off MIDI events using these
    // interfaces.
    //
    static void sendMappedEvent(const MappedEvent& mE);
    static void sendMappedEventList(const MappedEventList &mC);

    // MappedInstrument
    //
    static void sendMappedInstrument(const MappedInstrument &mI);

    // Send the Quarter Note Length has changed to the sequencer
    //
    static void sendQuarterNoteLength(const RealTime &length);

    static void playPreviewNote(Instrument *instrument, int pitch,
                                int velocity, RealTime duration,
                                bool oneshot = true);

    /// Send bank selects, program changes, and controllers immediately.
    /**
     * Set up a channel for output.
     *
     * This is used for fixed channel instruments and also to set up
     * MIDI thru channels.
     */
    static void sendChannelSetup(Instrument *instrument, int channel);
    static void sendController(const Instrument *instrument, int channel,
                               MidiByte controller, MidiByte value);

 private:
    static ImmediateNote *getFiller(void);

    static QMutex         m_instanceMutex;
    static ImmediateNote *m_immediateNoteFiller;
};

}

#endif
