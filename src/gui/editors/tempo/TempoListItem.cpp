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

#include "TempoListItem.h"

namespace Rosegarden {

int
TempoListItem::compare(QTreeWidgetItem */* i */, int /* col */, bool /* ascending */) const
{
	//&&& FIX: disabled TempoListItem::compare function
	return 1;
	
	/*
    TempoListItem *ti = dynamic_cast<TempoListItem *>(i);
    if (!ti) return QTreeWidgetItem::compare(i, col, ascending);

    if (col == 0) { // time
	if (m_time == ti->m_time) {
	    return int(m_type) - int(ti->m_type);
	} else {
	    return int(m_time - ti->m_time);
	}
    } else if (col == 1) { // type
	if (m_type == ti->m_type) {
	    return int(m_time - ti->m_time);
	} else {
	    return int(m_type) - int(ti->m_type);
	}
    } else {
	return key(col, ascending).compare(i->key(col, ascending));
    }
	*/
}

}
