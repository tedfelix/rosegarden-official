/***************************************************************************
                          notationhlayout.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "notationhlayout.h"

NotationHLayout::NotationHLayout(unsigned int barWidth)
    : m_barWidth(barWidth),
      m_lastPos(10)
{
}

void
NotationHLayout::layout(Event *el)
{
    Note note = m_quantizer.quantizeToNoteType( el->getDuration() );
    el->set<Int>("Notation::NoteType", note);

    el->set<Int>("Notation::X", m_lastPos + 20);
    m_lastPos += 20;
}
