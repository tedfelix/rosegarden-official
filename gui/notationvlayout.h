/***************************************************************************
                          notationvlayout.h  -  description
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

#ifndef NOTATIONVLAYOUT_H
#define NOTATIONVLAYOUT_H

#include "layoutengine.h"
#include "staff.h"
#include "notationelement.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationVLayout : public NotationLayout
{
public:
    NotationVLayout(Staff&);

protected:
    virtual void layout(NotationElementList::iterator&);

    Staff &m_staff;
};

#endif
