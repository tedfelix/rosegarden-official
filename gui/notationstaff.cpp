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

#include "notationstaff.h"
#include "staffline.h"
#include "qcanvassimplesprite.h"
#include "notationproperties.h"
#include "notationview.h" // for EventSelection
#include "BaseProperties.h"

#include "rosedebug.h"

#include "Event.h"
#include "Track.h"
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
using Rosegarden::Segment;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::TimeSignature;
using Rosegarden::PropertyName;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

const int NotationStaff::nbLines      = 5;
const int NotationStaff::nbLegerLines = 5;
const int NotationStaff::linesOffset  = 55;

using std::string;

NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             unsigned int id,
                             string fontName, int resolution) :
    Rosegarden::Staff<NotationElement>(*segment),
    QCanvasItemGroup(canvas),
    m_id(id),
    m_barLineHeight(0),
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
    // staff-line (where 0 <= m <= 4) is m * lineWidth + linesOffset.
    // For a staff at height h, m = (8-h)/2.  Therefore the y-coord of
    // a staff at height h is (8-h)/2 * lineWidth + linesOffset

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

    m_barLineHeight = (nbLines - 1) * m_npf->getLineSpacing();

    delete m_initialBarA;
    delete m_initialBarB;

    // First line - thick
    //
    QPen pen(black, 3);
    pen.setCapStyle(Qt::SquareCap);
    m_initialBarA = new QCanvasLineGroupable(canvas(), this);
    m_initialBarA->setPen(pen);
    m_initialBarA->setPoints(0, linesOffset + 1,
                             0, m_barLineHeight + linesOffset - 1);
    
    // Second line - thin
    //
    m_initialBarB = new QCanvasLineGroupable(canvas(), this);
    m_initialBarB->setPoints(4, linesOffset,
                             4, m_barLineHeight + linesOffset);
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
    y = linesOffset + (y * m_npf->getLineSpacing()) / 2;
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

        barLine->setPoints(0, linesOffset,
                           0, getBarLineHeight() + linesOffset);
        barLine->moveBy(barPos + x() + i, y());
        if (!correct) barLine->setPen(QPen(red, 1));
        barLine->show();

        LineList::iterator insertPoint = lower_bound(m_barLines.begin(),
                                                     m_barLines.end(),
                                                     barLine, compareBarPos);

        m_barLines.insert(insertPoint, barLine);
    }
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
    for (LineList::iterator i = m_staffLines.begin();
         i != m_staffLines.end(); ++i) {

        QPoint p = (*i)->startPoint();
        (*i)->setPoints((int)xfrom - 4, p.y(), (int)xto, p.y());
    }

    QPoint sp = m_initialBarA->startPoint();
    QPoint ep = m_initialBarA->endPoint();

    m_initialBarA->setPoints((int)xfrom - 4, sp.y(), (int)xfrom - 4, ep.y());
    m_initialBarB->setPoints((int)xfrom, sp.y(), (int)xfrom, ep.y());
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


bool NotationStaff::showElements()
{
    NotationElementList *notes = getViewElementList();
    return showElements(notes->begin(), notes->end(), false);
}

bool NotationStaff::showElements(NotationElementList::iterator from,
				 NotationElementList::iterator to,
				 bool positionOnly)
{
    return showElements(from, to, positionOnly ? PositionRefresh : FullRefresh);
}

