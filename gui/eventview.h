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

#ifndef _EVENTVIEW_H_
#define _EVENTVIEW_H_

// Event view list
//
//


#include <vector>

#include <qlistview.h>

#include "editviewbase.h"

class RosegardenGUIDoc;

class EventView : public EditViewBase
{
    Q_OBJECT
public:
    EventView(RosegardenGUIDoc *doc,
              std::vector<Rosegarden::Segment *> segments,
              QWidget *parent);

    virtual ~EventView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    virtual void updateView();
    virtual void slotEditCut();
    virtual void slotEditCopy();
    virtual void slotEditPaste();
    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);


signals:    
public slots:
protected:

    virtual void readOptions();


    //--------------- Data members ---------------------------------
    QListView *m_eventList;

};

#endif

