// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include <set>

#include "editviewbase.h"

#include "Segment.h"

class RosegardenGUIDoc;
class KListView;
class QListViewItem;
class QPushButton;
class QCheckBox;
class QButtonGroup;

class EventView : public EditViewBase, public Rosegarden::SegmentObserver
{
    Q_OBJECT

    // Event filters
    //
    enum EventFilter
    {
        None               = 0x0000,
        Note               = 0x0001,
        Rest               = 0x0002,
        Text               = 0x0004,
        SystemExclusive    = 0x0008,
        Controller         = 0x0010,
        ProgramChange      = 0x0020,
        PitchBend          = 0x0040,
        ChannelPressure    = 0x0080,
        KeyPressure        = 0x0100,
	Indication	   = 0x0200,
	Other		   = 0x0400
    };

public:
    EventView(RosegardenGUIDoc *doc,
              std::vector<Rosegarden::Segment *> segments,
              QWidget *parent);

    virtual ~EventView();

    static const char* const ConfigGroup;

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    virtual void updateView();

    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);

    // Set the button states to the current fileter positions
    //
    void setButtonsToFilter();

    // Menu creation and show
    //
    void createMenu();

public slots:

    // standard slots
    virtual void slotEditCut();
    virtual void slotEditCopy();
    virtual void slotEditPaste();

    // other edit slots
    void slotEditDelete();
    void slotEditInsert();
    void slotEditEvent();
    void slotEditEventAdvanced();

    void slotFilterSelection();
    void slotSelectAll();
    void slotClearSelection();

    // Show RMB menu
    //
    void slotPopupMenu(QListViewItem*, const QPoint&, int);
    void slotMenuActivated(int);

    // on double click on the event list
    //
    void slotPopupEventEditor(QListViewItem*);

    // Change filter parameters
    //
    void slotModifyFilter(int);

    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *) { }
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void endMarkerTimeChanged(const Rosegarden::Segment *, bool) { }
    virtual void segmentDeleted(const Rosegarden::Segment *) { }

protected slots:

    virtual void slotSaveOptions();

protected:

    virtual void readOptions();

    //--------------- Data members ---------------------------------
    KListView   *m_eventList;
    int          m_eventFilter;

    static int   m_lastSetEventFilter;

    QButtonGroup   *m_filterGroup;
    QCheckBox      *m_noteCheckBox;
    QCheckBox      *m_textCheckBox;
    QCheckBox      *m_sysExCheckBox;
    QCheckBox      *m_programCheckBox;
    QCheckBox      *m_controllerCheckBox;
    QCheckBox      *m_restCheckBox;
    QCheckBox      *m_pitchBendCheckBox;
    QCheckBox      *m_keyPressureCheckBox;
    QCheckBox      *m_channelPressureCheckBox;
    QCheckBox      *m_indicationCheckBox;
    QCheckBox      *m_otherCheckBox;

    static const char* const LayoutConfigGroupName;

    std::vector<int> m_listSelection;
    std::set<Rosegarden::Event *> m_deletedEvents; // deleted since last refresh

    QPopupMenu     *m_menu;

};

#endif

