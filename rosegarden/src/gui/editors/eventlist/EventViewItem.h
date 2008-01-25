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

#ifndef _RG_EVENTVIEWITEM_H_
#define _RG_EVENTVIEWITEM_H_

#include <klistview.h>

namespace Rosegarden
{

class Segment;
class Event;

// EventView specialisation of a QListViewItem with the
// addition of a segment pointer
//
class EventViewItem : public KListViewItem
{
public:
    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  KListView *parent) : 
        KListViewItem(parent),
        m_segment(segment),
        m_event(event) {;}
    
    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  KListViewItem *parent) : 
        KListViewItem(parent),
        m_segment(segment),
        m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  QListView *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null) :
        KListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8),
        m_segment(segment),
        m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  KListViewItem *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null) :
        KListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8), 
        m_segment(segment),
        m_event(event) {;}

    Rosegarden::Segment* getSegment() { return m_segment; }
    Rosegarden::Event* getEvent() { return m_event; }

    // Reimplement so that we can sort numerically
    //
    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:

    Rosegarden::Segment *m_segment;
    Rosegarden::Event *m_event;
};

}

#endif /*EVENTVIEWITEM_H_*/
