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

#include "notationcanvasview.h"
#include "notationelement.h"
#include "qcanvasgroupableitem.h"
#include "qcanvasitemgroup.h"
#include "qcanvassimplesprite.h"
#include "staffline.h"
#include "NotationTypes.h"
#include "notationstaff.h"

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
NotationCanvasView::contentsMouseReleaseEvent(QMouseEvent *e)
{
    emit mouseRelease(e);
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
            QCanvasNotationSprite *noteSprite = 0;
            
            if ((staffLine = dynamic_cast<StaffLine*>(item))) {
                m_currentHighlightedLine = staffLine;

                m_lastYPosNearStaff = e->y();

                //int pitch = getPitchForLine(m_currentHighlightedLine);
                QString noteName = getNoteNameForLine(m_currentHighlightedLine);
//                 kdDebug(KDEBUG_AREA) << "NotationCanvasView::contentsMouseMoveEvent() : "
//                                      << noteName << endl;

                emit hoveredOverNoteChange(noteName);

            } else if ((noteSprite = dynamic_cast<QCanvasNotationSprite*>(item))) {
                unsigned int elAbsoluteTime = noteSprite->getNotationElement().getAbsoluteTime();
                emit hoveredOverAbsoluteTimeChange(elAbsoluteTime);
            }
            
        }
    }

    // if(tracking) ??
    emit mouseMove(e);
}

void
NotationCanvasView::contentsMousePressEvent(QMouseEvent *e)
{
    kdDebug(KDEBUG_AREA) << "mousepress" << endl;

    if (!m_currentHighlightedLine) {
        handleMousePress(0, -1, e->pos());
        return;
    }
    
    kdDebug(KDEBUG_AREA) << "mousepress : m_currentHighlightedLine != 0 - inserting note - staff pitch : "
                         << "(no longer relevant)" << endl;

    // Check if we haven't actually clicked on a sprite
    //
    QCanvasItemList itemList = canvas()->collisions(e->pos());

    if(itemList.isEmpty()) { // click was not on an item
        kdDebug(KDEBUG_AREA) << "mousepress : Not on an item" << endl;
        emit handleMousePress(0, -1, e->pos());
        return;
    }

    QCanvasItemList::Iterator it;
    QCanvasNotationSprite *sprite = 0;

    // Get the pitch were the click occurred
    Rosegarden::Key key;
    Rosegarden::Clef clef;

    int clickPitch = Rosegarden::NotationDisplayPitch(m_currentHighlightedLine->getHeight(), Rosegarden::NoAccidental).
        getPerformancePitch(clef, key);

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(item))) {
            NotationElement &el = sprite->getNotationElement();

            if (el.isNote()) { // make sure we get the right note

                long eventPitch = 0;
                el.event()->get<Rosegarden::Int>("pitch", eventPitch);
                if (eventPitch == clickPitch) break;

            } else { // it's not a note, so we don't care about checking the pitch

                break;

            }
            
        }
        
    }

    int staffNo = -1;

    // Find staff on which the click occurred
    QCanvasItemGroup *group = m_currentHighlightedLine->group();
    NotationStaff *staff = dynamic_cast<NotationStaff*>(group);

    if (staff)
        staffNo = staff->getId();
    else
        kdDebug(KDEBUG_AREA) << "NotationCanvasView::handleMousePress() : big problem - couldn't find staff for staff line\n";


    if (sprite)
        handleMousePress(m_currentHighlightedLine, staffNo,
                         e->pos(), &(sprite->getNotationElement()));
    else
        handleMousePress(m_currentHighlightedLine, staffNo, e->pos());
}


// Used for a note-shaped cursor - leaving around just in case
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
NotationCanvasView::handleMousePress(const StaffLine *line,
                                     int staffNo,
                                     const QPoint &pos,
                                     NotationElement *el)
{
    int h = line ? line->getHeight() : StaffLine::NoHeight;

    kdDebug(KDEBUG_AREA) << "NotationCanvasView::handleMousePress() at height " << h << endl;

    emit itemClicked(h, staffNo, pos, el);
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
