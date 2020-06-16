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

#define RG_MODULE_STRING "[EventView]"

#include "EventView.h"
#include "EventViewItem.h"
#include "TrivialVelocityDialog.h"

#include "base/BaseProperties.h"
#include "base/Clipboard.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentPerformanceHelper.h"
#include "base/Selection.h"
#include "base/Track.h"
#include "base/TriggerSegment.h"
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
#include "commands/segment/SetTriggerSegmentDefaultRetuneCommand.h"
#include "commands/segment/SetTriggerSegmentDefaultTimeAdjustCommand.h"
#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/general/ListEditView.h"
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
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QDesktopServices>

#include <algorithm>


namespace Rosegarden
{

EventView::EventView(RosegardenDocument *doc,
                     std::vector<Segment *> segments,
                     QWidget *parent):
        ListEditView(doc, segments, 2, parent),
        m_eventFilter(Note | Text | SystemExclusive | Controller |
                      ProgramChange | PitchBend | Indication | Other |
                      GeneratedRegion | SegmentID),
        m_menu(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_isTriggerSegment = false;
    m_triggerName = m_triggerPitch = m_triggerVelocity = nullptr;

    if (!segments.empty()) {
        Segment *s = *segments.begin();
        if (s->getComposition()) {
            int id = s->getComposition()->getTriggerSegmentId(s);
            if (id >= 0)
                m_isTriggerSegment = true;
        }
    }

    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup = new QGroupBox( tr("Event filters"), getCentralWidget() );
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    m_filterGroup->setAlignment( Qt::AlignHorizontal_Mask );

    m_noteCheckBox = new QCheckBox(tr("Note"), m_filterGroup);
    m_programCheckBox = new QCheckBox(tr("Program Change"), m_filterGroup);
    m_controllerCheckBox = new QCheckBox(tr("Controller"), m_filterGroup);
    m_pitchBendCheckBox = new QCheckBox(tr("Pitch Bend"), m_filterGroup);
    m_sysExCheckBox = new QCheckBox(tr("System Exclusive"), m_filterGroup);
    m_keyPressureCheckBox = new QCheckBox(tr("Key Pressure"), m_filterGroup);
    m_channelPressureCheckBox = new QCheckBox(tr("Channel Pressure"), m_filterGroup);
    m_restCheckBox = new QCheckBox(tr("Rest"), m_filterGroup);
    m_indicationCheckBox = new QCheckBox(tr("Indication"), m_filterGroup);
    m_textCheckBox = new QCheckBox(tr("Text"), m_filterGroup);
    m_generatedRegionCheckBox = new QCheckBox(tr("Generated regions"), m_filterGroup);
    m_segmentIDCheckBox = new QCheckBox(tr("Segment ID"), m_filterGroup);
    m_otherCheckBox = new QCheckBox(tr("Other"), m_filterGroup);

    filterGroupLayout->addWidget(m_noteCheckBox);
    filterGroupLayout->addWidget(m_programCheckBox);
    filterGroupLayout->addWidget(m_controllerCheckBox);
    filterGroupLayout->addWidget(m_pitchBendCheckBox);
    filterGroupLayout->addWidget(m_sysExCheckBox);
    filterGroupLayout->addWidget(m_keyPressureCheckBox);
    filterGroupLayout->addWidget(m_channelPressureCheckBox);
    filterGroupLayout->addWidget(m_restCheckBox);
    filterGroupLayout->addWidget(m_indicationCheckBox);
    filterGroupLayout->addWidget(m_textCheckBox);
    filterGroupLayout->addWidget(m_generatedRegionCheckBox);
    filterGroupLayout->addWidget(m_segmentIDCheckBox);
    filterGroupLayout->addWidget(m_otherCheckBox);
    m_filterGroup->setLayout(filterGroupLayout);

    m_grid->addWidget(m_filterGroup, 2, 0);

    m_eventList = new QTreeWidget(getCentralWidget());
    
    //m_eventList->setItemsRenameable(true); //&&& use item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable );

    m_grid->addWidget(m_eventList, 2, 1);

    if (m_isTriggerSegment) {

        int id = segments[0]->getComposition()->getTriggerSegmentId(segments[0]);
        TriggerSegmentRec *rec =
            segments[0]->getComposition()->getTriggerSegmentRec(id);

        QGroupBox *frame = new QGroupBox( tr("Triggered Segment Properties"), getCentralWidget() );
        frame->setAlignment( Qt::AlignHorizontal_Mask );
        frame->setContentsMargins(5, 5, 5, 5);
        QGridLayout *layout = new QGridLayout(frame);
        layout->setSpacing(5);

        layout->addWidget(new QLabel(tr("Label:  "), frame), 0, 0);
        QString label = strtoqstr(segments[0]->getLabel());
        if (label == "") label = tr("<no label>");
        m_triggerName = new QLabel(label, frame);
        layout->addWidget(m_triggerName, 0, 1);
        QPushButton *editButton = new QPushButton(tr("edit"), frame);
        layout->addWidget(editButton, 0, 2);
        connect(editButton, &QAbstractButton::clicked, this, &EventView::slotEditTriggerName);

        layout->addWidget(new QLabel(tr("Base pitch:  "), frame), 1, 0);
        m_triggerPitch = new QLabel(QString("%1").arg(rec->getBasePitch()), frame);
        layout->addWidget(m_triggerPitch, 1, 1);
        editButton = new QPushButton(tr("edit"), frame);
        layout->addWidget(editButton, 1, 2);
        connect(editButton, &QAbstractButton::clicked, this, &EventView::slotEditTriggerPitch);

        layout->addWidget(new QLabel(tr("Base velocity:  "), frame), 2, 0);
        m_triggerVelocity = new QLabel(QString("%1").arg(rec->getBaseVelocity()), frame);
        layout->addWidget(m_triggerVelocity, 2, 1);
        editButton = new QPushButton(tr("edit"), frame);
        layout->addWidget(editButton, 2, 2);
        connect(editButton, &QAbstractButton::clicked, this, &EventView::slotEditTriggerVelocity);

        /*!!! Comment out these two options, which are not yet used
          anywhere else -- intended for use with library ornaments, not
          yet implemented

        layout->addWidget(new QLabel(tr("Default timing:  "), frame), 3, 0);

        QComboBox *adjust = new QComboBox(frame);
        layout->addWidget(adjust, 3, 1, 1, 2);
        adjust->addItem(tr("As stored"));
        adjust->addItem(tr("Truncate if longer than note"));
        adjust->addItem(tr("End at same time as note")); 
        adjust->addItem(tr("Stretch or squash segment to note duration"));

        std::string timing = rec->getDefaultTimeAdjust();
        if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
            adjust->setCurrentIndex(0);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
            adjust->setCurrentIndex(3);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
            adjust->setCurrentIndex(1);
        } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
            adjust->setCurrentIndex(2);
        }

        connect(adjust, SIGNAL(activated(int)), this, SLOT(slotTriggerTimeAdjustChanged(int)));
            
        QCheckBox *retune = new QCheckBox(tr("Adjust pitch to trigger note by default"), frame);
        retune->setChecked(rec->getDefaultRetune());
        connect(retune, SIGNAL(clicked()), this, SLOT(slotTriggerRetuneChanged()));
        layout->addWidget(retune, 4, 1, 1, 2);

        */

        frame->setLayout(layout);
        m_grid->addWidget(frame, 2, 2);

    }

    updateViewCaption();
    connect(getDocument(), &RosegardenDocument::documentModified,
            this, &EventView::updateWindowTitle);

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->addObserver(this);
    }

