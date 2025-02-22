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
#include "misc/PreferenceBool.h"
#include "misc/PreferenceInt.h"

#include <QAction>
#include <QCheckBox>
#include <QDialog>
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


namespace
{

    // Persistent filter settings.
    PreferenceBool a_showNoteSetting(
            EventViewConfigGroup,
            "showNote",
            true);
    PreferenceBool a_showRestSetting(
            EventViewConfigGroup,
            "showRest",
            true);
    PreferenceBool a_showProgramChangeSetting(
            EventViewConfigGroup,
            "showProgramChange",
            true);
    PreferenceBool a_showControllerSetting(
            EventViewConfigGroup,
            "showController",
            true);
    PreferenceBool a_showPitchBendSetting(
            EventViewConfigGroup,
            "showPitchBend",
            true);
    PreferenceBool a_showSystemExclusiveSetting(
            EventViewConfigGroup,
            "showSystemExclusive",
            true);
    PreferenceBool a_showKeyPressureSetting(
            EventViewConfigGroup,
            "showKeyPressure",
            true);
    PreferenceBool a_showChannelPressureSetting(
            EventViewConfigGroup,
            "showChannelPressure",
            true);
    PreferenceBool a_showIndicationSetting(
            EventViewConfigGroup,
            "showIndication",
            true);
    PreferenceBool a_showTextSetting(
            EventViewConfigGroup,
            "showText",
            true);
    PreferenceBool a_showGeneratedRegionSetting(
            EventViewConfigGroup,
            "showGeneratedRegion",
            true);
    PreferenceBool a_showSegmentIDSetting(
            EventViewConfigGroup,
            "showSegmentID",
            true);
    PreferenceBool a_showOtherSetting(
            EventViewConfigGroup,
            "showOther",
            true);

    PreferenceInt a_timeModeSetting(
            EventViewConfigGroup,
            "timemode",
            int(Composition::TimeMode::MusicalTime));

    const char * const EventListLayoutGroup = "EventList_Layout";

}


EventView::EventView(RosegardenDocument *doc,
                     const std::vector<Segment *> &segments) :
    EditViewBase(segments)
{
    // We only support a single Segment.
    if (segments.size() != 1) {
        RG_WARNING << "Segment count was not 1.  (" << segments.size() << ")  Giving up...";
        return;
    }
    if (segments[0] == nullptr) {
        RG_WARNING << "Segment pointer is null.";
        return;
    }

    setAttribute(Qt::WA_DeleteOnClose);

    setWindowIcon(IconLoader::loadPixmap("window-eventlist"));

    setStatusBar(new QStatusBar(this));

    // Connect for changes so we can update the list.
    connect(doc, &RosegardenDocument::documentModified,
            this, &EventView::slotDocumentModified);

    Composition &comp = doc->getComposition();

    m_isTriggerSegment = (comp.getTriggerSegmentId(segments[0]) >= 0);

    setupActions();

    // Create main widget and layout.
    QWidget *mainWidget = new QWidget(this);
    QGridLayout *mainLayout = new QGridLayout(mainWidget);
    setCentralWidget(mainWidget);

    // *** Event filters

    m_filterGroup = new QGroupBox(tr("Event filters"), mainWidget);
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    // SetFixedSize - Make the layout exactly the size of its contents and
    //                do not allow it to expand or contract.
    filterGroupLayout->setSizeConstraint(QLayout::SetFixedSize);
    m_filterGroup->setLayout(filterGroupLayout);

    m_noteCheckBox = new QCheckBox(tr("Note"), m_filterGroup);
    connect(m_noteCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_noteCheckBox);

    m_restCheckBox = new QCheckBox(tr("Rest"), m_filterGroup);
    connect(m_restCheckBox, &QCheckBox::clicked,
            this, &EventView::slotFilterClicked);
    filterGroupLayout->addWidget(m_restCheckBox);

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

    mainLayout->addWidget(m_filterGroup, 0, 0, Qt::AlignHCenter);
    mainLayout->setRowMinimumHeight(0, m_filterGroup->height());

    // Tree Widget

    m_treeWidget = new QTreeWidget(mainWidget);
    // Double-click to edit.
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &EventView::slotItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &EventView::slotItemSelectionChanged);

    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget,
                &QWidget::customContextMenuRequested,
            this, &EventView::slotContextMenu);

    m_treeWidget->setAllColumnsShowFocus(true);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringList columnNames;
    columnNames << tr("Time  ");
    columnNames << tr("Duration  ");
    columnNames << tr("Event Type  ");
    columnNames << tr("Pitch  ");
    columnNames << tr("Velocity  ");
    columnNames << tr("Type (Data1)  ");
    columnNames << tr("Value (Data2)  ");
    m_treeWidget->setHeaderLabels(columnNames);

    // Make sure time columns have the right amount of space.
    constexpr int timeWidth = 110;
    // Plus a little for the tree diagram in the first column.
    m_treeWidget->setColumnWidth(0, timeWidth + 23);
    m_treeWidget->setColumnWidth(1, timeWidth);
    m_treeWidget->setMinimumWidth(700);

    mainLayout->addWidget(m_treeWidget, 0, 1, 3, 1);

    // Trigger Segment Group Box

    if (m_isTriggerSegment) {

        const int triggerSegmentID = comp.getTriggerSegmentId(segments[0]);
        TriggerSegmentRec *triggerSegment =
                comp.getTriggerSegmentRec(triggerSegmentID);

        QGroupBox *groupBox = new QGroupBox(
                tr("Triggered Segment Properties"), mainWidget);
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
        // ??? All of this is implemented and stored with the trigger segment
        //     along with pitch and velocity.  We can probably get this working,
        //     but should we?

        // Default timing
        layout->addWidget(new QLabel(tr("Default timing:  "), mainWidget), 3, 0);
        QComboBox *adjust = new QComboBox(mainWidget);
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
        QCheckBox *retune = new QCheckBox(tr("Adjust pitch to trigger note by default"), mainWidget);
        retune->setChecked(triggerSegment->getDefaultRetune());
        connect(retune, SIGNAL(clicked()), this, SLOT(slotTriggerRetuneChanged()));
        layout->addWidget(retune, 4, 1, 1, 2);
#endif

        groupBox->setLayout(layout);
        //m_gridLayout->addWidget(groupBox, 0, 2);
        mainLayout->addWidget(groupBox, 1, 0);

    }

    // Add a third row to expand to fill the remaining space and prevent
    // expansion of the contents of the first column.
    mainLayout->setRowStretch(2, 1);

    // Make sure main widget never gets too small.
    mainWidget->setMinimumSize(mainLayout->minimumSize());

    updateWindowTitle(false);

    loadOptions();
    updateFilterCheckBoxes();
    updateTreeWidget();

    makeInitialSelection(comp.getPosition());
}

