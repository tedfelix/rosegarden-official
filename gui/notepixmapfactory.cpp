// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <cstdio>

#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <map>

#include <qbitmap.h>
#include <qimage.h>

#include <kmessagebox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>

#include "kstartuplogo.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguiview.h"
#include "rosegardenconfigurationpage.h"
#include "notationstrings.h"
#include "notepixmapfactory.h"
#include "pixmapfunctions.h"
#include "NotationTypes.h"
#include "Equation.h"
#include "Profiler.h"

#include "colours.h"
#include "notefont.h"
#include "notestyle.h"
#include "spline.h"


// #define DUMP_STATS 1

#include <iostream>
using std::endl;


using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::TimeSignature;
using Rosegarden::Equation;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using std::set;
using std::string;
using std::vector;

static clock_t drawBeamsTime = 0;
static clock_t makeNotesTime = 0;
static int drawBeamsCount = 0;
static int drawBeamsBeamCount = 0;

class NotePixmapCache : public std::map<CharName, QCanvasPixmap*>
{
    // nothing to add -- just so we can predeclare it in the header
};

class NotePixmapPainter
{
    // Just a trivial class that instructs two painters to do the
    // same thing (one for the pixmap, one for the mask).  We only
    // duplicate those methods we actually use in NotePixmapFactory

public:
    NotePixmapPainter() :
	m_painter(&m_myPainter) { }

    void beginExternal(QPainter *painter) {

	m_externalPainter = painter;
	m_useMask = false;

	if (m_externalPainter) {
	    m_painter = m_externalPainter;
	} else {
	    m_painter = &m_myPainter;
	}
    }

    bool begin(QPaintDevice *device, QPaintDevice *mask = 0, bool unclipped = false) {

	m_externalPainter = 0;

	if (mask) {
	    m_useMask = true;
	    m_maskPainter.begin(mask, unclipped);
	} else {
	    m_useMask = false;
	}

	m_painter = &m_myPainter;
	return m_painter->begin(device, unclipped);
    }

    bool end() {
	if (m_useMask) m_maskPainter.end();
	return m_painter->end();
    }

    QPainter &painter() {
	return *m_painter;
    }

    QPainter &maskPainter() {
	return m_maskPainter;
    }

    void drawPoint(int x, int y) {
	m_painter->drawPoint(x, y);
	if (m_useMask) m_maskPainter.drawPoint(x, y);
    }

    void drawLine(int x1, int y1, int x2, int y2) {
	m_painter->drawLine(x1, y1, x2, y2);
	if (m_useMask) m_maskPainter.drawLine(x1, y1, x2, y2);
    }

    void drawRect(int x, int y, int w, int h) {
	m_painter->drawRect(x, y, w, h);
	if (m_useMask) m_maskPainter.drawRect(x, y, w, h);
    }
    
    void drawArc(int x, int y, int w, int h, int a, int alen) {
	m_painter->drawArc(x, y, w, h, a, alen);
	if (m_useMask) m_maskPainter.drawArc(x, y, w, h, a, alen);
    }

    void drawPolygon(const QPointArray &a, bool winding = false,
		     int index = 0, int n = -1) {
	m_painter->drawPolygon(a, winding, index, n);
	if (m_useMask) m_maskPainter.drawPolygon(a, winding, index, n);
    }

    void drawPolyline(const QPointArray &a, int index = 0, int n = -1) {
	m_painter->drawPolyline(a, index, n);
	if (m_useMask) m_maskPainter.drawPolyline(a, index, n);
    }

    void drawPixmap(int x, int y, const QPixmap &pm,
		    int sx = 0, int sy = 0, int sw = -1, int sh = -1) {
	m_painter->drawPixmap(x, y, pm, sx, sy, sw, sh);
	if (m_useMask) m_maskPainter.drawPixmap(x, y, *(pm.mask()), sx, sy, sw, sh);
    }

    void drawText(int x, int y, const QString &string) {
	m_painter->drawText(x, y, string);
	if (m_useMask) m_maskPainter.drawText(x, y, string);
    }

    void drawNoteCharacter(int x, int y, const NoteCharacter &character) {
	character.draw(m_painter, x, y);
	if (m_useMask) character.drawMask(&m_maskPainter, x, y);
    }

private:
    bool m_useMask;
    QPainter  m_myPainter;
    QPainter  m_maskPainter;
    QPainter *m_externalPainter;
    QPainter *m_painter;
};

    

NotePixmapParameters::NotePixmapParameters(Note::Type noteType,
                                           int dots,
                                           Accidental accidental) :
    m_noteType(noteType),
    m_dots(dots),
    m_accidental(accidental),
    m_cautionary(false),
    m_shifted(false),
    m_dotShifted(false),
    m_accidentalShift(0),
    m_drawFlag(true),
    m_drawStem(true),
    m_stemGoesUp(true),
    m_stemLength(-1),
    m_legerLines(0),
    m_slashes(0),
    m_selected(false),
    m_highlighted(false),
    m_quantized(false),
    m_trigger(false),
    m_onLine(false),
    m_safeVertDistance(0),
    m_beamed(false),
    m_nextBeamCount(0),
    m_thisPartialBeams(false),
    m_nextPartialBeams(false),
    m_width(1),
    m_gradient(0.0),
    m_tupletCount(0),
    m_tuplingLineY(0),
    m_tuplingLineWidth(0),
    m_tuplingLineGradient(0.0),
    m_tied(false),
    m_tieLength(0)
{
    // nothing else
}

NotePixmapParameters::~NotePixmapParameters()
{
    // nothing to see here
}

void
NotePixmapParameters::setMarks(const std::vector<Rosegarden::Mark> &marks)
{
    m_marks.clear();
    for (unsigned int i = 0; i < marks.size(); ++i)
	m_marks.push_back(marks[i]);
}

void
NotePixmapParameters::removeMarks()
{
    m_marks.clear();
}

std::vector<Rosegarden::Mark>
NotePixmapParameters::getNormalMarks() const
{ 
    std::vector<Rosegarden::Mark> marks;

    for (std::vector<Rosegarden::Mark>::const_iterator mi = m_marks.begin();
	 mi != m_marks.end(); ++mi) {

	if (*mi == Rosegarden::Marks::Pause ||
	    *mi == Rosegarden::Marks::UpBow ||
	    *mi == Rosegarden::Marks::DownBow ||
	    *mi == Rosegarden::Marks::Trill ||
	    *mi == Rosegarden::Marks::Turn ||
	    Rosegarden::Marks::isFingeringMark(*mi)) continue;
	
	marks.push_back(*mi);
    }

    return marks;
}

std::vector<Rosegarden::Mark>
NotePixmapParameters::getAboveMarks() const
{ 
    std::vector<Rosegarden::Mark> marks;

    // fingerings before other marks

    for (std::vector<Rosegarden::Mark>::const_iterator mi = m_marks.begin();
	 mi != m_marks.end(); ++mi) {

	if (Rosegarden::Marks::isFingeringMark(*mi)) {
	    marks.push_back(*mi);
	}
    }

    for (std::vector<Rosegarden::Mark>::const_iterator mi = m_marks.begin();
	 mi != m_marks.end(); ++mi) {

	if (*mi == Rosegarden::Marks::Pause ||
	    *mi == Rosegarden::Marks::UpBow ||
	    *mi == Rosegarden::Marks::DownBow ||
	    *mi == Rosegarden::Marks::Trill ||
	    *mi == Rosegarden::Marks::Turn) {
	    marks.push_back(*mi);
	}
    }

    return marks;
}

NotePixmapFactory::NotePixmapFactory(std::string fontName, int size) :
    m_selected(false),
    m_tupletCountFont("times", 8, QFont::Bold),
    m_tupletCountFontMetrics(m_tupletCountFont),
    m_textMarkFont("times", 8, QFont::Bold, true),
    m_textMarkFontMetrics(m_textMarkFont),
    m_fingeringFont("times", 8, QFont::Bold),
    m_fingeringFontMetrics(m_fingeringFont),
    m_timeSigFont("new century schoolbook", 8, QFont::Bold),
    m_timeSigFontMetrics(m_timeSigFont),
    m_bigTimeSigFont("new century schoolbook", 12, QFont::Normal),
    m_bigTimeSigFontMetrics(m_bigTimeSigFont),
    m_ottavaFont("times", 8, QFont::Normal, true),
    m_ottavaFontMetrics(m_ottavaFont),
    m_generatedPixmap(0),
    m_generatedMask(0),
    m_generatedWidth(-1),
    m_generatedHeight(-1),
    m_inPrinterMethod(false),
    m_p(new NotePixmapPainter()),
    m_dottedRestCache(new NotePixmapCache)
{
    init(fontName, size);
}

NotePixmapFactory::NotePixmapFactory(const NotePixmapFactory &npf) :
    m_selected(false),
    m_tupletCountFont(npf.m_tupletCountFont),
    m_tupletCountFontMetrics(m_tupletCountFont),
    m_textMarkFont(npf.m_textMarkFont),
    m_textMarkFontMetrics(m_textMarkFont),
    m_fingeringFont(npf.m_fingeringFont),
    m_fingeringFontMetrics(m_fingeringFont),
    m_timeSigFont(npf.m_timeSigFont),
    m_timeSigFontMetrics(m_timeSigFont),
    m_bigTimeSigFont(npf.m_bigTimeSigFont),
    m_bigTimeSigFontMetrics(m_bigTimeSigFont),
    m_ottavaFont("times", 8, QFont::Normal, true),
    m_ottavaFontMetrics(m_ottavaFont),
    m_generatedPixmap(0),
    m_generatedMask(0),
    m_generatedWidth(-1),
    m_generatedHeight(-1),
    m_inPrinterMethod(false),
    m_p(new NotePixmapPainter()),
    m_dottedRestCache(new NotePixmapCache)
{
    init(npf.m_font->getName(), npf.m_font->getSize());
}

NotePixmapFactory &
NotePixmapFactory::operator=(const NotePixmapFactory &npf)
{
    if (&npf != this) {
	m_selected = npf.m_selected;
	m_timeSigFont = npf.m_timeSigFont;
	m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);
	m_bigTimeSigFont = npf.m_bigTimeSigFont;
	m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);
	m_tupletCountFont = npf.m_tupletCountFont;
	m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);
	m_textMarkFont = npf.m_textMarkFont;
	m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);
	m_fingeringFont = npf.m_fingeringFont;
	m_fingeringFontMetrics = QFontMetrics(m_fingeringFont);
	m_ottavaFont = npf.m_ottavaFont;
	m_ottavaFontMetrics = QFontMetrics(m_ottavaFontMetrics);
	init(npf.m_font->getName(), npf.m_font->getSize());
	m_dottedRestCache->clear();
	m_textFontCache.clear();
    }
    return *this;
}

void
NotePixmapFactory::init(std::string fontName, int size)
{
    try {
	m_style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);
    } catch (NoteStyleFactory::StyleUnavailable u) {
	KStartupLogo::hideIfStillThere();
	KMessageBox::error(0, i18n(strtoqstr(u.getMessage())));
	throw;
    }

    int origSize = size;

    if (fontName != "") {
	try {
	    if (size < 0) size = NoteFontFactory::getDefaultSize(fontName);
	    m_font = NoteFontFactory::getFont(fontName, size);
	} catch (Rosegarden::Exception f) {
	    fontName = "";
	    // fall through 
	}
    }

    if (fontName == "") { // either because it was passed in or because read failed
	try {
	    fontName = NoteFontFactory::getDefaultFontName();
	    size = origSize;
	    if (size < 0) size = NoteFontFactory::getDefaultSize(fontName);
	    m_font = NoteFontFactory::getFont(fontName, size);
	} catch (Rosegarden::Exception f) { // already reported
	    throw;
	}
    }

    // Resize the fonts, because the original constructor used point
    // sizes only and we want pixels

    m_timeSigFont.setPixelSize(size * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    m_bigTimeSigFont.setPixelSize(size * 4 + 2);
    m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);

    m_tupletCountFont.setPixelSize(size * 2);
    m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);

    m_textMarkFont.setPixelSize(size * 2);
    m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);

    m_fingeringFont.setPixelSize(size * 5 / 3);
    m_fingeringFontMetrics = QFontMetrics(m_fingeringFont);

    m_ottavaFont.setPixelSize(size * 2);
    m_ottavaFontMetrics = QFontMetrics(m_ottavaFont);
}

