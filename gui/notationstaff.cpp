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

#include "eventselection.h"
#include "notationstaff.h"
#include "qcanvassimplesprite.h"
#include "notationproperties.h"
#include "notationview.h" // for EventSelection
#include "BaseProperties.h"

#include "rosedebug.h"
#include "colours.h"

#include "Event.h"
#include "Segment.h"
#include "Quantizer.h"
#include "NotationTypes.h"

using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Segment;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::Accidentals::NoAccidental;
using Rosegarden::TimeSignature;
using Rosegarden::PropertyName;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

using std::string;

NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             int id, bool pageMode, double pageWidth, 
                             string fontName, int resolution) :
    LinedStaff<NotationElement>(canvas, segment, id, resolution,
				1, //!!!
				pageMode, pageWidth,
				0 //!!!
	),
    m_npf(0)
{
    setLegatoDuration(Note(Note::Shortest).getDuration());
    changeFont(fontName, resolution);
}

NotationStaff::~NotationStaff()
{
    deleteTimeSignatures();
}

void
NotationStaff::changeFont(string fontName, int resolution) 
{
    setResolution(resolution);

    delete m_npf;
    m_npf = new NotePixmapFactory(fontName, resolution);

//!!!    resizeStaffLines();
}

void
NotationStaff::setLegatoDuration(Rosegarden::timeT duration)
{
//!!!
    kdDebug(KDEBUG_AREA) << "NotationStaff::setLegatoDuration" << endl;

    const Rosegarden::Quantizer *q = getSegment().getQuantizer();


    kdDebug(KDEBUG_AREA) << "NotationStaff: Quantizer status is:\n"
			 << "Unit = " << q->getUnit()
			 << "\nMax Dots = " << q->getMaxDots() << endl;

    Rosegarden::Quantizer *wq = const_cast<Rosegarden::Quantizer *>(q);
    wq->setUnit(duration);
}

void NotationStaff::insertTimeSignature(int layoutX,
					const TimeSignature &timeSig)
{
    QCanvasPixmap *pixmap =
	new QCanvasPixmap(m_npf->makeTimeSigPixmap(timeSig));
    QCanvasSimpleSprite *sprite = new QCanvasSimpleSprite(pixmap, m_canvas);

    LinedStaffCoords sigCoords =
	getCanvasCoordsForLayoutCoords(layoutX, getLayoutYForHeight(4));

    sprite->move(sigCoords.first, (double)sigCoords.second);
    sprite->show();
    m_timeSigs.insert(sprite);
}

void NotationStaff::deleteTimeSignatures()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteTimeSignatures()\n";
    
    for (SpriteSet::iterator i = m_timeSigs.begin();
	 i != m_timeSigs.end(); ++i) {
        delete (*i);
    }

    m_timeSigs.clear();
}

//!!! to lined staff?
void NotationStaff::getClefAndKeyAtCanvasCoords(double cx, int cy,
						Clef &clef, 
						Rosegarden::Key &key) const
{
    cx -= m_x;

    int i;
    int row = getRowForCanvasCoords(cx, cy);

    //??? do I have this right?
    //!!! rewrite taking advantage of cleaner LinedStaff API, even
    // if we don't move to LinedStaff itself

    for (i = 0; i < m_clefChanges.size(); ++i) {
	if (m_clefChanges[i].first > (m_pageWidth * row) + cx) break;
	clef = m_clefChanges[i].second;
    }

    for (i = 0; i < m_keyChanges.size(); ++i) {
	if (m_keyChanges[i].first > (m_pageWidth * row) + cx) break;
	key = m_keyChanges[i].second;
    }
}

string
NotationStaff::getNoteNameAtCanvasCoords(double x, int y,
					 Rosegarden::Accidental acc) const
{
//!!! getting wrong results for clef here -- changes within staff
// not being taken into account properly

    Rosegarden::Clef clef;
    Rosegarden::Key key;
    getClefAndKeyAtCanvasCoords(x, y, clef, key);

    return
	Rosegarden::NotationDisplayPitch
	(getHeightAtCanvasY(y), acc).getAsString(clef, key);
}

