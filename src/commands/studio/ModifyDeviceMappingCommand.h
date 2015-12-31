
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MODIFYDEVICEMAPPINGCOMMAND_H
#define RG_MODIFYDEVICEMAPPINGCOMMAND_H

#include "base/Device.h"
#include "base/Track.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>


class Modify;


namespace Rosegarden
{

class Studio;
class RosegardenDocument;
class Composition;


class ModifyDeviceMappingCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ModifyDeviceMappingCommand)

public:
    ModifyDeviceMappingCommand(RosegardenDocument *doc,
                               DeviceId fromDevice,
                               DeviceId toDevice);

    static QString getGlobalName() { return tr("Modify &Device Mapping"); }

    virtual void execute();
    virtual void unexecute();
protected:
    Composition *m_composition;
    Studio      *m_studio;
    DeviceId     m_fromDevice;
    DeviceId     m_toDevice;

    std::vector<std::pair<TrackId, InstrumentId> > m_mapping;
};


}

#endif
