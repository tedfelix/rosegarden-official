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
#include <kcommand.h>

#include "trackeditor.h"
#include "segmentcanvas.h"
#include "rosegardenguidoc.h"
#include "RulerScale.h"
#include "Track.h"
#include "colours.h"
#include "NotationTypes.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"

#include "rosedebug.h"

#include <qlayout.h>
#include <qcanvas.h>

#include <kmessagebox.h>

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
    m_segmentCanvas(0),
    m_hHeader(0),
    m_vHeader(0)
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

    QGridLayout *grid = new QGridLayout(this, 2, 2);

    setupHorizontalHeader(firstBar, lastBar);

    grid->addWidget(m_hHeader, 0, 1);
    grid->addWidget(m_vHeader =
         new Rosegarden::TrackHeader(nbTracks, this), 1, 0);
    m_vHeader->setOrientation(Qt::Vertical);

    // set up vert. header
    for (int i = 0; i < m_vHeader->count(); ++i) {
        m_vHeader->resizeSection(i, 25);
        m_vHeader->setLabel(i, QString("Track %1").arg(i));
    }

    m_vHeader->setMinimumWidth(100);
    m_vHeader->setResizeEnabled(false);

    QObject::connect(m_vHeader, SIGNAL(indexChange(int,int,int)),
                     this, SLOT(segmentOrderChanged(int,int,int)));

    QCanvas *canvas = new QCanvas(this);

    int canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
			    m_rulerScale->getBarWidth(lastBar));

    canvas->resize(canvasWidth, m_vHeader->sectionSize(0) * nbTracks);
    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    m_segmentCanvas = new SegmentCanvas
	(m_rulerScale, m_vHeader->sectionSize(0), *canvas, this);

    grid->addWidget(m_segmentCanvas, 1, 1);

    // Hide both headers - we use these for measurement and not show!
    //
    //!!! Get rid of the horizontal header -- it doesn't help, it
    // just confuses matters
    m_vHeader->hide();
    m_hHeader->hide();

    connect(this, SIGNAL(needUpdate()),
            m_segmentCanvas, SLOT(update()));

    QObject::connect(m_segmentCanvas, SIGNAL(addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)),
                     this,            SLOT  (addSegment(Rosegarden::TrackId, Rosegarden::timeT, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(deleteSegment(Rosegarden::Segment *)),
                     this,            SLOT  (deleteSegment(Rosegarden::Segment *)));

    QObject::connect(m_segmentCanvas, SIGNAL(updateSegmentDuration(Rosegarden::Segment *, Rosegarden::timeT)),
                     this,            SLOT  (updateSegmentDuration(Rosegarden::Segment *, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(updateSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)),
                     this,            SLOT  (updateSegmentTrackAndStartTime(Rosegarden::Segment *, Rosegarden::TrackId, Rosegarden::timeT)));

    QObject::connect(m_segmentCanvas, SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)),
		     this, SIGNAL(splitSegment(Rosegarden::Segment*, Rosegarden::timeT)));

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


void TrackEditor::addSegment(TrackId track, timeT time, timeT duration)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();
    SegmentInsertCommand *command =
	new SegmentInsertCommand(&comp, track, time, duration);

    addCommandToHistory(command);

/*!!!    

    Rosegarden::Segment* segment =
        new Rosegarden::Segment(Rosegarden::Segment::Internal, start);

    segment->setTrack(track);
    comp.addSegment(segment);
    segment->setDuration(nbTimeSteps);

    int y = m_vHeader->sectionPos(track);
    SegmentItem *newItem = m_segmentCanvas->addSegmentItem
	(y, segment->getStartTime(), segment->getDuration());
    newItem->setSegment(segment);

    emit needUpdate();
*/
}


void TrackEditor::segmentOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    updateSegmentOrder();
    emit needUpdate();
}

/*!!!
void
TrackEditor::addSegment(int y, Rosegarden::timeT time, Rosegarden::timeT duration)
{
    // first find track for segment, as it is used for indexing
    //
//!!!    Rosegarden::TrackId track = static_cast<Rosegarden::TrackId>(m_vHeader->sectionAt(y));

    emit createNewSegment(time, duration, y);
}
*/

void TrackEditor::deleteSegment(Rosegarden::Segment *p)
{
    Composition& composition = m_document->getComposition();

    if (!composition.deleteSegment(p)) {
        KMessageBox::error(0, QString("TrackEditor::deleteSegment() : part %1 not found").arg(long(p), 0, 16));
        
        kdDebug(KDEBUG_AREA) << "TrackEditor::deleteSegment() : segment "
                             << p << " not found" << endl;
    }
}


