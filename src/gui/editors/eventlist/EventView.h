
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
    //void eventRemoved(const Segment *, Event *) override;
    void segmentDeleted(const Segment *) override;

signals:

    /// Connected to RosegardenMainViewWidget::slotEditTriggerSegment().
    /**
     * ??? When is this ever emitted?  The other editors notice the user has
     *     asked to edit a note that triggers a trigger Segment and then emit
     *     this.  This never emits.
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

    /// Connected to RosegardenDocument::setModified(bool)
    void slotUpdateWindowTitle(bool modified);

    void slotDocumentModified(bool modified);

private:

    QFrame *m_frame{nullptr};
    QGridLayout *m_gridLayout{nullptr};

    /// Set the button states to the current filter positions
    void updateFilterCheckBoxes();

    void setupActions();

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

    // ??? QTreeWidget seems like overkill.  We never have sub items.
    //     QTableWidget seems like a better choice.  See
    //     TempoAndTimeSignatureEditor for a complete example of using
    //     QTableWidget.
    QTreeWidget *m_treeWidget;
    QString makeDurationString(timeT time,
                               timeT duration, int timeMode);
    bool updateTreeWidget();

    /// Pop-up menu for the event list.
    QMenu *m_popUpMenu{nullptr};
    /// Create and show popup menu.
    void createPopUpMenu();

    void makeInitialSelection(timeT);

    bool m_isTriggerSegment{false};
    QLabel *m_triggerName{nullptr};
    QLabel *m_triggerPitch{nullptr};
    QLabel *m_triggerVelocity{nullptr};

    // ??? read/save?  How about load/save or read/write?
    void readOptions();
    void saveOptions();

};


}

#endif
