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

#ifndef RG_TEMPOLISTITEM_H
#define RG_TEMPOLISTITEM_H

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "base/Event.h"

namespace Rosegarden {

class Composition;

class TempoListItem : public QTreeWidgetItem
{
public:
    enum Type { TimeSignature, Tempo };

    TempoListItem(Composition *composition,
		  Type type,
		  timeT time,
		  int index,
		  QTreeWidget *parent,
		QStringList labels
// 		  QString label1,
// 		  QString label2,
// 		  QString label3,
// 		  QString label4 = QString::null
		) :
	QTreeWidgetItem( parent, labels ), //label1, label2, label3, label4),
	m_composition(composition),
	m_type(type),
	m_time(time),
	m_index(index) { }

    Rosegarden::Composition *getComposition() { return m_composition; }
    Type getType() const { return m_type; }
    Rosegarden::timeT getTime() const { return m_time; }
    int getIndex() const { return m_index; }

    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

protected:
    Rosegarden::Composition *m_composition;
    Type m_type;
    Rosegarden::timeT m_time;
    int m_index;
};

}

#endif
