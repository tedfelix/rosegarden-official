
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

#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

// Layout stuff
#include <algorithm>

#include "Event.h"
#include "notationelement.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Richard Bown
  */

class LayoutEngine
{
public: 
    LayoutEngine();
    virtual ~LayoutEngine();

    unsigned int status() const { return m_status; }

protected:
    unsigned int m_status;
};

class NotationLayout : public LayoutEngine, public std::unary_function<NotationElement*, void>
{
public:

    void operator() (NotationElement *el) { layout(el); }

protected:
    virtual void layout(NotationElement*) = 0;
};

#endif
