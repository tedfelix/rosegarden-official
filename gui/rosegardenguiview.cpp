// -*- c-basic-offset: 4 -*-

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

    connect(tracksEditor->canvas(), SIGNAL(editTrack(Rosegarden::Track*)),
            SLOT(editTrackNotation(Rosegarden::Track*)));

    connect(tracksEditor,  SIGNAL(createNewTrack(TrackItem*,int)),
            getDocument(), SLOT  (createNewTrack(TrackItem*,int)));

    connect(this,                   SIGNAL(setTool(TracksCanvas::ToolType)),
            tracksEditor->canvas(), SLOT(setTool(TracksCanvas::ToolType)));

    connect(this,                   SIGNAL(setPositionPointer(int)),
            tracksEditor,           SLOT(setPointerPosition(int)));

    if (doc)
        tracksEditor->setupTracks();

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
    emit setTool(TracksCanvas::Pencil);
}

void RosegardenGUIView::eraseSelected()
{
    emit setTool(TracksCanvas::Eraser);
}

void RosegardenGUIView::moveSelected()
{
    emit setTool(TracksCanvas::Mover);
}

void RosegardenGUIView::resizeSelected()
{
    emit setTool(TracksCanvas::Resizer);
}


void
RosegardenGUIView::editTrackNotation(Rosegarden::Track* p)
{
    std::vector<Rosegarden::Track *> tracksToEdit;
    tracksToEdit.push_back(p);

    m_notationView = new NotationView(getDocument(), tracksToEdit, this, 8);
    m_notationView->show();
}

void
RosegardenGUIView::editAllTracks(Rosegarden::Composition* p)
{
    std::vector<Rosegarden::Track *> tracksToEdit;

    for (Rosegarden::Composition::trackcontainer::iterator i =
             p->tracks().begin(); i != p->tracks().end(); ++i) {
        tracksToEdit.push_back(*i);
    }

    m_notationView = new NotationView(getDocument(), tracksToEdit, this, 8);
    m_notationView->show();
}

void
RosegardenGUIView::setPointerPosition(const int &position)
{
  emit setPositionPointer(position);
}
