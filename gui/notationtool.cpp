// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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
#include <qtimer.h>
#include <qregexp.h>
#include <qapplication.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>

#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "Selection.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "notationview.h"
#include "notationstrings.h"
#include "staffline.h"
#include "qcanvassimplesprite.h"

#include "notationcommands.h"
#include "editcommands.h"
#include "dialogs.h"

#include "rosedebug.h"
#include "colours.h"

using Rosegarden::Accidental;
using Rosegarden::Event;
using Rosegarden::EventSelection;
using Rosegarden::Clef;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;
using Rosegarden::ViewElement;
using namespace Rosegarden::BaseProperties;

//////////////////////////////////////////////////////////////////////
//               Notation Toolbox
//////////////////////////////////////////////////////////////////////

NotationToolBox::NotationToolBox(NotationView *parent)
    : EditToolBox(parent),
      m_nParentView(parent)
{
    //m_tools.setAutoDelete(true);
}


// NotationTool* NotationToolBox::getTool(const QString& toolName)
// {
//     NotationTool* tool = m_tools[toolName];

//     if (!tool) tool = createTool(toolName);
    
//     return tool;
// }


EditTool* NotationToolBox::createTool(const QString& toolName)
{
    NotationTool* tool = 0;

    QString toolNamelc = toolName.lower();
    
    if (toolNamelc == NoteInserter::ToolName)

        tool = new NoteInserter(m_nParentView);

    else if (toolNamelc == RestInserter::ToolName)

        tool = new RestInserter(m_nParentView);

    else if (toolNamelc == ClefInserter::ToolName)

        tool = new ClefInserter(m_nParentView);

    else if (toolNamelc == TextInserter::ToolName)

        tool = new TextInserter(m_nParentView);

    else if (toolNamelc == NotationEraser::ToolName)

        tool = new NotationEraser(m_nParentView);

    else if (toolNamelc == NotationSelector::ToolName)

        tool = new NotationSelector(m_nParentView);

    else {
        KMessageBox::error(0, QString("NotationToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
}


//////////////////////////////////////////////////////////////////////
//               Notation Tool
//////////////////////////////////////////////////////////////////////

NotationTool::NotationTool(const QString& menuName, NotationView* view)
    : EditTool(menuName, view),
      m_nParentView(view)
{
}

NotationTool::~NotationTool()
{
    NOTATION_DEBUG << "NotationTool::~NotationTool()" << endl;

    //     delete m_menu;
    //     m_parentView->factory()->removeClient(this);
    //     m_instance = 0;
}

void NotationTool::ready()
{
    m_nParentView->setCanvasCursor(Qt::arrowCursor);
    m_nParentView->setHeightTracking(false);
}


//------------------------------


NoteInserter::NoteInserter(NotationView* view)
    : NotationTool("NoteInserter", view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_autoBeam(true),
      m_accidental(Rosegarden::Accidentals::NoAccidental),
      m_lastAccidental(Rosegarden::Accidentals::NoAccidental),
      m_followAccidental(false)
{
    QIconSet icon;

    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    m_autoBeam = config->readBoolEntry("autobeam", true);
    m_matrixInsertType = (config->readNumEntry("inserttype", 0) > 0);
    m_defaultStyle = qstrtostr(config->readEntry
	("style", strtoqstr(NoteStyleFactory::DefaultStyle)));

    KToggleAction *autoBeamAction =
	new KToggleAction(i18n("Auto-Beam when appropriate"), 0, this,
			  SLOT(slotToggleAutoBeam()), actionCollection(),
			  "toggle_auto_beam");
    autoBeamAction->setChecked(m_autoBeam);

    for (unsigned int i = 0; i < 6; ++i) {

	icon = QIconSet
	    (NotePixmapFactory::toQPixmap(NotePixmapFactory::
	     makeToolbarPixmap(m_actionsAccidental[i][3])));
        KRadioAction* noteAction = new KRadioAction(i18n(m_actionsAccidental[i][0]),
                                                    icon, 0, this,
                                                    m_actionsAccidental[i][1],
                                                    actionCollection(),
                                                    m_actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("dotted-crotchet")));
    new KToggleAction(i18n("Dotted note"), icon, 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
		    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("rest-crotchet")));
    new KAction(i18n("Switch to Inserting Rests"), icon, 0, this,
                SLOT(slotRestsSelected()), actionCollection(),
                "rests");

    createMenu("noteinserter.rc");

    connect(m_parentView, SIGNAL(changeAccidental(Rosegarden::Accidental, bool)),
            this,         SLOT(slotSetAccidental(Rosegarden::Accidental, bool)));
}

NoteInserter::NoteInserter(const QString& menuName, NotationView* view)
    : NotationTool(menuName, view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_autoBeam(false),
      m_clickHappened(false),
      m_accidental(Rosegarden::Accidentals::NoAccidental),
      m_lastAccidental(Rosegarden::Accidentals::NoAccidental),
      m_followAccidental(false)
{
    connect(m_parentView, SIGNAL(changeAccidental(Rosegarden::Accidental, bool)),
            this,         SLOT(slotSetAccidental(Rosegarden::Accidental, bool)));
}

NoteInserter::~NoteInserter()
{
}

void NoteInserter::ready()
{
    m_clickHappened = false;
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(true);
}



void    
NoteInserter::handleLeftButtonPress(Rosegarden::timeT,
                                    int,
                                    int staffNo,
				    QMouseEvent* e,
				    ViewElement*)
{
    if (staffNo < 0) return;
    computeLocationAndPreview(e);
}


int
NoteInserter::handleMouseMove(Rosegarden::timeT,
			      int,
			      QMouseEvent *e)
{
    if (m_clickHappened) {
        computeLocationAndPreview(e);
    }
    
    return RosegardenCanvasView::NoFollow;
}


void
NoteInserter::handleMouseRelease(Rosegarden::timeT,
				 int,
				 QMouseEvent *e)
{
    if (!m_clickHappened) return;
    bool okay = computeLocationAndPreview(e);
    m_clickHappened = false;
    if (!okay) return;
    clearPreview();

    Note note(m_noteType, m_noteDots);
    timeT endTime = m_clickTime + note.getDuration();
    Segment &segment = m_nParentView->getStaff(m_clickStaffNo)->getSegment();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (!segment.isBeforeEndMarker(  realEnd) ||
	!segment.isBeforeEndMarker(++realEnd)) {
	endTime = segment.getEndMarkerTime();
    } else {
	endTime = std::max(endTime, (*realEnd)->getNotationAbsoluteTime());
    }

    Event *lastInsertedEvent = doAddCommand
	(segment, m_clickTime, endTime, note, m_clickPitch,
	 (m_accidental == Rosegarden::Accidentals::NoAccidental &&
	  m_followAccidental) ?
	    m_lastAccidental : m_accidental);

    if (lastInsertedEvent) {

	m_nParentView->setSingleSelectedEvent
	    (m_clickStaffNo, lastInsertedEvent);

	if (m_nParentView->isInChordMode()) {
	    m_nParentView->slotSetInsertCursorAndRecentre
		(lastInsertedEvent->getAbsoluteTime(), e->x(), (int)e->y(),
		 false);
	} else {
	    m_nParentView->slotSetInsertCursorAndRecentre
		(lastInsertedEvent->getAbsoluteTime() +
		 lastInsertedEvent->getDuration(), e->x(), (int)e->y(),
		 false);
	}
    }
}


void
NoteInserter::insertNote(Segment &segment, timeT insertionTime,
			 int pitch, Rosegarden::Accidental accidental,
			 bool suppressPreview)
{
    Note note(m_noteType, m_noteDots);
    timeT endTime = insertionTime + note.getDuration();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (!segment.isBeforeEndMarker(  realEnd) ||
	!segment.isBeforeEndMarker(++realEnd)) {
	endTime = segment.getEndMarkerTime();
    } else {
	endTime = std::max(endTime, (*realEnd)->getNotationAbsoluteTime());
    }

    Event *lastInsertedEvent = doAddCommand
	(segment, insertionTime, endTime, note, pitch, accidental);

    if (lastInsertedEvent) {

	m_nParentView->setSingleSelectedEvent(segment, lastInsertedEvent);

	if (m_nParentView->isInChordMode()) {
	    m_nParentView->slotSetInsertCursorPosition
		(lastInsertedEvent->getAbsoluteTime(), true, false);
	} else {
	    m_nParentView->slotSetInsertCursorPosition
		(lastInsertedEvent->getAbsoluteTime() +
		 lastInsertedEvent->getDuration(), true, false);
	}
    }

    if (!suppressPreview) m_nParentView->playNote(segment, pitch);
}


bool
NoteInserter::computeLocationAndPreview(QMouseEvent *e)
{
    double x = e->x();
    int y = (int)e->y();

    NotationStaff *staff = dynamic_cast<NotationStaff *>
	(m_nParentView->getStaffForCanvasCoords(e->x(), y));
    if (!staff) {
	clearPreview();
	return false;
    }

    int staffNo = staff->getId();
    if (m_clickHappened && staffNo != m_clickStaffNo) {
	// abandon
	clearPreview();
	return false;
    }

    int height = staff->getHeightAtCanvasCoords(x, y);

    Event *clefEvt = 0, *keyEvt = 0;
    Rosegarden::Clef clef;
    Rosegarden::Key key;

    NotationElementList::iterator itr =
	staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);
    if (itr == staff->getViewElementList()->end()) {
	clearPreview();
	return false;
    }

    timeT time = (*itr)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    m_clickInsertX = (*itr)->getLayoutX();
    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);
    if (keyEvt) key = Rosegarden::Key(*keyEvt);

    NotationElement* el = static_cast<NotationElement*>(*itr);
    if (el->isRest() && el->getCanvasItem()) {
	time += getOffsetWithinRest(staffNo, itr, x);
	m_clickInsertX += (x - el->getCanvasX());
    }

    Rosegarden::Pitch p(height, clef, key, m_accidental);
    int pitch = p.getPerformancePitch();

    // [RFE 987960] When inserting via mouse, if no accidental is
    // selected, we use the same accidental (and thus the same pitch)
    // as of the previous note found at this height -- iff such a note
    // is found more recently than the last key signature.

    if (m_accidental == Rosegarden::Accidentals::NoAccidental &&
	m_followAccidental) {
	Rosegarden::Segment &segment = staff->getSegment();
	m_lastAccidental = m_accidental;
	Segment::iterator i = segment.findNearestTime(time);
	while (i != segment.end()) {
	    if ((*i)->isa(Rosegarden::Key::EventType)) break;
	    if ((*i)->isa(Rosegarden::Note::EventType)) {
		if ((*i)->has(NotationProperties::HEIGHT_ON_STAFF) &&
		    (*i)->has(Rosegarden::BaseProperties::PITCH)) {
		    int h = (*i)->get<Int>(NotationProperties::HEIGHT_ON_STAFF);
		    if (h == height) {
			pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
			(*i)->get<String>(Rosegarden::BaseProperties::ACCIDENTAL,
					  m_lastAccidental);
			break;
		    }
		}
	    }
	    if (i == segment.begin()) break;
	    --i;
	}
    }

    bool changed = false;

    if (m_clickHappened) {
	if (time != m_clickTime ||
	    pitch != m_clickPitch ||
	    height != m_clickHeight ||
	    staffNo != m_clickStaffNo) {
	    changed = true;
	}
    } else {
	m_clickHappened = true;
	changed = true;
    }

    if (changed) {
	m_clickTime = time;
	m_clickPitch = pitch;
	m_clickHeight = height;
	m_clickStaffNo = staffNo;
	
	showPreview();
    }

    return true;
}

void NoteInserter::showPreview()
{
    Segment &segment = m_nParentView->getStaff(m_clickStaffNo)->getSegment();

    int pitch = m_clickPitch;
    pitch += getOttavaShift(segment, m_clickTime) * 12;

    m_nParentView->showPreviewNote(m_clickStaffNo, m_clickInsertX,
				   pitch, m_clickHeight,
				   Note(m_noteType, m_noteDots));
}

void NoteInserter::clearPreview()
{
    m_nParentView->clearPreviewNote();
}
    

timeT
NoteInserter::getOffsetWithinRest(int staffNo,
				  const NotationElementList::iterator &i,
				  double &canvasX) // will be snapped
{
    //!!! To make this work correctly in tuplet mode, our divisor would
    // have to be the tupletified duration of the tuplet unit -- we can
    // do that, we just haven't yet
    if (m_nParentView->isInTripletMode()) return 0;

    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    NotationElement* el = static_cast<NotationElement*>(*i);
    if (!el->getCanvasItem()) return 0;
    double offset = canvasX - el->getCanvasX();

    if (offset < 0) return 0;

    double airX, airWidth;
    el->getLayoutAirspace(airX, airWidth);
    double origin = ((*i)->getLayoutX() - airX) / 2;
    double width = airWidth - origin;

    timeT duration = (*i)->getViewDuration();

    Rosegarden::TimeSignature timeSig =
	staff->getSegment().getComposition()->getTimeSignatureAt
	((*i)->event()->getAbsoluteTime());
    timeT unit = timeSig.getUnitDuration();

    int unitCount = duration / unit;
    if (unitCount > 1) {

	timeT result = (int)((offset / width) * unitCount);
	if (result > unitCount - 1) result = unitCount - 1;

	double visibleWidth(airWidth);
	NotationElementList::iterator j(i);
	if (++j != staff->getViewElementList()->end()) {
	    visibleWidth = (*j)->getLayoutX() - (*i)->getLayoutX();
	}
	offset = (visibleWidth * result) / unitCount;
	canvasX = el->getCanvasX() + offset;
	
	result *= unit;
	return result;
    }
    
    return 0;
}

int
NoteInserter::getOttavaShift(Segment &segment, timeT time)
{
    // Find out whether we're in an ottava section.

    int ottavaShift = 0;

    for (Segment::iterator i = segment.findTime(time); ; --i) {

	if (!segment.isBeforeEndMarker(i)) break;

	if ((*i)->isa(Rosegarden::Note::EventType) &&
	    (*i)->has(NotationProperties::OTTAVA_SHIFT)) {
	    ottavaShift = (*i)->get<Int>(NotationProperties::OTTAVA_SHIFT);
	    break;
	}

	if ((*i)->isa(Rosegarden::Indication::EventType)) {
	    try {
		Rosegarden::Indication ind(**i);
		if (ind.isOttavaType()) {
		    ottavaShift = ind.getOttavaShift();
		    break;
		}
	    } catch (...) { }
	}

	if (i == segment.begin()) break;
    }

    return ottavaShift;
}

Event *
NoteInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int pitch, Accidental accidental)
{
    timeT noteEnd = time + note.getDuration();

    // #1046934: make it possible to insert triplet at end of segment!
    if (m_nParentView->isInTripletMode()) {
	noteEnd = time + (note.getDuration() * 2 / 3);
    }

    if (time < segment.getStartTime() ||
	endTime > segment.getEndMarkerTime() ||
	noteEnd > segment.getEndMarkerTime()) {
	return 0;
    }

    pitch += getOttavaShift(segment, time) * 12;

    NoteInsertionCommand *insertionCommand =
	new NoteInsertionCommand
        (segment, time, endTime, note, pitch, accidental,
	 m_autoBeam && !m_nParentView->isInTripletMode(),
	 m_matrixInsertType,
	 m_defaultStyle);

    KCommand *activeCommand = insertionCommand;

    if (m_nParentView->isInTripletMode()) {
	Segment::iterator i(segment.findTime(time));
	if (i != segment.end() &&
	    !(*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

	    KMacroCommand *command = new KMacroCommand(insertionCommand->name());

	    // Attempted fix to bug reported on rg-user by SlowPic
	    // <slowpic@web.de> 28/02/2005 22:32:56 UTC: Triplet input error
	    if ((*i)->isa(Rosegarden::Note::EventRestType) &&
		(*i)->getNotationDuration() > (note.getDuration() * 3)) {
		// split the rest
		command->addCommand(new RestInsertionCommand
				    (segment, time,
				     time + note.getDuration() * 2,
				     Note::getNearestNote(note.getDuration() * 2)));
	    }

	    command->addCommand(new AdjustMenuTupletCommand
				(segment, time, note.getDuration(),
				 3, 2, true)); // #1046934: "has timing already"
	    command->addCommand(insertionCommand);
	    activeCommand = command;
	}
    }

    m_nParentView->addCommandToHistory(activeCommand);
    
    NOTATION_DEBUG << "NoteInserter::doAddCommand: accidental is "
			 << accidental << endl;

    return insertionCommand->getLastInsertedEvent();
} 

void NoteInserter::slotSetNote(Rosegarden::Note::Type nt)
{
    m_noteType = nt;
}

void NoteInserter::slotSetDots(unsigned int dots)
{
    m_noteDots = dots;

    KToggleAction *dotsAction = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_dot"));
    if (dotsAction) dotsAction->setChecked(dots > 0);
}

void NoteInserter::slotSetAccidental(Rosegarden::Accidental accidental,
				     bool follow)
{
    NOTATION_DEBUG << "NoteInserter::setAccidental: accidental is "
			 << accidental << endl;
    m_accidental = accidental;
    m_followAccidental = follow;
}

void NoteInserter::slotNoAccidental()
{
    m_parentView->actionCollection()->action("no_accidental")->activate();
}

void NoteInserter::slotFollowAccidental()
{
    m_parentView->actionCollection()->action("follow_accidental")->activate();
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
    m_noteDots = (m_noteDots) ? 0 : 1;
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");
    KAction *action = m_parentView->actionCollection()->action(actionName);
    if (!action) {
	std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
	action->activate();
    }
}

void NoteInserter::slotToggleAutoBeam()
{
    m_autoBeam = !m_autoBeam;
}

void NoteInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void NoteInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void NoteInserter::slotRestsSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note, true));
    actionName.replace(QRegExp("-"), "_");
    KAction *action = m_parentView->actionCollection()->action(actionName);
    if (!action) {
	std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
	action->activate();
    }
}

