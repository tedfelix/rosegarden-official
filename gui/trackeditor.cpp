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
    m_barButtons(0),
    m_trackButtons(0),
    m_segmentCanvas(0),
    m_trackButtonsScrollPos(0)
{
    Composition &comp = doc->getComposition();

    int tracks = doc->getNbTracks();

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

    QHBoxLayout *hbox = new QHBoxLayout(this);

    QCanvas *canvas = new QCanvas(this);

    int canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
			    m_rulerScale->getBarWidth(lastBar));

    canvas->resize(canvasWidth, getTrackCellHeight() * nbTracks);
    // TODO : take barbuttons and trackbuttons into account for canvas size

    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    int trackLabelWidth = 156;
    int trackLabelOffset = 0;//!!!3;

    int barButtonsHeight = 25;
    int barButtonsOffset = 0;//!!!2;

    m_segmentCanvas = new SegmentCanvas(m_rulerScale,
                                        getTrackCellHeight(),
                                        trackLabelOffset + trackLabelWidth,
					barButtonsOffset + barButtonsHeight,
                                        canvas, this);

    hbox->addWidget(m_segmentCanvas);
    m_barButtons = new BarButtons(m_document,
                                  m_rulerScale,
                                  barButtonsHeight,
                                  false,
                                  m_segmentCanvas);

    m_trackButtons = new TrackButtons(m_document,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      m_segmentCanvas);

    m_trackButtons->setGeometry(trackLabelOffset,
				barButtonsOffset + barButtonsHeight,
                                trackLabelWidth,
                                getTrackCellHeight() * nbTracks + 5);

    m_barButtons->setGeometry(trackLabelOffset + trackLabelWidth,
			      barButtonsOffset,
                              getSegmentCanvas()->viewport()->width(),
                              barButtonsHeight);

    // Synchronize side widgets (bar and track buttons) with scrollbars
    //
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotScrollTrackButtons(int)));
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotScrollTrackButtons(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_barButtons, SLOT(scrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_barButtons, SLOT(scrollHoriz(int)));

    connect(this, SIGNAL(needUpdate()),
            m_segmentCanvas, SLOT(update()));

    QObject::connect(m_segmentCanvas, SIGNAL(addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)),
                     this,            SLOT  (slotAddSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(deleteSegment(Rosegarden::Segment *)),
                     this,            SLOT  (slotDeleteSegment(Rosegarden::Segment *)));

    QObject::connect(m_segmentCanvas, SIGNAL(changeSegmentDuration(Rosegarden::Segment *, Rosegarden::timeT)),
                     this,            SLOT  (slotChangeSegmentDuration(Rosegarden::Segment *, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(changeSegmentTimes(Rosegarden::Segment *, Rosegarden::timeT, Rosegarden::timeT)),
                     this,            SLOT  (slotChangeSegmentTimes(Rosegarden::Segment *, Rosegarden::timeT, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(changeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)),
                     this,            SLOT  (slotChangeSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)),
		     this,	      SLOT  (slotSplitSegment(Rosegarden::Segment*, Rosegarden::timeT)));

    QObject::connect
	(getCommandHistory(), SIGNAL(commandExecuted(KCommand *)),
	 this,		      SLOT  (commandExecuted(KCommand *)));

    // create the position pointer
    m_pointer = new QCanvasLine(canvas);
    m_pointer->setPen(RosegardenGUIColours::TimePointer);
    m_pointer->setPoints(0, 0, 0, canvas->height());
    m_pointer->setZ(10);
    m_pointer->show();
}

void TrackEditor::resizeEvent(QResizeEvent*)
{
    m_barButtons->setGeometry(m_barButtons->x(), 2,
                              getSegmentCanvas()->viewport()->width(),
                              m_barButtons->height());
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

void TrackEditor::commandExecuted(KCommand *command)
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

    m_segmentCanvas->update();
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
TrackEditor::setPointerPosition(Rosegarden::timeT position)
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
TrackEditor::setSelectAdd(bool value)
{
    m_segmentCanvas->setSelectAdd(value);
}

void
TrackEditor::setSelectCopy(bool value)
{
     m_segmentCanvas->setSelectCopy(value);
}

void
TrackEditor::setFineGrain(bool value)
{
     m_segmentCanvas->setFineGrain(value);
}


// Show a Segment as it's being recorded
//
void
TrackEditor::updateRecordingSegmentItem(Rosegarden::Segment *segment)
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
TrackEditor::deleteRecordingSegmentItem()
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





