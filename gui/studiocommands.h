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

#include <vector>

#include <klocale.h>

#include "basiccommand.h"

#include "Instrument.h"

namespace Rosegarden { class Studio; }

class ModifyBankCommand : public XKCommand
{
public:
    ModifyBankCommand(Rosegarden::Studio *studio,
                      int device,
                      int bank,
                      int msb,
                      int lsb,
                      std::vector<Rosegarden::MidiProgram> programList);

    static QString getGlobalName() { return i18n("Modify &MIDI Bank"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Studio                    *m_studio;
    int                                    m_device;
    int                                    m_bank;
    int                                    m_msb;
    int                                    m_lsb;
    std::vector<Rosegarden::MidiProgram>   m_programList;
};