const char* NoteInserter::m_actionsAccidental[][4] = 
    {
        { "No accidental",  "1slotNoAccidental()",  "no_accidental",
          "accidental-none" },
        { "Follow accidental",  "1slotFollowAccidental()",  "follow_accidental",
          "accidental-follow" },
        { "Sharp",          "1slotSharp()",         "sharp_accidental",
          "accidental-sharp" },
        { "Flat",           "1slotFlat()",          "flat_accidental",
          "accidental-flat" },
        { "Natural",        "1slotNatural()",       "natural_accidental",
          "accidental-natural" },
        { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental",
          "accidental-doublesharp" },
        { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",
          "accidental-doubleflat" }
    };

//------------------------------

RestInserter::RestInserter(NotationView* view)
    : NoteInserter("RestInserter", view)
{
    QIconSet icon;

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("dotted-rest-crotchet")));
    new KToggleAction(i18n("Dotted rest"), icon, 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
		    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Inserting Notes"), icon, 0, this,
                SLOT(slotNotesSelected()), actionCollection(),
                "notes");

    createMenu("restinserter.rc");
}

void
RestInserter::showPreview()
{
    // no preview available for now
}

Event *
RestInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int, Accidental)
{
    if (time < segment.getStartTime() ||
	endTime > segment.getEndMarkerTime() ||
	time + note.getDuration() > segment.getEndMarkerTime()) {
	return 0;
    }

    NoteInsertionCommand *insertionCommand = 
	new RestInsertionCommand(segment, time, endTime, note);

    KCommand *activeCommand = insertionCommand;

    if (m_nParentView->isInTripletMode()) {
	Segment::iterator i(segment.findTime(time));
	if (i != segment.end() &&
	    !(*i)->has(BEAMED_GROUP_TUPLET_BASE)) {

	    KMacroCommand *command = new KMacroCommand(insertionCommand->name());
	    command->addCommand(new AdjustMenuTupletCommand
				(segment, time, note.getDuration()));
	    command->addCommand(insertionCommand);
	    activeCommand = command;
	}
    }

    m_nParentView->addCommandToHistory(activeCommand);

    return insertionCommand->getLastInsertedEvent();
} 

