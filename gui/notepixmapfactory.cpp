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

#include "notefont.h"

using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::TimeSignature;
using Rosegarden::Sharp;
using Rosegarden::Flat;
using Rosegarden::DoubleSharp;
using Rosegarden::DoubleFlat;
using Rosegarden::Natural;

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
    m_gradient(0.0)
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
    m_timeSigFontMetrics(m_timeSigFont)
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
    m_timeSigFont.setPixelSize((size) * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

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
    
    if (isStemmed) {
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

    createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                        m_noteBodyHeight + m_above + m_below);

    QPoint s0, s1;

    if (isStemmed) {

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

    if (isStemmed) {
        unsigned int thickness;
        m_font->getStemThickness(thickness);
        for (unsigned int i = 0; i < thickness; ++i) {
            m_p.drawLine(s0, s1);
            m_pm.drawLine(s0, s1);
            ++s0.rx();
            ++s1.rx();
        }
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

    // We're done - generate the returned pixmap with the right offset
    //
    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, hotspot);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
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


// Bresenham algorithm, Wu antialiasing:

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
        colours.push_back(QColor(-1, 0, 64, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 128, QColor::Hsv));
        colours.push_back(QColor(-1, 0, 160, QColor::Hsv));
    }

    int cx = x0, cy = y0;

    int inc = 1;
    if (dv < 0) { dv = -dv; inc = -1; }

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
        if (inc > 0) quartile = 3 - quartile;

//        kdDebug(KDEBUG_AREA)
//            << "x = " << c.x() << ", y = " << c.y()
//            << ", g = " << g << ", dg1 = " << dg1 << ", dg2 = " << dg2
//            << ", seg = " << segment << ", q = " << quartile << endl;

        // I don't know enough about Qt to be sure of this, but I
        // suspect this may be some of the most inefficient code ever
        // written:

	int off = 0;

        m_p.setPen(colours[quartile]);
        m_p.drawPoint(cx, cy);
        m_pm.drawPoint(cx, cy);

	if (thickness > 1) m_p.setPen(colours[0]);
	while (++off < thickness) {
            m_p.drawPoint(cx, cy + off);
            m_pm.drawPoint(cx, cy + off);
        }
        
        m_p.setPen(colours[3-quartile]);
        m_p.drawPoint(cx, cy + off);
        m_pm.drawPoint(cx, cy + off);

        ++cx;
    }

    m_p.setPen(colours[0]);
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

    for (int i = 0; i < restType.getDots(); ++i) {
        int x = pixmap.width() + i * dot.width();
        int y = getLineSpacing()*5 / 4;
        m_p.drawPixmap(x, y, dot); 
        m_pm.drawPixmap(x, y, *(dot.mask()));
    }

    m_p.end();
    m_pm.end();

    QPoint hotspot(m_font->getHotspot(charName));
    hotspot.setX(0);
    QCanvasPixmap restPixmap(*m_generatedPixmap, hotspot);
    QBitmap mask(*m_generatedMask);
    restPixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return restPixmap;
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
    int delta = accidentalPixmap.width() - 2*m_origin.x(); //!!!

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

    m_p.end();
    m_pm.end();

    QCanvasPixmap p(*m_generatedPixmap, m_pointZero);
    QBitmap m(*m_generatedMask);
    p.setMask(m);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return p;
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

    m_p.end();
    m_pm.end();

    QCanvasPixmap p(*m_generatedPixmap, m_pointZero);
    QBitmap m(*m_generatedMask);
    p.setMask(m);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return p;
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