    // Connect double clicker
    //
    connect(m_eventList, &QTreeWidget::itemDoubleClicked,
            this, &EventView::slotPopupEventEditor);

    m_eventList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_eventList,
            &QWidget::customContextMenuRequested,
            this, &EventView::slotPopupMenu);

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode( QAbstractItemView::ExtendedSelection );

    QStringList sl;
    sl << tr("Time  ");
    sl << tr("Duration  ");
    sl << tr("Event Type  ");
    sl << tr("Pitch  ");
    sl << tr("Velocity  ");
    sl << tr("Type (Data1)  ");
    sl << tr("Type (Data1)  ");
    sl << tr("Value (Data2)  ");
    
    m_eventList->setHeaderLabels ( sl );
    
//    for (int col = 0; col < m_eventList->columns(); ++col)
//        m_eventList->setRenameable(col, true); //&&& use item->setFlags( Qt::ItemIsSelectable |

    readOptions();
    setButtonsToFilter();
    applyLayout();

    // Connect the checkboxes AFTER calling setButtonsToFilter() to set up the
    // initial states.  Otherwise, the first state change triggers
    // slotModifyFilter() prematurely, and it wrecks the filter.
    //
    // That took an astonishing amount of time to work out.
    //
    //
    connect(m_noteCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);              
    connect(m_programCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_controllerCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_pitchBendCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_sysExCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_keyPressureCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_channelPressureCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_restCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_indicationCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_textCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_generatedRegionCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_segmentIDCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);
    connect(m_otherCheckBox, &QCheckBox::stateChanged, this, &EventView::slotModifyFilter);

    makeInitialSelection(doc->getComposition().getPosition());

    slotCompositionStateUpdate();


    // Restore window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Event_List_View_Geometry").toByteArray());
    this->restoreState(settings.value("Event_List_View_State").toByteArray());
    settings.endGroup();
}

