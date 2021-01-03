/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
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
#include <QString>


namespace Rosegarden
{

ModifyMarkerCommand::ModifyMarkerCommand(Composition *comp,
        int id,
        timeT time,
        timeT newTime,
        const std::string &name,
        const std::string &des):
        NamedCommand(getGlobalName()),
        m_composition(comp),
        m_time(time),
        m_newTime(newTime),
        m_id(id),
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
        if ((*it)->getID() == m_id) {
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
        if ((*it)->getID() == m_id) {
            (*it)->setName(m_oldName);
            (*it)->setDescription(m_oldDescription);
            (*it)->setTime(m_time);
        }
    }
}

}
