
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTVIEW_H
#define RG_EVENTVIEW_H

#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "gui/general/ListEditView.h"
#include "base/Event.h"

#include <set>
#include <vector>

#include <QSize>
#include <QString>


class QWidget;
class QMenu;
class QPoint;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QCheckBox;
class QGroupBox;
class QTreeWidget;


namespace Rosegarden
{

class Segment;
class RosegardenDocument;
class Event;


class EventView : public ListEditView, public SegmentObserver
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
        Other              = 0x0400,
        GeneratedRegion    = 0x0800,
        SegmentID          = 0x1000,
    };

public:
    EventView(RosegardenDocument *doc,
              std::vector<Segment *> segments,
              QWidget *parent);

    virtual ~EventView();

    void closeEvent(QCloseEvent *event);

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0);

    virtual void updateView();

    void setupActions();
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

    void slotSelectAll();
    void slotClearSelection();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    // Show RMB menu
    //
    void slotPopupMenu(const QPoint&);
    void slotMenuActivated(int);

    // on double click on the event list
    //
    void slotPopupEventEditor(QTreeWidgetItem*, int);

    // Change filter parameters
    //
    void slotModifyFilter();

    virtual void eventAdded(const Segment *, Event *) { }
    virtual void eventRemoved(const Segment *, Event *);
    virtual void endMarkerTimeChanged(const Segment *, bool) { }
    virtual void segmentDeleted(const Segment *);

    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void editTriggerSegment(int);

protected slots:
    virtual void slotSaveOptions();

    void slotEditTriggerName();
    void slotEditTriggerPitch();
    void slotEditTriggerVelocity();
    void slotTriggerTimeAdjustChanged(int);
    void slotTriggerRetuneChanged();

    /// slot connected to signal RosegardenDocument::setModified(bool)
    void updateWindowTitle(bool m = false);

protected:

    /// virtual function inherited from the base class, this implementation just
    /// calls updateWindowTitle() and avoids a refactoring job, even though
    /// updateViewCaption is superfluous
    virtual void updateViewCaption();

    virtual void readOptions();
    void makeInitialSelection(timeT);
    QString makeTimeString(timeT time, int timeMode);
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);
    virtual Segment *getCurrentSegment();

    //--------------- Data members ---------------------------------

    bool         m_isTriggerSegment;
    QLabel      *m_triggerName;
    QLabel      *m_triggerPitch;
    QLabel      *m_triggerVelocity;

    QTreeWidget *m_eventList;
    int          m_eventFilter;

    QGroupBox   *m_filterGroup;
    QCheckBox   *m_noteCheckBox;
    QCheckBox   *m_textCheckBox;
    QCheckBox   *m_sysExCheckBox;
    QCheckBox   *m_programCheckBox;
    QCheckBox   *m_controllerCheckBox;
    QCheckBox   *m_restCheckBox;
    QCheckBox   *m_pitchBendCheckBox;
    QCheckBox   *m_keyPressureCheckBox;
    QCheckBox   *m_channelPressureCheckBox;
    QCheckBox   *m_indicationCheckBox;
    QCheckBox   *m_generatedRegionCheckBox;
    QCheckBox   *m_segmentIDCheckBox;
    QCheckBox   *m_otherCheckBox;

    std::vector<int> m_listSelection;
    std::set<Event *> m_deletedEvents; // deleted since last refresh

    QMenu       *m_menu;

};


}

#endif
