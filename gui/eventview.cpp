// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// Event view list
//
//

#include <klocale.h>
#include <kconfig.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <ktmpstatusmsg.h>
#include <kstdaction.h> 
#include <kaction.h>
#include <kglobal.h>
#include <kstddirs.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qpopupmenu.h>
#include <qiconset.h>

#include <klistview.h>

#include "editview.h"
#include "eventview.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "editcommands.h"
#include "matrixtool.h"
#include "rosedebug.h"
#include "eventcommands.h"

#include "Segment.h"
#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include "MidiTypes.h"
#include "Selection.h"
#include "Clipboard.h"


using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::BaseProperties;

// EventView specialisation of a QListViewItem with the
// addition of a segment pointer
//
class EventViewItem : public QListViewItem
{
public:
    EventViewItem(Rosegarden::Segment *segment,
		  Rosegarden::Event *event,
                  QListView *parent) : 
	QListViewItem(parent),
	m_segment(segment),
	m_event(event) {;}
    
    EventViewItem(Rosegarden::Segment *segment,
		  Rosegarden::Event *event,
                  QListViewItem *parent) : 
	QListViewItem(parent),
	m_segment(segment),
	m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
		  Rosegarden::Event *event,
                  QListView *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null) :
	QListViewItem(parent, label1, label2, label3, label4,
		      label5, label6, label7, label8),
	m_segment(segment),
	m_event(event) {;}

    EventViewItem(Rosegarden::Segment *segment,
		  Rosegarden::Event *event,
                  QListViewItem *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null) :
	QListViewItem(parent, label1, label2, label3, label4,
		      label5, label6, label7, label8), 
	m_segment(segment),
	m_event(event) {;}

    Rosegarden::Segment* getSegment() { return m_segment; }
    Rosegarden::Event* getEvent() { return m_event; }

    // Reimplement so that we can sort numerically
    //
    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:

    Rosegarden::Segment *m_segment;
    Rosegarden::Event *m_event;
};


// Reimplementation of sort for numeric columns - taking the
// right hand argument from the left is equivalent to the
// the QString compare().
//
int
EventViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    if (col == 2) // event type
        return key(col, ascending).compare(i->key(col, ascending));
    else          // numeric comparison
        return key(col, ascending).toInt() - i->key(col, ascending).toInt();
}


////////////////////////////////////////////////////////////////////////

int
EventView::m_lastSetEventFilter = -1;

EventView::EventView(RosegardenGUIDoc *doc,
                     std::vector<Rosegarden::Segment *> segments,
                     QWidget *parent):
    EditViewBase(doc, segments, 2, parent, "eventview"),
    m_eventFilter(Note | Text | SystemExclusive | Controller |
		  ProgramChange | PitchBend | Indication | Other),
    m_menu(0)
{
    if (m_lastSetEventFilter < 0) m_lastSetEventFilter = m_eventFilter;
    else m_eventFilter = m_lastSetEventFilter;

    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup =
        new QButtonGroup(1, Horizontal, i18n("Event filters"), getCentralFrame());

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

    m_eventList = new KListView(getCentralFrame());
    m_eventList->setItemsRenameable(true);

    m_grid->addWidget(m_eventList, 2, 1);

    if (segments.size() == 1) {

        setCaption(QString("%1 - Segment Track #%2 - Event List")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));

    } else {

        setCaption(QString("%1 - %2 Segments - Event List")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
	m_segments[i]->addObserver(this);
    }

    // Connect double clicker
    //
    connect(m_eventList, SIGNAL(doubleClicked(QListViewItem*)),
            SLOT(slotPopupEventEditor(QListViewItem*)));

    connect(m_eventList, 
            SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
            SLOT(slotPopupMenu(QListViewItem*, const QPoint&, int)));

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QListView::Extended);

    m_eventList->addColumn(i18n("Time  "));
    m_eventList->addColumn(i18n("Duration  "));
    m_eventList->addColumn(i18n("Event Type  "));
    m_eventList->addColumn(i18n("Pitch  "));
    m_eventList->addColumn(i18n("Velocity  "));
    m_eventList->addColumn(i18n("Type (Data1)  "));
    m_eventList->addColumn(i18n("Value (Data2)  "));

    for(int col = 0; col < m_eventList->columns(); ++col)
   	m_eventList->setRenameable(col, true);

    readOptions();
    setButtonsToFilter();
    applyLayout();
}

