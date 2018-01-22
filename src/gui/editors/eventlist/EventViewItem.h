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

#ifndef RG_EVENTVIEWITEM_H
#define RG_EVENTVIEWITEM_H

#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace Rosegarden
{

class Segment;
class Event;


// EventView specialisation of a QTreeWidgetItem with the
// addition of a segment pointer
//
class EventViewItem : public QTreeWidgetItem
{
public:
    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  QTreeWidget *parent
		): 
        QTreeWidgetItem(parent),
        m_segment(segment),
        m_event(event) {;}
    
    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  QTreeWidgetItem *parent
				): 
        QTreeWidgetItem(parent),
	    m_segment(segment),
	    m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  QTreeWidget *parent, 
				  QStringList &strings
				  
				  /*
				  // qt4 note: uses QStringList instead
				  QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null
				  */
				 ):
//		QTreeWidgetItem ( QTreeWidget * parent, const QStringList & strings, int type = Type )
        QTreeWidgetItem(parent, strings),
        m_segment(segment),
        m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  Rosegarden::Event *event,
                  QTreeWidgetItem *parent, 
				  QStringList &strings
				  
				  /*
				QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null
				 */
				 ):
		QTreeWidgetItem( parent, strings ), 
// 	   QTreeWidgetItem(parent, label1, label2, label3, label4,
// 					   label5, label6, label7, label8), 
		m_segment(segment),
        m_event(event) {;}

    Rosegarden::Segment* getSegment() { return m_segment; }
    Rosegarden::Event* getEvent() { return m_event; }

    // Reimplement so that we can sort numerically
    //
    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

protected:

    Rosegarden::Segment *m_segment;
    Rosegarden::Event *m_event;
};

}

#endif /*EVENTVIEWITEM_H_*/
