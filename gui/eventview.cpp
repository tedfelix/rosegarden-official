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
#include <qlistview.h>
#include <qpushbutton.h>

#include "eventview.h"
#include "rosegardenguidoc.h"
#include "Segment.h"
#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"


using Rosegarden::Int;
using Rosegarden::BaseProperties;

EventView::EventView(RosegardenGUIDoc *doc,
                     std::vector<Rosegarden::Segment *> segments,
                     QWidget *parent):
    EditViewBase(doc, segments, 2, parent, "eventview"),
    m_eventFilter(Note|Text|SysEx|Controller|ProgramChange)
{

    readOptions();

    QVBox *filterBox = new QVBox(getCentralFrame());
    m_grid->addWidget(filterBox, 2, 0);

    // define some note filtering buttons
    //
    m_noteFilter = new QPushButton(i18n("Note"), filterBox);
    m_programFilter = new QPushButton(i18n("Program Change"), filterBox);
    m_controllerFilter = new QPushButton(i18n("Controller"), filterBox);
    m_sysExFilter = new QPushButton(i18n("Systerm Exclusive"), filterBox);
    m_textFilter = new QPushButton(i18n("Text"), filterBox);

    m_noteFilter->setToggleButton(true);
    connect(m_noteFilter, SIGNAL(toggled(bool)),
            SLOT(slotNoteFilter(bool)));

    m_programFilter->setToggleButton(true);
    connect(m_programFilter, SIGNAL(toggled(bool)),
            SLOT(slotProgramFilter(bool)));

    m_controllerFilter->setToggleButton(true);
    connect(m_controllerFilter, SIGNAL(toggled(bool)),
            SLOT(slotControllerFilter(bool)));

    m_sysExFilter->setToggleButton(true);
    connect(m_sysExFilter, SIGNAL(toggled(bool)),
            SLOT(slotSetSysExFilter(bool)));

    m_textFilter->setToggleButton(true);
    connect(m_textFilter, SIGNAL(toggled(bool)),
            SLOT(slotTextFilter(bool)));

    m_eventList = new QListView(getCentralFrame());
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

    m_eventList->setAllColumnsShowFocus(true);
    m_eventList->setSelectionMode(QListView::Single);

    m_eventList->addColumn(i18n("Time"));
    m_eventList->addColumn(i18n("Duration"));
    m_eventList->addColumn(i18n("Event"));
    m_eventList->addColumn(i18n("Note (Data1)"));
    m_eventList->addColumn(i18n("Velocity (Data2)"));

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

            try
            {
                pitchStr = QString("%1").
                    arg((*it)->get<Int>(BaseProperties::PITCH));
                velyStr = QString("%1").
                    arg((*it)->get<Int>(BaseProperties::VELOCITY));
            }
            catch(...)
            {
                velyStr = i18n("<not set>");
            }

            new QListViewItem(m_eventList,
                              QString("%1").arg(eventTime),
                              QString("%1").arg((*it)->getDuration()),
                              QString((*it)->getType().c_str()),
                              pitchStr,
                              velyStr);


        }
    }

    return true;
}

void
EventView::refreshSegment(Rosegarden::Segment *segment,
                          Rosegarden::timeT startTime,
                          Rosegarden::timeT endTime)
{
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
    
}

void
EventView::slotNoteFilter(bool value)
{
    if (value)
        m_eventFilter |= EventView::Note;
    else
        m_eventFilter ^= EventView::Note;

    applyLayout(0);
    setButtonsToFilter();
}


void
EventView::slotProgramFilter(bool value)
{
    if (value)
        m_eventFilter |= EventView::ProgramChange;
    else
        m_eventFilter ^= EventView::ProgramChange;

    applyLayout(0);
    setButtonsToFilter();
}

void
EventView::slotControllerFilter(bool value)
{
    if (value)
        m_eventFilter |= EventView::Controller;
    else
        m_eventFilter ^= EventView::Controller;

    applyLayout(0);
    setButtonsToFilter();
}


void
EventView::slotSysExFilter(bool value)
{
    if (value)
        m_eventFilter |= EventView::SysEx;
    else
        m_eventFilter ^= EventView::SysEx;

    applyLayout(0);
    setButtonsToFilter();
}

void
EventView::slotTextFilter(bool value)
{
    if (value)
        m_eventFilter |= EventView::Text;
    else
        m_eventFilter ^= EventView::Text;

    applyLayout(0);
    setButtonsToFilter();
}


void
EventView::setButtonsToFilter()
{
    if (m_eventFilter & Note)
        m_noteFilter->setDown(true);
    else
        m_noteFilter->setDown(false);

    if (m_eventFilter & ProgramChange)
        m_programFilter->setDown(true);
    else
        m_programFilter->setDown(false);

    if (m_eventFilter & Controller)
        m_controllerFilter->setDown(true);
    else
        m_controllerFilter->setDown(false);

    if (m_eventFilter & SysEx)
        m_sysExFilter->setDown(true);
    else
        m_sysExFilter->setDown(false);

    if (m_eventFilter & Text)
        m_textFilter->setDown(true);
    else
        m_textFilter->setDown(false);

}

