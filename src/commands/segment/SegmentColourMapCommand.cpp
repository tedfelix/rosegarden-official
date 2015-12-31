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


#include "SegmentColourMapCommand.h"

#include "base/ColourMap.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include <QString>


namespace Rosegarden
{

SegmentColourMapCommand::SegmentColourMapCommand(
    RosegardenDocument *doc,
    const ColourMap &map):
        NamedCommand(tr("Change Segment Color Map")),
        m_doc(doc),
        m_oldMap(m_doc->getComposition().getSegmentColourMap()),
        m_newMap(map)
{
}

SegmentColourMapCommand::~SegmentColourMapCommand()
{}

void
SegmentColourMapCommand::execute()
{
    m_doc->getComposition().setSegmentColourMap(m_newMap);
    m_doc->slotDocColoursChanged();
}

void
SegmentColourMapCommand::unexecute()
{
    m_doc->getComposition().setSegmentColourMap(m_oldMap);
    m_doc->slotDocColoursChanged();
}

}