NotePixmapFactory::~NotePixmapFactory()
{
    delete m_p;
    delete m_dottedRestCache;
}

string
NotePixmapFactory::getFontName() const
{
    return m_font->getName();
}

int
NotePixmapFactory::getSize() const
{
    return m_font->getSize();
}

QPixmap
NotePixmapFactory::toQPixmap(QCanvasPixmap* cp)
{
    QPixmap p = *cp;
    delete cp;
    return p;
}


void
NotePixmapFactory::dumpStats(std::ostream &s)
{
#ifdef DUMP_STATS
    s << "NotePixmapFactory: total times since last stats dump:\n"
      << "makeNotePixmap: "
      << (makeNotesTime * 1000 / CLOCKS_PER_SEC) << "ms\n"
      << "drawBeams: "
      << (drawBeamsTime * 1000 / CLOCKS_PER_SEC) << "ms"
      << " (drew " << drawBeamsCount << " individual points in " << drawBeamsBeamCount << " beams)" 
      << endl;
    makeNotesTime = 0;
    drawBeamsTime = 0;
    drawBeamsCount = 0;
    drawBeamsBeamCount = 0;
#endif
    (void)s; // avoid warnings
}


QCanvasPixmap*
NotePixmapFactory::makeNotePixmap(const NotePixmapParameters &params)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeNotePixmap");
    clock_t startTime = clock();

    drawNoteAux(params, 0, 0, 0);

    QPoint hotspot(m_left, m_above + m_noteBodyHeight/2);

//#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    m_p->painter().setPen(Qt::red); m_p->painter().setBrush(Qt::red);

    m_p->drawLine(0,0,0,m_generatedHeight - 1);
    m_p->drawLine(m_generatedWidth - 1, 0, 
                 m_generatedWidth - 1,
                 m_generatedHeight - 1);

    {
	int hsx = hotspot.x();
	int hsy = hotspot.y();
	m_p->drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
	m_p->drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
    }
#endif

    clock_t endTime = clock();
    makeNotesTime += (endTime - startTime);

    return makeCanvasPixmap(hotspot);
}

void
NotePixmapFactory::drawNote(const NotePixmapParameters &params,
			    QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawNote");
    m_inPrinterMethod = true;
    drawNoteAux(params, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawNoteAux(const NotePixmapParameters &params,
			       QPainter *painter, int x, int y)
{
    NoteFont::CharacterType charType = m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    bool drawFlag = params.m_drawFlag;

    if (params.m_beamed) drawFlag = false;

    // A note pixmap is formed of note head, stem, flags,
    // accidentals, dots and beams.  Assume the note head first, then
    // do the rest of the calculations left to right, ie accidentals,
    // stem, flags, dots, beams

    m_noteBodyWidth  = getNoteBodyWidth(params.m_noteType);
    m_noteBodyHeight = getNoteBodyHeight(params.m_noteType);

    // Spacing surrounding the note head.  For top and bottom, we
    // adjust this according to the discrepancy between the nominal
    // and actual heights of the note head pixmap.  For left and
    // right, we use the hotspot x coordinate of the head.
    int temp;
    if (!m_font->getHotspot(m_style->getNoteHeadCharName(params.m_noteType).first,
			    m_borderX, temp)) m_borderX = 0;

    int actualNoteBodyHeight =
	m_font->getHeight(m_style->getNoteHeadCharName(params.m_noteType).first);
    
    m_left = m_right = m_borderX;
    m_above = m_borderY = (actualNoteBodyHeight - m_noteBodyHeight) / 2;
    m_below = (actualNoteBodyHeight - m_noteBodyHeight) - m_above;

//    NOTATION_DEBUG << "actualNoteBodyHeight: " << actualNoteBodyHeight
//		   << ", noteBodyHeight: " << m_noteBodyHeight << ", borderX: "
//		   << m_borderX << ", borderY: "
//		   << m_borderY << endl;

    bool isStemmed = m_style->hasStem(params.m_noteType);
    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    if (!slashCount) slashCount = m_style->getSlashCount(params.m_noteType);

    if (params.m_accidental != NoAccidental) {
        makeRoomForAccidental(params.m_accidental,
			      params.m_cautionary,
			      params.m_accidentalShift,
			      params.m_accidentalExtra);
    }

    NoteCharacter dot(m_font->getCharacter(NoteCharacterNames::DOT, charType));
    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth()/2) dotWidth = getNoteBodyWidth()/2;

    int stemLength = getStemLength(params);

    if (params.m_marks.size() > 0) {
	makeRoomForMarks(isStemmed, params, stemLength);
    }

    if (params.m_legerLines != 0) {
	makeRoomForLegerLines(params);
    }

    if (slashCount > 0) {
	m_left = std::max(m_left, m_noteBodyWidth / 2);
	m_right = std::max(m_right, m_noteBodyWidth / 2);
    }

    if (params.m_tupletCount > 0) {
	makeRoomForTuplingLine(params);
    }

    m_right = std::max(m_right, params.m_dots * dotWidth + dotWidth/2);
    if (params.m_dotShifted) {
	m_right += m_noteBodyWidth;
    }
    if (params.m_onLine) {
        m_above = std::max(m_above, dot.getHeight()/2);
    }

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            m_right += m_noteBodyWidth;
        } else {
            m_left = std::max(m_left, m_noteBodyWidth);
        }
    }

    if (params.m_tied) {
        m_right = std::max(m_right, params.m_tieLength);
        if (params.m_stemGoesUp) {
            m_below = std::max(m_below, m_noteBodyHeight * 2);
        } else {
            m_above = std::max(m_above, m_noteBodyHeight * 2);
        }
    }

    QPoint startPoint, endPoint;
    if (isStemmed && params.m_drawStem) {
	makeRoomForStemAndFlags(drawFlag ? flagCount : 0, stemLength, params,
				startPoint, endPoint);
    }

    if (isStemmed && params.m_drawStem && params.m_beamed) {
	makeRoomForBeams(params);
    }
    
    // for all other calculations we use the nominal note-body height
    // (same as the gap between staff lines), but here we want to know
    // if the pixmap itself is taller than that
/*!!!
    int actualNoteBodyHeight = m_font->getHeight
	(m_style->getNoteHeadCharName(params.m_noteType).first);
//	- 2*m_origin.y();
    if (actualNoteBodyHeight > m_noteBodyHeight) {
	m_below = std::max(m_below, actualNoteBodyHeight - m_noteBodyHeight);
    }
*/
    if (painter) {
	painter->save();
	m_p->beginExternal(painter);
//	NOTATION_DEBUG << "Translate: (" << x << "," << y << ")" << endl;
	painter->translate(x - m_left, y - m_above - m_noteBodyHeight/2);
    } else {
	createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
			    m_noteBodyHeight + m_above + m_below);
    }

    if (params.m_tupletCount > 0) {
	drawTuplingLine(params);
    }

    if (isStemmed && params.m_drawStem && drawFlag) {
	drawFlags(flagCount, params, startPoint, endPoint);
    }

    if (params.m_accidental != NoAccidental) {
        drawAccidental(params.m_accidental, params.m_cautionary);
    }

    NoteCharacter body;
    NoteStyle::CharNameRec charNameRec
	(m_style->getNoteHeadCharName(params.m_noteType));
    CharName charName = charNameRec.first;
    bool inverted = charNameRec.second;

    if (m_selected || params.m_selected) {
	body = m_font->getCharacterColoured
	    (charName,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue,
	     charType, inverted);
    } else if (params.m_highlighted) {
	body = m_font->getCharacterColoured
	    (charName,
	     RosegardenGUIColours::HighlightedElementHue,
	     RosegardenGUIColours::HighlightedElementMinValue,
	     charType, inverted);
    } else if (params.m_quantized) {
	body = m_font->getCharacterColoured
	    (charName,
	     RosegardenGUIColours::QuantizedNoteHue,
	     RosegardenGUIColours::QuantizedNoteMinValue,
	     charType, inverted);
    } else if (params.m_trigger) {
	body = m_font->getCharacterColoured
	    (charName,
	     RosegardenGUIColours::TriggerNoteHue,
	     RosegardenGUIColours::TriggerNoteMinValue,
	     charType, inverted);
    } else {
	body = m_font->getCharacter
	    (charName, charType, inverted);
    }

    QPoint bodyLocation(m_left - m_borderX,
			m_above - m_borderY + getStaffLineThickness()/2);
    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            bodyLocation.rx() += m_noteBodyWidth;
        } else {
            bodyLocation.rx() -= m_noteBodyWidth - 1;
        }
    }
    
    m_p->drawNoteCharacter(bodyLocation.x(), bodyLocation.y(), body);

    if (params.m_dots > 0) {

        int x = m_left + m_noteBodyWidth + dotWidth/2;
        int y = m_above + m_noteBodyHeight/2 - dot.getHeight()/2;

        if (params.m_onLine)  y -= m_noteBodyHeight/2;

        if (params.m_shifted) x += m_noteBodyWidth;
	else if (params.m_dotShifted) x += m_noteBodyWidth;

        for (int i = 0; i < params.m_dots; ++i) {
	    m_p->drawNoteCharacter(x, y, dot);
            x += dotWidth;
        }
    }

    if (isStemmed && params.m_drawStem) {

	drawStem(params, startPoint, endPoint);

	if (flagCount > 0 && !drawFlag && params.m_beamed) {
	    drawBeams(endPoint, params, flagCount);
        }

	if (slashCount > 0) {
	    drawSlashes(startPoint, params, slashCount);
	}
    }

    if (params.m_marks.size() > 0) {
	drawMarks(isStemmed, params, stemLength);
    }

    if (params.m_legerLines != 0) {
	drawLegerLines(params);
    }

    if (params.m_tied) {
        drawTie(!params.m_stemGoesUp, params.m_tieLength,
		dotWidth * params.m_dots);
    }
    
    if (painter) {
	painter->restore();
    }
}

int
NotePixmapFactory::getStemLength(const NotePixmapParameters &params) const
{
    if (params.m_beamed && params.m_stemLength >= 0) {
	return params.m_stemLength;
    }

    int stemLength = getStemLength();

    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    bool stemUp = params.m_stemGoesUp;
    int nbh = m_noteBodyHeight;
    
    if (flagCount > 2) {
	stemLength += getLineSpacing() * (flagCount - 2);
    }

    int width = 0, height = 0;

    if (flagCount > 0) {

	if (!stemUp) stemLength += nbh / 2;

	if (m_font->getDimensions(m_style->getFlagCharName(flagCount),
				  width, height)) {

	    stemLength = std::max(stemLength, height);

	} else if (m_font->getDimensions(m_style->getPartialFlagCharName(true),
					 width, height) ||
		   m_font->getDimensions(m_style->getPartialFlagCharName(false),
					 width, height)) {
	    
	    unsigned int flagSpace = m_noteBodyHeight;
	    (void)m_font->getFlagSpacing(flagSpace);
	    
	    stemLength = std::max(stemLength,
				  height + (flagCount - 1) * (int)flagSpace);
	}
    }

    if (slashCount > 3 && flagCount < 3) {
	stemLength += (slashCount - 3) * (nbh / 2);
    }

    if (params.m_stemLength >= 0) {
	if (flagCount == 0) return params.m_stemLength;
	stemLength = std::max(stemLength, params.m_stemLength);
    }

    return stemLength;
}

