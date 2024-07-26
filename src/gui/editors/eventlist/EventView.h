
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#include "base/Segment.h"
#include "gui/general/ListEditView.h"

#include <set>
#include <vector>

#include <QString>


class QWidget;
class QMenu;
class QPoint;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QCheckBox;
class QGroupBox;


namespace Rosegarden
{


class RosegardenDocument;
class Event;


/// The "Event List" window.
/**
 * This is the event list editor.
 *
 * Segment > Edit With > Open in Event List Editor.  Or just press "E".
 */
class EventView : public ListEditView, public SegmentObserver
{
    Q_OBJECT

public:

    EventView(RosegardenDocument *doc,
              const std::vector<Segment *> &segments);

    ~EventView() override;

signals:

    /// Connected to RosegardenMainViewWidget::slotEditTriggerSegment().
    void editTriggerSegment(int);

protected slots:

    // EditViewBase overrides.
    void slotEditCut() override;
    void slotEditCopy() override;
    void slotEditPaste() override;

protected:

    void initStatusBar();

    Segment *getCurrentSegment() override;

    // ListEditView overrides.
    void refreshSegment(Segment *segment,
                        timeT startTime = 0,
                        timeT endTime = 0) override;
    void updateView() override;

    // SegmentObserver overrides.
    void eventAdded(const Segment *, Event *) override { }
    void eventRemoved(const Segment *, Event *) override;
    void endMarkerTimeChanged(const Segment *, bool) override { }
    void segmentDeleted(const Segment *) override;

    // QWidget override.
    void closeEvent(QCloseEvent *event) override;

private slots:

    void slotEditDelete();
    void slotEditInsert();

    void slotSelectAll();
    void slotClearSelection();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    void slotModifyFilter();

    void slotHelpRequested();
    void slotHelpAbout();

    // Menu Handlers

    /// Edit > Edit Event
    void slotEditEvent();
    /// Edit > Advanced Event Editor
    void slotEditEventAdvanced();

    /// Handle double-click on an event in the event list.
    void slotPopupEventEditor(QTreeWidgetItem *item, int column);

    /// Right-click context menu.
    void slotPopupMenu(const QPoint &);
    /// Right-click context menu handler.
    void slotOpenInEventEditor(bool checked);
    /// Right-click context menu handler.
    void slotOpenInExpertEventEditor(bool checked);

    void slotEditTriggerName();
    void slotEditTriggerPitch();
    void slotEditTriggerVelocity();
    // unused void slotTriggerTimeAdjustChanged(int);
    // unused void slotTriggerRetuneChanged();

    /// slot connected to signal RosegardenDocument::setModified(bool)
    /**
     * ??? Rename: slotUpdateWindowTitle() like all others.
     */
    void updateWindowTitle(bool modified);

private:

    /// Create and show popup menu.
    void createMenu();

    /// Set the button states to the current filter positions
    void setButtonsToFilter();

    void setupActions();

    bool updateTreeWidget();

    QString makeTimeString(timeT time, int timeMode);
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);

    // Event filters
    QGroupBox *m_filterGroup;
    QCheckBox *m_noteCheckBox;
    QCheckBox *m_programCheckBox;
    QCheckBox *m_controllerCheckBox;
    QCheckBox *m_pitchBendCheckBox;
    QCheckBox *m_sysExCheckBox;
    QCheckBox *m_keyPressureCheckBox;
    QCheckBox *m_channelPressureCheckBox;
    QCheckBox *m_restCheckBox;
    QCheckBox *m_indicationCheckBox;
    QCheckBox *m_textCheckBox;
    // ??? What is this?
    QCheckBox *m_generatedRegionCheckBox;
    // ??? What is this?
    QCheckBox *m_segmentIDCheckBox;
    QCheckBox *m_otherCheckBox;

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
    int m_eventFilter{0x1FFF};

    // ??? Why is this a tree widget?  When are there ever sub-nodes?
    QTreeWidget *m_eventList;

    std::vector<int> m_listSelection;
    void makeInitialSelection(timeT);

    std::set<Event *> m_deletedEvents; // deleted since last refresh

    // Pop-up menu for the event list.
    QMenu *m_menu{nullptr};

    bool m_isTriggerSegment{false};
    QLabel *m_triggerName{nullptr};
    QLabel *m_triggerPitch{nullptr};
    QLabel *m_triggerVelocity{nullptr};

    void readOptions();
    void saveOptions();

};


}

#endif
