// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#include <kmessagebox.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>

#include "notationview.h"
#include "notationstaff.h"
#include "qcanvassimplesprite.h"
#include "notationproperties.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "colours.h"
#include "notestyle.h"
#include "widgets.h"

#include "Event.h"
#include "Segment.h"
#include "Quantizer.h"
#include "NotationTypes.h"
#include "SegmentNotationHelper.h"
#include "Selection.h"
#include "Composition.h"
#include "BaseProperties.h"

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

using std::string;


NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             Rosegarden::SnapGrid *snapGrid, int id,
			     NotationView *view,
			     bool pageMode, double pageWidth,
                             string fontName, int resolution) :
    ProgressReporter(0),
    LinedStaff<NotationElement>(canvas, segment, snapGrid, id, resolution,
				resolution / 16 + 1, // line thickness
				pageMode, pageWidth,
				0 // row spacing
	                        ),
    m_notePixmapFactory(0),
    m_graceNotePixmapFactory(0),
    m_previewSprite(0),
    m_staffName(0),
    m_notationView(view),
    m_colourQuantize(true),
    m_showUnknowns(true)
{
    changeFont(fontName, resolution);
}

NotationStaff::~NotationStaff()
{
    deleteTimeSignatures();
    delete m_notePixmapFactory;
    delete m_graceNotePixmapFactory;
}

void
NotationStaff::changeFont(string fontName, int size) 
{
    setResolution(size);

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(fontName, size);

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(fontName);
    int graceSize = size;
    for (unsigned int i = 0; i < sizes.size(); ++i) {
	if (sizes[i] == size) break;
	graceSize = sizes[i];
    }
    delete m_graceNotePixmapFactory;
    m_graceNotePixmapFactory = new NotePixmapFactory(fontName, graceSize);
}

void
NotationStaff::insertTimeSignature(double layoutX,
				   const TimeSignature &timeSig)
{
    m_notePixmapFactory->setSelected(false);
    QCanvasPixmap *pixmap = m_notePixmapFactory->makeTimeSigPixmap(timeSig);
    QCanvasSimpleSprite *sprite = new QCanvasSimpleSprite(pixmap, m_canvas);

    LinedStaffCoords sigCoords =
	getCanvasCoordsForLayoutCoords(layoutX, getLayoutYForHeight(4));

    sprite->move(sigCoords.first, (double)sigCoords.second);
    sprite->show();
    m_timeSigs.insert(sprite);
}

void
NotationStaff::deleteTimeSignatures()
{
    NOTATION_DEBUG << "NotationStaff::deleteTimeSignatures()\n";
    
    for (SpriteSet::iterator i = m_timeSigs.begin();
	 i != m_timeSigs.end(); ++i) {
        delete (*i);
    }

    m_timeSigs.clear();
}

void
NotationStaff::drawStaffName()
{
    delete m_staffName;

    std::string name =
	getSegment().getComposition()->
	getTrackByIndex(getSegment().getTrack())->getLabel();

    m_staffName = new QCanvasSimpleSprite
	(m_notePixmapFactory->makeTextPixmap
	  (Rosegarden::Text(name, Rosegarden::Text::StaffName)),
	 m_canvas);

    int layoutY = getLayoutYForHeight(5);
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(0, layoutY);
    m_staffName->move(getX() + m_notePixmapFactory->getNoteBodyWidth(),
		      (double)coords.second);
    m_staffName->show();
}

void
NotationStaff::getClefAndKeyAtCanvasCoords(double cx, int cy,
					   Clef &clef, 
					   Rosegarden::Key &key) const
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);
    int i;

    for (i = 0; i < m_clefChanges.size(); ++i) {
	if (m_clefChanges[i].first > layoutCoords.first) break;
	clef = m_clefChanges[i].second;
    }

    for (i = 0; i < m_keyChanges.size(); ++i) {
	if (m_keyChanges[i].first > layoutCoords.first) break;
	key = m_keyChanges[i].second;
    }
}

