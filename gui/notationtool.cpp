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

#include <kmessagebox.h>
#include <klocale.h>

#include "Event.h"
#include "NotationTypes.h"
#include "TrackNotationHelper.h"

#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "notationview.h"
#include "staffline.h"
#include "qcanvassimplesprite.h"

#include "rosedebug.h"

using Rosegarden::Accidental;
using Rosegarden::NoAccidental;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

NotationTool::NotationTool(NotationView& view)
    : m_parentView(view)
{
    m_parentView.setCanvasCursor(Qt::arrowCursor);
}

NotationTool::~NotationTool()
{
}

// void NotationTool::handleMouseDblClick(int height, int staffNo,
//                                        const QPoint &eventPos,
//                                        NotationElement* e)
// {
//     handleMousePress(height, staffNo, eventPos, e);
// }

void NotationTool::handleMouseMove(QMouseEvent*)
{
}

void NotationTool::handleMouseRelease(QMouseEvent*)
{
}

//------------------------------

NoteInserter::NoteInserter(Rosegarden::Note::Type type,
                           unsigned int dots,
                           NotationView& view)
    : NotationTool(view),
      m_noteType(type),
      m_noteDots(dots)
{
    m_parentView.setCanvasCursor(Qt::crossCursor);
}

NoteInserter::~NoteInserter()
{
}

void    
NoteInserter::handleMousePress(int height, int staffNo,
                               const QPoint &eventPos,
                               NotationElement*)
{
    if (height == StaffLine::NoHeight || staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote(eventPos.x(), tsig, clef, key, staffNo);

    //!!! Could be nicer! Likewise the other inserters.

    if (closestNote ==
        m_parentView.getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }


    kdDebug(KDEBUG_AREA) << "NoteInserter::handleMousePress() : accidental = "
                         << m_accidental << endl;

    int pitch = Rosegarden::NotationDisplayPitch(height, m_accidental).
        getPerformancePitch(clef ? Clef(*clef) : Clef::DefaultClef,
                            key ? Rosegarden::Key(*key) :
                            Rosegarden::Key::DefaultKey);

    // We are going to modify the document so mark it as such
    //
    m_parentView.getDocument()->setModified();

    Note note(m_noteType, m_noteDots);
    SegmentNotationHelper nt(m_parentView.getStaff(staffNo)->getSegment());

    timeT time = (*closestNote)->getAbsoluteTime();
    timeT endTime = time + note.getDuration(); //???
    Event *newEvent = doInsert(nt, time, note, pitch, m_accidental);

    m_parentView.redoLayout(staffNo, time, endTime);
    m_parentView.setSingleSelectedEvent(staffNo, newEvent);
}

Event *NoteInserter::doInsert(SegmentNotationHelper& nt,
			      Rosegarden::timeT absTime,
			      const Note& note, int pitch,
			      Accidental accidental)
{
    Segment::iterator i = nt.insertNote(absTime, note, pitch, accidental);

    if (i != nt.segment().end()) {
	return (*i);
    } else {
	return 0;
    }
}

void NoteInserter::setAccidental(Rosegarden::Accidental accidental)
{
    m_accidental = accidental;
}

Rosegarden::Accidental NoteInserter::m_accidental = NoAccidental;

//------------------------------

RestInserter::RestInserter(Rosegarden::Note::Type type,
                           unsigned int dots, NotationView& view)
    : NoteInserter(type, dots, view)
{
}

Event *RestInserter::doInsert(SegmentNotationHelper& nt,
			      Rosegarden::timeT absTime,
			      const Note& note, int,
			      Accidental)
{
    Segment::iterator i = nt.insertRest(absTime, note);

    if (i != nt.segment().end()) {
	return (*i);
    } else {
	return 0;
    }
}

//------------------------------

ClefInserter::ClefInserter(std::string clefType, NotationView& view)
    : NotationTool(view),
      m_clef(clefType)
{
    m_parentView.setCanvasCursor(Qt::crossCursor);
}
    
void ClefInserter::handleMousePress(int, int staffNo,
                                    const QPoint &eventPos,
                                    NotationElement*)
{
    Event *tsig = 0, *clef = 0, *key = 0;

    if (staffNo < 0) return;

    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote
        (eventPos.x(), tsig, clef, key, staffNo, 100);

    if (closestNote ==
        m_parentView.getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();
    SegmentNotationHelper nt
        (m_parentView.getStaff(staffNo)->getSegment());

    m_parentView.getDocument()->setModified();

    Segment::iterator i = nt.insertClef(time, m_clef);
    Event *newEvent = 0;
    if (i != nt.segment().end()) newEvent = *i;

    m_parentView.redoLayout(staffNo, time, time + 1);
    m_parentView.setSingleSelectedEvent(staffNo, newEvent);
}


//------------------------------

NotationEraser::NotationEraser(NotationView& view)
    : NotationTool(view),
      m_collapseRest(false)
{
    m_parentView.setCanvasCursor(Qt::pointingHandCursor);
}

void NotationEraser::handleMousePress(int, int staffNo,
                                      const QPoint&,
                                      NotationElement* element)
{
    bool needLayout = false;
    if (!element || staffNo < 0) return;

    SegmentNotationHelper nt
        (m_parentView.getStaff(staffNo)->getSegment());

    timeT absTime = 0;

    if (element->isNote()) {

        absTime = element->getAbsoluteTime();
        nt.deleteNote(element->event(), m_collapseRest);
        needLayout = true;

    } else if (element->isRest()) {

        absTime = element->getAbsoluteTime();
        nt.deleteRest(element->event());
        needLayout = true;

    } else {
        // we don't know what it is
        KMessageBox::sorry(0, "Not Implemented Yet");

    }
    
    if (needLayout) {
        m_parentView.getDocument()->setModified();
        m_parentView.redoLayout(staffNo, absTime, absTime);
    }
}

//------------------------------

NotationSelector::NotationSelector(NotationView& view)
    : NotationTool(view),
      m_selectionRect(new QCanvasRectangle(m_parentView.canvas())),
      m_updateRect(false),
      m_clickedStaff(-1),
      m_clickedElement(0)
{
    m_selectionRect->hide();
    m_selectionRect->setPen(Qt::blue);

    connect(&m_parentView, SIGNAL(usedSelection()),
            this,          SLOT(hideSelection()));
}

NotationSelector::~NotationSelector()
{
    delete m_selectionRect;
    m_parentView.canvas()->update();
}

void NotationSelector::handleMousePress(int, int staffNo,
                                        const QPoint& p,
                                        NotationElement *element)
{
    m_clickedStaff = staffNo;
    m_clickedElement = element;

    m_selectionRect->setX(p.x());
    m_selectionRect->setY(p.y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;

    //m_parentView.setCursorPosition(p.x());
}

void NotationSelector::handleMouseMove(QMouseEvent* e)
{
    if (!m_updateRect) return;

    int w = int(e->x() - m_selectionRect->x());
    int h = int(e->y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    m_selectionRect->setSize(w,h);

    m_parentView.canvas()->update();
}

void NotationSelector::handleMouseRelease(QMouseEvent*)
{
    m_updateRect = false;
    setViewCurrentSelection();

    // If we didn't drag out a meaningful area, but _did_ click on
    // an individual event, then select just that event
    
    if (m_selectionRect->width()  > -2 &&
	m_selectionRect->width()  <  2 &&
	m_selectionRect->height() > -2 &&
	m_selectionRect->height() <  2 &&
	m_clickedElement != 0	       &&
	m_clickedStaff >= 0) {

	m_parentView.setSingleSelectedEvent
	    (m_clickedStaff, m_clickedElement->event());
    }
}

void NotationSelector::hideSelection()
{
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_parentView.canvas()->update();
}


EventSelection* NotationSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

//    kdDebug(KDEBUG_AREA) << "Selection x,y: " << m_selectionRect->x() << ","
//			 << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -2 &&
	m_selectionRect->width()  <  2 &&
        m_selectionRect->height() > -2 &&
        m_selectionRect->height() <  2) return 0;

    double middleY = m_selectionRect->y() + m_selectionRect->height()/2;
    int staffNo = m_parentView.findClosestStaff(middleY);
    if (staffNo < 0) return 0;
    Segment& originalSegment = m_parentView.getStaff(staffNo)->getSegment();
    
    EventSelection* selection = new EventSelection(originalSegment);

    QCanvasItemList itemList = m_selectionRect->collisions(true);
    QCanvasItemList::Iterator it;

    QRect rect = m_selectionRect->rect().normalize();

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        QCanvasNotationSprite *sprite = 0;
        
        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(item))) {

            if (!rect.contains(int(item->x()), int(item->y()), true)) {

                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Skipping item not really in selection rect\n";
                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
                continue;
            } else {
                
                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Item in rect : Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
            }
            
            NotationElement &el = sprite->getNotationElement();

            selection->addEvent(el.event());

//             kdDebug(KDEBUG_AREA) << "Selected event : \n";
//             el.event()->dump(std::cerr);
        }
        
    }

    return (selection->getAddedEvents() > 0) ? selection : 0;
}

