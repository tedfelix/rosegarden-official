// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>

#include <klistview.h>

#include "eventview.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "editcommands.h"

#include "Segment.h"
#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include "MidiTypes.h"


using Rosegarden::Int;
using Rosegarden::BaseProperties;

// EventView specialisation of a QListViewItem with the
// addition of a segment pointer
//
class EventViewItem : public QListViewItem
{
public:
    EventViewItem(Rosegarden::Segment *segment,
                  QListView *parent):QListViewItem(parent),
                                     m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListViewItem *parent):QListViewItem(parent),
                                         m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListView *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null)
        :QListViewItem(parent, label1, label2, label3, label4,
                       label5, label6, label7, label8), m_segment(segment) {;}

    EventViewItem(Rosegarden::Segment *segment,
                  QListViewItem *parent, QString label1,
                  QString label2 = QString::null,
                  QString label3 = QString::null,
                  QString label4 = QString::null,
                  QString label5 = QString::null,
                  QString label6 = QString::null,
                  QString label7 = QString::null,
                  QString label8 = QString::null)
        :QListViewItem(parent, label1, label2, label3, label4,
                       label5, label6, label7, label8), m_segment(segment) {;}

    void setSegment(Rosegarden::Segment *segment) { m_segment = segment; }
    Rosegarden::Segment* getSegment() { return m_segment; }

    // Reimplement so that we can sort numerically
    //
    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:

    Rosegarden::Segment *m_segment;
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

EventView::EventView(RosegardenGUIDoc *doc,
                     std::vector<Rosegarden::Segment *> segments,
                     QWidget *parent):
    EditViewBase(doc, segments, 2, parent, "eventview"),
    m_eventFilter(Note|Text|SysEx|Controller|ProgramChange|PitchBend),
    m_doc(doc)
{

    readOptions();

    QVBox *filterBox = new QVBox(getCentralFrame());
    m_grid->addWidget(filterBox, 2, 0);
    filterBox->setSpacing(5);

    /*
    QLabel *label = new QLabel(i18n("Filters"), filterBox);
    label->setAlignment(Qt::AlignHCenter);
    label->setFixedHeight(20);
    */

    // define some note filtering buttons in a group
    //
    m_filterGroup =
        new QButtonGroup(1, Horizontal, i18n("Event filters"), filterBox);

    m_noteCheckBox = new QCheckBox(i18n("Note"), m_filterGroup);
    m_programCheckBox = new QCheckBox(i18n("Program Change"), m_filterGroup);
    m_controllerCheckBox = new QCheckBox(i18n("Controller"), m_filterGroup);
    m_pitchBendCheckBox = new QCheckBox(i18n("Pitch Bend"), m_filterGroup);
    m_sysExCheckBox = new QCheckBox(i18n("System Exclusive"), m_filterGroup);
    m_textCheckBox = new QCheckBox(i18n("Text"), m_filterGroup);
    m_restCheckBox = new QCheckBox(i18n("Rest"), m_filterGroup);

    // Connect up
    //
    connect(m_filterGroup, SIGNAL(released(int)),
            SLOT(slotModifyFilter(int)));

    m_eventList = new KListView(getCentralFrame());
    m_grid->addWidget(m_eventList, 2, 1);

    if (segments.size() == 1)
    {
        setCaption(QString(i18n("%1  - Event List for Segment Track #%2"))
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    }
    else
    {
        setCaption(i18n("Event List"));
    }

    // Connect double clicker
    //
    connect(m_eventList, SIGNAL(doubleClicked(QListViewItem*)),
            SLOT(slotPopupEventEditor(QListViewItem*)));

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QListView::Single);

    m_eventList->addColumn(i18n("Time  "));
    m_eventList->addColumn(i18n("Duration  "));
    m_eventList->addColumn(i18n("Type  "));
    m_eventList->addColumn(i18n("Pitch  "));
    m_eventList->addColumn(i18n("Velocity  "));
    m_eventList->addColumn(i18n("Data1  "));
    m_eventList->addColumn(i18n("Data2  "));

    setButtonsToFilter();
    applyLayout();
}

EventView::~EventView()
{
}

