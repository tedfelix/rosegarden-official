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

#ifndef _PROGRESS_H_
#define _PROGRESS_H_

#include <iostream>

// An abstract progress framework.  Inherited by gui progress counting
// components this allows base/ or sound/ to interract without direct
// dependency.
//
//

namespace Rosegarden 
{

class Progress
{
public:
    Progress(int max):m_max(max), m_value(0) {;}
    virtual ~Progress() {;}

    int getMax() { return m_max; }
    int getValue() { return m_value; }

    // Set the progress bar value
    //
    virtual void set(int value) = 0;

    // Process GUI events
    //
    virtual void process() = 0;
    
protected:
    int m_max;
    int m_value;
};

}
 
#endif // _PROGRESS_H_
