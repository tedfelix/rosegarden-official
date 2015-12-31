
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLPARAMETERITEM_H
#define RG_CONTROLPARAMETERITEM_H

#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>


namespace Rosegarden
{


class ControlParameterItem : public QTreeWidgetItem
{
public:
    ControlParameterItem(int id,
                         QTreeWidget *parent,
						 QStringList &strlist
//                          QString str1,
//                          QString str2,
//                          QString str3,
//                          QString str4,
//                          QString str5,
//                          QString str6,
//                          QString str7,
//                          QString str8,
//                          QString str9
		)
		: QTreeWidgetItem(parent, strlist),	//str1, str2, str3, str4, str5, str6, str7, str8),
        m_id(id) {
// 			setText(8, str9);
			setText(8, strlist[8]);
		}

    int getId() const { return m_id; }

protected:

    int     m_id;
    QString m_string9;
};


}

#endif
