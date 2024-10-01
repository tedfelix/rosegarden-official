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

#define RG_MODULE_STRING "[EventView]"
#define RG_NO_DEBUG_PRINT

#include "EventView.h"

#include "EventViewItem.h"
#include "TrivialVelocityDialog.h"

#include "base/BaseProperties.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentPerformanceHelper.h"
#include "base/figuration/GeneratedRegion.h"
#include "base/figuration/SegmentID.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventEditCommand.h"
#include "commands/edit/EventInsertionCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SetTriggerSegmentBasePitchCommand.h"
#include "commands/segment/SetTriggerSegmentBaseVelocityCommand.h"
//#include "commands/segment/SetTriggerSegmentDefaultRetuneCommand.h"
//#include "commands/segment/SetTriggerSegmentDefaultTimeAdjustCommand.h"
#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/general/IconLoader.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QDesktopServices>


namespace Rosegarden
{


EventView::EventView(RosegardenDocument *doc,
                     const std::vector<Segment *> &segments) :
    EditViewBase(segments)
{
    setAttribute(Qt::WA_DeleteOnClose);

    setStatusBar(new QStatusBar(this));

    // Connect for changes so we can update the list.
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &EventView::slotDocumentModified);

    // For each Segment, subscribe for updates.
    // ??? We are seeing observers that are still extant with trigger
    //     segments.  This then causes a use after free when Segment's
    //     dtor tries to dump the name of the observer since we are gone.
    //     Our dtor does removeObserver(), but maybe there is an issue?
    //
    //     I was thinking that the fact that duplicates were in the observer
    //     list might be it, but remove() removes all that it finds.
    // ??? We do this again below.
    // ??? AND why are we subscribing for all when we only work with the
    //     first one?
    for (Segment *segment : m_segments) {
        segment->addObserver(this);
    }

    Composition &comp = doc->getComposition();

    // Note: We only edit the first Segment in segments.

    if (!segments.empty()) {
        Segment *s = *segments.begin();
        int id = comp.getTriggerSegmentId(s);
        if (id >= 0)
            m_isTriggerSegment = true;
    }

    // Clear the statusbar.
    // ??? I suspect it's always cleared at this point.  Don't think
    //     we need this.
    statusBar()->showMessage("");

    setupActions();

    // Create frame and layout.
    m_frame = new QFrame(this);
    // ??? Might not be wide enough for a trigger segment.
    m_frame->setMinimumSize(500, 300);
    m_gridLayout = new QGridLayout(m_frame);
    m_frame->setLayout(m_gridLayout);
    setCentralWidget(m_frame);

    // Event filters
    m_filterGroup = new QGroupBox(tr("Event filters"), m_frame);
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    m_filterGroup->setLayout(filterGroupLayout);

    m_noteCheckBox = new QCheckBox(tr("Note"), m_filterGroup);
    connect(m_noteCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_noteCheckBox);

    m_programCheckBox = new QCheckBox(tr("Program Change"), m_filterGroup);
    connect(m_programCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_programCheckBox);

    m_controllerCheckBox = new QCheckBox(tr("Controller"), m_filterGroup);
    connect(m_controllerCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_controllerCheckBox);

    m_pitchBendCheckBox = new QCheckBox(tr("Pitch Bend"), m_filterGroup);
    connect(m_pitchBendCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_pitchBendCheckBox);

    m_sysExCheckBox = new QCheckBox(tr("System Exclusive"), m_filterGroup);
    connect(m_sysExCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_sysExCheckBox);

    m_keyPressureCheckBox = new QCheckBox(tr("Key Pressure"), m_filterGroup);
    connect(m_keyPressureCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_keyPressureCheckBox);

    m_channelPressureCheckBox = new QCheckBox(tr("Channel Pressure"), m_filterGroup);
    connect(m_channelPressureCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_channelPressureCheckBox);

    m_restCheckBox = new QCheckBox(tr("Rest"), m_filterGroup);
    connect(m_restCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_restCheckBox);

    m_indicationCheckBox = new QCheckBox(tr("Indication"), m_filterGroup);
    connect(m_indicationCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_indicationCheckBox);

    m_textCheckBox = new QCheckBox(tr("Text"), m_filterGroup);
    connect(m_textCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_textCheckBox);

    m_generatedRegionCheckBox = new QCheckBox(tr("Generated regions"), m_filterGroup);
    connect(m_generatedRegionCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_generatedRegionCheckBox);

    m_segmentIDCheckBox = new QCheckBox(tr("Segment ID"), m_filterGroup);
    connect(m_segmentIDCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_segmentIDCheckBox);

    m_otherCheckBox = new QCheckBox(tr("Other"), m_filterGroup);
    connect(m_otherCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_otherCheckBox);

    m_gridLayout->addWidget(m_filterGroup, 0, 0);

    // Tree Widget

    // ??? Initial size is not wide enough.  Need to test with a trigger segment.
    m_eventList = new QTreeWidget(m_frame);
    // Double-click to edit.
    connect(m_eventList, &QTreeWidget::itemDoubleClicked,
            this, &EventView::slotPopupEventEditor);

