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
#include "loopruler.h"
#include "RulerScale.h"

using Rosegarden::SimpleRulerScale;


RosegardenGUIView::RosegardenGUIView(QWidget *parent, const char* /*name*/)
    : QVBox(parent),
      m_rulerScale(0),
      m_trackEditor(0)
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

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(slotEditSegmentNotation(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(slotEditSegmentMatrix(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentAudio(Rosegarden::Segment*)),
            SLOT(slotEditSegmentAudio(Rosegarden::Segment*)));

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

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->setPageMode(true);
//     notationView->show();
    notationView->print(&printpainter);

    printpainter.end();
}

void RosegardenGUIView::selectTool(SegmentCanvas::ToolType tool)
{
    m_trackEditor->getSegmentCanvas()->slotSetTool(tool);
}


void RosegardenGUIView::slotEditSegmentNotation(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->show();
}

void RosegardenGUIView::slotEditSegmentMatrix(Rosegarden::Segment* p)
{
    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    MatrixView *matrixView = new MatrixView(getDocument(), segmentsToEdit, this);
    matrixView->show();
}

void RosegardenGUIView::slotEditSegmentAudio(Rosegarden::Segment* p)
{
    std::cout << "RosegardenGUIView::slotEditSegmentAudio() - got segment" << endl;
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

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->show();
}

// Highlight all the Segments on a Track because the Track has been selected
// We have to ensure we create a Selector object before we can highlight
// these tracks.
//
//
void RosegardenGUIView::slotSelectTrackSegments(int trackId)
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
    m_trackEditor->getSegmentCanvas()->slotSelectSegments(segments);
}

// Show a segment as it records
//
void RosegardenGUIView::showRecordingSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->slotUpdateRecordingSegmentItem(segment);
}

void RosegardenGUIView::deleteRecordingSegmentItem()
{
    m_trackEditor->slotDeleteRecordingSegmentItem();
}


// Send a MappedEvent to the track meter
//
void RosegardenGUIView::showVisuals(const Rosegarden::MappedEvent *mE)
{
    double value = ((double)mE->getVelocity()) / 127.0;
    m_trackEditor->getTrackButtons()->slotSetTrackMeter(value, mE->getTrack());
}


void
RosegardenGUIView::setControl(const bool &value)
{
    m_trackEditor->slotSetSelectCopy(value);
}

void
RosegardenGUIView::setShift(const bool &value)
{
    m_trackEditor->slotSetSelectAdd(value);
    m_trackEditor->getTopBarButtons()->getLoopRuler()->
	slotSetLoopingMode(value);
    m_trackEditor->slotSetFineGrain(value);
} 