EventView::~EventView()
{
    saveOptions();
}

bool
EventView::updateTreeWidget()
{
    // ??? This has scrolling issues.  It seems to not maintain at least the
    //     time position when filtering is changed.  Also when time mode is
    //     changed it jumps when there is a selection.  There are no
    //     scrollToItem() calls, so it isn't
    //     that.  It's odd since other lists seem to maintain scroll position
    //     just fine, especially when nothing really changes (time mode).

    // Store the selection.

    std::set<QString /*key*/> selection;

    // For each item in the tree...
    for (int itemIndex = 0;
         itemIndex < m_treeWidget->topLevelItemCount();
         ++itemIndex) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(itemIndex);
        // If this item is selected, add its key to the list.
        if (item->isSelected())
            selection.insert(item->data(0, Qt::UserRole).toString());
    }

    // Store "current" item.

    bool haveCurrentItem;
    QString currentItemKey;

    // Scope to avoid accidentally reusing currentItem after it is gone.
    {
        // The "current item" has the focus outline which is only
        // visible if it happens to be selected.
        QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
        haveCurrentItem = currentItem;
        if (haveCurrentItem)
            currentItemKey = currentItem->data(0, Qt::UserRole).toString();
    }


    // *** Create the event list.

    m_treeWidget->clear();

    SegmentPerformanceHelper helper(*m_segments[0]);

    const Composition::TimeMode timeMode =
            static_cast<Composition::TimeMode>(a_timeModeSetting.get());

    // For each Event in the Segment...
    for (Segment::iterator eventIter = m_segments[0]->begin();
         m_segments[0]->isBeforeEndMarker(eventIter);
         ++eventIter) {
        Event *event = *eventIter;

        // Event filters

        if (event->isa(Note::EventRestType)) {
            if (!m_showRest)
                continue;

        } else if (event->isa(Note::EventType)) {
            if (!m_showNote)
                continue;

        } else if (event->isa(Indication::EventType)) {
            if (!m_showIndication)
                continue;

        } else if (event->isa(PitchBend::EventType)) {
            if (!m_showPitchBend)
                continue;

        } else if (event->isa(SystemExclusive::EventType)) {
            if (!m_showSystemExclusive)
                continue;

        } else if (event->isa(ProgramChange::EventType)) {
            if (!m_showProgramChange)
                continue;

        } else if (event->isa(ChannelPressure::EventType)) {
            if (!m_showChannelPressure)
                continue;

        } else if (event->isa(KeyPressure::EventType)) {
            if (!m_showKeyPressure)
                continue;

        } else if (event->isa(Controller::EventType)) {
            if (!m_showController)
                continue;

        } else if (event->isa(Text::EventType)) {
            if (!m_showText)
                continue;

        } else if (event->isa(GeneratedRegion::EventType)) {
            if (!m_showGeneratedRegion)
                continue;

        } else if (event->isa(SegmentID::EventType)) {
            if (!m_showSegmentID)
                continue;

        } else {
            if (!m_showOther)
                continue;
        }

        // Format each column.

        // Time

        Composition &comp = RosegardenDocument::currentDocument->getComposition();

        const timeT eventTime = helper.getSoundingAbsoluteTime(eventIter);

        const QString timeStr = comp.makeTimeString(eventTime, timeMode);

        // Duration

        QString durationStr;
        QString musicalDuration;

        if (event->getDuration() > 0  ||
            event->isa(Note::EventType)  ||
            event->isa(Note::EventRestType)) {
            durationStr = comp.makeDurationString(
                    eventTime,
                    event->getDuration(),
                    timeMode);
            // For the key.
            musicalDuration = comp.makeDurationString(
                    eventTime,
                    event->getDuration(),
                    Composition::TimeMode::MusicalTime);
        }

        // Pitch

        QString pitchStr;

        if (event->has(BaseProperties::PITCH)) {
            const int pitch = event->get<Int>(BaseProperties::PITCH);
            pitchStr = QString("%1 %2  ")
                       .arg(pitch).arg(MidiPitchLabel(pitch).getQString());
        } else if (event->isa(Note::EventType)) {
            pitchStr = tr("<not set>");
        }

        // Velocity

        QString velocityStr;

        if (event->has(BaseProperties::VELOCITY)) {
            velocityStr = QString("%1  ").
                      arg(event->get<Int>(BaseProperties::VELOCITY));
        } else if (event->isa(Note::EventType)) {
            velocityStr = tr("<not set>");
        }

        // Data 1

        QString data1Str;

        if (event->has(Controller::NUMBER)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(Controller::NUMBER));
        } else if (event->has(Text::TextTypePropertyName)) {
            data1Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               Text::TextTypePropertyName)));
        } else if (event->has(Indication::IndicationTypePropertyName)) {
            data1Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               Indication::IndicationTypePropertyName)));
        } else if (event->has(Key::KeyPropertyName)) {
            data1Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               Key::KeyPropertyName)));
        } else if (event->has(Clef::ClefPropertyName)) {
            data1Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               Clef::ClefPropertyName)));
        } else if (event->has(PitchBend::MSB)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(PitchBend::MSB));
        } else if (event->has(BaseProperties::BEAMED_GROUP_TYPE)) {
            data1Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               BaseProperties::BEAMED_GROUP_TYPE)));
        } else if (event->has(GeneratedRegion::FigurationPropertyName)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(
                               GeneratedRegion::FigurationPropertyName));
        } else if (event->has(SegmentID::IDPropertyName)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(SegmentID::IDPropertyName));
        }

        if (event->has(ProgramChange::PROGRAM)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(ProgramChange::PROGRAM) + 1);
        }

        if (event->has(ChannelPressure::PRESSURE)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(ChannelPressure::PRESSURE));
        }

        if (event->isa(KeyPressure::EventType)  &&
            event->has(KeyPressure::PITCH)) {
            data1Str = QString("%1  ").
                       arg(event->get<Int>(KeyPressure::PITCH));
        }

        // Data 2

        QString data2Str;

        if (event->has(Controller::VALUE)) {
            data2Str = QString("%1  ").
                       arg(event->get<Int>(Controller::VALUE));
        } else if (event->has(Text::TextPropertyName)) {
            data2Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               Text::TextPropertyName)));
