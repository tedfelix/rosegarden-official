// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>

#include <klistview.h>

#include "editview.h"
#include "eventview.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "editcommands.h"
#include "matrixtool.h"
#include "sequencemanager.h"

#include "Segment.h"
#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include "MidiTypes.h"


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
		  ProgramChange | PitchBend | Indication | Other)
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

    // Connect double clicker
    //
    connect(m_eventList, SIGNAL(doubleClicked(QListViewItem*)),
            SLOT(slotPopupEventEditor(QListViewItem*)));

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QListView::Single);

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
    // Give the sequencer something to suck on while we close
    //
    getDocument()->getSequenceManager()->
        setTemporarySequencerSliceSize(Rosegarden::RealTime(2, 0));
}

bool
EventView::applyLayout(int /*staffNo*/)
{
    // Remember list position if we've selected something
    //
    int lastTime = 0;
    int lastDuration = 0;
    bool lastEventSelected = false;

    if (m_eventList->selectedItem())
    {
        lastEventSelected = true;
        lastTime = m_eventList->selectedItem()->text(0).toInt();
        lastDuration = m_eventList->selectedItem()->text(1).toInt();
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

            // Select the same event as the one we had selected before
            //
            if (lastEventSelected &&
                lastTime == eventTime &&
                lastDuration == (*it)->getDuration())
            {
                m_eventList->setSelected(m_eventList->lastItem(), true);
            }
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
        m_eventList->setSelectionMode(QListView::Single);

    return true;
}

void
EventView::refreshSegment(Rosegarden::Segment * /*segment*/,
                          Rosegarden::timeT /*startTime*/,
                          Rosegarden::timeT /*endTime*/)
{
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
}

void
EventView::slotEditCopy()
{
}

void
EventView::slotEditPaste()
{
}

void
EventView::setupActions()
{
    EditViewBase::setupActions("eventlist.rc");

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
    m_config->setGroup("EventList Options");
    EditViewBase::readOptions();
    m_eventFilter = m_config->readNumEntry("eventfilter", m_eventFilter);
    m_eventList->restoreLayout(m_config, LayoutConfigGroupName);
}

const char* const EventView::LayoutConfigGroupName = "EventList Layout";

void
EventView::slotSaveOptions()
{
    m_config->setGroup("EventList Options");
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
    //Rosegarden::Composition &comp = m_doc->getComposition();

    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);

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
