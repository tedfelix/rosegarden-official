// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include <qvbox.h>

#include <kdialogbase.h>

class PlayListView;
class QListViewItem;
class QDropEvent;
class QHBoxLayout;
class KConfig;

class PlayList : public QVBox
{
    Q_OBJECT

public:
    PlayList(QWidget *parent = 0, const char *name = 0);
    ~PlayList();

    PlayListView* getListView() { return m_listView; }

    void enableButtons(QListViewItem*);

    static const char* const PlayListConfigGroup;

signals:
    void play(QString);

protected slots:
    void slotOpenFiles();
    void slotPlay();
    void slotMoveUp();
    void slotMoveDown();
    void slotDeleteCurrent();
    void slotClear();
    void slotCurrentItemChanged(QListViewItem*);
    void slotDropped(QDropEvent*, QListViewItem*);

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


class PlayListDialog : public KDialogBase
{
    Q_OBJECT

public:
    PlayListDialog(QString caption, QWidget* parent = 0, const char* name = 0);

    PlayList* getPlayList() { return m_playList; }

public slots:
    void slotClose();

signals:
    void closing();

protected:    
    virtual void closeEvent(QCloseEvent *e);

    void save();
    void restore();

    //--------------- Data members ---------------------------------
    PlayList* m_playList;
};

#endif // _PLAYLIST_H_
