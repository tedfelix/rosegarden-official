
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
      m_dualArrow(new QCanvasSimpleSprite("pixmaps/9/dual-arrow.xpm", canvas())),
      m_lastYPosNearStaff(0)
{
//     setCurrentNotePixmap(m_notePixmapFactory.makeNotePixmap(Note::WholeNote));

    viewport()->setMouseTracking(true);
}

NotationCanvasView::~NotationCanvasView()
{
    m_dualArrow->hide();
    delete m_dualArrow;
}


void
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent*)
{
    canvas()->update();
}

void
NotationCanvasView::contentsMouseMoveEvent(QMouseEvent *e)
{
    bool needUpdate = false;
    
//     if (m_currentHighlightedLine)
//         m_currentHighlightedLine->setHighlighted(false);

    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty() && posIsTooFarFromStaff(e->pos())) {

        m_currentHighlightedLine = 0;
        m_dualArrow->hide();

        needUpdate = true;

    } else {

        QCanvasItemList::Iterator it;

        for (it = itemList.begin(); it != itemList.end(); ++it) {

            QCanvasItem *item = *it;

            StaffLine *staffLine = 0;
    
            if ((staffLine = dynamic_cast<StaffLine*>(item))) {
                m_currentHighlightedLine = staffLine;

                // the offset is needed to avoid that the pixmap is
                // hidden by the mouse pointer
                m_dualArrow->setX(e->x() + 10);

                QPoint point = m_currentHighlightedLine->startPoint();

                // TODO : change this when Chris finishes pitch<->y stuff
                //
                m_dualArrow->setY(point.y() + m_currentHighlightedLine->y());

                m_dualArrow->show();
                m_lastYPosNearStaff = e->y();

                needUpdate = true;
                break;
            }
        }
    }
    
    if (needUpdate)
        canvas()->update();
}

void
NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "mousepress" << endl;

    if (m_currentHighlightedLine && m_dualArrow->visible()) {
        kdDebug(KDEBUG_AREA) << "mousepress : m_currentHighlightedLine != 0 - inserting note - staff pitch : "
                             << "(no longer relevant)" << endl;
//!!!                             << m_currentHighlightedLine->associatedPitch() << endl;
        insertNote(m_currentHighlightedLine, e->pos());

        return;
    }
    
#if 0 // all this should be removed
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
    int h = line->getHeight();

    //!!! TODO -- take clef & key into account, and then accidental
    int pitch = NotationDisplayPitch(h, NoAccidental).
        getPerformancePitch(Clef::DefaultClef, ::Key::DefaultKey);

    kdDebug(KDEBUG_AREA) << "NotationCanvasView::insertNote() : insertNote at height " << h << " ( = pitch "
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
