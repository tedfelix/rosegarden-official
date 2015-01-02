/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SymbolInsertionCommand.h"

#include "misc/Strings.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"
#include "base/Selection.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

SymbolInsertionCommand::SymbolInsertionCommand(Segment &segment, timeT time,
        Symbol symbol) :
        BasicCommand(getGlobalName(&symbol), segment, time, time + 1),
        m_symbol(symbol),
        m_lastInsertedEvent(0)
{
    // nothing
}

SymbolInsertionCommand::~SymbolInsertionCommand()
{
    // nothing
}

EventSelection *
SymbolInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

QString
SymbolInsertionCommand::getGlobalName(Symbol *)
{
    return tr("Insert &Symbol...");
    /*
        }
    */
}

void
SymbolInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = getSegment().findTime(getStartTime());

    i = helper.insertSymbol(getStartTime(), m_symbol);
    if (i != helper.segment().end())
        m_lastInsertedEvent = *i;

}

}