NotationElementList::iterator
NotationStaff::getClosestElementToCanvasCoords(double cx, int cy,
					       Rosegarden::Event *&clef,
					       Rosegarden::Event *&key,
					       bool notesAndRestsOnly,
					       int proximityThreshold)
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);

    return getClosestElementToLayoutX
	(layoutCoords.first, clef, key, notesAndRestsOnly, proximityThreshold);
}


NotationElementList::iterator
NotationStaff::getClosestElementToLayoutX(double x,
					  Rosegarden::Event *&clef,
					  Rosegarden::Event *&key,
					  bool notesAndRestsOnly,
					  int proximityThreshold)
{
    START_TIMING;

    double minDist = 10e9, prevDist = 10e9;

    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it, result;

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {

	bool before = ((*it)->getLayoutX() < x);
	
	if (!(*it)->isNote() && !(*it)->isRest()) {
	    if (before) {
		if ((*it)->event()->isa(Clef::EventType)) {
		    clef = (*it)->event();
		} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
		    key = (*it)->event();
		}
	    }
	    if (notesAndRestsOnly) continue;
	}

	double dx = x - (*it)->getLayoutX();
	if (dx < 0) dx = -dx;

	if (dx < minDist) {
	    minDist = dx;
	    result = it;
	} else if (!before) {
	    break;
	}

	prevDist = dx;
    }

    if (proximityThreshold > 0 && minDist > proximityThreshold) {
        NOTATION_DEBUG << "NotationStaff::getClosestElementToLayoutX() : element is too far away : "
                             << minDist << endl;
        return notes->end();
    }
        
    NOTATION_DEBUG << "NotationStaff::getClosestElementToLayoutX: found element at layout " << (*result)->getLayoutX() << " - we're at layout " << x << endl;

    PRINT_ELAPSED("NotationStaff::getClosestElementToLayoutX");

    return result;
}


NotationElementList::iterator
NotationStaff::getElementUnderCanvasCoords(double cx, int cy,
					   Rosegarden::Event *&clef,
					   Rosegarden::Event *&key)
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);
    return getElementUnderLayoutX(layoutCoords.first, clef, key);
}

NotationElementList::iterator
NotationStaff::getElementUnderLayoutX(double x,
				      Rosegarden::Event *&clef,
				      Rosegarden::Event *&key)
{
    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it;

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {

	bool before = ((*it)->getLayoutX() < x);
	
	if (!(*it)->isNote() && !(*it)->isRest()) {
	    if (before) {
		if ((*it)->event()->isa(Clef::EventType)) {
		    clef = (*it)->event();
		} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
		    key = (*it)->event();
		}
	    }
	}

	double airX, airWidth;
	(*it)->getLayoutAirspace(airX, airWidth);
	if (x >= airX && x < airX + airWidth) {
	    return it;
	} else if (!before) {
	    break;
	}
    }

    return notes->end();
}

 
string
NotationStaff::getNoteNameAtCanvasCoords(double x, int y,
					 Rosegarden::Accidental acc) const
{
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
    NOTATION_DEBUG << "NotationStaff " << this << "::renderElements()" << endl;
    START_TIMING;
    
    KConfig *config = kapp->config();
    config->setGroup("Notation Options");
    m_colourQuantize = config->readBoolEntry("colourquantize", true);
    m_showUnknowns = config->readBoolEntry("showunknowns", true);

    emit setOperationName(i18n("Rendering staff %1...").arg(getId() + 1));
    emit setProgress(0);
    kapp->processEvents();

    throwIfCancelled();//!!!

    Clef currentClef; // default is okay to start with

    int elementCount = 0;
    timeT endTime =
	(to != getViewElementList()->end() ? (*to)->getAbsoluteTime() :
	 getSegment().getEndMarkerTime());
    timeT startTime = (from != to ? (*from)->getAbsoluteTime() : endTime);

    for (NotationElementList::iterator it = from, nextIt = from;
	 it != to; it = nextIt) {
	
	++nextIt;

	if ((*it)->event()->isa(Clef::EventType)) {
	    currentClef = Clef(*(*it)->event());
	}

	bool selected = isSelected(it);
//	NOTATION_DEBUG << "Rendering at " << (*it)->getAbsoluteTime()
//			     << " (selected = " << selected << ")" << endl;

	renderSingleElement(*it, (nextIt == to ? 0 : *nextIt),
			    currentClef, selected);

	if ((endTime > startTime) &&
	    (++elementCount % 20 == 0)) {

	    timeT myTime = (*it)->getAbsoluteTime();
	    emit setProgress((myTime - startTime) * 100 / (endTime - startTime));
	    kapp->processEvents();
	    throwIfCancelled();//!!!
	}
    }

    NOTATION_DEBUG << "NotationStaff::renderElements: "
			 << elementCount << " elements rendered" << endl;

    PRINT_ELAPSED("NotationStaff::renderElements");
}	

