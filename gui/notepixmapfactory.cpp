// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <qbitmap.h>
#include <qimage.h>

#include <kmessagebox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>

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


#include <iostream>
using std::cerr;
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

class NotePixmapCache : public __HASH_NS::hash_map<CharName, QCanvasPixmap*,
                                                   CharNameHash, CharNamesEqual>
{
    // nothing to add -- just so we can predeclare it in the header
};

NotePixmapParameters::NotePixmapParameters(Note::Type noteType,
                                           int dots,
                                           Accidental accidental) :
    m_noteType(noteType),
    m_dots(dots),
    m_accidental(accidental),
    m_shifted(false),
    m_drawFlag(true),
    m_drawStem(true),
    m_stemGoesUp(true),
    m_stemLength(-1),
    m_legerLines(0),
    m_slashes(0),
    m_selected(false),
    m_highlighted(false),
    m_quantized(false),
    m_onLine(false),
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



NotePixmapFactory::NotePixmapFactory(std::string fontName, int size) :
    m_selected(false),
    m_tupletCountFont("times", 8, QFont::Bold, true),
    m_tupletCountFontMetrics(m_tupletCountFont),
    m_textMarkFont("times", 8, QFont::Bold, true),
    m_textMarkFontMetrics(m_textMarkFont),
    m_timeSigFont("new century schoolbook", 8, QFont::Bold),
    m_timeSigFontMetrics(m_timeSigFont),
    m_bigTimeSigFont("new century schoolbook", 12, QFont::Normal),
    m_bigTimeSigFontMetrics(m_bigTimeSigFont),
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
    m_timeSigFont(npf.m_timeSigFont),
    m_timeSigFontMetrics(m_timeSigFont),
    m_bigTimeSigFont(npf.m_bigTimeSigFont),
    m_bigTimeSigFontMetrics(m_bigTimeSigFont),
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
	KMessageBox::error(0, i18n(strtoqstr(u.getMessage())));
	throw;
    }

    try {
	if (fontName == "") fontName = NoteFontFactory::getDefaultFontName();
	if (size < 0) size = NoteFontFactory::getDefaultSize(fontName);
        m_font = NoteFontFactory::getFont(fontName, size);
    } catch (Rosegarden::Exception f) {
        KMessageBox::error(0, i18n(strtoqstr(f.getMessage())));
        throw;
    }

    // Resize the fonts, because the original constructor used point
    // sizes only and we want pixels

    // 8 => 20, 4 => 10
    m_timeSigFont.setPixelSize(size * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    // 8 => 34, 4 => 18
    m_bigTimeSigFont.setPixelSize(size * 4 + 2);
    m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);

    // 8 => 12, 4 => 6
    m_tupletCountFont.setPixelSize(size * 3 / 2);
    m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);

    // 8 => 12, 4 => 6
    m_textMarkFont.setPixelSize(size * 3 / 2);
    m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);

    unsigned int x, y;
    m_font->getBorderThickness(x, y);
    m_origin = QPoint(x, y);
}

NotePixmapFactory::~NotePixmapFactory()
{
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
#ifndef NDEBUG
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
}


