// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include "Midi.h"

#include "rgapplication.h"
#include "studiocontrol.h"
#include "rosedebug.h"
#include "rosegardendcop.h"

#include "Profiler.h"

namespace Rosegarden
{

MappedObjectId
StudioControl::createStudioObject(MappedObject::MappedObjectType type)
{
    Rosegarden::Profiler profiler("StudioControl::createStudioObject", true);

    int value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)type;

    if (!rgapp->sequencerCall("createMappedObject(int)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "createStudioObject - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    return value;
}

bool
StudioControl::destroyStudioObject(MappedObjectId id)
{
    Rosegarden::Profiler profiler("StudioControl::destroyStudioObject", true);

    int value = 0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << int(id);

    if (!rgapp->sequencerCall("destroyMappedObject(int)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "destroyStudioObject - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    if (value == 1)
        return true;
    else
        return false;
}

MappedObjectPropertyList
StudioControl::getStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property)
{
    Rosegarden::Profiler profiler("StudioControl::getStudioObjectProperty", true);

    MappedObjectPropertyList list;

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << QString(property);

    if (!rgapp->sequencerCall("getPropertyList(int, QString)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getStudioObjectProperty - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> list;
    }

    return list;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property,
                        MappedObjectValue value)
{
    Rosegarden::Profiler profiler("StudioControl::setStudioObjectProperty(float)", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << (QString)property;
    streamOut << (float)value;

    rgapp->sequencerSend("setMappedProperty(int, QString, float)", data);

    return true;
}

bool
StudioControl::setStudioObjectProperties(const MappedObjectIdList &ids,
					 const MappedObjectPropertyList &properties,
					 const MappedObjectValueList &values)
{
    Rosegarden::Profiler profiler("StudioControl::setStudioObjectProperties(floats)", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << ids;
    streamOut << properties;
    streamOut << values;

    rgapp->sequencerSend("setMappedProperties(Rosegarden::MappedObjectIdList, Rosegarden::MappedObjectPropertyList, Rosegarden::MappedObjectValueList)", data);

    return true;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property,
                        const QString &value)
{
    Rosegarden::Profiler profiler("StudioControl::setStudioObjectProperty(string)", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << (QString)property;
    streamOut << (QString)value;

    rgapp->sequencerSend("setMappedProperty(int, QString, QString)", data);

    return true;
}

bool
StudioControl::setStudioObjectPropertyList(MappedObjectId id,
                        const MappedObjectProperty &property,
                        const MappedObjectPropertyList &values)
{
    Rosegarden::Profiler profiler("StudioControl::setStudioObjectPropertyList", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << (QString)property;
    streamOut << values;

    RG_DEBUG << "StudioControl::setStudioObjectPropertyList: " << values.size() << " values for property " << property << endl;

    rgapp->sequencerSend("setMappedPropertyList(int, QString, Rosegarden::MappedObjectPropertyList)",
                         data);
    
    return true;
}

MappedObjectId
StudioControl::getStudioObjectByType(MappedObject::MappedObjectType type)
{
    Rosegarden::Profiler profiler("StudioControl::getStudioObjectByType", true);

    int value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)type;

    if (!rgapp->sequencerCall("getMappedObjectId(int)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getStudioObjectByType - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    return value;
}

void
StudioControl::setStudioPluginPort(MappedObjectId pluginId,
                                   unsigned long portId,
                                   MappedObjectValue value)
{
    Rosegarden::Profiler profiler("StudioControl::setStudioPluginPort", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // Use new MappedEvent interface
    //
    streamOut << (int)pluginId;
    streamOut << (unsigned long)portId;
    streamOut << (float)value;

    rgapp->sequencerSend("setMappedPort(int, unsigned long int, float)", data);
}

MappedObjectValue
StudioControl::getStudioPluginPort(MappedObjectId pluginId,
                                   unsigned long portId)
{
    Rosegarden::Profiler profiler("StudioControl::getStudioPluginPort", true);

    float value = 0.0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)pluginId;
    streamOut << (unsigned long)portId;

    if (!rgapp->sequencerCall("getMappedPort(int, unsigned long int)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getStudioPluginPort - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    return value;
}


MappedObjectPropertyList
StudioControl::getPluginInformation()
{
    MappedObjectPropertyList list;

    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("getPluginInformation()",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getPluginInformation - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> list;
    }

    return list;
}


QString
StudioControl::getPluginProgram(MappedObjectId id, int bank, int program)
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << (int)bank;
    streamOut << (int)program;

    QString programName;

    if (!rgapp->sequencerCall("getPluginProgram(int, int, int)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getPluginProgram - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
	QDataStream streamIn(replyData, IO_ReadOnly);
	streamIn >> programName;
    }

    return programName;
}


unsigned long
StudioControl::getPluginProgram(MappedObjectId id, QString name)
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << name;

    unsigned long rv;

    if (!rgapp->sequencerCall("getPluginProgram(int, QString)",
                              replyType, replyData, data))
    {
        SEQMAN_DEBUG << "getPluginProgram - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
	QDataStream streamIn(replyData, IO_ReadOnly);
	streamIn >> rv;
    }

    return rv;
}


void
StudioControl::connectStudioObjects(MappedObjectId id1,
				    MappedObjectId id2)
{
    Rosegarden::Profiler profiler("StudioControl::connectStudioObjects", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id1;
    streamOut << (int)id2;

    if (!rgapp->sequencerSend("connectMappedObjects(int, int)", data))
    {
        SEQMAN_DEBUG << "connectStudioObjects - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }

    return;
}

void
StudioControl::disconnectStudioObjects(MappedObjectId id1,
				       MappedObjectId id2)
{
    Rosegarden::Profiler profiler("StudioControl::disconnectStudioObjects", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id1;
    streamOut << (int)id2;

    if (!rgapp->sequencerSend("disconnectMappedObjects(int, int)", data))
    {
        SEQMAN_DEBUG << "disconnectStudioObjects - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }

    return;
}

void
StudioControl::disconnectStudioObject(MappedObjectId id)
{
    Rosegarden::Profiler profiler("StudioControl::disconnectStudioObject", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;

    if (!rgapp->sequencerSend("disconnectMappedObject(int)", data))
    {
        SEQMAN_DEBUG << "disconnectStudioObject - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }

    return;
}

void
StudioControl::sendMappedEvent(const Rosegarden::MappedEvent &mE)
{
    Rosegarden::Profiler profiler("StudioControl::sendMappedEvent", true);

    static Rosegarden::MappedEvent mEs;
    
    mEs = mE; // just in case the passed mapped event has dubious
              // origins and taking its address isn't safe

    mEs.setPersistent(true); // to avoid that MappedComposition dtor try to free it

    Rosegarden::MappedComposition mC;
    mC.insert(&mEs);
    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendMappedComposition(const Rosegarden::MappedComposition &mC)
{
    Rosegarden::Profiler profiler("StudioControl::sendMappedComposition", true);

    if (mC.size() == 0)
        return;

    QCString replyType;
    QByteArray replyData;

    MappedComposition::const_iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << (*it);
        rgapp->sequencerSend("processMappedEvent(Rosegarden::MappedEvent)", data);
    }
}

void
StudioControl::sendMappedInstrument(const Rosegarden::MappedInstrument &mI)
{
    Rosegarden::Profiler profiler("StudioControl::sendMappedInstrument", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)mI.getType();
    streamOut << (unsigned char)mI.getChannel();
    streamOut << (unsigned int)mI.getId();

    rgapp->sequencerSend("setMappedInstrument(int, unsigned char, unsigned int)", data);
}


void
StudioControl::sendQuarterNoteLength(const Rosegarden::RealTime &length)
{
    Rosegarden::Profiler profiler("StudioControl::sendQuarterNoteLength", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (long)length.sec;
    streamOut << (long)length.nsec;

    rgapp->sequencerSend("setQuarterNoteLength(long int, long int)", data);
}


void
StudioControl::sendRPN(Rosegarden::InstrumentId instrumentId,
                       Rosegarden::MidiByte paramMSB,
                       Rosegarden::MidiByte paramLSB,
                       Rosegarden::MidiByte /* controller */,
                       Rosegarden::MidiByte value)
{
    Rosegarden::Profiler profiler("StudioControl::sendRPN", true);

    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    paramMSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     paramLSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     6, // data value changed
                                     value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE =  new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    Rosegarden::MidiMaxValue); // null
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     Rosegarden::MidiMaxValue); // null
    mC.insert(mE);


    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendNRPN(Rosegarden::InstrumentId instrumentId,
                        Rosegarden::MidiByte paramMSB,
                        Rosegarden::MidiByte paramLSB,
                        Rosegarden::MidiByte /* controller */,
                        Rosegarden::MidiByte value)
{
    Rosegarden::Profiler profiler("StudioControl::sendNRPN", true);

    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_NRPN_2,
                                    paramMSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_NRPN_1,
                                     paramLSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     6, // data value changed
                                     value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE =  new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    Rosegarden::MidiMaxValue); // null
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     Rosegarden::MidiMaxValue); // null
    mC.insert(mE);
}


}

