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

#include <kdebug.h>

#include "rosegardenguiview.h"
#include "notepixmapfactory.h"
#include "staff.h"
#include "pitchtoheight.h"

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

    for(unsigned int i = 0; i < 4; ++i) {
        
        m_tailsUp.push_back(new QPixmap(pixmapTailDownFileName.arg(i+1)));
        m_tailsDown.push_back(new QPixmap(pixmapTailUpFileName.arg(i+1)));
    }

    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;
    m_noteBodyHeight        = m_noteBodyEmpty.height();
    m_noteBodyWidth         = m_noteBodyEmpty.width();
    m_tailWidth             = m_tailsUp[0]->width();

}

NotePixmapFactory::~NotePixmapFactory()
{
    for(unsigned int i = 0; i < m_tailsUp.size(); ++i) {
        delete m_tailsUp[i];
        delete m_tailsDown[i];
    }
}



QPixmap
NotePixmapFactory::makeNotePixmap(unsigned int duration, bool drawTail,
                                  bool stalkGoesUp)
{
    Note note = duration2note(duration);

    if(note > SixtyFourth) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : note > 6 ("
                             << note << ")\n";
        throw -1;
    }

    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;

    readjustGeneratedPixmapHeight(note);

    // X-offset at which the tail should be drawn
    unsigned int tailOffset = (note > 2 && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask(tailOffset);

    // paint note body

    QPixmap *body = (note < Quarter) ? &m_noteBodyEmpty : &m_noteBodyFilled;
    bool noteHasStalk = note > Whole;

    if(stalkGoesUp) {

        m_p.drawPixmap (0,m_generatedPixmap->height() - body->height(), *body);
        m_pm.drawPixmap(0,m_generatedPixmap->height() - body->height(), *(body->mask()));

    } else {

        m_p.drawPixmap (0,0, *body);
        m_pm.drawPixmap(0,0, *(body->mask()));
    }

    if(noteHasStalk)
        drawStalk(note, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    QPixmap notePixmap(*m_generatedPixmap);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;
}

void
NotePixmapFactory::readjustGeneratedPixmapHeight(NotePixmapFactory::Note note)
{
    if(note > Eighth) {

        // readjust pixmap height according to its duration - the stalk
        // is longer for 8th, 16th, etc.. because the tail is higher
        //
        m_generatedPixmapHeight += m_tailsUp[note - 3]->height() - (12 + note);

    }
}

void
NotePixmapFactory::createPixmapAndMask(unsigned int tailOffset)
{
    // create pixmap and mask
    m_generatedPixmap = new QPixmap(m_noteBodyEmpty.width() + tailOffset,
                                    m_generatedPixmapHeight);
    m_generatedMask = new QBitmap(m_generatedPixmap->width(), m_generatedPixmap->height());

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    m_p.begin(m_generatedPixmap);
    m_pm.begin(m_generatedMask);
    m_p.setPen(Qt::black); m_p.setBrush(Qt::black);
    m_pm.setPen(Qt::white); m_pm.setBrush(Qt::white);
}


QPixmap
NotePixmapFactory::makeChordPixmap(const chordpitches &pitches,
                                   unsigned int duration, bool drawTail,
                                   bool stalkGoesUp)
{
    PitchToHeight& pitchToHeight(PitchToHeight::instance());

    int highestNote = pitchToHeight[pitches[pitches.size() - 1]],
        lowestNote = pitchToHeight[pitches[0]];
    
    Note note = duration2note(duration);

    m_generatedPixmapHeight = highestNote - lowestNote + m_noteBodyHeight + Staff::stalkLen;;

    kdDebug(KDEBUG_AREA) << "m_generatedPixmapHeight : " << m_generatedPixmapHeight << endl
                         << "highestNote : " << highestNote << " - lowestNote : " << lowestNote << endl;
    

    readjustGeneratedPixmapHeight(note);

    // X-offset at which the tail should be drawn
    unsigned int tailOffset = (note > Quarter && stalkGoesUp) ? m_tailWidth : 0;

    createPixmapAndMask(tailOffset);

    // paint note bodies

    // set mask painter RasterOp to Or
    m_pm.setRasterOp(Qt::OrROP);

    QPixmap *body = (note < Quarter) ? &m_noteBodyEmpty : &m_noteBodyFilled;
    bool noteHasStalk = note > Whole;

    if(stalkGoesUp) {
        int offset = m_generatedPixmap->height() - body->height() - highestNote;
        
        for(int i = pitches.size() - 1; i >= 0; --i) {
            m_p.drawPixmap (0, pitchToHeight[pitches[i]] + offset, *body);
            m_pm.drawPixmap(0, pitchToHeight[pitches[i]] + offset, *(body->mask()));
        }
        
    } else {
        int offset = lowestNote;
        for(unsigned int i = 0; i < pitches.size(); ++i) {
            m_p.drawPixmap (0,pitchToHeight[pitches[i]] - offset, *body);
            m_pm.drawPixmap(0,pitchToHeight[pitches[i]] - offset, *(body->mask()));
        }
    }

    // restore mask painter RasterOp to Copy
    m_pm.setRasterOp(Qt::CopyROP);

    if(noteHasStalk) // disable for now
        drawStalk(note, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    QPixmap notePixmap(*m_generatedPixmap);
    QBitmap mask(*m_generatedMask);
    notePixmap.setMask(mask);

    delete m_generatedPixmap;
    delete m_generatedMask;

    return notePixmap;

}


void
NotePixmapFactory::drawStalk(NotePixmapFactory::Note note,
                             bool drawTail, bool stalkGoesUp)
{
    if(stalkGoesUp) {

        m_p.drawLine(m_noteBodyWidth - 1, m_generatedPixmapHeight - m_noteBodyHeight / 2 - 1,
                     m_noteBodyWidth - 1, 0);
        m_pm.drawLine(m_noteBodyWidth - 1, m_generatedPixmapHeight - m_noteBodyHeight / 2 - 1,
                      m_noteBodyWidth - 1, 0);
    } else {

        m_p.drawLine(0, m_noteBodyHeight / 2,
                     0, m_generatedPixmapHeight);
        m_pm.drawLine(0, m_noteBodyHeight / 2,
                      0, m_generatedPixmapHeight);
    }

    if(drawTail && note > 2) {
        // need to add a tail pixmap
        //
        QPixmap *tailPixmap = 0;

        if(stalkGoesUp) {
            tailPixmap = m_tailsUp[note - 3];

            m_p.drawPixmap (m_noteBodyWidth, 0, *tailPixmap);
            m_pm.drawPixmap(m_noteBodyWidth, 0, *(tailPixmap->mask()));

        } else {

            tailPixmap = m_tailsDown[note - 3];

            m_p.drawPixmap (1, m_generatedPixmapHeight - tailPixmap->height(), *tailPixmap);
            m_pm.drawPixmap(1, m_generatedPixmapHeight - tailPixmap->height(), *(tailPixmap->mask()));
        }

    }
}

NotePixmapFactory::Note
NotePixmapFactory::duration2note(unsigned int duration)
{
    // Very basic, very dumb.
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
