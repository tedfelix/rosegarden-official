
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
#include "rosedebug.h"
#include "rosegardenguiview.h"
#include "notepixmapfactory.h"
#include "NotationTypes.h"

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

NotePixmapOffsets::NotePixmapOffsets()
{
}

void
NotePixmapOffsets::offsetsFor(Note::Type note,
                              int dots,
                              Accidental accidental,
                              bool shifted,
                              bool drawTail,
                              bool stalkGoesUp,
                              bool fixedHeight)
{
    m_note = note;
    m_accidental = accidental;
    m_shifted = shifted;
    m_drawTail = drawTail;
    m_stalkGoesUp = stalkGoesUp;
    m_dots = dots;
    m_noteHasStalk = Note(note).isStalked();

    m_bodyOffset.setX(0);     m_bodyOffset.setY(0);
    m_hotSpot.setX(0);        m_hotSpot.setY(0);
    m_accidentalOffset.setX(0); m_accidentalOffset.setY(0);
    
    if (note >= Note::HalfNote) {
        if (note == Note::Breve) {
            m_bodySize = m_breveSize;
        } else {
            m_bodySize = m_noteBodyEmptySize;
        }
    } else {
        m_bodySize = m_noteBodyFilledSize;
    }

    computeAccidentalAndStalkSize();
    computePixmapSize();
    computeBodyOffset();

    if (fixedHeight) {
        //ew
        int d = m_pixmapSize.height();
        m_pixmapSize.setHeight(m_bodySize.height() * 6);
        d = m_pixmapSize.height() - d;
	if (d > 2) d = d - 2;
        m_stalkPoints.first.ry() += d;
        m_stalkPoints.second.ry() += d;
        m_bodyOffset.ry() += d;
        m_hotSpot.ry() += d;
        m_accidentalOffset.ry() += d;
    }
}

void
NotePixmapOffsets::computeAccidentalAndStalkSize()
{
    unsigned int tailOffset =
        (m_note < Note::QuarterNote && m_stalkGoesUp) ? m_tailWidth : 0;
    unsigned int totalXOffset = tailOffset,
        totalYOffset = 0;
    
    if (m_accidental == Sharp) {
        totalXOffset += m_sharpWidth + 1;
        totalYOffset = 3;
    }
    if (m_accidental == DoubleSharp) {
        totalXOffset += m_doubleSharpWidth + 1;
        totalYOffset = 3;
    }
    else if (m_accidental == Flat) {
        totalXOffset += m_flatWidth + 1;
        if (!m_noteHasStalk || !m_stalkGoesUp) totalYOffset = 4;
    }
    else if (m_accidental == DoubleFlat) {
        totalXOffset += m_doubleFlatWidth + 1;
        if (!m_noteHasStalk || !m_stalkGoesUp) totalYOffset = 4;
    }
    else if (m_accidental == Natural) {
        totalXOffset += m_naturalWidth + 1;
        totalYOffset = 3;
    }

    m_accidentalStalkSize.setWidth(totalXOffset);
    m_accidentalStalkSize.setHeight(totalYOffset);
}



void
NotePixmapOffsets::computePixmapSize()
{
    m_pixmapSize.setWidth(m_bodySize.width() + m_accidentalStalkSize.width() +
                          m_dots * m_dotSize.width());

    if (m_noteHasStalk) {

        m_pixmapSize.setHeight(m_bodySize.height() / 2 +
                               m_stalkLength +
			       m_extraBeamSpacing +
                               m_accidentalStalkSize.height());

    } else {
        m_pixmapSize.setHeight(m_bodySize.height() + m_accidentalStalkSize.height());
    }


    switch (m_accidental) {
        
    case NoAccidental:
        break;
        
    case Sharp:
    case DoubleSharp:
    case Natural:

        m_pixmapSize.rheight() += 
            (m_accidentalHeight - m_noteBodyEmptySize.height()) / 2;
        break;
        
    case Flat:
    case DoubleFlat:

        if (!m_stalkGoesUp) {
            m_pixmapSize.rheight() += 2 +
                (m_accidentalHeight - m_noteBodyEmptySize.height()) / 2;
        }

        break;
    }

    if (m_shifted) {
        m_pixmapSize.rwidth() += m_noteBodyFilledSize.width() - 1;
    }
}

