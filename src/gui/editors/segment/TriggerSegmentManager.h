
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRIGGERSEGMENTMANAGER_H_
#define _RG_TRIGGERSEGMENTMANAGER_H_

#include <kmainwindow.h>
#include <QString>
#include "base/Event.h"


class QWidget;
class QPushButton;
class QListViewItem;
class QCloseEvent;
class QShortcut;
class KListView;
class Command;


namespace Rosegarden
{

class RosegardenGUIDoc;
class MultiViewCommandHistory;


class TriggerSegmentManager : public KMainWindow
{
    Q_OBJECT

public:
    TriggerSegmentManager(QWidget *parent,
                          RosegardenGUIDoc *doc);
    ~TriggerSegmentManager();

    void initDialog();

    void addCommandToHistory(Command *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    QShortcut* getShortcuterators() { return m_shortcuterators; }

public slots:
    void slotUpdate();

    void slotAdd();
    void slotDelete();
    void slotDeleteAll();
    void slotClose();
    void slotEdit(QListViewItem *);
    void slotItemClicked(QListViewItem *);
    void slotPasteAsNew();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

signals:
    void editTriggerSegment(int);
    void closing();

protected:
    virtual void closeEvent(QCloseEvent *);

    void setupActions();
    QString makeDurationString(timeT startTime,
                               timeT duration, int timeMode);

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc        *m_doc;

    QPushButton             *m_closeButton;
    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;
    QPushButton             *m_deleteAllButton;

    KListView               *m_listView;

    bool                     m_modified;

    QShortcut *m_shortcuterators;
};


}

#endif
