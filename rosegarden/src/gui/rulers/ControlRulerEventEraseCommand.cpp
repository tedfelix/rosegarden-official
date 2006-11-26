/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include "ControlRulerEventEraseCommand.h"
#include "ControlItem.h"
#include "ElementAdapter.h"
#include "misc/Debug.h"
#include <klocale.h>

namespace Rosegarden
{

ControlRulerEventEraseCommand::ControlRulerEventEraseCommand(QCanvasItemList selectedItems,
                                                         Segment &segment,
                                                         Rosegarden::timeT start, Rosegarden::timeT end)
    : BasicCommand(i18n("Erase Controller Event(s)"),
                   segment,
                   start,
                   (start == end) ? start + 10 : end, 
                   true),
      m_selectedItems(selectedItems)
{
    RG_DEBUG << "ControllerEventEraseCommand : from " << start << " to " << end << endl;
}


void ControlRulerEventEraseCommand::modifySegment()
{
    Segment &segment(getSegment());

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            segment.eraseSingle(item->getElementAdapter()->getEvent());
    }
}

}
