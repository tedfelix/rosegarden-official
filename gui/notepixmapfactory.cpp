/***************************************************************************
                          notepixmapfactory.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstdio>

#include <algorithm>

#include <kdebug.h>

#include "rosegardenguiview.h"
#include "notepixmapfactory.h"
#include "staff.h"

NotePixmapFactory::NotePixmapFactory()
    : m_generatedPixmapHeight(0),
      m_noteBodyHeight(0),
      m_tailWidth(0),
      m_noteBodyFilled("pixmaps/note-bodyfilled.xpm"),
      m_noteBodyEmpty("pixmaps/note-bodyempty.xpm")
{
    // Yes, this is not a mistake. Don't ask me why - Chris named those
    QString pixmapTailUpFileName("pixmaps/tail-up-%1.xpm"),
        pixmapTailDownFileName("pixmaps/tail-down-%1.xpm");

    for (unsigned int i = 0; i < 4; ++i) {
        
        m_tailsUp.push_back(new QPixmap(pixmapTailDownFileName.arg(i+1)));
        m_tailsDown.push_back(new QPixmap(pixmapTailUpFileName.arg(i+1)));
    }

    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;
    m_noteBodyHeight        = m_noteBodyEmpty.height();
    m_noteBodyWidth         = m_noteBodyEmpty.width();
    m_tailWidth             = m_tailsUp[0]->width();

    // Load rests
    m_rests.push_back(new QPixmap("pixmaps/rest-hemidemisemi.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-demisemi.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-semiquaver.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-quaver.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-crotchet.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-minim.xpm"));
    m_rests.push_back(new QPixmap("pixmaps/rest-semibreve.xpm"));
    
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
NotePixmapFactory::makeNotePixmap(Note note, bool drawTail,
                                  bool stalkGoesUp)
{
    if (note > LastNote) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : note > LastNote ("
                             << note << ")\n";
        throw -1;
    }

    bool noteHasStalk = note < Whole;

    if (noteHasStalk) {
        m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;
        readjustGeneratedPixmapHeight(note);
    }
    else {
        m_generatedPixmapHeight = m_noteBodyEmpty.height();
    }

    // X-offset at which the tail should be drawn
    unsigned int tailOffset = (note < Quarter && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask(tailOffset);

    // paint note body
    //
    QPixmap *body = (note > QuarterDotted) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    int yOffset = 0;

    if (stalkGoesUp) {

        m_p.drawPixmap (0,m_generatedPixmap->height() - body->height(), *body);
        m_pm.drawPixmap(0,m_generatedPixmap->height() - body->height(), *(body->mask()));

        yOffset = m_generatedPixmap->height() - body->height() / 2;

    } else {

        m_p.drawPixmap (0,0, *body);
        m_pm.drawPixmap(0,0, *(body->mask()));

        yOffset = body->height() / 2;
    }

    // paint stalk (if needed)
    //
    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);


#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY
    // add red dots at each corner of the pixmap
    m_p.setPen(Qt::red); m_p.setBrush(Qt::red);
    m_p.drawPoint(0,0);
    m_p.drawPoint(0,yOffset);
    m_p.drawPoint(0,m_generatedPixmap->height() - 1);
    m_p.drawPoint(m_generatedPixmap->width() - 1,0);
    m_p.drawPoint(m_generatedPixmap->width() - 1,m_generatedPixmap->height() - 1);

    m_pm.drawPoint(0,0);
    m_pm.drawPoint(0,yOffset);
    m_pm.drawPoint(0,m_generatedPixmap->height() -1);
    m_pm.drawPoint(m_generatedPixmap->width() -1,0);
    m_pm.drawPoint(m_generatedPixmap->width() -1,m_generatedPixmap->height()-1);
#endif

    // We're done - generate the returned pixmap with the right offset
    //
    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, QPoint(0, yOffset));
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
}


QCanvasPixmap
NotePixmapFactory::makeRestPixmap(Note note)
{
    switch (note) {
    case SixtyFourth:
        return QCanvasPixmap(*m_rests[0], m_pointZero);
    case ThirtySecond:
        return QCanvasPixmap(*m_rests[1], m_pointZero);
    case Sixteenth:
        return QCanvasPixmap(*m_rests[2], m_pointZero);
    case Eighth:
        return QCanvasPixmap(*m_rests[3], m_pointZero);
    case Quarter:
        return QCanvasPixmap(*m_rests[4], m_pointZero);
    case Half:
        return QCanvasPixmap(*m_rests[5], m_pointZero);
    case Whole:
        return QCanvasPixmap(*m_rests[6], m_pointZero);
    default:
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeRestPixmap() for note "
                             << note << " not yet implemented or note out of range\n";
        return QCanvasPixmap(*m_rests[0], m_pointZero);
    }
}


void
NotePixmapFactory::readjustGeneratedPixmapHeight(Note note)
{
    if (note < Quarter) {

        // readjust pixmap height according to its duration - the stalk
        // is longer for 8th, 16th, etc.. because the tail is higher
        //
        if (note == Eighth || note == EighthDotted)
            m_generatedPixmapHeight += 1;
        else if (note == Sixteenth || note == SixteenthDotted)
            m_generatedPixmapHeight += 4;
        else if (note == ThirtySecond || note == ThirtySecondDotted)
            m_generatedPixmapHeight += 9;
        else if (note == SixtyFourth || note == SixtyFourthDotted)
            m_generatedPixmapHeight += 14;
    }
}

void
NotePixmapFactory::createPixmapAndMask(unsigned int tailOffset)
{
    // create pixmap and mask
    m_generatedPixmap = new QPixmap(m_noteBodyEmpty.width() + tailOffset,
                                    m_generatedPixmapHeight);
    m_generatedMask = new QBitmap(m_generatedPixmap->width(),
                                  m_generatedPixmap->height());

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
NotePixmapFactory::tailUp(Note note) const
{
    if (note > EighthDotted) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Eighth || note == EighthDotted)
        return m_tailsUp[0];
    if (note == Sixteenth || note == SixteenthDotted)
        return m_tailsUp[1];
    if (note == ThirtySecond || note == ThirtySecondDotted)
        return m_tailsUp[2];
    if (note == SixtyFourth || note == SixtyFourthDotted)
        return m_tailsUp[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
    
}

const QPixmap*
NotePixmapFactory::tailDown(Note note) const
{
    if (note > EighthDotted) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailDown : note("
                             << note << ") > EighthDotted" << endl;
        throw -1;
        return 0;
    }
    
    if (note == Eighth || note == EighthDotted)
        return m_tailsDown[0];
    if (note == Sixteenth || note == SixteenthDotted)
        return m_tailsDown[1];
    if (note == ThirtySecond || note == ThirtySecondDotted)
        return m_tailsDown[2];
    if (note == SixtyFourth || note == SixtyFourthDotted)
        return m_tailsDown[3];
    else {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::tailUp : unknown note"
                             << endl;
        throw -1;
    }
}



void
NotePixmapFactory::drawStalk(Note note,
                             bool drawTail, bool stalkGoesUp)
{
    if (stalkGoesUp) {

        m_p.drawLine(m_noteBodyWidth - 2, m_generatedPixmapHeight - m_noteBodyHeight / 2 - 1,
                     m_noteBodyWidth - 2, 0);
        m_pm.drawLine(m_noteBodyWidth - 2, m_generatedPixmapHeight - m_noteBodyHeight / 2 - 1,
                      m_noteBodyWidth - 2, 0);
    } else {

        m_p.drawLine(0, m_noteBodyHeight / 2,
                     0, m_generatedPixmapHeight);
        m_pm.drawLine(0, m_noteBodyHeight / 2,
                      0, m_generatedPixmapHeight);
    }

    if (drawTail && note < Quarter) {
        // need to add a tail pixmap
        //
        const QPixmap *tailPixmap = 0;

        if (stalkGoesUp) {
            tailPixmap = tailUp(note);

            m_p.drawPixmap (m_noteBodyWidth - 1, 0, *tailPixmap);
            m_pm.drawPixmap(m_noteBodyWidth - 1, 0, *(tailPixmap->mask()));

        } else {

            tailPixmap = tailDown(note);

            m_p.drawPixmap (1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *tailPixmap);

            m_pm.drawPixmap(1,
                            m_generatedPixmapHeight - tailPixmap->height(),
                            *(tailPixmap->mask()));
        }

    }
}


// Keep this around just in case
Note
NotePixmapFactory::duration2note(unsigned int duration)
{
    // Very basic, very dumb.
    // Make a quantization class - how to represent dotted notes ?
    static unsigned int wholeDuration = 384;
    Note rc;
    
    if (duration == wholeDuration)
        rc = Whole;
    else if (duration == wholeDuration / 2 )
        rc = Half;
    else if (duration == wholeDuration / 4 )
        rc = Quarter;
    else if (duration == wholeDuration / 8 )
        rc = Eighth;
    else if (duration == wholeDuration / 16 )
        rc = Sixteenth;
    else if (duration == wholeDuration / 32 )
        rc = ThirtySecond;
    else if (duration == wholeDuration / 64 )
        rc = SixtyFourth;

    kdDebug(KDEBUG_AREA) << "NotePixmapFactory::duration : duration = "
                         << duration << " - rc = " << rc << "\n";

    return rc;
}


QPoint
NotePixmapFactory::m_pointZero;


//////////////////////////////////////////////////////////////////////


ChordPixmapFactory::ChordPixmapFactory(const Staff &s)
    : m_referenceStaff(s)
{
}


QCanvasPixmap
ChordPixmapFactory::makeChordPixmap(const chordpitches &pitches,
                                    Note note, bool drawTail,
                                    bool stalkGoesUp)
{
    int highestNote = m_referenceStaff.pitchYOffset(pitches[pitches.size() - 1]),
        lowestNote = m_referenceStaff.pitchYOffset(pitches[0]);


    bool noteHasStalk = note < Whole;

    m_generatedPixmapHeight = highestNote - lowestNote + m_noteBodyHeight;

    if (noteHasStalk)
        m_generatedPixmapHeight += Staff::stalkLen;

    kdDebug(KDEBUG_AREA) << "m_generatedPixmapHeight : " << m_generatedPixmapHeight << endl
                         << "highestNote : " << highestNote << " - lowestNote : " << lowestNote << endl;
    

    readjustGeneratedPixmapHeight(note);

    // X-offset at which the tail should be drawn
    unsigned int tailOffset = (note < Quarter && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask(tailOffset);

    // paint note bodies

    // set mask painter RasterOp to Or
    m_pm.setRasterOp(Qt::OrROP);

    QPixmap *body = (note > QuarterDotted) ? &m_noteBodyEmpty : &m_noteBodyFilled;

    if (stalkGoesUp) {
        int offset = m_generatedPixmap->height() - body->height() - highestNote;
        
        for (int i = pitches.size() - 1; i >= 0; --i) {
            m_p.drawPixmap (0, m_referenceStaff.pitchYOffset(pitches[i]) + offset, *body);
            m_pm.drawPixmap(0, m_referenceStaff.pitchYOffset(pitches[i]) + offset, *(body->mask()));
        }
        
    } else {
        int offset = lowestNote;
        for (unsigned int i = 0; i < pitches.size(); ++i) {
            m_p.drawPixmap (0, m_referenceStaff.pitchYOffset(pitches[i]) - offset, *body);
            m_pm.drawPixmap(0, m_referenceStaff.pitchYOffset(pitches[i]) - offset, *(body->mask()));
        }
    }

    // restore mask painter RasterOp to Copy
    m_pm.setRasterOp(Qt::CopyROP);

    if (noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    QCanvasPixmap notePixmap(*m_generatedPixmap, m_pointZero);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;

}

