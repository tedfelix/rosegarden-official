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

const int NotationStaff::nbLines      = 5;
const int NotationStaff::nbLegerLines = 8;

const int NotationStaff::NoHeight = -1000;

using std::string;

NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             unsigned int id, bool pageMode,
			     double pageWidth, 
                             string fontName, int resolution) :
    Rosegarden::Staff<NotationElement>(*segment),
    QCanvasItemGroup(canvas),
    m_id(id),
    m_pageMode(pageMode),
    m_pageWidth(pageWidth),
    m_lineBreakGap(0),
    m_connectingLineHeight(0),
    m_horizLineStart(getRowLeftX(0)),
    m_horizLineEnd(getRowRightX(0)),
    m_initialBarA(0),
    m_initialBarB(0),
    m_npf(0),
    m_haveSelection(false)
{
    setLegatoDuration(Note(Note::Shortest).getDuration());
    changeFont(fontName, resolution);
    setActive(false);  // don't react to mousePress events
}

NotationStaff::~NotationStaff()
{
    for (int i = 0; i < (int)m_staffLines.size(); ++i) clearStaffLineRow(i);
}

void
NotationStaff::changeFont(string fontName, int resolution) 
{
    m_resolution = resolution;

    delete m_npf;
    m_npf = new NotePixmapFactory(fontName, resolution);

    resizeStaffLines();
}

void
NotationStaff::resizeStaffLines()
{
    int firstRow = getRowForLayoutX(m_horizLineStart);
    int  lastRow = getRowForLayoutX(m_horizLineEnd);
    
    int i;

    while ((int)m_staffLines.size() <= lastRow) {
	m_staffLines.push_back(StaffLineList());
	m_staffConnectingLines.push_back(0);
    }

    // Remove all the staff lines that precede the start of the staff

    for (i = 0; i < firstRow; ++i) clearStaffLineRow(i);

    // now i == firstRow

    while (i <= lastRow) {

	double x0 = 0;
	double x1 = getPageWidth();

	if (i == firstRow) {
	    x0 = getXForLayoutX(m_horizLineStart);
	}

	if (i == lastRow) {
	    x1 = getXForLayoutX(m_horizLineEnd);
	}

	resizeStaffLineRow(i, x0, x1 - x0);

	++i;
    }

    // now i == lastRow+1

    while (i < (int)m_staffLines.size()) clearStaffLineRow(i++);
}


// m_staffLines[row] must already exist (although it may be empty)

void
NotationStaff::clearStaffLineRow(int row)
{
    for (int h = 0; h < (int)m_staffLines[row].size(); ++h) {
	delete m_staffLines[row][h];
    }
    m_staffLines[row].clear();

    delete m_staffConnectingLines[row];
    m_staffConnectingLines[row] = 0;
}


// m_staffLines[row] must already exist (although it may be empty)