void
NotePixmapOffsets::computeBodyOffset()
{
    // Simple case : no accidental - Y coord is valid for all cases
    //
    if (m_stalkGoesUp) {

        m_bodyOffset.setY(m_pixmapSize.height() - m_bodySize.height());
        m_hotSpot.setY(m_pixmapSize.height() - m_bodySize.height() / 2);
        m_stalkPoints.first.setY
	    (m_pixmapSize.height() - m_bodySize.height() / 2 - 1);

    } else {

        m_hotSpot.setY(m_bodySize.height() / 2);
        m_stalkPoints.first.setY(m_bodySize.height() / 2);
    }   

    int accidentalProtrusion =
        (m_accidentalHeight - m_noteBodyEmptySize.height()) / 2;

    switch (m_accidental) {
        
    case NoAccidental:
        break;
        
    case Sharp:
    case DoubleSharp:

        m_bodyOffset.setX(m_sharpWidth + 1);

        if (m_stalkGoesUp) {
            m_bodyOffset.ry() -= accidentalProtrusion;
            m_hotSpot.ry() -= accidentalProtrusion;
            m_stalkPoints.first.ry() -= accidentalProtrusion;
        } else {
            m_bodyOffset.ry() += accidentalProtrusion;
            m_hotSpot.ry() += accidentalProtrusion;
            m_stalkPoints.first.ry() += accidentalProtrusion;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - accidentalProtrusion);
        break;
        
    case Flat:
    case DoubleFlat:

        // flat is the same height as sharp and natural, but has a
        // different "centre"
        
        m_bodyOffset.setX(m_flatWidth + 1);

        if (!m_stalkGoesUp) {
            m_bodyOffset.ry() += accidentalProtrusion + 1;
            m_hotSpot.ry() += accidentalProtrusion + 1;
            m_stalkPoints.first.ry() += accidentalProtrusion + 1;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - (accidentalProtrusion + 2));
        break;
        
    case Natural:

        m_bodyOffset.setX(m_naturalWidth + 1);

        if (m_stalkGoesUp) {
            m_bodyOffset.ry() -= accidentalProtrusion;
            m_hotSpot.ry() -= accidentalProtrusion;
            m_stalkPoints.first.ry() -= accidentalProtrusion;
        } else {
            m_bodyOffset.ry() += accidentalProtrusion;
            m_hotSpot.ry() += accidentalProtrusion;
            m_stalkPoints.first.ry() += accidentalProtrusion;
        }

        m_accidentalOffset.setY(m_bodyOffset.y() - accidentalProtrusion);
        break;

    }

    if (m_accidental != NoAccidental)
        m_hotSpot.setX(m_bodyOffset.x());

    if (m_stalkGoesUp)
        m_stalkPoints.first.setX(m_bodyOffset.x() + m_bodySize.width() - 2);
    else
        m_stalkPoints.first.setX(m_bodyOffset.x());

    m_stalkPoints.second.setX(m_stalkPoints.first.x());

    if (m_stalkGoesUp)
	m_stalkPoints.second.setY(m_stalkPoints.first.y() - m_stalkLength);
    else
	m_stalkPoints.second.setY(m_stalkPoints.first.y() + m_stalkLength);

    if (m_shifted)
        m_bodyOffset.rx() += m_noteBodyFilledSize.width() - 1;
}


void
NotePixmapOffsets::setNoteBodySizes(QSize empty, QSize filled, QSize breve)
{
    m_noteBodyEmptySize = empty;
    m_noteBodyFilledSize = filled;
    m_breveSize = breve;
}

void
NotePixmapOffsets::setTailWidth(unsigned int s)
{
    m_tailWidth = s;
}

void
NotePixmapOffsets::setStalkLength(unsigned int s)
{
    m_stalkLength = s;
}

void
NotePixmapOffsets::setAccidentalHeight(unsigned int h)
{
    m_accidentalHeight = h;
}

void
NotePixmapOffsets::setExtraBeamSpacing(unsigned int bs)
{
    m_extraBeamSpacing = bs;
}

