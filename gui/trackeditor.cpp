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

#include <kcommand.h>
#include <kmessagebox.h>

#include "RulerScale.h"
#include "Track.h"
#include "NotationTypes.h"

#include "trackeditor.h"
#include "segmentcanvas.h"
#include "rosegardenguidoc.h"
#include "colours.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"
#include "barbuttons.h"
#include "trackbuttons.h"
#include "loopruler.h"

#include "rosedebug.h"

using Rosegarden::Composition;
using Rosegarden::RulerScale;
using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::TrackId;

TrackEditor::TrackEditor(RosegardenGUIDoc* doc,
			 RulerScale *rulerScale,
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
    m_trackButtonsScrollPos(0)
{
    Composition &comp = doc->getComposition();

    int tracks = comp.getNbTracks();

    // If we have no Track then create a default document with 10 of them
    //
    if (tracks == 0)
    {
        // default number of Tracks
        //
        tracks = 10;

        // Create the Tracks on the Composition
        //
        Rosegarden::Track *track;
        for (int i = 0; i < tracks; i++)
        {
            track = new Rosegarden::Track(i, false, Rosegarden::Track::Midi,
                                          std::string("untitled"), i, 0);

            comp.addTrack(track);
        }

        // Add a default Instrument
        //
        Rosegarden::Instrument *instr = new Rosegarden::Instrument(0,
                          Rosegarden::Instrument::Midi, std::string("Instrument 1"));
        comp.addInstrument(instr);
    }

    init(tracks,
	 comp.getBarNumber(comp.getStartMarker()),
	 comp.getBarNumber(comp.getEndMarker()));
}


void
TrackEditor::init(unsigned int nbTracks, int firstBar, int lastBar)
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::init(nbTracks = "
                         << nbTracks << ", firstBar = " << firstBar
                         << ", lastBar = " << lastBar << ")" << endl;

    QGridLayout *grid = new QGridLayout(this, 4, 2);

    QCanvas *canvas = new QCanvas(this);

    int canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
			    m_rulerScale->getBarWidth(lastBar));

    canvas->resize(canvasWidth, getTrackCellHeight() * nbTracks);

    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    int trackLabelWidth = 156;
    int trackLabelOffset = 0;//!!!3;

    int barButtonsHeight = 25;
    int barButtonsOffset = 0;//!!!2;

    m_topBarButtons = new BarButtons(m_rulerScale,
                                     barButtonsHeight,
                                     false,
                                     this);
    m_topBarButtons->connectRulerToDocPointer(m_document);

    grid->addWidget(m_topBarButtons, 0, 1);

    m_horizontalScrollBar = new QScrollBar(Horizontal, this);

    m_segmentCanvas = new SegmentCanvas(m_rulerScale,
                                        m_horizontalScrollBar,
                                        getTrackCellHeight(),
                                        canvas, this);
    // We need our own scrollbar so that it is displayed below the
    // bottom bar buttons
    m_segmentCanvas->setHScrollBarMode(QScrollView::AlwaysOff);

    grid->addWidget(m_segmentCanvas, 1, 1);

    m_bottomBarButtons = new BarButtons(m_rulerScale,
                                        barButtonsHeight,
                                        true,
                                        this);
    m_bottomBarButtons->connectRulerToDocPointer(m_document);
    
    grid->addWidget(m_bottomBarButtons, 2, 1);

    m_horizontalScrollBar->setRange(m_segmentCanvas->horizontalScrollBar()->minValue(),
                                    m_segmentCanvas->horizontalScrollBar()->maxValue());

    m_horizontalScrollBar->setSteps(m_segmentCanvas->horizontalScrollBar()->lineStep(),
                                    m_segmentCanvas->horizontalScrollBar()->pageStep());

    grid->addWidget(m_horizontalScrollBar, 3, 1);
    

    m_trackButtons = new TrackButtons(m_document,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      this);

    grid->addWidget(m_trackButtons, 1, 0);

    // Synchronize side widgets (bar and track buttons) with scrollbars
    //
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotScrollTrackButtons(int)));
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotScrollTrackButtons(int)));

    // Connect horiz. scrollbar
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

    connect(m_segmentCanvas, SIGNAL(addSegment(Rosegarden::TrackId,
					       Rosegarden::timeT,
					       Rosegarden::timeT)),
	    this, SLOT(slotAddSegment(Rosegarden::TrackId,
				      Rosegarden::timeT,
				      Rosegarden::timeT)));

    connect(m_segmentCanvas, SIGNAL(deleteSegment(Rosegarden::Segment *)),
	    this, SLOT(slotDeleteSegment(Rosegarden::Segment *)));

    connect(m_segmentCanvas, SIGNAL(changeSegmentDuration(Rosegarden::Segment*,
							  Rosegarden::timeT)),
	    this, SLOT(slotChangeSegmentDuration(Rosegarden::Segment *,
						 Rosegarden::timeT)));

    connect(m_segmentCanvas, SIGNAL(changeSegmentTimes(Rosegarden::Segment *,
						       Rosegarden::timeT,
						       Rosegarden::timeT)),
	    this, SLOT(slotChangeSegmentTimes(Rosegarden::Segment *,
					      Rosegarden::timeT,
					      Rosegarden::timeT)));

    connect(m_segmentCanvas, SIGNAL(changeSegmentTrackAndStartTime
				    (Rosegarden::Segment *,
				     Rosegarden::TrackId, Rosegarden::timeT)),
	    this, SLOT(slotChangeSegmentTrackAndStartTime(Rosegarden::Segment*,
							  Rosegarden::TrackId,
							  Rosegarden::timeT)));

    connect(m_segmentCanvas, SIGNAL(splitSegment(Rosegarden::Segment*,
						 Rosegarden::timeT)),
	    this, SLOT(slotSplitSegment(Rosegarden::Segment*,
					Rosegarden::timeT)));

    connect(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
	    this, SLOT(slotCommandExecuted(KCommand *)));

    connect(m_document, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	    this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));
 
    connect(m_document, SIGNAL(loopChanged(Rosegarden::timeT,
					   Rosegarden::timeT)),
	    this, SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));
 
    // create the position pointer
    m_pointer = new QCanvasLine(canvas);
    m_pointer->setPen(RosegardenGUIColours::TimePointer);
    m_pointer->setPoints(0, 0, 0, canvas->height());
    m_pointer->setZ(10);
    m_pointer->show();
}