void RestInserter::slotToggleDot()
{
    m_noteDots = (m_noteDots) ? 0 : 1;
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note, true));
    actionName.replace(QRegExp("-"), "_");
    KAction *action = m_parentView->actionCollection()->action(actionName);
    if (!action) {
	std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
	action->activate();
    }
}

void RestInserter::slotNotesSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp(" "), "_");
    m_parentView->actionCollection()->action(actionName)->activate();
}


//------------------------------

ClefInserter::ClefInserter(NotationView* view)
    : NotationTool("ClefInserter", view),
      m_clef(Rosegarden::Clef::Treble)
{
    QIconSet icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
		    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Inserting Notes"), icon, 0, this,
                SLOT(slotNotesSelected()), actionCollection(),
                "notes");

    createMenu("clefinserter.rc");
}

void ClefInserter::slotNotesSelected()
{
    m_nParentView->slotLastNoteAction();
}

void ClefInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void ClefInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void ClefInserter::ready()
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(false);
}

void ClefInserter::setClef(std::string clefType)
{
    m_clef = clefType;
}

void ClefInserter::handleLeftButtonPress(Rosegarden::timeT,
                                         int,
                                         int staffNo,
					 QMouseEvent* e,
					 ViewElement*)
{
    if (staffNo < 0) return;
    Event *clef = 0, *key = 0;

    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    
    NotationElementList::iterator closestElement =
	staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
					       clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end()) return;

    timeT time = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()


    ClefInsertionCommand *command = 
	new ClefInsertionCommand(staff->getSegment(), time, m_clef);

    m_nParentView->addCommandToHistory(command);

    Event *event = command->getLastInsertedEvent();
    if (event) m_nParentView->setSingleSelectedEvent(staffNo, event);
}