void
NotationStaff::resizeStaffLineRow(int row, double offset, double length)
{
//    kdDebug(KDEBUG_AREA) << "NotationStaff::resizeStaffLineRow: row "
//			 << row << ", offset " << offset << ", length " 
//			 << length << ", pagewidth " << getPageWidth() << endl;


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

    QColor connectingLineColour(128, 128, 128);

    int h, j;
    int staffLineThickness = m_npf->getStaffLineThickness();

    if (m_pageMode && row > 0 && offset == 0.0) {
	offset = (double)m_npf->getBarMargin() / 2;
	length -= offset;
    }

    QCanvasLine *line;
    double lx;
    int ly;

    delete m_staffConnectingLines[row];
    line = 0;

    if (m_pageMode && m_connectingLineHeight > 0.1) {
	line = new QCanvasLine(canvas());
	lx = (int)x() + getRowLeftX(row) + offset + length - 1;
	ly = (int)y() + getTopLineOffsetForRow(row);
	line->setPoints(lx, ly, lx,
			ly + getBarLineHeight() + m_connectingLineHeight);
	line->setPen(QPen(connectingLineColour, 1));
	line->setZ(-2);
	line->show();
    }

    m_staffConnectingLines[row] = line;

    while ((int)m_staffLines[row].size() <= nbLines * staffLineThickness) {
	m_staffLines[row].push_back(0);
    }

    int lineIndex = 0;

    for (h = 0; h < nbLines; ++h) {

	for (j = 0; j < staffLineThickness; ++j) {

	    if (m_staffLines[row][lineIndex] != 0) {
		line = m_staffLines[row][lineIndex];
	    } else {
		line = new QCanvasLine(canvas());
	    }

	    lx = (int)x() + getRowLeftX(row) + offset;
	    ly = (int)y() + getTopOfStaffForRow(row) +
		yCoordOfHeight(2 * h) + j; //!!!

//	    kdDebug(KDEBUG_AREA) << "My coords: " << x() << "," << y()
//				 << "; setting line points to ("
//				 << lx << "," << ly << ") -> ("
//				 << (lx+length-1) << "," << ly << ")" << endl;

	    line->setPoints(lx, ly, lx + length - 1, ly);

//	    if (j > 0) line->setSignificant(false);

	    line->setPen(QPen(lineColour, 1));
	    line->setZ(z);

	    if (m_staffLines[row][lineIndex] == 0) {
		m_staffLines[row][lineIndex] = line;
	    }

	    line->show();

	    ++lineIndex;
	}
    }

    while (lineIndex < (int)m_staffLines[row].size()) {
	delete m_staffLines[row][lineIndex];
	m_staffLines[row][lineIndex] = 0;
	++lineIndex;
    }
/*
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
    m_initialBarA->show();
    
    // Second line - thin
    //
    m_initialBarB = new QCanvasLineGroupable(canvas(), this);
    m_initialBarB->setPoints(4, getTopLineOffset(),
                             4, getBarLineHeight() + getTopLineOffset());
    m_initialBarB->show();
*/
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


    int y = 8 - h;
    y = getTopLineOffset() + (y * m_npf->getLineSpacing()) / 2;
    if (h > 0 && h < 8 && (h % 2 == 1)) ++y;
    else if (h < 0 && (-h % 2 == 1)) ++y;
    return y;
}

int NotationStaff::heightOfYCoord(int y) const
{
    // 0 is bottom staff-line, 8 is top one, leger lines above & below

    //!!! the lazy route: approximate, then get the right value
    // by calling yCoordOfHeight a few times... ugh

//    kdDebug(KDEBUG_AREA) << "\nNotationStaff::heightOfYCoord: y = " << y
//			 << ", getTopLineOffset() = " << getTopLineOffset()
//			 << ", getLineSpacing() = " << m_npf->getLineSpacing()
//			 << endl;

    int ph = (y - (int)getTopLineOffset()) * 2 / m_npf->getLineSpacing();
    ph = 8 - ph;

    int i;
    int mi = -2;
    int md = m_npf->getLineSpacing() * 2;

    int testi = -2;
    int testMd = 1000;

    for (i = -1; i <= 1; ++i) {
	int d = y - yCoordOfHeight(ph + i);
	if (d < 0) d = -d;
	if (d < md) { md = d; mi = i; }
	if (d < testMd) { testMd = d; testi = i; }
    }
    
    if (mi > -2) {
//	kdDebug(KDEBUG_AREA) << "NotationStaff::heightOfYCoord: " << y
//			     << " -> " << (ph + mi) << " (mi is " << mi << ", distance "
//			     << md << ")" << endl;
	if (mi == 0) {
//	    kdDebug(KDEBUG_AREA) << "GOOD APPROXIMATION" << endl;
	} else {
//	    kdDebug(KDEBUG_AREA) << "BAD APPROXIMATION" << endl;
	}
	return ph + mi;
    } else {
//	kdDebug(KDEBUG_AREA) << "NotationStaff::heightOfYCoord: heuristic got " << ph << ", nothing within range (closest was " << (ph + testi) << " which is " << testMd << " away)" << endl;
	return 0;
    }
}

bool
NotationStaff::compareBarPos(const BarPair &barLine1, const BarPair &barLine2)
{
    return (barLine1.first < barLine2.first);
}