QCanvasPixmap*
NotePixmapFactory::makeNotePixmap(const NotePixmapParameters &params)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeNotePixmap");
    clock_t startTime = clock();

    bool drawFlag = params.m_drawFlag;
    int stemLength = params.m_stemLength;

    if (params.m_beamed) drawFlag = false;

    // A note pixmap is formed of note head, stem, flags,
    // accidentals, dots and beams.  Assume the note head first, then
    // do the rest of the calculations left to right, ie accidentals,
    // stem, flags, dots, beams

    // spacing surrounding the note head
    m_left = m_right = m_origin.x();
    m_above = m_below = m_origin.y();

    m_noteBodyWidth  = getNoteBodyWidth(params.m_noteType);
    m_noteBodyHeight = getNoteBodyHeight(params.m_noteType);

    bool isStemmed = m_style->hasStem(params.m_noteType);
    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    if (!slashCount) slashCount = m_style->getSlashCount(params.m_noteType);

    if (params.m_accidental != NoAccidental) {
        makeRoomForAccidental(params.m_accidental);
    }

    QPixmap dot(m_font->getPixmap(NoteCharacterNames::DOT));

    if (stemLength < 0) {
	// This should only happen if the note is unbeamed
	stemLength = getStemLength(params);
    }

    if (params.m_marks.size() > 0) {
	makeRoomForMarks(isStemmed, params);
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

    m_right = std::max(m_right, params.m_dots * dot.width() + dot.width()/2);
    if (params.m_onLine) {
        m_above = std::max(m_above, dot.height()/2);
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
    int actualNoteBodyHeight = m_font->getHeight
	(m_style->getNoteHeadCharName(params.m_noteType).first)
	- 2*m_origin.y();
    if (actualNoteBodyHeight > m_noteBodyHeight) {
	m_below = std::max(m_below, actualNoteBodyHeight - m_noteBodyHeight);
    }

    createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                        m_noteBodyHeight + m_above + m_below);

    if (params.m_tupletCount > 0) {
	drawTuplingLine(params);
    }

    if (isStemmed && params.m_drawStem && drawFlag) {
	drawFlags(flagCount, params, startPoint, endPoint);
    }

    if (params.m_accidental != NoAccidental) {
        drawAccidental(params.m_accidental);
    }

    QPixmap body;
    NoteStyle::CharNameRec charNameRec
	(m_style->getNoteHeadCharName(params.m_noteType));
    CharName charName = charNameRec.first;
    bool inverted = charNameRec.second;

    if (m_selected || params.m_selected) {
	body = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue,
	     inverted);
    } else if (params.m_highlighted) {
	body = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::HighlightedElementHue,
	     RosegardenGUIColours::HighlightedElementMinValue,
	     inverted);
    } else if (params.m_quantized) {
	body = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::QuantizedNoteHue,
	     RosegardenGUIColours::QuantizedNoteMinValue,
	     inverted);
    } else {
	body = m_font->getPixmap(charName, inverted);
    }

    QPoint bodyLocation(m_left - m_origin.x(), m_above - m_origin.y());
    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            bodyLocation.rx() += m_noteBodyWidth;
        } else {
            bodyLocation.rx() -= m_noteBodyWidth - 1;
        }
    }
    
    m_p.drawPixmap (bodyLocation, body);
    m_pm.drawPixmap(bodyLocation, *(body.mask()));

    if (params.m_dots > 0) {

        int x = m_left + m_noteBodyWidth + dot.width()/2;
        int y = m_above + m_noteBodyHeight/2 - dot.height()/2;

        if (params.m_shifted) x += m_noteBodyWidth;
        if (params.m_onLine)  y -= m_noteBodyHeight/2;

        for (int i = 0; i < params.m_dots; ++i) {
            m_p.drawPixmap(x, y, dot);
            m_pm.drawPixmap(x, y, *(dot.mask()));
            x += dot.width();
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
	drawMarks(isStemmed, params);
    }

    if (params.m_legerLines != 0) {
	drawLegerLines(params);
    }

    if (params.m_tied) {
        drawTie(!params.m_stemGoesUp, params.m_tieLength);
    }
            
    QPoint hotspot(m_left, m_above + m_noteBodyHeight/2);

//#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);

    m_p.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_p.drawLine(m_generatedPixmap->width() - 1, 0, 
                 m_generatedPixmap->width() - 1,
                 m_generatedPixmap->height() - 1);

    m_pm.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_pm.drawLine(m_generatedPixmap->width() - 1, 0,
                  m_generatedPixmap->width() - 1,
                  m_generatedPixmap->height() - 1);

    {
	int hsx = hotspot.x();
	int hsy = hotspot.y();
	m_p.drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
	m_pm.drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
	m_p.drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
	m_pm.drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
    }
#endif

    clock_t endTime = clock();
    makeNotesTime += (endTime - startTime);

    return makeCanvasPixmap(hotspot);
}

int
NotePixmapFactory::getStemLength(const NotePixmapParameters &params) const
{
    //!!! use flagSpacing? or need a makeRoomForFlags in case
    //they're too long?  (flagSpacing won't work well because we
    //need more room for flags on downward-pointing stems than
    //upward ones)

    unsigned int stemLength = getStemLength();

    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    unsigned int nbh = m_noteBodyHeight;
    
    switch (flagCount) {
    case 1: stemLength += nbh / 3; break;
    case 2: stemLength += nbh * 3 / 4; break;
    case 3: stemLength += nbh + nbh / 4; break;
    case 4: stemLength += nbh * 2 - nbh / 4; break;
    default: break;
    }
    
    int width = 0, height = 0;

    if (m_font->getDimensions(m_style->getFlagCharName(flagCount),
			      width, height)) {

	stemLength = std::min(stemLength, height + nbh);

    } else if (m_font->getDimensions(m_style->getFlagCharName(0),
				     width, height)) {

	unsigned int flagSpace = m_noteBodyHeight;
	(void)m_font->getFlagSpacing(flagSpace);

	stemLength = std::min(stemLength,
			      height + (flagCount - 1) * flagSpace + nbh);
    }

    if (slashCount > 3 && flagCount < 3) {
	stemLength += (slashCount - 3) * (nbh / 2);
    }

    return stemLength;
}

void
NotePixmapFactory::makeRoomForAccidental(Accidental a)
{
    QPixmap ap(m_font->getPixmap(m_style->getAccidentalCharName(a)));
    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    m_left += ap.width() + (m_noteBodyWidth/4 - m_origin.x());

    int above = ah.y() - m_noteBodyHeight/2;
    int below = (ap.height() - ah.y()) -
        (m_noteBodyHeight - m_noteBodyHeight/2); // subtract in case it's odd

    if (above > 0) m_above = std::max(m_above, above);
    if (below > 0) m_below = std::max(m_below, below);
}

void
NotePixmapFactory::drawAccidental(Accidental a)
{
    QPixmap ap(m_font->getPixmap(m_style->getAccidentalCharName(a)));
    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    m_p.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), ap);
    m_pm.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), *(ap.mask()));
}

void
NotePixmapFactory::makeRoomForMarks(bool isStemmed,
				    const NotePixmapParameters &params)
{
    int height = 0, width = 0;
    int gap = m_noteBodyHeight / 5 + 1;

    for (unsigned int i = 0; i < params.m_marks.size(); ++i) {

	if (!Rosegarden::Marks::isTextMark(params.m_marks[i])) {

	    QPixmap pixmap(m_font->getPixmap
			   (m_style->getMarkCharName(params.m_marks[i])));
	    height += pixmap.height() + gap;
	    if (pixmap.width() > width) width = pixmap.width();

	} else {
	    // Inefficient to do this here _and_ in drawMarks, but
	    // text marks are not all that common
	    QString text = strtoqstr(Rosegarden::Marks::getTextFromMark
				     (params.m_marks[i]));
	    QRect bounds = m_textMarkFontMetrics.boundingRect(text);
	    height += bounds.height() + gap;
	    if (bounds.width() > width) width = bounds.width();
	}
    }

    if (isStemmed && params.m_stemGoesUp) {
	m_below += height + 1;
    } else {
	m_above += height + 1;
    }

    m_left = std::max(m_left, width/2 - m_noteBodyWidth/2);
    m_right = std::max(m_right, width/2 - m_noteBodyWidth/2);
}