bool NotationStaff::showElements(NotationElementList::iterator from,
				 NotationElementList::iterator to,
				 RefreshType refresh,
				 const EventSelection *selection)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::showElements()" << endl;

    START_TIMING;

    Clef currentClef; // default is okay to start with
    m_clefChanges.empty();

    Rosegarden::Key currentKey; // likewise
    m_keyChanges.empty();

    NotationElementList::iterator end = getViewElementList()->end();

    for (NotationElementList::iterator it = from; it != end; ++it) {

	if ((*it)->event()->isa(Clef::EventType)) {
	    currentClef = Clef(*(*it)->event());
	    m_clefChanges.push_back(ClefChange((*it)->getLayoutX(),
					       currentClef));
	}

	if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
	    currentKey = Rosegarden::Key(*(*it)->event());
	    m_keyChanges.push_back(KeyChange((*it)->getLayoutX(),
					     currentKey));
	}

	bool needNewSprite = true;
	bool needBlueSprite = false;

	switch (refresh) {

	case FullRefresh:
	    break;

	case PositionRefresh: 
	    if ((*it)->canvasItem()) {

		// We can't only reposition if the event is a beamed
		// or tied-forward note, because changing the position
		// normally requires changing the beam or tie's length
		// and/or angle as well

		if ((*it)->isNote()) {
                
		    bool spanning = false;
		    (void)((*it)->event()->get<Bool>(BEAMED, spanning));
		    if (!spanning)
			(void)((*it)->event()->get<Bool>
			       (TIED_FORWARD, spanning));
		    if (!spanning) needNewSprite = false;

		} else {
		    needNewSprite = false;
		}
	    }
	    break;

	case SelectionRefresh:
	    if ((*it)->canvasItem()) {

		bool selected = false;
		(void)((*it)->event()->get<Bool>(SELECTED, selected));

		bool inNewSelection =
		    selection && selection->contains((*it)->event());

		if (selected != inNewSelection) {
		    needNewSprite = true;
		    if (inNewSelection) {
			(*it)->event()->set<Bool>(SELECTED, true);
		    } else {
			(*it)->event()->unset(SELECTED);
		    }
		} else {
		    needNewSprite = false;
		}
	    }
	    break;
        }

	if (!needNewSprite) {
	    (*it)->reposition(x(), y());
	    continue;
	}

	(void)((*it)->event()->get<Bool>(SELECTED, needBlueSprite));

	if (needBlueSprite) {
	    m_npf->setSelected(true);
	} else {
	    m_npf->setSelected(false);
	}

        try {

            QCanvasPixmap *pixmap = 0;
            QCanvasSimpleSprite *sprite = 0;

//	    kdDebug(KDEBUG_AREA) << "\nNotationStaff::showElements: Event is:" << endl;
//	    (*it)->event()->dump(std::cerr);

            if ((*it)->isNote()) {

                sprite = makeNoteSprite(it);

            } else if ((*it)->isRest()) {

		timeT duration = (*it)->event()->get<Int>
		    (Rosegarden::Quantizer::LegatoDurationProperty);

		if (duration > 0) {
		    Note::Type note = (*it)->event()->get<Int>(NOTE_TYPE);
		    int dots = (*it)->event()->get<Int>(NOTE_DOTS);
		    pixmap = new QCanvasPixmap
			(m_npf->makeRestPixmap(Note(note, dots)));
		} else {
		    kdDebug(KDEBUG_AREA) << "Omitting too-short rest" << endl;
		}

            } else if ((*it)->event()->isa(Clef::EventType)) {

                pixmap = new QCanvasPixmap
                    (m_npf->makeClefPixmap(Rosegarden::Clef(*(*it)->event())));

            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

                pixmap = new QCanvasPixmap
                    (m_npf->makeKeyPixmap
		     (Rosegarden::Key(*(*it)->event()), currentClef));

            } else {
                    
                kdDebug(KDEBUG_AREA)
                    << "NotationElement of unrecognised type "
                    << (*it)->event()->getType() << endl;
                pixmap = new QCanvasPixmap(m_npf->makeUnknownPixmap());
            }

            if (!sprite && pixmap) {
                sprite = new QCanvasNotationSprite(*(*it), pixmap, canvas());
            }

            // Show the sprite
            //
            if (sprite) {
                (*it)->setCanvasItem(sprite, x(), y());
		if (needBlueSprite) sprite->setZ(2);
		else sprite->setZ(0);
                sprite->show();
            } else {
		(*it)->removeCanvasItem();
	    }
            
        } catch (...) {
            kdDebug(KDEBUG_AREA) << "Event lacks the proper properties: "
				 << (*(*it)->event())
                                 << endl;
        }

        if (it == to && refresh == FullRefresh) {
	    refresh = PositionRefresh; // from now on
	}
    }

    kdDebug(KDEBUG_AREA) << "NotationStaff::showElements() exiting" << endl;

    PRINT_ELAPSED("NotationStaff::showElements");
    return true;
}


