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

#include <algorithm>

#include "trackseditor.h"
#include "trackscanvas.h"
#include "rosegardenguidoc.h"

#include "rosedebug.h"

#include <qlayout.h>
#include <qcanvas.h>

#include <kmessagebox.h>

using Rosegarden::Composition;

static double canvasPosition;

TracksEditor::TracksEditor(RosegardenGUIDoc* doc,
                           QWidget* parent, const char* name,
                           WFlags) :
    QWidget(parent, name),
    DCOPObject("TracksEditorIface"),
    m_document(doc),
    m_tracksCanvas(0),
    m_hHeader(0),
    m_vHeader(0)
{
    int tracks(doc->getNbTracks());
    int bars(doc->getNbBars());
/*
    if (tracks == 0) tracks = 64;
    if (bars == 0) bars = 100;
*/
    tracks = 64;
    bars = 100;

    init(tracks, bars);
}


TracksEditor::TracksEditor(unsigned int nbTracks,
                           unsigned int nbBars,
                           QWidget *parent,
                           const char *name,
                           WFlags) :
    QWidget(parent, name),
    m_document(0),
    m_tracksCanvas(0),
    m_hHeader(0),
    m_vHeader(0)
{
    init(nbTracks, nbBars);
}


void
TracksEditor::init(unsigned int nbTracks, unsigned int nbBars)
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::init(nbTracks = "
                         << nbTracks << ", nbBars = " << nbBars
                         << ")" << endl;

    QGridLayout *grid = new QGridLayout(this, 2, 2);

    grid->addWidget(m_hHeader = new QHeader(nbBars, this), 0, 1);
    grid->addWidget(m_vHeader =
         new Rosegarden::TrackHeader(nbTracks, this), 1, 0);
    m_vHeader->setOrientation(Qt::Vertical);

    setupHorizontalHeader();

    // set up vert. header
    for (int i = 0; i < m_vHeader->count(); ++i) {
        m_vHeader->resizeSection(i, 25);
        m_vHeader->setLabel(i, QString("Instr. %1").arg(i));
    }

    m_vHeader->setMinimumWidth(100);
    m_vHeader->setResizeEnabled(false);

    QObject::connect(m_vHeader, SIGNAL(indexChange(int,int,int)),
                     this, SLOT(trackOrderChanged(int,int,int)));

    QCanvas *canvas = new QCanvas(this);
    canvas->resize(m_hHeader->sectionSize(0) * nbBars,
                   m_vHeader->sectionSize(0) * nbTracks);

    canvas->setBackgroundColor(Qt::lightGray);

    m_tracksCanvas = new TracksCanvas(m_hHeader->sectionSize(0),
                                      m_vHeader->sectionSize(0),
                                      *canvas, this);

    grid->addWidget(m_tracksCanvas, 1,1);
    connect(this, SIGNAL(needUpdate()),
            m_tracksCanvas, SLOT(update()));

    QObject::connect(m_tracksCanvas, SIGNAL(addTrack(TrackItem*)),
                     this,           SLOT(addTrack(TrackItem*)));

    QObject::connect(m_tracksCanvas, SIGNAL(deleteTrack(Rosegarden::Track*)),
                     this,           SLOT(deleteTrack(Rosegarden::Track*)));

    QObject::connect(m_tracksCanvas, SIGNAL(updateTrackDuration(TrackItem*)),
                     this,           SLOT(updateTrackDuration(TrackItem*)));

    QObject::connect(m_tracksCanvas, SIGNAL(updateTrackInstrumentAndStartIndex(TrackItem*)),
                     this,           SLOT(updateTrackInstrumentAndStartIndex(TrackItem*)));

    // create the position pointer
    m_pointer = new QCanvasLine(canvas);
    m_pointer->setPen(Qt::darkBlue);
    m_pointer->setPoints(0, 0, 0, canvas->height());
    m_pointer->setZ(10);
    m_pointer->show();

}

void
TracksEditor::setupTracks()
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::setupTracks() begin" << endl;

    if (!m_document) return; // sanity check
    
    Composition &comp = m_document->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

        if ((*i)) {

            kdDebug(KDEBUG_AREA) << "TracksEditor::setupTracks() add track"
                                 << " - start idx : " << (*i)->getStartIndex()
                                 << " - nb time steps : " << (*i)->getDuration()
                                 << " - instrument : " << (*i)->getInstrument()
                                 << endl;

	    int startBar = comp.getBarNumber((*i)->getStartIndex());
	    int barCount = comp.getBarNumber((*i)->getEndIndex()) - startBar +1;

            int y = m_vHeader->sectionPos((*i)->getInstrument());
	    int x = m_hHeader->sectionPos(startBar);

            TrackItem *newItem = m_tracksCanvas->addPartItem(x, y, barCount);
            newItem->setTrack(*i);
        }
        
    }
}


