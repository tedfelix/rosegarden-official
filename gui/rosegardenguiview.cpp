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
#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <kprinter.h>

// application specific includes
#include "MappedEvent.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "notationview.h"
#include "matrixview.h"
#include "trackbuttons.h"
#include "barbuttons.h"
#include "RulerScale.h"

using Rosegarden::SimpleRulerScale;


RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char* /*name*/)
    : QVBox(parent),
      m_trackEditorScrollView(0)
{
    RosegardenGUIDoc* doc = getDocument();

    // This apparently arbitrary figure is what we think is an
    // appropriate width in pixels for a 4/4 bar.  Beware of making it
    // too narrow, as shorter bars will be proportionally smaller --
    // the visual difference between 2/4 and 4/4 is perhaps greater
    // than it sounds.

    double barWidth44 = 100.0;  // so random, so rare
    double unitsPerPixel =
        Rosegarden::TimeSignature(4, 4).getBarDuration() / barWidth44;

    Rosegarden::Composition *comp = &doc->getComposition();
    m_rulerScale = new SimpleRulerScale(comp, 0, unitsPerPixel);
    
    // Construct the trackEditor first so we can then
    // query it for placement information
    //
    m_trackEditor  = new TrackEditor(doc, m_rulerScale, this);

    // --------------- create the bar buttons ----------------
    //
    // We create the barbuttons and the man segmentview/trackeditor
    // in a vertical layout box.
    //
    //

    // Set a track label width for both specialised track and
    // bar buttons widgets to use.
    //
//     int trackLabelWidth = 156;

//     QVBox       *topBox = new QVBox(this);
//     QHBox       *topSplit = new QHBox(topBox);

//     topBox->setSpacing(0);
//     topBox->setMargin(0);
//     topSplit->setMinimumHeight(m_trackEditor->getTrackCellHeight());
//     topSplit->setMaximumHeight(m_trackEditor->getTrackCellHeight());
//     topSplit->setSpacing(0);
//     topSplit->setMargin(0);
//     topSplit->setFrameStyle(Plain);

//     QLabel *label = new QLabel(topSplit);
//     label->setMinimumWidth(trackLabelWidth);
//     label->setMaximumWidth(trackLabelWidth);
//     label->setMinimumHeight(m_trackEditor->getTrackCellHeight());
//     label->setMaximumHeight(m_trackEditor->getTrackCellHeight());

//     QScrollView *barButtonsView = new QScrollView(topSplit);

//     barButtonsView->setHScrollBarMode(QScrollView::AlwaysOff);
//     barButtonsView->setVScrollBarMode(QScrollView::AlwaysOff);

//     m_barButtons = new BarButtons
//         (doc,
//          m_rulerScale,
//          m_trackEditor->getVHeader()->sectionSize(0),
// 	 false, 
//          m_trackEditor->getSegmentCanvas());
//     m_barButtons->setGeometry(0,0, m_trackEditor->getSegmentCanvas()->viewport()->width(),
//                               20);

    // set a plain frame for the scrollview
//     barButtonsView->setFrameStyle(Plain);

//     barButtonsView->
//         setMinimumHeight(m_trackEditor->getVHeader()->sectionSize(0));
//     barButtonsView->
//         setMaximumHeight(m_trackEditor->getVHeader()->sectionSize(0));

//     barButtonsView->addChild(m_barButtons);

    // Construct the top level horizontal box and drop the
    // button box (TrackButtons) and the main ScrollView
    // straight into it.  We use this arrangement until we
    // get around to specialising the QHeader properly.
    //
    //
//     QHBox *mainPane = new QHBox(topBox);

    // --------------- create the track buttons ----------------
    //
    //
//     QScrollView *trackButtonsView = new QScrollView(mainPane);
//     m_trackButtons = new TrackButtons(doc,
//                                       m_trackEditor->getTrackCellHeight(),
//                                       trackLabelWidth,
//                                       trackButtonsView);

//     trackButtonsView->addChild(m_trackButtons);
//     trackButtonsView->setFrameStyle(Plain);

//     connect(m_trackButtons, SIGNAL(trackSelected(int)),
//             this,         SLOT(selectTrackSegments(int)));

//     // turn off the scrollbars on the track buttons and set width
//     //
//     trackButtonsView->setHScrollBarMode(QScrollView::AlwaysOff);
//     trackButtonsView->setVScrollBarMode(QScrollView::AlwaysOff);
//     trackButtonsView->setMinimumWidth(trackLabelWidth);
//     trackButtonsView->setMaximumWidth(trackLabelWidth);

   
    // --------------- create the scrollview ----------------
    //
    //
//     m_trackEditorScrollView = new QScrollView(mainPane); // on the rhs

    // Now link up the vertical scrollbar to the track buttons
    // and the bar buttons
    //
//     connect(m_trackEditorScrollView, SIGNAL(contentsMoving(int, int)),
//             trackButtonsView,        SLOT(setContentsPos(int, int)));

//     connect(m_trackEditorScrollView, SIGNAL(contentsMoving(int, int)),
//             barButtonsView,          SLOT(setContentsPos(int, int)));

//     m_trackEditorScrollView->addChild(m_trackEditor);

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(editSegmentNotation(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(editSegmentMatrix(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentAudio(Rosegarden::Segment*)),
            SLOT(editSegmentAudio(Rosegarden::Segment*)));

/*!!!
    connect(m_trackEditor, SIGNAL(createNewSegment(Rosegarden::timeT,
                                                  Rosegarden::timeT,
                                                  Rosegarden::TrackId)),
            getDocument(), SLOT  (createNewSegment(Rosegarden::timeT,
                                                   Rosegarden::timeT,
                                                   Rosegarden::TrackId)));
*/

    connect(m_trackEditor,  SIGNAL(scrollHorizTo(int)),
            SLOT(scrollTrackEditorHoriz(int)));

    connect(m_trackEditor,
            SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)),
            doc,
            SLOT(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)));

    // Connections upwards from LoopRuler - re-emission of signals
    //
