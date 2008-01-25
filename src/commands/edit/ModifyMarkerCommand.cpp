/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ModifyMarkerCommand.h"

#include "base/Composition.h"
#include <qstring.h>


namespace Rosegarden
{

ModifyMarkerCommand::ModifyMarkerCommand(Composition *comp,
        timeT time,
        timeT newTime,
        const std::string &name,
        const std::string &des):
        KNamedCommand(getGlobalName()),
        m_composition(comp),
        m_time(time),
        m_newTime(newTime),
        m_name(name),
        m_description(des),
        m_oldName(""),
        m_oldDescription("")
{}

ModifyMarkerCommand::~ModifyMarkerCommand()
{}

void
ModifyMarkerCommand::execute()
{
    Composition::markercontainer markers =
        m_composition->getMarkers();

    Composition::markerconstiterator it = markers.begin();

    for (; it != markers.end(); ++it) {
        if ((*it)->getTime() == m_time) {
            if (m_oldName.empty())
                m_oldName = (*it)->getName();
            if (m_oldDescription.empty())
                m_oldDescription = (*it)->getDescription();

            (*it)->setName(m_name);
            (*it)->setDescription(m_description);
            (*it)->setTime(m_newTime);
            return ;
        }
    }
}

void
ModifyMarkerCommand::unexecute()
{
    Composition::markercontainer markers =
        m_composition->getMarkers();

    Composition::markerconstiterator it = markers.begin();

    for (; it != markers.end(); ++it) {
        if ((*it)->getTime() == m_newTime) {
            (*it)->setName(m_oldName);
            (*it)->setDescription(m_oldDescription);
            (*it)->setTime(m_time);
        }
    }
}

}