EventView::~EventView()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        RG_DEBUG << "~EventView - removing this observer from " << m_segments[i];
        m_segments[i]->removeObserver(this);
    }
}

void
EventView::closeEvent(QCloseEvent *event)
{
    // Save m_eventFilter for next time
    slotSaveOptions();

    // Save window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Event_List_View_Geometry", this->saveGeometry());
    settings.setValue("Event_List_View_State", this->saveState());
    settings.endGroup();

    QWidget::closeEvent(event);
}

void
EventView::eventRemoved(const Segment *, Event *e)
{
    m_deletedEvents.insert(e);
}

void
EventView::segmentDeleted(const Segment *s)
{
    std::vector<Segment *>::iterator i = std::find(m_segments.begin(), m_segments.end(), s);

    if (i != m_segments.end()) {
        m_segments.erase(i);
    } else {
        RG_DEBUG << "%%% WARNING - EventView::segmentDeleted() called on non-registered segment - should not happen\n";
    }

}

bool
EventView::applyLayout(int /*staffNo*/)
{
    // If no selection has already been set then we copy what's
    // already set and try to replicate this after the rebuild
    // of the view.
    //
    if (m_listSelection.size() == 0) {
        
        QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();
        
        if( selection.count() ){
            QTreeWidgetItem *listItem;
            
            for( int i=0; i< selection.count(); i++ ) {
                
                listItem = selection.at(i);
                m_listSelection.push_back( m_eventList->indexOfTopLevelItem(listItem) );
                
            }
        }// end if selection.count()
    }// end if m_listSel..
    
    // Ok, recreate list
    //
    m_eventList->clear();

    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt();

    settings.endGroup();

    for (unsigned int i = 0; i < m_segments.size(); i++) {
        SegmentPerformanceHelper helper(*m_segments[i]);

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

            QString timeStr = makeTimeString(eventTime, timeMode);

            QStringList sl;
            sl << timeStr
            << durationStr
            << strtoqstr( (*it)->getType() )
            << pitchStr
            << velyStr
            << data1Str
            << data2Str;
            
            new EventViewItem(m_segments[i],
                              *it,
                              m_eventList,
                              sl
                               );
        }
    }


    if ( m_eventList->topLevelItemCount() == 0 ) {
        if (m_segments.size())
            new QTreeWidgetItem(m_eventList,
                                QStringList() << tr("<no events at this filter level>"));
        else
            new QTreeWidgetItem(m_eventList, QStringList() << tr("<no events>"));

        m_eventList->setSelectionMode(QTreeWidget::NoSelection);
        leaveActionState("have_selection");
    } else {
        
        m_eventList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);

        enterActionState("have_selection");
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt) {
        index = *sIt;

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

    EventViewItem *goodItem = nullptr;
    int goodItemNo = 0;

    int i = 0;
    QTreeWidgetItem *child = nullptr;
    
    for( i=0; i< m_eventList->topLevelItemCount(); i++ ){
        child = m_eventList->topLevelItem(i);

        EventViewItem * item = dynamic_cast<EventViewItem *>(child);

        if (item) {
            if (item->getEvent()->getAbsoluteTime() > time)
                break;
            goodItem = item;
            goodItemNo = i;
        }

        ++i;
    }
    /*!!!
        for (int i = 0; m_eventList->itemAtIndex(i); ++i) {
     
        EventViewItem *item = dynamic_cast<EventViewItem *>
            (m_eventList->itemAtIndex(i));
     
        if (item) {
            if (item->getEvent()->getAbsoluteTime() > time) break;
            goodItem = item;
            goodItemNo = i;
        }
        }
    */
    if (goodItem) {
        m_listSelection.push_back(goodItemNo);
        m_eventList->setCurrentItem( goodItem );
        m_eventList->scrollToItem(goodItem);
    }
}

