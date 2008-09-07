
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

#ifndef _RG_EVENTVIEW_H_
#define _RG_EVENTVIEW_H_

#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "gui/general/EditViewBase.h"
#include <set>
#include <QSize>
#include <QString>
#include <vector>
#include "base/Event.h"


class QWidget;
class QPopupMenu;
class QPoint;
class QListViewItem;
class QLabel;
class QCheckBox;
class QGroupBox;
class QListView;


namespace Rosegarden
{

class Segment;
class RosegardenGUIDoc;
class Event;


class EventView : public EditViewBase, public SegmentObserver
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
        Indication         = 0x0200,
        Other              = 0x0400
    };

public:
    EventView(RosegardenGUIDoc *doc,
              std::vector<Segment *> segments,
              QWidget *parent);

    virtual ~EventView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0);

    virtual void updateView();

    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);

    // Set the button states to the current filter positions
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

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

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

    virtual void eventAdded(const Segment *, Event *) { }
    virtual void eventRemoved(const Segment *, Event *);
    virtual void endMarkerTimeChanged(const Segment *, bool) { }
    virtual void segmentDeleted(const Segment *);

signals:
    void editTriggerSegment(int);

protected slots:
    virtual void slotSaveOptions();

    void slotEditTriggerName();
    void slotEditTriggerPitch();
    void slotEditTriggerVelocity();
    void slotTriggerTimeAdjustChanged(int);
    void slotTriggerRetuneChanged();

protected:

    virtual void readOptions();
    void makeInitialSelection(timeT);
    QString makeTimeString(timeT time, int timeMode);
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);
    virtual Segment *getCurrentSegment();

    virtual void updateViewCaption();

    //--------------- Data members ---------------------------------

    bool         m_isTriggerSegment;
    QLabel      *m_triggerName;
    QLabel      *m_triggerPitch;
    QLabel      *m_triggerVelocity;

    QListView   *m_eventList;
    int          m_eventFilter;

    static int   m_lastSetEventFilter;

    QGroupBox   *m_filterGroup;
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

    std::vector<int> m_listSelection;
    std::set<Event *> m_deletedEvents; // deleted since last refresh

    QPopupMenu     *m_menu;

};


}

#endif
