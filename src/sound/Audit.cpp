/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Audit.h"

namespace Rosegarden
{

Audit::~Audit()
{
    std::string s = str();
    std::cerr << s;
    m_audit += s;
}

std::string
Audit::getAudit()
{
    return m_audit;
}

std::string Audit::m_audit;

}

