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
#include "staffline.h"
#include "qcanvassimplesprite.h"
#include "notationproperties.h"
#include "notationview.h" // for EventSelection
#include "BaseProperties.h"

#include "rosedebug.h"

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
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Segment;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::TimeSignature;
using Rosegarden::PropertyName;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

const int NotationStaff::nbLines      = 5;
const int NotationStaff::nbLegerLines = 8;

using std::string;

NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             unsigned int id,
                             string fontName, int resolution) :
    Rosegarden::Staff<NotationElement>(*segment),
    QCanvasItemGroup(canvas),
    m_id(id),
    m_horizLineLength(0),
    m_initialBarA(0),
    m_initialBarB(0),
    m_npf(0),
    m_haveSelection(false)
{
    setLegatoDuration(Note(Note::Shortest).getDuration());
    int w = canvas->width();
    m_horizLineLength = w - 20;
    changeFont(fontName, resolution);
    setActive(false);  // don't react to mousePress events
}

NotationStaff::~NotationStaff()
{
    // TODO : this causes a crash on quit - don't know why
//     for (LineList::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
//         delete (*i);
}

void
NotationStaff::changeFont(string fontName, int resolution) 
{
    m_resolution = resolution;

    delete m_npf;
    m_npf = new NotePixmapFactory(fontName, resolution);

    // Pitch is represented with the MIDI pitch scale; NotationTypes.h
    // contains methods to convert this to and from staff-height
    // according to the current clef and key.  Staff-height is
    // represented with signed integers such that the bottom staff
    // line is 0, the space immediately above it is 1, and so on up to
    // the top staff line which has a height of 8.  We shouldn't be
    // concerned with pitch in this class, only with staff-height.

    // Now, the y-coord of a staff m whole-lines below the top
    // staff-line (where 0 <= m <= 4) is m * lineWidth + lineOffset.
    // For a staff at height h, m = (8-h)/2.  Therefore the y-coord of
    // a staff at height h is (8-h)/2 * lineWidth + lineOffset

    int h;
    for (h = 0; h < int(m_staffLines.size()); ++h) {
        delete m_staffLines[h];
    }
    m_staffLines.clear();

    // If the resolution is 8 or less, we want to reduce the blackness
    // of the staff lines somewhat to make them less intrusive
    int level = 0;
    int z = 1;
    if (m_resolution < 6) {
        z = -1;
        level = (9 - m_resolution) * 32;
        if (level > 200) level = 200;
    }
    QColor lineColour(level, level, level);

    for (h = -2 * nbLegerLines; h <= (2*(nbLines + nbLegerLines) - 2); ++h) {

        for (int i = 0; i < m_npf->getStaffLineThickness(); ++i) {

            StaffLine *line = new StaffLine(canvas(), this, h);
            int y = yCoordOfHeight(h) + i;
            line->setPoints(0, y, m_horizLineLength, y);
	    if (i > 0) line->setSignificant(false);

            if ((h % 2 == 1) ||
                (h < 0 || h > (nbLines * 2 - 2))) {

                // make the line invisible
                line->setPen(QPen(white, 1));
                line->setZ(-1);
                
            } else {
                line->setPen(QPen(lineColour, 1));
                line->setZ(z);
            }

            line->show();
            m_staffLines.push_back(line);
        }
    }

    delete m_initialBarA;
    delete m_initialBarB;

    // First line - thick
    //
    QPen pen(black, 3);
    pen.setCapStyle(Qt::SquareCap);
    m_initialBarA = new QCanvasLineGroupable(canvas(), this);
    m_initialBarA->setPen(pen);
    m_initialBarA->setPoints(0, getTopLineOffset() + 1,
                             0, getBarLineHeight() + getTopLineOffset() - 1);
    
    // Second line - thin
    //
    m_initialBarB = new QCanvasLineGroupable(canvas(), this);
    m_initialBarB->setPoints(4, getTopLineOffset(),
                             4, getBarLineHeight() + getTopLineOffset());
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

int NotationStaff::yCoordOfHeight(int h) const
{
    // 0 is bottom staff-line, 8 is top one, leger lines above & below

    int y = 8 - h;
    y = getTopLineOffset() + (y * m_npf->getLineSpacing()) / 2;
    if (h > 0 && h < 8 && (h % 2 == 1)) ++y;
    else if (h < 0 && (-h % 2 == 1)) ++y;
    return y;
}

static bool
compareBarPos(QCanvasLineGroupable *barLine1, QCanvasLineGroupable *barLine2)
{
    return barLine1->x() < barLine2->x();
}

static bool
compareBarToPos(QCanvasLineGroupable *barLine1, unsigned int pos)
{
    return barLine1->x() < pos;
}

void NotationStaff::insertBar(unsigned int barPos, bool correct)
{
    for (int i = 0; i < m_npf->getStemThickness(); ++i) {

        QCanvasLineGroupable* barLine =
            new QCanvasLineGroupable(canvas(), this);

        barLine->setPoints(0, getTopLineOffset(),
                           0, getBarLineHeight() + getTopLineOffset());
        barLine->moveBy(barPos + x() + i, y());
        if (!correct) barLine->setPen(QPen(red, 1));
        barLine->show();

        LineList::iterator insertPoint = lower_bound(m_barLines.begin(),
                                                     m_barLines.end(),
                                                     barLine, compareBarPos);

        m_barLines.insert(insertPoint, barLine);
    }
}

QRect
NotationStaff::getBarExtents(unsigned int myx)
{
    QRect rect(x(), y(), 0, getStaffHeight());

    for (int i = 1; i < m_barLines.size(); ++i) {

	if (m_barLines[i]->x() <= myx) continue;
	
	rect.setX(m_barLines[i-1]->x());
	rect.setWidth(m_barLines[i]->x() - m_barLines[i-1]->x());

	return rect;
    }

    return rect;
}


void NotationStaff::insertTimeSignature(unsigned int tsx,
					const TimeSignature &timeSig)
{
    QCanvasPixmap *pixmap =
	new QCanvasPixmap(m_npf->makeTimeSigPixmap(timeSig));
    QCanvasSimpleSprite *sprite = new QCanvasSimpleSprite(pixmap, canvas());
    kdDebug(KDEBUG_AREA) << "Inserting time signature at " << tsx << endl;
    sprite->move(tsx + x(), yCoordOfHeight(4) + y());
    sprite->show();
    m_timeSigs.insert(sprite);
}

void NotationStaff::deleteBars(unsigned int fromPos)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteBars from " << fromPos << endl;

    LineList::iterator startDeletePoint =
        lower_bound(m_barLines.begin(), m_barLines.end(),
                    fromPos, compareBarToPos);

    if (startDeletePoint != m_barLines.end())
        kdDebug(KDEBUG_AREA) << "startDeletePoint pos : "
                             << (*startDeletePoint)->x() << endl;

    // delete the canvas lines
    for (LineList::iterator i = startDeletePoint; i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.erase(startDeletePoint, m_barLines.end());
}

void NotationStaff::deleteBars()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteBars()\n";
    
    for (LineList::iterator i = m_barLines.begin(); i != m_barLines.end(); ++i)
        delete (*i);

    m_barLines.clear();
}