void
NotationStaff::renderElements(NotationElementList::iterator from,
			      NotationElementList::iterator to)
{
//    kdDebug(KDEBUG_AREA) << "NotationStaff " << this << "::renderElements()" << endl;

    START_TIMING;

    Clef currentClef; // default is okay to start with

    for (NotationElementList::iterator it = from; it != to; ++it) {

	if ((*it)->event()->isa(Clef::EventType)) {
	    currentClef = Clef(*(*it)->event());
	}

	bool selected = false;
	(void)((*it)->event()->get<Bool>(SELECTED, selected));

//	kdDebug(KDEBUG_AREA) << "Rendering at " << (*it)->getAbsoluteTime()
//			     << " (selected = " << selected << ")" << endl;

	renderSingleElement(*it, currentClef, selected);
    }

    PRINT_ELAPSED("NotationStaff::renderElements");
}	

void
NotationStaff::positionElements(timeT from, timeT to)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff " << this << "::positionElements()" << endl;

    START_TIMING;
    
    cerr << "positionElements: " << from << " -> " << to << endl;

    Clef currentClef; // default is okay to start with
    m_clefChanges.empty();

    Rosegarden::Key currentKey; // likewise
    m_keyChanges.empty();

    NotationElementList *nel = getViewElementList();

    // Track back bar-by-bar until we find one whose start position
    // hasn't changed, and begin the reposition from there

    NotationElementList::iterator beginAt = nel->begin();
    if (from >= 0) {
	NotationElementList::iterator candidate = nel->begin();
	do {
	    candidate = nel->findTime(getSegment().getBarStart(from - 1));
	    if (candidate != nel->end()) from = (*candidate)->getAbsoluteTime();
	} while (candidate != nel->begin() &&
		 (candidate == nel->end() || !elementNotMoved(*candidate)));
	beginAt = candidate;
    }

    // Track forward to the end, similarly.  Here however it's very
    // common for all the positions to have changed right up to the
    // end of the piece; so we save time by assuming that to be the
    // case if we get more than (arbitrary) 3 changed bars.

    NotationElementList::iterator endAt = nel->end();
    int changedBarCount = 0;
    if (to >= 0) {
	NotationElementList::iterator candidate = nel->end();
	do {
	    candidate = nel->findTime(getSegment().getBarEnd(to));
	    if (candidate != nel->end()) to = (*candidate)->getAbsoluteTime();
	    ++changedBarCount;
	} while (changedBarCount < 4 &&
		 candidate != nel->end() && !elementNotMoved(*candidate));
	if (changedBarCount < 4) endAt = candidate;
    }

    for (NotationElementList::iterator it = beginAt; it != endAt; ++it) {

	if ((*it)->event()->isa(Clef::EventType)) {

	    currentClef = Clef(*(*it)->event());
	    m_clefChanges.push_back(ClefChange((*it)->getLayoutX(),
					       currentClef));

	} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

	    currentKey = Rosegarden::Key(*(*it)->event());
	    m_keyChanges.push_back(KeyChange((*it)->getLayoutX(),
					     currentKey));
	}

	bool selected = false;
	(void)((*it)->event()->get<Bool>(SELECTED, selected));

	bool needNewSprite = (selected != (*it)->isSelected());

	if (!(*it)->getCanvasItem()) {

	    needNewSprite = true;

	} else if ((*it)->isNote()) {

	    // If the event is a beamed or tied-forward note, then we
	    // might need a new sprite if the distance from this note
	    // to the next has changed (because the beam or tie is
	    // part of the note's sprite).

	    bool spanning = false;
	    (void)((*it)->event()->get<Bool>(BEAMED, spanning));
	    if (!spanning)
		(void)((*it)->event()->get<Bool>(TIED_FORWARD, spanning));
	    
	    if (spanning) {

		needNewSprite = true;

		/*!!! Can't be this simple, I'm afraid: we probably need
		  the next-contiguous event, but what about chords?  Still,
		  might be worth trying to catch the case however complex
		  it is -- it's still cheaper than re-rendering

		int myCanvasX = (*it)->getCanvasItem()->x();
		NotationElementList::iterator scooter(it);
		++scooter;

		if (scooter != end && (*scooter)->getCanvasItem()) {
		    int nextCanvasX = (*scooter)->getCanvasItem()->x();
		    if ((nextCanvasX - myCanvasX) !=
			((*scooter)->getLayoutX() - (*it)->getLayoutX())) {
			needNewSprite = true;
		    }
		}
		*/
	    }
	} else if ((*it)->event()->isa(Indication::EventType)) {
	    needNewSprite = true;
	}

	if (needNewSprite) {
	    //kdDebug(KDEBUG_AREA) << "Rendering at " << (*it)->getAbsoluteTime()
//			     << " (selected = " << selected << ", canvas item selected = " << (*it)->isSelected() << ")" << endl;

	    renderSingleElement(*it, currentClef, selected);
	} else {
	    //kdDebug(KDEBUG_AREA) << "Positioning at " << (*it)->getAbsoluteTime()
	    //<< " (selected = " << selected << ", canvas item selected = " << (*it)->isSelected() << ")" << endl;

	    LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
		((*it)->getLayoutX(), (int)(*it)->getLayoutY());
	    (*it)->reposition(coords.first, (double)coords.second);
	}
    }

    PRINT_ELAPSED("NotationStaff::positionElements");
}