bool
EventView::applyLayout(int /*staffNo*/)
{
    m_eventList->clear();

    for (unsigned int i = 0; i < m_segments.size(); i++)
    {
        Rosegarden::SegmentPerformanceHelper helper(*m_segments[i]);

        for (Rosegarden::Segment::iterator it = m_segments[i]->begin();
                               it != m_segments[i]->end(); it++)
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
            if((*it)->isa(Rosegarden::Note::EventRestType) &&
               !(m_eventFilter & Rest))
                continue;

            if((*it)->isa(Rosegarden::Note::EventType) &&
               !(m_eventFilter & Note))
                continue;

            if((*it)->isa(Rosegarden::PitchBend::EventType) &&
               !(m_eventFilter & PitchBend))
                continue;

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

            if ((*it)->has(Rosegarden::Controller::DATA1)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::Controller::DATA1));
            }

            if ((*it)->has(Rosegarden::Controller::DATA2)) {
                data2Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::Controller::DATA2));
            }

            if ((*it)->has(Rosegarden::PitchBend::MSB)) {
                data1Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::PitchBend::MSB));
            }

            if ((*it)->has(Rosegarden::PitchBend::LSB)) {
                data2Str = QString("%1  ").
                    arg((*it)->get<Int>(Rosegarden::PitchBend::LSB));
            }

	    if ((*it)->getDuration() > 0 ||
		(*it)->isa(Rosegarden::Note::EventType) ||
		(*it)->isa(Rosegarden::Note::EventRestType)) {
		durationStr = QString("%1  ").arg((*it)->getDuration());
	    }

            new EventViewItem(m_segments[i],
                              m_eventList,
                              QString("%1").arg(eventTime),
                              QString("%1").arg((*it)->getDuration()),
                              QString((*it)->getType().c_str()),
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
}

void
EventView::initStatusBar()
{
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
    m_eventList->restoreLayout(m_config, LayoutConfigGroupName);
}

const char* const EventView::LayoutConfigGroupName = "EventList Layout";

void
EventView::slotSaveOptions()
{
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
                m_eventFilter |= EventView::Rest;
                break;

            case 2:
                m_eventFilter |= EventView::PitchBend;
                break;

            case 3:
                m_eventFilter |= EventView::ProgramChange;
                break;

            case 4:
                m_eventFilter |= EventView::Controller;
                break;

            case 5:
                m_eventFilter |= EventView::SysEx;
                break;

            case 6:
                m_eventFilter |= EventView::Text;
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
                m_eventFilter ^= EventView::Rest;
                break;

            case 2:
                m_eventFilter ^= EventView::PitchBend;
                break;

            case 3:
                m_eventFilter ^= EventView::ProgramChange;
                break;

            case 4:
                m_eventFilter ^= EventView::Controller;
                break;

            case 5:
                m_eventFilter ^= EventView::SysEx;
                break;

            case 6:
                m_eventFilter ^= EventView::Text;
                break;

            default:
                break;
        }
    }

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

    if (m_eventFilter & SysEx)
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

}

void
EventView::slotPopupEventEditor(QListViewItem *item)
{
    //Rosegarden::Composition &comp = m_doc->getComposition();

    EventViewItem *eItem = dynamic_cast<EventViewItem*>(item);

    if (eItem)
    {
        // For the moment just get one event
        //
        Rosegarden::Segment::iterator it = eItem->getSegment()->
            findTime(eItem->text(0).toInt());

        do
        {
            // if types don't match then return
            if ((*it)->getType() != std::string(eItem->text(2).data()))
                continue;

            // try to match durations
            if ((*it)->getDuration() != eItem->text(1).toInt())
                continue;

            if((*it)->isa(Rosegarden::Note::EventRestType))
            {
                break;
            }

            if((*it)->isa(Rosegarden::Note::EventType))
            {
                // check pitch
	        if ((*it)->has(BaseProperties::PITCH) &&
                   ((*it)->get<Int>(BaseProperties::PITCH)
                        == eItem->text(3).toInt()))
                    break;
            }


            if ((*it)->getAbsoluteTime() > eItem->text(0).toInt())
            {
                std::cerr << "EventView::slotPopupEventEditor - "
                          << "couldn't find event" << std::endl;
                return;
            }

        }
        while (++it != eItem->getSegment()->end());

        // Ok, pop up and execute dialog and perform command if 
        // we've modified the event.
        //
        EventEditDialog *dialog = new EventEditDialog(this, **it);

        if (dialog->exec() == QDialog::Accepted && dialog->isModified())
        {
            EventEditCommand *command =
                new EventEditCommand(*(eItem->getSegment()),
                                     (*it),
                                     dialog->getEvent());

            addCommandToHistory(command);
        }

    }
}
