/*
    Rosegarden-4
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

#include "studiocommands.h"

ModifyBankCommand::ModifyBankCommand(Rosegarden::Studio *studio,
                                     int device,
                                     int bank,
                                     int msb,
                                     int lsb,
                                     std::vector<Rosegarden::MidiProgram> programList):
    XKCommand(getGlobalName()),
    m_studio(studio),
    m_device(device),
    m_bank(bank),
    m_msb(msb),
    m_lsb(lsb),
    m_programList(programList)
{
}

void
ModifyBankCommand::execute()
{

}

void
ModifyBankCommand::unexecute()
{
}