#if 0
        } else if (event->has(Indication::IndicationTypePropertyName)) {
            data2Str = QString("%1  ").
                       arg(event->get<Int>(
                               Indication::IndicationDurationPropertyName));
#endif
        } else if (event->has(PitchBend::LSB)) {
            data2Str = QString("%1  ").
                       arg(event->get<Int>(PitchBend::LSB));
        } else if (event->has(BaseProperties::BEAMED_GROUP_ID)) {
            data2Str = tr("(group %1)  ").
                       arg(event->get<Int>(BaseProperties::BEAMED_GROUP_ID));
        } else if (event->has(GeneratedRegion::ChordPropertyName)) {
            data2Str = QString("%1  ").
                       arg(event->get<Int>(GeneratedRegion::ChordPropertyName));
        } else if (event->has(SegmentID::SubtypePropertyName)) {
            data2Str = QString("%1  ").
                       arg(strtoqstr(event->get<String>(
                               SegmentID::SubtypePropertyName)));
        }

        if (event->has(KeyPressure::PRESSURE)) {
            data2Str = QString("%1  ").
                       arg(event->get<Int>(KeyPressure::PRESSURE));
        }

        if (event->has(BaseProperties::TRIGGER_SEGMENT_ID))
            data2Str = "TRIGGER";

        QStringList values;
        values << timeStr <<
                  durationStr <<
                  strtoqstr(event->getType()) <<
                  pitchStr <<
                  velocityStr <<
                  data1Str <<
                  data2Str;

        EventViewItem *item = new EventViewItem(
                m_segments[0],  // segment
                event,  // event
                m_treeWidget,  // parent
                values);  // strings

        // Assemble a key so we can uniquely identify each row for selection
        // persistence across updates.
        // ??? Why not just use the Event pointer address?  It's dangerous,
        //     but it should work well enough.  And it will be faster than a
        //     string comparison.  It will also avoid needing to keep
        //     musicalTime and musicalDuration.
        // Always use musical time in case the time mode changes.
        const QString musicalTime = comp.makeTimeString(
                                eventTime,
                                Composition::TimeMode::MusicalTime);
        const QString key = musicalTime + musicalDuration +
                strtoqstr(event->getType()) + pitchStr + velocityStr +
                data1Str + data2Str;
        item->setData(0, Qt::UserRole, key);

        // Restore current item.
        if (key == currentItemKey)
            m_treeWidget->setCurrentItem(item, 0, QItemSelectionModel::NoUpdate);

        // Restore selection.
        if (selection.find(key) != selection.end())
            item->setSelected(true);
    }

    return true;
}

