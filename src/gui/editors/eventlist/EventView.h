
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

    ~EventView() override;

    void closeEvent(QCloseEvent *event) override;

    virtual bool applyLayout(int staffNo = -1);

    void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0) override;

    void updateView() override;

    void setupActions();
    void initStatusBar() override;
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
    void slotEditCut() override;
    void slotEditCopy() override;
    void slotEditPaste() override;

    // other edit slots
    void slotEditDelete();
    void slotEditInsert();

    void slotSelectAll();
    void slotClearSelection();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    // Change filter parameters
    //
    void slotModifyFilter();

    void eventAdded(const Segment *, Event *) override { }
    void eventRemoved(const Segment *, Event *) override;
    void endMarkerTimeChanged(const Segment *, bool) override { }
    void segmentDeleted(const Segment *) override;

    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void editTriggerSegment(int);

private slots:
    void slotSaveOptions() override;

    // Menu Handlers

    /// Edit > Edit Event
    void slotEditEvent();
    /// Edit > Advanced Event Editor
    void slotEditEventAdvanced();

    /// Handle double-click on an event in the event list.
    void slotPopupEventEditor(QTreeWidgetItem *item, int column);

    /// Right-click context menu.
    void slotPopupMenu(const QPoint&);
    /// Right-click context menu handler.
    void slotOpenInEventEditor(bool checked);
    /// Right-click context menu handler.
    void slotOpenInExpertEventEditor(bool checked);

    void slotEditTriggerName();
    void slotEditTriggerPitch();
    void slotEditTriggerVelocity();
    void slotTriggerTimeAdjustChanged(int);
    void slotTriggerRetuneChanged();

    /// slot connected to signal RosegardenDocument::setModified(bool)
    void updateWindowTitle(bool m = false);

private:

    /// virtual function inherited from the base class, this implementation just
    /// calls updateWindowTitle() and avoids a refactoring job, even though
    /// updateViewCaption is superfluous
    void updateViewCaption() override;

    void readOptions() override;
    void makeInitialSelection(timeT);
    QString makeTimeString(timeT time, int timeMode);
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);
    Segment *getCurrentSegment() override;

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
