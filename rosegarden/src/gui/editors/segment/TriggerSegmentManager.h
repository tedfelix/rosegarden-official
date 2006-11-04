
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_TRIGGERSEGMENTMANAGER_H_
#define _RG_TRIGGERSEGMENTMANAGER_H_

#include <kmainwindow.h>
#include <qstring.h>
#include "base/Event.h"
#include "document/ConfigGroups.h"


class QWidget;
class QPushButton;
class QListViewItem;
class QCloseEvent;
class QAccel;
class KListView;
class KCommand;


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

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    QAccel* getAccelerators() { return m_accelerators; }

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

    QAccel *m_accelerators;
};


}

#endif