void
NotePixmapFactory::makeRoomForAccidental(Accidental a,
					 bool cautionary, int shift, bool extra)
{
    // General observation: where we're only using a character to
    // determine its dimensions, we should (for the moment) just
    // request it in screen mode, because it may be quicker and we
    // don't need to render it, and the dimensions are the same.
    NoteCharacter ac
	(m_font->getCharacter(m_style->getAccidentalCharName(a)));

    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    m_left += ac.getWidth() + (m_noteBodyWidth/4 - m_borderX);

    if (shift > 0) {
	if (extra) {
	    // The extra flag indicates that the first shift is to get
	    // out of the way of a note head, thus has to move
	    // possibly further, or at least a different amount.  So
	    // replace the first shift with a different one.
	    --shift;
	    m_left += m_noteBodyWidth - m_noteBodyWidth/5;
	}
	if (shift > 0) {
	    // The amount we shift for each accidental is the greater
	    // of the probable shift for that accidental and the
	    // probable shift for a sharp, on the assumption (usually
	    // true in classical notation) that the sharp is the
	    // widest accidental and that we may have other
	    // accidentals possibly including sharps on other notes in
	    // this chord that we can't know about here.
	    int step = ac.getWidth() - ah.x();
	    if (a != Rosegarden::Accidentals::Sharp) {
		NoteCharacter acSharp
		    (m_font->getCharacter(m_style->getAccidentalCharName
					  (Rosegarden::Accidentals::Sharp)));
		QPoint ahSharp
		    (m_font->getHotspot(m_style->getAccidentalCharName
					(Rosegarden::Accidentals::Sharp)));
		step = std::max(step, acSharp.getWidth() - ahSharp.x());
	    }
	    m_left += shift * step;
	}
    }

    if (cautionary) m_left += m_noteBodyWidth;

    int above = ah.y() - m_noteBodyHeight/2;
    int below = (ac.getHeight() - ah.y()) -
        (m_noteBodyHeight - m_noteBodyHeight/2); // subtract in case it's odd

    if (above > 0) m_above = std::max(m_above, above);
    if (below > 0) m_below = std::max(m_below, below);
}

void
NotePixmapFactory::drawAccidental(Accidental a, bool cautionary)
{
    NoteCharacter ac
	(m_font->getCharacter
	 (m_style->getAccidentalCharName(a),
	  m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen));

    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    int ax = 0;

    if (cautionary) {
	ax += m_noteBodyWidth / 2;
	int bl = ac.getHeight() * 2 / 3;
	int by = m_above + m_noteBodyHeight/2 - bl/2;
	drawBracket(bl, true,  false, m_noteBodyWidth*3/8, by);
	drawBracket(bl, false, false, ac.getWidth() + m_noteBodyWidth*5/8, by);
    }

    m_p->drawNoteCharacter(ax, m_above + m_noteBodyHeight/2 - ah.y(), ac);
}

void
NotePixmapFactory::makeRoomForMarks(bool isStemmed,
				    const NotePixmapParameters &params,
				    int stemLength)
{
    int height = 0, width = 0;
    int gap = m_noteBodyHeight / 5 + 1;

    std::vector<Rosegarden::Mark> normalMarks = params.getNormalMarks();
    std::vector<Rosegarden::Mark> aboveMarks = params.getAboveMarks();

    for (std::vector<Rosegarden::Mark>::iterator i = normalMarks.begin();
	 i != normalMarks.end(); ++i) {

	if (!Rosegarden::Marks::isTextMark(*i)) {

	    NoteCharacter character(m_font->getCharacter(m_style->getMarkCharName(*i)));
	    height += character.getHeight() + gap;
	    if (character.getWidth() > width) width = character.getWidth();

	} else {
	    // Inefficient to do this here _and_ in drawMarks, but
	    // text marks are not all that common
	    QString text = strtoqstr(Rosegarden::Marks::getTextFromMark(*i));
	    QRect bounds = m_textMarkFontMetrics.boundingRect(text);
	    height += bounds.height() + gap;
	    if (bounds.width() > width) width = bounds.width();
	}
    }

    if (height > 0) {
	if (isStemmed && params.m_stemGoesUp) {
	    m_below += height + 1;
	} else {
	    m_above += height + 1;
	}
    }

    height = 0;

    if (params.m_safeVertDistance > 0 && !aboveMarks.empty()) {
	m_above = std::max(m_above, params.m_safeVertDistance);
    }

    for (std::vector<Rosegarden::Mark>::iterator i = aboveMarks.begin();
	 i != aboveMarks.end(); ++i) {

	if (!Rosegarden::Marks::isFingeringMark(*i)) {

	    NoteCharacter character(m_font->getCharacter(m_style->getMarkCharName(*i)));
	    height += character.getHeight() + gap;
	    if (character.getWidth() > width) width = character.getWidth();

	} else {

	    // Inefficient to do this here _and_ in drawMarks
	    QString text = strtoqstr(Rosegarden::Marks::getFingeringFromMark(*i));
	    QRect bounds = m_fingeringFontMetrics.boundingRect(text);
	    height += bounds.height() + gap + 3;
	    if (bounds.width() > width) width = bounds.width();
	}
    }

    if (height > 0) {
	if (isStemmed && params.m_stemGoesUp && params.m_safeVertDistance == 0) {
	    m_above += stemLength + height + 1;
	} else {
	    m_above += height + 1;
	}
    }

    m_left = std::max(m_left, width/2 - m_noteBodyWidth/2);
    m_right = std::max(m_right, width/2 - m_noteBodyWidth/2);
}

void
NotePixmapFactory::drawMarks(bool isStemmed,
			     const NotePixmapParameters &params,
			     int stemLength)
{
    int gap = m_noteBodyHeight / 5 + 1;
    int dy = gap;

    std::vector<Rosegarden::Mark> normalMarks = params.getNormalMarks();
    std::vector<Rosegarden::Mark> aboveMarks = params.getAboveMarks();

    bool normalMarksAreAbove = !(isStemmed && params.m_stemGoesUp);

    for (std::vector<Rosegarden::Mark>::iterator i = normalMarks.begin();
	 i != normalMarks.end(); ++i) {

	if (!Rosegarden::Marks::isTextMark(*i)) {

	    NoteCharacter character
		(m_font->getCharacter(m_style->getMarkCharName(*i),
		  m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen,
		  !normalMarksAreAbove));

	    int x = m_left + m_noteBodyWidth/2 - character.getWidth()/2;
	    int y = (normalMarksAreAbove ?
		     (m_above - dy - character.getHeight() - 1) :
		     (m_above + m_noteBodyHeight + m_borderY*2 + dy));

	    m_p->drawNoteCharacter(x, y, character);
	    dy += character.getHeight() + gap;

	} else {

	    QString text = strtoqstr(Rosegarden::Marks::getTextFromMark(*i));
	    QRect bounds = m_textMarkFontMetrics.boundingRect(text);
	    
	    m_p->painter().setFont(m_textMarkFont);
	    if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_textMarkFont);

	    int x = m_left + m_noteBodyWidth/2 - bounds.width()/2;
	    int y = (normalMarksAreAbove ?
		     (m_above - dy - 3) :
		     (m_above + m_noteBodyHeight + m_borderY*2 + dy + bounds.height() + 1));

	    m_p->drawText(x, y, text);
	    dy += bounds.height() + gap;
	}
    }

    if (!normalMarksAreAbove) dy = gap;
    if (params.m_safeVertDistance > 0) {
	dy += params.m_safeVertDistance;
    } else if (isStemmed && params.m_stemGoesUp) {
	dy += stemLength;
    }

    for (std::vector<Rosegarden::Mark>::iterator i = aboveMarks.begin();
	 i != aboveMarks.end(); ++i) {

	if (!Rosegarden::Marks::isFingeringMark(*i)) {

	    NoteCharacter character
		(m_font->getCharacter
		 (m_style->getMarkCharName(*i),
		  m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen,
		  false));
	    
	    int x = m_left + m_noteBodyWidth/2 - character.getWidth()/2;
	    int y = m_above - dy - character.getHeight() - 1;
	    
	    m_p->drawNoteCharacter(x, y, character);
	    dy += character.getHeight() + gap;

	} else {
	    QString text = strtoqstr(Rosegarden::Marks::getFingeringFromMark(*i));
	    QRect bounds = m_fingeringFontMetrics.boundingRect(text);
	    
	    m_p->painter().setFont(m_fingeringFont);
	    if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_fingeringFont);

	    int x = m_left + m_noteBodyWidth/2 - bounds.width()/2;
	    int y = m_above - dy - 3;

	    m_p->drawText(x, y, text);
	    dy += bounds.height() + gap;
	}
    }
}

void
NotePixmapFactory::makeRoomForLegerLines(const NotePixmapParameters &params)
{
    if (params.m_legerLines < 0) {
        m_above = std::max(m_above,
                           (m_noteBodyHeight + 1) *
                           (-params.m_legerLines / 2));
    } else if (params.m_legerLines > 0) {
        m_below = std::max(m_below,
                           (m_noteBodyHeight + 1) *
                           (params.m_legerLines / 2));
    }
    if (params.m_legerLines != 0) {
        m_left  = std::max(m_left,  m_noteBodyWidth / 5 + 1);
        m_right = std::max(m_right, m_noteBodyWidth / 5 + 1);
    }
}

void
NotePixmapFactory::drawLegerLines(const NotePixmapParameters &params)
{
    int x0, x1, y;

    if (params.m_legerLines == 0) return;

    x0 = m_left - m_noteBodyWidth / 5 - 1;
    x1 = m_left + m_noteBodyWidth + m_noteBodyWidth / 5 /* + 1 */;

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
	    x0 += m_noteBodyWidth;
	    x1 += m_noteBodyWidth;
	} else {
	    x0 -= m_noteBodyWidth;
	    x1 -= m_noteBodyWidth;
	}
    }

    int offset = m_noteBodyHeight + getStaffLineThickness();
    int legerLines = params.m_legerLines;
    bool below = (legerLines < 0);
    
    if (below) {
	legerLines = -legerLines;
	offset = -offset;
    }

    if (!below) { // note above staff
	if (legerLines % 2) { // note is between lines
	    y = m_above + m_noteBodyHeight;
	} else { // note is on a line
	    y = m_above + m_noteBodyHeight / 2 - getStaffLineThickness()/2;
	} 
    } else { // note below staff
	if (legerLines % 2) { // note is between lines
	    y = m_above - getStaffLineThickness();
	} else { // note is on a line
	    y = m_above + m_noteBodyHeight / 2;
	}
    }

//    NOTATION_DEBUG << "draw leger lines: " << legerLines << " lines, below "
//		   << below
//		   << ", note body height " << m_noteBodyHeight
//		   << ", thickness " << getLegerLineThickness()
//		   << " (staff line " << getStaffLineThickness() << ")"
//		   << ", offset " << offset << endl;

//    bool first = true;
    
    if (getLegerLineThickness() > getStaffLineThickness()) {
	y -= (getLegerLineThickness() - getStaffLineThickness() + 1) /2;
    }

    for (int i = legerLines - 1; i >= 0; --i) { 
	if (i % 2) {
//	    NOTATION_DEBUG << "drawing at y = " << y << endl;
	    for (int j = 0; j < getLegerLineThickness(); ++j) {
		m_p->drawLine(x0, y + j, x1, y + j);
	    }
	    y += offset;
//	    if (first) {
//		x0 += getStemThickness();
//		x1 -= getStemThickness();
//		first = false;
//	    }
	}
    }
}