//------------------------------

TextInserter::TextInserter(NotationView* view)
    : NotationTool("TextInserter", view),
      m_text("", Rosegarden::Text::Dynamic)
{
    QIconSet icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
		    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Inserting Notes"), icon, 0, this,
                SLOT(slotNotesSelected()), actionCollection(),
                "notes");

    createMenu("textinserter.rc");
}

void TextInserter::slotNotesSelected()
{
    m_nParentView->slotLastNoteAction();
}

void TextInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void TextInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void TextInserter::ready()
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(false);
}

void TextInserter::handleLeftButtonPress(Rosegarden::timeT,
                                         int,
                                         int staffNo,
					 QMouseEvent* e,
					 ViewElement *element)
{
    if (staffNo < 0) return;
    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    
    Rosegarden::Text defaultText(m_text);
    timeT insertionTime;
    Event *eraseEvent = 0;

    if (element && element->event()->isa(Rosegarden::Text::EventType)) {

	// edit an existing text, if that's what we clicked on

	try {
	    defaultText = Rosegarden::Text(*element->event());
	} catch (Rosegarden::Exception e) {
	}

	insertionTime = element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

	eraseEvent = element->event();

    } else {

	Event *clef = 0, *key = 0;

	NotationElementList::iterator closestElement =
	    staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
						   clef, key, false, -1);

	if (closestElement == staff->getViewElementList()->end()) return;
	
	insertionTime = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    }

    TextEventDialog *dialog = new TextEventDialog
	(m_nParentView, m_nParentView->getNotePixmapFactory(), defaultText);

    if (dialog->exec() == QDialog::Accepted) {
	
	m_text = dialog->getText();

	TextInsertionCommand *command = 
	    new TextInsertionCommand
	    (staff->getSegment(), insertionTime, m_text);

	if (eraseEvent) {
	    KMacroCommand *macroCommand = new KMacroCommand(command->name());
	    macroCommand->addCommand(new EraseEventCommand(staff->getSegment(),
							   eraseEvent, false));
	    macroCommand->addCommand(command);
	    m_nParentView->addCommandToHistory(macroCommand);
	} else {
	    m_nParentView->addCommandToHistory(command);
	}

	Event *event = command->getLastInsertedEvent();
	if (event) m_nParentView->setSingleSelectedEvent(staffNo, event);
    }

    delete dialog;
}


//------------------------------

NotationEraser::NotationEraser(NotationView* view)
    : NotationTool("NotationEraser", view),
      m_collapseRest(false)
{
    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    m_collapseRest = config->readBoolEntry("collapse", false);

    new KToggleAction(i18n("Collapse rests after erase"), 0, this,
                      SLOT(slotToggleRestCollapse()), actionCollection(),
                      "toggle_rest_collapse");

    QIconSet icon
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("crotchet")));
    new KAction(i18n("Switch to Insert Tool"), icon, 0, this,
                      SLOT(slotInsertSelected()), actionCollection(),
                      "insert");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
		    makeToolbarPixmap("select")));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    createMenu("notationeraser.rc");
}

void NotationEraser::ready()
{
    m_nParentView->setCanvasCursor(Qt::pointingHandCursor);
    m_nParentView->setHeightTracking(false);
}

void NotationEraser::handleLeftButtonPress(Rosegarden::timeT,
                                           int,
                                           int staffNo,
                                           QMouseEvent*,
                                           ViewElement* element)
{
    if (!element || staffNo < 0) return;

    EraseEventCommand *command =
	new EraseEventCommand(m_nParentView->getStaff(staffNo)->getSegment(),
			      element->event(),
			      m_collapseRest);

    m_nParentView->addCommandToHistory(command);
}

void NotationEraser::slotToggleRestCollapse()
{
    m_collapseRest = !m_collapseRest;
}

void NotationEraser::slotInsertSelected()
{
    m_nParentView->slotLastNoteAction();
}

void NotationEraser::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}


//------------------------------

NotationSelector::NotationSelector(NotationView* view)
    : NotationTool("NotationSelector", view),
      m_selectionRect(0),
      m_updateRect(false),
      m_selectedStaff(0),
      m_clickedElement(0),
      m_selectionToMerge(0),
      m_justSelectedBar(false),
      m_wholeStaffSelectionComplete(false)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this,         SLOT(slotHideSelection()));

    connect(this, SIGNAL(editElement(NotationStaff *, NotationElement *, bool)),
	    m_parentView, SLOT(slotEditElement(NotationStaff *, NotationElement *, bool)));

    QIconSet icon
	(NotePixmapFactory::toQPixmap(NotePixmapFactory::
	 makeToolbarPixmap("crotchet")));
    new KToggleAction(i18n("Switch to Insert Tool"), icon, 0, this,
                      SLOT(slotInsertSelected()), actionCollection(),
                      "insert");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

// (this crashed, and it might be superfluous with ^N anyway, so I'm
// commenting it out, but leaving it here in case I change my mind about
// fooling with it.)  (DMM)
//    new KAction(i18n("Normalize Rests"), 0, 0, this,
//		SLOT(slotCollapseRests()), actionCollection(),
//		"collapse_rests"); 

    new KAction(i18n("Collapse Rests"), 0, 0, this,
		SLOT(slotCollapseRestsHard()), actionCollection(),
		"collapse_rests_aggressively"); 

    new KAction(i18n("Respell as Flat"), 0, 0, this,
		SLOT(slotRespellFlat()), actionCollection(),
		"respell_flat"); 

    new KAction(i18n("Respell as Sharp"), 0, 0, this,
		SLOT(slotRespellSharp()), actionCollection(),
		"respell_sharp"); 

    new KAction(i18n("Respell as Natural"), 0, 0, this,
		SLOT(slotRespellNatural()), actionCollection(),
		"respell_natural"); 

    new KAction(i18n("Collapse Notes"), 0, 0, this,
		SLOT(slotCollapseNotes()), actionCollection(),
		"collapse_notes"); 

    new KAction(i18n("Interpret"), 0, 0, this,
		SLOT(slotInterpret()), actionCollection(),
		"interpret"); 

    new KAction(i18n("Make Invisible"), 0, 0, this,
		SLOT(slotMakeInvisible()), actionCollection(),
		"make_invisible"); 

    new KAction(i18n("Make Visible"), 0, 0, this,
		SLOT(slotMakeVisible()), actionCollection(),
		"make_visible"); 

    createMenu("notationselector.rc");
}

