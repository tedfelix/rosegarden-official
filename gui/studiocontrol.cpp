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

#include <kapp.h>
#include <dcopclient.h>

#include "studiocontrol.h"
#include "rosedebug.h"
#include "rosegardendcop.h"

namespace Rosegarden
{

static MappedObjectId
StudioControl::createStudioObject(MappedObject::MappedObjectType type)
{
    Rosegarden::MappedObjectId value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << type;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "createMappedObject(int)",
                                  data, replyType, replyData))
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

static bool
StudioControl::destroyStudioObject(MappedObjectId id)
{
    int value = 0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << int(id);

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "destroyMappedObject(int)",
                                  data, replyType, replyData))
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

    return bool(value);
}

static MappedObjectPropertyList
StudioControl::getStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property)
{
    QValueVector<QString> list;

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << id;
    streamOut << QString(property);

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getPropertyList(int, QString)",
                                  data, replyType, replyData))
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

static bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property,
                        MappedObjectValue value)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // Use new MappedEvent interface
    //
    streamOut << id;
    streamOut << property;
    streamOut << value;

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setMappedProperty(unsigned int, QString, int)",
                 data))
    {
        SEQMAN_DEBUG << "setStudioObjectProperty - "
                     << "failed to contact Rosegarden sequencer" << endl;
    }

    return true;
}


};

