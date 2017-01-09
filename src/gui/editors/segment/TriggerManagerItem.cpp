/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TriggerManagerItem.h"

namespace Rosegarden {

int
TriggerManagerItem::compare(QTreeWidgetItem * /* i */, int /* col */, bool /* ascending */) const
{
	return 1;
	//&&& FIX: disabled TriggerManagerItem::compare function
	
	/*
    TriggerManagerItem *ei = 
        dynamic_cast<TriggerManagerItem *>(i);

    if (!ei) return QTreeWidgetItem::compare(i, col, ascending);

    // col 0 -> index -- numeric compare
    // col 1 -> ID -- numeric compare
    // col 2 -> label -- default string compare
    // col 3 -> duration -- raw duration compare
    // col 4 -> base pitch -- pitch compare
    // col 5 -> base velocity -- numeric compare
    // col 6 -> usage count -- numeric compare
    //
    if (col == 2) {  
        return QTreeWidgetItem::compare(i, col, ascending);
    } else if (col == 3) {
        if (m_rawDuration < ei->getRawDuration()) return -1;
        else if (ei->getRawDuration() < m_rawDuration) return 1;
        else return 0;
    } else if (col == 4) {
        if (m_pitch < ei->getPitch()) return -1;
        else if (ei->getPitch() < m_pitch) return 1;
        else return 0;
    } else {
        return key(col, ascending).toInt() - i->key(col, ascending).toInt();
    }
	*/
}

}
