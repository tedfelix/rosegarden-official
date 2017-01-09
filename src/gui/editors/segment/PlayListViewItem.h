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

#ifndef RG_PLAYLISTVIEWITEM_H
#define RG_PLAYLISTVIEWITEM_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
// #include <kurl.h>

namespace Rosegarden {

class PlayListViewItem : public QTreeWidgetItem
{
public:
    PlayListViewItem(QTreeWidget* parent, QUrl );
    PlayListViewItem(QTreeWidget* parent, QTreeWidgetItem*, QUrl );
	
// 	const KURL& getURL() { return m_kurl; }
	const QUrl& getURL() { return m_kurl; }

protected:
// 	KURL m_kurl;
	QUrl m_kurl;
	
    /*
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseMoveEvent(QMouseEvent *event);
    
    virtual QStringList mimeTypes () const;
    virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
    */
};

}

#endif
