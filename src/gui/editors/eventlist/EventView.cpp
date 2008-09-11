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


#include <Q3CanvasPixmap>
#include "EventView.h"
#include "EventViewItem.h"
#include "TrivialVelocityDialog.h"
#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
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
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventEditCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/EventInsertionCommand.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SetTriggerSegmentBasePitchCommand.h"
#include "commands/segment/SetTriggerSegmentBaseVelocityCommand.h"
#include "commands/segment/SetTriggerSegmentDefaultRetuneCommand.h"
#include "commands/segment/SetTriggerSegmentDefaultTimeAdjustCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/dialogs/EventFilterDialog.h"
#include <kaction.h>
#include <QSettings>
#include <klocale.h>
#include <kstatusbar.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <klineeditdlg.h>
#include <QListWidget>
#include <kxmlguiclient.h>
#include <QGroupBox>
#include <Q3Canvas>
#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPixmap>
#include <QPoint>
#include <qpopupmenu.h>
#include <QPushButton>
#include <QSize>
#include <QString>
#include <QWidget>
#include <algorithm>


namespace Rosegarden
{

int
EventView::m_lastSetEventFilter = -1;


EventView::EventView(RosegardenGUIDoc *doc,
                     std::vector<Segment *> segments,
                     QWidget *parent):
        EditViewBase(doc, segments, 2, parent, "eventview"),
        m_eventFilter(Note | Text | SystemExclusive | Controller |
                      ProgramChange | PitchBend | Indication | Other),
        m_menu(0)
{
    m_isTriggerSegment = false;
    m_triggerName = m_triggerPitch = m_triggerVelocity = 0;

    if (!segments.empty()) {
        Segment *s = *segments.begin();
        if (s->getComposition()) {
            int id = s->getComposition()->getTriggerSegmentId(s);
            if (id >= 0)
                m_isTriggerSegment = true;
        }
    }

    if (m_lastSetEventFilter < 0)
        m_lastSetEventFilter = m_eventFilter;
    else
        m_eventFilter = m_lastSetEventFilter;

    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup =
        new QGroupBox(1, Horizontal, i18n("Event filters"), getCentralWidget());

    m_noteCheckBox = new QCheckBox(i18n("Note"), m_filterGroup);
    m_programCheckBox = new QCheckBox(i18n("Program Change"), m_filterGroup);
    m_controllerCheckBox = new QCheckBox(i18n("Controller"), m_filterGroup);
    m_pitchBendCheckBox = new QCheckBox(i18n("Pitch Bend"), m_filterGroup);
    m_sysExCheckBox = new QCheckBox(i18n("System Exclusive"), m_filterGroup);
    m_keyPressureCheckBox = new QCheckBox(i18n("Key Pressure"), m_filterGroup);
    m_channelPressureCheckBox = new QCheckBox(i18n("Channel Pressure"), m_filterGroup);
    m_restCheckBox = new QCheckBox(i18n("Rest"), m_filterGroup);
    m_indicationCheckBox = new QCheckBox(i18n("Indication"), m_filterGroup);
    m_textCheckBox = new QCheckBox(i18n("Text"), m_filterGroup);
    m_otherCheckBox = new QCheckBox(i18n("Other"), m_filterGroup);
    m_grid->addWidget(m_filterGroup, 2, 0);

    // Connect up
    //
    connect(m_filterGroup, SIGNAL(released(int)),
            SLOT(slotModifyFilter(int)));

    m_eventList = new QListWidget(getCentralWidget());
    m_eventList->setItemsRenameable(true);

    m_grid->addWidget(m_eventList, 2, 1);

    if (m_isTriggerSegment) {

        int id = segments[0]->getComposition()->getTriggerSegmentId(segments[0]);
        TriggerSegmentRec *rec =
            segments[0]->getComposition()->getTriggerSegmentRec(id);

        QGroupBox *groupBox = new QGroupBox
                              (1, Horizontal, i18n("Triggered Segment Properties"), getCentralWidget());

        QFrame *frame = new QFrame(groupBox);
        QGridLayout *layout = new QGridLayout(frame, 5, 3, 5, 5);

        layout->addWidget(new QLabel(i18n("Label:  "), frame), 0, 0);
        QString label = strtoqstr(segments[0]->getLabel());
        if (label == "")
            label = i18n("<no label>");
        m_triggerName = new QLabel(label, frame);
        layout->addWidget(m_triggerName, 0, 1);
        QPushButton *editButton = new QPushButton(i18n("edit"), frame);
        layout->addWidget(editButton, 0, 2);
        connect(editButton, SIGNAL(clicked()), this, SLOT(slotEditTriggerName()));

        layout->addWidget(new QLabel(i18n("Base pitch:  "), frame), 1, 0);
        m_triggerPitch = new QLabel(QString("%1").arg(rec->getBasePitch()), frame);
        layout->addWidget(m_triggerPitch, 1, 1);
        editButton = new QPushButton(i18n("edit"), frame);
        layout->addWidget(editButton, 1, 2);
        connect(editButton, SIGNAL(clicked()), this, SLOT(slotEditTriggerPitch()));

        layout->addWidget(new QLabel(i18n("Base velocity:  "), frame), 2, 0);
        m_triggerVelocity = new QLabel(QString("%1").arg(rec->getBaseVelocity()), frame);
        layout->addWidget(m_triggerVelocity, 2, 1);
        editButton = new QPushButton(i18n("edit"), frame);
        layout->addWidget(editButton, 2, 2);
        connect(editButton, SIGNAL(clicked()), this, SLOT(slotEditTriggerVelocity()));

        /*!!! Comment out these two options, which are not yet used
          anywhere else -- intended for use with library ornaments, not
          yet implemented

        layout->addWidget(new QLabel(i18n("Default timing:  "), frame), 3, 0);

        QComboBox *adjust = new QComboBox(frame);
        layout->addWidget(adjust, 3, 1, 1, 2);
        adjust->addItem(i18n("As stored"));
        adjust->addItem(i18n("Truncate if longer than note"));
        adjust->addItem(i18n("End at same time as note")); 
        adjust->addItem(i18n("Stretch or squash segment to note duration"));

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
            
        QCheckBox *retune = new QCheckBox(i18n("Adjust pitch to trigger note by default"), frame);
        retune->setChecked(rec->getDefaultRetune());
        connect(retune, SIGNAL(clicked()), this, SLOT(slotTriggerRetuneChanged()));
        layout->addWidget(retune, 4, 1, 1, 2);

        */

        m_grid->addWidget(groupBox, 2, 2);

    }

    updateViewCaption();

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->addObserver(this);
    }

