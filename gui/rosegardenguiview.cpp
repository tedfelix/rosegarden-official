
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
    connect(tracksEditor->canvas(), SIGNAL(editTrackPartSmall(TrackPart*)),
            SLOT(editTrackNotationSmall(TrackPart*)));

    connect(tracksEditor,  SIGNAL(createNewTrack(unsigned int, unsigned int, unsigned int)),
            getDocument(), SLOT  (createNewTrack(unsigned int, unsigned int, unsigned int)));

    connect(this,                   SIGNAL(setTool(TracksCanvas::ToolType)),
            tracksEditor->canvas(), SLOT(setTool(TracksCanvas::ToolType)));

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

void RosegardenGUIView::print(QPrinter *pPrinter)
{
    QPainter printpainter;
    printpainter.begin(pPrinter);
	
    // TODO: add your printing code here

    printpainter.end();
}

void RosegardenGUIView::drawSelected()
{
    //m_tracksEditor->canvas()->setTool(TracksCanvas::Pencil);
    emit setTool(TracksCanvas::Pencil);
}

void RosegardenGUIView::eraseSelected()
{
    //m_tracksEditor->canvas()->setTool(TracksCanvas::Eraser);
    emit setTool(TracksCanvas::Eraser);
}

void RosegardenGUIView::moveSelected()
{
    //m_tracksEditor->canvas()->setTool(TracksCanvas::Mover);
    emit setTool(TracksCanvas::Mover);
}



void
RosegardenGUIView::editTrackNotation(TrackPart* p)
{
    unsigned int trackNb = p->trackNb();
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::editTrackNotation() : showing track " << trackNb << endl;

    m_notationView = new NotationView(getDocument(), trackNb, this, 9);
 
    m_notationView->show();
}

void
RosegardenGUIView::editTrackNotationSmall(TrackPart* p)
{
    unsigned int trackNb = p->trackNb();
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::editTrackNotationSmall() : showing track " << trackNb << endl;

    m_notationView = new NotationView(getDocument(), trackNb, this, 5);
 
    m_notationView->show();
}
