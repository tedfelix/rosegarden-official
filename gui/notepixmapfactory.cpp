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

#include <cstdio>

#include <vector>
#include <set>
#include <string>
#include <algorithm>

#include <kmessagebox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include "rosedebug.h"
#include "rosegardenguiview.h"
#include "notepixmapfactory.h"
#include "NotationTypes.h"
#include "Equation.h"

#include "notefont.h"
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
    m_selected(false),
    m_onLine(false),
    m_beamed(false),
    m_nextBeamCount(0),
    m_thisPartialBeams(false),
    m_nextPartialBeams(false),
    m_width(1),
    m_gradient(0.0),
    m_tupledCount(0),
    m_tuplingLineY(0),
    m_tuplingLineWidth(0),
    m_tuplingLineGradient(0.0)
{
    // nothing else
}

NotePixmapParameters::~NotePixmapParameters()
{
    // nothing to see here
}



NotePixmapFactory::NotePixmapFactory(std::string fontName, int size) :
    m_selected(false),
    m_timeSigFont("new century schoolbook", 8, QFont::Bold),
    m_timeSigFontMetrics(m_timeSigFont),
    m_tupledCountFont("new century schoolbook", 8, QFont::Bold, true),
    m_tupledCountFontMetrics(m_tupledCountFont)
{
    if (fontName == "") fontName = NotePixmapFactory::getDefaultFont();
    if (size < 0) size = NotePixmapFactory::getDefaultSize(fontName);

    try {
        m_font = new NoteFont(fontName, size);
    } catch (NoteFontMap::MappingFileReadFailed f) {
        KMessageBox::error(0, f.reason.c_str());
        throw;
    } catch (NoteFont::BadFont f) {
        KMessageBox::error(0, f.reason.c_str());
        throw;
    }

    // 8 => 20, 4 => 10
    m_timeSigFont.setPixelSize(size * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    // 8 => 12, 4 => 6
    m_tupledCountFont.setPixelSize(size * 3 / 2);
    m_tupledCountFontMetrics = QFontMetrics(m_tupledCountFont);

    unsigned int x, y;
    m_font->getBorderThickness(x, y);
    m_origin = QPoint(x, y);
}

NotePixmapFactory::~NotePixmapFactory()
{
    delete m_font;
}

set<string>
NotePixmapFactory::getAvailableFontNames()
{
    return NoteFont::getAvailableFontNames();
}

string
NotePixmapFactory::getDefaultFont()
{
    set<string> fontNames = getAvailableFontNames();
    if (fontNames.find("feta") != fontNames.end()) return "feta";
    else if (fontNames.size() == 0) throw -1; //!!!
    else return *fontNames.begin();
}

vector<int>
NotePixmapFactory::getAvailableSizes(string fontName)
{
    set<int> s(NoteFont(fontName).getSizes());
    vector<int> v(s.begin(), s.end());
    std::sort(v.begin(), v.end());
    return v;
}

int
NotePixmapFactory::getDefaultSize(string fontName)
{
    vector<int> sizes(getAvailableSizes(fontName));
    return sizes[sizes.size()/2];
}

string
NotePixmapFactory::getFontName() const
{
    return m_font->getNoteFontMap().getName();
}

int
NotePixmapFactory::getSize() const
{
    return m_font->getCurrentSize();
}

QCanvasPixmap
NotePixmapFactory::makeNotePixmap(const NotePixmapParameters &params)
{
    //!!! This function is far too long, and it'd be fairly
    // straightforward to take many of the conditional blocks out
    // into other functions.  Might possibly improve performance too
    // (because the compiler could optimise better).

    Note note(params.m_noteType, params.m_dots);

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

    bool isStemmed = note.hasStem();
    int flagCount = note.getFlagCount();

    if (params.m_accidental != NoAccidental) {
        makeRoomForAccidental(params.m_accidental);
    }

    QPixmap dot(m_font->getPixmap(NoteCharacterNames::DOT));

    if (stemLength < 0) {

        stemLength = getStemLength();
        int nbh = m_noteBodyHeight;

        switch (flagCount) {
        case 1: stemLength += nbh / 3; break;
        case 2: stemLength += nbh * 3 / 4; break;
        case 3: stemLength += nbh + nbh / 4; break;
        case 4: stemLength += nbh * 2 - nbh / 4; break;
        default: break;
        }
    }

    if (params.m_marks.size() > 0) {
	makeRoomForMarks(isStemmed, params);
    }
    
    if (isStemmed && params.m_drawStem) {
        if (params.m_stemGoesUp) {
            m_above = std::max
                (m_above, stemLength - m_noteBodyHeight/2);
        } else {
            m_below = std::max
                (m_below, stemLength - m_noteBodyHeight/2);
        }

        if (params.m_beamed) {

            int beamSpacing = (int)(params.m_width * params.m_gradient);

            if (params.m_stemGoesUp) {

                beamSpacing = -beamSpacing;
                if (beamSpacing < 0) beamSpacing = 0;
                m_above += beamSpacing + 1;

                m_right = std::max(m_right, params.m_width);

            } else {

                if (beamSpacing < 0) beamSpacing = 0;
                m_below += beamSpacing + 1;
                
                m_right = std::max(m_right, params.m_width - m_noteBodyWidth);
            }
        }
    }            

    if (params.m_tupledCount > 0) {

	int lineSpacing =
	    (int)(params.m_tuplingLineWidth * params.m_tuplingLineGradient);
	int th = m_tupledCountFontMetrics.height();

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
        m_left  = std::max(m_left,  getStaffLineThickness() + 1);
        m_right = std::max(m_right, getStaffLineThickness() + 1);
    }

    if (drawFlag && flagCount > 0) {
        if (params.m_stemGoesUp) {
            m_right += m_font->getWidth(getFlagCharName(flagCount));
        }
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
        m_right = std::max(m_right, params.m_tieLength/*!!! - m_noteBodyWidth/2*/);
        if (params.m_stemGoesUp) {
            m_below = std::max(m_below, m_noteBodyHeight * 2);
        } else {
            m_above = std::max(m_above, m_noteBodyHeight * 2);
        }
    }

    createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                        m_noteBodyHeight + m_above + m_below);

    QPoint s0, s1;

    if (isStemmed && params.m_drawStem) {

        s0.setY(m_above + m_noteBodyHeight/2);

        if (params.m_stemGoesUp) {
            s0.setX(m_left + m_noteBodyWidth - 1);
            s1.setY(s0.y() - stemLength);
        } else {
            s0.setX(m_left);
            s1.setY(s0.y() + stemLength);
        }

        s1.setX(s0.x());

        if (flagCount > 0) {

            if (drawFlag) {

                QPixmap flags = m_font->getPixmap
                    (getFlagCharName(flagCount), !params.m_stemGoesUp);

                if (params.m_stemGoesUp) {
                    m_p.drawPixmap(s1.x() - m_origin.x(),
                                   s1.y(), flags);
                    m_pm.drawPixmap(s1.x() - m_origin.x(),
                                    s1.y(), *(flags.mask()));
                } else {
                    m_p.drawPixmap(s1.x() - m_origin.x(),
                                   s1.y() - flags.height(), flags);
                    m_pm.drawPixmap(s1.x() - m_origin.x(),
                                    s1.y() - flags.height(), *(flags.mask()));
                }
            } else if (params.m_beamed) {
                drawBeams(s1, params, flagCount);
            }
        }
    }

    if (params.m_tupledCount > 0) {
	drawTuplingLine(params);
    }

    if (params.m_accidental != NoAccidental) {
        drawAccidental(params.m_accidental);
    }

    QPixmap body;
    if (!m_selected && !params.m_selected) {
        body = m_font->getPixmap(getNoteHeadCharName(params.m_noteType));
    } else {
        body = m_font->getColouredPixmap
            (getNoteHeadCharName(params.m_noteType), NoteFont::Blue);
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
        unsigned int thickness;
        m_font->getStemThickness(thickness);
        for (unsigned int i = 0; i < thickness; ++i) {
            m_p.drawLine(s0, s1);
            m_pm.drawLine(s0, s1);
            ++s0.rx();
            ++s1.rx();
        }
    }        

    if (params.m_marks.size() > 0) {
	drawMarks(isStemmed, params);
    }

    if (params.m_legerLines != 0) {

        s0.setX(m_left - getStemThickness());
        s1.setX(m_left + m_noteBodyWidth + getStemThickness());

        s0.setY(m_above + m_noteBodyHeight / 2);
        s1.setY(s0.y());

        int offset = m_noteBodyHeight + 1;
        int legerLines = params.m_legerLines;

        if (legerLines < 0) {
            legerLines = -legerLines;
            offset = -offset;
        }

        bool first = true;

        for (int i = legerLines - 1; i >= 0; --i) {
            if (i % 2 == 1) {
                m_p.drawLine(s0, s1);
                m_pm.drawLine(s0, s1);
                s0.ry() += offset;
                s1.ry() += offset;
                if (first) {
                    ++s0.rx();
                    --s1.rx();
                    first = false;
                }
            } else if (first) {
                s0.ry() += offset/2;
                s1.ry() += offset/2;
                if (legerLines < 0) {
                    --s0.ry();
                    --s1.ry();
                }
            }                
        }
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

    return makeCanvasPixmap(hotspot);
}

void
NotePixmapFactory::makeRoomForAccidental(Accidental a)
{
    QPixmap ap(m_font->getPixmap(getAccidentalCharName(a)));
    QPoint ah(m_font->getHotspot(getAccidentalCharName(a)));

    m_left += ap.width();

    int above = ah.y() - m_noteBodyHeight/2;
    int below = (ap.height() - ah.y()) -
        (m_noteBodyHeight - m_noteBodyHeight/2); // subtract in case it's odd

    if (above > 0) m_above = std::max(m_above, above);
    if (below > 0) m_below = std::max(m_below, below);
}

void
NotePixmapFactory::drawAccidental(Accidental a)
{
    QPixmap ap(m_font->getPixmap(getAccidentalCharName(a)));
    QPoint ah(m_font->getHotspot(getAccidentalCharName(a)));

    m_p.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), ap);
    m_pm.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), *(ap.mask()));
}

