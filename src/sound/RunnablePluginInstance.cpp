/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RunnablePluginInstance.h"
#include "PluginFactory.h"


namespace Rosegarden
{

RunnablePluginInstance::~RunnablePluginInstance()
{
//    std::cerr << "RunnablePluginInstance::~RunnablePluginInstance" << std::endl;

    if (m_factory) {
//        std::cerr << "Asking factory to release " << m_identifier << std::endl;

        m_factory->releasePlugin(this, m_identifier);
    }
}

}