bool
NotationStaff::compareBarToPos(const BarPair &barLine1, unsigned int pos)
{
    return (barLine1.first < pos);
}

void NotationStaff::insertBar(unsigned int barPos, bool correct)
{
    for (int i = 0; i < m_npf->getStemThickness(); ++i) {

        QCanvasLineGroupable *barLine =
            new QCanvasLineGroupable(canvas(), this);

	int row = getRowForLayoutX(barPos);

        barLine->setPoints
	    (0, getTopLineOffsetForRow(row),
	     0, getBarLineHeight() + getTopLineOffsetForRow(row));

        barLine->moveBy
	    (getRowLeftX(row) + getXForLayoutX(barPos) + x() + i,
	     y());

        if (!correct) barLine->setPen(QPen(red, 1));
        barLine->show();
	barLine->setZ(-1);

	BarPair barPair(barPos, barLine);
        BarLineList::iterator insertPoint = lower_bound
	    (m_barLines.begin(), m_barLines.end(), barPair, compareBarPos);
        m_barLines.insert(insertPoint, barPair);

	if (m_connectingLineHeight > 0) {

	    QCanvasLineGroupable *connectingLine =
		new QCanvasLineGroupable(canvas(), this);
	    
	    connectingLine->setPoints
		(0, getBarLineHeight() + getTopLineOffsetForRow(row) + 1,
		 0, getBarLineHeight() + getTopLineOffsetForRow(row) + 1 +
		 m_connectingLineHeight);
	    
	    connectingLine->moveBy
		(getRowLeftX(row) + getXForLayoutX(barPos) + x() + i, y());

	    connectingLine->setPen(QPen(QColor(192, 192, 192), 1));
	    connectingLine->setZ(-3);
	    connectingLine->show();

	    BarPair connectingPair(barPos, connectingLine);
	    insertPoint = lower_bound
		(m_barLines.begin(), m_barLines.end(),
		 connectingPair, compareBarPos);
	    m_barLines.insert(insertPoint, connectingPair);
	}
    }
}

//!!! Can't work with page mode
QRect
NotationStaff::getBarExtents(unsigned int myx)
{
    QRect rect(x(), y(), 0, getStaffHeight());

    for (unsigned int i = 1; i < m_barLines.size(); ++i) {

	if (m_barLines[i].second->x() <= myx) continue;
	
	rect.setX(m_barLines[i-1].second->x());
	rect.setWidth(m_barLines[i].second->x() - m_barLines[i-1].second->x());

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

    BarLineList::iterator startDeletePoint =
        lower_bound(m_barLines.begin(), m_barLines.end(),
                    fromPos, compareBarToPos);

    if (startDeletePoint != m_barLines.end())
        kdDebug(KDEBUG_AREA) << "startDeletePoint pos : "
                             << (*startDeletePoint).first << endl;

    // delete the canvas lines
    for (BarLineList::iterator i = startDeletePoint;
	 i != m_barLines.end(); ++i) {
	delete i->second;
    }

    m_barLines.erase(startDeletePoint, m_barLines.end());
}

void NotationStaff::deleteBars()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteBars()\n";
    
    for (BarLineList::iterator i = m_barLines.begin();
	 i != m_barLines.end(); ++i) {
        delete i->second;
    }

    m_barLines.clear();
}

void NotationStaff::deleteTimeSignatures()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteTimeSignatures()\n";
    
    for (SpriteSet::iterator i = m_timeSigs.begin(); i != m_timeSigs.end(); ++i)
        delete (*i);

    m_timeSigs.clear();
}