QString
EventView::makeTimeString(timeT time, int timeMode)
{
    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            getDocument()->getComposition().getMusicalTimeForAbsoluteTime
            (time, bar, beat, fraction, remainder);
            ++bar;
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
                getDocument()->getComposition().getElapsedRealTime(time);
            //    return QString("%1  ").arg(rt.toString().c_str());
            return QString("%1  ").arg(rt.toText().c_str());
        }

    default:
        return QString("%1  ").arg(time);
    }
}

QString
EventView::makeDurationString(timeT time,
                              timeT duration, int timeMode)
{
    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            getDocument()->getComposition().getMusicalTimeForDuration
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
                getDocument()->getComposition().getRealTimeDifference
                (time, time + duration);
            //    return QString("%1  ").arg(rt.toString().c_str());
            return QString("%1  ").arg(rt.toText().c_str());
        }

    default:
        return QString("%1  ").arg(duration);
    }
}

void
EventView::refreshSegment(Segment * /*segment*/,
                          timeT /*startTime*/,
                          timeT /*endTime*/)
{
    RG_DEBUG << "EventView::refreshSegment";
    applyLayout(0);
}

void
EventView::updateView()
{
    m_eventList->update();
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
        addCommandToHistory(cmd);
        m_triggerName->setText(newLabel);
    }
}

void
EventView::slotEditTriggerPitch()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *rec =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    PitchDialog *dlg = new PitchDialog(this, tr("Base pitch"), rec->getBasePitch());

    if (dlg->exec() == QDialog::Accepted) {
        addCommandToHistory(new SetTriggerSegmentBasePitchCommand
                            (&getDocument()->getComposition(), id, dlg->getPitch()));
        m_triggerPitch->setText(QString("%1").arg(dlg->getPitch()));
    }
}

void
EventView::slotEditTriggerVelocity()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *rec =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    TrivialVelocityDialog *dlg = new TrivialVelocityDialog
                                 (this, tr("Base velocity"), rec->getBaseVelocity());

    if (dlg->exec() == QDialog::Accepted) {
        addCommandToHistory(new SetTriggerSegmentBaseVelocityCommand
                            (&getDocument()->getComposition(), id, dlg->getVelocity()));
        m_triggerVelocity->setText(QString("%1").arg(dlg->getVelocity()));
    }
}

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

//    TriggerSegmentRec *rec =  // remove warning
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    addCommandToHistory(new SetTriggerSegmentDefaultTimeAdjustCommand
                        (&getDocument()->getComposition(), id, adjust));
}

void
EventView::slotTriggerRetuneChanged()
{
    int id = m_segments[0]->getComposition()->getTriggerSegmentId(m_segments[0]);

    TriggerSegmentRec *rec =
        m_segments[0]->getComposition()->getTriggerSegmentRec(id);

    addCommandToHistory(new SetTriggerSegmentDefaultRetuneCommand
                        (&getDocument()->getComposition(), id, !rec->getDefaultRetune()));
}

void
EventView::slotEditCut()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCut - cutting "
    << selection.count() << " items" << endl;

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    QTreeWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *cutSelection = nullptr;
    int itemIndex = -1;

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        listItem = selection.at(i);
    
//        item = dynamic_cast<EventViewItem*>((*it));
        item = dynamic_cast<EventViewItem*>(listItem);

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

        addCommandToHistory(new CutCommand(*cutSelection,
                                           getClipboard()));
    }
}

void
EventView::slotEditCopy()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCopy - copying "
    << selection.count() << " items" << endl;

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    QTreeWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *copySelection = nullptr;

    // clear the selection for post modification updating
    //
    m_listSelection.clear();

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        listItem = selection.at(i);
        
//         item = dynamic_cast<EventViewItem*>((*it));
        item = dynamic_cast<EventViewItem*>(listItem);

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
        addCommandToHistory(new CopyCommand(*copySelection,
                                            getClipboard()));
    }
}

void
EventView::slotEditPaste()
{
    if (getClipboard()->isEmpty()) {
        slotStatusHelpMsg(tr("Clipboard is empty"));
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
        QTreeWidgetItem *listItem;

//        while ((listItem = it.current()) != 0) {
        for( int i=0; i< selection.size(); i++ ){
            listItem = selection.at(i);
            
            m_listSelection.push_back(m_eventList->indexOfTopLevelItem(listItem));
//             ++it;
        }
    }


    PasteEventsCommand *command = new PasteEventsCommand
                                  (*m_segments[0], getClipboard(),
                                   insertionTime, PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        slotStatusHelpMsg(tr("Couldn't paste at this point"));
    } else
        addCommandToHistory(command);

    RG_DEBUG << "EventView::slotEditPaste - pasting "
    << selection.count() << " items" << endl;
}