void TrackEditor::slotScrollTrackButtons(int newPos)
{
    int oldPos = m_trackButtonsScrollPos;

    m_trackButtonsScrollPos = newPos;

//     kdDebug(KDEBUG_AREA) << "TrackEditor::scrollTrackButtons : y = "
//                          << m_trackButtons->y()
//                          << " - newPos = "
//                          << newPos
//                          << " - currentPos = " << oldPos
//                          << " - scroll by : " << oldPos - m_trackButtonsScrollPos
//                          << std::endl;

    m_trackButtons->scroll(0, oldPos - m_trackButtonsScrollPos);

}

int TrackEditor::getTrackCellHeight() const
{
    static QFont defaultFont;
    
    return defaultFont.pixelSize() + 12; // For the moment
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
			     << " - nb time steps : " << (*i)->getDuration()
			     << " - track : " << (*i)->getTrack()
			     << endl;

	m_segmentCanvas->addSegmentItem((*i));
    }
}

void TrackEditor::slotCommandExecuted(KCommand *command)
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::commandExecuted" << endl;

    SegmentCommand *segmentCommand = dynamic_cast<SegmentCommand *>(command);
    if (!segmentCommand) {
	kdDebug(KDEBUG_AREA) << "TrackEditor::commandExecuted: not a segment command" << endl;
	return;
    }
	
    SegmentCommand::SegmentSet segments;
    segmentCommand->getSegments(segments);

    Composition &composition = m_document->getComposition();

    for (SegmentCommand::SegmentSet::iterator i = segments.begin();
	 i != segments.end(); ++i) {

	if (composition.contains(*i)) {
	    kdDebug(KDEBUG_AREA) << "Existing segment" << endl;
	
	    m_segmentCanvas->updateSegmentItem(*i);
	} else {
	    kdDebug(KDEBUG_AREA) << "Defunct segment" << endl;
	
	    m_segmentCanvas->removeSegmentItem(*i);
	}
    }

    m_segmentCanvas->slotUpdate();
}


void TrackEditor::addSegment(int track, int start, unsigned int duration)
{
    slotAddSegment(Rosegarden::TrackId(track),
		   Rosegarden::timeT(start),
		   Rosegarden::timeT(duration));
}


void TrackEditor::slotAddSegment(TrackId track, timeT time, timeT duration)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();
    SegmentInsertCommand *command =
	new SegmentInsertCommand(&comp, track, time, duration);

    addCommandToHistory(command);
}


void TrackEditor::slotSegmentOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    //!!! how do we get here? need to involve a command
    emit needUpdate();
}


void TrackEditor::slotDeleteSegment(Rosegarden::Segment *segment)
{
    addCommandToHistory(new SegmentEraseCommand(segment));
}


void TrackEditor::slotChangeSegmentDuration(Segment *s, timeT duration)
{
    SegmentReconfigureCommand *command =
	new SegmentReconfigureCommand("Resize Segment");
    command->addSegment(s, s->getStartTime(), duration, s->getTrack());
    addCommandToHistory(command);
}


void TrackEditor::slotChangeSegmentTimes(Segment *s,
					 timeT startTime, timeT duration)
{
    SegmentReconfigureCommand *command =
	new SegmentReconfigureCommand("Resize Segment");
    command->addSegment(s, startTime, duration, s->getTrack());
    addCommandToHistory(command);
}

void TrackEditor::slotChangeSegmentTrackAndStartTime(Segment *s, TrackId track,
						     timeT time)
{
    SegmentReconfigureCommand *command =
	new SegmentReconfigureCommand("Move Segment");
    command->addSegment(s, time, s->getDuration(), track);
    addCommandToHistory(command);
}


void TrackEditor::slotSplitSegment(Segment *s, timeT splitTime)
{
    addCommandToHistory(new SegmentSplitCommand(s, splitTime));
}


void TrackEditor::clear()
{
    //!!! when is this used? do we want to throw away the command history too?
    m_segmentCanvas->clear();
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

	m_pointer->setX(canvasPosition);
        emit scrollHorizTo((int)canvasPosition);
	emit needUpdate();
    }
}

void
TrackEditor::slotSetLoop(Rosegarden::timeT start, Rosegarden::timeT end)
{
    getTopBarButtons()->getLoopRuler()->slotSetLoopMarker(start, end);
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


// Show a Segment as it's being recorded
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

    m_segmentCanvas->showRecordingSegmentItem(y, startTime, duration);

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