void
NotationStaff::positionElements(timeT from, timeT to)
{
    NOTATION_DEBUG << "NotationStaff " << this << "::positionElements()"
                         << from << " -> " << to << "\n";
    START_TIMING;
    emit setOperationName(i18n("Positioning staff %1...").arg(getId() + 1));
    emit setProgress(0);
    kapp->processEvents();
    throwIfCancelled();//!!!

    const NotationProperties &properties(m_notationView->getProperties());

    int elementsPositioned = 0;
    int elementsRendered = 0; // diagnostic
    
    timeT nextBarTime;
    NotationElementList::iterator beginAt = findUnchangedBarStart(from);
    NotationElementList::iterator endAt = findUnchangedBarEnd(to, nextBarTime);
    if (beginAt == getViewElementList()->end()) return;

    truncateClefsAndKeysAt(static_cast<int>((*beginAt)->getLayoutX()));

    Clef currentClef; // used for rendering key sigs
    bool haveCurrentClef = false;

    for (NotationElementList::iterator it = beginAt, nextIt = beginAt;
	 it != endAt; it = nextIt) {

	++nextIt;

	if ((*it)->event()->isa(Clef::EventType)) {

	    currentClef = Clef(*(*it)->event());
	    m_clefChanges.push_back(ClefChange(int((*it)->getLayoutX()),
					       currentClef));
	    haveCurrentClef = true;

	} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

	    m_keyChanges.push_back
		(KeyChange(int((*it)->getLayoutX()),
			   Rosegarden::Key(*(*it)->event())));

	    if (!haveCurrentClef) { // need this to know how to present the key
		Rosegarden::SegmentNotationHelper helper(getSegment());
		Rosegarden::Key someKey;
		helper.getClefAndKeyAt
		    ((*it)->getAbsoluteTime(), currentClef, someKey);
		haveCurrentClef = true;
	    }
	}

	bool selected = isSelected(it); 
	bool needNewSprite = (selected != (*it)->isSelected());

	if (!(*it)->getCanvasItem()) {

	    needNewSprite = true;

	} else if ((*it)->isNote() && !(*it)->isRecentlyRegenerated()) {

	    // If the note's y-coordinate has changed, we should
	    // redraw it -- its stem direction may have changed, or it
	    // may need leger lines.  This will happen e.g. if the
	    // user inserts a new clef; unfortunately this means
	    // inserting clefs is rather slow.
	    
	    needNewSprite = needNewSprite || !elementNotMovedInY(*it);
	    
	    if (!needNewSprite) {

		// If the event is a beamed or tied-forward note, then
		// we might need a new sprite if the distance from
		// this note to the next has changed (because the beam
		// or tie is part of the note's sprite).

		bool spanning = false;
		(void)((*it)->event()->get<Bool>
		       (properties.BEAMED, spanning));
		if (!spanning) {
		    (void)((*it)->event()->get<Bool>(TIED_FORWARD, spanning));
		}
	    
		if (spanning) {
		    needNewSprite =
			((*it)->getAbsoluteTime() < nextBarTime ||
			 !elementShiftedOnly(it));
		}
	    }

	} else if ((*it)->event()->isa(Indication::EventType) &&
		   !(*it)->isRecentlyRegenerated()) {
	    needNewSprite = true;
	}

	if (needNewSprite) {
	    renderSingleElement(*it, (nextIt == endAt ? 0 : *nextIt),
				currentClef, selected);
	    ++elementsRendered;
	}

	LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
	    ((*it)->getLayoutX(), (int)(*it)->getLayoutY());
	(*it)->reposition(coords.first, (double)coords.second);
	(*it)->setSelected(selected);

	if ((to > from) &&
	    (++elementsPositioned % 20 == 0)) {
	    timeT myTime = (*it)->getAbsoluteTime();
	    emit setProgress((myTime - from) * 100 / (to - from));
	    kapp->processEvents();
	    throwIfCancelled();//!!!
	}
    }

    NOTATION_DEBUG << "NotationStaff::positionElements: "
			 << elementsPositioned << " elements positioned, "
			 << elementsRendered << " elements re-rendered"
			 << endl;

    PRINT_ELAPSED("NotationStaff::positionElements");
    NotePixmapFactory::dumpStats(std::cerr);
}