void NotationStaff::deleteTimeSignatures()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteTimeSignatures()\n";
    
    for (SpriteSet::iterator i = m_timeSigs.begin(); i != m_timeSigs.end(); ++i)
        delete (*i);

    m_timeSigs.clear();
}

void NotationStaff::setLines(double xfrom, double xto)
{
    START_TIMING;

    for (LineList::iterator i = m_staffLines.begin();
         i != m_staffLines.end(); ++i) {

        QPoint p = (*i)->startPoint();
        (*i)->setPoints((int)xfrom - 4, p.y(), (int)xto, p.y());
    }

    QPoint sp = m_initialBarA->startPoint();
    QPoint ep = m_initialBarA->endPoint();

    m_initialBarA->setPoints((int)xfrom - 4, sp.y(), (int)xfrom - 4, ep.y());
    m_initialBarB->setPoints((int)xfrom, sp.y(), (int)xfrom, ep.y());

    PRINT_ELAPSED("NotationStaff::setLines");
}

void NotationStaff::getClefAndKeyAtX(int myx, Clef &clef,
				     Rosegarden::Key &key) const
{
    unsigned int i;

    for (i = 0; i < m_clefChanges.size(); ++i) {
	if (m_clefChanges[i].first + x() > myx) break;
	clef = m_clefChanges[i].second;
    }

    for (i = 0; i < m_keyChanges.size(); ++i) {
	if (m_keyChanges[i].first + x() > myx) break;
	key = m_keyChanges[i].second;
    }
}

