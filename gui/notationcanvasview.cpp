
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

#include "notationcanvasview.h"
#include "qcanvasgroupableitem.h"
#include "qcanvasitemgroup.h"
#include "qcanvassimplesprite.h"
#include "staffline.h"
#include "NotationTypes.h"

#include "rosedebug.h"

NotationCanvasView::NotationCanvasView(QCanvas *viewing, QWidget *parent,
                                       const char *name, WFlags f)
    : QCanvasView(viewing, parent, name, f),
      m_currentHighlightedLine(0),
      //!!! resolution from staff! or get the NPF to make arrow, or whatever
      m_lastYPosNearStaff(0)
{
    viewport()->setMouseTracking(true);
}

NotationCanvasView::~NotationCanvasView()
{
}


void
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent*)
{
    canvas()->update();
}

void
NotationCanvasView::contentsMouseMoveEvent(QMouseEvent *e)
{
    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty() && posIsTooFarFromStaff(e->pos())) {

        emit currentPitchChange(-1);

        m_currentHighlightedLine = 0;

    } else {

        QCanvasItemList::Iterator it;

        for (it = itemList.begin(); it != itemList.end(); ++it) {

            QCanvasItem *item = *it;

            StaffLine *staffLine = 0;
    
            if ((staffLine = dynamic_cast<StaffLine*>(item))) {
                m_currentHighlightedLine = staffLine;

                m_lastYPosNearStaff = e->y();

                int pitch = getPitchForLine(m_currentHighlightedLine);
                
                emit currentPitchChange(pitch);

                break;
            }
        }
    }
    
}

void
NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "mousepress" << endl;

    if (m_currentHighlightedLine) {
        kdDebug(KDEBUG_AREA) << "mousepress : m_currentHighlightedLine != 0 - inserting note - staff pitch : "
                             << "(no longer relevant)" << endl;
//!!!                             << m_currentHighlightedLine->associatedPitch() << endl;
        insertNote(m_currentHighlightedLine, e->pos());

        return;
    }
    
#if 0 // old version : collision detection. Keep around just in case.
    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty()) { // click was not on an item
        kdDebug(KDEBUG_AREA) << "mousepress : Not on an item" << endl;
        return;
    }

    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;

        StaffLine *staffLine;
    
        if ((staffLine = dynamic_cast<StaffLine*>(item))) {
            kdDebug(KDEBUG_AREA) << "mousepress : on a staff Line - insert note - staff pitch : "
                             << "(no longer relevant)" << endl;
//!!!                                 << staffLine->associatedPitch() << endl;
            insertNote(staffLine, e->pos());
            // staffLine->setPen(blue); - debug feedback to confirm which line what clicked on
        
            return;
        }
    }
#endif
    
    canvas()->update();
}


// void
// NotationCanvasView::currentNoteChanged(Note::Type note)
// {
//     QCanvasPixmap notePixmap = m_notePixmapFactory.makeNotePixmap(note);
//     setCurrentNotePixmap(notePixmap);
// }

// void
// NotationCanvasView::setCurrentNotePixmap(QCanvasPixmap note)
// {
//     delete m_currentNotePixmap;
//     m_currentNotePixmap = new QCanvasSimpleSprite(&note, canvas());
// }


void
NotationCanvasView::insertNote(const StaffLine *line, const QPoint &pos)
{
    int pitch = getPitchForLine(line);

    kdDebug(KDEBUG_AREA) << "NotationCanvasView::insertNote() : insertNote at height "
                         << line->getHeight() << " ( = pitch "
			 << pitch << " )" << endl;
    //!!! TODO -- get the right pitch
    emit noteInserted(pitch, pos);
}

bool
NotationCanvasView::posIsTooFarFromStaff(const QPoint &pos)
{
    // return true if pos.y is more than 10 pixels away from
    // the last pos for which a collision was detected
    //
    return (pos.y() > m_lastYPosNearStaff) ?
        (pos.y() - m_lastYPosNearStaff) > 10 :
        (m_lastYPosNearStaff - pos.y()) > 10;
    
}

int
NotationCanvasView::getPitchForLine(const StaffLine *line)
{
    int h = line->getHeight();

    //!!! TODO -- take clef & key into account, and then accidental
    int pitch = NotationDisplayPitch(h, NoAccidental).
        getPerformancePitch(Clef::DefaultClef, ::Key::DefaultKey);

    return pitch;
}