void
NotePixmapOffsets::setAccidentalsWidth(unsigned int sharp,
				       unsigned int flat,
				       unsigned int doublesharp,
				       unsigned int doubleflat,
				       unsigned int natural)
{
    m_sharpWidth = sharp;
    m_flatWidth = flat;
    m_doubleSharpWidth = doublesharp;
    m_doubleFlatWidth = doubleflat;
    m_naturalWidth = natural;
}

void NotePixmapOffsets::setDotSize(QSize size)
{
    m_dotSize = size;
}


NotePixmapFactory::NotePixmapFactory(int resolution) :
    m_resolution(resolution),
    m_pixmapDirectory(getPixmapDirectory(resolution)),
    m_generatedPixmapHeight(0),
    m_timeSigFont("new century schoolbook", 8),
    m_timeSigFontMetrics(m_timeSigFont),
    m_noteBodyFilled(m_pixmapDirectory + "/note-bodyfilled.xpm"),
    m_noteBodyEmpty(m_pixmapDirectory + "/note-bodyempty.xpm"),
    m_breve(m_pixmapDirectory + "/note-breve.xpm"),
    m_accidentalSharp(m_pixmapDirectory + "/notemod-sharp.xpm"),
    m_accidentalFlat(m_pixmapDirectory + "/notemod-flat.xpm"),
    m_accidentalDoubleSharp(m_pixmapDirectory + "/notemod-doublesharp.xpm"),
    m_accidentalDoubleFlat(m_pixmapDirectory + "/notemod-doubleflat.xpm"),
    m_accidentalNatural(m_pixmapDirectory + "/notemod-natural.xpm"),
    m_dot(m_pixmapDirectory + "/dot.xpm"),
    m_clefWidth(-1)
{
    // 9 => 20, 5 => 10
    m_timeSigFont.setPixelSize((resolution - 1) * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    QString pixmapTailUpFileName(m_pixmapDirectory + "/tail-up-%1.xpm"),
          pixmapTailDownFileName(m_pixmapDirectory + "/tail-down-%1.xpm");

    for (unsigned int i = 0; i < 4; ++i) {
        m_tailsUp.push_back(new QPixmap(pixmapTailDownFileName.arg(i+1)));
        m_tailsDown.push_back(new QPixmap(pixmapTailUpFileName.arg(i+1)));
    }

    m_generatedPixmapHeight = getNoteBodyHeight() / 2 + getStalkLength();

    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-hemidemisemi.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-demisemi.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-semiquaver.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-quaver.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-crotchet.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-minim.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-semibreve.xpm"));
    m_rests.push_back(new QPixmap(m_pixmapDirectory + "/rest-breve.xpm"));

    // Init offsets
    m_offsets.setNoteBodySizes(m_noteBodyEmpty.size(),
                               m_noteBodyFilled.size(),
                               m_breve.size());

    m_offsets.setTailWidth(m_tailsUp[0]->width());
    m_offsets.setStalkLength(getStalkLength());
    m_offsets.setAccidentalsWidth(m_accidentalSharp.width(),
                                  m_accidentalFlat.width(),
                                  m_accidentalDoubleSharp.width(),
                                  m_accidentalDoubleFlat.width(),
                                  m_accidentalNatural.width());
    m_offsets.setAccidentalHeight(m_accidentalSharp.height());
    
    m_offsets.setDotSize(m_dot.size());
}

QString NotePixmapFactory::getPixmapDirectory(int resolution)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    
    QString res = QString("%1/%2").arg(pixmapDir).arg(resolution);
    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::getPixmapDirectory() : "
                         << res << " - pixmapDir : " << pixmapDir << endl;
    return res;
}

NotePixmapFactory::~NotePixmapFactory()
{
    for (unsigned int i = 0; i < m_tailsUp.size(); ++i) {
        delete m_tailsUp[i];
        delete m_tailsDown[i];
    }

    for (unsigned int i = 0; i < m_rests.size(); ++i) {
        delete m_rests[i];
    }
}