void
NotationStaff::renderElements()
{
    renderElements(getViewElementList()->begin(),
		   getViewElementList()->end());
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
NotationStaff::positionElements()
{
    positionElements(-1, -1);
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
	} while (candidate != nel->begin() && !elementNotMoved(*candidate));
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

	    (*it)->reposition(x(), y());
	}
    }

    PRINT_ELAPSED("NotationStaff::positionElements");
}


bool
NotationStaff::elementNotMoved(NotationElement *elt)
{
    bool ok =
	((elt->getCanvasItem()) &&
	 (int)(elt->getCanvasItem()->x()) == (int)(elt->getLayoutX() + x()) &&
	 (int)(elt->getCanvasItem()->y()) == (int)(elt->getLayoutY() + y()));
    cerr << "elementNotMoved: elt at " << elt->getAbsoluteTime() <<
	", ok is " << ok << endl;
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
		length = (*indicationEnd)->getLayoutX() - elt->getLayoutX();
		y1 = (*indicationEnd)->getLayoutY();
	    } else {
		//!!! imperfect
		--indicationEnd;
		length = (*indicationEnd)->getLayoutX() +
		    m_npf->getNoteBodyWidth() * 3 - elt->getLayoutX();
		y1 = (*indicationEnd)->getLayoutY();
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
	    canvasItem = new QCanvasNotationSprite(*elt, pixmap, canvas());
	}

	// Show the sprite
	//
	if (canvasItem) {
	    elt->setCanvasItem(canvasItem, x(), y());
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

    elt->reposition(x(), y());
}

QCanvasSimpleSprite *
NotationStaff::makeNoteSprite(NotationElement *elt)
{
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = elt->event()->get<Int>(NOTE_TYPE);
    int dots = elt->event()->get<Int>(NOTE_DOTS);

    Accidental accidental = NoAccidental;

    long acc;
    if (elt->event()->get<Int>(DISPLAY_ACCIDENTAL, acc)) {
        accidental = Accidental(acc);
    }

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
    params.setStemGoesUp(up);
    params.setLegerLines(legerLines);
    params.setBeamed(beamed);
    params.setIsOnLine(heightOnStaff % 2 == 0);

    long markCount = 0;
    (void)(elt->event()->get<Int>(MARK_COUNT, markCount));
    if (markCount == 0) {
	params.removeMarks();
    } else {
	std::vector<Rosegarden::Mark> marks;
	for (int i = 0; i < markCount; ++i) {
	    std::string markName;
	    if (elt->event()->get<String>(getMarkPropertyName(i), markName)) {
		marks.push_back(Note::getMarkByName(markName));
	    }
	}
	params.setMarks(marks);
    }

    long tieLength;
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

            params.setNextBeamCount(nextBeamCount);
            params.setThisPartialBeams(thisPartialBeams);
            params.setNextPartialBeams(nextPartialBeams);
            params.setWidth(width);
            params.setGradient((double)gradient / 100.0);

        } else {
            params.setBeamed(false);
        }
    }
    
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

    params.setStemLength(stemLength);
    QCanvasPixmap notePixmap(m_npf->makeNotePixmap(params));
    return new QCanvasNotationSprite(*elt,
                                     new QCanvasPixmap(notePixmap), canvas());
}