void
NotationStaff::truncateClefsAndKeysAt(int x)
{
    for (FastVector<ClefChange>::iterator i = m_clefChanges.begin();
	 i != m_clefChanges.end(); ++i) {
	if (i->first >= x) {
	    m_clefChanges.erase(i, m_clefChanges.end());
	    break;
	}
    }
    
    for (FastVector<KeyChange>::iterator i = m_keyChanges.begin();
	 i != m_keyChanges.end(); ++i) {
	if (i->first >= x) {
	    m_keyChanges.erase(i, m_keyChanges.end());
	    break;
	}
    }
}

NotationElementList::iterator
NotationStaff::findUnchangedBarStart(timeT from)
{
    NotationElementList *nel = (NotationElementList *)getViewElementList();

    // Track back bar-by-bar until we find one whose start position
    // hasn't changed

    NotationElementList::iterator beginAt = nel->begin();
    do {
	from = getSegment().getBarStartForTime(from - 1);
	beginAt = nel->findTime(from);
    } while (beginAt != nel->begin() &&
	     (beginAt == nel->end() || !elementNotMoved(*beginAt)));

    return beginAt;
}

NotationElementList::iterator
NotationStaff::findUnchangedBarEnd(timeT to, timeT &nextBarTime)
{
    NotationElementList *nel = (NotationElementList *)getViewElementList();

    // Track forward to the end, similarly.  Here however it's very
    // common for all the positions to have changed right up to the
    // end of the piece; so we save time by assuming that to be the
    // case if we get more than (arbitrary) 3 changed bars.

    // We also record the start of the bar following the changed
    // section, for later use.

    NotationElementList::iterator endAt = nel->end();
    nextBarTime = -1;

    int changedBarCount = 0;
    NotationElementList::iterator candidate = nel->end();
    do {
	candidate = nel->findTime(getSegment().getBarEndForTime(to));
	if (candidate != nel->end()) {
	    to = (*candidate)->getAbsoluteTime();
	    if (nextBarTime < 0) nextBarTime = to;
	} else {
	    nextBarTime = getSegment().getEndTime();
	}
	++changedBarCount;
    } while (changedBarCount < 4 &&
	     candidate != nel->end() && !elementNotMoved(*candidate));

    if (changedBarCount < 4) return candidate;
    else return endAt;
}


bool
NotationStaff::elementNotMoved(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());

    bool ok =
	(int)(elt->getCanvasX()) == (int)(coords.first) &&
	(int)(elt->getCanvasY()) == (int)(coords.second);


