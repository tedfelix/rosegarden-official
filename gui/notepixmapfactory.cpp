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


NotePixmapFactory::NotePixmapFactory(int size, std::string fontName) :
    m_timeSigFont("new century schoolbook", 8),
    m_timeSigFontMetrics(m_timeSigFont)
{
    --size;

    try {
//        fontName = "feta";

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
}

NotePixmapFactory::~NotePixmapFactory()
{
    delete m_font;
}


QCanvasPixmap
NotePixmapFactory::makeNotePixmap(Note::Type noteType,
                                  int dots,
                                  Accidental accidental,
                                  bool shifted,
                                  bool drawTail,
                                  bool stemGoesUp,
                                  int stemLength)
{
    return makeNotePixmapAux(noteType, dots, accidental, shifted, drawTail,
                             stemGoesUp, false, stemLength, 0, false,
                             false, 0, 0.0);
}

QCanvasPixmap
NotePixmapFactory::makeBeamedNotePixmap(Note::Type note,
					int dots,
					Accidental accidental,
                                        bool shifted,
					bool stemGoesUp,
					int stemLength,
					int nextTailCount,
                                        bool thisPartialTails,
                                        bool nextPartialTails,
					int width,
					double gradient)
{
    return makeNotePixmapAux
        (note, dots, accidental, shifted, false, stemGoesUp, true,
         stemLength, nextTailCount, thisPartialTails, nextPartialTails,
         width, gradient);
}

QCanvasPixmap
NotePixmapFactory::makeNotePixmapAux(Rosegarden::Note::Type noteType,
                                     int dots,
                                     Rosegarden::Accidental accidental,
                                     bool noteHeadShifted,
                                     bool drawTail,
                                     bool stemGoesUp,
                                     bool isBeamed,
                                     int stemLength,
                                     int nextTailCount,
                                     bool thisPartialTails,
                                     bool nextPartialTails,
                                     int width,
                                     double gradient)
{
    Note note(noteType, dots);

    if (isBeamed) drawTail = false;

    // A note pixmap is formed of note head, stem, flags,
    // accidentals, dots and beams.  Assume the note head first, then
    // do the rest of the calculations left to right, ie accidentals,
    // stem, flags, dots, beams

    // spacing surrounding the note head
    m_left = m_right = m_above = m_below = 0;

    m_noteBodyWidth  = getNoteBodyWidth(noteType);
    m_noteBodyHeight = getNoteBodyHeight(noteType);

    bool isStemmed = note.isStalked();
    int tailCount = note.getTailCount();

    if (accidental != NoAccidental) {
        makeRoomForAccidental(accidental);
    }

    if (stemLength < 0) {

        stemLength = getStemLength();
        int nbh = m_noteBodyHeight;

        //!!! We could do better by taking measurements out of the
        //mappings file, probably -- some fonts need longer stems
        //than others

        switch (tailCount) {
        case 1: stemLength += nbh / 8; break;
        case 2: stemLength += nbh / 2; break;
        case 3: stemLength += nbh + nbh / 8; break;
        case 4: stemLength += nbh * 2 - nbh / 4; break;
        default: break;
        }
    }
    
    if (isStemmed) {
        if (stemGoesUp) {
            m_above = std::max(m_above, stemLength - m_noteBodyHeight/2);
        } else {
            m_below = std::max(m_below, stemLength - m_noteBodyHeight/2);
        }

        if (isBeamed) {

            int beamSpacing = (int)(width * gradient);

            if (stemGoesUp) {

                beamSpacing = -beamSpacing;
                if (beamSpacing < 0) beamSpacing = 0;
                m_above += beamSpacing + 1;

                m_right = std::max(m_right, width);

            } else {

                if (beamSpacing < 0) beamSpacing = 0;
                m_below += beamSpacing + 1;
                
                m_right = std::max(m_right, width - m_noteBodyWidth);
            }
        }
    }            

    if (tailCount > 0) {
        if (stemGoesUp) {
            m_right += m_font->getWidth(getFlagCharName(tailCount));
        }
    }

    m_right = std::max(m_right, dots * getDotWidth());

    createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                        m_noteBodyHeight + m_above + m_below);

    if (accidental != NoAccidental) {
        drawAccidental(accidental);
    }

    //!!! Need some cleverness -- I guess there's no mask if the
    // pixmap has depth > 1?

    QPixmap body(m_font->getPixmap(getNoteHeadCharName(noteType)));

    m_p.drawPixmap (m_left, m_above, body);
    m_pm.drawPixmap(m_left, m_above, *(body.mask()));

    //!!! These should come back out into other functions again

    if (dots > 0) {

        QPixmap dot(m_font->getPixmap(NoteCharacterNames::DOT));
        int x = m_left + m_noteBodyWidth;
        int y = m_above + m_noteBodyHeight/2 - dot.height()/2;

        for (int i = 0; i < dots; ++i) {
            m_p.drawPixmap(x, y, dot);
            m_pm.drawPixmap(x, y, *(dot.mask()));
            x += dot.width();
        }
    }

    if (isStemmed) {

        QPoint s0, s1;
        s0.setY(m_above + m_noteBodyHeight/2);

        if (stemGoesUp) {
            s0.setX(m_left + m_noteBodyWidth - 1);
            s1.setY(s0.y() - stemLength);
        } else {
            s0.setX(m_left);
            s1.setY(s0.y() + stemLength);
        }

        s1.setX(s0.x());

        if (tailCount > 0) {

            if (drawTail) {

                QPixmap tails = m_font->getPixmap
                    (getFlagCharName(tailCount), !stemGoesUp);

                if (stemGoesUp) {
                    m_p.drawPixmap(s1.x(), s1.y(), tails);
                    m_pm.drawPixmap(s1.x(), s1.y(), *(tails.mask()));
                } else {
                    m_p.drawPixmap(s1.x(), s1.y() - tails.height(), tails);
                    m_pm.drawPixmap(s1.x(), s1.y() - tails.height(), *(tails.mask()));
                }
            } else if (isBeamed) {

                drawBeams(s1, stemGoesUp, tailCount, nextTailCount,
                          thisPartialTails, nextTailCount,
                          width, gradient);

            }
        }

        m_p.drawLine(s0, s1);
        m_pm.drawLine(s0, s1);
    }

    QPoint hotspot(m_left, m_above + m_noteBodyHeight/2);

//#define ROSE_XDEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_XDEBUG_NOTE_PIXMAP_FACTORY
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);

    m_p.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_p.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_pm.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    {
	int hsx = hotspot.x();
	int hsy = hotspot.y();
	m_p.drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
	m_pm.drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
	m_p.drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
	m_pm.drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
    }


