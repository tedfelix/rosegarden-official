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
#include <kmessagebox.h>

// application specific includes
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "notationview.h"
#include "matrixview.h"


RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char* /*name*/)
    : QVBox(parent),
      m_notationView(0),
      m_matrixView(0)
{
    QScrollView *scrollView = new QScrollView(this);
    
    RosegardenGUIDoc* doc = getDocument();
    TrackEditor *tracksEditor = 0;
    
    if (doc) {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : doc != 0\n";
        tracksEditor = new TrackEditor(doc, this);

    } else {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : no doc\n";
        tracksEditor = new TrackEditor(12, 50, this);

    }
    
    scrollView->addChild(tracksEditor);

    connect(tracksEditor->canvas(), SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(editSegmentNotation(Rosegarden::Segment*)));

    connect(tracksEditor->canvas(), SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(editSegmentMatrix(Rosegarden::Segment*)));

    connect(tracksEditor,  SIGNAL(createNewSegment(SegmentItem*,int)),
            getDocument(), SLOT  (createNewSegment(SegmentItem*,int)));

    connect(this,                   SIGNAL(setTool(SegmentCanvas::ToolType)),
            tracksEditor->canvas(), SLOT(setTool(SegmentCanvas::ToolType)));

    connect(this,                   SIGNAL(setPositionPointer(int)),
            tracksEditor,           SLOT(setPointerPosition(int)));

    if (doc)
        tracksEditor->setupSegments();

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
    emit setTool(SegmentCanvas::Pencil);
}

void RosegardenGUIView::eraseSelected()
{
    emit setTool(SegmentCanvas::Eraser);
}

void RosegardenGUIView::moveSelected()
{
    emit setTool(SegmentCanvas::Mover);
}

void RosegardenGUIView::resizeSelected()
{
    emit setTool(SegmentCanvas::Resizer);
}


void
RosegardenGUIView::editSegmentNotation(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    m_notationView = new NotationView(getDocument(), segmentsToEdit, this);
    m_notationView->show();
}

void
RosegardenGUIView::editSegmentMatrix(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    m_matrixView = new MatrixView(getDocument(), segmentsToEdit, this);
    m_matrixView->show();
}

void
RosegardenGUIView::editAllTracks(Rosegarden::Composition* p)
{
    if (p->getNbSegments() == 0) {
	KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
	return;
    }

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Rosegarden::Composition::iterator i = p->begin(); i != p->end(); ++i) {
        segmentsToEdit.push_back(*i);
    }

    m_notationView = new NotationView(getDocument(), segmentsToEdit, this);
    m_notationView->show();
}

void
RosegardenGUIView::setPointerPosition(const int &position)
{
//    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::setPointerPosition" << endl;

    emit setPositionPointer(position);
}
