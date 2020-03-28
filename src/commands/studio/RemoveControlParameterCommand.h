
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_REMOVECONTROLPARAMETERCOMMAND_H
#define RG_REMOVECONTROLPARAMETERCOMMAND_H

#include "base/ControlParameter.h"
#include "base/Device.h"
#include "document/Command.h"  // for NamedCommand

#include <QString>


namespace Rosegarden
{


class Studio;

/// Remove a Controller (CC) from a MidiDevice.
class RemoveControlParameterCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RemoveControlParameterCommand)

public:
    RemoveControlParameterCommand(Studio *studio,
                                  DeviceId device,
                                  int controllerIndex) :
    NamedCommand(tr("&Remove Control Parameter")),
    m_studio(studio),
    m_device(device),
    m_controllerIndex(controllerIndex)
    { }

    // Command overrides.
    void execute() override;
    void unexecute() override;

private:
    Studio *m_studio;
    DeviceId m_device;
    /// Index into the Device's list of controllers.
    int m_controllerIndex;

    ControlParameter m_oldControl;

};


}

#endif
