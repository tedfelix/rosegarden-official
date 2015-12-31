/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CycleSlashesCommand.h"

#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"


namespace Rosegarden
{

void
CycleSlashesCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;

        // skip everything except notes
        if (!(*i)->isa(Note::EventType)) continue;

        // get the number of existing slashes n
        if ((*i)->has(NotationProperties::SLASHES)) {
            (*i)->get<Int>(NotationProperties::SLASHES, n);
        }

        // unset any existing slashes
        if (n) (*i)->unset(NotationProperties::SLASHES);

        n++;

        if ((*i)->has(BaseProperties::IS_GRACE_NOTE) && (*i)->get<Bool>(BaseProperties::IS_GRACE_NOTE)) {
            // if this is a grace note, limit slash count to 1
            if (n > 1) n = 0;
        } else {
            // otherwise limit slash count to 5
            if (n > 5) n = 0;
        }

        // if n is still positive after rolling over, set the new slashes
        if (n) (*i)->set<Int>(NotationProperties::SLASHES, n);
    }
}

}
