// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "ViewElement.h"
#include <iostream>

namespace Rosegarden 
{

ViewElement::ViewElement(Event *e) :
    m_event(e)
{
    // nothing
}

ViewElement::~ViewElement()
{
    // nothing
}

//////////////////////////////////////////////////////////////////////

bool
operator<(const ViewElement &a, const ViewElement &b)
{
    timeT at = a.getViewAbsoluteTime(), bt = b.getViewAbsoluteTime();
    if (at == bt) return *(a.event()) < *(b.event());
    else return (at < bt);
}

//////////////////////////////////////////////////////////////////////


 
}