    m_eventList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_eventList,
                &QWidget::customContextMenuRequested,
            this, &EventView::slotPopupMenu);

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringList columnNames;
    columnNames << tr("Time  ");
    columnNames << tr("Duration  ");
    columnNames << tr("Event Type  ");
    columnNames << tr("Pitch  ");
    columnNames << tr("Velocity  ");
    columnNames << tr("Type (Data1)  ");
    columnNames << tr("Type (Data1)  ");
    columnNames << tr("Value (Data2)  ");
    m_eventList->setHeaderLabels(columnNames);

    // Make sure time columns have the right amount of space.
    // ??? We should use the font size of "000-00-00-00" as the default size.
    //     Then we should save and restore the column widths.  See
    //     ShortcutDialog's ctor.
    constexpr int timeWidth = 110;
    // Plus a little for the tree diagram in the first column.
    m_eventList->setColumnWidth(0, timeWidth + 23);
    m_eventList->setColumnWidth(1, timeWidth);

    m_gridLayout->addWidget(m_eventList, 0, 1, 2, 1);

    // Trigger Segment Group Box

    if (m_isTriggerSegment) {

        const int triggerSegmentID = comp.getTriggerSegmentId(segments[0]);
        TriggerSegmentRec *triggerSegment =
                comp.getTriggerSegmentRec(triggerSegmentID);

        QGroupBox *groupBox = new QGroupBox(
                tr("Triggered Segment Properties"), m_frame);
        groupBox->setContentsMargins(5, 5, 5, 5);
        QGridLayout *layout = new QGridLayout(groupBox);
        layout->setSpacing(5);

        // Label
        layout->addWidget(new QLabel(tr("Label:  "), groupBox), 0, 0);
        QString label = strtoqstr(segments[0]->getLabel());
        if (label == "")
            label = tr("<no label>");
        m_triggerName = new QLabel(label, groupBox);
        layout->addWidget(m_triggerName, 0, 1);
        QPushButton *editButton = new QPushButton(tr("edit"), groupBox);
        layout->addWidget(editButton, 0, 2);
        connect(editButton, &QAbstractButton::clicked,
                this, &EventView::slotEditTriggerName);

        // Base pitch
        layout->addWidget(new QLabel(tr("Base pitch:  "), groupBox), 1, 0);
        m_triggerPitch = new QLabel(
                QString("%1").arg(triggerSegment->getBasePitch()), groupBox);
        layout->addWidget(m_triggerPitch, 1, 1);
        editButton = new QPushButton(tr("edit"), groupBox);
        layout->addWidget(editButton, 1, 2);
        connect(editButton, &QAbstractButton::clicked,
                this, &EventView::slotEditTriggerPitch);

        // Base velocity
        layout->addWidget(new QLabel(tr("Base velocity:  "), groupBox), 2, 0);
        m_triggerVelocity = new QLabel(
                QString("%1").arg(triggerSegment->getBaseVelocity()), groupBox);
        layout->addWidget(m_triggerVelocity, 2, 1);
        editButton = new QPushButton(tr("edit"), groupBox);
        layout->addWidget(editButton, 2, 2);
        connect(editButton, &QAbstractButton::clicked,
                this, &EventView::slotEditTriggerVelocity);

#if 0
        // These two options are not yet used anywhere else.  Intended for use
        // with library ornaments, not yet implemented

        // ??? All of this is implemented and stored with the trigger segment
        //     along with pitch and velocity.  We can probably get this working,
        //     but should we?

        // Default timing
        layout->addWidget(new QLabel(tr("Default timing:  "), frame), 3, 0);
        QComboBox *adjust = new QComboBox(frame);
        layout->addWidget(adjust, 3, 1, 1, 2);
        adjust->addItem(tr("As stored"));
        adjust->addItem(tr("Truncate if longer than note"));
        adjust->addItem(tr("End at same time as note"));
        adjust->addItem(tr("Stretch or squash segment to note duration"));
        std::string timing = triggerSegment->getDefaultTimeAdjust();
        if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
            adjust->setCurrentIndex(0);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
            adjust->setCurrentIndex(3);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
            adjust->setCurrentIndex(1);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
            adjust->setCurrentIndex(2);
        }
        connect(adjust,
                    static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &EventView::slotTriggerTimeAdjustChanged);

        // Adjust pitch to trigger note by default
        QCheckBox *retune = new QCheckBox(tr("Adjust pitch to trigger note by default"), frame);
        retune->setChecked(triggerSegment->getDefaultRetune());
        connect(retune, SIGNAL(clicked()), this, SLOT(slotTriggerRetuneChanged()));
        layout->addWidget(retune, 4, 1, 1, 2);