void
NotePixmapFactory::makeRoomForStemAndFlags(int flagCount, int stemLength,
					   const NotePixmapParameters &params,
					   QPoint &s0, QPoint &s1)
{
    // The coordinates we set in s0 and s1 are relative to (m_above, m_left)

    if (params.m_stemGoesUp) {
	m_above = std::max
	    (m_above, stemLength - m_noteBodyHeight/2);
    } else {
	m_below = std::max
	    (m_below, stemLength - m_noteBodyHeight/2 + 1);
    }

    if (flagCount > 0) {
	if (params.m_stemGoesUp) {
	    int width = 0, height = 0;
	    if (!m_font->getDimensions
		(m_style->getFlagCharName(flagCount), width, height)) {
		width = m_font->getWidth(m_style->getPartialFlagCharName(false));
	    }
	    m_right += width;
	}
    }

    unsigned int stemThickness = getStemThickness();

    NoteStyle::HFixPoint hfix;
    NoteStyle::VFixPoint vfix;
    m_style->getStemFixPoints(params.m_noteType, hfix, vfix);
    
    switch (hfix) {
	
    case NoteStyle::Normal:
    case NoteStyle::Reversed:
	if (params.m_stemGoesUp ^ (hfix == NoteStyle::Reversed)) {
	    s0.setX(m_noteBodyWidth - stemThickness);
	} else {
	    s0.setX(0);
	}
	break;
	
    case NoteStyle::Central:
	if (params.m_stemGoesUp ^ (hfix == NoteStyle::Reversed)) {
	    s0.setX(m_noteBodyWidth/2 + 1);
	} else {
	    s0.setX(m_noteBodyWidth/2);
	}
	break;
    }
    
    switch (vfix) {
	
    case NoteStyle::Near:
    case NoteStyle::Far:
	if (params.m_stemGoesUp ^ (vfix == NoteStyle::Far)) {
	    s0.setY(0);
	} else {
	    s0.setY(m_noteBodyHeight);
	}
	if (vfix == NoteStyle::Near) {
	    stemLength -= m_noteBodyHeight/2;
	} else {
	    stemLength += m_noteBodyHeight/2;
	}		
	break;

    case NoteStyle::Middle:
	if (params.m_stemGoesUp) {
	    s0.setY(m_noteBodyHeight * 3 / 8);
	} else {
	    s0.setY(m_noteBodyHeight * 5 / 8);
	}
	stemLength -= m_noteBodyHeight / 8;
	break;
    }	    

    if (params.m_stemGoesUp) {
	s1.setY(s0.y() - stemLength + getStaffLineThickness());
    } else {
	s1.setY(s0.y() + stemLength);
    }

    s1.setX(s0.x());
}

void
NotePixmapFactory::drawFlags(int flagCount, 
			     const NotePixmapParameters &params,
			     const QPoint &, const QPoint &s1)
{
    if (flagCount < 1) return;

    NoteCharacter flagChar;
    bool found = m_font->getCharacter(m_style->getFlagCharName(flagCount),
				      flagChar,
				      m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen,
				      !params.m_stemGoesUp);
    
    if (!found) {

	// Handle fonts that don't have all the flags in separate characters

	found = m_font->getCharacter(m_style->getPartialFlagCharName(false),
				     flagChar,
				     m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen,
				     !params.m_stemGoesUp);

	if (!found) {
	    std::cerr << "Warning: NotePixmapFactory::drawFlags: No way to draw note with " << flagCount << " flags in this font!?" << std::endl;
	    return;
	}
	
	QPoint hotspot = flagChar.getHotspot();
	
	NoteCharacter oneFlagChar;
	bool foundOne =
	    (flagCount > 1 ?
	     m_font->getCharacter(m_style->getPartialFlagCharName(true),
				  oneFlagChar,
				  m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen,
				  !params.m_stemGoesUp) : false);
	
	unsigned int flagSpace = m_noteBodyHeight;
	(void)m_font->getFlagSpacing(flagSpace);
	
	for (int flag = 0; flag < flagCount; ++flag) {

	    // use flag_1 in preference to flag_0 for the final flag, so
	    // as to end with a flourish
	    if (flag == flagCount - 1 && foundOne) flagChar = oneFlagChar;
	    
	    int y = m_above + s1.y();
	    if (params.m_stemGoesUp) y += flag * flagSpace;
	    else y -= (flag * flagSpace) + flagChar.getHeight();

	    if (!m_inPrinterMethod) {
	    
		m_p->end();

		// Super-slow
	    
		PixmapFunctions::drawPixmapMasked(*m_generatedPixmap,
						  *m_generatedMask,
						  m_left + s1.x() - hotspot.x(),
						  y,
						  *flagChar.getPixmap());
	    
		m_p->begin(m_generatedPixmap, m_generatedMask);

	    } else {

		// No problem with mask here
		m_p->drawNoteCharacter(m_left + s1.x() - hotspot.x(),
				       y,
				       flagChar);
	    }
	}

    } else { // the normal case

	QPoint hotspot = flagChar.getHotspot();
	
	int y = m_above + s1.y();
	if (!params.m_stemGoesUp) y -= flagChar.getHeight();
	
	m_p->drawNoteCharacter(m_left + s1.x() - hotspot.x(), y, flagChar);
    }
}

void
NotePixmapFactory::drawStem(const NotePixmapParameters &,
			    const QPoint &s0, const QPoint &s1)
{
    for (int i = 0; i < getStemThickness(); ++i) {
	m_p->drawLine(m_left + s0.x() + i, m_above + s0.y(),
		      m_left + s1.x() + i, m_above + s1.y());
    }
}

void
NotePixmapFactory::makeRoomForBeams(const NotePixmapParameters &params)
{
    int beamSpacing = (int)(params.m_width * params.m_gradient);
    
    if (params.m_stemGoesUp) {
	
	beamSpacing = -beamSpacing;
	if (beamSpacing < 0) beamSpacing = 0;
	m_above += beamSpacing + 1;
	
	// allow a bit extra in case the h fixpoint is non-normal
	m_right = std::max(m_right, params.m_width + m_noteBodyWidth);
	
    } else {
	
	if (beamSpacing < 0) beamSpacing = 0;
	m_below += beamSpacing + 1;
	
	m_right = std::max(m_right, params.m_width);
    }
}


// Bresenham algorithm, Wu antialiasing

void
NotePixmapFactory::drawShallowLine(int x0, int y0, int x1, int y1,
                                   int thickness, bool smooth)
{
    if (!smooth || m_inPrinterMethod || (y0 == y1)) {

	if (thickness < 4) {
	    for (int i = 0; i < thickness; ++i) {
		m_p->drawLine(x0, y0 + i, x1, y1 + i);
	    }
	} else {
	    Rosegarden::Profiler profiler("NotePixmapFactory::drawShallowLine(polygon)");
	    QPointArray qp(4);
	    qp.setPoint(0, x0, y0);
	    qp.setPoint(1, x0, y0 + thickness);
	    qp.setPoint(2, x1, y1 + thickness);
	    qp.setPoint(3, x1, y1);
	    m_p->drawPolygon(qp);
	}

        return;
    }

    Rosegarden::Profiler profiler("NotePixmapFactory::drawShallowLine(points)");
  
    int dv = y1 - y0;
    int dh = x1 - x0;

    static std::vector<QColor> colours, selectedColours;
    if (colours.size() == 0) {
	int h, s, v;
	RosegardenGUIColours::SelectedElement.hsv(&h, &s, &v);
	for (int step = 0; step < 256; step += (step == 0 ? 63 : 64)) {
	    colours.push_back(QColor(-1, 0, step, QColor::Hsv));
	    selectedColours.push_back(QColor(h, 255-step, v, QColor::Hsv));
	}
    }

    int cx = x0, cy = y0;

    int inc = 1;

    if (dv < 0) {
	dv = -dv; inc = -1;
    }

    int g = 2 * dv - dh;
    int dg1 = 2 * (dv - dh);
    int dg2 = 2 * dv;

    int segment = (dg2 - dg1) / 4;

    while (cx < x1) {

        if (g > 0) {
            g += dg1;
            cy += inc;
        } else {
            g += dg2;
        }

        int quartile = segment ? ((dg2 - g) / segment) : 0;
        if (quartile < 0) quartile = 0;
        if (quartile > 3) quartile = 3;
        if (inc > 0) quartile = 4 - quartile;
/*
        NOTATION_DEBUG
            << "x = " << cx << ", y = " << cy
            << ", g = " << g << ", dg1 = " << dg1 << ", dg2 = " << dg2
            << ", seg = " << segment << ", q = " << quartile << endl;
*/
        // I don't know enough about Qt to be sure of this, but I
        // suspect this may be some of the most inefficient code ever
        // written:

	int off = 0;

	if (m_selected) {
	    m_p->painter().setPen(selectedColours[quartile]);
	} else {
	    m_p->painter().setPen(colours[quartile]);
	}

        m_p->drawPoint(cx, cy);
	drawBeamsCount ++;

	if (thickness > 1) {
	    if (m_selected) {
		m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
	    } else {
		m_p->painter().setPen(Qt::black);
	    }
	}

	while (++off < thickness) {
            m_p->drawPoint(cx, cy + off);
	    drawBeamsCount ++;
        }
        
	if (m_selected) {
	    m_p->painter().setPen(selectedColours[4 - quartile]);
	} else {
	    m_p->painter().setPen(colours[4 - quartile]);
	}

        m_p->drawPoint(cx, cy + off);
	drawBeamsCount ++;
	    
        ++cx;
    }

    m_p->painter().setPen(Qt::black);
}


void
NotePixmapFactory::drawBeams(const QPoint &s1,
                             const NotePixmapParameters &params,
                             int beamCount)
{
    clock_t startTime = clock();

    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw beams for those that appear at the
    // end only

    int startY = m_above + s1.y(), startX = m_left + s1.x();
    int commonBeamCount = std::min(beamCount, params.m_nextBeamCount);

    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);
                
    int width = params.m_width;
    double grad = params.m_gradient;
    bool smooth = m_font->isSmooth();
    int spacing = getLineSpacing();

    int sign = (params.m_stemGoesUp ? 1 : -1);

    if (!params.m_stemGoesUp) startY -= thickness;

    if (!smooth) startY -= sign;
    else if (grad > -0.01 && grad < 0.01) startY -= sign;

    if (m_inPrinterMethod) {
	startX += getStemThickness() / 2;
    }

    for (int j = 0; j < commonBeamCount; ++j) {
        int y = sign * j * spacing;
        drawShallowLine(startX, startY + y, startX + width,
                        startY + (int)(width*grad) + y,
                        thickness, smooth);
	    drawBeamsBeamCount ++;
    }

    int partWidth = width / 3;
    if (partWidth < 2) partWidth = 2;
    else if (partWidth > m_noteBodyWidth) partWidth = m_noteBodyWidth;
    
    if (params.m_thisPartialBeams) {
        for (int j = commonBeamCount; j < beamCount; ++j) {
	    int y = sign * j * spacing;
            drawShallowLine(startX, startY + y, startX + partWidth,
                            startY + (int)(partWidth*grad) + y,
                            thickness, smooth);
	    drawBeamsBeamCount ++;
        }
    }
    
    if (params.m_nextPartialBeams) {
        startX += width - partWidth;
        startY += (int)((width - partWidth) * grad);
        
        for (int j = commonBeamCount; j < params.m_nextBeamCount; ++j) {
	    int y = sign * j * spacing;
            drawShallowLine(startX, startY + y, startX + partWidth,
                            startY + (int)(partWidth*grad) + y,
                            thickness, smooth);
	    drawBeamsBeamCount ++;
        }
    }

    clock_t endTime = clock();
    drawBeamsTime += (endTime - startTime);
}

void
NotePixmapFactory::drawSlashes(const QPoint &s0,
			       const NotePixmapParameters &params,
			       int slashCount)
{
    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);
    thickness = thickness * 3 / 4;
    if (thickness < 1) thickness = 1;

    int gap = thickness - 1;
    if (gap < 1) gap = 1;

    bool smooth = m_font->isSmooth();

    int width = m_noteBodyWidth * 4 / 5;
    int sign = (params.m_stemGoesUp ? -1 : 1);

    int offset = 
	(slashCount == 1 ? m_noteBodyHeight * 2 :
	 slashCount == 2 ? m_noteBodyHeight * 3/2 :
	 m_noteBodyHeight);
    int y = m_above + s0.y() + sign * (offset + thickness/2);

    for (int i = 0; i < slashCount; ++i) {
	int yoff = width / 2;
	drawShallowLine(m_left + s0.x() - width/2, y + yoff/2,
			m_left + s0.x() + width/2 + getStemThickness(), y - yoff/2,
			thickness, smooth);
	y += sign * (thickness + gap);
    }
}


void
NotePixmapFactory::makeRoomForTuplingLine(const NotePixmapParameters &params)
{
    int lineSpacing =
	(int)(params.m_tuplingLineWidth * params.m_tuplingLineGradient);
    int th = m_tupletCountFontMetrics.height();

    if (params.m_tuplingLineY < 0) {

	lineSpacing = -lineSpacing;
	if (lineSpacing < 0) lineSpacing = 0;
	m_above = std::max(m_above, -params.m_tuplingLineY + th/2);
	m_above += lineSpacing + 1;
	
    } else {
	
	if (lineSpacing < 0) lineSpacing = 0;
	m_below = std::max(m_below, params.m_tuplingLineY + th/2);
	m_below += lineSpacing + 1;
    }
    
    m_right = std::max(m_right, params.m_tuplingLineWidth);
}

