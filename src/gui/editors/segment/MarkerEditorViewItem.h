/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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

#ifndef _RG_MARKEREDITORVIEWITEM_H_
#define _RG_MARKEREDITORVIEWITEM_H_

#include <klistview.h>

#include "base/Event.h"

namespace Rosegarden {


class MarkerEditorViewItem : public KListViewItem
{
public:
    MarkerEditorViewItem(QListView * parent, QString label1, 
                         QString label2 = QString::null, 
                         QString label3 = QString::null,
                         QString label4 = QString::null, 
                         QString label5 = QString::null, 
                         QString label6 = QString::null, 
                         QString label7 = QString::null, 
                         QString label8 = QString::null):
        KListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8),
	m_rawTime(0), m_fake(false) { ; }

    virtual int compare(QListViewItem * i, int col, bool ascending) const;

    void setRawTime(Rosegarden::timeT rawTime) { m_rawTime = rawTime; }
    Rosegarden::timeT getRawTime() const { return m_rawTime; }

    void setFake(bool fake) { m_fake = true; }
    bool isFake() const { return m_fake; }

protected:
    Rosegarden::timeT m_rawTime;
    bool m_fake;
};


}

#endif
