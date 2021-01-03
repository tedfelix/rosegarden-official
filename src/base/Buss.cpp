/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Buss]"

#include "Buss.h"

#include "AudioPluginInstance.h"

#include <ostream>  // for std::endl
#include <sstream>  // for std::stringstream

namespace Rosegarden
{


Buss::Buss(BussId id) :
    PluginContainer(true),
    m_id(id),
    m_level(0.0),
    m_pan(100),
    m_mappedId(0)
{
}

std::string
Buss::toXmlString() const
{
    std::stringstream buss;

    buss << "    <buss id=\"" << m_id << "\">" << std::endl;
    buss << "       <pan value=\"" << (int)m_pan << "\"/>" << std::endl;
    buss << "       <level value=\"" << m_level << "\"/>" << std::endl;

    AudioPluginVector::const_iterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it) {
        buss << (*it)->toXmlString();
    }

    buss << "    </buss>" << std::endl;

    return buss.str();
}

std::string
Buss::getName() const
{
    char buffer[20];
    sprintf(buffer, "Submaster %d", m_id);
    return buffer;
}

std::string
Buss::getPresentationName() const
{
    return getName();
}

std::string
Buss::getAlias() const
{
    return getName();
}


}