void
NotePixmapFactory::drawTuplingLine(const NotePixmapParameters &params)
{
    int thickness = getStaffLineThickness() * 3 / 2;
    int countSpace = thickness * 2;

    QString count;
    count.setNum(params.m_tupletCount);
    QRect cr = m_tupletCountFontMetrics.boundingRect(count);

    int tlw = params.m_tuplingLineWidth;
    int indent = m_noteBodyWidth / 2;
    
    if (tlw < (cr.width() + countSpace * 2 + m_noteBodyWidth * 2)) {
	tlw += m_noteBodyWidth - 1;
	indent = 0;
    }

    int w = (tlw - cr.width())/2 - countSpace;

    int startX = m_left + indent;
    int endX = startX + w;

    int startY = params.m_tuplingLineY + m_above + getLineSpacing() / 2;
    int endY = startY + (int)(params.m_tuplingLineGradient * w);

    if (startY == endY) ++thickness;

    int tickOffset = getLineSpacing()/2;
    if (params.m_tuplingLineY >= 0) tickOffset = -tickOffset;

//    NOTATION_DEBUG << "adjusted params.m_tuplingLineWidth = "
//			 << tlw
//			 << ", cr.width = " << cr.width()
//			 << ", tickOffset = " << tickOffset << endl;
//    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
//			 << endX << "," << endY << ")" << endl;

    bool smooth = m_font->isSmooth();

    if (!params.m_tuplingLineFollowsBeam) {
	m_p->drawLine(startX, startY, startX, startY + tickOffset);
	drawShallowLine(startX, startY, endX, endY, thickness, smooth);
    }

    m_p->painter().setFont(m_tupletCountFont);
    if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_tupletCountFont);

    int textX = endX + countSpace;
    int textY = endY + cr.height()/2;
//    NOTATION_DEBUG << "text: (" << textX << "," << textY << ")" << endl;

    m_p->drawText(textX, textY, count);

    startX += tlw - w;
    endX = startX + w;

    startY += (int)(params.m_tuplingLineGradient * (tlw - w));
    endY = startY + (int)(params.m_tuplingLineGradient * w);

//    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
//			 << endX << "," << endY << ")" << endl;

    if (!params.m_tuplingLineFollowsBeam) {
	drawShallowLine(startX, startY, endX, endY, thickness, smooth);
	m_p->drawLine(endX, endY, endX, endY + tickOffset);
    }
}


void
NotePixmapFactory::drawTie(bool above, int length, int shift)
{
#ifdef NASTY_OLD_FLAT_TIE_CODE

    int tieThickness = getStaffLineThickness() * 2;
    int tieCurve = m_font->getSize() * 2 / 3;
    int height = tieCurve + tieThickness;
    int x = m_left + m_noteBodyWidth;
    int y = (above ? m_above - height - tieCurve/2 :
                     m_above + m_noteBodyHeight + tieCurve/2 + 1);
    int i;

    length -= m_noteBodyWidth;
    if (length < tieCurve * 2) length = tieCurve * 2;
    if (length < m_noteBodyWidth * 3) {
	length += m_noteBodyWidth - 2;
	x -= m_noteBodyWidth/2 - 1;
    }

    for (i = 0; i < tieThickness; ++i) {

        if (above) {

            m_p->drawArc
                (x, y + i, tieCurve*2, tieCurve*2, 90*16, 70*16);

            m_p->drawLine
                (x + tieCurve, y + i, x + length - tieCurve - 2, y + i);

            m_p->drawArc
                (x + length - 2*tieCurve - 1, y + i,
                 tieCurve*2, tieCurve*2, 20*16, 70*16);

        } else {

            m_p->drawArc
                (x, y + i - tieCurve, tieCurve*2, tieCurve*2, 200*16, 70*16);

            m_p->drawLine
                (x + tieCurve, y + height - i - 1,
                 x + length - tieCurve - 2, y + height - i - 1);

            m_p->drawArc
                (x + length - 2*tieCurve - 1, y + i - tieCurve,
                 tieCurve*2, tieCurve*2, 270*16, 70*16);
        }
    }
#else

    int origLength = length;

    int x = m_left + m_noteBodyWidth + m_noteBodyWidth / 4 + shift;
    length = origLength - m_noteBodyWidth - m_noteBodyWidth / 3 - shift;

    // if the length is short, move the tie a bit closer to both notes
    if (length < m_noteBodyWidth*2) {
	x = m_left + m_noteBodyWidth + shift;
	length = origLength - m_noteBodyWidth - shift;
    }

    if (length < m_noteBodyWidth) {
	length = m_noteBodyWidth;
    }

    // We can't request a smooth slur here, because that always involves
    // creating a new pixmap
    
    QPoint hotspot;
    drawSlurAux(length, 0, above, false, true, false, hotspot,
		&m_p->painter(),
		x,
		above ? m_above : m_above + m_noteBodyHeight);
//		above ? m_above - m_noteBodyHeight/2 :
//		        m_above + m_noteBodyHeight + m_noteBodyHeight/2);

#endif
}

QCanvasPixmap*
NotePixmapFactory::makeRestPixmap(const NotePixmapParameters &params) 
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeRestPixmap");

    CharName charName(m_style->getRestCharName(params.m_noteType));
    bool encache = false;

    if (params.m_tupletCount == 0 && !m_selected) {
	if (params.m_dots == 0) {
	    return m_font->getCharacter(charName).getCanvasPixmap();
	} else {
	    NotePixmapCache::iterator ci(m_dottedRestCache->find(charName));
	    if (ci != m_dottedRestCache->end())
		return new QCanvasPixmap
		    (*ci->second, QPoint(ci->second->offsetX(),
					 ci->second->offsetY()));
	    else encache = true;
	}
    }

    QPoint hotspot(m_font->getHotspot(charName));
    drawRestAux(params, hotspot, 0, 0, 0);

    QCanvasPixmap* canvasMap = makeCanvasPixmap(hotspot);
    if (encache) {
	m_dottedRestCache->insert(std::pair<CharName, QCanvasPixmap*>
				  (charName, new QCanvasPixmap
				   (*canvasMap, hotspot)));
    }
    return canvasMap;
}

void
NotePixmapFactory::drawRest(const NotePixmapParameters &params,
			    QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawRest");
    m_inPrinterMethod = true;
    QPoint hotspot; // unused
    drawRestAux(params, hotspot, &painter, x, y);
    m_inPrinterMethod = false;
}
    
void
NotePixmapFactory::drawRestAux(const NotePixmapParameters &params,
			       QPoint &hotspot, QPainter *painter, int x, int y)
{
    NoteCharacter character;
    CharName charName(m_style->getRestCharName(params.m_noteType));

    if (m_selected || params.m_selected) {
	character = m_font->getCharacterColoured
	    (charName, 
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else if (params.m_quantized) {
	character = m_font->getCharacterColoured
	    (charName,
	     RosegardenGUIColours::QuantizedNoteHue,
	     RosegardenGUIColours::QuantizedNoteMinValue);
    } else {
        character = m_font->getCharacter(charName);
    }

    NoteCharacter dot = m_font->getCharacter(NoteCharacterNames::DOT);

    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth()/2) dotWidth = getNoteBodyWidth()/2;

    m_above = m_left = 0;
    m_below = dot.getHeight() / 2; // for dotted shallow rests like semibreve
    m_right = dotWidth/2 + dotWidth * params.m_dots;
    m_noteBodyWidth = character.getWidth();
    m_noteBodyHeight = character.getHeight();

    if (params.m_tupletCount) makeRoomForTuplingLine(params);

    // we'll adjust this for tupling line after drawing rest character:
    hotspot = m_font->getHotspot(charName);

    if (painter) {
	painter->save();
	m_p->beginExternal(painter);
	painter->translate(x - m_left, y - m_above - hotspot.y());
    } else {
	createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
			    m_noteBodyHeight + m_above + m_below);
    }

    m_p->drawNoteCharacter(m_left, m_above, character);

    if (params.m_tupletCount) drawTuplingLine(params);

    hotspot.setX(m_left);
    hotspot.setY(m_above + hotspot.y());

    int restY = hotspot.y() - dot.getHeight() - getStaffLineThickness();
    if (params.m_noteType == Note::Semibreve ||
	params.m_noteType == Note::Breve) {
	restY += getLineSpacing();
    }

    for (int i = 0; i < params.m_dots; ++i) {
        int x = m_left + m_noteBodyWidth + i * dotWidth + dotWidth/2;
        m_p->drawNoteCharacter(x, restY, dot); 
    }

    if (painter) {
	painter->restore();
    }
}