//    if (!ok) {
//	NOTATION_DEBUG
//	    << "elementNotMoved: elt at " << elt->getAbsoluteTime() <<
//	    ", ok is " << ok << endl;
//	std::cerr << "(cf " << (int)(elt->getCanvasX()) << " vs "
//		  << (int)(coords.first) << ", "
//		  << (int)(elt->getCanvasY()) << " vs "
//		  << (int)(coords.second) << ")" << std::endl;
//    }
    return ok;
}

bool
NotationStaff::elementNotMovedInY(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());

    bool ok = (int)(elt->getCanvasY()) == (int)(coords.second);

//     if (!ok) {
// 	NOTATION_DEBUG
// 	    << "elementNotMovedInY: elt at " << elt->getAbsoluteTime() <<
// 	    ", ok is " << ok << endl;
// 	std::cerr << "(cf " << (int)(elt->getCanvasY()) << " vs "
// 		  << (int)(coords.second) << ")" << std::endl;
//     }
    return ok;
}

bool
NotationStaff::elementShiftedOnly(NotationElementList::iterator i)
{
    int shift = 0;
    bool ok = false;

    for (NotationElementList::iterator j = i;
	 j != getViewElementList()->end(); ++j) {

	NotationElement *elt = *j;
	if (!elt->getCanvasItem()) break;

	LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	    (elt->getLayoutX(), (int)elt->getLayoutY());
	
	// regard any shift in y as suspicious
	if ((int)(elt->getCanvasY()) != (int)(coords.second)) break;

	int myShift = (int)(elt->getCanvasX()) - (int)(coords.first);
	if (j == i) shift = myShift;
	else if (myShift != shift) break;
	
	if (elt->getAbsoluteTime() > (*i)->getAbsoluteTime()) {
	    // all events up to and including this one have passed
	    ok = true;
	    break;
	}
    }

    if (!ok) {
	NOTATION_DEBUG 
	    << "elementShiftedOnly: elt at " << (*i)->getAbsoluteTime()
	    << ", ok is " << ok << endl;
    }

    return ok;
}


