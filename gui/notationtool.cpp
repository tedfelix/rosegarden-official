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

#include <qpopupmenu.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>

#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "SegmentNotationHelper.h"

#include "eventselection.h"
#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "notationview.h"
#include "staffline.h"
#include "qcanvassimplesprite.h"

#include "notationcommands.h"

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

NotationTool::NotationTool(const QString& menuName, NotationView* view)
    : QObject(0),
      m_menuName(menuName),
      m_parentView(view),
      m_menu(0)
{
    m_parentView->setCanvasCursor(Qt::arrowCursor);
    m_parentView->setPositionTracking(false);
}

NotationTool::~NotationTool()
{
    kdDebug(KDEBUG_AREA) << "NotationTool::~NotationTool()\n";

//     delete m_menu;
//     m_parentView->factory()->removeClient(this);
//    m_instance = 0;
}

void NotationTool::finalize()
{
    m_parentView->setCanvasCursor(Qt::arrowCursor);
    m_parentView->setPositionTracking(false);
}

void NotationTool::createMenu(const QString& rcFileName)
{
    setXMLFile(rcFileName);
    m_parentView->factory()->addClient(this);

    QWidget* tmp =  m_parentView->factory()->container(m_menuName, this);

    m_menu = dynamic_cast<QPopupMenu*>(tmp);
}

void NotationTool::handleMousePress(int height, int staffNo,
                                    QMouseEvent* e,
                                    NotationElement* el)
{
    kdDebug(KDEBUG_AREA) << "NotationTool::handleMousePress : mouse button = "
                         << e->button() << endl;

    switch (e->button()) {

    case Qt::LeftButton:
        handleLeftButtonPress(height, staffNo, e, el);
        break;

    case Qt::RightButton:
        handleRightButtonPress(height, staffNo, e, el);
        break;

    case Qt::MidButton:
        handleMidButtonPress(height, staffNo, e, el);
        break;

    default:
        kdDebug(KDEBUG_AREA) << "NotationTool::handleMousePress : no button mouse press\n";
        break;
    }
}

void NotationTool::handleMidButtonPress(int, int,
                                        QMouseEvent*,
                                        NotationElement*)
{
}

void NotationTool::handleRightButtonPress(int, int,
                                          QMouseEvent*,
                                          NotationElement*)
{
    showMenu();
}

void NotationTool::handleMouseDblClick(int, int,
                                       QMouseEvent*,
                                       NotationElement*)
{
    // nothing
}


void NotationTool::handleMouseMove(QMouseEvent*)
{
}

void NotationTool::handleMouseRelease(QMouseEvent*)
{
}

void NotationTool::showMenu()
{
    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        kdDebug(KDEBUG_AREA) << "NotationTool::showMenu() : no menu to show\n";
}

void NotationTool::setParentView(NotationView* view)
{
    m_parentView = view;
}


//------------------------------


class
NoteInsertionCommand : public BasicCommand
{
public:
    NoteInsertionCommand(const QString &name,
			 NotationView *view, Segment &segment, timeT time,
			 timeT endTime, Note note, int pitch,
			 Accidental accidental) :
	BasicCommand(name, view, segment, time, endTime),
	m_note(note),
	m_pitch(pitch),
	m_accidental(accidental),
	m_justInsertedEvent(0),
	m_staffId(0) { }
    virtual ~NoteInsertionCommand() { }

protected:
    virtual void modifySegment(SegmentNotationHelper &helper) {

	Segment::iterator i =
	    helper.insertNote(getBeginTime(), m_note, m_pitch, m_accidental);

	if (i != helper.segment().end()) {
	    m_justInsertedEvent = *i;
	    m_staffId = getView()->getStaff(helper.segment())->getId();
	}
    }

    virtual void finishExecute() {

	BasicCommand::finishExecute();

	if (m_justInsertedEvent) {
	    getView()->setSingleSelectedEvent(m_staffId, m_justInsertedEvent);
	    m_justInsertedEvent = 0;
	}
    }

    Note m_note;
    int m_pitch;
    Accidental m_accidental;
    Rosegarden::Event *m_justInsertedEvent;
    int m_staffId;
};

class RestInsertionCommand : public NoteInsertionCommand
{
public:
    RestInsertionCommand(const QString &name,
			 NotationView *view, Segment &segment, timeT time,
			 timeT endTime, Note note) :
	NoteInsertionCommand(name, view, segment, time, endTime,
			     note, 0, NoAccidental) { }
    virtual ~RestInsertionCommand() { }

