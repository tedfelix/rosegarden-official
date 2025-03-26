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

#ifndef RG_EVENTVIEW2_H
#define RG_EVENTVIEW2_H

#include "base/Segment.h"  // SegmentObserver
#include "gui/general/EditViewBase.h"

#include <set>
#include <vector>

#include <QString>


class QWidget;
class QMenu;
class QPoint;
class QTableWidget;
class QTableWidgetItem;
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
class EventView2 : public EditViewBase
{
    Q_OBJECT

public:

    EventView2(RosegardenDocument *doc,
              const std::vector<Segment *> &segments);

    ~EventView2() override;

signals:

    /// Connected to RosegardenMainViewWidget::slotEditTriggerSegment().
    /**
     * Emitted by context menu "Edit Triggered Segment".
     */
    void editTriggerSegment(int id);

protected:

    // EditViewBase override.
    Segment *getCurrentSegment() override;

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
    void slotFilterClicked(bool);

    /// Handle double-click on an event in the event list.
    void slotCellDoubleClicked(int row, int column);
    void slotItemSelectionChanged();

    /// Right-click context menu.
    void slotContextMenu(const QPoint &);
    /// Right-click context menu handler.
    void slotOpenInEventEditor(bool checked);
    /// Right-click context menu handler.
    void slotOpenInExpertEventEditor(bool checked);
    /// Right-click context menu handler.
    void slotEditTriggeredSegment(bool checked);

    // Trigger Segments
    void slotEditTriggerName();
    void slotEditTriggerPitch();
    void slotEditTriggerVelocity();
    // unused void slotTriggerTimeAdjustChanged(int);
    // unused void slotTriggerRetuneChanged();

    void slotDocumentModified(bool modified);

private:

    /// Set the button states to the current filter positions
    void updateFilterCheckBoxes();

    void setupActions();

    void updateWindowTitle(bool modified);

    // Widgets

    // Event filters
    QGroupBox *m_filterGroup;
    QCheckBox *m_noteCheckBox;
    bool m_showNote{true};
    QCheckBox *m_restCheckBox;
    bool m_showRest{true};
    QCheckBox *m_programCheckBox;
    bool m_showProgramChange{true};
    QCheckBox *m_controllerCheckBox;
    bool m_showController{true};
    QCheckBox *m_pitchBendCheckBox;
    bool m_showPitchBend{true};
    QCheckBox *m_sysExCheckBox;
    bool m_showSystemExclusive{true};
    QCheckBox *m_keyPressureCheckBox;
    bool m_showKeyPressure{true};
    QCheckBox *m_channelPressureCheckBox;
    bool m_showChannelPressure{true};
    QCheckBox *m_indicationCheckBox;
    bool m_showIndication{true};
    QCheckBox *m_textCheckBox;
    bool m_showText{true};
    QCheckBox *m_generatedRegionCheckBox;
    bool m_showGeneratedRegion{true};
    QCheckBox *m_segmentIDCheckBox;
    bool m_showSegmentID{true};
    QCheckBox *m_otherCheckBox;
    bool m_showOther{true};

    // The Event table.
    QTableWidget *m_tableWidget;
    bool updateTableWidget();

    /// Pop-up menu for the event table.
    QMenu *m_contextMenu{nullptr};
    QAction *m_editTriggeredSegment{nullptr};

    void makeInitialSelection(timeT);

    bool m_isTriggerSegment{false};
    QLabel *m_triggerName{nullptr};
    QLabel *m_triggerPitch{nullptr};
    QLabel *m_triggerVelocity{nullptr};


    void loadOptions();
    void saveOptions();

    void editItem(const QTableWidgetItem *item);

};


}

#endif
