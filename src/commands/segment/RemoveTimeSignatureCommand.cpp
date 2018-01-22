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


#include "RemoveTimeSignatureCommand.h"

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include <QString>


namespace Rosegarden
{

void
RemoveTimeSignatureCommand::execute()
{
    if (m_timeSigIndex >= 0) {
        std::pair<timeT, TimeSignature> data =
            m_composition->getTimeSignatureChange(m_timeSigIndex);

        // store
        m_oldTime = data.first;
        m_oldTimeSignature = data.second;
    }

    // do we need to (re)store the index number?
    //
    m_composition->removeTimeSignature(m_timeSigIndex);

}

void
RemoveTimeSignatureCommand::unexecute()
{
    m_composition->addTimeSignature(m_oldTime, m_oldTimeSignature);
}

}
