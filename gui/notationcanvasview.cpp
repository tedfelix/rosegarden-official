
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
#include "notationelement.h"
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

        emit hoveredOverNoteChange(QString::null);

        m_currentHighlightedLine = 0;

    } else {

        QCanvasItemList::Iterator it;

        for (it = itemList.begin(); it != itemList.end(); ++it) {

            QCanvasItem *item = *it;

            StaffLine *staffLine = 0;
            QCanvasSimpleSprite *noteSprite = 0;
            
            if ((staffLine = dynamic_cast<StaffLine*>(item))) {
                m_currentHighlightedLine = staffLine;

                m_lastYPosNearStaff = e->y();

                //int pitch = getPitchForLine(m_currentHighlightedLine);
                QString noteName = getNoteNameForLine(m_currentHighlightedLine);
//                 kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMouseMoveEvent() : "
//                                      << noteName << endl;

                emit hoveredOverNoteChange(noteName);

            } else if ((noteSprite = dynamic_cast<QCanvasSimpleSprite*>(item))) {
                // TODO : fetch element's absolute time - can't do that right
                // now because the noteSprite doesn't have a link back to
                // the event it represents
//                 unsigned int elAbsoluteTime = el->event()->getAbsoluteTime();
//                 kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMouseMoveEvent() : abs. Time : " << elAbsoluteTime << endl;
//                 emit hoveredOverAbsoluteTimeChange(elAbsoluteTime);

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
        handleClick(m_currentHighlightedLine, e->pos());

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
NotationCanvasView::handleClick(const StaffLine *line, const QPoint &pos)
{
    int h = line->getHeight();

    kdDebug(KDEBUG_AREA) << "NotationCanvasView::insertNote() : insertNote at height " << h << endl;

    emit noteClicked(h, pos);
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
/*!
int
NotationCanvasView::getPitchForLine(const StaffLine *line)
{
    int h = line->getHeight();

    //!!! TODO -- take clef & key into account, and then accidental
    int pitch = NotationDisplayPitch(h, NoAccidental).
        getPerformancePitch(Clef::DefaultClef, ::Key::DefaultKey);

    return pitch;
}
*/

//??? ew... can't be doing this here can we? don't have the right info
//using Rosegarden;

QString
NotationCanvasView::getNoteNameForLine(const StaffLine *line)
{
    int h = line->getHeight();

    //!!! TODO -- take clef & key into account, and then accidental
    std::string noteName = Rosegarden::NotationDisplayPitch(h,
                                                       Rosegarden::NoAccidental).
        getAsString(Rosegarden::Clef::DefaultClef,
                    Rosegarden::Key::DefaultKey);

    return QString(noteName.c_str());
}