    // Connect double clicker
    //
    connect(m_eventList, SIGNAL(doubleClicked(QListWidgetItem*)),
            SLOT(slotPopupEventEditor(QListWidgetItem*)));

    connect(m_eventList,
            SIGNAL(rightButtonPressed(QListWidgetItem*, const QPoint&, int)),
            SLOT(slotPopupMenu(QListWidgetItem*, const QPoint&, int)));

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QListWidget::Extended);

    m_eventList->addColumn(i18n("Time  "));
    m_eventList->addColumn(i18n("Duration  "));
    m_eventList->addColumn(i18n("Event Type  "));
    m_eventList->addColumn(i18n("Pitch  "));
    m_eventList->addColumn(i18n("Velocity  "));
    m_eventList->addColumn(i18n("Type (Data1)  "));
    m_eventList->addColumn(i18n("Value (Data2)  "));

    for (int col = 0; col < m_eventList->columns(); ++col)
        m_eventList->setRenameable(col, true);

    readOptions();
    setButtonsToFilter();
    applyLayout();

    makeInitialSelection(doc->getComposition().getPosition());

    slotCompositionStateUpdate();

    setOutOfCtor();
}

EventView::~EventView()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        RG_DEBUG << "~EventView - removing this observer from " << m_segments[i] << endl;
        m_segments[i]->removeObserver(this);
    }
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
        QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

        if (selection.count()) {
            QPtrListIterator<QListWidgetItem> it(selection);
            QListWidgetItem *listItem;

            while ((listItem = it.current()) != 0) {
                m_listSelection.push_back(m_eventList->itemIndex(*it));
                ++it;
            }
        }
    }

    // Ok, recreate list
    //
    m_eventList->clear();

    QSettings m_config;

    m_config.beginGroup( EventViewConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );

    //  

    int timeMode = m_config.value("timemode", 0).toInt() ;

    for (unsigned int i = 0; i < m_segments.size(); i++) {
        SegmentPerformanceHelper helper(*m_segments[i]);

        for (Segment::iterator it = m_segments[i]->begin();
                m_segments[i]->isBeforeEndMarker(it); it++) {
            timeT eventTime =
                helper.getSoundingAbsoluteTime(it);

            QString velyStr;
            QString pitchStr;
            QString data1Str = "";
            QString data2Str = "";
            QString durationStr;

            // Event filters
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
                pitchStr = "<not set>";
            }

            if ((*it)->has(BaseProperties::VELOCITY)) {
                velyStr = QString("%1  ").
                          arg((*it)->get
                              <Int>(BaseProperties::VELOCITY));
            } else if ((*it)->isa(Note::EventType)) {
                velyStr = "<not set>";
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
                data2Str = i18n("(group %1)  ", 
                           (*it)->get
                               <Int>(BaseProperties::BEAMED_GROUP_ID));
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

            new EventViewItem(m_segments[i],
                              *it,
                              m_eventList,
                              timeStr,
                              durationStr,
                              strtoqstr((*it)->getType()),
                              pitchStr,
                              velyStr,
                              data1Str,
                              data2Str);
        }
    }


    if (m_eventList->childCount() == 0) {
        if (m_segments.size())
            new QListWidgetItem(m_eventList,
                              i18n("<no events at this filter level>"));
        else
            new QListWidgetItem(m_eventList, i18n("<no events>"));

        m_eventList->setSelectionMode(QListWidget::NoSelection);
        stateChanged("have_selection", KXMLGUIClient::StateReverse);
    } else {
        m_eventList->setSelectionMode(QListWidget::Extended);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);

        stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt) {
        index = *sIt;

        while (index > 0 && !m_eventList->itemAtIndex(index))
            index--;

        m_eventList->setSelected(m_eventList->itemAtIndex(index), true);
        m_eventList->setCurrentIndex(m_eventList->itemAtIndex(index));

        // ensure visible
        m_eventList->ensureItemVisible(m_eventList->itemAtIndex(index));
    }

    m_listSelection.clear();
    m_deletedEvents.clear();

    return true;
}