bool
NotationStaff::elementNotMoved(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());
    int xoff = (int)coords.first;
    int yoff = coords.second;

    bool ok =
	(int)(elt->getCanvasX()) == (int)(elt->getLayoutX() + xoff) &&
	(int)(elt->getCanvasY()) == (int)(elt->getLayoutY() + yoff);
    cerr << "elementNotMoved: elt at " << elt->getAbsoluteTime() <<
	", ok is " << ok << endl;
    if (!ok) {
	cerr << "(cf " << (int)(elt->getCanvasX()) << " vs "
	     << (int)(elt->getLayoutX() + xoff) << ", "
	     << (int)(elt->getCanvasY()) << " vs "
	     << (int)(elt->getLayoutY() + yoff) << ")" << endl;
    }
    return ok;
}


void
NotationStaff::renderSingleElement(NotationElement *elt,
				   const Rosegarden::Clef &currentClef,
				   bool selected)
{
    try {

	QCanvasPixmap *pixmap = 0;
	QCanvasItem *canvasItem = 0;

	m_npf->setSelected(selected);

	if (elt->isNote()) {

	    canvasItem = makeNoteSprite(elt);

	} else if (elt->isRest()) {

//    kdDebug(KDEBUG_AREA) << "NotationStaff::renderSingleElement: about to query legato duration property" << endl;
	    timeT duration = elt->event()->get<Int>
		(Rosegarden::Quantizer::LegatoDurationProperty);
//kdDebug(KDEBUG_AREA) << "done" <<endl;

	    if (duration > 0) {
		Note::Type note = elt->event()->get<Int>(NOTE_TYPE);
		int dots = elt->event()->get<Int>(NOTE_DOTS);
		pixmap = new QCanvasPixmap
		    (m_npf->makeRestPixmap(Note(note, dots)));
	    } else {
		kdDebug(KDEBUG_AREA) << "Omitting too-short rest" << endl;
	    }

	} else if (elt->event()->isa(Clef::EventType)) {

	    pixmap = new QCanvasPixmap
		(m_npf->makeClefPixmap(Rosegarden::Clef(*elt->event())));

	} else if (elt->event()->isa(Rosegarden::Key::EventType)) {

	    pixmap = new QCanvasPixmap
		(m_npf->makeKeyPixmap
		 (Rosegarden::Key(*elt->event()), currentClef));

	} else if (elt->event()->isa(Indication::EventType)) {

	    timeT indicationDuration =
		elt->event()->get<Int>(Indication::IndicationDurationPropertyName);
	    NotationElementList::iterator indicationEnd =
		getViewElementList()->findTime(elt->getAbsoluteTime() +
					       indicationDuration);

	    string indicationType = 
		elt->event()->get<String>(Indication::IndicationTypePropertyName);

	    int length, y1;

	    if (indicationType == Indication::Slur &&
		indicationEnd != getViewElementList()->begin()) {
		--indicationEnd;
	    }

	    if (indicationEnd != getViewElementList()->end()) {
		length = (int)((*indicationEnd)->getLayoutX() -
			       elt->getLayoutX());
		y1 = (int)(*indicationEnd)->getLayoutY();
	    } else {
		//!!! imperfect
		--indicationEnd;
		length = (int)((*indicationEnd)->getLayoutX() +
			       m_npf->getNoteBodyWidth() * 3 -
			       elt->getLayoutX());
		y1 = (int)(*indicationEnd)->getLayoutY();
	    }

	    if (length < m_npf->getNoteBodyWidth()) {
		length = m_npf->getNoteBodyWidth();
	    }

	    if (indicationType == Indication::Crescendo) {

		pixmap = new QCanvasPixmap
		    (m_npf->makeHairpinPixmap(length, true));

	    } else if (indicationType == Indication::Decrescendo) {

		pixmap = new QCanvasPixmap
		    (m_npf->makeHairpinPixmap(length, false));

	    } else if (indicationType == Indication::Slur) {

		bool above = true;
		long dy = 0;
		long length = 10;
		
		elt->event()->get<Bool>(SLUR_ABOVE, above);
		elt->event()->get<Int>(SLUR_Y_DELTA, dy);
		elt->event()->get<Int>(SLUR_LENGTH, length);
		
		pixmap = new QCanvasPixmap
		    (m_npf->makeSlurPixmap(length, dy, above));
		    
	    } else {
		//!!!
	    }

	} else {
                    
	    kdDebug(KDEBUG_AREA)
		<< "NotationElement of unrecognised type "
		<< elt->event()->getType() << endl;
	    pixmap = new QCanvasPixmap(m_npf->makeUnknownPixmap());
	}

	if (!canvasItem && pixmap) {
	    canvasItem = new QCanvasNotationSprite(*elt, pixmap, m_canvas);
	}

	// Show the sprite
	//
	if (canvasItem) {
	    LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
		(elt->getLayoutX(), (int)elt->getLayoutY());
	    elt->setCanvasItem
		(canvasItem, coords.first, (double)coords.second);
	    canvasItem->setZ(selected ? 2 : 0);
	    canvasItem->show();
	} else {
	    elt->removeCanvasItem();
	}
	
//	kdDebug(KDEBUG_AREA) << "NotationStaff::renderSingleElement: Setting selected at " << elt->getAbsoluteTime() << " to " << selected << endl;
	elt->setSelected(selected);
            
    } catch (...) {
	kdDebug(KDEBUG_AREA) << "Event lacks the proper properties: "
			     << (*elt->event())
			     << endl;
    }

    LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());
    elt->reposition(coords.first, (double)coords.second);
}

