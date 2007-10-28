// -*- c-basic-offset: 4 -*-


/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _NOTATION_RULES_H_
#define _NOTATION_RULES_H_

namespace Rosegarden
{

/*
 * NotationRules.h
 *
 * This file contains the model for rules which are used in notation decisions.
 *
 */

class NotationRules
{
public:
    NotationRules() { };
    ~NotationRules() { };

    /**
     * If a single note is on the middle line, the preferred direction is down.
     */
    bool isStemUp(int heightOnStaff) { return heightOnStaff < 4; }

    /**
     * If the extreme notes in a chord are an equal distance from the middle line,
     * the preferred direction is down.
     */
    bool isStemUp(int highestHeightOnStaff, int lowestHeightOnStaff) {
        return (highestHeightOnStaff + lowestHeightOnStaff) < 2*4;
    }
};

}

#endif
