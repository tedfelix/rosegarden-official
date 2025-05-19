/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKEREDITORVIEWITEM_H
#define RG_MARKEREDITORVIEWITEM_H

#include "base/TimeT.h"

#include <QTreeWidgetItem>
#include <QStringList>

class QTreeWidget;


namespace Rosegarden
{


class MarkerEditorViewItem : public QTreeWidgetItem
{
public:
	MarkerEditorViewItem(QTreeWidget *parent, int id, QStringList strlist):
        QTreeWidgetItem(parent, strlist),
        m_id(id)
	{
	}

    virtual int compare(QTreeWidgetItem * i, int col, bool ascending) const;

    void setRawTime(timeT rawTime) { m_rawTime = rawTime; }
    timeT getRawTime() const { return m_rawTime; }

    void setFake(bool fake) { m_fake = fake; }
    bool isFake() const { return m_fake; }

    int getID() const { return m_id; }
    
protected:
    timeT m_rawTime{0};
    bool m_fake{false};
    int m_id;
};


}

#endif