#endif

        groupBox->setLayout(layout);
        m_gridLayout->addWidget(groupBox, 0, 2);

    }

    // ??? Layout looks really bad.  The filter check boxes are all spread
    //     out.  The trigger segment check boxes are worse.  Perhaps we should:
    //     1. Move the filter check boxes to the menu.
    //     2. Move the trigger segment property editing to the trigger segment
    //        manager somehow.  I think it shows that info in its list.  Make
    //        the list editable.
    // ??? Doesn't work.  How do we fill the rest of the space?
    //m_gridLayout->setRowStretch(0, 1);
    //m_gridLayout->setRowStretch(1, 200);

    updateWindowTitle(false);
    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::documentModified,
            this, &EventView::updateWindowTitle);

    // ??? We are seeing observers that are still extant with trigger
    //     segments.  This then causes a use after free when Segment's
    //     dtor tries to dump the name of the observer since we are gone.
    //     Our dtor does removeObserver(), but maybe there is an issue?
    //
    //     I was thinking that the fact that duplicates were in the observer
    //     list might be it, but remove() removes all that it finds.
    // ??? AND why are we subscribing for all when we only work with the
    //     first one?
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->addObserver(this);
    }

    readOptions();
    updateFilterCheckBoxes();
    updateTreeWidget();

    makeInitialSelection(comp.getPosition());


    // Restore window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Event_List_View_Geometry").toByteArray());
    restoreState(settings.value("Event_List_View_State").toByteArray());
    settings.endGroup();
}

EventView::~EventView()
{
    saveOptions();

    for (Segment *segment : m_segments) {
        segment->removeObserver(this);
    }
}

void
EventView::closeEvent(QCloseEvent *event)
{
    // ??? This does nothing.  Get rid of it.

    QWidget::closeEvent(event);
}

void
EventView::eventRemoved(const Segment *, Event *e)
{
    m_deletedEvents.insert(e);
}

