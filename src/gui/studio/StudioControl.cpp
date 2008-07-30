/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "StudioControl.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "base/MidiProgram.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
#include "gui/application/RosegardenApplication.h"
#include "sound/MappedCommon.h"
#include "sound/MappedComposition.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"
#include "sound/MappedStudio.h"
#include <qcstring.h>
#include <qdatastream.h>
#include <qstring.h>


namespace Rosegarden
{

MappedObjectId
StudioControl::createStudioObject(MappedObject::MappedObjectType type)
{
Profiler profiler("StudioControl::createStudioObject", true);

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
} else
{
    QDataStream streamIn(replyData, IO_ReadOnly);
    streamIn >> value;
}

return value;
}

bool
StudioControl::destroyStudioObject(MappedObjectId id)
{
    Profiler profiler("StudioControl::destroyStudioObject", true);

    int value = 0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << int(id);

    if (!rgapp->sequencerCall("destroyMappedObject(int)",
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "destroyStudioObject - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
    Profiler profiler("StudioControl::getStudioObjectProperty", true);

    MappedObjectPropertyList list;

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << QString(property);

    if (!rgapp->sequencerCall("getPropertyList(int, QString)",
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getStudioObjectProperty - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
    Profiler profiler("StudioControl::setStudioObjectProperty(float)", true);

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
    Profiler profiler("StudioControl::setStudioObjectProperties(floats)", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << ids;
    streamOut << properties;
    streamOut << values;

    rgapp->sequencerSend("setMappedProperties(MappedObjectIdList, MappedObjectPropertyList, MappedObjectValueList)", data);

    return true;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                                       const MappedObjectProperty &property,
                                       const QString &value)
{
    Profiler profiler("StudioControl::setStudioObjectProperty(string)", true);

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
    Profiler profiler("StudioControl::setStudioObjectPropertyList", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;
    streamOut << (QString)property;
    streamOut << values;

    RG_DEBUG << "StudioControl::setStudioObjectPropertyList: " << values.size() << " values for property " << property << endl;

    rgapp->sequencerSend("setMappedPropertyList(int, QString, MappedObjectPropertyList)",
                         data);

    return true;
}

MappedObjectId
StudioControl::getStudioObjectByType(MappedObject::MappedObjectType type)
{
    Profiler profiler("StudioControl::getStudioObjectByType", true);

    int value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)type;

    if (!rgapp->sequencerCall("getMappedObjectId(int)",
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getStudioObjectByType - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
    Profiler profiler("StudioControl::setStudioPluginPort", true);

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
    Profiler profiler("StudioControl::getStudioPluginPort", true);

    float value = 0.0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)pluginId;
    streamOut << (unsigned long)portId;

    if (!rgapp->sequencerCall("getMappedPort(int, unsigned long int)",
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getStudioPluginPort - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getPluginInformation - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getPluginProgram - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
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
                              replyType, replyData, data)) {
        SEQMAN_DEBUG << "getPluginProgram - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    } else {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> rv;
    }

    return rv;
}

void
StudioControl::connectStudioObjects(MappedObjectId id1,
                                    MappedObjectId id2)
{
    Profiler profiler("StudioControl::connectStudioObjects", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id1;
    streamOut << (int)id2;

    if (!rgapp->sequencerSend("connectMappedObjects(int, int)", data)) {
        SEQMAN_DEBUG << "connectStudioObjects - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    }

    return ;
}

void
StudioControl::disconnectStudioObjects(MappedObjectId id1,
                                       MappedObjectId id2)
{
    Profiler profiler("StudioControl::disconnectStudioObjects", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id1;
    streamOut << (int)id2;

    if (!rgapp->sequencerSend("disconnectMappedObjects(int, int)", data)) {
        SEQMAN_DEBUG << "disconnectStudioObjects - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    }

    return ;
}

void
StudioControl::disconnectStudioObject(MappedObjectId id)
{
    Profiler profiler("StudioControl::disconnectStudioObject", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)id;

    if (!rgapp->sequencerSend("disconnectMappedObject(int)", data)) {
        SEQMAN_DEBUG << "disconnectStudioObject - "
        << "failed to contact Rosegarden sequencer"
        << endl;
    }

    return ;
}

void
StudioControl::sendMappedEvent(const MappedEvent &mE)
{
    Profiler profiler("StudioControl::sendMappedEvent", true);

    static MappedEvent mEs;

    mEs = mE; // just in case the passed mapped event has dubious
    // origins and taking its address isn't safe

    mEs.setPersistent(true); // to avoid that MappedComposition dtor try to free it

    MappedComposition mC;
    mC.insert(&mEs);
    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendMappedComposition(const MappedComposition &mC)
{
    Profiler profiler("StudioControl::sendMappedComposition", true);

    if (mC.size() == 0)
        return ;

    QCString replyType;
    QByteArray replyData;

    MappedComposition::const_iterator it = mC.begin();

    for (; it != mC.end(); it++) {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << (*it);
        rgapp->sequencerSend("processMappedEvent(MappedEvent)", data);
    }
}

void
StudioControl::sendMappedInstrument(const MappedInstrument &mI)
{
    Profiler profiler("StudioControl::sendMappedInstrument", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (int)mI.getType();
    streamOut << (unsigned char)mI.getChannel();
    streamOut << (unsigned int)mI.getId();

    rgapp->sequencerSend("setMappedInstrument(int, unsigned char, unsigned int)", data);
}

void
StudioControl::sendQuarterNoteLength(const RealTime &length)
{
    Profiler profiler("StudioControl::sendQuarterNoteLength", true);

    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << (long)length.sec;
    streamOut << (long)length.nsec;

    rgapp->sequencerSend("setQuarterNoteLength(long int, long int)", data);
}

void
StudioControl::sendRPN(InstrumentId instrumentId,
                       MidiByte paramMSB,
                       MidiByte paramLSB,
                       MidiByte /* controller */,
                       MidiByte value)
{
    Profiler profiler("StudioControl::sendRPN", true);

    MappedComposition mC;
    MappedEvent *mE =
        new MappedEvent(instrumentId,
                        MappedEvent::MidiController,
                        MIDI_CONTROLLER_RPN_2,
                        paramMSB);
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_RPN_1,
                         paramLSB);
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         6,  // data value changed
                         value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_RPN_2,
                         MidiMaxValue); // null
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_RPN_1,
                         MidiMaxValue); // null
    mC.insert(mE);


    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendNRPN(InstrumentId instrumentId,
                        MidiByte paramMSB,
                        MidiByte paramLSB,
                        MidiByte /* controller */,
                        MidiByte value)
{
    Profiler profiler("StudioControl::sendNRPN", true);

    MappedComposition mC;
    MappedEvent *mE =
        new MappedEvent(instrumentId,
                        MappedEvent::MidiController,
                        MIDI_CONTROLLER_NRPN_2,
                        paramMSB);
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_NRPN_1,
                         paramLSB);
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         6,  // data value changed
                         value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_RPN_2,
                         MidiMaxValue); // null
    mC.insert(mE);

    mE = new MappedEvent(instrumentId,
                         MappedEvent::MidiController,
                         MIDI_CONTROLLER_RPN_1,
                         MidiMaxValue); // null
    mC.insert(mE);
}

}