/*
    m_p.drawPoint(0,0);
    m_p.drawPoint(0,m_offsets.getHotSpot().y());
    m_p.drawPoint(0,m_generatedPixmap->height() - 1);
    m_p.drawPoint(m_generatedPixmap->width() - 1,0);
    m_p.drawPoint(m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawPoint(0,0);
    m_pm.drawPoint(0,m_offsets.getHotSpot().y());
    m_pm.drawPoint(0,m_generatedPixmap->height() -1);
    m_pm.drawPoint(m_generatedPixmap->width() -1,0);
    m_pm.drawPoint(m_generatedPixmap->width() -1,m_generatedPixmap->height()-1);
*/
#endif
    
    //#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    // add red dots at each corner of the pixmap
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);
    m_p.drawPoint(0,0);
    m_p.drawPoint(0,m_offsets.getHotSpot().y());
    m_p.drawPoint(0,m_generatedPixmap->height() - 1);
    m_p.drawPoint(m_generatedPixmap->width() - 1,0);
    m_p.drawPoint(m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawPoint(0,0);
    m_pm.drawPoint(0,m_offsets.getHotSpot().y());
    m_pm.drawPoint(0,m_generatedPixmap->height() -1);
    m_pm.drawPoint(m_generatedPixmap->width() -1,0);
    m_pm.drawPoint(m_generatedPixmap->width() -1,m_generatedPixmap->height()-1);
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

    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::drawAccidental: m_above is " << m_above << ", hotspot-y is " << ah.y() << ", note body height is " << m_noteBodyHeight << ", accidental height is " << ap.height() << endl;

    m_p.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), ap);
    m_pm.drawPixmap(0, m_above + m_noteBodyHeight/2 - ah.y(), *(ap.mask()));
}