bool
EventView::updateTreeWidget()
{
    // ??? This selection stuff is extremely buggy.  It stores the indices,
    //     so if anything has changed, it will re-select the wrong items.
    //     It also completely loses the scroll position.  Working on fixing
    //     this in TempoAndTimeSignatureEditor.  See that for the latest.

    // If no selection, copy UI selection to m_listSelection.
    if (m_listSelection.size() == 0) {

        QList<QTreeWidgetItem *> selection = m_eventList->selectedItems();

        if (!selection.empty()) {
            for (int i = 0; i < selection.count(); ++i) {
                QTreeWidgetItem *listItem = selection.at(i);
                m_listSelection.push_back(
                        m_eventList->indexOfTopLevelItem(listItem));
            }
        }
    }

    // *** Create the event list.

    m_eventList->clear();

    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt();

    settings.endGroup();

    // For each Segment...
    for (unsigned int i = 0; i < m_segments.size(); i++) {
        SegmentPerformanceHelper helper(*m_segments[i]);

        // For each Event in the Segment...
        for (Segment::iterator it = m_segments[i]->begin();
                m_segments[i]->isBeforeEndMarker(it); ++it) {
            timeT eventTime =
                helper.getSoundingAbsoluteTime(it);

            QString velyStr;
            QString pitchStr;
            QString data1Str = "";
            QString data2Str = "";
            QString durationStr;

            // Event filters
            //
            //

            if ((*it)->isa(Note::EventRestType)) {
                if (!(m_eventFilter & Rest))
                    continue;

            } else if ((*it)->isa(Note::EventType)) {
                if (!(m_eventFilter & Note))
                    continue;

            } else if ((*it)->isa(Indication::EventType)) {
                if (!(m_eventFilter & Indication))
                    continue;

            } else if ((*it)->isa(PitchBend::EventType)) {
                if (!(m_eventFilter & PitchBend))
                    continue;

            } else if ((*it)->isa(SystemExclusive::EventType)) {
                if (!(m_eventFilter & SystemExclusive))
                    continue;

            } else if ((*it)->isa(ProgramChange::EventType)) {
                if (!(m_eventFilter & ProgramChange))
                    continue;

            } else if ((*it)->isa(ChannelPressure::EventType)) {
                if (!(m_eventFilter & ChannelPressure))
                    continue;

            } else if ((*it)->isa(KeyPressure::EventType)) {
                if (!(m_eventFilter & KeyPressure))
                    continue;

            } else if ((*it)->isa(Controller::EventType)) {
                if (!(m_eventFilter & Controller))
                    continue;

            } else if ((*it)->isa(Text::EventType)) {
                if (!(m_eventFilter & Text))
                    continue;

            } else if ((*it)->isa(GeneratedRegion::EventType)) {
                if (!(m_eventFilter & GeneratedRegion))
                    continue;

            } else if ((*it)->isa(SegmentID::EventType)) {
                if (!(m_eventFilter & SegmentID))
                    continue;

            } else {
                if (!(m_eventFilter & Other))
                    continue;
            }

            // avoid debug stuff going to stderr if no properties found

            if ((*it)->has(BaseProperties::PITCH)) {
                int p = (*it)->get
                        <Int>(BaseProperties::PITCH);
                pitchStr = QString("%1 %2  ")
                           .arg(p).arg(MidiPitchLabel(p).getQString());
            } else if ((*it)->isa(Note::EventType)) {
                pitchStr = tr("<not set>");
            }

            if ((*it)->has(BaseProperties::VELOCITY)) {
                velyStr = QString("%1  ").
                          arg((*it)->get
                              <Int>(BaseProperties::VELOCITY));
            } else if ((*it)->isa(Note::EventType)) {
                velyStr = tr("<not set>");
            }

            if ((*it)->has(Controller::NUMBER)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(Controller::NUMBER));
            } else if ((*it)->has(Text::TextTypePropertyName)) {
                data1Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (Text::TextTypePropertyName)));
            } else if ((*it)->has(Indication::
                                  IndicationTypePropertyName)) {
                data1Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (Indication::
                                          IndicationTypePropertyName)));
            } else if ((*it)->has(::Rosegarden::Key::KeyPropertyName)) {
                data1Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (::Rosegarden::Key::KeyPropertyName)));
            } else if ((*it)->has(Clef::ClefPropertyName)) {
                data1Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (Clef::ClefPropertyName)));
            } else if ((*it)->has(PitchBend::MSB)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(PitchBend::MSB));
            } else if ((*it)->has(BaseProperties::BEAMED_GROUP_TYPE)) {
                data1Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (BaseProperties::BEAMED_GROUP_TYPE)));
            } else if ((*it)->has(GeneratedRegion::FigurationPropertyName)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(GeneratedRegion::FigurationPropertyName));
            } else if ((*it)->has(SegmentID::IDPropertyName)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(SegmentID::IDPropertyName));
            }

            if ((*it)->has(Controller::VALUE)) {
                data2Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(Controller::VALUE));
            } else if ((*it)->has(Text::TextPropertyName)) {
                data2Str = QString("%1  ").
                           arg(strtoqstr((*it)->get
                                         <String>
                                         (Text::TextPropertyName)));
                /*!!!
                        } else if ((*it)->has(Indication::
                                  IndicationTypePropertyName)) {
                        data2Str = QString("%1  ").
                            arg((*it)->get<Int>(Indication::
                                    IndicationDurationPropertyName));
                */
            } else if ((*it)->has(PitchBend::LSB)) {
                data2Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(PitchBend::LSB));
            } else if ((*it)->has(BaseProperties::BEAMED_GROUP_ID)) {
                data2Str = tr("(group %1)  ")
                           .arg((*it)->get
                               <Int>(BaseProperties::BEAMED_GROUP_ID));
            } else if ((*it)->has(GeneratedRegion::ChordPropertyName)) {
                data2Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(GeneratedRegion::ChordPropertyName));
            } else if ((*it)->has(SegmentID::SubtypePropertyName)) {
                data2Str = QString("%1  ").
                    arg(strtoqstr((*it)->get
                                  <String>
                                  (SegmentID::SubtypePropertyName)));
            }

            if ((*it)->has(ProgramChange::PROGRAM)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(ProgramChange::PROGRAM) + 1);
            }

            if ((*it)->has(ChannelPressure::PRESSURE)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(ChannelPressure::PRESSURE));
            }

            if ((*it)->isa(KeyPressure::EventType) &&
                    (*it)->has(KeyPressure::PITCH)) {
                data1Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(KeyPressure::PITCH));
            }

            if ((*it)->has(KeyPressure::PRESSURE)) {
                data2Str = QString("%1  ").
                           arg((*it)->get
                               <Int>(KeyPressure::PRESSURE));
            }


            if ((*it)->getDuration() > 0 ||
                    (*it)->isa(Note::EventType) ||
                    (*it)->isa(Note::EventRestType)) {
                durationStr = makeDurationString(eventTime,
                                                 (*it)->getDuration(),
                                                 timeMode);
            }

            const QString timeStr = RosegardenDocument::currentDocument->
                    getComposition().makeTimeString(
                            eventTime,
                            static_cast<Composition::TimeMode>(timeMode));

            QStringList sl;
            sl << timeStr
               << durationStr
               << strtoqstr( (*it)->getType() )
               << pitchStr
               << velyStr
               << data1Str
               << data2Str;

            new EventViewItem(m_segments[i],  // segment
                              *it,  // event
                              m_eventList,  // parent
                              sl);  // strings
        }
    }


    // No Events?
    if ( m_eventList->topLevelItemCount() == 0 ) {
        if (m_segments.size())
            new QTreeWidgetItem(m_eventList,
                                QStringList() << tr("<no events at this filter level>"));
        else
            new QTreeWidgetItem(m_eventList, QStringList() << tr("<no events>"));

        m_eventList->setSelectionMode(QTreeWidget::NoSelection);
        leaveActionState("have_selection");
    } else {  // We have Events.

        m_eventList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);

        enterActionState("have_selection");
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();

    for (; sIt != m_listSelection.end(); ++sIt) {
        int index = *sIt;

        while (index > 0 && !m_eventList->topLevelItem(index))        // was itemAtIndex
            index--;

        m_eventList->setCurrentItem( m_eventList->topLevelItem(index) );

        // ensure visible
        m_eventList->scrollToItem(m_eventList->topLevelItem(index));
    }

    m_listSelection.clear();
    m_deletedEvents.clear();

    return true;
}