void NotationSelector::setViewCurrentSelection()
{
    EventSelection* selection = getSelection();
    m_parentView.setCurrentSelection(selection);
}

//------------------------------

NotationSelectionPaster::NotationSelectionPaster(NotationView& parent,
                                                 EventSelection& es)
    : NotationTool(parent),
      m_selection(es)
{
    m_parentView.setCanvasCursor(Qt::crossCursor);
}

NotationSelectionPaster::~NotationSelectionPaster()
{
}

void NotationSelectionPaster::handleMousePress(int, int staffNo,
                                               const QPoint &eventPos,
                                               NotationElement*)
{
    kdDebug(KDEBUG_AREA) << "NotationSelectionPaster::handleMousePress : staffNo = "
                         << staffNo
                         << "event pos : "
                         << eventPos.x() << "," << eventPos.y() << endl;

    if (staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote(eventPos.x(), tsig, clef, key, staffNo);

    if (closestNote ==
        m_parentView.getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();

    Segment& segment = m_parentView.getStaff(staffNo)->getSegment();

    if (m_selection.pasteToSegment(segment, time)) {

        m_parentView.redoLayout(staffNo,
                                0,
                                time + m_selection.getTotalDuration() + 1);

    } else {
        
        m_parentView.slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
    
    //m_parentView.slotStatusHelpMsg(i18n("Ready."));

}

