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


#ifndef _TRIGGEREDITOR_H_
#define _TRIGGEREDITOR_H_

#include <kmainwindow.h>
#include <klistview.h>
#include <kdialogbase.h>

#include <qpushbutton.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qspinbox.h>

#include "timewidget.h"


class RosegardenGUIDoc;
class KCommand;
class MultiViewCommandHistory;
class QLabel;
class QAccel;

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
    QString makeDurationString(Rosegarden::timeT startTime,
			       Rosegarden::timeT duration, int timeMode);

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc        *m_doc;

    QPushButton             *m_closeButton;
    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;
    QPushButton             *m_deleteAllButton;

    KListView               *m_listView;

    bool                     m_modified;

    static const char* const TriggerManagerConfigGroup;

    QAccel *m_accelerators;
};

#endif // _TRIGGEREDITOR_H_

