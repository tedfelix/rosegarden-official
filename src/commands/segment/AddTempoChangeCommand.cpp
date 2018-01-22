/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddTempoChangeCommand.h"

#include "base/Composition.h"
#include <QString>


namespace Rosegarden
{

AddTempoChangeCommand::~AddTempoChangeCommand()
{
    // nothing here either
}

void
AddTempoChangeCommand::execute()
{
    int oldIndex = m_composition->getTempoChangeNumberAt(m_time);

    if (oldIndex >= 0) {
        std::pair<timeT, tempoT> data =
            m_composition->getTempoChange(oldIndex);

        if (data.first == m_time)
            m_oldTempo = data.second;
    }

    m_tempoChangeIndex = m_composition->addTempoAtTime(m_time, m_tempo, m_target);
}

void
AddTempoChangeCommand::unexecute()
{
    m_composition->removeTempoChange(m_tempoChangeIndex);

    if (m_oldTempo != 0) {
        m_composition->addTempoAtTime(m_time, m_oldTempo);
    }
}

}
