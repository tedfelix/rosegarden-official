
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_GUITARCHORDINSERTIONCOMMAND_H
#define RG_GUITARCHORDINSERTIONCOMMAND_H

#include "document/BasicCommand.h"
#include "base/Event.h"
#include "gui/editors/guitar/Chord.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Event;


class GuitarChordInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::GuitarChordInsertionCommand)

public:
    GuitarChordInsertionCommand(Segment &segment,
                              timeT time,
                              const Guitar::Chord& chord);
    virtual ~GuitarChordInsertionCommand();

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Guitar::Chord m_chord;
    Event *m_lastInsertedEvent;
};


}

#endif
