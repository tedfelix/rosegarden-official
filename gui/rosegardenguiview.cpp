/***************************************************************************
                          rosegardenguiview.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 23:41:03 CEST 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// include files for Qt
#include <qprinter.h>

// KDE includes
#include <kdebug.h>

// application specific includes
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackseditor.h"
#include "trackscanvas.h"
#include "notationview.h"

RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char* /*name*/)
    : QVBox(parent),
      m_notationView(0)
{
    QScrollView *scrollView = new QScrollView(this);
    
    RosegardenGUIDoc* doc = getDocument();
    TracksEditor *tracksEditor = 0;
    
    if (doc) {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : doc != 0\n";
        tracksEditor = new TracksEditor(doc, this);

    } else {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : no doc\n";
        tracksEditor = new TracksEditor(12, 50, this);

    }
    
    scrollView->addChild(tracksEditor);

    connect(tracksEditor->canvas(), SIGNAL(editTrackPart(TrackPart*)),
            SLOT(editTrackNotation(TrackPart*)));

    connect(tracksEditor,  SIGNAL(createNewTrack(unsigned int, unsigned int, unsigned int)),
            getDocument(), SLOT  (createNewTrack(unsigned int, unsigned int, unsigned int)));

//     if (getDocument()) {
        
//         m_notationView = new NotationView(getDocument(), this);
 
//         m_notationView->show();
//     }
}


RosegardenGUIView::~RosegardenGUIView()
{
    kdDebug(KDEBUG_AREA) << "~RosegardenGUIView()\n";
}

RosegardenGUIDoc*
RosegardenGUIView::getDocument() const
{
    QWidget *t = parentWidget();
    
    if (!t) {
        kdDebug(KDEBUG_AREA) << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is 0\n";
        return 0;
    }

    RosegardenGUIApp *theApp = dynamic_cast<RosegardenGUIApp*>(t);
    
    if (!theApp) {
        kdDebug(KDEBUG_AREA) << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is of the wrong type\n";
        return 0;
    }
    
    return theApp->getDocument();
}

void
RosegardenGUIView::print(QPrinter *pPrinter)
{
    QPainter printpainter;
    printpainter.begin(pPrinter);
	
    // TODO: add your printing code here

    printpainter.end();
}

void
RosegardenGUIView::editTrackNotation(TrackPart* p)
{
    unsigned int trackNb = p->trackNb();
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::editTrackNotation() : showing track " << trackNb << endl;

    m_notationView = new NotationView(getDocument(), trackNb, this);
 
    m_notationView->show();
}
