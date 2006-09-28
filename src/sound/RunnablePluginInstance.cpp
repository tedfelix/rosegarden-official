// -*- c-basic-offset: 4 -*-

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

#include "RunnablePluginInstance.h"
#include "PluginFactory.h"

#include <iostream>

namespace Rosegarden
{

RunnablePluginInstance::~RunnablePluginInstance()
{
    std::cerr << "RunnablePluginInstance::~RunnablePluginInstance" << std::endl;

    if (m_factory) {
        std::cerr << "Asking factory to release " << m_identifier << std::endl;

        m_factory->releasePlugin(this, m_identifier);
    }
}

}