void TracksEditor::addTrack(int instrument, int start,
                            unsigned int nbTimeSteps)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();

    Rosegarden::Track* track = new Rosegarden::Track(start);
    track->setInstrument(instrument);
    comp.addTrack(track);
    track->setDuration(nbTimeSteps);

    int startBar = comp.getBarNumber(start);
    int barCount = comp.getBarNumber(start + nbTimeSteps) - startBar + 1;

    int y = m_vHeader->sectionPos(instrument);
    int x = m_hHeader->sectionPos(startBar);

    TrackItem *newItem = m_tracksCanvas->addPartItem(x, y, barCount);
    newItem->setTrack(track);

    emit needUpdate();
}


void TracksEditor::trackOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TracksEditor::trackOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    updateTrackOrder();
    emit needUpdate();
}


void
TracksEditor::addTrack(TrackItem *p)
{
    // first find instrument for part, as it is used for indexing
    //
    int instrument = m_vHeader->sectionAt(static_cast<int>(p->y()));

    emit createNewTrack(p, instrument);

    kdDebug(KDEBUG_AREA) << QString("TracksEditor::addTrack() : track instr is %1 at y=%2")
        .arg(instrument).arg(p->y())
                         << ", p = " << p << endl;

}


void TracksEditor::deleteTrack(Rosegarden::Track *p)
{
    Composition& composition = m_document->getComposition();

    if (!composition.deleteTrack(p)) {
        KMessageBox::error(0, QString("TracksEditor::deleteTrack() : part %1 not found").arg(long(p), 0, 16));
        
        kdDebug(KDEBUG_AREA) << "TracksEditor::deleteTrack() : track "
                             << p << " not found" << endl;
    }
}


void TracksEditor::updateTrackDuration(TrackItem *i)
{
    Composition& composition = m_document->getComposition();

    int startBar = m_hHeader->sectionAt(int(i->x()));
    int barCount = i->getItemNbBars();

    timeT startIndex = composition.getBarRange(startBar).first;
    timeT duration = composition.getBarRange(startBar + barCount).first -
	startIndex;

    kdDebug(KDEBUG_AREA) << "TracksEditor::updateTrackDuration() : set duration to "
                         << duration << endl;

    i->getTrack()->setDuration(duration);
}


void TracksEditor::updateTrackInstrumentAndStartIndex(TrackItem *i)
{
    Composition& composition = m_document->getComposition();

    int instrument = m_vHeader->sectionAt(int(i->y()));
    int startBar = m_hHeader->sectionAt(int(i->x()));
    timeT startIndex = composition.getBarRange(startBar).first;

    kdDebug(KDEBUG_AREA) << "TracksEditor::updateTrackInstrumentAndStartIndex() : set instrument to "
                         << instrument
                         << " - start Index to : " << startIndex << endl;

    i->setInstrument(instrument);
    i->getTrack()->setStartIndex(startIndex);
}


void TracksEditor::updateTrackOrder()
{
    QCanvasItemList itemList = canvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasItem *item = *it;
        TrackItem *trackItem = dynamic_cast<TrackItem*>(item);
        
        if (trackItem) {
            trackItem->setY(m_vHeader->sectionPos(trackItem->getInstrument()));
        }
    }
}


void TracksEditor::clear()
{
    m_tracksCanvas->clear();
}


void TracksEditor::setupHorizontalHeader()
{
    QString num;

    for (int i = 0; i < m_hHeader->count(); ++i) {
        m_hHeader->resizeSection(i, 50);
	//!!! ??? I guess this is related to the bar resolution of the trackscanvas??
        m_hHeader->setLabel(i, num.setNum(i));
    }
}


// Move the position pointer
void
TracksEditor::setPointerPosition(timeT position)
{
  if (!m_pointer) return;
/*!!!
  canvasPosition = (m_hHeader->sectionSize(0) * position /
		    m_timeStepsResolution );

  m_pointer->setX(canvasPosition < 0 ? 0 : canvasPosition);
*/
  emit needUpdate();
}