void
NotationStaff::renderSingleElement(NotationElement *elt,
				   NotationElement *nextElt,
				   const Rosegarden::Clef &currentClef,
				   bool selected)
{
    const NotationProperties &properties(m_notationView->getProperties());
    static NotePixmapParameters restParams(Note::Crotchet, 0);

    try {
	m_notePixmapFactory->setNoteStyle
	    (NoteStyleFactory::getStyleForEvent(elt->event()));

    } catch (NoteStyleFactory::StyleUnavailable u) {

	NOTATION_DEBUG
	    << "WARNING: Note style unavailable: " << u.getMessage() << endl;

	static bool warned = false;
	if (!warned) {
	    KMessageBox::error(0, i18n(strtoqstr(u.getMessage())));
	    warned = true;
	}
    }

    try {

	QCanvasPixmap *pixmap = 0;
	QCanvasItem *canvasItem = 0;

	m_notePixmapFactory->setSelected(selected);
	int z = selected ? 3 : 0;

	if (elt->isNote()) {

	    canvasItem = makeNoteSprite(elt);

	} else if (elt->isRest()) {

	    timeT absTime = 
		m_notationView->getLegatoQuantizer()->getQuantizedAbsoluteTime
		(elt->event());
	    timeT duration =
		m_notationView->getLegatoQuantizer()->getQuantizedDuration
		(elt->event());

	    bool ignoreRest = false;
	    elt->event()->get<Bool>(properties.REST_TOO_SHORT, ignoreRest);
	    /*!!!	    
	    bool ignoreRest = (duration == 0);
	    if (!ignoreRest) {
		if (nextElt) {
		    timeT nextTime =
			m_notationView->getLegatoQuantizer()->
			getQuantizedAbsoluteTime(nextElt->event());
		    if (nextTime - absTime
			< Note(Note::Shortest).getDuration()) {
			ignoreRest = true;
		    }
		}
	    }
	    */
	    if (!ignoreRest) {

		Note::Type note =
		    elt->event()->get<Int>(properties.NOTE_TYPE);
		int dots = elt->event()->get<Int>(properties.NOTE_DOTS);
		restParams.setNoteType(note);
		restParams.setDots(dots);
		setTuplingParameters(elt, restParams);
		bool quantized = (absTime != elt->getAbsoluteTime() ||
				  duration != elt->getDuration());
		restParams.setQuantized(quantized);
		if (quantized) z = 2;
		pixmap = m_notePixmapFactory->makeRestPixmap(restParams);

	    } else {
		NOTATION_DEBUG << "Omitting too-short rest" << endl;
	    }

	} else if (elt->event()->isa(Clef::EventType)) {

	    pixmap = m_notePixmapFactory->makeClefPixmap
		 (Rosegarden::Clef(*elt->event()));

	} else if (elt->event()->isa(Rosegarden::Key::EventType)) {

	    pixmap = m_notePixmapFactory->makeKeyPixmap
		 (Rosegarden::Key(*elt->event()), currentClef);

	} else if (elt->event()->isa(Rosegarden::Text::EventType)) {

	    if (elt->event()->has(Rosegarden::Text::TextTypePropertyName) &&
		elt->event()->get<String>
		(Rosegarden::Text::TextTypePropertyName) ==
		Rosegarden::Text::Annotation &&
		!m_notationView->areAnnotationsVisible()) {
		
		//!!! what? we should probably show somewhere that
		// annotations are switched off

	    } else {

		pixmap = m_notePixmapFactory->makeTextPixmap
		     (Rosegarden::Text(*elt->event()));
	    }

	} else if (elt->event()->isa(Indication::EventType)) {

	    timeT indicationDuration =
		elt->event()->get<Int>
		(Indication::IndicationDurationPropertyName);
	    NotationElementList::iterator indicationEnd =
		getViewElementList()->findTime(elt->getAbsoluteTime() +
					       indicationDuration);

	    string indicationType = 
		elt->event()->get<String>
		(Indication::IndicationTypePropertyName);

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
			       m_notePixmapFactory->getNoteBodyWidth() * 3 -
			       elt->getLayoutX());
		y1 = (int)(*indicationEnd)->getLayoutY();
	    }

	    if (length < m_notePixmapFactory->getNoteBodyWidth()) {
		length = m_notePixmapFactory->getNoteBodyWidth();
	    }

	    if (indicationType == Indication::Crescendo) {

		pixmap = m_notePixmapFactory->makeHairpinPixmap(length, true);

	    } else if (indicationType == Indication::Decrescendo) {

		pixmap = m_notePixmapFactory->makeHairpinPixmap(length, false);

	    } else if (indicationType == Indication::Slur) {

		bool above = true;
		long dy = 0;
		long length = 10;
		
		elt->event()->get<Bool>(properties.SLUR_ABOVE, above);
		elt->event()->get<Int>(properties.SLUR_Y_DELTA, dy);
		elt->event()->get<Int>(properties.SLUR_LENGTH, length);
		
		pixmap = m_notePixmapFactory->makeSlurPixmap(length, dy, above);
		    
	    } else {

		NOTATION_DEBUG
		    << "Unrecognised indicationType " << indicationType << endl;
		if (m_showUnknowns) {
		    pixmap = m_notePixmapFactory->makeUnknownPixmap();
		}
	    }

	} else {
                    
	    NOTATION_DEBUG
		<< "NotationElement of unrecognised type "
		<< elt->event()->getType() << endl;
	    if (m_showUnknowns) {
		pixmap = m_notePixmapFactory->makeUnknownPixmap();
	    }
	}

	if (!canvasItem && pixmap) {
	    canvasItem = new QCanvasNotationSprite(*elt, pixmap, m_canvas);
	    canvasItem->setZ(z);
	}

	// Show the sprite
	//
	if (canvasItem) {
	    LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
		(elt->getLayoutX(), (int)elt->getLayoutY());
	    elt->setCanvasItem
		(canvasItem, coords.first, (double)coords.second);
	    canvasItem->show();
	} else {
	    elt->removeCanvasItem();
	}
	