void
EventView::makeInitialSelection(timeT time)
{
    m_listSelection.clear();

    EventViewItem *goodItem = 0;
    int goodItemNo = 0;

    int i = 0;

    for (QListWidgetItem *child = m_eventList->firstChild();
            child;
            child = child->nextSibling()) {

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
        m_eventList->setSelected(goodItem, true);
        m_eventList->ensureItemVisible(goodItem);
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
            //	return QString("%1  ").arg(rt.toString().c_str());
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
            //	return QString("%1  ").arg(rt.toString().c_str());
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
    RG_DEBUG << "EventView::refreshSegment" << endl;
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
    QString newLabel = KLineEditDlg::getText(i18n("Segment label"), i18n("Label:"),
                       strtoqstr(m_segments[0]->getLabel()),
                       &ok, this);

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

    PitchDialog *dlg = new PitchDialog(this, i18n("Base pitch"), rec->getBasePitch());

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
                                 (this, i18n("Base velocity"), rec->getBaseVelocity());

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

    TriggerSegmentRec *rec =
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
    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCut - cutting "
    << selection.count() << " items" << endl;

    QPtrListIterator<QListWidgetItem> it(selection);
    QListWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *cutSelection = 0;
    int itemIndex = -1;

    while ((listItem = it.current()) != 0) {
        item = dynamic_cast<EventViewItem*>((*it));

        if (itemIndex == -1)
            itemIndex = m_eventList->itemIndex(*it);

        if (item) {
            if (cutSelection == 0)
                cutSelection =
                    new EventSelection(*(item->getSegment()));

            cutSelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (cutSelection) {
        if (itemIndex >= 0) {
            m_listSelection.clear();
            m_listSelection.push_back(itemIndex);
        }

        addCommandToHistory(new CutCommand(*cutSelection,
                                           getDocument()->getClipboard()));
    }
}

void
EventView::slotEditCopy()
{
    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditCopy - copying "
    << selection.count() << " items" << endl;

    QPtrListIterator<QListWidgetItem> it(selection);
    QListWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *copySelection = 0;

    // clear the selection for post modification updating
    //
    m_listSelection.clear();

    while ((listItem = it.current()) != 0) {
        item = dynamic_cast<EventViewItem*>((*it));

        m_listSelection.push_back(m_eventList->itemIndex(*it));

        if (item) {
            if (copySelection == 0)
                copySelection =
                    new EventSelection(*(item->getSegment()));

            copySelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (copySelection) {
        addCommandToHistory(new CopyCommand(*copySelection,
                                            getDocument()->getClipboard()));
    }
}

void
EventView::slotEditPaste()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return ;
    }

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    timeT insertionTime = 0;

    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();
    if (selection.count()) {
        EventViewItem *item = dynamic_cast<EventViewItem*>(selection.at(0));

        if (item)
            insertionTime = item->getEvent()->getAbsoluteTime();

        // remember the selection
        //
        m_listSelection.clear();

        QPtrListIterator<QListWidgetItem> it(selection);
        QListWidgetItem *listItem;

        while ((listItem = it.current()) != 0) {
            m_listSelection.push_back(m_eventList->itemIndex(*it));
            ++it;
        }
    }


    PasteEventsCommand *command = new PasteEventsCommand
                                  (*m_segments[0], getDocument()->getClipboard(),
                                   insertionTime, PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else
        addCommandToHistory(command);

    RG_DEBUG << "EventView::slotEditPaste - pasting "
    << selection.count() << " items" << endl;
}

void
EventView::slotEditDelete()
{
    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();
    if (selection.count() == 0)
        return ;

    RG_DEBUG << "EventView::slotEditDelete - deleting "
    << selection.count() << " items" << endl;

    QPtrListIterator<QListWidgetItem> it(selection);
    QListWidgetItem *listItem;
    EventViewItem *item;
    EventSelection *deleteSelection = 0;
    int itemIndex = -1;

    while ((listItem = it.current()) != 0) {
        item = dynamic_cast<EventViewItem*>((*it));

        if (itemIndex == -1)
            itemIndex = m_eventList->itemIndex(*it);

        if (item) {
            if (m_deletedEvents.find(item->getEvent()) != m_deletedEvents.end()) {
                ++it;
                continue;
            }

            if (deleteSelection == 0)
                deleteSelection =
                    new EventSelection(*m_segments[0]);

            deleteSelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (deleteSelection) {

        if (itemIndex >= 0) {
            m_listSelection.clear();
            m_listSelection.push_back(itemIndex);
        }

        addCommandToHistory(new EraseCommand(*deleteSelection));

    }
}

void
EventView::slotEditInsert()
{
    RG_DEBUG << "EventView::slotEditInsert" << endl;

    timeT insertTime = m_segments[0]->getStartTime();
    timeT insertDuration = 960;

    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

        if (item) {
            insertTime = item->getEvent()->getAbsoluteTime();
            insertDuration = item->getEvent()->getDuration();
        }
    }

    // Create default event
    //
    Event *event =
        new Event(Note::EventType,
                  insertTime,
                  insertDuration);
    event->set
    <Int>(BaseProperties::PITCH, 70);
    event->set
    <Int>(BaseProperties::VELOCITY, 100);

    SimpleEventEditDialog dialog(this, getDocument(), *event, true);

    if (dialog.exec() == QDialog::Accepted) {
        EventInsertionCommand *command =
            new EventInsertionCommand(*m_segments[0],
                                      new Event(dialog.getEvent()));
        addCommandToHistory(command);
    }
}

void
EventView::slotEditEvent()
{
    RG_DEBUG << "EventView::slotEditEvent" << endl;

    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

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
    RG_DEBUG << "EventView::slotEditEventAdvanced" << endl;

    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0) {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

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
    for (int i = 0; m_eventList->itemAtIndex(i); ++i) {
        m_listSelection.push_back(i);
        m_eventList->setSelected(m_eventList->itemAtIndex(i), true);
    }
}

void
EventView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_eventList->itemAtIndex(i); ++i) {
        m_eventList->setSelected(m_eventList->itemAtIndex(i), false);
    }
}

void
EventView::slotFilterSelection()
{
    m_listSelection.clear();
    QPtrList<QListWidgetItem> selection = m_eventList->selectedItems();
    if (selection.count() == 0)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {

        QPtrListIterator<QListWidgetItem> it(selection);
        QListWidgetItem *listItem;

        while ((listItem = it.current()) != 0) {

            EventViewItem * item = dynamic_cast<EventViewItem*>(*it);
            if (!item) {
                ++it;
                continue;
            }

            if (!dialog.keepEvent(item->getEvent())) {
                m_listSelection.push_back(m_eventList->itemIndex(*it));
                m_eventList->setSelected(item, false);
            }

            ++it;
        }
    }
}

void
EventView::setupActions()
{
    EditViewBase::setupActions("eventlist.rc");

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIcon icon(QPixmap(pixmapDir + "/toolbar/event-insert.png"));

    new KAction(i18n("&Insert Event"), icon, Qt::Key_I, this,
                SLOT(slotEditInsert()), actionCollection(),
                "insert");

    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/event-delete.png");
    icon = QIcon(pixmap);

    new KAction(i18n("&Delete Event"), icon, Qt::Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");

    pixmap.load(pixmapDir + "/toolbar/event-edit.png");
    icon = QIcon(pixmap);

    new KAction(i18n("&Edit Event"), icon, Qt::Key_E, this,
                SLOT(slotEditEvent()), actionCollection(),
                "edit_simple");

    pixmap.load(pixmapDir + "/toolbar/event-edit-advanced.png");
    icon = QIcon(pixmap);

    new KAction(i18n("&Advanced Event Editor"), icon, Qt::Key_A, this,
                SLOT(slotEditEventAdvanced()), actionCollection(),
                "edit_advanced");

    //    icon = QIcon(Q3CanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), "filter", Qt::Key_F, this,
                SLOT(slotFilterSelection()), actionCollection(),
                "filter_selection");

    new KAction(i18n("Select &All"), Qt::Key_A + Qt::CTRL, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(i18n("Clear Selection"), Qt::Key_Escape, this,
                SLOT(slotClearSelection()), actionCollection(),
                "clear_selection");

    QSettings m_config;

    m_config.beginGroup( EventViewConfigGroup );

    // 

    // FIX-manually-(GW), add:

    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );

    //  

    int timeMode = m_config.value("timemode", 0).toInt() ;

    KRadioAction *action;

    pixmap.load(pixmapDir + "/toolbar/time-musical.png");
    icon = QIcon(pixmap);

    action = new KRadioAction(i18n("&Musical Times"), icon, 0, this,
                              SLOT(slotMusicalTime()),
                              actionCollection(), "time_musical");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 0)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.png");
    icon = QIcon(pixmap);

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
                              SLOT(slotRealTime()),
                              actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.png");
    icon = QIcon(pixmap);

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
                              SLOT(slotRawTime()),
                              actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2)
        action->setChecked(true);

    if (m_isTriggerSegment) {
        KAction *action = actionCollection()->action("open_in_matrix");
        if (action)
            delete action;
        action = actionCollection()->action("open_in_notation");
        if (action)
            delete action;
    }

    createGUI(getRCFileName());
}

void
EventView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    /*
    m_hoveredOverNoteName      = new QLabel(sb);
    m_hoveredOverAbsoluteTime  = new QLabel(sb);

    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);
    */

    sb->addItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);

    //m_selectionCounter = new QLabel(sb);
    //sb->addWidget(m_selectionCounter);
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
    QSettings m_config;
    m_config.beginGroup( EventViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );
    //  

    EditViewBase::readOptions();
    m_eventFilter = m_config.value("eventfilter", m_eventFilter).toInt() ;
    m_eventList->restoreLayout(m_config, EventViewLayoutConfigGroupName);
}