QCanvasSimpleSprite *NotationStaff::makeNoteSprite(NotationElementList::iterator it)
{
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = (*it)->event()->get<Int>(NOTE_TYPE);
    int dots = (*it)->event()->get<Int>(NOTE_DOTS);

    Accidental accidental = NoAccidental;

    long acc;
    if ((*it)->event()->get<Int>(DISPLAY_ACCIDENTAL, acc)) {
        accidental = Accidental(acc);
    }

    bool up = true;
    (void)((*it)->event()->get<Bool>(STEM_UP, up));

    bool flag = true;
    (void)((*it)->event()->get<Bool>(DRAW_FLAG, flag));

    bool beamed = false;
    (void)((*it)->event()->get<Bool>(BEAMED, beamed));

    bool shifted = false;
    (void)((*it)->event()->get<Bool>(NOTE_HEAD_SHIFTED, shifted));

    long stemLength = m_npf->getNoteBodyHeight();
    (void)((*it)->event()->get<Int>(UNBEAMED_STEM_LENGTH, stemLength));
    
    long heightOnStaff = 0;
    int legerLines = 0;

    (void)((*it)->event()->get<Int>(HEIGHT_ON_STAFF, heightOnStaff));
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

    long tieLength;
    (void)((*it)->event()->get<Int>(TIE_LENGTH, tieLength));
    if (tieLength > 0) {
        params.setTied(true);
        params.setTieLength(tieLength);
    } else {
        params.setTied(false);
    }

    if (beamed) {

        if ((*it)->event()->get<Bool>(BEAM_PRIMARY_NOTE)) {

            int myY = (*it)->event()->get<Int>(BEAM_MY_Y);

            stemLength = myY - (int)(*it)->getLayoutY();
            if (stemLength < 0) stemLength = -stemLength;

            int nextBeamCount =
                (*it)->event()->get<Int>(BEAM_NEXT_BEAM_COUNT);
            int width =
                (*it)->event()->get<Int>(BEAM_SECTION_WIDTH);
            int gradient =
                (*it)->event()->get<Int>(BEAM_GRADIENT);

            bool thisPartialBeams(false), nextPartialBeams(false);
            (void)(*it)->event()->get<Bool>
                (BEAM_THIS_PART_BEAMS, thisPartialBeams);
            (void)(*it)->event()->get<Bool>
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
    bool tupled = ((*it)->event()->get<Int>(TUPLING_LINE_MY_Y, tuplingLineY));

    if (tupled) {
	int tuplingLineWidth =
	    (*it)->event()->get<Int>(TUPLING_LINE_WIDTH);
	double tuplingLineGradient =
	    (double)((*it)->event()->get<Int>(TUPLING_LINE_GRADIENT)) / 100.0;

	long tupledCount;
	if ((*it)->event()->get<Int>(BEAMED_GROUP_TUPLED_COUNT, tupledCount)) {
	    params.setTupledCount(tupledCount);
	    params.setTuplingLineY(tuplingLineY - (int)(*it)->getLayoutY());
	    params.setTuplingLineWidth(tuplingLineWidth);
	    params.setTuplingLineGradient(tuplingLineGradient);
	}
    }

    params.setStemLength(stemLength);
    QCanvasPixmap notePixmap(m_npf->makeNotePixmap(params));
    return new QCanvasNotationSprite(*(*it),
                                     new QCanvasPixmap(notePixmap), canvas());
}

bool NotationStaff::clearSelection()
{
    if (m_haveSelection) {
	NotationElementList *notes = getViewElementList();
	bool shown = showElements(notes->begin(), notes->end(),
				  SelectionRefresh, 0);
	m_haveSelection = false;
	return shown;
    } else return true;
}

bool NotationStaff::showSelection(const EventSelection *selection)
{
    NotationElementList *notes = getViewElementList();
    bool shown;

    if (!selection) {
	shown = showElements
	    (notes->begin(), notes->end(), SelectionRefresh, 0);
	m_haveSelection = false;
	return shown;
    }

    const Segment *segment = &(selection->getSegment());
    if (segment != &getSegment()) return false;

    shown = showElements
	(notes->begin(), notes->end(), SelectionRefresh, selection);

    if (shown) m_haveSelection = true;
    return shown;
}