void
EventView::makeInitialSelection(timeT time)
{
    m_listSelection.clear();

    const int itemCount = m_eventList->topLevelItemCount();

    EventViewItem *goodItem = nullptr;
    int goodItemNo = 0;

    // For each item in the event list.
    // ??? Performance: LINEAR SEARCH
    //     While we could speed this up with a binary search, it would be
    //     smarter to find the appropriate event while we are creating the
    //     m_eventList in updateTreeWidget().
    for (int itemNo = 0; itemNo < itemCount; ++itemNo) {
        EventViewItem *item =
                dynamic_cast<EventViewItem *>(
                        m_eventList->topLevelItem(itemNo));

        // Not an EventViewItem?  Try the next.
        if (!item)
            continue;

        // If this item is past the playback position pointer, we are
        // finished searching.
        if (item->getEvent()->getAbsoluteTime() > time)
            break;

        // Remember the last good item.
        goodItem = item;
        goodItemNo = itemNo;
    }

    // Nothing found?  Bail.
    if (!goodItem)
        return;

    // Select the item prior to the playback position pointer.
    m_listSelection.push_back(goodItemNo);
    m_eventList->setCurrentItem(goodItem);
    m_eventList->scrollToItem(goodItem);
}

QString
EventView::makeDurationString(timeT time,
                              timeT duration, int timeMode)
{
    // ??? Same as TriggerSegmentManager::makeDurationString().  Move to
    //     Composition like makeTimeString().

    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            RosegardenDocument::currentDocument->getComposition().getMusicalTimeForDuration
            (time, duration, bar, beat, fraction, remainder);
            return QString("%1%2%3-%4%5-%6%7-%8%9   ")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case 1:  // real time
        {
            RealTime rt =
                RosegardenDocument::currentDocument->getComposition().getRealTimeDifference
                (time, time + duration);
            //    return QString("%1  ").arg(rt.toString().c_str());
            return QString("%1  ").arg(rt.toText().c_str());
        }

    default:
        return QString("%1  ").arg(duration);
    }
}

void
EventView::slotEditTriggerName()
{
    bool ok = false;
    QString newLabel = InputDialog::getText(this,
                                            tr("Segment label"),
                                            tr("Label:"),
                                            LineEdit::Normal,
                                            strtoqstr(m_segments[0]->getLabel()),
                                            &ok);

    if (ok) {
        SegmentSelection selection;
        selection.insert(m_segments[0]);
        SegmentLabelCommand *cmd = new SegmentLabelCommand(selection, newLabel);
        CommandHistory::getInstance()->addCommand(cmd);
        m_triggerName->setText(newLabel);
    }
}

void
EventView::slotEditTriggerPitch()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *triggerSegment =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    PitchDialog *dlg = new PitchDialog(this, tr("Base pitch"), triggerSegment->getBasePitch());

    if (dlg->exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(
                new SetTriggerSegmentBasePitchCommand(
                        &RosegardenDocument::currentDocument->getComposition(),
                        id,
                        dlg->getPitch()));
        m_triggerPitch->setText(QString("%1").arg(dlg->getPitch()));
    }
}

void
EventView::slotEditTriggerVelocity()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *triggerSegment =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    TrivialVelocityDialog *dlg = new TrivialVelocityDialog
                                 (this, tr("Base velocity"), triggerSegment->getBaseVelocity());

    if (dlg->exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(
                new SetTriggerSegmentBaseVelocityCommand(
                        &RosegardenDocument::currentDocument->getComposition(),
                        id,
                        dlg->getVelocity()));
        m_triggerVelocity->setText(QString("%1").arg(dlg->getVelocity()));
    }
}

/* unused
void
EventView::slotTriggerTimeAdjustChanged(int option)
{
    std::string adjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;

    switch (option) {

    case 0:
        adjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
        break;
    case 1:
        adjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START;
        break;
    case 2:
        adjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END;
        break;
    case 3:
        adjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;
        break;

    default:
        break;
    }

    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

//    TriggerSegmentRec *triggerSegment =  // remove warning
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    addCommandToHistory(new SetTriggerSegmentDefaultTimeAdjustCommand
                        (&RosegardenDocument::currentDocument->getComposition(), id, adjust));
}
*/

/* unused
void
EventView::slotTriggerRetuneChanged()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *triggerSegment =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    addCommandToHistory(new SetTriggerSegmentDefaultRetuneCommand
                        (&RosegardenDocument::currentDocument->getComposition(), id, !triggerSegment->getDefaultRetune()));
}
*/

void
EventView::slotEditCut()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCut - cutting "
    << selection.count() << " items";

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    EventSelection *cutSelection = nullptr;
    int itemIndex = -1;

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        QTreeWidgetItem *listItem = selection.at(i);

//        item = dynamic_cast<EventViewItem*>((*it));
        EventViewItem *item = dynamic_cast<EventViewItem*>(listItem);

        if (itemIndex == -1)
            itemIndex = m_eventList->indexOfTopLevelItem(listItem);
            //itemIndex = m_eventList->itemIndex(*it);

        if (item) {
            if (cutSelection == nullptr)
                cutSelection =
                    new EventSelection(*(item->getSegment()));

            cutSelection->addEvent(item->getEvent());
        }
//        ++it;
    }

    if (cutSelection) {
        if (itemIndex >= 0) {
            m_listSelection.clear();
            m_listSelection.push_back(itemIndex);
        }

        CommandHistory::getInstance()->addCommand(
                new CutCommand(cutSelection, Clipboard::mainClipboard()));
    }
}