void
EventView::slotEditDelete()
{
    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();
    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditDelete - deleting "
    << selection.count() << " items" << endl;

//    QPtrListIterator<QTreeWidgetItem> it(selection);
    QTreeWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *deleteSelection = nullptr;
    int itemIndex = -1;

//    while ((listItem = it.current()) != 0) {
    for( int i=0; i< selection.size(); i++ ){
        listItem = selection.at(i);
            
//         item = dynamic_cast<EventViewItem*>((*it));
        item = dynamic_cast<EventViewItem*>(listItem);

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

        addCommandToHistory(new EraseCommand(*deleteSelection));
        updateView();
    }
}

void
EventView::slotEditInsert()
{
    RG_DEBUG << "EventView::slotEditInsert";

    timeT insertTime = m_segments[0]->getStartTime();
    timeT insertDuration = 960;

    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.first());

        if (item) {
            insertTime = item->getEvent()->getAbsoluteTime();
            insertDuration = item->getEvent()->getDuration();
        }
    }

    // Create default event
    //
    Event event(Note::EventType, insertTime, insertDuration);
    event.set<Int>(BaseProperties::PITCH, 70);
    event.set<Int>(BaseProperties::VELOCITY, 100);

    SimpleEventEditDialog dialog(this, getDocument(), event, true);

    if (dialog.exec() == QDialog::Accepted) {
        EventInsertionCommand *command =
            new EventInsertionCommand(
                    *m_segments[0],
                    new Event(dialog.getEvent()));
        addCommandToHistory(command);
    }
}

void
EventView::slotEditEvent()
{
    RG_DEBUG << "EventView::slotEditEvent";

    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.first());

        if (item) {
            Event *event = item->getEvent();
            SimpleEventEditDialog dialog(this, getDocument(), *event, false);

            if (dialog.exec() == QDialog::Accepted && dialog.isModified()) {
                EventEditCommand *command =
                    new EventEditCommand(*(item->getSegment()),
                                         event,
                                         dialog.getEvent());

                addCommandToHistory(command);
            }
        }
    }
}

void
EventView::slotEditEventAdvanced()
{
    RG_DEBUG << "EventView::slotEditEventAdvanced";

    QList<QTreeWidgetItem*> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.first());

        if (item) {
            Event *event = item->getEvent();
            EventEditDialog dialog(this, *event);

            if (dialog.exec() == QDialog::Accepted && dialog.isModified()) {
                EventEditCommand *command =
                    new EventEditCommand(*(item->getSegment()),
                                         event,
                                         dialog.getEvent());

                addCommandToHistory(command);
            }
        }
    }
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
    ListEditView::setupActions("eventlist.rc");

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

    createMenusAndToolbars(getRCFileName());

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

void
EventView::initStatusBar()
{
    QStatusBar* sb = statusBar();
    sb->showMessage(QString());
}

QSize
EventView::getViewSize()
{
    return m_eventList->size();
}

void
EventView::setViewSize(QSize s)
{
    m_eventList->setFixedSize(s);
}

void
EventView::readOptions()
{
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    EditViewBase::readOptions();
    m_eventFilter = settings.value("event_list_filter", m_eventFilter).toInt();
    
    QByteArray qba = settings.value(EventViewLayoutConfigGroupName).toByteArray();
    m_eventList->restoreGeometry(qba);

    settings.endGroup();
}

void
EventView::slotSaveOptions()
{
    QSettings settings;
    settings.beginGroup(EventViewConfigGroup);

    settings.setValue("event_list_filter", m_eventFilter);
    settings.setValue(EventViewLayoutConfigGroupName, m_eventList->saveGeometry());

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
EventView::slotModifyFilter()
{
    m_eventFilter = 0;

    if (m_noteCheckBox->isChecked()) m_eventFilter |= EventView::Note;

    if (m_programCheckBox->isChecked()) m_eventFilter |= EventView::ProgramChange;

    if (m_controllerCheckBox->isChecked()) m_eventFilter |= EventView::Controller;

    if (m_pitchBendCheckBox->isChecked()) m_eventFilter |= EventView::PitchBend;

    if (m_sysExCheckBox->isChecked()) m_eventFilter |= EventView::SystemExclusive;

    if (m_keyPressureCheckBox->isChecked()) m_eventFilter |= EventView::KeyPressure;

    if (m_channelPressureCheckBox->isChecked()) m_eventFilter |= EventView::ChannelPressure;

    if (m_restCheckBox->isChecked()) m_eventFilter |= EventView::Rest;

    if (m_indicationCheckBox->isChecked()) m_eventFilter |= EventView::Indication;

    if (m_textCheckBox->isChecked()) m_eventFilter |= EventView::Text;

    if (m_generatedRegionCheckBox->isChecked()) m_eventFilter |= EventView::GeneratedRegion;
    
    if (m_segmentIDCheckBox->isChecked()) m_eventFilter |= EventView::SegmentID;
    
    if (m_otherCheckBox->isChecked()) m_eventFilter |= EventView::Other;

    applyLayout(0);
}

void
EventView::setButtonsToFilter()
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
    applyLayout();

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
    applyLayout();

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
    applyLayout();

    settings.endGroup();
}

