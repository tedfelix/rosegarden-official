
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

#ifndef _RG_MARKEREDITOR_H_
#define _RG_MARKEREDITOR_H_

#include <kmainwindow.h>
#include <QString>
#include "base/Event.h"


class QWidget;
class QPushButton;
class QListWidgetItem;
class QLabel;
class QCloseEvent;
class QShortcut;
class QListWidget;
class Command;


namespace Rosegarden
{

class RosegardenGUIDoc;
class MultiViewCommandHistory;


class MarkerEditor : public KMainWindow
{
    Q_OBJECT

public:
    MarkerEditor(QWidget *parent,
                       RosegardenGUIDoc *doc);
    ~MarkerEditor();

    void initDialog();

    void addCommandToHistory(Command *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    // update pointer position
    void updatePosition();

    QShortcut* getShortcuterators() { return m_shortcuterators; }

public slots:
    void slotUpdate();

    void slotAdd();
    void slotDelete();
    void slotDeleteAll();
    void slotClose();
    void slotEdit(QListWidgetItem *);
    void slotItemClicked(QListWidgetItem *);

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

signals:
    void closing();
    void jumpToMarker(timeT);

protected:
    virtual void closeEvent(QCloseEvent *);

    void setupActions();
    QString makeTimeString(timeT time, int timeMode);

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc        *m_doc;

    QLabel                  *m_absoluteTime;
    QLabel                  *m_realTime;
    QLabel                  *m_barTime;

    QPushButton             *m_closeButton;


    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;
    QPushButton             *m_deleteAllButton;

    QListWidget               *m_listView;

    bool                     m_modified;

    QShortcut *m_shortcuterators;
};


}

#endif