void
EventView::slotEditCopy()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCopy - copying "
    << selection.count() << " items";

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    EventSelection *copySelection = nullptr;

    // clear the selection for post modification updating
    //
    m_listSelection.clear();

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        QTreeWidgetItem *listItem = selection.at(i);

//         item = dynamic_cast<EventViewItem*>((*it));
        EventViewItem *item = dynamic_cast<EventViewItem*>(listItem);

//         m_listSelection.push_back(m_eventList->itemIndex(*it));
        m_listSelection.push_back(m_eventList->indexOfTopLevelItem(listItem));

        if (item) {
            if (copySelection == nullptr)
                copySelection =
                    new EventSelection(*(item->getSegment()));

            copySelection->addEvent(item->getEvent());
        }
//         ++it;
    }

    if (copySelection) {
        CommandHistory::getInstance()->addCommand(
                new CopyCommand(copySelection, Clipboard::mainClipboard()));
    }
}

void
EventView::slotEditPaste()
{
    if (Clipboard::mainClipboard()->isEmpty()) {
        showStatusBarMessage(tr("Clipboard is empty"));
        return ;
    }

    TmpStatusMsg msg(tr("Inserting clipboard contents..."), this);

    timeT insertionTime = 0;

    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count()) {
        EventViewItem *item = dynamic_cast<EventViewItem*>(selection.at(0));

        if (item)
            insertionTime = item->getEvent()->getAbsoluteTime();

        // remember the selection
        //
        m_listSelection.clear();

//        QPtrListIterator<QTreeWidgetItem> it(selection);

//        while ((listItem = it.current()) != 0) {
        for( int i=0; i< selection.size(); i++ ){
            QTreeWidgetItem *listItem = selection.at(i);

            m_listSelection.push_back(m_eventList->indexOfTopLevelItem(listItem));
//             ++it;
        }
    }


    PasteEventsCommand *command = new PasteEventsCommand
                                  (*m_segments[0], Clipboard::mainClipboard(),
                                   insertionTime, PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        showStatusBarMessage(tr("Couldn't paste at this point"));
    } else
        CommandHistory::getInstance()->addCommand(command);

    RG_DEBUG << "EventView::slotEditPaste - pasting "
    << selection.count() << " items";
}

void
EventView::slotEditDelete()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();
    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditDelete - deleting "
    << selection.count() << " items";

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    EventSelection *deleteSelection = nullptr;
    int itemIndex = -1;

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        QTreeWidgetItem *listItem = selection.at(i);

//         item = dynamic_cast<EventViewItem*>((*it));
        EventViewItem *item = dynamic_cast<EventViewItem*>(listItem);

        if (itemIndex == -1)
            itemIndex = m_eventList->indexOfTopLevelItem(listItem);
            //itemIndex = m_eventList->itemIndex(*it);

        if (item) {
            if (m_deletedEvents.find(item->getEvent()) != m_deletedEvents.end()) {
//                ++it;
                continue;
            }

            if (deleteSelection == nullptr)
                deleteSelection =
                    new EventSelection(*m_segments[0]);

            deleteSelection->addEvent(item->getEvent());
        }
//         ++it;
    }

    if (deleteSelection) {

        if (itemIndex >= 0) {
            m_listSelection.clear();
            m_listSelection.push_back(itemIndex);
        }

        CommandHistory::getInstance()->addCommand(
                new EraseCommand(deleteSelection));

        // ??? What does this do?  Wouldn't updateTreeWidget() be more
        //     appropriate?
        m_eventList->update();
    }
}

void
EventView::slotEditInsert()
{
    timeT insertTime = m_segments[0]->getStartTime();
    // Go with a crotchet by default.
    timeT insertDuration = 960;

    QList<QTreeWidgetItem *> selection = m_eventList->selectedItems();

    // If something is selected, use the time and duration from the
    // first selected event.
    if (!selection.isEmpty()) {
        EventViewItem *item =
            dynamic_cast<EventViewItem *>(selection.first());

        if (item) {
            insertTime = item->getEvent()->getAbsoluteTime();
            insertDuration = item->getEvent()->getDuration();

            // ??? Could check for a note event and copy pitch and velocity.
        }
    }

    // Create default event
    //
    Event event(Note::EventType, insertTime, insertDuration);
    event.set<Int>(BaseProperties::PITCH, 70);
    event.set<Int>(BaseProperties::VELOCITY, 100);

    SimpleEventEditDialog dialog(
            this,
            RosegardenDocument::currentDocument,
            event,
            true);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    EventInsertionCommand *command =
            new EventInsertionCommand(
                    *m_segments[0],
                    new Event(dialog.getEvent()));

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotEditEvent()
{
    // See slotOpenInEventEditor().

    // ??? Why not use currentItem()?
    QList<QTreeWidgetItem *> selection = m_eventList->selectedItems();

    if (selection.isEmpty())
        return;

    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(selection.first());
    if (!eventViewItem)
        return;

    // Get the Segment.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

    // Get the Event.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    SimpleEventEditDialog dialog(
            this,
            RosegardenDocument::currentDocument,
            *event,
            false);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,
                                 dialog.getEvent());

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotEditEventAdvanced()
{
    // See slotOpenInExpertEventEditor().

    QList<QTreeWidgetItem *> selection = m_eventList->selectedItems();

    if (selection.isEmpty())
        return;

    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(selection.first());
    if (!eventViewItem)
        return;

    // Get the Segment.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

    // Get the Event.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    EventEditDialog dialog(this, *event);

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,
                                 dialog.getEvent());

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotSelectAll()
{
    m_listSelection.clear();
    for (int i = 0; m_eventList->topLevelItem(i); ++i) {
        m_listSelection.push_back(i);
        //m_eventList->setSelected(m_eventList->topLevelItem(i), true);
        m_eventList->setCurrentItem(m_eventList->topLevelItem(i));
    }
}