void NotationStaff::setLines(double xfrom, double xto, bool sizeCanvas)
{
    START_TIMING;

    m_horizLineStart = (int)xfrom;
    m_horizLineEnd = (int)xto;
    resizeStaffLines();

    if (sizeCanvas) {

	double canvasWidth =
	    (x() * 2) + getRowRightX(getRowForLayoutX(m_horizLineEnd));

	double canvasHeight =
	    (y() * 2) + getTopOfStaffForRow(getRowForLayoutX(m_horizLineEnd)) +
	    getStaffHeight();

	canvas()->resize(canvasWidth, canvasHeight);
    }

    

/*!!!   needs to be cleverer
    for (LineList::iterator i = m_staffLines.begin();
         i != m_staffLines.end(); ++i) {

        QPoint p = (*i)->startPoint();
        (*i)->setPoints((int)xfrom - 4, p.y(), (int)xto, p.y());
    }

    QPoint sp = m_initialBarA->startPoint();
    QPoint ep = m_initialBarA->endPoint();

    m_initialBarA->setPoints((int)xfrom - 4, sp.y(), (int)xfrom - 4, ep.y());
    m_initialBarB->setPoints((int)xfrom, sp.y(), (int)xfrom, ep.y());
*/
    PRINT_ELAPSED("NotationStaff::setLines");
}

//!!! page mode
void NotationStaff::getClefAndKeyAtX(int myx, Clef &clef,
				     Rosegarden::Key &key) const
{
    int i;

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

	    double xoff, yoff;
	    getPageOffsets((*it), xoff, yoff);
	    (*it)->reposition(xoff, yoff);
	}
    }

    PRINT_ELAPSED("NotationStaff::positionElements");
}


bool
NotationStaff::elementNotMoved(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    double xoff, yoff;
    getPageOffsets(elt, xoff, yoff);

    bool ok =
	(int)(elt->getCanvasItem()->x()) == (int)(elt->getLayoutX() + xoff) &&
	(int)(elt->getCanvasItem()->y()) == (int)(elt->getLayoutY() + yoff);
    cerr << "elementNotMoved: elt at " << elt->getAbsoluteTime() <<
	", ok is " << ok << endl;
    if (!ok) {
	cerr << "(cf " << (int)(elt->getCanvasItem()->x()) << " vs "
	     << (int)(elt->getLayoutX() + xoff) << ", "
	     << (int)(elt->getCanvasItem()->y()) << " vs "
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
	    canvasItem = new QCanvasNotationSprite(*elt, pixmap, canvas());
	}

	// Show the sprite
	//
	if (canvasItem) {
	    double xoff, yoff;
	    getPageOffsets(elt, xoff, yoff);
	    elt->setCanvasItem(canvasItem, xoff, yoff);
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

    double xoff, yoff;
    getPageOffsets(elt, xoff, yoff);
    elt->reposition(xoff, yoff);
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
                                     new QCanvasPixmap(notePixmap), canvas());
}

double
NotationStaff::getPageWidth()
{
    return m_pageWidth;
}

int
NotationStaff::getRowForLayoutX(double lx)
{
    return (int)(lx / getPageWidth());
}

double
NotationStaff::getXForLayoutX(double lx)
{
    return (lx - (getPageWidth() * getRowForLayoutX(lx)));
}

int
NotationStaff::getTopOfStaffForRow(int row)
{
    if (!m_pageMode) return 0;
    else return (m_lineBreakGap * row);
}

int
NotationStaff::getTopLineOffsetForRow(int row)
{
    if (!m_pageMode) return getTopLineOffset();
    else return (getTopLineOffset() + (m_lineBreakGap * row));
}

int
NotationStaff::getRowCount()
{
    return getRowForLayoutX(m_horizLineEnd) + 1;
}

double
NotationStaff::getRowLeftX(int row)
{
    if (!m_pageMode) return (row * getPageWidth());
    else return 0;
}

double
NotationStaff::getRowRightX(int row)
{
    if (!m_pageMode) return (row * getPageWidth()) + getPageWidth();
    else return getPageWidth();
}

void
NotationStaff::getPageOffsets(NotationElement *elt,
			      double &xoff, double &yoff)
{
    double lx = elt->getLayoutX();
    double ly = elt->getLayoutY();

    int row = getRowForLayoutX(lx);

    xoff = getXForLayoutX(lx) - lx + getRowLeftX(row) + x();
    yoff = getTopOfStaffForRow(row) + y();
}

