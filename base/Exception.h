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

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>

namespace Rosegarden {

class Exception
{
public:
    Exception(std::string message);
    Exception(std::string message, std::string file, int line);

    std::string getMessage() const { return m_message; }

private:
    std::string m_message;
};


}

#endif
