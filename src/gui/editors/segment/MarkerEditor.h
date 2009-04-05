
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

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

#include "gui/general/ActionFileClient.h"

#include <QMainWindow>
#include <QString>
#include <QModelIndex>

#include "base/Event.h"


class QWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QCloseEvent;
class QShortcut;


namespace Rosegarden
{

class Command;
class RosegardenDocument;


class MarkerEditor : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    MarkerEditor(QWidget *parent,
                       RosegardenDocument *doc);
    ~MarkerEditor();

    void initDialog();

    void addCommandToHistory(Command *command);

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenDocument *doc);

    // update pointer position
    void updatePosition();

    QShortcut* getShortcuts() { return m_shortcuts; }

public slots:
    void slotUpdate();

    void slotAdd();
    void slotDelete();
    void slotDeleteAll();
    void slotClose();
    void slotEdit(QTreeWidgetItem *, int);
    //@@@ The following generates:
    //
    // Object::connect: No such signal QTreeWidget::pressed(QTreeWidgetItem*)
    //
    // There is no pressed(QTreeWidgetItem *) signal under QTreeWidget.  Near as
    // I can tell, it inherits 
    //
    // void pressed ( const QModelIndex & index )
    //
    // from QAbstractViewItem at least.  I'm not sure how to adapt this, or even
    // what this particular bit of code is for.  I took a quick stab at it, and
    // didn't make any progress, so I'm leaving it broken for now, with this
    // bigass comment to draw somebody's attention to the broken signal
    void slotItemClicked(QTreeWidgetItem *);

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
    RosegardenDocument        *m_doc;

    QLabel                  *m_absoluteTime;
    QLabel                  *m_realTime;
    QLabel                  *m_barTime;

    QPushButton             *m_closeButton;


    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;
    QPushButton             *m_deleteAllButton;

    QTreeWidget               *m_listView;

    bool                     m_modified;

    QShortcut *m_shortcuts;
};


}

#endif
