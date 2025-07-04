/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRIGGERSEGMENTMANAGER_H
#define RG_TRIGGERSEGMENTMANAGER_H

#include "gui/general/ActionFileClient.h"

#include <QMainWindow>

class QWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QCloseEvent;


namespace Rosegarden
{


class Command;
class RosegardenDocument;


/// The "Manage Trigger Segments" dialog.
/**
 * This and EventListEditor work together to allow for
 * editing of all aspects of triggered segments.
 *
 * ??? This is awkward.  The Event List editor UI becomes a bloated mess
 *     when it is asked to edit a triggered Segment and it must show
 *     Label, Base Pitch, and Base Velocity.  TriggerSegmentManager
 *     should allow *editing* of the data it displays.  Then the Event
 *     List would not need to offer editing of triggered segment
 *     properties.
 */
class TriggerSegmentManager : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:

    TriggerSegmentManager(QWidget *parent,
                          RosegardenDocument *doc);
    ~TriggerSegmentManager() override;

    // reset the document
    void setDocument(RosegardenDocument *doc);

signals:

    void editTriggerSegment(int);
    void closing();

protected:

    void closeEvent(QCloseEvent *) override;

private slots:

    void slotUpdate();

    void slotAdd();
    void slotDelete();
    void slotDeleteAll();
    void slotClose();
    void slotEdit(QTreeWidgetItem *);
    void slotItemClicked(QTreeWidgetItem *);
    void slotPasteAsNew();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();
    void slotHelpRequested();
    void slotHelpAbout();

private:

    void setupActions();
    void initDialog();

    RosegardenDocument *m_doc;

    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_deleteAllButton;

    // ??? QTableWidget would make more sense.
    QTreeWidget *m_treeWidget;

    // ??? Set but never used for anything.  Get rid of this.
    bool m_modified{false};

    void setModified(bool modified);
    void addCommandToHistory(Command *command);

};


}

#endif
