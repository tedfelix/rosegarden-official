// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "LayoutEngine.h"
#include "Staff.h"
#include "notationelement.h"
#include "FastVector.h"

class NotationStaff;

/**
 * Vertical notation layout
 *
 * computes the Y coordinate of notation elements
 */

class NotationVLayout : public Rosegarden::VerticalLayoutEngine<NotationElement>
{
public:
    NotationVLayout(Rosegarden::Composition *c);
    virtual ~NotationVLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &,
			    Rosegarden::timeT = 0,
			    Rosegarden::timeT = 0);

    /**
     * Lay out a single staff.
     */
    virtual void scanStaff(StaffType &,
			   Rosegarden::timeT = 0,
			   Rosegarden::timeT = 0);

    /**
     * Do any layout dependent on more than one staff.  As it
     * happens, we have none, but we do have some layout that
     * depends on the final results from the horizontal layout
     * (for slurs), so we should do that here
     */
    virtual void finishLayout(Rosegarden::timeT = 0,
			      Rosegarden::timeT = 0);

private:

    void positionSlur(NotationStaff &staff, NotationElementList::iterator i);

    typedef FastVector<NotationElementList::iterator> SlurList;
    typedef std::map<StaffType *, SlurList> SlurListMap;

    const Rosegarden::Quantizer *getQuantizer();

    SlurListMap m_slurs;
    SlurList &getSlurList(StaffType &);

    Rosegarden::Composition *m_composition;
};

#endif
