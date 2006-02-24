// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _AUDIT_H_
#define _AUDIT_H_

#include <string>
#include <iostream>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

// A staggeringly simple-minded audit trail implementation.

namespace Rosegarden {

class Audit : public std::stringstream
{
public:
    Audit() { }

    virtual ~Audit() {
#if (__GNUC__ < 3)
	*this << std::ends;
#endif
	std::cerr << str();
	m_audit += str();
    }

    static std::string getAudit() { return m_audit; }

protected:
    static std::string m_audit;
};

}

#endif
