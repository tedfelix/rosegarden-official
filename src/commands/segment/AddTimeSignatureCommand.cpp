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


#include "AddTimeSignatureCommand.h"

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include <QString>


namespace Rosegarden
{

AddTimeSignatureCommand::AddTimeSignatureCommand(Composition *composition,
        timeT time,
        TimeSignature timeSig) :
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_time(time),
        m_timeSignature(timeSig),
        m_oldTimeSignature(nullptr)
{
    // nothing else
}

AddTimeSignatureCommand::~AddTimeSignatureCommand()
{
    if (m_oldTimeSignature)
        delete m_oldTimeSignature;
}

void
AddTimeSignatureCommand::execute()
{
    int oldIndex = m_composition->getTimeSignatureNumberAt(m_time);
    if (oldIndex >= 0) {
        std::pair<timeT, TimeSignature> data =
            m_composition->getTimeSignatureChange(oldIndex);
        if (data.first == m_time) {
            m_oldTimeSignature = new TimeSignature(data.second);
        }
    }

    m_timeSigIndex = m_composition->addTimeSignature(m_time, m_timeSignature);
}

void
AddTimeSignatureCommand::unexecute()
{
    m_composition->removeTimeSignature(m_timeSigIndex);
    if (m_oldTimeSignature) {
        m_composition->addTimeSignature(m_time, *m_oldTimeSignature);
    }
}

}
