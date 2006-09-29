
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_FRETBOARDINSERTIONCOMMAND_H_
#define _RG_FRETBOARDINSERTIONCOMMAND_H_

#include "document/BasicCommand.h"
#include "base/Event.h"
#include "gui/editors/guitar/Fingering.h"


namespace Rosegarden
{

class Segment;
class Event;


class FretboardInsertionCommand : public BasicCommand
{
public:
    FretboardInsertionCommand(Segment &segment,
                              timeT time,
                              Guitar::Fingering chord);
    virtual ~FretboardInsertionCommand();

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Guitar::Fingering m_chord;
    Event *m_lastInsertedEvent;
};


}

#endif