EventView::~EventView()
{
    if (!getDocument()->isBeingDestroyed()) {
	for (unsigned int i = 0; i < m_segments.size(); ++i)
	    m_segments[i]->removeObserver(this);
    }
}

void
EventView::eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *e)
{
    m_deletedEvents.insert(e);
}

bool
EventView::applyLayout(int /*staffNo*/)
{
    // If no selection has already been set then we copy what's
    // already set and try to replicate this after the rebuild
    // of the view.
    //
    if (m_listSelection.size() == 0)
    {
        QPtrList<QListViewItem> selection = m_eventList->selectedItems();

        if (selection.count())
        {
            QPtrListIterator<QListViewItem> it(selection);
            QListViewItem *listItem;

            while((listItem = it.current()) != 0)
            {
                m_listSelection.push_back(m_eventList->itemIndex(*it));
                ++it;
            }
        }
    }

    // Ok, recreate list
    //
    m_eventList->clear();

    for (unsigned int i = 0; i < m_segments.size(); i++)
    {
        Rosegarden::SegmentPerformanceHelper helper(*m_segments[i]);

        for (Rosegarden::Segment::iterator it = m_segments[i]->begin();
                               m_segments[i]->isBeforeEndMarker(it); it++)
        {
            Rosegarden::timeT eventTime =
                helper.getSoundingAbsoluteTime(it);

            QString velyStr;
            QString pitchStr;
            QString data1Str = "";
            QString data2Str = "";
	    QString durationStr;

            // Event filters
            //

            if ((*it)->isa(Rosegarden::Note::EventRestType)) {
		if (!(m_eventFilter & Rest)) continue;

	    } else if ((*it)->isa(Rosegarden::Note::EventType)) {
		if (!(m_eventFilter & Note)) continue;

	    } else if ((*it)->isa(Rosegarden::Indication::EventType)) {
		if (!(m_eventFilter & Indication)) continue;

	    } else if ((*it)->isa(Rosegarden::PitchBend::EventType)) {
		if (!(m_eventFilter & PitchBend)) continue;

	    } else if ((*it)->isa(Rosegarden::SystemExclusive::EventType)) {
		if (!(m_eventFilter & SystemExclusive)) continue;

	    } else if ((*it)->isa(Rosegarden::ProgramChange::EventType)) {
		if (!(m_eventFilter & ProgramChange)) continue;

	    } else if ((*it)->isa(Rosegarden::ChannelPressure::EventType)) {
		if (!(m_eventFilter & ChannelPressure)) continue;

	    } else if ((*it)->isa(Rosegarden::KeyPressure::EventType)) {
		if (!(m_eventFilter & KeyPressure)) continue;

	    } else if ((*it)->isa(Rosegarden::Controller::EventType)) {
		if (!(m_eventFilter & Controller)) continue;

	    } else if ((*it)->isa(Rosegarden::Text::EventType)) {
		if (!(m_eventFilter & Text)) continue;

	    } else {
		if (!(m_eventFilter & Other)) continue;
	    }

	    // avoid debug stuff going to stderr if no properties found

	    if ((*it)->has(BaseProperties::PITCH)) {
		pitchStr = QString("%1  ").
                    arg((*it)->get<Int>(BaseProperties::PITCH));
	    } else if ((*it)->isa(Rosegarden::Note::EventType)) {
		pitchStr = "<not set>";
	    }

	    if ((*it)->has(BaseProperties::VELOCITY)) {
		velyStr = QString("%1  ").
                    arg((*it)->get<Int>(BaseProperties::VELOCITY));
	    } else if ((*it)->isa(Rosegarden::Note::EventType)) {
		velyStr = "<not set>";
	    }

            if ((*it)->has(Rosegarden::Controller::NUMBER)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::Controller::NUMBER));
            } else if ((*it)->has(Rosegarden::Text::TextTypePropertyName)) {
		data1Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (Rosegarden::Text::TextTypePropertyName)));
	    } else if ((*it)->has(Rosegarden::Indication::
				  IndicationTypePropertyName)) {
		data1Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (Rosegarden::Indication::
				   IndicationTypePropertyName)));
	    } else if ((*it)->has(Rosegarden::Key::KeyPropertyName)) {
		data1Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (Rosegarden::Key::KeyPropertyName)));
	    } else if ((*it)->has(Rosegarden::Clef::ClefPropertyName)) {
		data1Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (Rosegarden::Clef::ClefPropertyName)));
	    } else if ((*it)->has(Rosegarden::PitchBend::MSB)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::PitchBend::MSB));
            } else if ((*it)->has(BaseProperties::BEAMED_GROUP_TYPE)) {
		data1Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (BaseProperties::BEAMED_GROUP_TYPE)));
	    }

            if ((*it)->has(Rosegarden::Controller::VALUE)) {
                data2Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::Controller::VALUE));
            } else if ((*it)->has(Rosegarden::Text::TextPropertyName)) {
		data2Str = QString("%1  ").
		    arg(strtoqstr((*it)->get<String>
				  (Rosegarden::Text::TextPropertyName)));
	    } else if ((*it)->has(Rosegarden::Indication::
				  IndicationTypePropertyName)) {
		data2Str = QString("%1  ").
		    arg((*it)->get<Int>(Rosegarden::Indication::
					IndicationDurationPropertyName));
	    } else if ((*it)->has(Rosegarden::PitchBend::LSB)) {
                data2Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::PitchBend::LSB));
            } else if ((*it)->has(BaseProperties::BEAMED_GROUP_ID)) {
		data2Str = QString("%1  ").
		    arg((*it)->get<Int>(BaseProperties::BEAMED_GROUP_ID));
	    }

            if ((*it)->has(Rosegarden::ProgramChange::PROGRAM)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::ProgramChange::PROGRAM));
            }

            if ((*it)->has(Rosegarden::ChannelPressure::PRESSURE)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::ChannelPressure::PRESSURE));
            }

            if ((*it)->has(Rosegarden::KeyPressure::PITCH)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::KeyPressure::PITCH));
            }

            if ((*it)->has(Rosegarden::KeyPressure::PRESSURE)) {
                data2Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::KeyPressure::PRESSURE));
            }


	    if ((*it)->getDuration() > 0 ||
		(*it)->isa(Rosegarden::Note::EventType) ||
		(*it)->isa(Rosegarden::Note::EventRestType)) {
		durationStr = QString("%1  ").arg((*it)->getDuration());
	    }

            new EventViewItem(m_segments[i],
			      *it,
                              m_eventList,
                              QString("%1  ").arg(eventTime),
			      durationStr,
                              strtoqstr((*it)->getType()),
                              pitchStr,
                              velyStr,
			      data1Str,
			      data2Str);
        }
    }


    if (m_eventList->childCount() == 0)
    {
        if (m_segments.size())
            new QListViewItem(m_eventList,
                          i18n("<no events at this filter level>"));
        else
            new QListViewItem(m_eventList, i18n("<no events>"));

        m_eventList->setSelectionMode(QListView::NoSelection);
    }
    else
    {
        m_eventList->setSelectionMode(QListView::Extended);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt)
    {
        index = *sIt;

        while (index > 0 && !m_eventList->itemAtIndex(index))
                index--;

        m_eventList->setSelected(m_eventList->itemAtIndex(index), true);
        m_eventList->setCurrentItem(m_eventList->itemAtIndex(index));

        // ensure visible
        m_eventList->ensureItemVisible(m_eventList->itemAtIndex(index));
    }

    m_listSelection.clear();
    m_deletedEvents.clear();

    return true;
}