void TrackEditor::updateSegmentDuration(Segment *s, timeT duration)
{
    Composition& composition = m_document->getComposition();
    s->setDuration(duration);

//!!! start time could have changed too (because we can
// drag backwards on canvas as well as forwards -- possibility
// of -ve duration, which gets normalised to a change in start
// time) -- so this function's a bit misnamed
//!!!    composition.setSegmentStartTimeAndTrack
//	(i->getSegment(), i->getStartTime(), i->getSegment()->getTrack());

    m_document->documentModified();
}


void TrackEditor::updateSegmentTrackAndStartTime(Segment *s, TrackId track,
						 timeT time)
{
    Composition& composition = m_document->getComposition();
    composition.setSegmentStartTimeAndTrack(s, time, track);
    m_document->documentModified();
}


void TrackEditor::updateSegmentOrder()
{
    QCanvasItemList itemList = getSegmentCanvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasItem *item = *it;
        SegmentItem *segmentItem = dynamic_cast<SegmentItem*>(item);
        
        if (segmentItem) {
            segmentItem->setY(m_vHeader->sectionPos(segmentItem->getTrack()));
        }
    }
}


void TrackEditor::clear()
{
    m_segmentCanvas->clear();
}


void TrackEditor::setupHorizontalHeader(int firstBar, int lastBar)
{
    QString num;
    m_hHeader = new QHeader(lastBar - firstBar + 1, this);

    int x = 0;

    for (int i = firstBar; i <= lastBar; ++i) {

	// The (i < lastBar) case resynchronises against the absolute
	// bar position at each stage so as to avoid gradually increasing
	// error through integer rounding

	int width;
	if (i < lastBar) {
	    width = (int)(m_rulerScale->getBarPosition(i+1) - (double)x);
	    x += width;
	} else {
	    width = (int)(m_rulerScale->getBarWidth(i));
	}

	m_hHeader->resizeSection(i - firstBar, width);
        m_hHeader->setLabel(i - firstBar, num.setNum(i));
    }
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


// Just like setupSegments() this creates a SegmentItem
// on the SegmentCanvas if we've just recorded a Segment
// or if we need to add one say as part of an Undo/Redo.
//
void
TrackEditor::addSegmentItem(Rosegarden::Segment *segment)
{
    if (!m_document) return; // sanity check

    // Check that a SegmentItem doesn't already exist
    // for this Segment
    //
    QCanvasItemList itemList = getSegmentCanvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it)
    {
        QCanvasItem *item = *it;
        SegmentItem *segmentItem = dynamic_cast<SegmentItem*>(item);

        if (segmentItem)
            if (segmentItem->getSegment() == segment)
                return;
    }

    int y = m_vHeader->sectionPos(segment->getTrack());
    SegmentItem *newItem = m_segmentCanvas->addSegmentItem
	(y, segment->getStartTime(), segment->getDuration());
    newItem->setSegment(segment);

    emit needUpdate();

}

void
TrackEditor::deleteSegmentItem(Rosegarden::Segment *segment)
{
    QCanvasItemList itemList = getSegmentCanvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasItem *item = *it;
        SegmentItem *segmentItem = dynamic_cast<SegmentItem*>(item);

        if (segmentItem)
        {
            if (segmentItem->rtti() == SegmentItem::SegmentItemRTTI &&
                segmentItem->getSegment() == segment)
            {
                delete segmentItem;
                itemList.remove(it);
                break;
            }
        }
    }

    emit needUpdate();
}

void
TrackEditor::updateSegmentItem(Rosegarden::Segment *segment)
{
    QCanvasItemList itemList = getSegmentCanvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasItem *item = *it;
        SegmentItem *segmentItem = dynamic_cast<SegmentItem*>(item);

        if (segmentItem)
        {
            if (segmentItem->rtti() == SegmentItem::SegmentItemRTTI &&
                segmentItem->getSegment() == segment)
            {
                segmentItem->setStartTime(segment->getStartTime());
                segmentItem->setDuration(segment->getDuration());
            }
        }
    }

    emit needUpdate();
}





// Show a Segment as its being recorded
//
void
TrackEditor::updateRecordingSegmentItem(Rosegarden::Segment *segment)
{
    Composition &comp = m_document->getComposition();
    int y = m_vHeader->sectionPos(segment->getTrack());

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





