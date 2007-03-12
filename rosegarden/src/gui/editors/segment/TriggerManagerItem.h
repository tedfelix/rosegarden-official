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

#ifndef _RG_TRIGGERMANAGERITEM_H_
#define _RG_TRIGGERMANAGERITEM_H_

#include <klistview.h>

#include "base/Event.h"
#include "base/TriggerSegment.h"

namespace Rosegarden {

class TriggerManagerItem : public QListViewItem
{
public:
    TriggerManagerItem(QListView * parent, QString label1, 
		      QString label2 = QString::null, 
		      QString label3 = QString::null,
		      QString label4 = QString::null, 
		      QString label5 = QString::null, 
		      QString label6 = QString::null, 
		      QString label7 = QString::null, 
		      QString label8 = QString::null):
        QListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8) { ; }

    virtual int compare(QListViewItem * i, int col, bool ascending) const;

    void setRawDuration(timeT raw) { m_rawDuration = raw; }
    timeT getRawDuration() const { return m_rawDuration; }

    void setId(TriggerSegmentId id) { m_id = id; }
    TriggerSegmentId getId() const { return m_id; }

    void setUsage(int usage) { m_usage = usage; }
    int getUsage() const { return m_usage; }

    void setPitch(int pitch) { m_pitch = pitch; }
    int getPitch() const { return m_pitch; }

protected:
    timeT m_rawDuration;
    TriggerSegmentId m_id;
    int m_usage;
    int m_pitch;
};

}

#endif
