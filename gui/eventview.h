// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef _EVENTVIEW_H_
#define _EVENTVIEW_H_

// Event view list - allow filtering by loads of different event types
// and allow access to the event editor dialog.
//
//


#include <vector>
#include <qlistview.h>

#include "editviewbase.h"

class RosegardenGUIDoc;
class KListView;
class QPushButton;


// EventView specialisation of a QListViewItem with the
// addition of a segment pointer
//
class EventViewItem : public QListViewItem
{
public:
    EventViewItem(Rosegarden::Segment *segment,
                  QListView *parent):QListViewItem(parent),
                                     m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListViewItem *parent):QListViewItem(parent),
                                         m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListView *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null)
        :QListViewItem(parent, label1, label2, label3, label4,
                       label5, label6, label7, label8), m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListViewItem *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null)
        :QListViewItem(parent, label1, label2, label3, label4,
                       label5, label6, label7, label8), m_segment(segment) {;}

    void setSegment(Rosegarden::Segment *segment) { m_segment = segment; }
    Rosegarden::Segment* getSegment() { return m_segment; }

protected:

    Rosegarden::Segment *m_segment;
};

class EventView : public EditViewBase
{
    Q_OBJECT

    // Event filters
    //
    enum EventFilter
    {
        None           = 0x0000,
        Note           = 0x0001,
        Rest           = 0x0002,
        Text           = 0x0004,
        SysEx          = 0x0008,
        Controller     = 0x0010,
        ProgramChange  = 0x0020
    };

public:
    EventView(RosegardenGUIDoc *doc,
              std::vector<Rosegarden::Segment *> segments,
              QWidget *parent);

    virtual ~EventView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    virtual void updateView();
    virtual void slotEditCut();
    virtual void slotEditCopy();
    virtual void slotEditPaste();
    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);

    // Set the button states to the current fileter positions
    //
    void setButtonsToFilter();

public slots:
    void slotNoteFilter(bool);
    void slotProgramFilter(bool);
    void slotControllerFilter(bool);
    void slotSysExFilter(bool);
    void slotTextFilter(bool);
    void slotRestFilter(bool);

    // on double click on the event list
    //
    void slotPopupEventEditor(QListViewItem*);

protected slots:

    virtual void slotSaveOptions();

protected:

    virtual void readOptions();

    //--------------- Data members ---------------------------------
    KListView   *m_eventList;
    int          m_eventFilter;

    QPushButton *m_noteFilter;
    QPushButton *m_textFilter;
    QPushButton *m_sysExFilter;
    QPushButton *m_programFilter;
    QPushButton *m_controllerFilter;
    QPushButton *m_restFilter;

    RosegardenGUIDoc *m_doc;
    static const char* const LayoutConfigGroupName;

};

#endif