QCanvasPixmap
NotePixmapFactory::makeNotePixmap(Note::Type note,
                                  int dots,
                                  Accidental accidental,
                                  bool shifted,
                                  bool drawTail,
                                  bool stalkGoesUp,
                                  bool fixedHeight,
                                  int stalkLength)
{
    if (stalkLength < 0) {

        stalkLength = getStalkLength();

        if (note < Note::QuarterNote) {

            int nbh = getNoteBodyHeight();
	
            // readjust pixmap height according to its duration - the stalk
            // is longer for 8th, 16th, etc.. because the tail is higher
            //
            if (note == Note::EighthNote)
                stalkLength += nbh / 8;
            else if (note == Note::SixteenthNote)
                stalkLength += nbh / 2;
            else if (note == Note::ThirtySecondNote)
                stalkLength += nbh + nbh / 8;
            else if (note == Note::SixtyFourthNote)
                stalkLength += nbh * 2 - nbh / 4;
        }
    }

    m_offsets.setStalkLength(stalkLength);

    m_offsets.setExtraBeamSpacing(0);
    m_offsets.offsetsFor
        (note, dots, accidental, shifted, drawTail, stalkGoesUp, fixedHeight);

    if (note > Note::Longest) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : note > Note::Longest ("
                             << note << ")\n";
        throw -1;
    }

    bool noteHasStalk = note < Note::WholeNote;

    m_generatedPixmapHeight = m_offsets.getPixmapSize().height();
    
    createPixmapAndMask();

    // paint note body
    //
    QPixmap *body =
        (note == Note::Breve) ? &m_breve :
        (note >= Note::HalfNote) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    m_p.drawPixmap (m_offsets.getBodyOffset(), *body);
    m_pm.drawPixmap(m_offsets.getBodyOffset(), *(body->mask()));
    
    if (dots > 0)
        drawDots(dots);

    // paint stalk (if needed)
    //
    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    // paint accidental (if needed)
    //
    if (accidental != NoAccidental)
        drawAccidental(accidental, stalkGoesUp);

//#define ROSE_XDEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_XDEBUG_NOTE_PIXMAP_FACTORY
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);

    m_p.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_p.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_pm.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    {
	int hsx = m_offsets.getHotSpot().x();
	int hsy = m_offsets.getHotSpot().y();
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

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_offsets.getHotSpot());
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
}


// Implementation-wise this could easily be rolled in with
// makeNotePixmap (taking the beam stuff out into another method), but
// the real problem is how to make a coherent API for all this

QCanvasPixmap
NotePixmapFactory::makeBeamedNotePixmap(Note::Type note,
					int dots,
					Accidental accidental,
                                        bool shifted,
					bool stalkGoesUp,
					int stalkLength,
					int nextTailCount,
                                        bool thisPartialTails,
                                        bool nextPartialTails,
					int width,
					double gradient)
{
    m_offsets.setStalkLength(stalkLength);

    int beamSpacing = (int)(width * gradient);
    if (beamSpacing > 0) {
	if (stalkGoesUp) beamSpacing = 1;
	else beamSpacing += 1;
    } else {
	if (!stalkGoesUp) beamSpacing = 1;
	else beamSpacing = -beamSpacing + 1;
    }
    m_offsets.setExtraBeamSpacing(beamSpacing);

    m_offsets.offsetsFor
        (note, dots, accidental, shifted, false, stalkGoesUp, false);

    if (note > Note::Longest) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : note > Note::Longest ("
                             << note << ")\n";
        throw -1;
    }

    bool noteHasStalk = note < Note::WholeNote;

    m_generatedPixmapHeight = m_offsets.getPixmapSize().height();
    
    int startY, startX;
//    if (stalkGoesUp) {
	startY = m_offsets.getStalkPoints().second.y();
	startX = m_offsets.getStalkPoints().second.x();
//    } else {
//	startY = m_offsets.getStalkPoints().first.y();
//	startX = m_offsets.getStalkPoints().first.x();
//    }

    int endX = startX + width;
    if (endX >= m_offsets.getPixmapSize().width()) {
	createPixmapAndMask(endX + 1, m_offsets.getPixmapSize().height());
    } else {
	createPixmapAndMask();
    }

    // paint note body
    //
    QPixmap *body =
        (note == Note::Breve) ? &m_breve :
        (note >= Note::HalfNote) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    m_p.drawPixmap (m_offsets.getBodyOffset(), *body);
    m_pm.drawPixmap(m_offsets.getBodyOffset(), *(body->mask()));
    
    if (dots > 0)
        drawDots(dots);

    // paint stalk (if needed)
    //
    if (noteHasStalk)
        drawStalk(note, false, stalkGoesUp);

    // paint accidental (if needed)
    //
    if (accidental != NoAccidental)
        drawAccidental(accidental, stalkGoesUp);

    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw tails for those that appear at the
    // end only

    int myTailCount = Note(note).getTailCount();
    int commonTailCount = std::min(myTailCount, nextTailCount);
    int thickness = (getNoteBodyHeight() + 2) / 3;
    int gap = thickness - 1;
    if (gap < 1) gap = 1;

    for (int j = 0; j < commonTailCount; ++j) {
	for (int i = 0; i < thickness; ++i) {
	    int offset = j * (thickness + gap) + i;
	    if (!stalkGoesUp) offset = -offset;
	    m_p.drawLine(startX, startY + offset, startX + width,
			 startY + (int)(width * gradient) + offset);
	    m_pm.drawLine(startX, startY + offset, startX + width,
			  startY + (int)(width * gradient) + offset);
	}
    }

    int partWidth = width / 3;
    if (partWidth < 2) partWidth = 2;
    else if (partWidth > getNoteBodyWidth()) partWidth = getNoteBodyWidth();

    if (thisPartialTails) {
        for (int j = commonTailCount; j < myTailCount; ++j) {
            for (int i = 0; i < thickness; ++i) {
                int offset = j * (thickness + gap) + i;
                if (!stalkGoesUp) offset = -offset;
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
                if (!stalkGoesUp) offset = -offset;
                m_p.drawLine(startX, startY + offset, startX + partWidth,
                             startY + (int)(partWidth * gradient) + offset);
                m_pm.drawLine(startX, startY + offset, startX + partWidth,
                              startY + (int)(partWidth * gradient) + offset);
            }
        }
    }
        

//#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);
    m_p.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_p.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);
    m_pm.drawLine(0,0,0,m_generatedPixmap->height() - 1);
    m_pm.drawLine(m_generatedPixmap->width() - 1,0,m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);
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

    // We're done - generate the returned pixmap with the right offset
    //
    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_offsets.getHotSpot());
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
}