void
NotePixmapFactory::drawMarks(bool isStemmed,
			     const NotePixmapParameters &params)
{
    int dy = 0;
    int gap = m_noteBodyHeight / 5 + 1;

    for (unsigned int i = 0; i < params.m_marks.size(); ++i) {

	bool markAbove = !(isStemmed && params.m_stemGoesUp);

	if (!Rosegarden::Marks::isTextMark(params.m_marks[i])) {

	    // get pixmap, inverting if it's a pause

	    QPixmap pixmap(m_font->getPixmap
			   (m_style->getMarkCharName(params.m_marks[i]),
			    ((params.m_marks[i] == Rosegarden::Marks::Pause) &&
			     !markAbove)));

	    int x = m_left + m_noteBodyWidth/2 - pixmap.width()/2;
	    int y = (markAbove ? (m_above - dy - pixmap.height() - 1) :
			         (m_above + m_noteBodyHeight + dy));

	    m_p.drawPixmap(x, y, pixmap);
	    m_pm.drawPixmap(x, y,  *(pixmap.mask()));
	    dy += pixmap.height() + gap;

	} else {

	    QString text = strtoqstr(Rosegarden::Marks::getTextFromMark
				     (params.m_marks[i]));
	    QRect bounds = m_textMarkFontMetrics.boundingRect(text);
	    
	    m_p.setFont(m_textMarkFont);
	    m_pm.setFont(m_textMarkFont);

	    int x = m_left + m_noteBodyWidth/2 - bounds.width()/2;
	    int y = (markAbove ? (m_above - dy - 3) :
				 (m_above + m_noteBodyHeight +
				  dy + bounds.height() + 1));

	    m_p.drawText(x, y, text);
	    m_pm.drawText(x, y, text);
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
    x1 = m_left + m_noteBodyWidth + m_noteBodyWidth / 5 + 1;
    y = m_above + m_noteBodyHeight / 2;
    
    int offset = m_noteBodyHeight + getLegerLineThickness();
    int legerLines = params.m_legerLines;
    
    if (legerLines < 0) {
	legerLines = -legerLines;
	offset = -offset;
    }
    
    bool first = true;
    
    for (int i = legerLines - 1; i >= 0; --i) {
	if (i % 2 == 1) {
	    for (int j = 0; j < getLegerLineThickness(); ++j) {
		QPoint p0(x0, y + j);
		QPoint p1(x1, y + j);
		m_p.drawLine(p0, p1);
		m_pm.drawLine(p0, p1);
	    }
	    y += offset;
	    if (first) {
		x0 += getStemThickness();
		x1 -= getStemThickness();
		first = false;
	    }
	} else if (first) {
	    y += offset/2;
	    if (legerLines < 0) {
		--y;
/	    }
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
	    (m_below, stemLength - m_noteBodyHeight/2);
    }

    if (flagCount > 0) {
	if (params.m_stemGoesUp) {
	    int width = 0, height = 0;
	    if (!m_font->getDimensions
		(m_style->getFlagCharName(flagCount), width, height)) {
		width = m_font->getWidth(m_style->getFlagCharName(0));
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
	s0.setY(m_noteBodyHeight/2);
	break;
    }	    

    if (params.m_stemGoesUp) {
	s1.setY(s0.y() - stemLength);
    } else {
	s1.setY(s0.y() + stemLength);
    }

    s1.setX(s0.x());
}

void
NotePixmapFactory::drawFlags(int flagCount, 
			     const NotePixmapParameters &params,
			     const QPoint &s0, const QPoint &s1)
{
    if (flagCount < 1) return;

    QPixmap flagMap;
    bool found = m_font->getPixmap(m_style->getFlagCharName(flagCount),
				   flagMap,
				   !params.m_stemGoesUp);
    
    if (!found) {

	// Handle fonts that don't have all the flags in separate characters
	
	found = m_font->getPixmap(m_style->getFlagCharName(0),
				  flagMap,
				  !params.m_stemGoesUp);
	
	if (!found) {
	    std::cerr << "Warning: NotePixmapFactory::drawFlags: No way to draw note with " << flagCount << " flags in this font!?" << std::endl;
	    return;
	}
	
	unsigned int flagSpace = m_noteBodyHeight;
	(void)m_font->getFlagSpacing(flagSpace);
	
	for (int flag = 0; flag < flagCount; ++flag) {
	    
	    int y = m_above + s1.y();
	    if (params.m_stemGoesUp) y += flag * flagSpace;
	    else y -= (flag * flagSpace) + flagMap.height();
	    
	    m_p.end();
	    m_pm.end();

	    // Super-slow
	    
	    PixmapFunctions::drawPixmapMasked(*m_generatedPixmap,
					      *m_generatedMask,
					      m_left + s1.x() - m_origin.x(),
					      y,
					      flagMap);
	    
	    m_p.begin(m_generatedPixmap);
	    m_pm.begin(m_generatedMask);
	}

    } else { // the normal case
	
	int y = m_above + s1.y();
	if (!params.m_stemGoesUp) y -= flagMap.height();
	
	m_p.drawPixmap(m_left + s1.x() - m_origin.x(), y, flagMap);
	m_pm.drawPixmap(m_left + s1.x() - m_origin.x(), y, *(flagMap.mask()));
    }
}

void
NotePixmapFactory::drawStem(const NotePixmapParameters &params,
			    const QPoint &s0, const QPoint &s1)
{
    for (unsigned int i = 0; i < getStemThickness(); ++i) {
	m_p.drawLine (m_left + s0.x() + i, m_above + s0.y(),
		      m_left + s1.x() + i, m_above + s1.y());
	m_pm.drawLine(m_left + s0.x() + i, m_above + s0.y(),
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
    if (!smooth || (y0 == y1)) {
        for (int i = 0; i < thickness; ++i) {
            m_p.drawLine(x0, y0 + i, x1, y1 + i);
            m_pm.drawLine(x0, y0 + i, x1, y1 + i);
        }
        return;
    }
  
    int dv = y1 - y0;
    int dh = x1 - x0;

    static std::vector<QColor> colours;
    if (colours.size() == 0) {
        colours.push_back(QColor(-1, 0, 0, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 63, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 127, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 191, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 255, QColor::Hsv));
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
        RG_DEBUG
            << "x = " << cx << ", y = " << cy
            << ", g = " << g << ", dg1 = " << dg1 << ", dg2 = " << dg2
            << ", seg = " << segment << ", q = " << quartile << endl;
*/
        // I don't know enough about Qt to be sure of this, but I
        // suspect this may be some of the most inefficient code ever
        // written:

	int off = 0;

        m_p.setPen(colours[quartile]);
        m_p.drawPoint(cx, cy);
        m_pm.drawPoint(cx, cy);
	drawBeamsCount ++;

	if (thickness > 1) m_p.setPen(Qt::black);
	while (++off < thickness) {
            m_p.drawPoint(cx, cy + off);
            m_pm.drawPoint(cx, cy + off);
	    drawBeamsCount ++;
        }
        
        m_p.setPen(colours[4 - quartile]);
        m_p.drawPoint(cx, cy + off);
        m_pm.drawPoint(cx, cy + off);
	drawBeamsCount ++;
	    
        ++cx;
    }

    m_p.setPen(Qt::black);
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

    int gap = thickness - 1;
    if (gap < 1) gap = 1;

    int sign = (params.m_stemGoesUp ? 1 : -1);

    if (!params.m_stemGoesUp) startY -= thickness;

    if (!smooth) startY -= sign;
    else if (grad > -0.01 && grad < 0.01) startY -= sign;

    for (int j = 0; j < commonBeamCount; ++j) {
        int y = sign * j * (thickness + gap);
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
            int y = sign * j * (thickness + gap);
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
            int y = sign * j * (thickness + gap);
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

    int startY = params.m_tuplingLineY + m_above + m_noteBodyHeight / 2;
    int endY = startY + (int)(params.m_tuplingLineGradient * w);

    if (startY == endY) ++thickness;

    int tickOffset = (params.m_tuplingLineY < 0) ? 3 : -3;
/*
    RG_DEBUG << "adjusted params.m_tuplingLineWidth = "
			 << tlw
			 << ", cr.width = " << cr.width()
			 << ", tickOffset = " << tickOffset << endl;
    RG_DEBUG << "line: (" << startX << "," << startY << ") -> ("
			 << endX << "," << endY << ")" << endl;
*/
    bool smooth = m_font->isSmooth();

    m_p.drawLine(startX, startY, startX, startY + tickOffset);
    m_pm.drawLine(startX, startY, startX, startY + tickOffset);

    drawShallowLine(startX, startY, endX, endY, thickness, smooth);

    m_p.setFont(m_tupletCountFont);
    m_pm.setFont(m_tupletCountFont);

    int textX = endX + countSpace;
    int textY = endY + cr.height()/2;
    RG_DEBUG << "text: (" << textX << "," << textY << ")" << endl;

    m_p.drawText(textX, textY, count);
    m_pm.drawText(textX, textY, count);

    startX += tlw - w;
    endX = startX + w;

    startY += (int)(params.m_tuplingLineGradient * (tlw - w));
    endY = startY + (int)(params.m_tuplingLineGradient * w);

    RG_DEBUG << "line: (" << startX << "," << startY << ") -> ("
			 << endX << "," << endY << ")" << endl;


    drawShallowLine(startX, startY, endX, endY, thickness, smooth);

    m_p.drawLine(endX, endY, endX, endY + tickOffset);
    m_pm.drawLine(endX, endY, endX, endY + tickOffset);
}


void
NotePixmapFactory::drawTie(bool above, int length) 
{
    if (length > 1000) {
	assert(0);
    }

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

            m_p.drawArc
                (x, y + i, tieCurve*2, tieCurve*2, 90*16, 70*16);
            m_pm.drawArc
                (x, y + i, tieCurve*2, tieCurve*2, 90*16, 70*16);

            m_p.drawLine
                (x + tieCurve, y + i, x + length - tieCurve - 2, y + i);
            m_pm.drawLine
                (x + tieCurve, y + i, x + length - tieCurve - 2, y + i);

            m_p.drawArc
                (x + length - 2*tieCurve - 1, y + i,
                 tieCurve*2, tieCurve*2, 20*16, 70*16);
            m_pm.drawArc
                (x + length - 2*tieCurve - 1, y + i,
                 tieCurve*2, tieCurve*2, 20*16, 70*16);

        } else {

            m_p.drawArc
                (x, y + i - tieCurve, tieCurve*2, tieCurve*2, 200*16, 70*16);
            m_pm.drawArc
                (x, y + i - tieCurve, tieCurve*2, tieCurve*2, 200*16, 70*16);

            m_p.drawLine
                (x + tieCurve, y + height - i - 1,
                 x + length - tieCurve - 2, y + height - i - 1);
            m_pm.drawLine
                (x + tieCurve, y + height - i - 1,
                 x + length - tieCurve - 2, y + height - i - 1);

            m_p.drawArc
                (x + length - 2*tieCurve - 1, y + i - tieCurve,
                 tieCurve*2, tieCurve*2, 270*16, 70*16);
            m_pm.drawArc
                (x + length - 2*tieCurve - 1, y + i - tieCurve,
                 tieCurve*2, tieCurve*2, 270*16, 70*16);
        }
    }
}

QCanvasPixmap*
NotePixmapFactory::makeRestPixmap(const NotePixmapParameters &params) 
{
    CharName charName(m_style->getRestCharName(params.m_noteType));
    bool encache = false;

    if (params.m_tupletCount == 0 && !m_selected) {
	if (params.m_dots == 0) {
	    return m_font->getCanvasPixmap(charName);
	} else {
	    NotePixmapCache::iterator ci(m_dottedRestCache->find(charName));
	    if (ci != m_dottedRestCache->end())
		return new QCanvasPixmap
		    (*ci->second, QPoint(ci->second->offsetX(),
					 ci->second->offsetY()));
	    else encache = true;
	}
    }

    QPixmap pixmap;

    if (m_selected || params.m_selected) {
        pixmap = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else if (params.m_quantized) {
        pixmap = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::QuantizedNoteHue,
	     RosegardenGUIColours::QuantizedNoteMinValue);
    } else {
        pixmap = m_font->getPixmap(charName);
    }
    QPixmap dot = m_font->getPixmap(NoteCharacterNames::DOT);

    m_above = m_left = 0;
    m_below = dot.height() / 2; // for dotted shallow rests like semibreve
    m_right = dot.width() * params.m_dots;
    m_noteBodyWidth = pixmap.width();
    m_noteBodyHeight = pixmap.height();

    if (params.m_tupletCount) makeRoomForTuplingLine(params);

    createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                        m_noteBodyHeight + m_above + m_below);

    m_p.drawPixmap(m_left, m_above, pixmap);
    m_pm.drawPixmap(m_left, m_above, *(pixmap.mask()));

    if (params.m_tupletCount) drawTuplingLine(params);

    QPoint hotspot(m_font->getHotspot(charName));
    hotspot.setX(m_left);
    hotspot.setY(m_above + hotspot.y());

    int restY = hotspot.y() - dot.height() - getStaffLineThickness();
    if (params.m_noteType == Note::Semibreve ||
	params.m_noteType == Note::Breve) {
	restY += getLineSpacing();
    }

    for (int i = 0; i < params.m_dots; ++i) {
        int x = m_left + m_noteBodyWidth + i * dot.width();
        m_p.drawPixmap(x, restY, dot); 
        m_pm.drawPixmap(x, restY, *(dot.mask()));
    }

    QCanvasPixmap* canvasMap = makeCanvasPixmap(hotspot);
    if (encache) {
	m_dottedRestCache->insert(std::pair<CharName, QCanvasPixmap*>
				  (charName, new QCanvasPixmap
				   (*canvasMap, hotspot)));
    }
    return canvasMap;
}


QCanvasPixmap*
NotePixmapFactory::makeClefPixmap(const Clef &clef)
{
    QCanvasPixmap *plainMap = 0;

    if (m_selected) {
	plainMap = m_font->getColouredCanvasPixmap
	    (m_style->getClefCharName(clef),
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else {
	plainMap = m_font->getCanvasPixmap(m_style->getClefCharName(clef));
    }

    int oct = clef.getOctaveOffset();
    if (oct == 0) return plainMap;

    QFont octaveFont("times");
    QFontMetrics octaveFontMetrics(octaveFont);
    QString text = QString("%1").arg(8 * (oct < 0 ? -oct : oct));
    QRect rect = octaveFontMetrics.boundingRect(text);
    
    createPixmapAndMask(plainMap->width(),
			plainMap->height() + rect.height());

    if (m_selected) {
	m_p.setPen(RosegardenGUIColours::SelectedElement);
    }
	
    m_p.drawPixmap(0, oct < 0 ? 0 : rect.height(), *plainMap);
    m_pm.drawPixmap(0, oct < 0 ? 0 : rect.height(), *(plainMap->mask()));

    m_p.setFont(octaveFont);
    m_pm.setFont(octaveFont);

    m_p.drawText(plainMap->width()/2 - rect.width()/2,
		 oct < 0 ? plainMap->height() + rect.height() - 1 :
		                                rect.height(), text);
    m_pm.drawText(plainMap->width()/2 - rect.width()/2,
		  oct < 0 ? plainMap->height() + rect.height() - 1 :
		                                 rect.height(), text);

    m_p.setPen(Qt::black);
    QPoint hotspot(plainMap->offsetX(), plainMap->offsetY());
    if (oct > 0) hotspot.setY(hotspot.y() + rect.height());
    delete plainMap;
    return makeCanvasPixmap(hotspot);
}

QCanvasPixmap*
NotePixmapFactory::makeUnknownPixmap()
{
    if (m_selected) {
        return m_font->getColouredCanvasPixmap
	    (NoteCharacterNames::UNKNOWN,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else {
        return m_font->getCanvasPixmap(NoteCharacterNames::UNKNOWN);
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
NotePixmapFactory::makeKeyPixmap(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);
    if (ah.size() == 0) return 0;

    CharName charName = (key.isSharp() ?
                         NoteCharacterNames::SHARP :
                         NoteCharacterNames::FLAT);

    QPixmap accidentalPixmap;
    if (m_selected) {
        accidentalPixmap = m_font->getColouredPixmap
	    (charName,
	     RosegardenGUIColours::SelectedElementHue,
	     RosegardenGUIColours::SelectedElementMinValue);
    } else {
        accidentalPixmap = m_font->getPixmap(charName);
    }
    QPoint hotspot(m_font->getHotspot(charName));

    int x = 0;
    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - 2*m_origin.x();

    
    NOTATION_DEBUG << "makeKeyPixmap: delta is " << delta << ", ah.size is " << ah.size() << ", m_origin.x is " << m_origin.x() << ", lw is " << lw << endl;


    createPixmapAndMask(delta * ah.size() + 2*m_origin.x(), lw * 8 + 1);

    for (unsigned int i = 0; i < ah.size(); ++i) {

	int h = ah[i];
	int y = (lw * 2) + ((8 - h) * lw) / 2 - hotspot.y();

        //!!! Here's an interesting problem.  The masked-out area of
        //one accidental's mask may end up overlapping the unmasked
        //area of another accidental's mask, so we lose some of the
        //right-hand edge of each accidental.  What can we do about
        //it?  (Apart from not overlapping the accidentals' x-coords,
        //which wouldn't be a great solution.)

	m_p.drawPixmap(x, y, accidentalPixmap);
	m_pm.drawPixmap(x, y, *(accidentalPixmap.mask()));

	x += delta;
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
    m_p.drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);
    m_pm.drawPixmap(x, y - clefPixmap->offsetY(), *(clefPixmap->mask()));

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
	m_p.drawLine(x/2, y, m_generatedPixmap->width() - x/2 - 1, y);
	m_pm.drawLine(x/2, y, m_generatedPixmap->width() - x/2 - 1, y);
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
    QPixmap accidentalPixmap(m_font->getPixmap(charName));
    QPoint hotspot(m_font->getHotspot(charName));

    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - 2*m_origin.x();
    int width = 11 * getAccidentalWidth(Rosegarden::Accidentals::Sharp) +
	clefPixmap->width();
    int x = clefPixmap->width() + 3 * delta;

    createPixmapAndMask(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    m_p.drawPixmap(2 * delta, y - clefPixmap->offsetY(), *clefPixmap);
    m_pm.drawPixmap(2 * delta, y - clefPixmap->offsetY(), *(clefPixmap->mask()));

    for (unsigned int i = 0; i < ah.size(); ++i) {

	h = ah[i];
	y = (lw * 3) + ((8 - h) * lw) / 2 - hotspot.y();

	m_p.drawPixmap(x, y, accidentalPixmap);
	m_pm.drawPixmap(x, y, *(accidentalPixmap.mask()));

	x += delta;
    }

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
	m_p.drawLine(delta, y, m_generatedPixmap->width() - 2*delta - 1, y);
	m_pm.drawLine(delta, y, m_generatedPixmap->width() - 2*delta - 1, y);
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

    int h = pitch.getHeightOnStaff(clef, Rosegarden::Key());
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
    m_p.drawPixmap(x, y, *notePixmap);
    m_pm.drawPixmap(x, y, *(notePixmap->mask()));

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p.drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);
    m_pm.drawPixmap(x, y - clefPixmap->offsetY(), *(clefPixmap->mask()));

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
	m_p.drawLine(x/2, y, m_generatedPixmap->width() - x/2, y);
	m_pm.drawLine(x/2, y, m_generatedPixmap->width() - x/2, y);
    }

    delete clefPixmap;
    delete notePixmap;

    return makeCanvasPixmap(m_pointZero);
}
    

QCanvasPixmap*
NotePixmapFactory::makeHairpinPixmap(int length, bool isCrescendo)
{
    int nbh = getNoteBodyHeight();
    int nbw = getNoteBodyWidth();

    int height = (int)(((double)nbh / (double)(nbw * 40)) * length) + nbh;
    int thickness = getStaffLineThickness() * 3 / 2;

//    RG_DEBUG << "NotePixmapFactory::makeHairpinPixmap: mapped length " << length << " to height " << height << " (nbh = " << nbh << ", nbw = " << nbw << ")" << endl;

    if (height < nbh)   height = nbh;
    if (height > nbh*2) height = nbh*2;

    height += thickness - 1;

    createPixmapAndMask(length, height);

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

    return makeCanvasPixmap(QPoint(0, height/2));
}

QCanvasPixmap*
NotePixmapFactory::makeSlurPixmap(int length, int dy, bool above)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeSlurPixmap");

    int thickness = getStaffLineThickness() * 2;
    int nbh = getNoteBodyHeight(), nbw = getNoteBodyWidth();

    if (length < nbw * 2) length = nbw * 2;

    Equation::Point a(0, 0);
    Equation::Point b(length, dy);

    int mx1 = length/5;
    int mx2 = length - length/5;

    double my1, my2;
    Equation::solveForYByEndPoints(a, b, mx1, my1);
    Equation::solveForYByEndPoints(a, b, mx2, my2);

    int y1 = 0, y2 = dy;
    
    if (above) {

	if (length < nbw * 10) {
	    my1 -= (nbh * length) / (nbw * 5);
	    my2 -= (nbh * length) / (nbw * 5);
	} else {
	    my1 -= (nbh * 3) / 2;
	    my2 -= (nbh * 3) / 2;
	}

	if      (dy >  nbh * 4) my2 -= nbh;
	else if (dy < -nbh * 4) my1 -= nbh;

	if      (my1 > my2 + nbh) my1 = my2 + nbh;
	else if (my2 > my1 + nbh) my2 = my1 + nbh;
	
	if      (y1 > my1 + nbh*2) y1 = (int)my1 + nbh*2;
	else if (y1 < my1 - nbh/2) my1 = y1 + nbh/2;

	if      (y2 > my2 + nbh*2) y2 = (int)my2 + nbh*2;
	else if (y2 < my2 - nbh/2) my2 = y2 + nbh/2;

    } else {

	if (length < nbw * 10) {
	    my1 += (nbh * length) / (nbw * 5);
	    my2 += (nbh * length) / (nbw * 5);
	} else {
	    my1 += (nbh * 3) / 2;
	    my2 += (nbh * 3) / 2;
	}

	if      (dy >  nbh * 4) my1 += nbh;
	else if (dy < -nbh * 4) my2 += nbh;

	if      (my1 < my2 - nbh) my1 = my2 - nbh;
	else if (my2 < my1 - nbh) my2 = my1 - nbh;
	
	if      (y1 < my1 - nbh*2) y1 = (int)my1 - nbh*2;
	else if (y1 > my1 + nbh/2) my1 = y1 - nbh/2;

	if      (y2 < my2 - nbh*2) y2 = (int)my2 - nbh*2;
	else if (y2 > my2 + nbh/2) my2 = y2 - nbh/2;
    }
    
//    RG_DEBUG << "Pixmap dimensions: " << length << "x" << height << endl;

    bool havePixmap = false;
    QPoint topLeft, bottomRight, hotspot;

    //!!! could remove "nbh > 5" requirement if we did a better job of
    // sizing so that any horizontal part was rescaled down to exactly
    // 1 pixel wide instead of blurring
    bool smooth = m_font->isSmooth() && nbh > 5;

    if (smooth) thickness += 2;

    for (int i = 0; i < thickness; ++i) {

	Spline::PointList pl;
	pl.push_back(QPoint(mx1, (int)my1));
	pl.push_back(QPoint(mx2, (int)my2));

	Spline::PointList *polyPoints = Spline::calculate
	    (QPoint(0, y1), QPoint(length-1, y2), pl, topLeft, bottomRight);

	if (!havePixmap) {
	    int width  = bottomRight.x() - topLeft.x();
	    int height = bottomRight.y() - topLeft.y() + thickness - 1;
	    createPixmapAndMask(smooth ? width*2+1  : width,
				smooth ? height*2+1 : height,
				width, height);
				
	    hotspot = QPoint(-topLeft.x(), -topLeft.y());
	    if (m_selected) m_p.setPen(RosegardenGUIColours::SelectedElement);
	    havePixmap = true;
	}

	int ppc = polyPoints->size();
	QPointArray qp(ppc);

	for (int j = 0; j < ppc; ++j) {
	    qp.setPoint(j,
			hotspot.x() + (*polyPoints)[j].x(),
			hotspot.y() + (*polyPoints)[j].y());
	}

	delete polyPoints;

	m_pm.drawPolyline(qp);

	if (!smooth || (i > 0 && i < thickness-1)) {
	    if (smooth) {
		for (int j = 0; j < ppc; ++j) {
		    qp.setPoint(j, qp.point(j).x()*2, qp.point(j).y()*2);
		}
		m_p.drawPolyline(qp);
		for (int j = 0; j < ppc; ++j) {
		    qp.setPoint(j, qp.point(j).x(), qp.point(j).y()+1);
		}
		m_p.drawPolyline(qp);
	    } else {
		m_p.drawPolyline(qp);
	    }
	}

	if (above) { ++my1; ++my2; }
	else { --my1; --my2; }
    }

    if (smooth) {
	QImage i = m_generatedPixmap->convertToImage();
	if (i.depth() == 1) i = i.convertDepth(32);
	i = i.smoothScale(i.width()/2, i.height()/2);
	m_generatedPixmap->convertFromImage(i);
    }

    m_generatedMask->fill(Qt::color1);

    if (m_selected) {
        m_p.setPen(Qt::black);
    }

    return makeCanvasPixmap(hotspot, true);
}

QCanvasPixmap*
NotePixmapFactory::makeTimeSigPixmap(const TimeSignature& sig)
{
    if (sig.isCommon()) {

	QString c("c");
	QRect r = m_bigTimeSigFontMetrics.boundingRect(c);

	int dy = getLineSpacing() / 4;
	createPixmapAndMask(r.width(), r.height() + dy*2);

	if (m_selected) {
	    m_p.setPen(RosegardenGUIColours::SelectedElement);
	}
	
	m_p.setFont(m_bigTimeSigFont);
	m_pm.setFont(m_bigTimeSigFont);

	m_p.drawText(0, r.height() + dy, c);
	m_pm.drawText(0, r.height() + dy, c);

	if (sig.getNumerator() == 2) { // cut common

	    int x = r.width()*3/5 - getStemThickness();

	    for (int i = 0; i < getStemThickness() * 2; ++i, ++x) {
		m_p.drawLine(x, 0, x, r.height() + dy*2 - 1);
		m_pm.drawLine(x, 0, x, r.height() + dy*2 - 1);
	    }
	}

	m_p.setPen(Qt::black);
	return makeCanvasPixmap(QPoint(0, r.height()/2 + dy));

    } else {

	int numerator = sig.getNumerator(),
	    denominator = sig.getDenominator();

	QString numS, denomS;

	numS.setNum(numerator);
	denomS.setNum(denominator);

	QRect numR = m_timeSigFontMetrics.boundingRect(numS);
	QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
	int width = std::max(numR.width(), denomR.width()) + 2;
	int x;

	createPixmapAndMask(width, denomR.height() * 2 + getNoteBodyHeight());
	
	if (m_selected) {
	    m_p.setPen(RosegardenGUIColours::SelectedElement);
	}

	m_p.setFont(m_timeSigFont);
	m_pm.setFont(m_timeSigFont);

	x = (width - numR.width()) / 2 - 1;
	m_p.drawText(x, denomR.height(), numS);
	m_pm.drawText(x, denomR.height(), numS);

	x = (width - denomR.width()) / 2 - 1;
	m_p.drawText(x, denomR.height() * 2 + (getNoteBodyHeight()/2) - 1, denomS);
	m_pm.drawText(x, denomR.height() * 2 + (getNoteBodyHeight()/2) - 1, denomS);
	
	m_p.setPen(Qt::black);
	
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
	type == Rosegarden::Text::LocalDirection) {
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

    int size = (large ? 16 : (serif ? 12 : 10));
	
    textFont.setStyleHint(serif ? QFont::Serif : QFont::SansSerif);
    textFont.setWeight(weight);
    textFont.setItalic(italic);

    if (large) size = getLineSpacing() * 2;
    else if (serif) size = getLineSpacing() * 5 / 3;
    else size = getLineSpacing() * 6 / 5;

    textFont.setPixelSize(size);

    m_textFontCache[type.c_str()] = textFont;
    return textFont;
}    

QCanvasPixmap*
NotePixmapFactory::makeTextPixmap(const Rosegarden::Text &text)
{
    Rosegarden::Profiler profiler("NotePixmapFactory::makeTextPixmap");

    QString s(strtoqstr(text.getText()));
    std::string type(text.getTextType());

    if (type == Rosegarden::Text::Annotation) {
	return makeAnnotationPixmap(text);
    }

    QFont textFont(getTextFont(text));
    QFontMetrics textMetrics(textFont);
    
    int offset = 2;
    int width = textMetrics.width(s) + 2*offset;
    int height = textMetrics.height() + 2*offset;

    createPixmapAndMask(width, height);
    
    if (m_selected) m_p.setPen(RosegardenGUIColours::SelectedElement);

    m_p.setFont(textFont);
    m_pm.setFont(textFont);

    m_p.drawText(offset, textMetrics.ascent() + offset, s);
    m_pm.drawText(offset, textMetrics.ascent() + offset, s);

    m_p.setPen(Qt::black);
    return makeCanvasPixmap(QPoint(2, 2), true);
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

    if (m_selected) m_p.setPen(RosegardenGUIColours::SelectedElement);

    m_p.setFont(textFont);
    m_pm.setFont(textFont);

    m_p.setBrush(RosegardenGUIColours::TextAnnotationBackground);
    m_p.drawRect(QRect(0, 0, pixmapWidth, pixmapHeight));
    m_pm.drawRect(QRect(0, 0, pixmapWidth, pixmapHeight));

    m_p.setBrush(Qt::black);
    m_p.drawText(QRect(sideGap, topGap,
		       annotationWidth + sideGap, pixmapHeight - bottomGap),
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

    m_generatedPixmap = new QPixmap(width, height);
    m_generatedMask = new QBitmap(maskWidth, maskHeight);

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    
    m_p.begin(m_generatedPixmap);
    m_pm.begin(m_generatedMask);

    m_p.setPen(Qt::black); m_p.setBrush(Qt::black);
    m_pm.setPen(Qt::white); m_pm.setBrush(Qt::white);
}

QCanvasPixmap*
NotePixmapFactory::makeCanvasPixmap(QPoint hotspot, bool generateMask)
{
    m_p.end();
    m_pm.end();

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
    return m_font->getWidth(m_style->getNoteHeadCharName(type).first) -2*m_origin.x();
}

int NotePixmapFactory::getNoteBodyHeight(Note::Type type)
    const {
    // this is by definition
    return m_font->getSize();
}

int NotePixmapFactory::getLineSpacing() const {
    return m_font->getSize() + getStaffLineThickness();
}

int NotePixmapFactory::getAccidentalWidth(const Accidental &a) const {
    if (a == Rosegarden::Accidentals::NoAccidental) return 0;
    return m_font->getWidth(m_style->getAccidentalCharName(a));
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

int NotePixmapFactory::getKeyWidth(const Rosegarden::Key &key) const {
    return 2*m_origin.x() + (key.getAccidentalCount() *
                             (getAccidentalWidth(key.isSharp() ? Sharp : Flat)));
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

