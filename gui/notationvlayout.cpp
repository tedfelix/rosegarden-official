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

NotationVLayout::NotationVLayout()
    : m_pitchToHeight(PitchToHeight::instance())
{
}

void
NotationVLayout::layout(Element2 *el)
{
    unsigned int pitch = el->get<Int>("pitch");
    el->set<Int>("Notation::Y", m_pitchToHeight[pitch]);
}