//     connect(m_barButtons, SIGNAL(setPointerPosition(Rosegarden::timeT)),
//             this,         SIGNAL(setGUIPositionPointer(Rosegarden::timeT)));

//     connect(m_barButtons, SIGNAL(setPlayPosition(Rosegarden::timeT)),
//             this, SIGNAL(setGUIPlayPosition(Rosegarden::timeT)));

//     connect(m_barButtons,
//             SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
//             this,
//             SIGNAL(setGUILoop(Rosegarden::timeT, Rosegarden::timeT)));

    connect(m_trackEditor->getBarButtons(),
            SIGNAL(setPointerPosition(Rosegarden::timeT)),
            parent,
            SLOT(setPointerPosition(Rosegarden::timeT)));

    connect(m_trackEditor->getBarButtons(),
            SIGNAL(setPlayPosition(Rosegarden::timeT)),
            parent,
            SLOT(setPlayPosition(Rosegarden::timeT)));

    connect(m_trackEditor->getBarButtons(),
            SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
            parent,
            SLOT(setLoop(Rosegarden::timeT, Rosegarden::timeT)));


    if (doc)
        m_trackEditor->setupSegments();
}


RosegardenGUIView::~RosegardenGUIView()
{
    delete m_rulerScale;
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

void RosegardenGUIView::print(KPrinter *pPrinter, Rosegarden::Composition* p)
{
    QPainter printpainter;
    printpainter.begin(pPrinter);

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Rosegarden::Composition::iterator i = p->begin(); i != p->end(); ++i) {
        segmentsToEdit.push_back(*i);
    }

    NotationView *notationView = new NotationView(this, segmentsToEdit, this);
    notationView->setPageMode(true);
//     notationView->show();
    notationView->print(&printpainter);

    printpainter.end();
}

void RosegardenGUIView::selectTool(SegmentCanvas::ToolType tool)
{
    m_trackEditor->getSegmentCanvas()->setTool(tool);
}


void RosegardenGUIView::editSegmentNotation(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    NotationView *notationView = new NotationView(this, segmentsToEdit, this);
    notationView->show();
}

void RosegardenGUIView::editSegmentMatrix(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    MatrixView *matrixView = new MatrixView(getDocument(), segmentsToEdit, this);
    matrixView->show();
}

void RosegardenGUIView::editSegmentAudio(Rosegarden::Segment* p)
{
    std::cout << "RosegardenGUIView::editSegmentAudio() - got segment" << endl;
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

    NotationView *notationView = new NotationView(this, segmentsToEdit, this);
    notationView->show();
}

void RosegardenGUIView::setPointerPosition(const Rosegarden::timeT &position)
{
    m_trackEditor->setPointerPosition(position);
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

    std::list<Rosegarden::Segment*> segments;

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
    m_trackEditor->getSegmentCanvas()->selectSegments(segments);
}


// Insert a newly created Segment (usually recorded) into
// the view - create a SegmentItem for it
//
void RosegardenGUIView::createSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->addSegmentItem(segment);
}

void RosegardenGUIView::deleteSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->deleteSegmentItem(segment);
}

void RosegardenGUIView::updateSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->updateSegmentItem(segment);
}

// Show a segment as it records
//
void RosegardenGUIView::showRecordingSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->updateRecordingSegmentItem(segment);
}

void RosegardenGUIView::deleteRecordingSegmentItem()
{
    m_trackEditor->deleteRecordingSegmentItem();
}


// Set the visible loop markers on all clients and the SegmentCanvas.
// Used when we've loaded a file or somehow set the loop marker from
// the top down rather than from the LoopRuler upwards.
//
void RosegardenGUIView::setLoopMarker(Rosegarden::timeT startLoop,
                                      Rosegarden::timeT endLoop)
{
    m_trackEditor->getBarButtons()->slotSetLoopMarker(startLoop, endLoop);
}


// Send a MappedEvent to the track meter
//
void RosegardenGUIView::showVisuals(const Rosegarden::MappedEvent *mE)
{
    double value = ((double)mE->getVelocity()) / 127.0;
    m_trackEditor->getTrackButtons()->setTrackMeter(value, mE->getTrack());
}


void
RosegardenGUIView::setControl(const bool &value)
{
    m_trackEditor->setSelectCopy(value);
}

void
RosegardenGUIView::setShift(const bool &value)
{
    m_trackEditor->setSelectAdd(value);
    m_trackEditor->getBarButtons()->setLoopingMode(value);
    m_trackEditor->setFineGrain(value);
} 