void
EventView::makeInitialSelection(timeT time)
{
    const int itemCount = m_treeWidget->topLevelItemCount();

    EventViewItem *foundItem{nullptr};
    // Unused for tree.  We'll likely need it for QTableWidget.
    //int foundRow{0};

    // For each row in the event list.
    for (int row = 0; row < itemCount; ++row) {
        EventViewItem *item =
                dynamic_cast<EventViewItem *>(
                        m_treeWidget->topLevelItem(row));

        // Not an EventViewItem?  Try the next.
        if (!item)
            continue;

        // If this item is past the playback position pointer, we are
        // finished searching.
        if (item->getEvent()->getAbsoluteTime() > time)
            break;

        // Remember the last good item.
        foundItem = item;
        //foundRow = row;
    }

    // Nothing found?  Bail.
    if (!foundItem)
        return;

    // Make it current so the keyboard works correctly.
    m_treeWidget->setCurrentItem(foundItem);

    // Select the item
    foundItem->setSelected(true);

    // Yield to the event loop so that the UI will be rendered before calling
    // scrollToItem().
    qApp->processEvents();

    // Make sure the item is visible.
    m_treeWidget->scrollToItem(foundItem, QAbstractItemView::PositionAtCenter);
}

void
EventView::slotEditTriggerName()
{
    bool ok;
    QString newLabel = InputDialog::getText(
            this,  // parent
            tr("Segment label"),  // title
            tr("Label:"),  // label
            LineEdit::Normal,  // mode
            strtoqstr(m_segments[0]->getLabel()),  // text
            &ok);  // ok

    if (!ok)
        return;

    // Create SegmentLabelCommand and run.
    SegmentSelection selection;
    selection.insert(m_segments[0]);
    CommandHistory::getInstance()->addCommand(
            new SegmentLabelCommand(selection, newLabel));

    m_triggerName->setText(newLabel);
}

void
EventView::slotEditTriggerPitch()
{
    const int triggerSegmentID =
            m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    const TriggerSegmentRec *triggerSegment =
            m_segments[0]->getComposition()->getTriggerSegmentRec(triggerSegmentID);

    PitchDialog *dlg = new PitchDialog(
            this,  // parent
            tr("Base pitch"),  // title
            triggerSegment->getBasePitch());  // defaultPitch

    if (dlg->exec() != QDialog::Accepted)
        return;

    CommandHistory::getInstance()->addCommand(
            new SetTriggerSegmentBasePitchCommand(
                    &RosegardenDocument::currentDocument->getComposition(),
                    triggerSegmentID,
                    dlg->getPitch()));

    m_triggerPitch->setText(QString("%1").arg(dlg->getPitch()));
}