QCanvasPixmap
NotePixmapFactory::makeRestPixmap(const Note &restType) 
{
    QPixmap *pixmap(m_rests[restType.getNoteType()]);

    createPixmapAndMask(getRestWidth(restType), pixmap->height());

    m_p.drawPixmap(0, 0, *pixmap);
    m_pm.drawPixmap(0, 0, *(pixmap->mask()));

    for (int i = 0; i < restType.getDots(); ++i) {
        int x = pixmap->width() + i * m_dot.width();
        int y = getLineSpacing()*5 / 4;
        m_p.drawPixmap(x, y, m_dot); 
        m_pm.drawPixmap(x, y, *(m_dot.mask()));
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
    QString filename = m_pixmapDirectory;
    filename += QString("/clef-") + clef.getClefType().c_str() + ".xpm";
    return QCanvasPixmap(filename);
}

int NotePixmapFactory::getClefWidth() const
{
    if (m_clefWidth < 0) {
        QCanvasPixmap p(makeClefPixmap(Clef::DefaultClef));
        m_clefWidth = p.width();
    }
    return m_clefWidth;
}

QCanvasPixmap
NotePixmapFactory::makeUnknownPixmap()
{
    return QCanvasPixmap(m_pixmapDirectory + "/unknown.xpm");
}

QCanvasPixmap
NotePixmapFactory::makeKeyPixmap(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);

    QPixmap &accidentalPixmap
	(key.isSharp() ? m_accidentalSharp : m_accidentalFlat);

    int x = 0;
    int lw = getLineSpacing();
    int delta = getAccidentalWidth() - (key.isSharp() ? 1 : 2);

    createPixmapAndMask(delta * ah.size() + 2, lw * 8 + 1);

    for (unsigned int i = 0; i < ah.size(); ++i) {

	int h = ah[i];
	int y = (lw * 2) + ((8 - h) * lw) / 2// + ((h % 2 == 1) ? 1 : 0)
	    - (getAccidentalHeight() / 2);

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
    if (width < 0)  width  = m_offsets.getPixmapSize().width();
    if (height < 0) height = m_offsets.getPixmapSize().height();

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

const QPixmap*
NotePixmapFactory::tailUp(Note::Type note) const
{
    if (note >= Note::QuarterNote) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Note::EighthNote)
        return m_tailsUp[0];
    if (note == Note::SixteenthNote)
        return m_tailsUp[1];
    if (note == Note::ThirtySecondNote)
        return m_tailsUp[2];
    if (note == Note::SixtyFourthNote)
        return m_tailsUp[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
    
}

const QPixmap*
NotePixmapFactory::tailDown(Note::Type note) const
{
    if (note >= Note::QuarterNote) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailDown : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Note::EighthNote)
        return m_tailsDown[0];
    if (note == Note::SixteenthNote)
        return m_tailsDown[1];
    if (note == Note::ThirtySecondNote)
        return m_tailsDown[2];
    if (note == Note::SixtyFourthNote)
        return m_tailsDown[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
}


void
NotePixmapFactory::drawStalk(Note::Type note,
                             bool drawTail, bool stalkGoesUp)
{
    QPoint lineOrig, lineDest;

    lineOrig = m_offsets.getStalkPoints().first;
    lineDest = m_offsets.getStalkPoints().second;

//    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::drawStalk: drawing stalk " << lineDest.y() - lineOrig.y() << " pixels long" << endl;

    m_p.drawLine(lineOrig, lineDest);
    m_pm.drawLine(lineOrig, lineDest);

    if (drawTail && note < Note::QuarterNote) {
        // need to add a tail pixmap
        //
        const QPixmap *tailPixmap = 0;

        if (stalkGoesUp) {
            tailPixmap = tailUp(note);

            m_p.drawPixmap (m_offsets.getStalkPoints().second.x() + 1 , m_offsets.getStalkPoints().second.y(), *tailPixmap);
            m_pm.drawPixmap(m_offsets.getStalkPoints().second.x() + 1 , m_offsets.getStalkPoints().second.y(), *(tailPixmap->mask()));

        } else {

            tailPixmap = tailDown(note);

            m_p.drawPixmap (m_offsets.getStalkPoints().first.x() + 1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *tailPixmap);

            m_pm.drawPixmap(m_offsets.getStalkPoints().first.x() + 1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *(tailPixmap->mask()));

//             m_p.drawPixmap (1,
//                             m_generatedPixmapHeight - tailPixmap->height(),
//                             *tailPixmap);

//             m_pm.drawPixmap(1,
//                             m_generatedPixmapHeight - tailPixmap->height(),
//                             *(tailPixmap->mask()));
        }

    }
}

void
NotePixmapFactory::drawDots(int dots)
{
    int x = m_offsets.getBodyOffset().x() + m_noteBodyFilled.size().width();
    int y = m_offsets.getBodyOffset().y();

    for (int i = 0; i < dots; ++i) {
	m_p.drawPixmap(x, y, m_dot);
	m_pm.drawPixmap(x, y, *(m_dot.mask()));
	x += m_dot.size().width();
    }
}

void
NotePixmapFactory::drawAccidental(Accidental accidental, bool /*stalkGoesUp*/)
{
    const QPixmap *accidentalPixmap = 0;

    switch (accidental) {

    case NoAccidental:
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::drawAccidental() called with NoAccidental"
                             << endl;
        KMessageBox::error(0, "NotePixmapFactory::drawAccidental() called with NoAccidental");
        return;
        break;
        
    case Sharp:
        accidentalPixmap = &m_accidentalSharp;
        break;
        
    case DoubleSharp:
        accidentalPixmap = &m_accidentalDoubleSharp;
        break;

    case Flat:
        accidentalPixmap = &m_accidentalFlat;
        break;

    case DoubleFlat:
        accidentalPixmap = &m_accidentalFlat;
        break;

    case Natural:
        accidentalPixmap = &m_accidentalNatural;
        break;
    }

    m_p.drawPixmap(m_offsets.getAccidentalOffset().x(),
                   m_offsets.getAccidentalOffset().y(),
                   *accidentalPixmap);

    m_pm.drawPixmap(m_offsets.getAccidentalOffset().x(),
                    m_offsets.getAccidentalOffset().y(),
                    *(accidentalPixmap->mask()));
    
}


QPoint
NotePixmapFactory::m_pointZero;

