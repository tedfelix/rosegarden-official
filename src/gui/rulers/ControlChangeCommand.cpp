/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControlChangeCommand]"

#include "ControlChangeCommand.h"
#include "ControlItem.h"
#include "misc/Debug.h"

namespace Rosegarden {

ControlChangeCommand::ControlChangeCommand(ControlItemList selectedItems,
                                           Segment &segment,
                                           Rosegarden::timeT start, Rosegarden::timeT end)
    : BasicCommand(tr("Control Change"), segment,
            start, (start==end)?start+10:end, true),
      m_selectedItems(selectedItems)
{
    RG_DEBUG << "ControlChangeCommand : from " << start << " to " << end << endl;
}


void ControlChangeCommand::modifySegment()
{
    for (ControlItemList::iterator it = m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            item->updateSegment();
    }

    ///@TODO As this method can now add events, we need to use Normalise rests
}

}
