/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NoteInserter.h"
#include "misc/Debug.h"
#include <QApplication>

#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "commands/notation/NoteInsertionCommand.h"
#include "commands/notation/RestInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/RosegardenCanvasView.h"
#include "NotationProperties.h"
#include "NotationStrings.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotationStaff.h"
#include "NotePixmapFactory.h"
#include "NoteStyleFactory.h"
#include <QAction>
#include "document/Command.h"
#include <QSettings>
#include <QIcon>
#include <QRegExp>
#include <QString>


namespace Rosegarden
{

NoteInserter::NoteInserter(NotationView* view)
        : NotationTool("NoteInserter", view),
        m_noteType(Note::Quaver),
        m_noteDots(0),
        m_autoBeam(true),
        m_accidental(Accidentals::NoAccidental),
        m_lastAccidental(Accidentals::NoAccidental),
        m_followAccidental(false)
{
    QIcon icon;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_autoBeam = qStrToBool( settings.value("autobeam", "true" ) ) ;
    m_matrixInsertType = (settings.value("inserttype", 0).toInt()  > 0);
    m_defaultStyle = qstrtostr(settings.value("style", strtoqstr(NoteStyleFactory::DefaultStyle)).toString());
    settings.endGroup();

    /* was toggle */ 
	QAction *autoBeamAction = new QAction(  i18n("Auto-Beam when appropriate"), dynamic_cast<QObject*>(this) );
   	connect( autoBeamAction, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleAutoBeam()) ); 

	autoBeamAction->setObjectName("toggle_auto_beam");
	autoBeamAction->setCheckable( true );	//
	autoBeamAction->setAutoRepeat( false );	//
    // autoBeamAction->setActionGroup( 0 );	// QActionGroup*
    autoBeamAction->setChecked( m_autoBeam );

    for (unsigned int i = 0; i < 6; ++i) {

        icon = QIcon
               (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                             makeToolbarPixmap(m_actionsAccidental[i][3])));
        KRadioAction* noteAction = new KRadioAction(i18n(m_actionsAccidental[i][0]),
                                   icon, 0, this,
                                   m_actionsAccidental[i][1],
                                   actionCollection(),
                                   m_actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }

