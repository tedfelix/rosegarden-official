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

#include <algorithm>

#include <qlayout.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qtimer.h>

#include <kmessagebox.h>
#include <kapp.h>


#include "RulerScale.h"
#include "Track.h"
#include "NotationTypes.h"

#include "trackeditor.h"
#include "segmentcanvas.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "colours.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"
#include "barbuttons.h"
#include "trackbuttons.h"
#include "loopruler.h"
#include "qdeferscrollview.h"

#include "rosedebug.h"

using Rosegarden::Composition;
using Rosegarden::RulerScale;
using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::TrackId;

TrackEditor::TrackEditor(RosegardenGUIDoc* doc,
                         QWidget* rosegardenguiview,
			 RulerScale *rulerScale,
                         bool showTrackLabels,
			 QWidget* parent, const char* name,
			 WFlags) :
    QWidget(parent, name),
    DCOPObject("TrackEditorIface"),
    m_document(doc),
    m_rulerScale(rulerScale),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_trackButtons(0),
    m_horizontalScrollBar(0),
    m_segmentCanvas(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_canvasWidth(0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId())
{
    Composition &comp = doc->getComposition();

    int tracks = comp.getNbTracks();

    // If we have no Track then create a default document with 64 of them
    //
    if (tracks == 0)
    {
        // default number of Tracks
        //
        tracks = 64;

        // instrument assignment
        //
        Rosegarden::InstrumentId instBase = Rosegarden::MidiInstrumentBase;

        // Create the Tracks on the Composition
        //
        Rosegarden::Track *track;
        for (int i = 0; i < tracks; i++)
        {
            char tmp[256];
            sprintf(tmp, "#%d", i);
            std::string label = "untitled ";
            label += tmp;

            track = new Rosegarden::Track(i,                       // id
                                          (i + instBase) % 16,     // instrument
                                          i,                       // position
                                          label, 
                                          false);                  // mute

            comp.addTrack(track);
        }

    }

    init(rosegardenguiview, tracks,
	 comp.getBarNumber(comp.getStartMarker()),
	 comp.getBarNumber(comp.getEndMarker()));
}

TrackEditor::~TrackEditor()
{
    // delete all remaining canvas items
    //
    QCanvasItemList list = m_segmentCanvas->canvas()->allItems();
    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
        delete *it;

    // now flush gc (e.g. forget what's in there). The reason is, if
    // any SegmentItem had a repeat rectangle, that item marked it for
    // GC when it was deleted. But the repeat rect was also deleted,
    // because we iterated over all canvas items. Therefore the GC can
    // only contain already deleted items.
    //
    CanvasItemGC::flush();
}

void
TrackEditor::init(QWidget* rosegardenguiview,
                  unsigned int nbTracks, int firstBar, int lastBar)
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::init(nbTracks = "
                         << nbTracks << ", firstBar = " << firstBar
                         << ", lastBar = " << lastBar << ")" << endl;

    QGridLayout *grid = new QGridLayout(this, 4, 2);

    QCanvas *canvas = new QCanvas(this);

    m_canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
                          m_rulerScale->getBarWidth(lastBar));

    int segmentCanvasHeight = getTrackCellHeight() * 40;

    canvas->resize(m_canvasWidth, segmentCanvasHeight);

    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    int trackLabelWidth = 230;
    int barButtonsHeight = 25;

    //
    // Top Bar Buttons
    //
    m_topBarButtons = new BarButtons(m_rulerScale,
				     0,
                                     barButtonsHeight,
                                     false,
                                     this);
    m_topBarButtons->connectRulerToDocPointer(m_document);

    grid->addWidget(m_topBarButtons, 0, 1);

    // Horizontal scrollbar - we need to create it now even though
    // it's inserted in the layout later on, because we need to set it's
    // range according to the segment canvas' one
    m_horizontalScrollBar = new QScrollBar(Horizontal, this);

    //
    // Segment Canvas
    //
    m_segmentCanvas = new SegmentCanvas(m_document,
                                        m_rulerScale,
                                        m_horizontalScrollBar,
                                        getTrackCellHeight(),
                                        canvas, this);

    grid->addWidget(m_segmentCanvas, 1, 1);

    //
    // Bottom Bar Buttons
    //
    m_bottomBarButtons = new BarButtons(m_rulerScale,
					0, 
                                        barButtonsHeight,
                                        true,
                                        this);
    m_bottomBarButtons->connectRulerToDocPointer(m_document);
    
    grid->addWidget(m_bottomBarButtons, 2, 1);

    //
    // Horizontal Scrollbar
    //
    m_horizontalScrollBar->setRange(m_segmentCanvas->horizontalScrollBar()->minValue(),
                                    m_segmentCanvas->horizontalScrollBar()->maxValue());

    m_horizontalScrollBar->setSteps(m_segmentCanvas->horizontalScrollBar()->lineStep(),
                                    m_segmentCanvas->horizontalScrollBar()->pageStep());

    grid->addWidget(m_horizontalScrollBar, 3, 1);

  
    // Track Buttons
    //
    // (must be put in a QScrollView)
    //
    m_trackButtonScroll = new QDeferScrollView(this);
    grid->addWidget(m_trackButtonScroll, 1, 0);

    m_trackButtons = new TrackButtons(m_document,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      m_showTrackLabels,
                                      segmentCanvasHeight,
                                      m_trackButtonScroll->viewport());
    m_trackButtonScroll->addChild(m_trackButtons);
    m_trackButtonScroll->setHScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setVScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setMinimumWidth(m_trackButtonScroll->contentsWidth());

    connect(m_trackButtons, SIGNAL(widthChanged()),
            this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            rosegardenguiview, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            rosegardenguiview, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(const QString&, bool)),
            rosegardenguiview, SIGNAL(stateChange(const QString&, bool)));

    // Synchronize bar buttons' scrollview with segment canvas' scrollbar
    //
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    // scrolling with mouse wheel
    connect(m_trackButtonScroll, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_segmentCanvas, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Connect horizontal scrollbar
    //
    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)));

    connect(this, SIGNAL(needUpdate()), m_segmentCanvas, SLOT(slotUpdate()));

    connect(m_segmentCanvas, 
            SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            rosegardenguiview,
            SLOT(slotSelectedSegments(const Rosegarden::SegmentSelection &)));

    connect(m_segmentCanvas, SIGNAL(scrollTo(int)),
	    this, SLOT(slotScrollHorizSmallSteps(int)));

    connect(getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));

    connect(m_document, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	    this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));
 
    connect(m_document, SIGNAL(loopChanged(Rosegarden::timeT,
					   Rosegarden::timeT)),
	    this, SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));
 