void
EventView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_eventList->topLevelItem(i); ++i) {
        //m_eventList->setSelected(m_eventList->topLevelItem(i), false);
        m_eventList->setCurrentItem(m_eventList->topLevelItem(i));
    }
}

void
EventView::setupActions()
{
    setupBaseActions();

    createAction("edit_cut", SLOT(slotEditCut()));
    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));

    createAction("insert", SLOT(slotEditInsert()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("edit_simple", SLOT(slotEditEvent()));
    createAction("edit_advanced", SLOT(slotEditEventAdvanced()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("event_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    QAction *musical = createAction("time_musical", SLOT(slotMusicalTime()));
    musical->setCheckable(true);

    QAction *real = createAction("time_real", SLOT(slotRealTime()));
    real->setCheckable(true);

    QAction *raw = createAction("time_raw", SLOT(slotRawTime()));
    raw->setCheckable(true);

    createMenusAndToolbars("eventlist.rc");

    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt() ;

    settings.endGroup();

    if (timeMode == 0) musical->setChecked(true);
    else if (timeMode == 1) real->setChecked(true);
    else if (timeMode == 2) raw->setChecked(true);

    if (m_isTriggerSegment) {
        QAction *action = findAction("open_in_matrix");
        if (action) delete action;
        action = findAction("open_in_notation");
        if (action) delete action;
    }
}

/* unused
QSize
EventView::getViewSize()
{
    return m_eventList->size();
}
*/

/* unused
void
EventView::setViewSize(QSize s)
{
    m_eventList->setFixedSize(s);
}
*/

void
EventView::readOptions()
{
    QSettings settings;

    settings.beginGroup(EventViewConfigGroup);

    m_eventFilter = settings.value("event_list_filter", m_eventFilter).toInt();

    const QByteArray qba = settings.value(EventViewLayoutConfigGroupName).toByteArray();
    m_eventList->restoreGeometry(qba);

    settings.endGroup();
}

void
EventView::saveOptions()
{
    QSettings settings;

    settings.beginGroup(EventViewConfigGroup);
    settings.setValue("event_list_filter", m_eventFilter);
    settings.setValue(EventViewLayoutConfigGroupName, m_eventList->saveGeometry());
    settings.endGroup();

    // Save window geometry and toolbar/dock state
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Event_List_View_Geometry", saveGeometry());
    settings.setValue("Event_List_View_State", saveState());
    settings.endGroup();
}

Segment *
EventView::getCurrentSegment()
{
    if (m_segments.empty())
        return nullptr;
    else
        return *m_segments.begin();
}

void
EventView::slotFilterClicked(bool)
{
    m_eventFilter = 0;

    if (m_noteCheckBox->isChecked())
        m_eventFilter |= Note;

    if (m_programCheckBox->isChecked())
        m_eventFilter |= ProgramChange;

    if (m_controllerCheckBox->isChecked())
        m_eventFilter |= Controller;

    if (m_pitchBendCheckBox->isChecked())
        m_eventFilter |= PitchBend;

    if (m_sysExCheckBox->isChecked())
        m_eventFilter |= SystemExclusive;

    if (m_keyPressureCheckBox->isChecked())
        m_eventFilter |= KeyPressure;

    if (m_channelPressureCheckBox->isChecked())
        m_eventFilter |= ChannelPressure;

    if (m_restCheckBox->isChecked())
        m_eventFilter |= Rest;

    if (m_indicationCheckBox->isChecked())
        m_eventFilter |= Indication;

    if (m_textCheckBox->isChecked())
        m_eventFilter |= Text;

    if (m_generatedRegionCheckBox->isChecked())
        m_eventFilter |= GeneratedRegion;

    if (m_segmentIDCheckBox->isChecked())
        m_eventFilter |= SegmentID;

    if (m_otherCheckBox->isChecked())
        m_eventFilter |= Other;

    updateTreeWidget();
}

void
EventView::updateFilterCheckBoxes()
{
    m_noteCheckBox->setChecked          (m_eventFilter & Note);
    m_programCheckBox->setChecked        (m_eventFilter & ProgramChange);
    m_controllerCheckBox->setChecked     (m_eventFilter & Controller);
    m_sysExCheckBox->setChecked          (m_eventFilter & SystemExclusive);
    m_textCheckBox->setChecked           (m_eventFilter & Text);
    m_restCheckBox->setChecked           (m_eventFilter & Rest);
    m_pitchBendCheckBox->setChecked      (m_eventFilter & PitchBend);
    m_channelPressureCheckBox->setChecked(m_eventFilter & ChannelPressure);
    m_keyPressureCheckBox->setChecked    (m_eventFilter & KeyPressure);
    m_indicationCheckBox->setChecked     (m_eventFilter & Indication);
    m_generatedRegionCheckBox->setChecked(m_eventFilter & GeneratedRegion);
    m_segmentIDCheckBox->setChecked      (m_eventFilter & SegmentID);
    m_otherCheckBox->setChecked          (m_eventFilter & Other);
}

void
EventView::slotMusicalTime()
{
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    settings.setValue("timemode", 0);
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);
    updateTreeWidget();

    settings.endGroup();
}

void
EventView::slotRealTime()
{
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    settings.setValue("timemode", 1);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);
    updateTreeWidget();

    settings.endGroup();
}

void
EventView::slotRawTime()
{
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    settings.setValue("timemode", 2);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);
    updateTreeWidget();

    settings.endGroup();
}

void
EventView::slotPopupEventEditor(QTreeWidgetItem *item, int /* column */)
{
    EventViewItem *eventViewItem = dynamic_cast<EventViewItem *>(item);
    if (!eventViewItem) {
        RG_WARNING << "slotPopupEventEditor(): WARNING: No EventViewItem.";
        return;
    }

    // Get the Segment.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    // ??? Why is the QTreeWidgetItem going away?  It appears to be
    //     happening as a result of the call to exec() below.  Is someone
    //     refreshing the list?
    Segment *segment = eventViewItem->getSegment();
    if (!segment) {
        RG_WARNING << "slotPopupEventEditor(): WARNING: No Segment.";
        return;
    }

    // !!! trigger events

    // Get the Event.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Event *event = eventViewItem->getEvent();
    if (!event) {
        RG_WARNING << "slotPopupEventEditor(): WARNING: No Event.";
        return;
    }

    SimpleEventEditDialog dialog(
                    this,  // parent
                    RosegardenDocument::currentDocument,
                    *event,
                    false);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    // Note: At this point, item and eventViewItem may be invalid.
    //       Do not use them.

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,
                                 dialog.getEvent());

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotPopupMenu(const QPoint& pos)
{
    QTreeWidgetItem *item = m_eventList->itemAt(pos);

    if (!item)
        return ;

    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);
    if (!eItem || !eItem->getEvent())
        return ;

    if (!m_popUpMenu)
        createPopUpMenu();

    if (m_popUpMenu)
        //m_menu->exec(QCursor::pos());
        m_popUpMenu->exec(m_eventList->mapToGlobal(pos));
    else
        RG_DEBUG << "EventView::showMenu() : no menu to show\n";
}

