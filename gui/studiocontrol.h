// -*- c-basic-offset: 4 -*-
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

#include "MappedCommon.h"
#include "MappedStudio.h"
#include "MappedComposition.h"
#include "MappedInstrument.h"

#ifndef _STUDIOCONTROL_H_
#define _STUDIOCONTROL_H_

// Access and control the sequencer studio from the gui
//

namespace Rosegarden
{

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

    // Set a value to a string list
    //
    static bool setStudioObjectPropertyList(MappedObjectId id,
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
    static void sendMappedEvent(const Rosegarden::MappedEvent& mE);
    static void sendMappedComposition(const Rosegarden::MappedComposition &mC);

    // MappedInstrument
    //
    static void sendMappedInstrument(const Rosegarden::MappedInstrument &mI);

    // Send the Quarter Note Length has changed to the sequencer
    //
    static void sendQuarterNoteLength(const Rosegarden::RealTime &length);

    // Convenience wrappers for RPNs and NRPNs
    //
    static void sendRPN(Rosegarden::InstrumentId instrumentId,
                        Rosegarden::MidiByte paramMSB,
                        Rosegarden::MidiByte paramLSB,
                        Rosegarden::MidiByte controller,
                        Rosegarden::MidiByte value);

    static void sendNRPN(Rosegarden::InstrumentId instrumentId,
                         Rosegarden::MidiByte paramMSB,
                         Rosegarden::MidiByte paramLSB,
                         Rosegarden::MidiByte controller,
                         Rosegarden::MidiByte value);
};

}

#endif // _STUDIOCONTROL_H_