    virtual void modifySegment(SegmentNotationHelper &helper) {

	Segment::iterator i =
	    helper.insertRest(getBeginTime(), m_note);

	if (i != helper.segment().end()) {
	    m_justInsertedEvent = *i;
	    m_staffId = getView()->getStaff(helper.segment())->getId();
	}
    }
};


NoteInserter::NoteInserter(NotationView* view)
    : NotationTool("NoteInserter", view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_accidental(Rosegarden::NoAccidental)
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
    m_parentView->setPositionTracking(true);

    for (unsigned int i = 0, accidental = NoAccidental;
         i < 6; ++i, ++accidental) {

        KRadioAction* noteAction = new KRadioAction(i18n(m_actionsAccidental[i][0]),
                                                    0, this,
                                                    m_actionsAccidental[i][1],
                                                    actionCollection(),
                                                    m_actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }

    new KToggleAction(i18n("Toggle Dot"), 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");

    new KAction(i18n("Select"), 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Erase"), 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    createMenu("noteinserter.rc");
}

NoteInserter::NoteInserter(const QString& menuName,
                           NotationView* view)
    : NotationTool(menuName, view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_accidental(Rosegarden::NoAccidental)
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
}

NotationTool* NoteInserter::getInstance(NotationView* view)
{
    if (!m_instance)
        m_instance = new NoteInserter(view);

    m_instance->setParentView(view);

    return m_instance;
}


NoteInserter::~NoteInserter()
{
}

void NoteInserter::finalize()
{
    // disconnect previous signals, which were connected to a wrong
    // parentView
    //
    disconnect();
    
    connect(m_parentView, SIGNAL(changeAccidental(Rosegarden::Accidental)),
            this, SLOT(setAccidental(Rosegarden::Accidental)));

    m_parentView->setCanvasCursor(Qt::crossCursor);
    m_parentView->setPositionTracking(true);
}



void    
NoteInserter::handleLeftButtonPress(int height, int staffNo,
                                   QMouseEvent* e,
                                   NotationElement*)
{
    if (height == StaffLine::NoHeight || staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote(e->x(), tsig, clef, key, staffNo);

    //!!! Could be nicer! Likewise the other inserters.

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    int pitch = Rosegarden::NotationDisplayPitch(height, m_accidental).
        getPerformancePitch(clef ? Clef(*clef) : Clef::DefaultClef,
                            key ? Rosegarden::Key(*key) :
                            Rosegarden::Key::DefaultKey);

    Note note(m_noteType, m_noteDots);
    Segment &segment = m_parentView->getStaff(staffNo)->getSegment();

    timeT time = (*closestNote)->getAbsoluteTime();
    timeT endTime = time + note.getDuration();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (realEnd == segment.end() || ++realEnd == segment.end()) {
	endTime = segment.getEndIndex();
    } else {
	endTime = std::max(endTime, (*realEnd)->getAbsoluteTime());
    }

    doAddCommand(segment, time, endTime, note, pitch, m_accidental);
}

void
NoteInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int pitch, Accidental accidental)
{
    m_parentView->getCommandHistory()->addCommand
	(new NoteInsertionCommand
	 ("Insert Note",
	  m_parentView, segment, time, endTime, note, pitch, accidental));
} 

void NoteInserter::setNote(Rosegarden::Note::Type nt)
{
    m_noteType = nt;
}

void NoteInserter::setDots(unsigned int dots)
{
    m_noteDots = dots;
}

void NoteInserter::setAccidental(Rosegarden::Accidental accidental)
{
    m_accidental = accidental;
}

void NoteInserter::setAccidentalSync(Rosegarden::Accidental accidental)
{
    setAccidental(accidental);

    // Get the parent view toolbar in sync - check the corresponding action
    KAction* action =
        actionCollection()->action(m_actionsAccidental[accidental][2]);

    KToggleAction* tAction = 0;
    
    if ((tAction = dynamic_cast<KToggleAction*>(action)))
        tAction->setChecked(true);
    else
        KMessageBox::error(0, "NoteInserter::setAccidental : couldn't find action");
}

void NoteInserter::slotNoAccidental()
{
    m_parentView->actionCollection()->action("no_accidental")->activate();
}

void NoteInserter::slotSharp()
{
    m_parentView->actionCollection()->action("sharp_accidental")->activate();
}

void NoteInserter::slotFlat()
{
    m_parentView->actionCollection()->action("flat_accidental")->activate();
}

void NoteInserter::slotNatural()
{
    m_parentView->actionCollection()->action("natural_accidental")->activate();
}

void NoteInserter::slotDoubleSharp()
{
    m_parentView->actionCollection()->action("double_sharp_accidental")->activate();
}

void NoteInserter::slotDoubleFlat()
{
    m_parentView->actionCollection()->action("double_flat_accidental")->activate();
}

void NoteInserter::slotToggleDot()
{
    // TODO : sync. this with the NotationView toolbars
    m_noteDots = (m_noteDots) ? 0 : 1;
}

void NoteInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void NoteInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

const char* NoteInserter::m_actionsAccidental[][4] = 
    {
        { "No accidental",  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
        { "Sharp",          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
        { "Flat",           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
        { "Natural",        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
        { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
        { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
    };

//------------------------------

RestInserter::RestInserter(NotationView* view)
    : NoteInserter("RestInserter", view)
{
}

NotationTool* RestInserter::getInstance(NotationView* view)
{
    if (!m_instance)
        m_instance = new RestInserter(view);

    m_instance->setParentView(view);

    return m_instance;
}

void
RestInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int, Accidental)
{
    m_parentView->getCommandHistory()->addCommand
	(new RestInsertionCommand
	 ("Insert Rest", m_parentView, segment, time, endTime, note));
} 


//------------------------------

ClefInserter::ClefInserter(NotationView* view)
    : NotationTool("ClefInserter", view),
      m_clef(Rosegarden::Clef::Treble)
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
}

void ClefInserter::finalize()
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
}

NotationTool* ClefInserter::getInstance(NotationView* view)
{
    if (!m_instance)
        m_instance = new ClefInserter(view);

    m_instance->setParentView(view);

    return m_instance;
}
    
void ClefInserter::setClef(std::string clefType)
{
    m_clef = clefType;
}

void ClefInserter::handleLeftButtonPress(int, int staffNo,
                                        QMouseEvent* e,
                                        NotationElement*)
{
    Event *tsig = 0, *clef = 0, *key = 0;

    if (staffNo < 0) return;

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote
        (e->x(), tsig, clef, key, staffNo, 100);

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();
    SegmentNotationHelper nt
        (m_parentView->getStaff(staffNo)->getSegment());

    m_parentView->getDocument()->setModified();

    Segment::iterator i = nt.insertClef(time, m_clef);
    Event *newEvent = 0;
    if (i != nt.segment().end()) newEvent = *i;

    m_parentView->redoLayout(staffNo, time, time + 1);
    m_parentView->setSingleSelectedEvent(staffNo, newEvent);
}


//------------------------------

NotationEraser::NotationEraser(NotationView* view)
    : NotationTool("NotationEraser", view),
      m_collapseRest(false)
{
    m_parentView->setCanvasCursor(Qt::pointingHandCursor);

    new KToggleAction(i18n("Toggle Rest Collapse"), 0, this,
                      SLOT(toggleRestCollapse()), actionCollection(),
                      "toggle_rest_collapse");

    createMenu("notationeraser.rc");
}

void NotationEraser::finalize()
{
    m_parentView->setCanvasCursor(Qt::pointingHandCursor);
}

NotationTool* NotationEraser::getInstance(NotationView* view)
{
    if (!m_instance)
        m_instance = new NotationEraser(view);

    m_instance->setParentView(view);

    return m_instance;
}

void NotationEraser::handleLeftButtonPress(int, int staffNo,
                                          QMouseEvent*,
                                          NotationElement* element)
{
    bool needLayout = false;
    if (!element || staffNo < 0) return;

    SegmentNotationHelper nt
        (m_parentView->getStaff(staffNo)->getSegment());

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
        m_parentView->getDocument()->setModified();
        m_parentView->redoLayout(staffNo, absTime, absTime);
    }
}

void NotationEraser::toggleRestCollapse()
{
    m_collapseRest = !m_collapseRest;
}


//------------------------------

NotationSelector::NotationSelector(NotationView* view)
    : NotationTool("NotationSelector", view),
      m_selectionRect(new QCanvasRectangle(m_parentView->canvas())),
      m_updateRect(false),
      m_clickedStaff(-1),
      m_clickedElement(0)
{
    m_selectionRect->hide();
    m_selectionRect->setPen(Qt::blue);

    connect(m_parentView, SIGNAL(usedSelection()),
            this,         SLOT(hideSelection()));
}

NotationSelector::~NotationSelector()
{
    delete m_selectionRect;
    m_parentView->canvas()->update();
}

NotationTool* NotationSelector::getInstance(NotationView* view)
{
    if (!m_instance)
        m_instance = new NotationSelector(view);

    m_instance->setParentView(view);

    return m_instance;
}

void NotationSelector::handleLeftButtonPress(int, int staffNo,
                                            QMouseEvent* e,
                                            NotationElement *element)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMousePress" << endl;
    m_clickedStaff = staffNo;
    m_clickedElement = element;

    m_selectionRect->setX(e->x());
    m_selectionRect->setY(e->y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;

    //m_parentView->setCursorPosition(p.x());
}

void NotationSelector::handleMouseDblClick(int, int staffNo,
                                           QMouseEvent* e,
                                           NotationElement *element)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMouseDblClick" << endl;
    m_clickedStaff = staffNo;
    m_clickedElement = element;
    
    NotationStaff *staff = m_parentView->getStaff(staffNo);
    if (!staff) return;

    QRect rect = staff->getBarExtents(e->x());

    m_selectionRect->setX(rect.x() + 1);
    m_selectionRect->setY(rect.y());
    m_selectionRect->setSize(rect.width() - 1, rect.height());

    m_selectionRect->show();
    m_updateRect = false;
    return;
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

    m_parentView->canvas()->update();
}

void NotationSelector::handleMouseRelease(QMouseEvent*)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMouseRelease" << endl;
    m_updateRect = false;
    setViewCurrentSelection();

    // If we didn't drag out a meaningful area, but _did_ click on
    // an individual event, then select just that event
    
    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) {

	m_selectionRect->hide();

	if (m_clickedElement != 0 &&
	    m_clickedStaff   >= 0) {

	    m_parentView->setSingleSelectedEvent
		(m_clickedStaff, m_clickedElement->event());
	}
    }
}

void NotationSelector::hideSelection()
{
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_parentView->canvas()->update();
}


EventSelection* NotationSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

    //    kdDebug(KDEBUG_AREA) << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) return 0;

    double middleY = m_selectionRect->y() + m_selectionRect->height()/2;
    int staffNo = m_parentView->findClosestStaff(middleY);
    if (staffNo < 0) return 0;
    Segment& originalSegment = m_parentView->getStaff(staffNo)->getSegment();
    
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
    m_parentView->setCurrentSelection(selection);
}

//------------------------------

NotationSelectionPaster::NotationSelectionPaster(EventSelection& es,
                                                 NotationView* parent)
    : NotationTool("NotationPaster", parent),
      m_selection(es)
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
}

