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
#include <string>

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
    Progress(int max) : m_max(max), m_value(0) {;}
    virtual ~Progress() {;}

    int getMax() { return m_max; }
    int getValue() { return m_value; }

    // Set the name of the current operation
    //
    virtual void setOperationName(std::string) = 0;

    // Set the progress bar value - we have to use unusual naming
    // here to avoid clashes in future.
    //
    virtual void setCompleted(int value) = 0;

    // Add to the progress 
    //
    virtual void incrementCompletion(int value) {
	setCompleted(value + m_value);
    }

    // Process GUI events
    //
    virtual void processEvents() = 0;

    // Do the necessary to remove/kill/close this progress instance
    //
    virtual void done() = 0;

    // Return true if the operation has been cancelled
    //
    virtual bool wasOperationCancelled() { return false; }
    
protected:
    int m_max;
    int m_value;
};

}
 
#endif // _PROGRESS_H_