void
EventView::slotSaveOptions()
{
    QSettings m_config;
    m_config.beginGroup( EventViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );
    //  

    m_config.setValue("eventfilter", m_eventFilter);
    m_eventList->saveLayout(m_config, EventViewLayoutConfigGroupName);
}

Segment *
EventView::getCurrentSegment()
{
    if (m_segments.empty())
        return 0;
    else
        return *m_segments.begin();
}

void
EventView::slotModifyFilter(int button)
{
    QCheckBox *checkBox = dynamic_cast<QCheckBox*>(m_filterGroup->find(button));

    if (checkBox == 0)
        return ;

    if (checkBox->isChecked()) {
        switch (button) {
        case 0:
            m_eventFilter |= EventView::Note;
            break;

        case 1:
            m_eventFilter |= EventView::ProgramChange;
            break;

        case 2:
            m_eventFilter |= EventView::Controller;
            break;

        case 3:
            m_eventFilter |= EventView::PitchBend;
            break;

        case 4:
            m_eventFilter |= EventView::SystemExclusive;
            break;

        case 5:
            m_eventFilter |= EventView::KeyPressure;
            break;

        case 6:
            m_eventFilter |= EventView::ChannelPressure;
            break;

        case 7:
            m_eventFilter |= EventView::Rest;
            break;

        case 8:
            m_eventFilter |= EventView::Indication;
            break;

        case 9:
            m_eventFilter |= EventView::Text;
            break;

        case 10:
            m_eventFilter |= EventView::Other;
            break;

        default:
            break;
        }

    } else {
        switch (button) {
        case 0:
            m_eventFilter ^= EventView::Note;
            break;

        case 1:
            m_eventFilter ^= EventView::ProgramChange;
            break;

        case 2:
            m_eventFilter ^= EventView::Controller;
            break;

        case 3:
            m_eventFilter ^= EventView::PitchBend;
            break;

        case 4:
            m_eventFilter ^= EventView::SystemExclusive;
            break;

        case 5:
            m_eventFilter ^= EventView::KeyPressure;
            break;

        case 6:
            m_eventFilter ^= EventView::ChannelPressure;
            break;

        case 7:
            m_eventFilter ^= EventView::Rest;
            break;

        case 8:
            m_eventFilter ^= EventView::Indication;
            break;

        case 9:
            m_eventFilter ^= EventView::Text;
            break;

        case 10:
            m_eventFilter ^= EventView::Other;
            break;

        default:
            break;
        }
    }

    m_lastSetEventFilter = m_eventFilter;

    applyLayout(0);
}