void
EventView::createPopUpMenu()
{
    m_popUpMenu = new QMenu(this);

    QAction *eventEditorAction =
            m_popUpMenu->addAction(tr("Open in Event Editor"));
    connect(eventEditorAction, &QAction::triggered,
            this, &EventView::slotOpenInEventEditor);

    QAction *expertEventEditorAction =
            m_popUpMenu->addAction(tr("Open in Expert Event Editor"));
    connect(expertEventEditorAction, &QAction::triggered,
            this, &EventView::slotOpenInExpertEventEditor);
}

void
EventView::slotOpenInEventEditor(bool /* checked */)
{
    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(m_eventList->currentItem());
    if (!eventViewItem)
        return;

    // Get the Segment.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

    // Get the Event.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    SimpleEventEditDialog dialog(
            this,
            RosegardenDocument::currentDocument,
            *event,
            false);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,
                                 dialog.getEvent());

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotOpenInExpertEventEditor(bool /* checked */)
{
    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(m_eventList->currentItem());
    if (!eventViewItem)
        return;

    // Get the Segment.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

    // Get the Event.  Have to do this before launching
    // the dialog since eventViewItem might become invalid.
    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    EventEditDialog dialog(this, *event);

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,
                                 dialog.getEvent());

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::updateWindowTitle(bool modified)
{
    QString indicator = (modified ? "*" : "");

    if (m_isTriggerSegment) {

        setWindowTitle(tr("%1%2 - Triggered Segment: %3")
                       .arg(indicator)
                       .arg(RosegardenDocument::currentDocument->getTitle())
                       .arg(strtoqstr(m_segments[0]->getLabel())));


    } else {
        if (m_segments.size() == 1) {

            // Fix bug #3007112
            if (!m_segments[0]->getComposition()) {
                // The segment is no more in the composition.
                // Nothing to edit : close the editor.
                close();
                return;
            }
        }
        QString view = tr("Event List");
        setWindowTitle(getTitle(view));
    }

    setWindowIcon(IconLoader::loadPixmap("window-eventlist"));

}


void
EventView::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:eventView-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:eventView-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
EventView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
EventView::segmentDeleted(const Segment *s)
{
    // ??? Bit of a design flaw.  Cast away const...
    const_cast<Segment *>(s)->removeObserver(this);

    // This editor cannot handle Segments that go away.  So just close.
    close();
}

void
EventView::slotDocumentModified(bool /*modified*/)
{
    updateTreeWidget();
}


}
