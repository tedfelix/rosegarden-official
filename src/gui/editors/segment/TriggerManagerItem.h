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

#ifndef RG_TRIGGERMANAGERITEM_H
#define RG_TRIGGERMANAGERITEM_H

#include <QTreeWidget>
#include <QTreeWidgetItem>


#include "base/Event.h"
#include "base/TriggerSegment.h"


namespace Rosegarden {

class TriggerManagerItem : public QTreeWidgetItem
{
public:
    TriggerManagerItem(QTreeWidget * parent, 
						QStringList labels
					  ):
        QTreeWidgetItem(parent, labels ){ ; }
	

    virtual int compare(QTreeWidgetItem * i, int col, bool ascending) const;

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