void
EventView::slotPopupEventEditor(QTreeWidgetItem *item, int)
{
    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);

    //!!! trigger events

    if (eItem) {
        Event *event = eItem->getEvent();
        SimpleEventEditDialog *dialog =
            new SimpleEventEditDialog(this, getDocument(), *event, false);

        if (dialog->exec() == QDialog::Accepted && dialog->isModified()) {
            EventEditCommand *command =
                new EventEditCommand(*(eItem->getSegment()),
                                     event,
                                     dialog->getEvent());

            addCommandToHistory(command);
        }

    }
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

    if (!m_menu)
        createMenu();

    if (m_menu)
        //m_menu->exec(QCursor::pos());
        m_menu->exec(m_eventList->mapToGlobal(pos));
    else
        RG_DEBUG << "EventView::showMenu() : no menu to show\n";
}

void
EventView::createMenu()
{
    //m_menu = new QPopupMenu(this);
    m_menu = new QMenu(this);
    QAction *eventEditorAction = m_menu->addAction( tr("Open in Event Editor") );
    QAction *expertEventEditorAction = m_menu->addAction( tr("Open in Expert Event Editor") );

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(eventEditorAction, 0);
    signalMapper->setMapping(expertEventEditorAction, 1);

    connect(eventEditorAction, SIGNAL(triggered()),
            signalMapper, SLOT(map()));
    connect(expertEventEditorAction, SIGNAL(triggered()),
            signalMapper, SLOT(map()));

    connect(signalMapper, SIGNAL(mapped(int)),
            SLOT(slotMenuActivated(int)));
}

void
EventView::slotMenuActivated(int value)
{
    RG_DEBUG << "EventView::slotMenuActivated - value = " << value;

    if (value == 0) {
        EventViewItem *eItem = dynamic_cast<EventViewItem*>(m_eventList->currentItem());

        if (eItem) {
            Event *event = eItem->getEvent();
            SimpleEventEditDialog *dialog =
                new SimpleEventEditDialog(this, getDocument(), *event, false);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified()) {
                EventEditCommand *command =
                    new EventEditCommand(*(eItem->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }

        }
    } else if (value == 1) {
        EventViewItem *eItem = dynamic_cast<EventViewItem*>(m_eventList->currentItem());

        if (eItem) {
            Event *event = eItem->getEvent();
            EventEditDialog *dialog = new EventEditDialog(this, *event);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified()) {
                EventEditCommand *command =
                    new EventEditCommand(*(eItem->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }

        }
    }

    return ;
}

void
EventView::updateViewCaption()
{
    // err on the side of not showing modified status here, rather than showing
    // a false positive, since we're kind of kludging this together with the
    // existing virtual updateViewCaption() method
    updateWindowTitle();
}

void
EventView::updateWindowTitle(bool m)
{
    QString indicator = (m ? "*" : "");

    if (m_isTriggerSegment) {

        setWindowTitle(tr("%1%2 - Triggered Segment: %3")
                       .arg(indicator)
                       .arg(getDocument()->getTitle())
                       .arg(strtoqstr(m_segments[0]->getLabel())));


    } else if (m_segments.size() == 1) {

        // Fix bug #3007112
        if (!m_segments[0]->getComposition()) {
            // The segment is no more in the composition.
            // Nothing to edit : close the editor.
            close();
            return;
        }
        
        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        setWindowTitle(tr("%1%2 - Segment Track #%3 - Event List")
                       .arg(indicator)
                       .arg(getDocument()->getTitle())
                       .arg(trackPosition + 1));

    } else {

        setWindowTitle(tr("%1%2 - %3 Segments - Event List")
                       .arg(getDocument()->getTitle())
                       .arg(m_segments.size()));
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
}