NotationSelector::~NotationSelector()
{
    delete m_selectionToMerge;
}

void NotationSelector::handleLeftButtonPress(Rosegarden::timeT t,
                                             int height,
                                             int staffNo,
                                             QMouseEvent* e,
                                             ViewElement *element)
{
    NOTATION_DEBUG << "NotationSelector::handleMousePress: time is " << t << ", staffNo is " << staffNo << ", e and element are " << e << " and " << element << endl;

    if (m_justSelectedBar) {
	handleMouseTripleClick(t, height, staffNo, e, element);
	m_justSelectedBar = false;
	return;
    }

    m_wholeStaffSelectionComplete = false;

    delete m_selectionToMerge;
    const EventSelection *selectionToMerge = 0;
    if (e->state() & ShiftButton) {
	m_clickedShift = true;
	selectionToMerge = m_nParentView->getCurrentSelection();
    } else {
	m_clickedShift = false;
    }
    m_selectionToMerge =
	(selectionToMerge ? new EventSelection(*selectionToMerge) : 0);

    m_clickedElement = dynamic_cast<NotationElement*>(element);
    if (m_clickedElement) {
	m_selectedStaff = getStaffForElement(m_clickedElement);
	m_lastDragPitch = -400;
	m_lastDragTime = m_clickedElement->event()->getNotationAbsoluteTime();
    } else {
	m_selectedStaff = 0; // don't know yet; wait until we have an element
    }

    m_selectionRect->setX(e->x());
    m_selectionRect->setY(e->y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;
    m_startedFineDrag = false;

    //m_parentView->setCursorPosition(p.x());
}

void NotationSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void NotationSelector::handleMouseDoubleClick(Rosegarden::timeT,
					      int,
					      int staffNo,
					      QMouseEvent* e,
					      ViewElement *element)
{
    NOTATION_DEBUG << "NotationSelector::handleMouseDoubleClick" << endl;
    m_clickedElement = dynamic_cast<NotationElement*>(element);
    
    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    if (!staff) return;
    m_selectedStaff = staff;

    bool advanced = (e->state() & ShiftButton);

    if (m_clickedElement) {

	emit editElement(staff, m_clickedElement, advanced);

    } else {

	QRect rect = staff->getBarExtents(e->x(), e->y());

	m_selectionRect->setX(rect.x() + 1);
	m_selectionRect->setY(rect.y());
	m_selectionRect->setSize(rect.width() - 1, rect.height());

	m_selectionRect->show();
	m_updateRect = false;
	
	m_justSelectedBar = true;
	QTimer::singleShot(QApplication::doubleClickInterval(), this,
			   SLOT(slotClickTimeout()));
    }

    return;
}


void NotationSelector::handleMouseTripleClick(Rosegarden::timeT t,
					      int height,
					      int staffNo,
					      QMouseEvent* e,
					      ViewElement *element)
{
    if (!m_justSelectedBar) return;
    m_justSelectedBar = false;

    NOTATION_DEBUG << "NotationSelector::handleMouseTripleClick" << endl;
    m_clickedElement = dynamic_cast<NotationElement*>(element);
    
    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    if (!staff) return;
    m_selectedStaff = staff;

    if (m_clickedElement) {

	// should be safe, as we've already set m_justSelectedBar false
	handleLeftButtonPress(t, height, staffNo, e, element);
	return;

    } else {

	m_selectionRect->setX(staff->getX());
	m_selectionRect->setY(staff->getY());
	m_selectionRect->setSize(int(staff->getTotalWidth()) - 1,
				 staff->getTotalHeight() - 1);

	m_selectionRect->show();
	m_updateRect = false;
    }
    
    m_wholeStaffSelectionComplete = true;

    return;
}

int NotationSelector::handleMouseMove(timeT, int,
				      QMouseEvent* e)
{
    if (!m_updateRect) return RosegardenCanvasView::NoFollow;

    int w = int(e->x() - m_selectionRect->x());
    int h = int(e->y() - m_selectionRect->y());

    if (m_clickedElement /* && !m_clickedElement->isRest() */) {

	if (m_startedFineDrag) {
	    dragFine(e->x(), e->y(), false);
	} else if (m_clickedShift) {
	    if (w > 2 || w < -2 || h > 2 || h < -2) {
		dragFine(e->x(), e->y(), false);
	    }
	} else if (w > 3 || w < -3 || h > 3 || h < -3) {
	    drag(e->x(), e->y(), false);
	}

    } else {

	// Qt rectangle dimensions appear to be 1-based
	if (w > 0) ++w; else --w;
	if (h > 0) ++h; else --h;

	m_selectionRect->setSize(w,h);
	setViewCurrentSelection(true);
	m_nParentView->canvas()->update();
    }

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void NotationSelector::handleMouseRelease(timeT, int, QMouseEvent *e)
{
    NOTATION_DEBUG << "NotationSelector::handleMouseRelease" << endl;
    m_updateRect = false;

    NOTATION_DEBUG << "selectionRect width, height: " << m_selectionRect->width()
		   << ", " << m_selectionRect->height() << endl;

    // Test how far we've moved from the original click position -- not
    // how big the rectangle is (if we were dragging an event, the
    // rectangle size will still be zero).
    
    if (((e->x() - m_selectionRect->x()) > -3 &&
	 (e->x() - m_selectionRect->x()) <  3 &&
	 (e->y() - m_selectionRect->y()) > -3 &&
	 (e->y() - m_selectionRect->y()) <  3) &&
	!m_startedFineDrag) {

	if (m_clickedElement != 0 && m_selectedStaff) {
		
	    // If we didn't drag out a meaningful area, but _did_
	    // click on an individual event, then select just that
	    // event

	    if (m_selectionToMerge &&
		m_selectionToMerge->getSegment() ==
		m_selectedStaff->getSegment()) {

		m_selectionToMerge->addEvent(m_clickedElement->event());
		m_nParentView->setCurrentSelection(m_selectionToMerge,
						   true, true);
		m_selectionToMerge = 0;

	    } else {

		m_nParentView->setSingleSelectedEvent
		    (m_selectedStaff->getId(), m_clickedElement->event(),
		     true, true);
	    }
/*
	} else if (m_selectedStaff) {

	    // If we clicked on no event but on a staff, move the
	    // insertion cursor to the point where we clicked. 
	    // Actually we only really want this to happen if
	    // we aren't double-clicking -- consider using a timer
	    // to establish whether a double-click is going to happen

	    m_nParentView->slotSetInsertCursorPosition(e->x(), (int)e->y());
*/
	} else {
	    setViewCurrentSelection(false);
	}

    } else {

	if (m_startedFineDrag) {
	    dragFine(e->x(), e->y(), true);
	} else if (m_clickedElement /* && !m_clickedElement->isRest() */) {
	    drag(e->x(), e->y(), true);
	} else {
	    setViewCurrentSelection(false);
	}
    }

    m_clickedElement = 0;
    m_selectionRect->hide();
    m_wholeStaffSelectionComplete = false;
    m_nParentView->canvas()->update();
}

void NotationSelector::drag(int x, int y, bool final)
{
    NOTATION_DEBUG << "NotationSelector::drag " << x << ", " << y << endl;

    if (!m_clickedElement || !m_selectedStaff) return;

    EventSelection *selection = m_nParentView->getCurrentSelection();
    if (!selection || !selection->contains(m_clickedElement->event())) {
	selection = new EventSelection(m_selectedStaff->getSegment());
	selection->addEvent(m_clickedElement->event());
    }
    m_nParentView->setCurrentSelection(selection);

    NotationStaff *targetStaff = dynamic_cast<NotationStaff *>
	(m_nParentView->getStaffForCanvasCoords(x, y));
    if (!targetStaff) targetStaff = m_selectedStaff;

    // Calculate time and height
    
    timeT clickedTime = m_clickedElement->event()->getNotationAbsoluteTime();

    Accidental clickedAccidental = Rosegarden::Accidentals::NoAccidental;
    (void)m_clickedElement->event()->get<String>(ACCIDENTAL, clickedAccidental);

    long clickedPitch = 0;
    (void)m_clickedElement->event()->get<Rosegarden::Int>(PITCH, clickedPitch);

    long clickedHeight = 0;
    (void)m_clickedElement->event()->get<Rosegarden::Int>
	(NotationProperties::HEIGHT_ON_STAFF, clickedHeight);

    Event *clefEvt = 0, *keyEvt = 0;
    Rosegarden::Clef clef;
    Rosegarden::Key key;

    timeT dragTime = clickedTime;
    double layoutX = m_clickedElement->getLayoutX();
    timeT duration = m_clickedElement->getViewDuration();

    NotationElementList::iterator itr =
	targetStaff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (itr != targetStaff /* m_selectedStaff */ ->getViewElementList()->end()) {

	NotationElement *elt = dynamic_cast<NotationElement *>(*itr);
	dragTime = elt->getViewAbsoluteTime();
	layoutX  = elt->getLayoutX();

	if (elt->isRest() && duration > 0 && elt->getCanvasItem()) {

	    double restX = 0, restWidth = 0;
	    elt->getCanvasAirspace(restX, restWidth);
	    
	    timeT restDuration = elt->getViewDuration();

	    if (restWidth > 0 &&
		restDuration >= duration * 2) {

		int parts = restDuration / duration;
		double encroachment = x - restX;
		NOTATION_DEBUG << "encroachment is " << encroachment << ", restWidth is " << restWidth << endl;
		int part = (int)((encroachment / restWidth) * parts);
		if (part >= parts) part = parts-1;

		dragTime += part * restDuration / parts;
		layoutX  += part * restWidth / parts +
		    (restX - elt->getCanvasX());
	    }
	}
    }

    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);
    if (keyEvt) key = Rosegarden::Key(*keyEvt);
    
    int height = targetStaff->getHeightAtCanvasCoords(x, y);
    int pitch = clickedPitch;

    if (height != clickedHeight)
	pitch = 
	    Rosegarden::Pitch
	    (height, clef, key, clickedAccidental).getPerformancePitch();

    if (pitch < clickedPitch) {
	if (height < -10) {
	    height = -10;
	    pitch = Rosegarden::Pitch
		(height, clef, key, clickedAccidental).getPerformancePitch();
	}
    } else if (pitch > clickedPitch) {
	if (height > 18) {
	    height = 18;
	    pitch = Rosegarden::Pitch
		(height, clef, key, clickedAccidental).getPerformancePitch();
	}
    }	    

    bool singleNonNotePreview = !m_clickedElement->isNote() &&
	selection->getSegmentEvents().size() == 1;

    if (!final && !singleNonNotePreview) {

	if ((pitch != m_lastDragPitch || dragTime != m_lastDragTime) &&
	    m_clickedElement->isNote()) {
	    
	    m_nParentView->showPreviewNote(targetStaff->getId(),
					   layoutX, pitch, height,
					   Note::getNearestNote(duration));
	    m_lastDragPitch = pitch;
	    m_lastDragTime = dragTime;
	}

    } else {

	m_nParentView->clearPreviewNote();

	KMacroCommand *command = new KMacroCommand(MoveCommand::getGlobalName());
	bool haveSomething = false;

	MoveCommand *mc = 0;
	Event *lastInsertedEvent = 0;

	if (pitch != clickedPitch && m_clickedElement->isNote()) {
	    command->addCommand(new TransposeCommand(pitch - clickedPitch,
						     *selection));
	    haveSomething = true;
	}

	if (targetStaff != m_selectedStaff) {
	    command->addCommand(new MoveAcrossSegmentsCommand
				(m_selectedStaff->getSegment(),
				 targetStaff->getSegment(),
				 dragTime - clickedTime + selection->getStartTime(),
				 true,
				 *selection));
	    haveSomething = true;
	} else {
	    if (dragTime != clickedTime) {
		mc = new MoveCommand
		    (m_selectedStaff->getSegment(), //!!!sort
		     dragTime - clickedTime, true, *selection);
		command->addCommand(mc);
		haveSomething = true;
	    }
	}

	if (haveSomething) {

	    m_nParentView->addCommandToHistory(command);

	    if (mc && singleNonNotePreview) {

		lastInsertedEvent = mc->getLastInsertedEvent();

		if (lastInsertedEvent) {
		    m_nParentView->setSingleSelectedEvent(targetStaff->getId(),
							  lastInsertedEvent);

		    Rosegarden::ViewElementList::iterator vli =
			targetStaff->findEvent(lastInsertedEvent);

		    if (vli != targetStaff->getViewElementList()->end()) {
			m_clickedElement = dynamic_cast<NotationElement *>(*vli);
		    } else {
			m_clickedElement = 0;
		    }

		    m_selectionRect->setX(x);
		    m_selectionRect->setY(y);
		}
	    }
	} else {
	    delete command;
	}
    }
}

void NotationSelector::dragFine(int x, int y, bool final)
{
    NOTATION_DEBUG << "NotationSelector::drag " << x << ", " << y << endl;

    if (!m_clickedElement || !m_selectedStaff) return;

    EventSelection *selection = m_nParentView->getCurrentSelection();
    if (!selection) selection = new EventSelection(m_selectedStaff->getSegment());
    if (!selection->contains(m_clickedElement->event()))
	 selection->addEvent(m_clickedElement->event());
    m_nParentView->setCurrentSelection(selection);

    // Fine drag modifies the DISPLACED_X and DISPLACED_Y properties on
    // each event.  The modifications have to be relative to the previous
    // values of these properties, not to zero, so for each event we need
    // to store the previous value at the time the drag starts.

    static Rosegarden::PropertyName xProperty("temporary-displaced-x");
    static Rosegarden::PropertyName yProperty("temporary-displaced-y");

    if (!m_startedFineDrag) {
	// back up original properties

	for (EventSelection::eventcontainer::iterator i =
		selection->getSegmentEvents().begin();
	     i != selection->getSegmentEvents().end(); ++i) {
	    long prevX = 0, prevY = 0;
	    (*i)->get<Int>(DISPLACED_X, prevX);
	    (*i)->get<Int>(DISPLACED_Y, prevY);
	    (*i)->setMaybe<Int>(xProperty, prevX);
	    (*i)->setMaybe<Int>(yProperty, prevY);
	}

	m_startedFineDrag = true;
    }

    // We want the displacements in 1/1000ths of a staff space

    double dx = x - m_selectionRect->x();
    double dy = y - m_selectionRect->y();

    double noteBodyWidth = m_nParentView->getNotePixmapFactory()->getNoteBodyWidth();
    double lineSpacing = m_nParentView->getNotePixmapFactory()->getLineSpacing();
    dx = (1000.0 * dx) / noteBodyWidth;
    dy = (1000.0 * dy) / lineSpacing;

    if (final) {

	// reset original values (and remove backup values) before
	// applying command

	for (EventSelection::eventcontainer::iterator i =
		 selection->getSegmentEvents().begin();
	     i != selection->getSegmentEvents().end(); ++i) {
	    long prevX = 0, prevY = 0;
	    (*i)->get<Int>(xProperty, prevX);
	    (*i)->get<Int>(yProperty, prevY);
	    (*i)->setMaybe<Int>(DISPLACED_X, prevX);
	    (*i)->setMaybe<Int>(DISPLACED_Y, prevY);
	    (*i)->unset(xProperty);
	    (*i)->unset(yProperty);
	}

	IncrementDisplacementsCommand *command = new IncrementDisplacementsCommand
	    (*selection, long(dx), long(dy));
	m_nParentView->addCommandToHistory(command);

    } else {

	Rosegarden::timeT startTime = 0, endTime = 0;

	for (EventSelection::eventcontainer::iterator i =
		 selection->getSegmentEvents().begin();
	     i != selection->getSegmentEvents().end(); ++i) {
	    long prevX = 0, prevY = 0;
	    (*i)->get<Int>(xProperty, prevX);
	    (*i)->get<Int>(yProperty, prevY);
	    (*i)->setMaybe<Int>(DISPLACED_X, prevX + long(dx));
	    (*i)->setMaybe<Int>(DISPLACED_Y, prevY + long(dy));
	    if (i == selection->getSegmentEvents().begin()) {
		startTime = (*i)->getAbsoluteTime();
	    }
	    endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
	}
	
	if (startTime == endTime) ++endTime;
	selection->getSegment().updateRefreshStatuses(startTime, endTime);
	m_nParentView->update();
    }
}


void NotationSelector::ready()
{
    m_selectionRect = new QCanvasRectangle(m_nParentView->canvas());
    
    m_selectionRect->hide();
    m_selectionRect->setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::SelectionRectangle));

    m_nParentView->setCanvasCursor(Qt::arrowCursor);
    m_nParentView->setHeightTracking(false);
}