QCanvasSimpleSprite *
NotationStaff::makeNoteSprite(NotationElement *elt)
{
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = elt->event()->get<Int>(NOTE_TYPE);
    int dots = elt->event()->get<Int>(NOTE_DOTS);

    Accidental accidental = NoAccidental;
    (void)elt->event()->get<String>(DISPLAY_ACCIDENTAL, accidental);

    bool up = true;
    (void)(elt->event()->get<Bool>(STEM_UP, up));

    bool flag = true;
    (void)(elt->event()->get<Bool>(DRAW_FLAG, flag));

    bool beamed = false;
    (void)(elt->event()->get<Bool>(BEAMED, beamed));

    bool shifted = false;
    (void)(elt->event()->get<Bool>(NOTE_HEAD_SHIFTED, shifted));

    long stemLength = m_npf->getNoteBodyHeight();
    (void)(elt->event()->get<Int>(UNBEAMED_STEM_LENGTH, stemLength));
    
    long heightOnStaff = 0;
    int legerLines = 0;

    (void)(elt->event()->get<Int>(HEIGHT_ON_STAFF, heightOnStaff));
    if (heightOnStaff < 0) {
        legerLines = heightOnStaff;
    } else if (heightOnStaff > 8) {
        legerLines = heightOnStaff - 8;
    }

    params.setNoteType(note);
    params.setDots(dots);
    params.setAccidental(accidental);
    params.setNoteHeadShifted(shifted);
    params.setDrawFlag(flag);
    params.setDrawStem(true);
    params.setStemGoesUp(up);
    params.setLegerLines(legerLines);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.removeMarks();

    if (elt->event()->get<Bool>(CHORD_PRIMARY_NOTE)) {
	long markCount = 0;
	(void)(elt->event()->get<Int>(MARK_COUNT, markCount));
	if (markCount == 0) {
	} else {
	    std::vector<Rosegarden::Mark> marks;
	    for (int i = 0; i < markCount; ++i) {
		Rosegarden::Mark mark;
		if (elt->event()->get<String>(getMarkPropertyName(i), mark)) {
		    marks.push_back(mark);
		}
	    }
	    params.setMarks(marks);
	}
    }

    long tieLength = 0;
    (void)(elt->event()->get<Int>(TIE_LENGTH, tieLength));
    if (tieLength > 0) {
        params.setTied(true);
        params.setTieLength(tieLength);
    } else {
        params.setTied(false);
    }

    if (beamed) {

        if (elt->event()->get<Bool>(CHORD_PRIMARY_NOTE)) {

            int myY = elt->event()->get<Int>(BEAM_MY_Y);

            stemLength = myY - (int)elt->getLayoutY();
            if (stemLength < 0) stemLength = -stemLength;

            int nextBeamCount =
                elt->event()->get<Int>(BEAM_NEXT_BEAM_COUNT);
            int width =
                elt->event()->get<Int>(BEAM_SECTION_WIDTH);
            int gradient =
                elt->event()->get<Int>(BEAM_GRADIENT);

            bool thisPartialBeams(false), nextPartialBeams(false);
            (void)elt->event()->get<Bool>
                (BEAM_THIS_PART_BEAMS, thisPartialBeams);
            (void)elt->event()->get<Bool>
                (BEAM_NEXT_PART_BEAMS, nextPartialBeams);

	    params.setBeamed(true);
            params.setNextBeamCount(nextBeamCount);
            params.setThisPartialBeams(thisPartialBeams);
            params.setNextPartialBeams(nextPartialBeams);
            params.setWidth(width);
            params.setGradient((double)gradient / 100.0);

        } else {
            params.setBeamed(false);
            params.setDrawStem(false);
        }
    }
    
    params.setStemLength(stemLength);
    params.setTupledCount(0);
    long tuplingLineY = 0;
    bool tupled = (elt->event()->get<Int>(TUPLING_LINE_MY_Y, tuplingLineY));

    if (tupled) {
	int tuplingLineWidth =
	    elt->event()->get<Int>(TUPLING_LINE_WIDTH);
	double tuplingLineGradient =
	    (double)(elt->event()->get<Int>(TUPLING_LINE_GRADIENT)) / 100.0;

	long tupledCount;
	if (elt->event()->get<Int>(BEAMED_GROUP_TUPLED_COUNT, tupledCount)) {
	    params.setTupledCount(tupledCount);
	    params.setTuplingLineY(tuplingLineY - (int)elt->getLayoutY());
	    params.setTuplingLineWidth(tuplingLineWidth);
	    params.setTuplingLineGradient(tuplingLineGradient);
	}
    }

    QCanvasPixmap notePixmap(m_npf->makeNotePixmap(params));
    return new QCanvasNotationSprite(*elt,
                                     new QCanvasPixmap(notePixmap), m_canvas);
}

