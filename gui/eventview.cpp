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

#include <qvbox.h>
#include <qlayout.h>
#include <qlistview.h>
#include <klocale.h>

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
    EditView(doc, segments, 2, parent, "eventview")
{

    m_horizontalScrollBar->setDisabled(true);

    QVBox *filterBox = new QVBox(getCentralFrame());
    m_grid->addWidget(filterBox, 2, 0);

    m_eventList = new QListView(getCentralFrame());
    m_grid->addWidget(m_eventList, 2, 1);

    if (segments.size() == 1)
    {
        setCaption(QString("%1  - Event List for Segment Track #%2")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    }
    else
    {
        setCaption(i18n("Event List"));
    }


    m_eventList->addColumn(i18n("Time"));
    m_eventList->addColumn(i18n("Event"));
    m_eventList->addColumn(i18n("Duration"));
    m_eventList->addColumn(i18n("Note (Data1)"));
    m_eventList->addColumn(i18n("Velocity (Data2)"));

    applyLayout();
}

EventView::~EventView()
{
}

bool
EventView::applyLayout(int /*staffNo*/)
{


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
                              QString((*it)->getType().c_str()),
                              QString("%1").arg((*it)->getDuration()),
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