void NotationSelector::stow()
{
    delete m_selectionRect;
    m_selectionRect = 0;
    m_nParentView->canvas()->update();
}


void NotationSelector::slotHideSelection()
{
    if (!m_selectionRect) return;
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_nParentView->canvas()->update();
}

void NotationSelector::slotInsertSelected()
{
    m_nParentView->slotLastNoteAction();
}

void NotationSelector::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

//void NotationSelector::slotCollapseRests()
//{
//    m_parentView->actionCollection()->action("collapse_rests")->activate();
//}

void NotationSelector::slotCollapseRestsHard()
{
    m_parentView->actionCollection()->action("collapse_rests_aggressively")->activate();
}

void NotationSelector::slotRespellFlat()
{
    m_parentView->actionCollection()->action("respell_flat")->activate();
}

void NotationSelector::slotRespellSharp()
{
    m_parentView->actionCollection()->action("respell_sharp")->activate();
}

void NotationSelector::slotRespellNatural()
{
    m_parentView->actionCollection()->action("respell_natural")->activate();
}

void NotationSelector::slotCollapseNotes()
{
    m_parentView->actionCollection()->action("collapse_notes")->activate();
}

void NotationSelector::slotInterpret()
{
    m_parentView->actionCollection()->action("interpret")->activate();
}