    icon = QIcon
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                         makeToolbarPixmap("dotted-crotchet")));
    QAction* qa_toggle_dot = new QAction( icon, i18n("Dotted note"), dynamic_cast<QObject*>(this) );
	connect( qa_toggle_dot, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToogleDot()) );//@@@ slot exists ?	
	qa_toggle_dot->setObjectName( "toggle_dot" );	//### FIX: deallocate QAction ptr
	qa_toggle_dot->setCheckable( true );	//
	qa_toggle_dot->setAutoRepeat( false );	//
	//qa_toggle_dot->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_dot->setChecked( false );	//
	// ;

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                    makeToolbarPixmap("select")));
    QAction *qa_select = new QAction( "Switch to Select Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_select->setIcon(icon); 
			connect( qa_select, SIGNAL(triggered()), this, SLOT(slotSelectSelected())  );

    QAction *qa_erase = new QAction( "Switch to Erase Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_erase->setIconText("eraser"); 
			connect( qa_erase, SIGNAL(triggered()), this, SLOT(slotEraseSelected())  );

    icon = QIcon
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                         makeToolbarPixmap("rest-crotchet")));
    QAction *qa_rests = new QAction( "Switch to Inserting Rests", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_rests->setIcon(icon); 
			connect( qa_rests, SIGNAL(triggered()), this, SLOT(slotRestsSelected())  );

    createMenu("noteinserter.rc");

    connect(m_parentView, SIGNAL(changeAccidental(Accidental, bool)),
            this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    m_parentView->actionCollection()->action("no_accidental")->activate();
}

NoteInserter::NoteInserter(const QString& menuName, NotationView* view)
        : NotationTool(menuName, view),
        m_noteType(Note::Quaver),
        m_noteDots(0),
        m_autoBeam(false),
        m_clickHappened(false),
        m_accidental(Accidentals::NoAccidental),
        m_lastAccidental(Accidentals::NoAccidental),
        m_followAccidental(false)
{
    connect(m_parentView, SIGNAL(changeAccidental(Accidental, bool)),
            this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    m_parentView->actionCollection()->action("no_accidental")->activate();
}

NoteInserter::~NoteInserter()
{}

void NoteInserter::ready()
{
    m_clickHappened = false;
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(true);
}

void
NoteInserter::handleLeftButtonPress(timeT,
                                    int,
                                    int staffNo,
                                    QMouseEvent* e,
                                    ViewElement*)
{
    if (staffNo < 0)
        return ;
    computeLocationAndPreview(e);
}

int
NoteInserter::handleMouseMove(timeT,
                              int,
                              QMouseEvent *e)
{
    if (m_clickHappened) {
        computeLocationAndPreview(e);
    }

    return RosegardenCanvasView::NoFollow;
}

void
NoteInserter::handleMouseRelease(timeT,
                                 int,
                                 QMouseEvent *e)
{
    if (!m_clickHappened)
        return ;
    bool okay = computeLocationAndPreview(e);
    m_clickHappened = false;
    if (!okay)
        return ;
    clearPreview();

    Note note(m_noteType, m_noteDots);
    timeT endTime = m_clickTime + note.getDuration();
    Segment &segment = m_nParentView->getStaff(m_clickStaffNo)->getSegment();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (!segment.isBeforeEndMarker( realEnd) ||
            !segment.isBeforeEndMarker(++realEnd)) {
        endTime = segment.getEndMarkerTime();
    } else {
        endTime = std::max(endTime, (*realEnd)->getNotationAbsoluteTime());
    }

    Event *lastInsertedEvent = doAddCommand
                               (segment, m_clickTime, endTime, note, m_clickPitch,
                                (m_accidental == Accidentals::NoAccidental &&
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
                         int pitch, Accidental accidental,
                         bool suppressPreview)
{
    Note note(m_noteType, m_noteDots);
    timeT endTime = insertionTime + note.getDuration();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (!segment.isBeforeEndMarker( realEnd) ||
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

    if (!suppressPreview)
        m_nParentView->playNote(segment, pitch);
}

bool
NoteInserter::computeLocationAndPreview(QMouseEvent *e)
{
    double x = e->x();
    int y = (int)e->y();

    LinedStaff *staff = m_nParentView->getStaffForCanvasCoords(e->x(), y);
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

    // If we're inserting grace notes, then we need to "dress to the
    // right", as it were
    bool grace = m_nParentView->isInGraceMode();

    int height = staff->getHeightAtCanvasCoords(x, y);

    Event *clefEvt = 0, *keyEvt = 0;
    Clef clef;
    Rosegarden::Key key;

    NotationElementList::iterator itr =
        staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);
    if (itr == staff->getViewElementList()->end()) {
        clearPreview();
        return false;
    }

    NotationElement* el = static_cast<NotationElement*>(*itr);

    timeT time = el->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    m_clickInsertX = el->getLayoutX();
    if (clefEvt)
        clef = Clef(*clefEvt);
    if (keyEvt)
        key = Rosegarden::Key(*keyEvt);

    int subordering = el->event()->getSubOrdering();
    float targetSubordering = subordering;

    if (grace && el->getCanvasItem()) {

        NotationStaff *ns = dynamic_cast<NotationStaff *>(staff);
        if (!ns) {
            std::cerr << "WARNING: NoteInserter: Staff is not a NotationStaff"
                      << std::endl;
        } else {
            std::cerr << "x=" << x << ", el->getCanvasX()=" << el->getCanvasX() << std::endl;
            if (el->isRest()) std::cerr << "elt is a rest" << std::endl;
            if (x - el->getCanvasX() >
                ns->getNotePixmapFactory(false).getNoteBodyWidth()) {
                NotationElementList::iterator j(itr);
                while (++j != staff->getViewElementList()->end()) {
                    NotationElement *candidate = static_cast<NotationElement *>(*j);
                    if ((candidate->isNote() || candidate->isRest()) &&
                        (candidate->getViewAbsoluteTime()
                         > el->getViewAbsoluteTime() ||
                         candidate->event()->getSubOrdering()
                         > el->event()->getSubOrdering())) {
                        itr = j;
                        el = candidate;
                        m_clickInsertX = el->getLayoutX();
                        time = el->event()->getAbsoluteTime();
                        subordering = el->event()->getSubOrdering();
                        targetSubordering = subordering;
                        break;
                    }
                }
            }
        }

        if (x - el->getCanvasX() < 1) {
            targetSubordering -= 0.5;
        }
    }

    if (el->isRest() && el->getCanvasItem()) {
        time += getOffsetWithinRest(staffNo, itr, x);
        m_clickInsertX += (x - el->getCanvasX());
    }

    Pitch p(height, clef, key, m_accidental);
    int pitch = p.getPerformancePitch();

    // [RFE 987960] When inserting via mouse, if no accidental is
    // selected, we use the same accidental (and thus the same pitch)
    // as of the previous note found at this height -- iff such a note
    // is found more recently than the last key signature.

    if (m_accidental == Accidentals::NoAccidental &&
        m_followAccidental) {
        Segment &segment = staff->getSegment();
        m_lastAccidental = m_accidental;
        Segment::iterator i = segment.findNearestTime(time);
        while (i != segment.end()) {
            if ((*i)->isa(Rosegarden::Key::EventType)) break;
            if ((*i)->isa(Note::EventType)) {
                if ((*i)->has(NotationProperties::HEIGHT_ON_STAFF) &&
                    (*i)->has(BaseProperties::PITCH)) {
                    int h = (*i)->get<Int>(NotationProperties::HEIGHT_ON_STAFF);
                    if (h == height) {
                        pitch = (*i)->get<Int>(BaseProperties::PITCH);
                        (*i)->get<String>(BaseProperties::ACCIDENTAL,
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
            subordering != m_clickSubordering ||
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
        m_clickSubordering = subordering;
        m_clickPitch = pitch;
        m_clickHeight = height;
        m_clickStaffNo = staffNo;
        m_targetSubordering = targetSubordering;

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
                                   Note(m_noteType, m_noteDots),
                                   m_nParentView->isInGraceMode());
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
    if (m_nParentView->isInTripletMode())
        return 0;

    Staff *staff = m_nParentView->getStaff(staffNo);
    NotationElement* el = static_cast<NotationElement*>(*i);
    if (!el->getCanvasItem())
        return 0;
    double offset = canvasX - el->getCanvasX();

    if (offset < 0)
        return 0;

    double airX, airWidth;
    el->getLayoutAirspace(airX, airWidth);
    double origin = ((*i)->getLayoutX() - airX) / 2;
    double width = airWidth - origin;

    timeT duration = (*i)->getViewDuration();

    TimeSignature timeSig =
        staff->getSegment().getComposition()->getTimeSignatureAt
        ((*i)->event()->getAbsoluteTime());
    timeT unit = timeSig.getUnitDuration();

    int unitCount = duration / unit;
    if (unitCount > 1) {

        timeT result = (int)((offset / width) * unitCount);
        if (result > unitCount - 1)
            result = unitCount - 1;

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

        if (!segment.isBeforeEndMarker(i)) {
            break;
        }

        if ((*i)->isa(Indication::EventType)) {
            try {
                Indication ind(**i);
                if (ind.isOttavaType()) {
                    timeT endTime =
                        (*i)->getNotationAbsoluteTime() +
                        (*i)->getNotationDuration();
                    if (time < endTime) {
                        ottavaShift = ind.getOttavaShift();
                    }
                    break;
                }
            } catch (...) { }
        }

        if (i == segment.begin()) {
            break;
        }
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

    float targetSubordering = 0;
    if (m_nParentView->isInGraceMode()) {
        targetSubordering = m_targetSubordering;
    }

    NoteInsertionCommand *insertionCommand =
        new NoteInsertionCommand
        (segment, time, endTime, note, pitch, accidental,
         (m_autoBeam && !m_nParentView->isInTripletMode() && !m_nParentView->isInGraceMode()) ?
         NoteInsertionCommand::AutoBeamOn : NoteInsertionCommand::AutoBeamOff,
         m_matrixInsertType && !m_nParentView->isInGraceMode() ?
         NoteInsertionCommand::MatrixModeOn : NoteInsertionCommand::MatrixModeOff,
         m_nParentView->isInGraceMode() ?
         (m_nParentView->isInTripletMode() ?
          NoteInsertionCommand::GraceAndTripletModesOn :
          NoteInsertionCommand::GraceModeOn)
         : NoteInsertionCommand::GraceModeOff,
         targetSubordering,
         m_defaultStyle);

    Command *activeCommand = insertionCommand;

    if (m_nParentView->isInTripletMode() && !m_nParentView->isInGraceMode()) {
        Segment::iterator i(segment.findTime(time));
        if (i != segment.end() &&
            !(*i)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
            
            MacroCommand *command = new MacroCommand(insertionCommand->objectName());

            //## Attempted fix to bug reported on rg-user by SlowPic
            //## <slowpic@web.de> 28/02/2005 22:32:56 UTC: Triplet input error
            //# HJJ: Comment out this attempt. It breaks the splitting of
            //#      the first bars into rests.
            //## if ((*i)->isa(Note::EventRestType) &&
            //## (*i)->getNotationDuration() > (note.getDuration() * 3)) {
            // split the rest
            command->addCommand(new RestInsertionCommand
                                (segment, time,
                                 time + note.getDuration() * 2,
                                 Note::getNearestNote(note.getDuration() * 2)));
            //## }
            //# These comments should probably be deleted.

            command->addCommand(new TupletCommand
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

void NoteInserter::slotSetNote(Note::Type nt)
{
    m_noteType = nt;
}

void NoteInserter::slotSetDots(unsigned int dots)
{
    /* was toggle */ QAction *dotsAction = dynamic_cast<QAction*>
                                (actionCollection()->action("toggle_dot"));
    if (dotsAction && m_noteDots != dots) {
        dotsAction->setChecked(dots > 0);
        slotToggleDot();
        m_noteDots = dots;
    }
}

void NoteInserter::slotSetAccidental(Accidental accidental,
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

const QString NoteInserter::ToolName     = "noteinserter";

}
#include "NoteInserter.moc"