QCanvasPixmap*
NotePixmapFactory::makeClefPixmap(const Clef &clef)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeClefPixmap");
    NoteCharacter plain;

    if (m_selected) {
	plain = m_font->getCharacterColoured
	    (m_style->getClefCharName(clef),
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else {
	plain = m_font->getCharacter(m_style->getClefCharName(clef));
    }

    int oct = clef.getOctaveOffset();
    if (oct == 0) return plain.getCanvasPixmap();

    QFont octaveFont("times");
    QFontMetrics octaveFontMetrics(octaveFont);
    QString text = QString("%1").arg(8 * (oct < 0 ? -oct : oct));
    QRect rect = octaveFontMetrics.boundingRect(text);
    
    createPixmapAndMask(plain.getWidth(),
			plain.getHeight() + rect.height());

    if (m_selected) {
	m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
    }
	
    m_p->drawNoteCharacter(0, oct < 0 ? 0 : rect.height(), plain);

    m_p->painter().setFont(octaveFont);
    if (!m_inPrinterMethod) m_p->maskPainter().setFont(octaveFont);

    m_p->drawText(plain.getWidth()/2 - rect.width()/2,
		  oct < 0 ? plain.getHeight() + rect.height() - 1 :
		                                rect.height(), text);

    m_p->painter().setPen(Qt::black);
    QPoint hotspot(plain.getHotspot());
    if (oct > 0) hotspot.setY(hotspot.y() + rect.height());
    return makeCanvasPixmap(hotspot);
}

QCanvasPixmap*
NotePixmapFactory::makeUnknownPixmap()
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeUnknownPixmap");
    if (m_selected) {
        return m_font->getCharacterColoured
	    (NoteCharacterNames::UNKNOWN,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue).getCanvasPixmap();
    } else {
        return m_font->getCharacter(NoteCharacterNames::UNKNOWN).getCanvasPixmap();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeToolbarPixmap(const char *name)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    return new QCanvasPixmap(pixmapDir + "/toolbar/" + name + ".xpm");
}

QCanvasPixmap*
NotePixmapFactory::makeNoteMenuPixmap(Rosegarden::timeT duration,
				      Rosegarden::timeT &errorReturn)
{
    Note nearestNote = Note::getNearestNote(duration);
    bool triplet = false;
    errorReturn = 0;

    if (nearestNote.getDuration() != duration) {
	Note tripletNote = Note::getNearestNote(duration * 3 / 2);
	if (tripletNote.getDuration() == duration * 3 / 2) {
	    nearestNote = tripletNote;
	    triplet = true;
	} else {
	    errorReturn = duration - nearestNote.getDuration();
	}
    }

    QString noteName = NotationStrings::getReferenceName(nearestNote);
    if (triplet) noteName = "3-" + noteName;
    noteName = "menu-" + noteName;
    return makeToolbarPixmap(noteName);
}


QCanvasPixmap*
NotePixmapFactory::makeKeyPixmap(const Key &key,
				 const Clef &clef,
				 Key previousKey)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeKeyPixmap");

    std::vector<int> ah0 = previousKey.getAccidentalHeights(clef);
    std::vector<int> ah1 = key.getAccidentalHeights(clef);

    int cancelCount = 0;
    if (key.isSharp() != previousKey.isSharp()) cancelCount = ah0.size();
    else if (ah1.size() < ah0.size()) cancelCount = ah0.size() - ah1.size();

    CharName keyCharName;
    if (key.isSharp()) keyCharName = NoteCharacterNames::SHARP;
    else keyCharName = NoteCharacterNames::FLAT;

    NoteCharacter keyCharacter;
    NoteCharacter cancelCharacter;

    if (m_selected) {
        keyCharacter = m_font->getCharacterColoured
	    (keyCharName,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
	if (cancelCount > 0) {
	    cancelCharacter = m_font->getCharacterColoured
		(NoteCharacterNames::NATURAL,
		 RosegardenGUIColours::SelectedElementHue,
		 RosegardenGUIColours::SelectedElementMinValue);
	}
    } else {
        keyCharacter = m_font->getCharacter(keyCharName);
	if (cancelCount > 0) {
	    cancelCharacter = m_font->getCharacter(NoteCharacterNames::NATURAL);
	}
    }

    int x = 0;
    int lw = getLineSpacing();
    int keyDelta = keyCharacter.getWidth() - keyCharacter.getHotspot().x();

    int cancelDelta = 0;
    int between = 0;
    if (cancelCount > 0) {
	cancelDelta = cancelCharacter.getWidth() + cancelCharacter.getWidth()/3;
	between = cancelCharacter.getWidth();
    }

    createPixmapAndMask(keyDelta * ah1.size() + cancelDelta * cancelCount + between +
			keyCharacter.getWidth()/4, lw * 8 + 1);

    if (key.isSharp() != previousKey.isSharp()) {

	// cancellation first

	for (unsigned int i = 0; i < cancelCount; ++i) {
	    
	    int h = ah0[ah0.size() - cancelCount + i];
	    int y = (lw * 2) + ((8 - h) * lw) / 2 - cancelCharacter.getHotspot().y();
	    
	    m_p->drawNoteCharacter(x, y, cancelCharacter);
	    
	    x += cancelDelta;
	}

	if (cancelCount > 0) {
	    x += between;
	}
    }

    for (unsigned int i = 0; i < ah1.size(); ++i) {

	int h = ah1[i];
	int y = (lw * 2) + ((8 - h) * lw) / 2 - keyCharacter.getHotspot().y();

	m_p->drawNoteCharacter(x, y, keyCharacter);

	x += keyDelta;
    }

    if (key.isSharp() == previousKey.isSharp()) {

	// cancellation afterwards

	if (cancelCount > 0) {
	    x += between;
	}

	for (unsigned int i = 0; i < cancelCount; ++i) {
	    
	    int h = ah0[ah0.size() - cancelCount + i];
	    int y = (lw * 2) + ((8 - h) * lw) / 2 - cancelCharacter.getHotspot().y();
	    
	    m_p->drawNoteCharacter(x, y, cancelCharacter);
	    
	    x += cancelDelta;
	}
    }

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeClefDisplayPixmap(const Clef &clef)
{
    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);

    int lw = getLineSpacing();
    int width = clefPixmap->width() + 6 * getNoteBodyWidth();

    createPixmapAndMask(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    int x = 3 * getNoteBodyWidth();
    m_p->drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
	m_p->drawLine(x/2, y, m_generatedWidth - x/2 - 1, y);
    }

    delete clefPixmap;

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeKeyDisplayPixmap(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);

    CharName charName = (key.isSharp() ?
                         NoteCharacterNames::SHARP :
                         NoteCharacterNames::FLAT);

    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);
    QPixmap accidentalPixmap(*m_font->getCharacter(charName).getPixmap());
    QPoint hotspot(m_font->getHotspot(charName));

    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - hotspot.x();
    int maxDelta = getAccidentalWidth(Sharp);
    int width = clefPixmap->width() + 5 * maxDelta + 7 * maxDelta;
    int x = clefPixmap->width() + 5 * maxDelta / 2;

    createPixmapAndMask(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    m_p->drawPixmap(2 * maxDelta, y - clefPixmap->offsetY(), *clefPixmap);

    for (unsigned int i = 0; i < ah.size(); ++i) {

	h = ah[i];
	y = (lw * 3) + ((8 - h) * lw) / 2 - hotspot.y();

	m_p->drawPixmap(x, y, accidentalPixmap);

	x += delta;
    }

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
	m_p->drawLine(maxDelta, y, m_generatedWidth - 2*maxDelta - 1, y);
    }

    delete clefPixmap;
    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makePitchDisplayPixmap(int p, const Clef &clef,
					  bool useSharps)
{
    Rosegarden::Pitch pitch(p);
    Rosegarden::Accidental accidental(pitch.getAccidental(useSharps));
    NotePixmapParameters params(Rosegarden::Note::Crotchet, 0, accidental);

    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);

    int lw = getLineSpacing();
    int width = getClefWidth(Rosegarden::Clef::Bass) + 10 * getNoteBodyWidth();

    int h = pitch.getHeightOnStaff
	(clef,
	 useSharps ? Rosegarden::Key("C major") : Rosegarden::Key("A minor"));
    params.setStemGoesUp(h <= 4);

    if (h < -1) params.setStemLength(lw * (4 - h) / 2);
    else if (h > 9) params.setStemLength(lw * (h - 4) / 2);
    if (h > 8) params.setLegerLines(h - 8);
    else if (h < 0) params.setLegerLines(h);

    params.setIsOnLine(h % 2 == 0);
    params.setSelected(m_selected);

    QCanvasPixmap *notePixmap = makeNotePixmap(params);

    int pixmapHeight = lw * 12 + 1;
    int yoffset = lw * 3;
    if (h > 12) {
	pixmapHeight += 6 * lw;
	yoffset += 6 * lw;
    } else if (h < -4) {
	pixmapHeight += 6 * lw;
    }

    createPixmapAndMask(width, pixmapHeight);

    int x =
	getClefWidth(Rosegarden::Clef::Bass) + 5 * getNoteBodyWidth() -
	getAccidentalWidth(accidental);
    int y = yoffset + ((8 - h) * lw) / 2 - notePixmap->offsetY();
    m_p->drawPixmap(x, y, *notePixmap);

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p->drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
	m_p->drawLine(x/2, y, m_generatedWidth - x/2, y);
    }

    delete clefPixmap;
    delete notePixmap;

    return makeCanvasPixmap(m_pointZero);
}
    

QCanvasPixmap*
NotePixmapFactory::makeHairpinPixmap(int length, bool isCrescendo)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeHairpinPixmap");
    drawHairpinAux(length, isCrescendo, 0, 0, 0);
    return makeCanvasPixmap(QPoint(0, m_generatedHeight/2));
}