void
EventView::refreshSegment(Rosegarden::Segment * /*segment*/,
                          Rosegarden::timeT /*startTime*/,
                          Rosegarden::timeT /*endTime*/)
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
EventView::slotEditCut()
{
    QPtrList<QListViewItem> selection = m_eventList->selectedItems();

    if (selection.count() == 0) return;

    RG_DEBUG << "EventView::slotEditCut - cutting "
             << selection.count() << " items" << endl;

    QPtrListIterator<QListViewItem> it(selection);
    QListViewItem *listItem;
    EventViewItem *item;
    Rosegarden::EventSelection *cutSelection = 0;
    int itemIndex = -1;

    while((listItem = it.current()) != 0)
    {
        item = dynamic_cast<EventViewItem*>((*it));

        if (itemIndex == -1) itemIndex = m_eventList->itemIndex(*it);

        if (item)
        {
            if (cutSelection == 0)
                cutSelection = 
                    new Rosegarden::EventSelection(*(item->getSegment()));

            cutSelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (cutSelection)
    {
        if (itemIndex >= 0)
        {
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
    QPtrList<QListViewItem> selection = m_eventList->selectedItems();

    if (selection.count() == 0) return;

    RG_DEBUG << "EventView::slotEditCopy - copying "
             << selection.count() << " items" << endl;

    QPtrListIterator<QListViewItem> it(selection);
    QListViewItem *listItem;
    EventViewItem *item;
    Rosegarden::EventSelection *copySelection = 0;

    // clear the selection for post modification updating
    //
    m_listSelection.clear();

    while((listItem = it.current()) != 0)
    {
        item = dynamic_cast<EventViewItem*>((*it));

        m_listSelection.push_back(m_eventList->itemIndex(*it));

        if (item)
        {
            if (copySelection == 0)
                copySelection = 
                    new Rosegarden::EventSelection(*(item->getSegment()));

            copySelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (copySelection)
    {
        addCommandToHistory(new CopyCommand(*copySelection,
                            getDocument()->getClipboard()));
    }
}

void
EventView::slotEditPaste()
{
    if (getDocument()->getClipboard()->isEmpty()) 
    {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    Rosegarden::timeT insertionTime = 0;

    QPtrList<QListViewItem> selection = m_eventList->selectedItems();
    if (selection.count())
    {
        EventViewItem *item = dynamic_cast<EventViewItem*>(selection.at(0));

        if (item)
            insertionTime = item->getEvent()->getAbsoluteTime();
        
        // remember the selection
        //
        m_listSelection.clear();

        QPtrListIterator<QListViewItem> it(selection);
        QListViewItem *listItem;

        while((listItem = it.current()) != 0)
        {
            m_listSelection.push_back(m_eventList->itemIndex(*it));
            ++it;
        }
    }


    PasteEventsCommand *command = new PasteEventsCommand
        (*m_segments[0], getDocument()->getClipboard(),
         insertionTime, PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible())
    {
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
    else
        addCommandToHistory(command);

    RG_DEBUG << "EventView::slotEditPaste - pasting "
             << selection.count() << " items" << endl;
}

void
EventView::slotEditDelete()
{
    QPtrList<QListViewItem> selection = m_eventList->selectedItems();
    if (selection.count() == 0) return;

    RG_DEBUG << "EventView::slotEditDelete - deleting "
             << selection.count() << " items" << endl;

    QPtrListIterator<QListViewItem> it(selection);
    QListViewItem *listItem;
    EventViewItem *item;
    Rosegarden::EventSelection *deleteSelection = 0;
    int itemIndex = -1;

    while((listItem = it.current()) != 0)
    {
        item = dynamic_cast<EventViewItem*>((*it));

        if (itemIndex == -1) itemIndex = m_eventList->itemIndex(*it);

        if (item)
        {
	    if (m_deletedEvents.find(item->getEvent()) != m_deletedEvents.end()) {
		++it;
		continue;
	    }

            if (deleteSelection == 0)
                deleteSelection = 
                    new Rosegarden::EventSelection(*m_segments[0]);

            deleteSelection->addEvent(item->getEvent());
        }
        ++it;
    }

    if (deleteSelection)
    {

        if (itemIndex >= 0)
        {
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

    Rosegarden::timeT insertTime = m_segments[0]->getStartTime();
    Rosegarden::timeT insertDuration = 960;

    QPtrList<QListViewItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0)
    {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

        if (item)
        {
            insertTime = item->getEvent()->getAbsoluteTime();
            insertDuration = item->getEvent()->getDuration();
        }
    }

    // Create default event
    //
    Rosegarden::Event *event = 
        new Rosegarden::Event(Rosegarden::Note::EventType,
                              insertTime,
                              insertDuration);
    event->set<Int>(Rosegarden::BaseProperties::PITCH, 70);
    event->set<Int>(Rosegarden::BaseProperties::VELOCITY, 100);

    SimpleEventEditDialog *dialog =
        new SimpleEventEditDialog(this, getDocument(), *event, true);

    if (dialog->exec() == QDialog::Accepted)
    {
        EventInsertionCommand *command = 
            new EventInsertionCommand(*m_segments[0],
                                      new Rosegarden::Event(dialog->getEvent()));
        addCommandToHistory(command);
    }
}

void
EventView::slotEditEvent()
{
    RG_DEBUG << "EventView::slotEditEvent" << endl;

    QPtrList<QListViewItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0)
    {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

        if (item)
        {
            Rosegarden::Event *event = item->getEvent();
            SimpleEventEditDialog *dialog = 
                new SimpleEventEditDialog(this, getDocument(), *event, false);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified())
            {
                EventEditCommand *command =
                    new EventEditCommand(*(item->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }
        }
    }
}

void
EventView::slotEditEventAdvanced()
{
    RG_DEBUG << "EventView::slotEditEventAdvanced" << endl;

    QPtrList<QListViewItem> selection = m_eventList->selectedItems();

    if (selection.count() > 0)
    {
        EventViewItem *item =
            dynamic_cast<EventViewItem*>(selection.getFirst());

        if (item)
        {
            Rosegarden::Event *event = item->getEvent();
            EventEditDialog *dialog = new EventEditDialog(this, *event);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified())
            {
                EventEditCommand *command =
                    new EventEditCommand(*(item->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }
        }
    }
}


void
EventView::setupActions()
{
    EditViewBase::setupActions("eventlist.rc");
    
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/event-insert.xpm"));
    
    new KAction(i18n("&Insert Event"), icon, Key_I, this,
                SLOT(slotEditInsert()), actionCollection(),
                "insert");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/event-delete.xpm"));
    
    new KAction(i18n("&Delete Event"), icon, Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");
    
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/event-edit.xpm"));

    new KAction(i18n("&Edit Event"), icon, Key_E, this,
                SLOT(slotEditEvent()), actionCollection(),
                "edit_simple");
    
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/event-edit-advanced.xpm"));

    new KAction(i18n("&Advanced Event Editor"), icon, Key_A, this,
                SLOT(slotEditEventAdvanced()), actionCollection(),
                "edit_advanced");

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

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
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
    m_config->setGroup(ConfigGroup);
    EditViewBase::readOptions();
    m_eventFilter = m_config->readNumEntry("eventfilter", m_eventFilter);
    m_eventList->restoreLayout(m_config, LayoutConfigGroupName);
}

const char* const EventView::LayoutConfigGroupName = "EventList Layout";

void
EventView::slotSaveOptions()
{
    m_config->setGroup(ConfigGroup);
    m_config->writeEntry("eventfilter", m_eventFilter);
    m_eventList->saveLayout(m_config, LayoutConfigGroupName);
}

void 
EventView::slotModifyFilter(int button)
{
    QCheckBox *checkBox = dynamic_cast<QCheckBox*>(m_filterGroup->find(button));

    if (checkBox == 0) return;

    if (checkBox->isChecked())
    {
        switch (button)
        {
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

    }
    else
    {
        switch (button)
        {
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
EventView::slotPopupEventEditor(QListViewItem *item)
{
    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);

    if (eItem)
    {
	Rosegarden::Event *event = eItem->getEvent();
        SimpleEventEditDialog *dialog = 
            new SimpleEventEditDialog(this, getDocument(), *event, false);

        if (dialog->exec() == QDialog::Accepted && dialog->isModified())
        {
            EventEditCommand *command =
                new EventEditCommand(*(eItem->getSegment()),
                                     event,
                                     dialog->getEvent());

            addCommandToHistory(command);
        }

    }
}

void 
EventView::slotPopupMenu(QListViewItem *item, const QPoint &pos, int)
{
    if (!item) return;

    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);
    if (!eItem || !eItem->getEvent()) return;

    if (!m_menu) createMenu();

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
    m_menu->insertItem(i18n("Open in Event Editor"), 0);
    m_menu->insertItem(i18n("Open in Expert Event Editor"), 1);

    connect(m_menu, SIGNAL(activated(int)),
            SLOT(slotMenuActivated(int)));
}

void
EventView::slotMenuActivated(int value)
{
    RG_DEBUG << "EventView::slotMenuActivated - value = " << value << endl;

    if (value == 0)
    {
        EventViewItem *eItem = dynamic_cast<EventViewItem*>
            (m_eventList->currentItem());

        if (eItem)
        {
	    Rosegarden::Event *event = eItem->getEvent();
            SimpleEventEditDialog *dialog =
                new SimpleEventEditDialog(this, getDocument(), *event, false);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified())
            {
                EventEditCommand *command =
                    new EventEditCommand(*(eItem->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }

        }
    }
    else if (value == 1)
    {
        EventViewItem *eItem = dynamic_cast<EventViewItem*>
            (m_eventList->currentItem());

        if (eItem)
        {
	    Rosegarden::Event *event = eItem->getEvent();
            EventEditDialog *dialog = new EventEditDialog(this, *event);

            if (dialog->exec() == QDialog::Accepted && dialog->isModified())
            {
                EventEditCommand *command =
                    new EventEditCommand(*(eItem->getSegment()),
                                         event,
                                         dialog->getEvent());

                addCommandToHistory(command);
            }

        }
    }

    return;
}



const char* const EventView::ConfigGroup = "EventList Options";
