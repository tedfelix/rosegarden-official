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

// include files for Qt
#include <qprinter.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>

// application specific includes
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "notationview.h"
#include "matrixview.h"
#include "trackbuttons.h"
#include "barbuttons.h"


RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char* /*name*/)
    : QVBox(parent),
      m_notationView(0),
      m_matrixView(0),
      m_trackEditorScrollView(0)
{
    RosegardenGUIDoc* doc = getDocument();

    // Construct the tracksEditor first so we can then
    // query it for placement information
    //
    TrackEditor *tracksEditor = 0;
    
    if (doc) {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : doc != 0\n";
        tracksEditor = new TrackEditor(doc, this);

    } else {

        kdDebug(KDEBUG_AREA) << "RosegardenGUIView() : no doc\n";
        tracksEditor = new TrackEditor(12, 50, this);
    }
    
    // Construct the top level horizontal box and drop the
    // button box (TrackButtons) and the main ScrollView
    // straight into it.  We use this arrangement until we
    // get around to specialising the QHeader properly.
    //
    //
    QHBox *topBox = new QHBox(this);

    // --------------- create the track buttons ----------------
    //
    //
    QScrollView *trackButtonsView = new QScrollView(topBox);
    TrackButtons *trackButtons = new TrackButtons(doc,
                                                  trackButtonsView,
                                                  tracksEditor->getVHeader(),
                                                  tracksEditor->getHHeader());
    trackButtonsView->addChild(trackButtons);

    connect((QObject *)trackButtons, SIGNAL(trackSelected(int)),
                                     SLOT(selectTrackSegments(int)));

    connect(this,         SIGNAL(signalSetSelectAdd(bool)), 
            tracksEditor, SLOT(setSelectAdd(bool)));
            
    connect(this,         SIGNAL(signalSetSelectCopy(bool)), 
            tracksEditor, SLOT(setSelectCopy(bool)));
            
    // turn off the scrollbars on the track buttons and set width
    //
    trackButtonsView->setHScrollBarMode(QScrollView::AlwaysOff);
    trackButtonsView->setVScrollBarMode(QScrollView::AlwaysOff);
    trackButtonsView->setMinimumWidth(125);
    trackButtonsView->setMaximumWidth(125);

    // --------------- create the bar buttons ----------------
    //
    // We create the barbuttons and the man segmentview/trackeditor
    // in a vertical layout box.
    //
    //
/*
    QVBox       *rhsPane = new QVBox(topBox);

    QScrollView *barButtonsView = new QScrollView(rhsPane);

    BarButtons *barButtons = new BarButtons(doc,
                                            barButtonsView,
                                            tracksEditor->getVHeader(),
                                            tracksEditor->getHHeader());

    barButtonsView->setHScrollBarMode(QScrollView::AlwaysOff);
    barButtonsView->setVScrollBarMode(QScrollView::AlwaysOff);

    barButtonsView->
        setMinimumHeight(tracksEditor->getVHeader()->sectionSize(0));
    barButtonsView->
        setMaximumHeight(tracksEditor->getVHeader()->sectionSize(0));

    barButtonsView->addChild(barButtons);
*/
   
    // --------------- create the scrollview ----------------
    //
    //
    m_trackEditorScrollView = new QScrollView(topBox); // rhspane

    // Now link up the vertical scrollbar to the track buttons
    //
    connect(m_trackEditorScrollView, SIGNAL(contentsMoving(int, int)),
            trackButtonsView,        SLOT(setContentsPos(int, int)));

    m_trackEditorScrollView->addChild(tracksEditor);

    connect(tracksEditor->canvas(), SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(editSegmentNotation(Rosegarden::Segment*)));

    connect(tracksEditor->canvas(), SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(editSegmentMatrix(Rosegarden::Segment*)));

    connect(tracksEditor,  SIGNAL(createNewSegment(SegmentItem*,int)),
            getDocument(), SLOT  (createNewSegment(SegmentItem*,int)));

    connect(tracksEditor,  SIGNAL(scrollHorizTo(int)),
            SLOT(scrollTrackEditorHoriz(int)));

    connect(this,                   SIGNAL(setTool(SegmentCanvas::ToolType)),
            tracksEditor->canvas(), SLOT  (setTool(SegmentCanvas::ToolType)));

    connect(this,                   SIGNAL(setPositionPointer(int)),
            tracksEditor,           SLOT(setPointerPosition(int)));

    connect(this, SIGNAL(selectSegments(list<Rosegarden::Segment*>)),
            tracksEditor->canvas(), SLOT(selectSegments(list<Rosegarden::Segment*>)));

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

void RosegardenGUIView::pointerSelect()
{
    emit setTool(SegmentCanvas::Selector);
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


void RosegardenGUIView::editSegmentNotation(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    m_notationView = new NotationView(this, segmentsToEdit, this);
    m_notationView->show();
}

void RosegardenGUIView::editSegmentMatrix(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    m_matrixView = new MatrixView(getDocument(), segmentsToEdit, this);
    m_matrixView->show();
}

// This scrolling model pages the SegmentCanvas across the screen
//
//
void RosegardenGUIView::scrollTrackEditorHoriz(int hpos)
{
    if (hpos == 0) // If returning to zero
    {
        m_trackEditorScrollView->ensureVisible(0, m_trackEditorScrollView->contentsY() + m_trackEditorScrollView->visibleHeight()/2);
    }
    else // if moving off the right hand side of the view
    if (hpos >  ( m_trackEditorScrollView->contentsX() + 
                  m_trackEditorScrollView->visibleWidth() * 0.9 ) )
    { 
               
        m_trackEditorScrollView->scrollBy(
                (int)(m_trackEditorScrollView->visibleWidth() * 0.8),
                0);
    }
    else // if moving off the left hand side
    if (hpos < ( m_trackEditorScrollView->contentsX() +
                  m_trackEditorScrollView->visibleWidth() * 0.1 ) )
    {
        m_trackEditorScrollView->scrollBy(
                (int)(-m_trackEditorScrollView->visibleWidth() * 0.8),
                0);
    }
    
}

void RosegardenGUIView::editAllTracks(Rosegarden::Composition* p)
{
    if (p->getNbSegments() == 0) {
	KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
	return;
    }

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Rosegarden::Composition::iterator i = p->begin(); i != p->end(); ++i) {
        segmentsToEdit.push_back(*i);
    }

    m_notationView = new NotationView(this, segmentsToEdit, this);
    m_notationView->show();
}

void RosegardenGUIView::setPointerPosition(const int &position)
{
//    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::setPointerPosition" << endl;

    emit setPositionPointer(position);
}

// Highlight all the Segments on a Track because the Track has been selected
// We have to ensure we create a Selector object before we can highlight
// these tracks.
//
//
void RosegardenGUIView::selectTrackSegments(int trackId)
{
    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    emit activateTool(SegmentCanvas::Selector);

    list<Rosegarden::Segment*> segments;

    for (Rosegarden::Composition::iterator i =
              getDocument()->getComposition().begin();
         i != getDocument()->getComposition().end(); i++)
    {
        if (((int)(*i)->getTrack()) == trackId)
            segments.push_back(*i);
    }

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    emit selectSegments(segments);
}