void
NotePixmapFactory::drawBeams(const QPoint &s1, bool stemGoesUp,
                             int tailCount, int nextTailCount, 
                             bool thisPartialTails, bool nextPartialTails,
                             int width, double gradient)
{
    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw tails for those that appear at the
    // end only

    int startY = s1.y(), startX = s1.x();
    int commonTailCount = std::min(tailCount, nextTailCount);
    int thickness = (m_noteBodyHeight + 2) / 3;
    int gap = thickness - 1;
    if (gap < 1) gap = 1;
                
    for (int j = 0; j < commonTailCount; ++j) {
        for (int i = 0; i < thickness; ++i) {
            int offset = j * (thickness + gap) + i;
            if (!stemGoesUp) offset = -offset;
            m_p.drawLine(startX, startY + offset, startX + width,
                         startY + (int)(width * gradient) + offset);
            m_pm.drawLine(startX, startY + offset, startX + width,
                          startY + (int)(width * gradient) + offset);
        }
    }

    int partWidth = width / 3;
    if (partWidth < 2) partWidth = 2;
    else if (partWidth > m_noteBodyWidth) partWidth = m_noteBodyWidth;
    
    if (thisPartialTails) {
        for (int j = commonTailCount; j < tailCount; ++j) {
            for (int i = 0; i < thickness; ++i) {
                int offset = j * (thickness + gap) + i;
                if (!stemGoesUp) offset = -offset;
                m_p.drawLine(startX, startY + offset, startX + partWidth,
                             startY + (int)(partWidth * gradient) + offset);
                m_pm.drawLine(startX, startY + offset, startX + partWidth,
                              startY + (int)(partWidth * gradient) + offset);
            }
        }
    }
    
    if (nextPartialTails) {
        startX += width - partWidth;
        startY += (int)((width - partWidth) * gradient);
        
        for (int j = commonTailCount; j < nextTailCount; ++j) {
            for (int i = 0; i < thickness; ++i) {
                int offset = j * (thickness + gap) + i;
                if (!stemGoesUp) offset = -offset;
                m_p.drawLine(startX, startY + offset, startX + partWidth,
                             startY + (int)(partWidth * gradient) + offset);
                m_pm.drawLine(startX, startY + offset, startX + partWidth,
                              startY + (int)(partWidth * gradient) + offset);
            }
        }
    }
}

QCanvasPixmap
NotePixmapFactory::makeRestPixmap(const Note &restType) 
{
    //!!! hotspot

    QPixmap pixmap(m_font->getPixmap(getRestCharName(restType.getNoteType())));

    createPixmapAndMask(getRestWidth(restType), pixmap.height());

    m_p.drawPixmap(0, 0, pixmap);
    m_pm.drawPixmap(0, 0, *(pixmap.mask()));

    QPixmap dot(m_font->getPixmap(NoteCharacterNames::DOT));

    for (int i = 0; i < restType.getDots(); ++i) {
        int x = pixmap.width() + i * dot.width();
        int y = getLineSpacing()*5 / 4;
        m_p.drawPixmap(x, y, dot); 
        m_pm.drawPixmap(x, y, *(dot.mask()));
    }

    m_p.end();
    m_pm.end();

    QCanvasPixmap restPixmap(*m_generatedPixmap, m_pointZero);
    QBitmap mask(*m_generatedMask);
    restPixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return restPixmap;
}


QCanvasPixmap
NotePixmapFactory::makeClefPixmap(const Clef &clef) const
{
    return m_font->getCanvasPixmap(getClefCharName(clef));
}

QCanvasPixmap
NotePixmapFactory::makeUnknownPixmap()
{
    return m_font->getCanvasPixmap(NoteCharacterNames::UNKNOWN);
}

//!!!
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

    QPixmap accidentalPixmap
        (m_font->getPixmap(key.isSharp() ?
                           NoteCharacterNames::SHARP :
                           NoteCharacterNames::FLAT));

    int x = 0;
    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - (key.isSharp() ? 1 : 2); //!!!

    createPixmapAndMask(delta * ah.size() + 2, lw * 8 + 1);

    for (unsigned int i = 0; i < ah.size(); ++i) {

	int h = ah[i];
	int y = (lw * 2) + ((8 - h) * lw) / 2// + ((h % 2 == 1) ? 1 : 0)
	    - (accidentalPixmap.height() / 2);

	// tricky one: sharps and flats are the same size, but
	// they have different "centres"
	if (!key.isSharp()) y -= 2;

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

    x = (width - numR.width()) / 2 - 1;
    m_p.drawText(x, denomR.height(), numS);
    m_pm.drawText(x, denomR.height(), numS);

    x = (width - denomR.width()) / 2 - 1;
    m_p.drawText(x, denomR.height() * 2 + (getNoteBodyHeight()/2) - 1, denomS);
    m_pm.drawText(x, denomR.height() * 2 + (getNoteBodyHeight()/2) - 1, denomS);

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


int NotePixmapFactory::getNoteBodyHeight(Note::Type type) const {
    return m_font->getHeight(getNoteHeadCharName(type));
}

int NotePixmapFactory::getNoteBodyWidth(Note::Type type) const {
    return m_font->getWidth(getNoteHeadCharName(type));
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
    return getNoteBodyHeight() * 11/4;
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
    return m_font->getWidth(getRestCharName(restType.getNoteType())) +
        getDotWidth() * restType.getDots();
}

int NotePixmapFactory::getKeyWidth(const Rosegarden::Key &key) const {
    return (key.getAccidentalCount() *
            (getAccidentalWidth
             (key.isSharp() ? Sharp : Flat) - (key.isSharp()? 1 : 2))); //!!!
}

