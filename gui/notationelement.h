/***************************************************************************
                          notationelement.h  -  description
                             -------------------
    begin                : Sun Sep 10 2000
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

#ifndef NOTATIONELEMENT_H
#define NOTATIONELEMENT_H

#include "Element2.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationElement : public ViewElement
{
public: 
    NotationElement(Event *event);

    ~NotationElement();

    double x() { return m_x; }
    double y() { return m_y; }

    void setX(double x) { m_x = x; }
    void setY(double y) { m_y = y; }

protected:
    double m_x;
    double m_y;
};

typedef list<NotationElement*> NotationElementList;


#endif
