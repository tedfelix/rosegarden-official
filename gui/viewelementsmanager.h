
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef VIEWELEMENTSMANAGER_H
#define VIEWELEMENTSMANAGER_H

#include "Event.h"
#include "notationelement.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class ViewElementsManager
{
public: 
    ViewElementsManager(Track&);
    ~ViewElementsManager();

    NotationElementList* notationElementList(Track::iterator from,
                                             Track::iterator to);

    // overload these for each ViewElement type
    void insert(NotationElement*);
    void erase(NotationElementList::iterator);

protected:

    Track               &m_track;
    NotationElementList *m_notationElements;
    
};

#endif
