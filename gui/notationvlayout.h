
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

#ifndef NOTATIONVLAYOUT_H
#define NOTATIONVLAYOUT_H

#include "layoutengine.h"
#include "staff.h"
#include "notationelement.h"

/**
 * Vertical notation layout
 *
 * computes the Y coordinate of notation elements
 */

class NotationVLayout : public VerticalLayoutEngine
{
public:
    NotationVLayout();
    virtual ~NotationVLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset() { }

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Staff &) { }

    /**
     * Lay out a single staff.
     */
    virtual void scanStaff(Staff &staff);

    /**
     * Do any layout dependent on more than one staff.  As it
     * happens, we have none.
     */
    virtual void finishLayout() { }
};

#endif