/*!!! now handled in rosegardengui.cpp as Edit | Delete
    QAccel *a = new QAccel(this);
    a->connectItem(a->insertItem(Key_Delete),
                   this,
                   SLOT(slotDeleteSelectedSegments()));
*/

    // create the position pointer
    m_pointer = new QCanvasRectangle(canvas);
    m_pointer->setPen(RosegardenGUIColours::Pointer);
    m_pointer->setBrush(RosegardenGUIColours::Pointer);
    m_pointer->setSize(3, canvas->height());
    m_pointer->setX(-2);
    m_pointer->setY(0);
    m_pointer->setZ(10);
    m_pointer->show();



}

void TrackEditor::slotTrackButtonsWidthChanged()
{
    // We need to make sure the trackButtons geometry is fully updated
    //
    kapp->processEvents();

    m_trackButtonScroll->setMinimumWidth(m_trackButtons->width());
}


int TrackEditor::getTrackCellHeight() const
{
    int size;
    static QFont defaultFont;

    // do some scrabbling around for a reasonable size
    //
    size = defaultFont.pixelSize();

    if (size < 8)
    {
        if (QApplication::font(this).pixelSize() < 8)
            size = 12;
        else
            size = QApplication::font(this).pixelSize();
    }

    return size + 12;
}

void
TrackEditor::setupSegments()
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::setupSegments() begin" << endl;

    if (!m_document) return; // sanity check
    
    Composition &comp = m_document->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

        if (!(*i)) continue;

	kdDebug(KDEBUG_AREA) << "TrackEditor::setupSegments() add segment"
			     << " - start idx : " << (*i)->getStartTime()
			     << " - nb time steps : " << ((*i)->getEndTime() - (*i)->getStartTime())
			     << " - track : " << (*i)->getTrack()
			     << endl;

	m_segmentCanvas->addSegmentItem((*i));
    }
}

bool TrackEditor::isCompositionModified()
{
    return m_document->getComposition().getRefreshStatus(m_compositionRefreshStatusId).needsRefresh();
}

void TrackEditor::setCompositionModified(bool c)
{
    m_document->getComposition().getRefreshStatus(m_compositionRefreshStatusId).setNeedsRefresh(c);
}


void TrackEditor::paintEvent(QPaintEvent* e)
{
    Composition &composition = m_document->getComposition();

    if (isCompositionModified()) {
        m_segmentCanvas->updateAllSegmentItems();

        m_trackButtons->slotUpdateTracks();

        /*
        m_segmentCanvas->canvas()->resize(m_canvasWidth,
                                          getTrackCellHeight() * m_document->getComposition().getNbTracks());
                                          */

#ifdef RGKDE3

        if (composition.getNbSegments() == 0) {
            emit stateChange("have_segments", true); // no segments : reverse state
            emit stateChange("segment_selected", true); // no segments : reverse state
        }
        else {
            emit stateChange("have_segments", false);
            if (m_segmentCanvas->haveSelection())
                emit stateChange("segment_selected", false);
            else
                emit stateChange("segment_selected", true); // no selection : reverse state
        }

#endif

        setCompositionModified(false);
    }

    QWidget::paintEvent(e);
}

void TrackEditor::slotAddTracks(unsigned int nbNewTracks)
{
    Composition &comp = m_document->getComposition();

    AddTracksCommand* command = new AddTracksCommand(&comp, nbNewTracks);

    addCommandToHistory(command);
}

