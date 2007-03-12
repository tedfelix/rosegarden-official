/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include "EventViewItem.h"
#include "base/Event.h"

namespace Rosegarden
{

// Reimplementation of sort for numeric columns - taking the
// right hand argument from the left is equivalent to the
// the QString compare().
//
int
EventViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    EventViewItem *ei = dynamic_cast<EventViewItem *>(i);
    if (!ei) return QListViewItem::compare(i, col, ascending);

    if (col == 0) { // time
        Rosegarden::Event &e1 = *m_event;
        Rosegarden::Event &e2 = *ei->m_event;
        if (e2 < e1) return 1;
        else if (e1 < e2) return -1;
        else return 0;
    } else if (col == 2 || col == 5 || col == 6) { // event type, data1, data2
        // we have to do string compares even for data1/data2 which are
        // often numeric, just because they aren't _always_ numeric and
        // we don't want to prevent the user being able to separate
        // e.g. crescendo from decrescendo
        if (key(col, ascending).compare(i->key(col, ascending)) == 0) {
            return compare(i, 0, ascending);
        } else {
            return key(col, ascending).compare(i->key(col, ascending));
        }
    } else if (col == 3) { // pitch
        // numeric comparison for pitch used to work when we only
        // showed the numeric pitch number, but then we added the MIDI
        // pitch name as well and that broke plain numeric comparison
        return key(col, ascending).section(' ', 0, 0).toInt() -
            i->key(col, ascending).section(' ', 0, 0).toInt();
    } else {               // numeric comparison
        return key(col, ascending).toInt() - i->key(col, ascending).toInt();
    }
}

}