void NotationSelector::slotMakeInvisible()
{
    m_parentView->actionCollection()->action("make_invisible")->activate();
}

void NotationSelector::slotMakeVisible()
{
    m_parentView->actionCollection()->action("make_visible")->activate();
}


EventSelection* NotationSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

    //    NOTATION_DEBUG << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) return 0;

    QCanvasItemList itemList = m_selectionRect->collisions(false);
    QCanvasItemList::Iterator it;

    QRect rect = m_selectionRect->rect().normalize();
    QCanvasNotationSprite *sprite = 0;

    if (!m_selectedStaff) {

        // Scan the list of collisions, looking for a valid notation
	// element; if we find one, initialise m_selectedStaff from it.
	// If we don't find one, we have no selection.  This is a little
	// inefficient but we only do it for the first event in the
	// selection.

	for (it = itemList.begin(); it != itemList.end(); ++it) {
        
	    if ((sprite = dynamic_cast<QCanvasNotationSprite*>(*it))) {

		NotationElement &el = sprite->getNotationElement();

		NotationStaff *staff = getStaffForElement(&el);
		if (!staff) continue;
	    
		int x = (int)(*it)->x();
		bool shifted = false;
		int nbw = staff->getNotePixmapFactory(false).getNoteBodyWidth();

		
		// #957364 (Notation: Hard to select upper note in
		// chords of seconds) -- adjust x-coord for shifted
		// note head
		if (el.event()->get<Rosegarden::Bool>
		    (staff->getProperties().NOTE_HEAD_SHIFTED, shifted) && shifted) {
		    x += nbw;
		}

		if (!rect.contains(x, int((*it)->y()), true)) {
		    // #988217 (Notation: Special column of pixels
		    // prevents sweep selection) -- for notes, test
		    // again with centred x-coord
		    if (!el.isNote() || !rect.contains(x + nbw/2, int((*it)->y()), true)) {
			continue;
		    }
		}
            
		m_selectedStaff = staff;
		break;
	    }
	}
    }

    if (!m_selectedStaff) return 0;
    Segment& originalSegment = m_selectedStaff->getSegment();

    // If we selected the whole staff, force that to happen explicitly
    // rather than relying on collisions with the rectangle -- because
    // events way off the currently visible area might not even have
    // been drawn yet, and so will not appear in the collision list.
    // (We did still need the collision list to determine which staff
    // to use though.)
    
    if (m_wholeStaffSelectionComplete) {
	EventSelection *selection = new EventSelection(originalSegment,
						       originalSegment.getStartTime(),
						       originalSegment.getEndMarkerTime());
	return selection;
    }
	
    EventSelection* selection = new EventSelection(originalSegment);

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(*it))) {

	    NotationElement &el = sprite->getNotationElement();
	    
	    int x = (int)(*it)->x();
	    bool shifted = false;
	    int nbw = m_selectedStaff->getNotePixmapFactory(false).getNoteBodyWidth();

	    // #957364 (Notation: Hard to select upper note in chords
	    // of seconds) -- adjust x-coord for shifted note head
	    if (el.event()->get<Rosegarden::Bool>
		(m_selectedStaff->getProperties().NOTE_HEAD_SHIFTED, shifted) && shifted) {
		x += nbw;
	    }

            // check if the element's rect
            // is actually included in the selection rect.
            //
            if (!rect.contains(x, int((*it)->y()), true))  {
		// #988217 (Notation: Special column of pixels
		// prevents sweep selection) -- for notes, test again
		// with centred x-coord
		if (!el.isNote() || !rect.contains(x + nbw/2, int((*it)->y()), true)) {
		    continue;
		}
	    }
            
	    // must be in the same segment as we first started on,
	    // we can't select events across multiple segments
	    if (selection->getSegment().findSingle(el.event()) !=
		selection->getSegment().end()) {
		selection->addEvent(el.event());
	    }
        }
    }

    if (selection->getAddedEvents() > 0) {
	return selection;
    } else {
	delete selection;
	return 0;
    }
}

