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

#ifndef RG_AUDIOLISTVIEW_H
#define RG_AUDIOLISTVIEW_H

#include <QTreeWidget>


namespace Rosegarden {
        
class AudioListView : public QTreeWidget
{
    Q_OBJECT
public:
    AudioListView(QWidget *parent = nullptr, const char *name = 0);

protected:
signals:
    
    void dropped(QDropEvent*, QTreeWidget*, const QList<QUrl>&);
    
protected:
//     bool acceptDrag(QDropEvent* e) const;

    
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent*) override;
    
    void mouseMoveEvent(QMouseEvent *event) override;
    
//     virtual QMimeData *mimeData(const QList<QTreeWidgetItem*> items) const;
    QStringList mimeTypes() const override;
    
};



}

#endif /*AUDIOLISTVIEW_H_*/