void
NotePixmapFactory::drawHairpin(int length, bool isCrescendo,
			       QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawHairpin");
    m_inPrinterMethod = true;
    drawHairpinAux(length, isCrescendo, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawHairpinAux(int length, bool isCrescendo,
				  QPainter *painter, int x, int y)
{
    int nbh = getNoteBodyHeight();
    int nbw = getNoteBodyWidth();

    int height = (int)(((double)nbh / (double)(nbw * 40)) * length) + nbh;
    int thickness = getStaffLineThickness() * 3 / 2;

//    NOTATION_DEBUG << "NotePixmapFactory::makeHairpinPixmap: mapped length " << length << " to height " << height << " (nbh = " << nbh << ", nbw = " << nbw << ")" << endl;

    if (height < nbh)   height = nbh;
    if (height > nbh*2) height = nbh*2;

    height += thickness - 1;

    if (painter) {
	painter->save();
	m_p->beginExternal(painter);
	painter->translate(x, y - height/2);
    } else {
	createPixmapAndMask(length, height);
    }

    if (m_selected) {
	m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
    }

    int left = 1, right = length - 2*nbw/3 + 1;

    bool smooth = m_font->isSmooth();

    if (isCrescendo) {
	drawShallowLine(left, height/2-1,
			right, height - thickness - 1, thickness, smooth);
	drawShallowLine(left, height/2-1, right, 0, thickness, smooth);
    } else {
	drawShallowLine(left, 0, right, height/2-1, thickness, smooth);
	drawShallowLine(left, height - thickness - 1,
			right, height/2-1, thickness, smooth);
    }

    m_p->painter().setPen(Qt::black);

    if (painter) {
	painter->restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeSlurPixmap(int length, int dy, bool above, bool phrasing)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeSlurPixmap");

    //!!! could remove "height > 5" requirement if we did a better job of
    // sizing so that any horizontal part was rescaled down to exactly
    // 1 pixel wide instead of blurring
    bool smooth = m_font->isSmooth() && getNoteBodyHeight() > 5;
    QPoint hotspot;
    if (length < getNoteBodyWidth()*2) length = getNoteBodyWidth()*2;
    drawSlurAux(length, dy, above, smooth, false, phrasing, hotspot, 0, 0, 0);

    m_p->end();

    if (smooth) {

	QImage i = m_generatedPixmap->convertToImage();
	if (i.depth() == 1) i = i.convertDepth(32);
	i = i.smoothScale(i.width()/2, i.height()/2);

	delete m_generatedPixmap;
	delete m_generatedMask;
	QPixmap newPixmap(i);
	QCanvasPixmap *p = new QCanvasPixmap(newPixmap, hotspot);
	p->setMask(PixmapFunctions::generateMask(newPixmap,
						 Qt::white.rgb()));
	return p;

    } else {

	QCanvasPixmap *p = new QCanvasPixmap(*m_generatedPixmap, hotspot);
	p->setMask(PixmapFunctions::generateMask(*m_generatedPixmap, 
						 Qt::white.rgb()));
	delete m_generatedPixmap;
	delete m_generatedMask;
	return p;
    }
}

void
NotePixmapFactory::drawSlur(int length, int dy, bool above, bool phrasing,
			    QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawSlur");
    QPoint hotspot;
    m_inPrinterMethod = true;
    if (length < getNoteBodyWidth()*2) length = getNoteBodyWidth()*2;
    drawSlurAux(length, dy, above, false, false, phrasing, hotspot, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawSlurAux(int length, int dy, bool above,
			       bool smooth, bool flat, bool phrasing,
			       QPoint &hotspot, QPainter *painter, int x, int y)
{
    QWMatrix::TransformationMode mode = QWMatrix::transformationMode();
    QWMatrix::setTransformationMode(QWMatrix::Points);

    int thickness = getStaffLineThickness() * 2;
    if (phrasing) thickness = thickness * 3 / 4;
    int nbh = getNoteBodyHeight(), nbw = getNoteBodyWidth();

    // Experiment with rotating the painter rather than the control points.
    double theta = 0;
    bool rotate = false;
    if (dy != 0) {
	// We have opposite (dy) and adjacent (length).
	theta = atan(double(dy) / double(length)) * 180.0 / M_PI;
//	NOTATION_DEBUG << "slur: dy is " << dy << ", length " << length << ", rotating through " << theta << endl;
	rotate = true;
    }
    
    // draw normal slur for very slopey phrasing slur:
    if (theta < -5 || theta > 5) phrasing = false;

    int y0 = 0, my = 0;

    float noteLengths = float(length) / nbw;
    if (noteLengths < 1) noteLengths = 1;

    my = int(0 - nbh * sqrt(noteLengths) / 2);
    if (flat) my = my * 2 / 3;
    else if (phrasing) my = my * 3 / 4;
    if (!above) my = -my;

    bool havePixmap = false;
    QPoint topLeft, bottomRight;

    if (smooth) thickness += 2;

    for (int i = 0; i < thickness; ++i) {

	Spline::PointList pl;
	
	if (!phrasing) {
	    pl.push_back(QPoint(length/6, my));
	    pl.push_back(QPoint(length - length/6, my));
	} else {
	    pl.push_back(QPoint(abs(my)/4, my / 3));
	    pl.push_back(QPoint(length/6, my));

	    if (theta > 1) {
		pl.push_back(QPoint(length * 3 / 8, my * 3 / 2));
	    } else if (theta < -1) {
		pl.push_back(QPoint(length * 5 / 8, my * 3 / 2));
	    } else {
		pl.push_back(QPoint(length / 2, my * 4 / 3));
	    }

	    pl.push_back(QPoint(length - length/6, my));
	    pl.push_back(QPoint(length-abs(my)/4, my / 3));
	}
	
	Spline::PointList *polyPoints = Spline::calculate
	    (QPoint(0, y0), QPoint(length-1, y0), pl, topLeft, bottomRight);

	if (!havePixmap) {
	    int width  = bottomRight.x() - topLeft.x();
	    int height = bottomRight.y() - topLeft.y() + thickness - 1 + abs(dy);
	    hotspot = QPoint(0, -topLeft.y() + (dy < 0 ? -dy : 0));

//	    NOTATION_DEBUG << "slur: bottomRight (" << bottomRight.x() << "," << bottomRight.y() << "), topLeft (" << topLeft.x() << "," << topLeft.y() << "), width " << width << ", height " << height << ", hotspot (" << hotspot.x() << "," << hotspot.y() << "), dy " << dy << ", thickness " << thickness << endl;

	    if (painter) {

		// This conditional is because we're also called with
		// a painter arg from non-printer drawTie.  It's a big
		// hack.

		if (m_inPrinterMethod) {
		    painter->save();
		    m_p->beginExternal(painter);
		    painter->translate(x, y);
		    if (rotate) painter->rotate(theta);
		} else {
		    m_p->painter().save();
		    m_p->maskPainter().save();
		    m_p->painter().translate(x, y);
		    m_p->maskPainter().translate(x, y);
		    if (rotate) {
			m_p->painter().rotate(theta);
			m_p->maskPainter().rotate(theta);
		    }
		}

	    } else {
		createPixmapAndMask(smooth ? width*2+1  : width,
				    smooth ? height*2+thickness*2 : height + thickness,
				    width, height);

		QWMatrix m;
		if (smooth) m.translate(2 * hotspot.x(), 2 * hotspot.y());
		else m.translate(hotspot.x(), hotspot.y());
		m.rotate(theta);
		m_p->painter().setWorldMatrix(m);
		m_p->maskPainter().setWorldMatrix(m);
	    }

	    if (m_selected)
		m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
	    havePixmap = true;
	}
/*
	for (int j = 0; j < pl.size(); ++j) {
	    if (smooth) {
		m_p->drawPoint(pl[j].x()*2, pl[j].y()*2);
	    } else {
		m_p->drawPoint(pl[j].x(), pl[j].y());
	    }
	}
*/
	int ppc = polyPoints->size();
	QPointArray qp(ppc);

	for (int j = 0; j < ppc; ++j) {
	    qp.setPoint(j, (*polyPoints)[j].x(), (*polyPoints)[j].y());
	}

	delete polyPoints;

	if (!smooth || (i > 0 && i < thickness-1)) {
	    if (smooth) {
		for (int j = 0; j < ppc; ++j) {
		    qp.setPoint(j, qp.point(j).x()*2, qp.point(j).y()*2);
		}
		m_p->drawPolyline(qp);
		for (int j = 0; j < ppc; ++j) {
		    qp.setPoint(j, qp.point(j).x(), qp.point(j).y()+1);
		}
		m_p->drawPolyline(qp);
	    } else {
		m_p->drawPolyline(qp);
	    }
	}

	if (above) {
	    ++my;
	    if (i % 2) ++y0;
	} else {
	    --my;
	    if (i % 2) --y0;
	}
    }

    if (m_selected) {
        m_p->painter().setPen(Qt::black);
    }

    QWMatrix::setTransformationMode(mode);

    if (painter) {
	painter->restore();
	if (!m_inPrinterMethod) m_p->maskPainter().restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeOttavaPixmap(int length, int octavesUp)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeOttavaPixmap");
    m_inPrinterMethod = false;
    drawOttavaAux(length, octavesUp, 0, 0, 0);
    return makeCanvasPixmap(QPoint(0, m_generatedHeight-1));
}

void
NotePixmapFactory::drawOttava(int length, int octavesUp,
			      QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawOttava");
    m_inPrinterMethod = true;
    drawOttavaAux(length, octavesUp, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawOttavaAux(int length, int octavesUp,
				 QPainter *painter, int x, int y)
{
    int height = m_ottavaFontMetrics.height();
    int backpedal = 0;
    QString label;
    QRect r;

    if (octavesUp == 2 || octavesUp == -2) {
	label = "15ma  ";
	backpedal = m_ottavaFontMetrics.width("15") / 2;
    } else {
	label = "8va  ";
	backpedal = m_ottavaFontMetrics.width("8") / 2;
    }

    int width = length + backpedal;
    
    if (painter) {
	painter->save();
	m_p->beginExternal(painter);
	painter->translate(x - backpedal, y - height);
    } else {
	NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: making pixmap and mask " << width << "x" << height << endl;
	createPixmapAndMask(width, height);
    }

    int thickness = getStemThickness();
    QPen pen(Qt::black, thickness, Qt::DotLine);

    if (m_selected) {
	m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
	pen.setColor(RosegardenGUIColours::SelectedElement);
    }

    m_p->painter().setFont(m_ottavaFont);
    if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_ottavaFont);

    m_p->drawText(0, m_ottavaFontMetrics.ascent(), label);

    m_p->painter().setPen(pen);
//    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);

    int x0 = m_ottavaFontMetrics.width(label) + thickness;
    int x1 = width - thickness;
    int y0 = m_ottavaFontMetrics.ascent() * 2 / 3 - thickness/2;
    int y1 = (octavesUp < 0 ? 0 : m_ottavaFontMetrics.ascent());
    
    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x0 << "," << y0 << " to " << x1 << "," << y0 << ", thickness " << thickness << endl;

    m_p->drawLine(x0, y0, x1, y0);

    pen.setStyle(Qt::SolidLine);
    m_p->painter().setPen(pen);
//    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);
    
    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x1 << "," << y0 << " to " << x1 << "," << y1 << ", thickness " << thickness << endl;

    m_p->drawLine(x1, y0, x1, y1);

    m_p->painter().setPen(QPen());
    if (!m_inPrinterMethod) m_p->maskPainter().setPen(QPen());
    
    if (painter) {
	painter->restore();
    }
}

void
NotePixmapFactory::drawBracket(int length, bool left, bool curly, int x, int y)
{
    // curly mode not yet implemented

    int thickness = getStemThickness() * 2;
    
    int m1 = length/6;
    int m2 = length - length/6 - 1;

    int off0 = 0, moff = 0;

    int nbh = getNoteBodyHeight(), nbw = getNoteBodyWidth();
    float noteLengths = float(length) / nbw;
    if (noteLengths < 1) noteLengths = 1;
    moff = int(nbh * sqrt(noteLengths) / 2);
    moff = moff * 2 / 3;

    if (left) moff = -moff;

    QPoint topLeft, bottomRight;

    for (int i = 0; i < thickness; ++i) {

	Spline::PointList pl;
	pl.push_back(QPoint((int)moff, m1));
	pl.push_back(QPoint((int)moff, m2));
/*
	NOTATION_DEBUG << "bracket spline controls: " << moff << "," << m1
		       << ", " << moff << "," << m2 << "; end points "
		       << off0 << ",0, " << off0 << "," << length-1
		       << endl;
*/
	Spline::PointList *polyPoints = Spline::calculate
	    (QPoint(off0, 0), QPoint(off0, length-1), pl, topLeft, bottomRight);

	int ppc = polyPoints->size();
	QPointArray qp(ppc);
/*
	NOTATION_DEBUG << "bracket spline polypoints: " << endl;
	for (int j = 0; j < ppc; ++j) {
	    NOTATION_DEBUG << (*polyPoints)[j].x() << "," << (*polyPoints)[j].y() << endl;
	}
*/								

	for (int j = 0; j < ppc; ++j) {
	    qp.setPoint(j, x + (*polyPoints)[j].x(), y + (*polyPoints)[j].y());
	}

	delete polyPoints;

	m_p->drawPolyline(qp);

	if (!left) {
	    ++moff;
	    if (i % 2) ++off0;
	} else {
	    --moff;
	    if (i % 2) --off0;
	}
    }
}

QCanvasPixmap*
NotePixmapFactory::makeTimeSigPixmap(const TimeSignature& sig)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeTimeSigPixmap");

    if (sig.isCommon()) {

	NoteCharacter character;
	if (m_font->getCharacter(NoteCharacterNames::COMMON_TIME, character)) {
	    //!!! selected?
	    createPixmapAndMask(character.getWidth(), character.getHeight());
	    m_p->drawNoteCharacter(0, 0, character);
	    return makeCanvasPixmap(QPoint(0, character.getHeight()/2));
	}

	QString c("c");
	QRect r = m_bigTimeSigFontMetrics.boundingRect(c);

	int dy = getLineSpacing() / 4;
	createPixmapAndMask(r.width(), r.height() + dy*2);

	if (m_selected) {
	    m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
	}
	
	m_p->painter().setFont(m_bigTimeSigFont);
  	if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_bigTimeSigFont);

	m_p->drawText(0, r.height() + dy, c);

	if (sig.getNumerator() == 2) { // cut common

	    int x = r.width()*3/5 - getStemThickness();

	    for (int i = 0; i < getStemThickness() * 2; ++i, ++x) {
		m_p->drawLine(x, 0, x, r.height() + dy*2 - 1);
	    }
	}

	m_p->painter().setPen(Qt::black);
	return makeCanvasPixmap(QPoint(0, r.height()/2 + dy));

    } else {

	int numerator = sig.getNumerator(),
	    denominator = sig.getDenominator();

	QString numS, denomS;

	numS.setNum(numerator);
	denomS.setNum(denominator);

	NoteCharacter character;
	if (m_font->getCharacter(m_style->getTimeSignatureDigitName(0),
				 character)) {

	    // if the 0 digit exists, we assume 1-9 also all exist
	    // and all have the same width

	    int numW = character.getWidth() * numS.length();
	    int denomW = character.getWidth() * denomS.length();

	    int width = std::max(numW, denomW);
	    int height = getLineSpacing() * 4 - getStaffLineThickness();

	    createPixmapAndMask(width, height);

	    //!!! selected

	    for (unsigned int i = 0; i < numS.length(); ++i) {
		int x = width - (width - numW) / 2 - (i + 1) * character.getWidth();
		int y = height/4 - (character.getHeight()/2);
		NoteCharacter charCharacter = m_font->getCharacter
		    (m_style->getTimeSignatureDigitName(numerator % 10));
		m_p->drawNoteCharacter(x, y, charCharacter);
		numerator /= 10;
	    }

	    for (unsigned int i = 0; i < denomS.length(); ++i) {
		int x = width - (width - denomW) / 2 - (i + 1) * character.getWidth();
		int y = height - height/4 - (character.getHeight()/2);
		NoteCharacter charCharacter = m_font->getCharacter
		    (m_style->getTimeSignatureDigitName(denominator % 10));
		m_p->drawNoteCharacter(x, y, charCharacter);
		denominator /= 10;
	    }

	    return makeCanvasPixmap(QPoint(0, height/2));
	}

	QRect numR = m_timeSigFontMetrics.boundingRect(numS);
	QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
	int width = std::max(numR.width(), denomR.width()) + 2;
	int x;

	createPixmapAndMask(width, denomR.height() * 2 + getNoteBodyHeight());
	
	if (m_selected) {
	    m_p->painter().setPen(RosegardenGUIColours::SelectedElement);
	}

	m_p->painter().setFont(m_timeSigFont);
	if (!m_inPrinterMethod) m_p->maskPainter().setFont(m_timeSigFont);

	x = (width - numR.width()) / 2 - 1;
	m_p->drawText(x, denomR.height(), numS);

	x = (width - denomR.width()) / 2 - 1;
	m_p->drawText(x, denomR.height() * 2 + (getNoteBodyHeight()/2) - 1, denomS);
	
	m_p->painter().setPen(Qt::black);
	
	return makeCanvasPixmap(QPoint(0, denomR.height() +
				       (getNoteBodyHeight()/4) - 1));
    }
}

int NotePixmapFactory::getTimeSigWidth(const TimeSignature &sig) const
{
    if (sig.isCommon()) {

	QRect r(m_bigTimeSigFontMetrics.boundingRect("c"));
	return r.width() + 2;

    } else {

	int numerator = sig.getNumerator(),
	    denominator = sig.getDenominator();

	QString numS, denomS;
	
	numS.setNum(numerator);
	denomS.setNum(denominator);
	
	QRect numR = m_timeSigFontMetrics.boundingRect(numS);
	QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
	int width = std::max(numR.width(), denomR.width()) + 2;
	
	return width;
    }
}

QFont
NotePixmapFactory::getTextFont(const Rosegarden::Text &text) const
{
    std::string type(text.getTextType());
    TextFontCache::iterator i = m_textFontCache.find(type.c_str());
    if (i != m_textFontCache.end()) return i->second;

    /*
     * Text types:
     *
     * UnspecifiedType:    Nothing known, use small roman
     * StaffName:          Large roman, to left of start of staff
     * ChordName:          Not normally shown in score, use small roman
     * KeyName:            Not normally shown in score, use small roman
     * Lyric:              Small roman, below staff and dynamic texts
     * Chord:              Small bold roman, above staff
     * Dynamic:	           Small italic, below staff
     * Direction:          Large roman, above staff (by barline?)
     * LocalDirection:     Small bold italic, below staff (by barline?)
     * Tempo:              Large bold roman, above staff
     * LocalTempo:         Small bold roman, above staff
     * Annotation:         Very small sans-serif, in a box
     */
	
    int weight = QFont::Normal;
    bool italic = false;
    bool large = false;
    bool serif = true;
	
    if (type == Rosegarden::Text::Tempo ||
	type == Rosegarden::Text::LocalTempo ||
	type == Rosegarden::Text::LocalDirection ||
	type == Rosegarden::Text::Chord) {
	weight = QFont::Bold;
    }
	
    if (type == Rosegarden::Text::Dynamic ||
	type == Rosegarden::Text::LocalDirection) {
	italic = true;
    }
	
    if (type == Rosegarden::Text::StaffName ||
	type == Rosegarden::Text::Direction ||
	type == Rosegarden::Text::Tempo) {
	large = true;
    }

    QFont textFont("times");

    if (type == Rosegarden::Text::Annotation) {
	serif = false;
	textFont = QFont("lucida");
    }

    int size;
    if (large) size = getLineSpacing() * 7 / 2;
    else if (serif) size = getLineSpacing() * 2;
    else size = getLineSpacing() * 3 / 2;
	
    textFont.setPixelSize(size);
    textFont.setStyleHint(serif ? QFont::Serif : QFont::SansSerif);
    textFont.setWeight(weight);
    textFont.setItalic(italic);

    NOTATION_DEBUG << "NotePixmapFactory::getTextFont: requested size " << size
		   << " for type " << type << ", got " << textFont.pixelSize() << endl;

    m_textFontCache[type.c_str()] = textFont;
    return textFont;
}    

QCanvasPixmap*
NotePixmapFactory::makeTextPixmap(const Rosegarden::Text &text)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeTextPixmap");

    std::string type(text.getTextType());

    if (type == Rosegarden::Text::Annotation) {
	return makeAnnotationPixmap(text);
    }

    drawTextAux(text, 0, 0, 0);
    return makeCanvasPixmap(QPoint(2, 2), true);
}

void
NotePixmapFactory::drawText(const Rosegarden::Text &text,
			    QPainter &painter, int x, int y)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::drawText");

    std::string type(text.getTextType());

    if (type == Rosegarden::Text::Annotation) {
	QCanvasPixmap *map = makeAnnotationPixmap(text);
	painter.drawPixmap(x, y, *map);
	return;
    }

    m_inPrinterMethod = true;
    drawTextAux(text, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawTextAux(const Rosegarden::Text &text,
			       QPainter *painter, int x, int y)
{
    QString s(strtoqstr(text.getText()));
    QFont textFont(getTextFont(text));
    QFontMetrics textMetrics(textFont);
    
    int offset = 2;
    int width = textMetrics.width(s) + 2*offset;
    int height = textMetrics.height() + 2*offset;

    if (painter) {
	painter->save();
	m_p->beginExternal(painter);
	painter->translate(x - offset, y - offset);
    } else {
	createPixmapAndMask(width, height);
    }
    
    if (m_selected) m_p->painter().setPen(RosegardenGUIColours::SelectedElement);

    m_p->painter().setFont(textFont);
    if (!m_inPrinterMethod) m_p->maskPainter().setFont(textFont);

    m_p->drawText(offset, textMetrics.ascent() + offset, s);

    m_p->painter().setPen(Qt::black);

    if (painter) {
	painter->restore();
    }
}
    
QCanvasPixmap*
NotePixmapFactory::makeAnnotationPixmap(const Rosegarden::Text &text)
{
    QString s(strtoqstr(text.getText()));

    QFont textFont(getTextFont(text));
    QFontMetrics textMetrics(textFont);

    int annotationWidth = getLineSpacing() * 16;
    int annotationHeight = getLineSpacing() * 6;

    int    topGap = getLineSpacing() / 4 + 1;
    int bottomGap = getLineSpacing() / 3 + 1;
    int   sideGap = getLineSpacing() / 4 + 1;

    QRect r = textMetrics.boundingRect
	(0, 0, annotationWidth, annotationHeight, Qt::WordBreak, s);
    
    int pixmapWidth = r.width() + sideGap * 2;
    int pixmapHeight = r.height() + topGap + bottomGap;

    createPixmapAndMask(pixmapWidth, pixmapHeight);

    if (m_selected) m_p->painter().setPen(RosegardenGUIColours::SelectedElement);

    m_p->painter().setFont(textFont);
    if (!m_inPrinterMethod) m_p->maskPainter().setFont(textFont);

    m_p->painter().setBrush(RosegardenGUIColours::TextAnnotationBackground);
    m_p->drawRect(0, 0, pixmapWidth, pixmapHeight);

    m_p->painter().setBrush(Qt::black);
    m_p->painter().drawText(QRect(sideGap, topGap,
				  annotationWidth + sideGap,
				  pixmapHeight - bottomGap),
			    Qt::WordBreak, s);

/* unnecessary following the rectangle draw
    m_pm.drawText(QRect(sideGap, topGap,
			annotationWidth + sideGap, annotationHeight + topGap),
		  Qt::WordBreak, s);
*/
    
    return makeCanvasPixmap(QPoint(0, 0));
}

void
NotePixmapFactory::createPixmapAndMask(int width, int height,
				       int maskWidth, int maskHeight)
{
    if (maskWidth  < 0) maskWidth  = width;
    if (maskHeight < 0) maskHeight = height;

    m_generatedWidth = width;
    m_generatedHeight = height;
    m_generatedPixmap = new QPixmap(width, height);
    m_generatedMask = new QBitmap(maskWidth, maskHeight);

    static unsigned long total = 0;
    total += width*height;
//    NOTATION_DEBUG << "createPixmapAndMask: " << width << "x" << height << " (" << (width*height) << " px, " << total << " total)" << endl;

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    m_p->begin(m_generatedPixmap, m_generatedMask);

    m_p->painter().setPen(Qt::black); m_p->painter().setBrush(Qt::black);
    m_p->maskPainter().setPen(Qt::white); m_p->maskPainter().setBrush(Qt::white);
}

QCanvasPixmap*
NotePixmapFactory::makeCanvasPixmap(QPoint hotspot, bool generateMask)
{
    m_p->end();

    QCanvasPixmap* p = new QCanvasPixmap(*m_generatedPixmap, hotspot);

    if (generateMask) {
	p->setMask(PixmapFunctions::generateMask(*p));
    } else {
	p->setMask(*m_generatedMask);
    }

    delete m_generatedPixmap;
    delete m_generatedMask;
    return p;
}

QPoint
NotePixmapFactory::m_pointZero;


int NotePixmapFactory::getNoteBodyWidth(Note::Type type)
    const {
    CharName charName(m_style->getNoteHeadCharName(type).first);
    int hx, hy;
    if (!m_font->getHotspot(charName, hx, hy)) hx = 0;
    return m_font->getWidth(charName) - hx * 2;
}

int NotePixmapFactory::getNoteBodyHeight(Note::Type )
    const {
    // this is by definition
    return m_font->getSize();
}

int NotePixmapFactory::getLineSpacing() const {
    return m_font->getSize() + getStaffLineThickness();
}

int NotePixmapFactory::getAccidentalWidth(const Accidental &a,
					  int shift, bool extraShift) const {
    if (a == Rosegarden::Accidentals::NoAccidental) return 0;
    int w = m_font->getWidth(m_style->getAccidentalCharName(a));
    if (!shift) return w;
    else {
	int sw = w;
	if (extraShift) {
	    --shift;
	    w += getNoteBodyWidth() + getStemThickness();
	}
	w += shift *
	    (sw - m_font->getHotspot(m_style->getAccidentalCharName(a)).x());
    }
    return w;
}

int NotePixmapFactory::getAccidentalHeight(const Accidental &a) const {
    return m_font->getHeight(m_style->getAccidentalCharName(a));
}

int NotePixmapFactory::getStemLength() const {
    unsigned int l = 1;
    (void)m_font->getStemLength(l);
    return l;
}

int NotePixmapFactory::getStemThickness() const {
    unsigned int i = 1;
    (void)m_font->getStemThickness(i);
    return i;
}

int NotePixmapFactory::getStaffLineThickness() const {
    unsigned int i;
    (void)m_font->getStaffLineThickness(i);
    return i;
}

int NotePixmapFactory::getLegerLineThickness() const {
    unsigned int i;
    (void)m_font->getLegerLineThickness(i);
    return i;
}

int NotePixmapFactory::getDotWidth() const {
    return m_font->getWidth(NoteCharacterNames::DOT);
}

int NotePixmapFactory::getClefWidth(const Clef &clef) const {
    return m_font->getWidth(m_style->getClefCharName(clef.getClefType()));
}

int NotePixmapFactory::getBarMargin() const {
    return getNoteBodyWidth() * 2;
}

int NotePixmapFactory::getRestWidth(const Rosegarden::Note &restType) const {
    return m_font->getWidth(m_style->getRestCharName(restType.getNoteType()))
        + (restType.getDots() * getDotWidth());
}

int NotePixmapFactory::getKeyWidth(const Key &key,
				   Key previousKey) const
{
    std::vector<int> ah0 = previousKey.getAccidentalHeights(Clef());
    std::vector<int> ah1 = key.getAccidentalHeights(Clef());

    int cancelCount = 0;
    if (key.isSharp() != previousKey.isSharp()) cancelCount = ah0.size();
    else if (ah1.size() < ah0.size()) cancelCount = ah0.size() - ah1.size();

    CharName keyCharName;
    if (key.isSharp()) keyCharName = NoteCharacterNames::SHARP;
    else keyCharName = NoteCharacterNames::FLAT;

    NoteCharacter keyCharacter;
    NoteCharacter cancelCharacter;

    keyCharacter = m_font->getCharacter(keyCharName);
    if (cancelCount > 0) {
	cancelCharacter = m_font->getCharacter(NoteCharacterNames::NATURAL);
    }

    int x = 0;
    int lw = getLineSpacing();
    int keyDelta = keyCharacter.getWidth() - keyCharacter.getHotspot().x();

    int cancelDelta = 0;
    int between = 0;
    if (cancelCount > 0) {
	cancelDelta = cancelCharacter.getWidth() + cancelCharacter.getWidth()/3;
	between = cancelCharacter.getWidth();
    }

    return (keyDelta * ah1.size() + cancelDelta * cancelCount + between +
	    keyCharacter.getWidth()/4);
}

int NotePixmapFactory::getTextWidth(const Rosegarden::Text &text) const {
    QFontMetrics metrics(getTextFont(text));
    return metrics.boundingRect(strtoqstr(text.getText())).width() + 4;
}

/**









        you shall above all things be glad and young.
        For if you're young, whatever life you wear

        it will become you;and if you are glad
        whatever's living will yourself become.
        Girlboys may nothing more than boygirls need:
        i can entirely her only love

        whose any mystery makes every man's
        flesh put space on;and his mind take off time

        that you should ever think,may god forbid
        and(in his mercy)your true lover spare:
        for that way knowledge lies,the foetal grave
        called progress,and negation's dead undoom.

        I'd rather learn from one bird how to sing
        than teach ten thousand stars how not to dance



        e.e. cummings

















        boundingRect my arse



*/