void
NotePixmapFactory::makeRoomForMarks(bool isStemmed,
				    const NotePixmapParameters &params)
{
    int height = 0, width = 0;
    for (int i = 0; i < params.m_marks.size(); ++i) {
	//!!! Deal with textual marks (sf, rf) -- or make them not marks
	// at all?
	QPixmap pixmap(m_font->getPixmap(getMarkCharName(params.m_marks[i])));
	height += pixmap.height();
	if (pixmap.width() > width) width = pixmap.width();
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
    bool marksAbove = !(isStemmed && params.m_stemGoesUp);
    int dy = 0;

    for (int i = 0; i < params.m_marks.size(); ++i) {
	//!!! Deal with textual marks (sf, rf) -- or make them not marks
	// at all?
	QPixmap pixmap(m_font->getPixmap(getMarkCharName(params.m_marks[i])));
	m_p.drawPixmap(m_left + m_noteBodyWidth/2 - pixmap.width()/2,
		       (marksAbove ? (m_above - dy - pixmap.height() - 1) :
			(m_above + m_noteBodyWidth + dy)), pixmap);
	m_pm.drawPixmap(m_left + m_noteBodyWidth/2 - pixmap.width()/2,
			(marksAbove ? (m_above - dy - pixmap.height() - 1) :
			 (m_above + m_noteBodyWidth + dy)), *(pixmap.mask()));
	dy += pixmap.height();
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

        int quartile = (dg2 - g) / segment;
        if (quartile < 0) quartile = 0;
        if (quartile > 3) quartile = 3;
        if (inc > 0) quartile = 4 - quartile;
/*
        kdDebug(KDEBUG_AREA)
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

	if (thickness > 1) m_p.setPen(Qt::black);
	while (++off < thickness) {
            m_p.drawPoint(cx, cy + off);
            m_pm.drawPoint(cx, cy + off);
        }
        
        m_p.setPen(colours[4 - quartile]);
        m_p.drawPoint(cx, cy + off);
        m_pm.drawPoint(cx, cy + off);

        ++cx;
    }

    m_p.setPen(Qt::black);
}


void
NotePixmapFactory::drawBeams(const QPoint &s1,
                             const NotePixmapParameters &params,
                             int beamCount)
{
    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw beams for those that appear at the
    // end only

    int startY = s1.y(), startX = s1.x();
    int commonBeamCount = std::min(beamCount, params.m_nextBeamCount);

    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);
                
    int width = params.m_width;
    double grad = params.m_gradient;

    bool smooth = m_font->getNoteFontMap().isSmooth();

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
        }
    }
}

void
NotePixmapFactory::drawTuplingLine(const NotePixmapParameters &params)
{
    int thickness = getStaffLineThickness() * 3 / 2;
    int countSpace = thickness * 2;

    QString count;
    count.setNum(params.m_tupledCount);
    QRect cr = m_tupledCountFontMetrics.boundingRect(count);

    int w = (params.m_tuplingLineWidth - cr.width())/2 - countSpace;

    int startX = m_left + m_noteBodyWidth / 2;
    int endX = startX + w;

    int startY = params.m_tuplingLineY + m_above + m_noteBodyHeight / 2;
    int endY = startY + (int)(params.m_tuplingLineGradient * w);

    if (startY == endY) ++thickness;

    int tickOffset = (params.m_tuplingLineY < 0) ? 3 : -3;

    kdDebug(KDEBUG_AREA) << "params.m_tuplingLineWidth = "
			 << params.m_tuplingLineWidth
			 << ", cr.width = " << cr.width()
			 << ", tickOffset = " << tickOffset << endl;
    kdDebug(KDEBUG_AREA) << "line: (" << startX << "," << startY << ") -> ("
			 << endX << "," << endY << ")" << endl;

    m_p.drawLine(startX, startY, startX, startY + tickOffset);
    m_pm.drawLine(startX, startY, startX, startY + tickOffset);

    drawShallowLine(startX, startY, endX, endY, thickness, true);

    m_p.setFont(m_tupledCountFont);
    m_pm.setFont(m_tupledCountFont);

    int textX = endX + countSpace;
    int textY = endY + cr.height()/2;
    kdDebug(KDEBUG_AREA) << "text: (" << textX << "," << textY << ")" << endl;

    m_p.drawText(textX, textY, count);
    m_pm.drawText(textX, textY, count);

    startX += params.m_tuplingLineWidth - w;
    endX = startX + w;

    startY += (int)(params.m_tuplingLineGradient *
		    (params.m_tuplingLineWidth - w));
    endY = startY + (int)(params.m_tuplingLineGradient * w);

    kdDebug(KDEBUG_AREA) << "line: (" << startX << "," << startY << ") -> ("
			 << endX << "," << endY << ")" << endl;


    drawShallowLine(startX, startY, endX, endY, thickness, true);

    m_p.drawLine(endX, endY, endX, endY + tickOffset);
    m_pm.drawLine(endX, endY, endX, endY + tickOffset);
}


void
NotePixmapFactory::drawTie(bool above, int length) 
{
    int tieThickness = getStaffLineThickness() * 2;
    int tieCurve = m_font->getCurrentSize() * 2 / 3;
    int height = tieCurve + tieThickness;
    int x = m_left + m_noteBodyWidth /*!!! / 2*/;
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

QCanvasPixmap
NotePixmapFactory::makeRestPixmap(const Note &restType) 
{
    CharName charName(getRestCharName(restType.getNoteType()));
    if (restType.getDots() == 0 && !m_selected) {
        return m_font->getCanvasPixmap(charName);
    }

    QPixmap pixmap;

    if (!m_selected) {
        pixmap = m_font->getPixmap(charName);
    } else {
        pixmap = m_font->getColouredPixmap(charName, NoteFont::Blue);
    }
    QPixmap dot = m_font->getPixmap(NoteCharacterNames::DOT);

    createPixmapAndMask(pixmap.width() + dot.width() * restType.getDots(),
                        pixmap.height());

    m_p.drawPixmap(0, 0, pixmap);
    m_pm.drawPixmap(0, 0, *(pixmap.mask()));

    QPoint hotspot(m_font->getHotspot(charName));
    hotspot.setX(0);

    int restY = hotspot.y() - dot.height() - getStaffLineThickness();
    if (restType.getNoteType() == Note::Semibreve ||
	restType.getNoteType() == Note::Breve) {
	restY += getLineSpacing();
    }

    for (int i = 0; i < restType.getDots(); ++i) {
        int x = pixmap.width() + i * dot.width();
        m_p.drawPixmap(x, restY, dot); 
        m_pm.drawPixmap(x, restY, *(dot.mask()));
    }

    return makeCanvasPixmap(hotspot);
}


QCanvasPixmap
NotePixmapFactory::makeClefPixmap(const Clef &clef) const
{
    if (m_selected) {
        return m_font->getColouredCanvasPixmap(getClefCharName(clef),
                                               NoteFont::Blue);
    } else {
        return m_font->getCanvasPixmap(getClefCharName(clef));
    }
}

QCanvasPixmap
NotePixmapFactory::makeUnknownPixmap()
{
    if (m_selected) {
        return m_font->getColouredCanvasPixmap(NoteCharacterNames::UNKNOWN,
                                               NoteFont::Blue);
    } else {
        return m_font->getCanvasPixmap(NoteCharacterNames::UNKNOWN);
    }
}

QCanvasPixmap
NotePixmapFactory::makeToolbarPixmap(const char *name)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    return QCanvasPixmap(pixmapDir + "/toolbar/" + name + ".xpm");
}

QCanvasPixmap
NotePixmapFactory::makeKeyPixmap(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);

    CharName charName = (key.isSharp() ?
                         NoteCharacterNames::SHARP :
                         NoteCharacterNames::FLAT);

    QPixmap accidentalPixmap;
    if (m_selected) {
        accidentalPixmap = m_font->getColouredPixmap(charName,NoteFont::Blue);
    } else {
        accidentalPixmap = m_font->getPixmap(charName);
    }
    QPoint hotspot(m_font->getHotspot(charName));

    int x = 0;
    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - 2*m_origin.x();

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

QCanvasPixmap
NotePixmapFactory::makeHairpinPixmap(int length, bool isCrescendo)
{
    int nbh = getNoteBodyHeight();
    int nbw = getNoteBodyWidth();

    int height = (int)(((double)nbh / (double)(nbw * 40)) * length) + nbh;
    int thickness = getStaffLineThickness() * 3 / 2;

//    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeHairpinPixmap: mapped length " << length << " to height " << height << " (nbh = " << nbh << ", nbw = " << nbw << ")" << endl;

    if (height < nbh)   height = nbh;
    if (height > nbh*2) height = nbh*2;

    height += thickness - 1;

    createPixmapAndMask(length, height);

    int left = 1, right = length - 2*nbw/3 + 1;

    if (isCrescendo) {
	drawShallowLine(left, height/2-1,
			right, height - thickness - 1, thickness, true);
	drawShallowLine(left, height/2-1, right, 0, thickness, true);
    } else {
	drawShallowLine(left, 0, right, height/2-1, thickness, true);
	drawShallowLine(left, height - thickness - 1,
			right, height/2-1, thickness, true);
    }

    return makeCanvasPixmap(QPoint(0, height/2));
}

QCanvasPixmap
NotePixmapFactory::makeSlurPixmap(int length, int dy, bool above)
{
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
    
//    kdDebug(KDEBUG_AREA) << "Pixmap dimensions: " << length << "x" << height << endl;

    bool havePixmap = false;
    QPoint topLeft, bottomRight, hotspot;

    for (int i = 0; i < thickness; ++i) {

	Spline::PointList pl;
	pl.push_back(QPoint(mx1, (int)my1));
	pl.push_back(QPoint(mx2, (int)my2));

	Spline::PointList *polyPoints = Spline::calculate
	    (QPoint(0, y1), QPoint(length-1, y2), pl, topLeft, bottomRight);

	if (!havePixmap) {
	    createPixmapAndMask(bottomRight.x() - topLeft.x(),
				bottomRight.y() - topLeft.y() + thickness - 1);
	    hotspot = QPoint(-topLeft.x(), -topLeft.y());
	    if (m_selected) m_p.setPen(Qt::blue);
	    havePixmap = true;
	}


//	pl.push_back(QPoint(dx, hotspotY + m1));
//	pl.push_back(QPoint(length - dx - 1, hotspotY + m2));

//	Spline::PointList *polyPoints = Spline::calculate
//	    (QPoint(0, hotspotY), QPoint(length-1, hotspotY + dy), pl);

	int ppc = polyPoints->size();
	QPointArray qp(ppc);

	for (int i = 0; i < ppc; ++i) {
	    qp.setPoint(i,
			hotspot.x() + (*polyPoints)[i].x(),
			hotspot.y() + (*polyPoints)[i].y());
	}

	delete polyPoints;

	m_p.drawPolyline(qp);
	m_pm.drawPolyline(qp);

	if (above) { ++my1; ++my2; }
	else { --my1; --my2; }
    }

    if (m_selected) {
        m_p.setPen(Qt::black);
    }

    return makeCanvasPixmap(hotspot);
}

QCanvasPixmap
NotePixmapFactory::makeTimeSigPixmap(const TimeSignature& sig)
{
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

    m_p.setFont(m_timeSigFont);
    m_pm.setFont(m_timeSigFont);

    if (m_selected) {
        m_p.setPen(Qt::blue);
    }

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


int NotePixmapFactory::getTimeSigWidth(const TimeSignature &sig) const
{
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

void
NotePixmapFactory::createPixmapAndMask(int width, int height)
{
    m_generatedPixmap = new QPixmap(width, height);
    m_generatedMask =
        new QBitmap(m_generatedPixmap->width(), m_generatedPixmap->height());

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    m_p.begin(m_generatedPixmap);
    m_pm.begin(m_generatedMask);

    m_p.setPen(Qt::black); m_p.setBrush(Qt::black);
    m_pm.setPen(Qt::white); m_pm.setBrush(Qt::white);
}

QCanvasPixmap
NotePixmapFactory::makeCanvasPixmap(QPoint hotspot)
{
    m_p.end();
    m_pm.end();
    QCanvasPixmap p(*m_generatedPixmap, hotspot);
    QBitmap m(*m_generatedMask);
    p.setMask(m);
    delete m_generatedPixmap;
    delete m_generatedMask;
    return p;
}

QPoint
NotePixmapFactory::m_pointZero;


int NotePixmapFactory::getNoteBodyWidth(Note::Type type) const {
    return m_font->getWidth(getNoteHeadCharName(type)) - 2*m_origin.x();
}

int NotePixmapFactory::getNoteBodyHeight(Note::Type type) const {
    return m_font->getHeight(getNoteHeadCharName(type)) - 2*m_origin.y();
}

int NotePixmapFactory::getLineSpacing() const {
    return m_font->getCurrentSize() + 1;
}

int NotePixmapFactory::getAccidentalWidth(Accidental a) const {
    return m_font->getWidth(getAccidentalCharName(a));
}

int NotePixmapFactory::getAccidentalHeight(Accidental a) const {
    return m_font->getHeight(getAccidentalCharName(a));
}

int NotePixmapFactory::getStemLength() const {
    return getNoteBodyHeight() * 13/4;
}

int NotePixmapFactory::getStemThickness() const {
    unsigned int i;
    (void)m_font->getStemThickness(i);
    return i;
}

int NotePixmapFactory::getStaffLineThickness() const {
    unsigned int i;
    (void)m_font->getStaffLineThickness(i);
    return i;
}

int NotePixmapFactory::getDotWidth() const {
    return m_font->getWidth(NoteCharacterNames::DOT);
}

int NotePixmapFactory::getClefWidth(const Clef &clef) const {
    return m_font->getWidth(getClefCharName(clef.getClefType()));
}

int NotePixmapFactory::getBarMargin() const {
    return getNoteBodyWidth() * 2;
}

int NotePixmapFactory::getRestWidth(const Rosegarden::Note &restType) const {
    return m_font->getWidth(getRestCharName(restType.getNoteType()))
        + (restType.getDots() * getDotWidth());
}

int NotePixmapFactory::getKeyWidth(const Rosegarden::Key &key) const {
    return 2*m_origin.x() + (key.getAccidentalCount() *
                             (getAccidentalWidth(key.isSharp() ? Sharp : Flat)));
}