void
EventView::setButtonsToFilter()
{
    if (m_eventFilter & Note)
        m_noteCheckBox->setChecked(true);
    else
        m_noteCheckBox->setChecked(false);

    if (m_eventFilter & ProgramChange)
        m_programCheckBox->setChecked(true);
    else
        m_programCheckBox->setChecked(false);

    if (m_eventFilter & Controller)
        m_controllerCheckBox->setChecked(true);
    else
        m_controllerCheckBox->setChecked(false);

    if (m_eventFilter & SystemExclusive)
        m_sysExCheckBox->setChecked(true);
    else
        m_sysExCheckBox->setChecked(false);

    if (m_eventFilter & Text)
        m_textCheckBox->setChecked(true);
    else
        m_textCheckBox->setChecked(false);

    if (m_eventFilter & Rest)
        m_restCheckBox->setChecked(true);
    else
        m_restCheckBox->setChecked(false);

    if (m_eventFilter & PitchBend)
        m_pitchBendCheckBox->setChecked(true);
    else
        m_pitchBendCheckBox->setChecked(false);

    if (m_eventFilter & ChannelPressure)
        m_channelPressureCheckBox->setChecked(true);
    else
        m_channelPressureCheckBox->setChecked(false);

    if (m_eventFilter & KeyPressure)
        m_keyPressureCheckBox->setChecked(true);
    else
        m_keyPressureCheckBox->setChecked(false);

    if (m_eventFilter & Indication) {
        m_indicationCheckBox->setChecked(true);
    } else {
        m_indicationCheckBox->setChecked(false);
    }

    if (m_eventFilter & Other) {
        m_otherCheckBox->setChecked(true);
    } else {
        m_otherCheckBox->setChecked(false);
    }
}