void NotationSelector::setViewCurrentSelection(bool preview)
{
    EventSelection *selection = getSelection();
    
    if (m_selectionToMerge) {
	if (selection &&
	    m_selectionToMerge->getSegment() == selection->getSegment()) {
	    selection->addFromSelection(m_selectionToMerge);
	} else {
	    return; 
	}
    }

    m_nParentView->setCurrentSelection(selection, preview, true);
}

NotationStaff *
NotationSelector::getStaffForElement(NotationElement *elt)
{
    for (int i = 0; i < m_nParentView->getStaffCount(); ++i) {
	NotationStaff *staff = m_nParentView->getStaff(i);
	if (staff->getSegment().findSingle(elt->event()) !=
	    staff->getSegment().end()) return staff;
    }
    return 0;
}

//------------------------------

FretboardInserter::FretboardInserter(NotationView* view)
        : NotationTool("FretboardInserter", view),
	m_guitarChord_ref (m_nParentView)
{
    QIconSet icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                             makeToolbarPixmap("select")));

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    icon = QIconSet
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                         makeToolbarPixmap("crotchet")));

    new KAction(i18n("Switch to Inserting Notes"), icon, 0, this,
                SLOT(slotNoteSelected()), actionCollection(),
                "notes");

    m_guitarChord_ref.init();
    createMenu("fretboardinserter.rc");
}

void FretboardInserter::slotFretboardSelected()
{
    // Switch to last selected Fretboard
    // m_nParentView->slotLastFretboardAction();
}

void FretboardInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void FretboardInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

void FretboardInserter::handleLeftButtonPress(Rosegarden::timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    std::cout << "FretboardInserter::handleLeftButtonPress" << std::endl;

    if (staffNo < 0)
    {
        return;
    }

    NotationStaff *staff = m_nParentView->getStaff(staffNo);

    if (element && element->event()->isa(guitar::Fingering::EventType))
    {
        handleSelectedFretboard (element, staff);
    }
    else
    {
        createNewFretboard (element, staff, e);
    }
}

bool FretboardInserter::processDialog( NotationStaff* staff,
                                       Rosegarden::timeT& insertionTime)
{
    bool result = false;

    if (m_guitarChord_ref.exec() == QDialog::Accepted)
    {
        guitar::Fingering m_chord = m_guitarChord_ref.getArrangement();

        FretboardInsertionCommand *command =
            new FretboardInsertionCommand
            (staff->getSegment(), insertionTime, m_chord);

        m_nParentView->addCommandToHistory(command);
        result = true;
    }

    return result;
}

void FretboardInserter::handleSelectedFretboard (ViewElement* element, NotationStaff *staff)
{
    std::cout << "FretboardInserter::handleSelectedFretboard" << std::endl;


    // Get time of where fretboard is inserted
    timeT insertionTime = element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    // edit an existing fretboard, if that's what we clicked on
    try
    {
        guitar::Fingering existingFret (*element->event());

        m_guitarChord_ref.setArrangement( &existingFret );
        if ( processDialog( staff, insertionTime ) )
        {
            // Erase old fretboard
            EraseEventCommand *command =
                new EraseEventCommand(staff->getSegment(),
                                      element->event(),
                                      false);

            m_nParentView->addCommandToHistory(command);
        }
    }
    catch (Rosegarden::Exception e)
    {}
}

void FretboardInserter::createNewFretboard (ViewElement* element, NotationStaff *staff, QMouseEvent* e)
{
    std::cout << "FretboardInserter::createNewFretboard" << std::endl;
    Event *clef = 0, *key = 0;

    NotationElementList::iterator closestElement =
        staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
                                               clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end())
    {
        return;
    }

    timeT insertionTime = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    processDialog( staff, insertionTime );
}


//------------------------------

const QString NoteInserter::ToolName     = "noteinserter";
const QString RestInserter::ToolName     = "restinserter";
const QString ClefInserter::ToolName     = "clefinserter";
const QString TextInserter::ToolName     = "textinserter";
const QString NotationEraser::ToolName   = "notationeraser";
const QString NotationSelector::ToolName = "notationselector";
const QString FretboardInserter::ToolName = "fretboardinserter";


//----------------------------------------------------------------------
//               Unused
//----------------------------------------------------------------------

NotationSelectionPaster::NotationSelectionPaster(EventSelection& es,
                                                 NotationView* view)
    : NotationTool("NotationPaster", view),
      m_selection(es)
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
}

NotationSelectionPaster::~NotationSelectionPaster()
{
}

void NotationSelectionPaster::handleLeftButtonPress(Rosegarden::timeT,
                                                    int,
                                                    int staffNo,
                                                    QMouseEvent* e,
                                                    ViewElement*)
{
    if (staffNo < 0) return;
    Event *clef = 0, *key = 0;

    NotationStaff *staff = m_nParentView->getStaff(staffNo);
    
    NotationElementList::iterator closestElement =
	staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
					       clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end()) return;

    timeT time = (*closestElement)->getViewAbsoluteTime();

    Segment& segment = staff->getSegment();
    PasteEventsCommand *command = new PasteEventsCommand
	(segment, m_parentView->getDocument()->getClipboard(), time,
	 PasteEventsCommand::Restricted);

    if (!command->isPossible()) {
	m_parentView->slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
	m_parentView->addCommandToHistory(command);
	m_parentView->slotStatusHelpMsg(i18n("Ready."));
    }
}

#include "notationtool.moc"