void
EventView::slotEditTriggerVelocity()
{
    const int triggerSegmentID =
            m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    const TriggerSegmentRec *triggerSegment =
            m_segments[0]->getComposition()->getTriggerSegmentRec(triggerSegmentID);

    TrivialVelocityDialog *dlg = new TrivialVelocityDialog(
            this,  // parent
            tr("Base velocity"),  // label
            triggerSegment->getBaseVelocity());  // velocity

    if (dlg->exec() != QDialog::Accepted)
        return;

    CommandHistory::getInstance()->addCommand(
            new SetTriggerSegmentBaseVelocityCommand(
                    &RosegardenDocument::currentDocument->getComposition(),
                    triggerSegmentID,
                    dlg->getVelocity()));  // newVelocity

    m_triggerVelocity->setText(QString("%1").arg(dlg->getVelocity()));
}

#if 0
// ??? This is unused, but seems like we should finish it.
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

    addCommandToHistory(new SetTriggerSegmentDefaultTimeAdjustCommand(
            &RosegardenDocument::currentDocument->getComposition(),
            id,
            adjust));
}
#endif
#if 0
// ??? This is unused, but seems like we should finish it.
void
EventView::slotTriggerRetuneChanged()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *triggerSegment =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    addCommandToHistory(new SetTriggerSegmentDefaultRetuneCommand(
            &RosegardenDocument::currentDocument->getComposition(),
            id,
            !triggerSegment->getDefaultRetune()));
}
#endif

void
EventView::slotEditCut()
{
    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();

    if (selection.empty())
        return;

    RG_DEBUG << "slotEditCut() - cutting " << selection.count() << " items";

    EventSelection cutSelection(*m_segments[0]);

    // For each QTreeWidgetItem in the selection...
    for (QTreeWidgetItem *listItem : selection) {
        EventViewItem *item = dynamic_cast<EventViewItem *>(listItem);
        if (!item)
            continue;

        cutSelection.addEvent(item->getEvent());
    }

    if (cutSelection.empty())
        return;

    CommandHistory::getInstance()->addCommand(
            new CutCommand(&cutSelection, Clipboard::mainClipboard()));
}

void
EventView::slotEditCopy()
{
    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();

    if (selection.empty())
        return;

    RG_DEBUG << "slotEditCopy() - copying " << selection.count() << " items";

    EventSelection copySelection(*m_segments[0]);

    // For each QTreeWidgetItem in the selection...
    for (QTreeWidgetItem *listItem : selection) {
        EventViewItem *item = dynamic_cast<EventViewItem *>(listItem);
        if (!item)
            continue;

        copySelection.addEvent(item->getEvent());
    }

    if (copySelection.empty())
        return;

    CommandHistory::getInstance()->addCommand(
            new CopyCommand(&copySelection, Clipboard::mainClipboard()));
}

void
EventView::slotEditPaste()
{
    // ??? This does nothing if a Segment or multiple Segments are
    //     in the clipboard.  We should probably handle that better.
    //     I assume PasteEventsCommand only handles the "partial
    //     segment" clipboard mode?

    TmpStatusMsg msg(tr("Inserting clipboard contents..."), this);

    // Compute the insertion time.

    timeT insertionTime = 0;

    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();

    if (!selection.empty()) {
        // Go with the time of the first selected item.
        EventViewItem *item = dynamic_cast<EventViewItem *>(selection.at(0));

        if (item)
            insertionTime = item->getEvent()->getAbsoluteTime();
    }

    PasteEventsCommand *command = new PasteEventsCommand(
            *m_segments[0],  // segment
            Clipboard::mainClipboard(),  // clipboard
            insertionTime,  // pasteTime
            PasteEventsCommand::MatrixOverlay);  // pasteType

    // Not possible?
    if (!command->isPossible()) {
        showStatusBarMessage(tr("Couldn't paste at this point"));
        delete command;
        return;
    }

    CommandHistory::getInstance()->addCommand(command);

    RG_DEBUG << "slotEditPaste() - pasting " << selection.count() << " items";
}

