/***************************************************************************
                          notationvlayout.cpp  -  description
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

#include "notationvlayout.h"
#include "rosedebug.h"

NotationVLayout::NotationVLayout()
    : m_pitchToHeight(PitchToHeight::instance()),
      m_staffOffsetY(0)
{
}

void
NotationVLayout::layout(Event *el)
{
    unsigned int pitch = el->get<Int>("pitch");

    kdDebug(KDEBUG_AREA) << "pitch : " << pitch
                         << " - height : " << m_pitchToHeight[pitch]
                         << " - staffOffset : " << m_staffOffsetY
                         << " - height + staffOffset : " << m_pitchToHeight[pitch] + m_staffOffsetY
                         << endl;

    el->set<Int>("Notation::Y", m_pitchToHeight[pitch] + m_staffOffsetY);
}
