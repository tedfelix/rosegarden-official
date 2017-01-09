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

#include "MarkerEditorViewItem.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>


namespace Rosegarden {

int
MarkerEditorViewItem::compare(QTreeWidgetItem * /* i */, int /* col */, bool /* ascending */) const
{
	return 0;
	//MarkerEditorViewItem::compare //&&& FIX : QTreeWidgetItem::compare does not exist
	
	/*
    MarkerEditorViewItem *ei = 
        dynamic_cast<MarkerEditorViewItem *>(i);

    if (!ei) return QTreeWidgetItem::compare(i, col, ascending);

    // Raw time sorting on time column
    //
    if (col == 0) {  

        if (m_rawTime < ei->getRawTime()) return -1;
        else if (ei->getRawTime() < m_rawTime) return 1;
        else return 0;

    } else {
        return QTreeWidgetItem::compare(i, col, ascending);
    }
	*/
}

}

