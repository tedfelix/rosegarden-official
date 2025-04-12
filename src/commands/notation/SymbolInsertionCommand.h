/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SYMBOLINSERTIONCOMMAND_H
#define RG_SYMBOLINSERTIONCOMMAND_H

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Event;

class SymbolInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SymbolInsertionCommand)

public:
    SymbolInsertionCommand(Segment &segment,
                           timeT time,
                           const Symbol& symbol);
    ~SymbolInsertionCommand() override;

    static QString getGlobalName(const Symbol *symbol = nullptr);

    EventSelection *getSubsequentSelection() override;
    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    void modifySegment() override;

    Symbol m_symbol;

    Event *m_lastInsertedEvent;
};


}

#endif