NotationSelectionPaster::~NotationSelectionPaster()
{
}

void NotationSelectionPaster::handleLeftButtonPress(int, int staffNo,
                                                   QMouseEvent* e,
                                                   NotationElement*)
{
    QPoint eventPos = e->pos();
    
    kdDebug(KDEBUG_AREA) << "NotationSelectionPaster::handleLeftButtonPress : staffNo = "
                         << staffNo
                         << "event pos : "
                         << eventPos.x() << "," << eventPos.y() << endl;

    if (staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote(eventPos.x(), tsig, clef, key, staffNo);

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();

    Segment& segment = m_parentView->getStaff(staffNo)->getSegment();

    if (m_selection.pasteToSegment(segment, time)) {

        m_parentView->redoLayout(staffNo,
                                0,
                                time + m_selection.getTotalDuration() + 1);

    } else {
        
        m_parentView->slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
    
    //m_parentView->slotStatusHelpMsg(i18n("Ready."));

}

NotationTool* NotationTool::m_instance = 0;
NotationTool* NoteInserter::m_instance = 0;
NotationTool* RestInserter::m_instance = 0;
NotationTool* ClefInserter::m_instance = 0;
NotationTool* NotationEraser::m_instance = 0;
NotationTool* NotationSelector::m_instance = 0;

