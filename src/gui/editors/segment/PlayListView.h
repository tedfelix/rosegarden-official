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

#ifndef RG_PLAYLISTVIEW_H
#define RG_PLAYLISTVIEW_H

#include <QTreeWidget>
#include <QDropEvent>

namespace Rosegarden {

class PlayListView : public QTreeWidget
{
    Q_OBJECT
public:
    PlayListView(QWidget *parent=nullptr, const char *name=nullptr);

    QTreeWidgetItem* previousSibling(QTreeWidgetItem*);

protected: signals:
    void droppedURIs(QDropEvent*, QTreeWidget*, QStringList);
    
protected:
    
     void dragEnterEvent(QDragEnterEvent *event) override;
     void dropEvent(QDropEvent*) override;

//     virtual bool acceptDrag(QDropEvent*) const;
    
//     virtual void mousePressEvent ( QMouseEvent * event );
    void mouseMoveEvent(QMouseEvent *event) override;
    
//     virtual QMimeData *mimeData(const QList<QTreeWidgetItem*> items) const;
    QStringList mimeTypes() const override;
};

}

#endif

