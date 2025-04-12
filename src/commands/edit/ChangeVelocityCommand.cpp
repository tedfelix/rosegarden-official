/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ChangeVelocityCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"

#include <QString>


namespace Rosegarden
{


ChangeVelocityCommand::ChangeVelocityCommand(
        int delta, EventSelection &selection, bool rounddelta) :
    BasicCommand(name(delta),
                 selection,
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_delta(delta),
    m_rounddelta(rounddelta)
{
}

void
ChangeVelocityCommand::modifySegment()
{
    EventContainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {

            long velocity = 100;
            (*i)->get<Int>(BaseProperties::VELOCITY, velocity);

            // round velocity up to the next multiple of delta
            if (m_rounddelta) {
                velocity /= m_delta;
                velocity *= m_delta;
                velocity += m_delta;
            } else {
                velocity += m_delta;
            }

            if (velocity < 0)
                velocity = 0;
            if (velocity > 127)
                velocity = 127;
            (*i)->set<Int>(BaseProperties::VELOCITY, velocity);
        }
    }
}

QString
ChangeVelocityCommand::name(int delta)
{
    if (delta > 0)
        return tr("&Increase Velocity");
    else
        return tr("&Reduce Velocity");
}


}
