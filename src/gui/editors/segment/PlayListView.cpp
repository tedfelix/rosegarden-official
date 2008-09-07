/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlayListView.h"

#include <klocale.h>
#include <qdragobject.h>

namespace Rosegarden {

PlayListView::PlayListView(QWidget *parent, const char *name)
    : QListView(parent, name)
{
    addColumn(i18n("Title"));
    addColumn(i18n("File name"));

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropVisualizer(true);

    setShowToolTips(true);
    setShowSortIndicator(true);
    setAllColumnsShowFocus(true);
    setItemsMovable(true);
    setSorting(-1);
}

bool PlayListView::acceptDrag(QDropEvent* e) const
{
    return QUriDrag::canDecode(e) || QListView::acceptDrag(e);
}


QListViewItem* PlayListView::previousSibling(QListViewItem* item)
{
    QListViewItem* prevSib = firstChild();

    while(prevSib && prevSib->nextSibling() != item)
        prevSib = prevSib->nextSibling();

    return prevSib;
}

}