//	NOTATION_DEBUG << "NotationStaff::renderSingleElement: Setting selected at " << elt->getAbsoluteTime() << " to " << selected << endl;
            
    } catch (...) {
	NOTATION_DEBUG << "Event lacks the proper properties: "
			     << (*elt->event())
			     << endl;
    }
}

QCanvasSimpleSprite *
NotationStaff::makeNoteSprite(NotationElement *elt)
{
    const NotationProperties &properties(m_notationView->getProperties());
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = elt->event()->get<Int>(properties.NOTE_TYPE);
    int dots = elt->event()->get<Int>(properties.NOTE_DOTS);

    Accidental accidental = NoAccidental;
    (void)elt->event()->get<String>(properties.DISPLAY_ACCIDENTAL, accidental);

    bool up = true;
    (void)(elt->event()->get<Bool>(STEM_UP, up));

    bool flag = true;
    (void)(elt->event()->get<Bool>(properties.DRAW_FLAG, flag));

    bool beamed = false;
    (void)(elt->event()->get<Bool>(properties.BEAMED, beamed));

    bool shifted = false;
    (void)(elt->event()->get<Bool>(properties.NOTE_HEAD_SHIFTED, shifted));

    long stemLength = m_notePixmapFactory->getNoteBodyHeight();
    (void)(elt->event()->get<Int>(properties.UNBEAMED_STEM_LENGTH, stemLength));
    
    long heightOnStaff = 0;
    int legerLines = 0;

    (void)(elt->event()->get<Int>(properties.HEIGHT_ON_STAFF, heightOnStaff));
    if (heightOnStaff < 0) {
        legerLines = heightOnStaff;
    } else if (heightOnStaff > 8) {
        legerLines = heightOnStaff - 8;
    }

    long slashes = 0;
    (void)(elt->event()->get<Int>(properties.SLASHES, slashes));

    bool quantized = false;
    if (m_colourQuantize && !elt->isTuplet()) {
	timeT absTime = m_notationView->getLegatoQuantizer()->
	    getQuantizedAbsoluteTime(elt->event());
	timeT duration = m_notationView->getLegatoQuantizer()->
	    getQuantizedDuration(elt->event());

	quantized = (absTime != elt->getAbsoluteTime() ||
		     duration != elt->getDuration());
    }
    params.setQuantized(quantized);

    params.setNoteType(note);
    params.setDots(dots);
    params.setAccidental(accidental);
    params.setNoteHeadShifted(shifted);
    params.setDrawFlag(flag);
    params.setDrawStem(true);
    params.setStemGoesUp(up);
    params.setLegerLines(legerLines);
    params.setSlashes(slashes);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.removeMarks();

    if (elt->event()->get<Bool>(properties.CHORD_PRIMARY_NOTE)) {
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
    (void)(elt->event()->get<Int>(properties.TIE_LENGTH, tieLength));
    if (tieLength > 0) {
        params.setTied(true);
        params.setTieLength(tieLength);
    } else {
        params.setTied(false);
    }

    if (beamed) {

        if (elt->event()->get<Bool>(properties.CHORD_PRIMARY_NOTE)) {

            int myY = elt->event()->get<Int>(properties.BEAM_MY_Y);

            stemLength = myY - (int)elt->getLayoutY();
            if (stemLength < 0) stemLength = -stemLength;

            int nextBeamCount =
                elt->event()->get<Int>(properties.BEAM_NEXT_BEAM_COUNT);
            int width =
                elt->event()->get<Int>(properties.BEAM_SECTION_WIDTH);
            int gradient =
                elt->event()->get<Int>(properties.BEAM_GRADIENT);

            bool thisPartialBeams(false), nextPartialBeams(false);
            (void)elt->event()->get<Bool>
                (properties.BEAM_THIS_PART_BEAMS, thisPartialBeams);
            (void)elt->event()->get<Bool>
                (properties.BEAM_NEXT_PART_BEAMS, nextPartialBeams);

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
    setTuplingParameters(elt, params);

    QCanvasNotationSprite *item = 0;

    if (elt->isGrace()) {

	params.setLegerLines(0);
	m_graceNotePixmapFactory->setSelected
	    (m_notePixmapFactory->isSelected());
	item = new QCanvasNotationSprite
	    (*elt, m_graceNotePixmapFactory->makeNotePixmap(params), m_canvas);

    } else {
	item = new QCanvasNotationSprite
	    (*elt, m_notePixmapFactory->makeNotePixmap(params), m_canvas);
    }

    if (m_notePixmapFactory->isSelected()) item->setZ(3);
    else if (quantized) item->setZ(2);
    else item->setZ(0);
    return item;
}

void
NotationStaff::setTuplingParameters(NotationElement *elt,
				    NotePixmapParameters &params)
{
    const NotationProperties &properties(m_notationView->getProperties());

    params.setTupletCount(0);
    long tuplingLineY = 0;
    bool tupled = (elt->event()->get<Int>(properties.TUPLING_LINE_MY_Y, tuplingLineY));

    if (tupled) {
	int tuplingLineWidth =
	    elt->event()->get<Int>(properties.TUPLING_LINE_WIDTH);
	double tuplingLineGradient =
	    (double)(elt->event()->get<Int>(properties.TUPLING_LINE_GRADIENT)) / 100.0;

	long tupletCount;
	if (elt->event()->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT, tupletCount)) {
	    params.setTupletCount(tupletCount);
	    params.setTuplingLineY(tuplingLineY - (int)elt->getLayoutY());
	    params.setTuplingLineWidth(tuplingLineWidth);
	    params.setTuplingLineGradient(tuplingLineGradient);
	}
    }
}

bool
NotationStaff::isSelected(NotationElementList::iterator it)
{
    const Rosegarden::EventSelection *selection =
	m_notationView->getCurrentSelection();
    return selection && selection->contains((*it)->event());
}

void
NotationStaff::showPreviewNote(double layoutX, int heightOnStaff,
			       const Rosegarden::Note &note)
{
    NotePixmapParameters params(note.getNoteType(), note.getDots());

    params.setAccidental(NoAccidental);
    params.setNoteHeadShifted(false);
    params.setDrawFlag(true);
    params.setDrawStem(true);
    params.setStemGoesUp(heightOnStaff <= 4);
    params.setLegerLines(heightOnStaff < 0 ? heightOnStaff :
			 heightOnStaff > 8 ? heightOnStaff - 8 : 0);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.setTied(false);
    params.setBeamed(false);
    params.setTupletCount(0);
    params.setSelected(false);
    params.setHighlighted(true);

    delete m_previewSprite;
    m_previewSprite = new QCanvasSimpleSprite
	(m_notePixmapFactory->makeNotePixmap(params), m_canvas);

    int layoutY = getLayoutYForHeight(heightOnStaff);
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(layoutX, layoutY);

    m_previewSprite->move(coords.first, (double)coords.second);
    m_previewSprite->setZ(4);
    m_previewSprite->show();
    m_canvas->update();
} 

void
NotationStaff::clearPreviewNote()
{
    delete m_previewSprite;
    m_previewSprite = 0;
}

bool
NotationStaff::wrapEvent(Rosegarden::Event *e)
{
    return Rosegarden::Staff<NotationElement>::wrapEvent(e);
}

