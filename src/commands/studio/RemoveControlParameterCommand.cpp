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

#define RG_MODULE_STRING "[RemoveControlParameterCommand]"

#include "RemoveControlParameterCommand.h"

#include "base/MidiDevice.h"
#include "base/Studio.h"
#include "misc/Debug.h"

#include <iostream>


namespace Rosegarden
{


void
RemoveControlParameterCommand::execute()
{
    MidiDevice *midiDevice =
            dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (!midiDevice) {
        RG_WARNING << "execute(): WARNING: device " << m_device << " is not a MidiDevice in current studio";
        return;
    }

    ControlParameter *param = midiDevice->getControlParameter(m_controllerIndex);
    if (param)
        m_oldControl = *param;

    midiDevice->removeControlParameter(m_controllerIndex);
}

void
RemoveControlParameterCommand::unexecute()
{
    MidiDevice *midiDevice =
            dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (!midiDevice) {
        RG_WARNING << "unexecute(): WARNING: device " << m_device << " is not a MidiDevice in current studio";
        return;
    }

    midiDevice->addControlParameter(m_oldControl, m_controllerIndex, true);
}


}