void
EventView::slotMusicalTime()
{
    QSettings m_config;
    m_config.beginGroup( EventViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );
    //  

    m_config.setValue("timemode", 0);
    applyLayout();
}

void
EventView::slotRealTime()
{
    QSettings m_config;
    m_config.beginGroup( EventViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );
    //  

    m_config.setValue("timemode", 1);
    applyLayout();
}

void
EventView::slotRawTime()
{
    QSettings m_config;
    m_config.beginGroup( EventViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // m_config.endGroup();		// corresponding to: m_config.beginGroup( EventViewConfigGroup );
    //  

    m_config.setValue("timemode", 2);
    applyLayout();
}

void
EventView::slotPopupEventEditor(QListWidgetItem *item)
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
EventView::slotPopupMenu(QListWidgetItem *item, const QPoint &pos, int)
{
    if (!item)
        return ;

    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);
    if (!eItem || !eItem->getEvent())
        return ;

    if (!m_menu)
        createMenu();

    if (m_menu)
        //m_menu->exec(QCursor::pos());
        m_menu->exec(pos);
    else
        RG_DEBUG << "EventView::showMenu() : no menu to show\n";
}

void
EventView::createMenu()
{
    m_menu = new QPopupMenu(this);
    m_menu->addItem(i18n("Open in Event Editor"), 0);
    m_menu->addItem(i18n("Open in Expert Event Editor"), 1);

    connect(m_menu, SIGNAL(activated(int)),
            SLOT(slotMenuActivated(int)));
}

void
EventView::slotMenuActivated(int value)
{
    RG_DEBUG << "EventView::slotMenuActivated - value = " << value << endl;

    if (value == 0) {
        EventViewItem *eItem = dynamic_cast<EventViewItem*>
                               (m_eventList->currentIndex());

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
        EventViewItem *eItem = dynamic_cast<EventViewItem*>
                               (m_eventList->currentIndex());

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
    if (m_isTriggerSegment) {

        setCaption(i18n("%1 - Triggered Segment: %2",
                    getDocument()->getTitle(),
                    strtoqstr(m_segments[0]->getLabel())));


    } else if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        setCaption(i18n("%1 - Segment Track #%2 - Event List",
                    getDocument()->getTitle(),
                    trackPosition + 1));

    } else {

        setCaption(i18n("%1 - %2 Segments - Event List",
                    getDocument()->getTitle(),
                    m_segments.size()));
    }

}

}
#include "EventView.moc"
