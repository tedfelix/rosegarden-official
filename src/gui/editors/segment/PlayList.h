
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PLAYLIST_H
#define RG_PLAYLIST_H

#include <QWidget>


class QWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QHBoxLayout;
class QFrame;
class QDropEvent;


namespace Rosegarden
{

class PlayListView;


class PlayList : public QWidget
{
    Q_OBJECT

public:
    PlayList(QWidget *parent = nullptr);
    ~PlayList() override;

    PlayListView* getListView() { return m_listView; }

    void enableButtons(QTreeWidgetItem*);


signals:
    void play(QString);

protected slots:
    void slotOpenFiles();
    void slotPlay();
    void slotMoveUp();
    void slotMoveDown();
    void slotDeleteCurrent();
    void slotClear();
    void slotCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
//     void slotDropped(QDropEvent*, QTreeWidgetItem*);
    void slotDroppedURIs(QDropEvent* ev, QTreeWidget*, QStringList sl);

protected:
    void save();
    void restore();

    //--------------- Data members ---------------------------------
    PlayListView* m_listView;
    QFrame* m_buttonBar;
    QHBoxLayout* m_barLayout;

    QPushButton* m_openButton;
    QPushButton* m_playButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    QPushButton* m_deleteButton;
    QPushButton* m_clearButton;
};



}

#endif
