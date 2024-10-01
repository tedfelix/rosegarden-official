
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

#include "base/Segment.h"  // SegmentObserver
#include "gui/general/EditViewBase.h"

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
class QFrame;
class QGridLayout;


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
class EventView : public EditViewBase, public SegmentObserver
{
    Q_OBJECT

public:

    EventView(RosegardenDocument *doc,
              const std::vector<Segment *> &segments);

    ~EventView() override;

    // SegmentObserver overrides.
    void eventRemoved(const Segment *, Event *) override;
    void segmentDeleted(const Segment *) override;

signals:

    /// Connected to RosegardenMainViewWidget::slotEditTriggerSegment().
    /**
     * ??? When is this ever emitted?  The other editors notice the user has
     *     asked to edit a trigger Segment and then emit this.  This never
     *     emits.
     */
    void editTriggerSegment(int id);

protected:

    // EditViewBase override.
    Segment *getCurrentSegment() override;

    // QWidget override.
    void closeEvent(QCloseEvent *event) override;

private slots:

    // Menu Handlers

    // Edit
    void slotEditInsert();
    void slotEditDelete();
    /// Edit > Edit Event
    void slotEditEvent();
    /// Edit > Advanced Event Editor
    void slotEditEventAdvanced();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotSelectAll();
    void slotClearSelection();

    // View
    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    // Help
    void slotHelpRequested();
    void slotHelpAbout();

    /// Filter check boxes.
    void slotModifyFilter();

    /// Handle double-click on an event in the event list.
    void slotPopupEventEditor(QTreeWidgetItem *item, int column);

    /// Right-click context menu.
    void slotPopupMenu(const QPoint &);
    /// Right-click context menu handler.
    void slotOpenInEventEditor(bool checked);
    /// Right-click context menu handler.
    void slotOpenInExpertEventEditor(bool checked);

    // Trigger Segments
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

    void slotDocumentModified(bool modified);

private:

    QFrame *m_frame{nullptr};
    QGridLayout *m_gridLayout{nullptr};

    /// Set the button states to the current filter positions
    void setButtonsToFilter();

    void setupActions();

    // Widgets

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

    // Filter bitmask.
    // ??? Consider individual bools.
    static constexpr int Note            = 0x0001;
    static constexpr int Rest            = 0x0002;
    static constexpr int Text            = 0x0004;
    static constexpr int SystemExclusive = 0x0008;
    static constexpr int Controller      = 0x0010;
    static constexpr int ProgramChange   = 0x0020;
    static constexpr int PitchBend       = 0x0040;
    static constexpr int ChannelPressure = 0x0080;
    static constexpr int KeyPressure     = 0x0100;
    static constexpr int Indication      = 0x0200;
    static constexpr int Other           = 0x0400;
    static constexpr int GeneratedRegion = 0x0800;
    static constexpr int SegmentID       = 0x1000;
    int m_eventFilter{0x1FFF};

    // ??? QTreeWidget seems like overkill.  We never have sub items.
    //     QTableWidget seems like a better choice.  See
    //     TempoAndTimeSignatureEditor for a complete example of using
    //     QTableWidget.
    QTreeWidget *m_eventList;
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);
    bool updateTreeWidget();

    /// Pop-up menu for the event list.
    QMenu *m_popUpMenu{nullptr};
    /// Create and show popup menu.
    void createPopUpMenu();

    std::vector<int> m_listSelection;
    void makeInitialSelection(timeT);

    // Events deleted since last refresh.
    std::set<Event *> m_deletedEvents;

    bool m_isTriggerSegment{false};
    QLabel *m_triggerName{nullptr};
    QLabel *m_triggerPitch{nullptr};
    QLabel *m_triggerVelocity{nullptr};

    void readOptions();
    void saveOptions();

};


}

#endif