void
EventView::slotEditDelete()
{
    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();
    if (selection.empty())
        return;

    RG_DEBUG << "slotEditDelete() - deleting " << selection.count() << " items";

    EventSelection deleteSelection(*m_segments[0]);

    // For each item in the list...
    for (QTreeWidgetItem *listItem : selection) {
        EventViewItem *item = dynamic_cast<EventViewItem *>(listItem);
        if (!item)
            continue;

        deleteSelection.addEvent(item->getEvent());
    }

    if (deleteSelection.empty())
        return;

    CommandHistory::getInstance()->addCommand(
            new EraseCommand(&deleteSelection));
}

void
EventView::slotEditInsert()
{
    // Create default event in case nothing is selected.
    Event event(Note::EventType, m_segments[0]->getStartTime(), 960);
    event.set<Int>(BaseProperties::PITCH, 60);
    event.set<Int>(BaseProperties::VELOCITY, 100);

    // Copy new event from the first selected event.
    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();
    if (!selection.isEmpty()) {
        EventViewItem *item = dynamic_cast<EventViewItem *>(selection.first());
        event = *(item->getEvent());
    }

    SimpleEventEditDialog dialog(
            this,  // parent
            RosegardenDocument::currentDocument,  // doc
            event,
            true);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    CommandHistory::getInstance()->addCommand(
            new EventInsertionCommand(
                    *m_segments[0],
                    new Event(dialog.getEvent())));
}

void
EventView::slotEditEvent()
{
    // See slotOpenInEventEditor().

    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();

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
            this,  // parent
            RosegardenDocument::currentDocument,  // doc
            *event,
            false);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    CommandHistory::getInstance()->addCommand(
            new EventEditCommand(*segment,
                    event,
                    dialog.getEvent()));
}

void
EventView::slotEditEventAdvanced()
{
    // See slotOpenInExpertEventEditor().

    QList<QTreeWidgetItem *> selection = m_treeWidget->selectedItems();
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

    CommandHistory::getInstance()->addCommand(
            new EventEditCommand(*segment,
                    event,
                    dialog.getEvent()));
}

void
EventView::slotSelectAll()
{
    // For each item, select it.
    for (int itemIndex = 0;
         itemIndex < m_treeWidget->topLevelItemCount();
         ++itemIndex) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(itemIndex);
        item->setSelected(true);
    }
}

void
EventView::slotClearSelection()
{
    // For each item, deselect it.
    for (int itemIndex = 0;
         itemIndex < m_treeWidget->topLevelItemCount();
         ++itemIndex) {
        QTreeWidgetItem *item = m_treeWidget->topLevelItem(itemIndex);
        item->setSelected(false);
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

    slotUpdateClipboardActionState();

    const Composition::TimeMode timeMode =
            Composition::TimeMode(a_timeModeSetting.get());
    if (timeMode == Composition::TimeMode::MusicalTime)
        musical->setChecked(true);
    else if (timeMode == Composition::TimeMode::RealTime)
        real->setChecked(true);
    else if (timeMode == Composition::TimeMode::RawTime)
        raw->setChecked(true);

    // Disable launching of matrix and notation editors for triggered
    // segments.
    // Going with always enabled as it seems to be ok.  See
    // TriggerSegmentManager which only allows the Event List editor.
    // It should allow more.
    //findAction("open_in_matrix")->setEnabled(!m_isTriggerSegment);
    //findAction("open_in_notation")->setEnabled(!m_isTriggerSegment);
}

void
EventView::loadOptions()
{
    m_showNote = a_showNoteSetting.get();
    m_showRest = a_showRestSetting.get();
    m_showProgramChange = a_showProgramChangeSetting.get();
    m_showController = a_showControllerSetting.get();
    m_showPitchBend = a_showPitchBendSetting.get();
    m_showSystemExclusive = a_showSystemExclusiveSetting.get();
    m_showKeyPressure = a_showKeyPressureSetting.get();
    m_showChannelPressure = a_showChannelPressureSetting.get();
    m_showIndication = a_showIndicationSetting.get();
    m_showText = a_showTextSetting.get();
    m_showGeneratedRegion = a_showGeneratedRegionSetting.get();
    m_showSegmentID = a_showSegmentIDSetting.get();
    m_showOther = a_showOtherSetting.get();

    // Note that Wayland does not allow top-level window positioning.

    // Restore window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Event_List_View_Geometry").toByteArray());
    restoreState(settings.value("Event_List_View_State").toByteArray());
    settings.endGroup();

    // Restore list settings.
    settings.beginGroup(EventViewConfigGroup);
    m_treeWidget->restoreGeometry(
            settings.value(EventListLayoutGroup).toByteArray());
}

void
EventView::saveOptions()
{
    a_showNoteSetting.set(m_showNote);
    a_showRestSetting.set(m_showRest);
    a_showProgramChangeSetting.set(m_showProgramChange);
    a_showControllerSetting.set(m_showController);
    a_showPitchBendSetting.set(m_showPitchBend);
    a_showSystemExclusiveSetting.set(m_showSystemExclusive);
    a_showKeyPressureSetting.set(m_showKeyPressure);
    a_showChannelPressureSetting.set(m_showChannelPressure);
    a_showIndicationSetting.set(m_showIndication);
    a_showTextSetting.set(m_showText);
    a_showGeneratedRegionSetting.set(m_showGeneratedRegion);
    a_showSegmentIDSetting.set(m_showSegmentID);
    a_showOtherSetting.set(m_showOther);

    // Save list settings.
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);
    settings.setValue(EventListLayoutGroup, m_treeWidget->saveGeometry());
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

    return *m_segments.begin();
}

