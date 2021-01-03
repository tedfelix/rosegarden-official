/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddControlParameterCommand.h"

#include "base/ControlParameter.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include <QString>
#include <iostream>


namespace Rosegarden
{

AddControlParameterCommand::~AddControlParameterCommand()
{}

void
AddControlParameterCommand::execute()
{
    MidiDevice *md = dynamic_cast<MidiDevice *>
                     (m_studio->getDevice(m_device));
    if (!md) {
        std::cerr << "WARNING: AddControlParameterCommand::execute: device "
        << m_device << " is not a MidiDevice in current studio"
        << std::endl;
        return ;
    }

    md->addControlParameter(m_control, true);

    // store id of the new control
    m_id = int(md->getControlParameters().size()) - 1;
}

void
AddControlParameterCommand::unexecute()
{
    MidiDevice *md = dynamic_cast<MidiDevice *>
                     (m_studio->getDevice(m_device));
    if (!md) {
        std::cerr << "WARNING: AddControlParameterCommand::unexecute: device "
        << m_device << " is not a MidiDevice in current studio"
        << std::endl;
        return ;
    }

    md->removeControlParameter(m_id);
}

}