void TrackEditor::addSegment(int track, int time, unsigned int duration)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();
    SegmentInsertCommand *command =
	new SegmentInsertCommand(m_document, track, time, duration);

    addCommandToHistory(command);
}


void TrackEditor::slotSegmentOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    //!!! how do we get here? need to involve a command
    emit needUpdate();
}

// Move the position pointer
void
TrackEditor::slotSetPointerPosition(Rosegarden::timeT position)
{

//    kdDebug(KDEBUG_AREA) << "TrackEditor::setPointerPosition: time is " << position << endl;
    if (!m_pointer) return;

    double canvasPosition = m_rulerScale->getXForTime(position);
    double distance = (double)canvasPosition - m_pointer->x();

    if (distance < 0.0) distance = -distance;
    if (distance >= 1.0) {

	m_pointer->setX(canvasPosition - 1);
        slotScrollHoriz((int)canvasPosition);
	emit needUpdate();
    }
}

void
TrackEditor::slotSetLoop(Rosegarden::timeT start, Rosegarden::timeT end)
{
    getTopBarButtons()->getLoopRuler()->slotSetLoopMarker(start, end);
    getBottomBarButtons()->getLoopRuler()->slotSetLoopMarker(start, end);
}

void
TrackEditor::slotSetSelectAdd(bool value)
{
    m_segmentCanvas->slotSetSelectAdd(value);
}

void
TrackEditor::slotSetSelectCopy(bool value)
{
     m_segmentCanvas->slotSetSelectCopy(value);
}

void
TrackEditor::slotSetFineGrain(bool value)
{
     m_segmentCanvas->slotSetFineGrain(value);
}


// This scrolling model pages the SegmentCanvas across the screen
//
//
void TrackEditor::slotScrollHoriz(int hpos)
{
    QScrollView* scrollView = getSegmentCanvas();
    QScrollBar* hbar = getHorizontalScrollBar();

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if (hpos > (scrollView->contentsX() +
		       scrollView->visibleWidth() * 1.6) ||
	       hpos < (scrollView->contentsX() -
		       scrollView->visibleWidth() * 0.7)) {
	
	// miles off one side or the other
	hbar->setValue(hpos - int(scrollView->visibleWidth() * 0.4));

    } else if (hpos > (scrollView->contentsX() + 
		       scrollView->visibleWidth() * 0.9)) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + int(scrollView->visibleWidth() * 0.6));

    } else if (hpos < (scrollView->contentsX() +
		       scrollView->visibleWidth() * 0.1)) {

	// moving off the left
	hbar->setValue(hbar->value() - int(scrollView->visibleWidth() * 0.6));
    }
}


void TrackEditor::slotScrollHorizSmallSteps(int hpos)
{
    QScrollView* scrollView = getSegmentCanvas();
    QScrollBar* hbar = getHorizontalScrollBar();

    int diff = 0;

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if ((diff = hpos - (scrollView->contentsX() + 
			       scrollView->visibleWidth() * 0.9)) > 0) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + diff);

    } else if ((diff = hpos - (scrollView->contentsX() +
			       scrollView->visibleWidth() * 0.1)) < 0) {

	// moving off the left
	hbar->setValue(hbar->value() + diff);

    }
}


// Show a Segment as its being recorded
//
void
TrackEditor::slotUpdateRecordingSegmentItem(Rosegarden::Segment *segment)
{
    Composition &comp = m_document->getComposition();
    int y = segment->getTrack() * getTrackCellHeight();

    timeT startTime = segment->getStartTime();

    // Show recording SegmentItem from recording start point to
    // current point position
    //
    timeT duration = comp.getPosition() - startTime;

    // Alternatively show it to the last recorded event
    //
    //timeT duration = segment->getDuration();

    m_segmentCanvas->showRecordingSegmentItem(segment->getTrack(),
                                              startTime, duration);

    emit needUpdate();
}

void
TrackEditor::slotDeleteRecordingSegmentItem()
{
    m_segmentCanvas->deleteRecordingSegmentItem();
    emit needUpdate();
}


MultiViewCommandHistory*
TrackEditor::getCommandHistory()
{
    return m_document->getCommandHistory();
}


void
TrackEditor::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}


void
TrackEditor::slotDeleteSelectedSegments()
{
    KMacroCommand *macro = new KMacroCommand("Delete Segments");

    Rosegarden::SegmentSelection segments =
            m_segmentCanvas->getSelectedSegments();

    if (segments.size() == 0)
        return;

    Rosegarden::SegmentSelection::iterator it;

    // Clear the selection before erasing the Segments
    // the selection points to
    //
    m_segmentCanvas->clearSelected();

    // Create the compound command
    //
    for (it = segments.begin(); it != segments.end(); it++)
    {
        macro->addCommand(new SegmentEraseCommand(*it));
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotVerticalScrollTrackButtons(int y)
{
    m_trackButtonScroll->setContentsPos(0, y);
}

//------------------------------------------------------------