void
EventView::slotFilterClicked(bool)
{
    // Copy from check boxes to state.
    m_showNote = m_noteCheckBox->isChecked();
    m_showRest = m_restCheckBox->isChecked();
    m_showText = m_textCheckBox->isChecked();
    m_showSystemExclusive = m_sysExCheckBox->isChecked();
    m_showController = m_controllerCheckBox->isChecked();
    m_showProgramChange = m_programCheckBox->isChecked();
    m_showPitchBend = m_pitchBendCheckBox->isChecked();
    m_showChannelPressure = m_channelPressureCheckBox->isChecked();
    m_showKeyPressure = m_keyPressureCheckBox->isChecked();
    m_showIndication = m_indicationCheckBox->isChecked();
    m_showOther = m_otherCheckBox->isChecked();
    m_showGeneratedRegion = m_generatedRegionCheckBox->isChecked();
    m_showSegmentID = m_segmentIDCheckBox->isChecked();

    updateTreeWidget();
}

void
EventView::updateFilterCheckBoxes()
{
    // Copy from state to check boxes.
    m_noteCheckBox->setChecked(m_showNote);
    m_restCheckBox->setChecked(m_showRest);
    m_textCheckBox->setChecked(m_showText);
    m_sysExCheckBox->setChecked(m_showSystemExclusive);
    m_controllerCheckBox->setChecked(m_showController);
    m_programCheckBox->setChecked(m_showProgramChange);
    m_pitchBendCheckBox->setChecked(m_showPitchBend);
    m_channelPressureCheckBox->setChecked(m_showChannelPressure);
    m_keyPressureCheckBox->setChecked(m_showKeyPressure);
    m_indicationCheckBox->setChecked(m_showIndication);
    m_otherCheckBox->setChecked(m_showOther);
    m_generatedRegionCheckBox->setChecked(m_showGeneratedRegion);
    m_segmentIDCheckBox->setChecked(m_showSegmentID);
}

void
EventView::slotMusicalTime()
{
    a_timeModeSetting.set(int(Composition::TimeMode::MusicalTime));
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);

    updateTreeWidget();
}

void
EventView::slotRealTime()
{
    a_timeModeSetting.set(int(Composition::TimeMode::RealTime));
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);

    updateTreeWidget();
}

void
EventView::slotRawTime()
{
    a_timeModeSetting.set(int(Composition::TimeMode::RawTime));
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);

    updateTreeWidget();
}

