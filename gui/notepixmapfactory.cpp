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

NotePixmapFactory::NotePixmapFactory()
    : m_generatedPixmapHeight(0),
      m_noteBodyHeight(0),
      m_tailWidth(0),
      m_noteBodyFilled("pixmaps/note-bodyfilled.xpm"),
      m_noteBodyEmpty("pixmaps/note-bodyempty.xpm"),
      m_tailsUp(4),
      m_tailsDown(4)
{
    for(unsigned int i = 0; i < m_tailsUp.size(); ++i) {
        char pixmapTailUpFileName[256],
            pixmapTailDownFileName[256];

        sprintf(pixmapTailUpFileName, "pixmaps/tail-up-%u.xpm", i + 1);
        sprintf(pixmapTailDownFileName, "pixmaps/tail-down-%u.xpm", i + 1);

        // Yes, this is not a mistake. Don't ask me why - Chris named those
        m_tailsUp[i] = new QPixmap(pixmapTailDownFileName);
        m_tailsDown[i] = new QPixmap(pixmapTailUpFileName);
    }

    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;
    m_noteBodyHeight        = m_noteBodyEmpty.height();
    m_noteBodyWidth         = m_noteBodyEmpty.width();
    m_tailWidth             = m_tailsUp[0]->width();

}

QPixmap
NotePixmapFactory::makeNotePixmap(unsigned int duration, bool drawTail, bool stalkGoesUp)
{
    if(duration > 6) {
        kdDebug(KDEBUG_AREA) << "NotePixmapFactory::makeNotePixmap : duration > 6 ("
                             << duration << ")\n";
        throw -1;
    }

    m_generatedPixmapHeight = m_noteBodyEmpty.height() / 2 + Staff::stalkLen;

    unsigned int tailOffset = (duration > 2 && stalkGoesUp) ? m_tailWidth : 0;

    if(duration > 3) {

        // readjust pixmap height according to its duration
        m_generatedPixmapHeight += m_tailsUp[duration - 3]->height() - (12 + duration);

    }

    // create pixmap and mask
    QPixmap note(m_noteBodyEmpty.width() + tailOffset, m_generatedPixmapHeight);
    QBitmap noteMask(note.width(), note.height());

    // clear up pixmap and mask
    noteMask.fill(Qt::color0);
    note.fill();

    // initiate painting
    m_p.begin(&note);
    m_pm.begin(&noteMask);
    m_p.setPen(Qt::black); m_p.setBrush(Qt::black);
    m_pm.setPen(Qt::white); m_pm.setBrush(Qt::white);

    // paint note body

    QPixmap *body = (duration < 2) ? &m_noteBodyEmpty : &m_noteBodyFilled;
    bool noteHasStalk = duration > 0;

    if(stalkGoesUp) {

        m_p.drawPixmap (0,note.height() - body->height(), *body);
        m_pm.drawPixmap(0,note.height() - body->height(), *(body->mask()));

    } else {

        m_p.drawPixmap (0,0, *body);
        m_pm.drawPixmap(0,0, *(body->mask()));
    }

    if(noteHasStalk)
        drawStalk(duration, drawTail, stalkGoesUp);

    m_p.end();
    m_pm.end();

    note.setMask(noteMask);

    return note;
}

void
NotePixmapFactory::drawStalk(unsigned int duration, bool drawTail, bool stalkGoesUp)
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

    if(drawTail && duration > 2) {

        QPixmap *tailPixmap = 0;

        if(stalkGoesUp) {
            tailPixmap = m_tailsUp[duration - 3];

            m_p.drawPixmap (m_noteBodyWidth, 0, *tailPixmap);
            m_pm.drawPixmap(m_noteBodyWidth, 0, *(tailPixmap->mask()));

        } else {

            tailPixmap = m_tailsDown[duration - 3];

            m_p.drawPixmap (1, m_generatedPixmapHeight - tailPixmap->height(), *tailPixmap);
            m_pm.drawPixmap(1, m_generatedPixmapHeight - tailPixmap->height(), *(tailPixmap->mask()));
        }

    }
}

