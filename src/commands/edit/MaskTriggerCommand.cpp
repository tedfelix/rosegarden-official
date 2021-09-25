/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MaskTriggerCommand.h"

#include "base/BaseProperties.h"
#include "base/NotationTypes.h"


namespace Rosegarden
{


void
MaskTriggerCommand::modifySegment()
{

    // For each Event in the selection
    for (EventContainer::iterator i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end();
         ++i) {

        // We don't check for TRIGGER_SEGMENT_ID because the tied notes
        // won't have it.

        // Set TRIGGER_EXPAND appropriately for each Note event.
        if ((*i)->isa(Note::EventType))
            (*i)->set<Bool>(BaseProperties::TRIGGER_EXPAND, m_sounding);
    }

}

QString
MaskTriggerCommand::getGlobalName(bool sounding)
{
  if (sounding)
      return tr("&Unmask Ornament");
  else
      return tr("&Mask Ornament");
}


}