void
EventView::slotItemDoubleClicked(QTreeWidgetItem *item, int /* column */)
{
    EventViewItem *eventViewItem = dynamic_cast<EventViewItem *>(item);
    if (!eventViewItem) {
        RG_WARNING << "slotItemDoubleClicked(): WARNING: No EventViewItem.";
        return;
    }

    Segment *segment = eventViewItem->getSegment();
    if (!segment) {
        RG_WARNING << "slotItemDoubleClicked(): WARNING: No Segment.";
        return;
    }

    Event *event = eventViewItem->getEvent();
    if (!event) {
        RG_WARNING << "slotItemDoubleClicked(): WARNING: No Event.";
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

    EventEditCommand *command = new EventEditCommand(
            *segment,
            event,  // eventToModify
            dialog.getEvent());  // newEvent

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotContextMenu(const QPoint &pos)
{
    // Use itemAt() which is more predictable than currentItem().
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item)
        return;

    EventViewItem *eventViewItem = dynamic_cast<EventViewItem *>(item);
    if (!eventViewItem)
        return;

    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    // If the context menu hasn't been created, create it.
    if (!m_contextMenu) {
        m_contextMenu = new QMenu(this);
        if (!m_contextMenu) {
            RG_WARNING << "slotContextMenu() : Couldn't create context menu.";
            return;
        }

        QAction *eventEditorAction =
                m_contextMenu->addAction(tr("Open in Event Editor"));
        connect(eventEditorAction, &QAction::triggered,
                this, &EventView::slotOpenInEventEditor);

        QAction *expertEventEditorAction =
                m_contextMenu->addAction(tr("Open in Expert Event Editor"));
        connect(expertEventEditorAction, &QAction::triggered,
                this, &EventView::slotOpenInExpertEventEditor);

        m_contextMenu->addSeparator();

        m_editTriggeredSegment =
                m_contextMenu->addAction(tr("Edit Triggered Segment"));
        connect(m_editTriggeredSegment, &QAction::triggered,
                this, &EventView::slotEditTriggeredSegment);
    }

    // Enable/disable triggered segment menu item.
    const bool trigger = event->has(BaseProperties::TRIGGER_SEGMENT_ID);
    m_editTriggeredSegment->setEnabled(trigger);

    // Launch the context menu.
    m_contextMenu->exec(m_treeWidget->mapToGlobal(pos));
}

void
EventView::slotOpenInEventEditor(bool /* checked */)
{
    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(m_treeWidget->currentItem());
    if (!eventViewItem)
        return;

    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    SimpleEventEditDialog dialog(
            this,  // parent
            RosegardenDocument::currentDocument,  // doc
            *event,  // event
            false);  // inserting

    // Launch dialog.  Bail if canceled.
    if (dialog.exec() != QDialog::Accepted)
        return;

    // Not modified?  Bail.
    if (!dialog.isModified())
        return;

    EventEditCommand *command =
            new EventEditCommand(*segment,
                                 event,  // eventToModify
                                 dialog.getEvent());  // newEvent

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotOpenInExpertEventEditor(bool /* checked */)
{
    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(m_treeWidget->currentItem());
    if (!eventViewItem)
        return;

    Segment *segment = eventViewItem->getSegment();
    if (!segment)
        return;

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
                                 event,  // eventToModify
                                 dialog.getEvent());  // newEvent

    CommandHistory::getInstance()->addCommand(command);
}

void
EventView::slotEditTriggeredSegment(bool /*checked*/)
{
    EventViewItem *eventViewItem =
            dynamic_cast<EventViewItem *>(m_treeWidget->currentItem());
    if (!eventViewItem)
        return;

    Event *event = eventViewItem->getEvent();
    if (!event)
        return;

    int triggeredSegmentID =
            event->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID);

    emit editTriggerSegment(triggeredSegmentID);
}

void
EventView::updateWindowTitle(bool modified)
{
    if (m_segments.size() != 1) {
        RG_WARNING << "updateWindowTitle(): m_segments size is not 1: " << m_segments.size();
        return;
    }

    QString indicator = (modified ? "*" : "");

    if (m_isTriggerSegment) {

        setWindowTitle(
                tr("%1%2 - Triggered Segment: %3").
                        arg(indicator).
                        arg(RosegardenDocument::currentDocument->getTitle()).
                        arg(strtoqstr(m_segments[0]->getLabel())));


    } else {

        QString view = tr("Event List");
        setWindowTitle(getTitle(view));

    }
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
EventView::slotDocumentModified(bool modified)
{
    // Determine whether the Segment is still in the Composition.

    bool inComposition{false};

    if (m_isTriggerSegment) {
        const int triggerSegmentID = RosegardenDocument::currentDocument->
                getComposition().getTriggerSegmentId(m_segments[0]);
        inComposition = (triggerSegmentID != -1);
    } else {
        inComposition = RosegardenDocument::currentDocument->
                getComposition().contains(m_segments[0]);
    }

    // No longer in the Composition?  Close the window.
    //
    // I tried using CompositionObserver::segmentRemoved() to close the window
    // in a simple and straightforward way and ended up completely wrapped
    // around the axle as the order in which notifications are sent and things
    // go away is unnecessarily complicated and unexpected.  It really
    // shouldn't be.  This approach just happens to work, but doesn't seem
    // ideal.  However, it is similar to the way all the other editors deal
    // with segments going away.
    if (!inComposition) {
        close();
        return;
    }

    updateWindowTitle(modified);

    updateTreeWidget();
}

void
EventView::slotItemSelectionChanged()
{
    bool haveSelection = !m_treeWidget->selectedItems().empty();

    if (haveSelection)
        enterActionState("have_selection");
    else
        leaveActionState("have_selection");
}


}
