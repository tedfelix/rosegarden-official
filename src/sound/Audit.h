/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIT_H
#define RG_AUDIT_H

#include <string>

#include <sstream>

// A staggeringly simple-minded audit trail implementation.

namespace Rosegarden {

class Audit : public std::stringstream
{
public:
    Audit() { }

    virtual ~Audit();

    static std::string getAudit();

protected:
    static std::string m_audit;
};

}

#endif
