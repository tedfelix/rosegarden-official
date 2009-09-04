/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
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


#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/ViewElement.h"
#include "base/Composition.h"
#include "commands/notation/NoteInsertionCommand.h"
#include "commands/notation/RestInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "document/CommandHistory.h"
#include "gui/general/IconLoader.h"
#include "NotationProperties.h"
#include "NotationMouseEvent.h"
#include "NotationStrings.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotationStaff.h"
#include "NotationScene.h"
#include "NotePixmapFactory.h"
#include "NoteStyleFactory.h"
#include "document/Command.h"

#include <QAction>
#include <QSettings>
#include <QIcon>
#include <QRegExp>
#include <QString>


namespace Rosegarden
{

NoteInserter::NoteInserter(NotationWidget* widget) :
    NotationTool("noteinserter.rc", "NoteInserter", widget),
    m_noteType(Note::Quaver),
    m_noteDots(0),
    m_autoBeam(true),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(false) // this ctor isn't used by RestInserter, so this can't be true
{
    QIcon icon;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_autoBeam = qStrToBool( settings.value("autobeam", "true" ) ) ;
    m_matrixInsertType = (settings.value("inserttype", 0).toInt()  > 0);
    m_defaultStyle = settings.value("style", NoteStyleFactory::DefaultStyle).toString();
    settings.endGroup();

    QAction *a;

    a = createAction("toggle_auto_beam", SLOT(slotToggleAutoBeam()));
    if (m_autoBeam) { a->setCheckable(true); a->setChecked(true); }

    for (unsigned int i = 0; i < 6; ++i) {
        a = createAction(m_actionsAccidental[i][1], m_actionsAccidental[i][0]);
    }

    createAction("toggle_dot", SLOT(slotToggleDot()));

    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("rests", SLOT(slotRestsSelected()));

    connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
            this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");
}

NoteInserter::NoteInserter(QString rcFileName, QString menuName,
                           NotationWidget* widget) :
    NotationTool(rcFileName, menuName, widget),
    m_noteType(Note::Quaver),
    m_noteDots(0),
    m_autoBeam(false),
    m_clickHappened(false),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(menuName == "RestInserter") // we'll only return true if we were really called by RestInserter,
                                                  // which always sets that string thus when calling us (rubbish code FTW!)

{
    // Constructor used by subclass (e.g. RestInserter)

    connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
            this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");

    //!!! grace & triplet mode should be stored by this tool, not by widget!

    //!!! selection should be in scene, not widget!
}

NoteInserter::~NoteInserter()
{}

void NoteInserter::ready()
{
    m_clickHappened = false;
    m_clickStaff = 0;
    m_widget->setCanvasCursor(Qt::crossCursor);
//!!!   m_widget->setHeightTracking(true);
}

void
NoteInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    computeLocationAndPreview(e);
}

NoteInserter::FollowMode
NoteInserter::handleMouseMove(const NotationMouseEvent *e)
{
    if (m_clickHappened) {
        computeLocationAndPreview(e);
    }

    return NoFollow;
}

void
NoteInserter::handleMouseRelease(const NotationMouseEvent *e)
{
    NOTATION_DEBUG << "NoteInserter::handleMouseRelease: staff = " <<
        m_clickStaff << ", clicked = " << m_clickHappened << endl;
    
    NotationStaff *staff = m_clickStaff;
    if (!m_clickHappened || !staff) return;

    bool okay = computeLocationAndPreview(e);
    clearPreview();
    m_clickHappened = false;
    m_clickStaff = 0;
    if (!okay) return;

    Note note(m_noteType, m_noteDots);
    timeT endTime = m_clickTime + note.getDuration();
    Segment &segment = staff->getSegment();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (!segment.isBeforeEndMarker(realEnd) ||
        !segment.isBeforeEndMarker(++realEnd)) {
        endTime = segment.getEndMarkerTime();
    } else {
        endTime = std::max(endTime, (*realEnd)->getNotationAbsoluteTime());
    }

    Event *lastInsertedEvent =
        doAddCommand
        (segment, m_clickTime, endTime, note, m_clickPitch,
         ((m_accidental == Accidentals::NoAccidental && m_followAccidental) ?
          m_lastAccidental : m_accidental));

    if (lastInsertedEvent) {

        m_scene->setSingleSelectedEvent(&segment, lastInsertedEvent, false);

        if (m_widget->isInChordMode()){
            m_widget->setPointerPosition(lastInsertedEvent->getAbsoluteTime());

        } else {
            m_widget->setPointerPosition(lastInsertedEvent->getAbsoluteTime() +
                                         lastInsertedEvent->getDuration());
        }
/*!!!
        if (m_widget->isInChordMode()) {
            m_widget->slotSetInsertCursorAndRecentre
            (lastInsertedEvent->getAbsoluteTime(), e->x(), (int)e->y(),
             false);
        } else {
            m_widget->slotSetInsertCursorAndRecentre
            (lastInsertedEvent->getAbsoluteTime() +
             lastInsertedEvent->getDuration(), e->x(), (int)e->y(),
             false);
        }
*/
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

        m_scene->setSingleSelectedEvent(&segment, lastInsertedEvent, false);

        if (!m_widget->isInChordMode()){
            m_widget->setPointerPosition(lastInsertedEvent->getAbsoluteTime()
                                         +lastInsertedEvent->getDuration());
        }
//        if (m_widget->isInChordMode()) {
//            if (m_scene) {
//                m_scene->slotSetInsertCursorPosition
//                    (lastInsertedEvent->getAbsoluteTime(), true, false);
//            }
//        } else {
//            if (m_scene) {
//                m_scene->slotSetInsertCursorPosition
//                    (lastInsertedEvent->getAbsoluteTime() +
//                     lastInsertedEvent->getDuration(), true, false);
//            }
//        }
    }

    if (!suppressPreview) {
        if (m_scene) {
            m_scene->playNote(segment, pitch);
        }
    }
}

bool
NoteInserter::computeLocationAndPreview(const NotationMouseEvent *e)
{
    if (!e->staff || !e->element) {
        NOTATION_DEBUG << "computeLocationAndPreview: staff and/or element not supplied" << endl;
        clearPreview();
        return false;
    }

    if (m_clickHappened && (e->staff != m_clickStaff)) {
        NOTATION_DEBUG << "computeLocationAndPreview: staff changed from originally clicked one (" << e->staff << " vs " << m_clickStaff << ")" << endl;
        // abandon
        clearPreview();
        return false;
    }

    double x = e->sceneX;
    int y = e->sceneY;

    // If we're inserting grace notes, then we need to "dress to the
    // right", as it were
    bool grace = m_widget->isInGraceMode();

    NotationElement *el = e->element;
    ViewElementList::iterator itr = e->staff->getViewElementList()->findSingle(el);
    if (itr == e->staff->getViewElementList()->end()) {
        NOTATION_DEBUG << "computeLocationAndPreview: element provided is not found in staff" << endl;
        return false;
    }

    timeT time = el->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    m_clickInsertX = el->getLayoutX();

    int subordering = el->event()->getSubOrdering();
    float targetSubordering = subordering;

    if (grace && el->getItem()) {

        std::cerr << "x=" << x << ", el->getSceneX()=" << el->getSceneX() << std::endl;

        if (el->isRest()) std::cerr << "elt is a rest" << std::endl;
        if (x - el->getSceneX() >
            e->staff->getNotePixmapFactory(false).getNoteBodyWidth()) {
            NotationElementList::iterator j(itr);
            while (++j != e->staff->getViewElementList()->end()) {
                NotationElement *candidate = static_cast<NotationElement *>(*j);
                if ((candidate->isNote() || candidate->isRest()) &&
                    (candidate->getViewAbsoluteTime() > el->getViewAbsoluteTime() ||
                     candidate->event()->getSubOrdering() > el->event()->getSubOrdering())) {
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

        if (x - el->getSceneX() < 1) {
            targetSubordering -= 0.5;
        }
    }

    if (el->isRest() && el->getItem()) {
        time += getOffsetWithinRest(e->staff, itr, x);
        m_clickInsertX += (x - el->getSceneX());
    }

    Pitch p(e->height, e->clef, e->key, m_accidental);
    int pitch = p.getPerformancePitch();

    // [RFE 987960] When inserting via mouse, if no accidental is
    // selected, we use the same accidental (and thus the same pitch)
    // as of the previous note found at this height -- iff such a note
    // is found more recently than the last key signature.

    if (m_accidental == Accidentals::NoAccidental &&
        m_followAccidental) {
        Segment &segment = e->staff->getSegment();
        m_lastAccidental = m_accidental;
        Segment::iterator i = segment.findNearestTime(time);
        while (i != segment.end()) {
            if ((*i)->isa(Rosegarden::Key::EventType)) break;
            if ((*i)->isa(Note::EventType)) {
                if ((*i)->has(NotationProperties::HEIGHT_ON_STAFF) &&
                    (*i)->has(BaseProperties::PITCH)) {
                    int h = (*i)->get<Int>(NotationProperties::HEIGHT_ON_STAFF);
                    if (h == e->height) {
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
            e->height != m_clickHeight ||
            e->staff != m_clickStaff) {
            changed = true;
        }
    } else {
        m_clickHappened = true;
        m_clickStaff = e->staff;
        changed = true;
    }

    if (changed) {
        m_clickTime = time;
        m_clickSubordering = subordering;
        m_clickPitch = pitch;
        m_clickHeight = e->height;
        m_clickStaff = e->staff;
        m_targetSubordering = targetSubordering;

        showPreview();
    }

    return true;
}

void NoteInserter::showPreview()
{
    if (!m_clickStaff) return;
    Segment &segment = m_clickStaff->getSegment();

    int pitch = m_clickPitch;
    pitch += getOttavaShift(segment, m_clickTime) * 12;

    if (m_scene) {
        m_scene->showPreviewNote(m_clickStaff, m_clickInsertX,
                                 pitch, m_clickHeight,
                                 Note(m_noteType, m_noteDots),
                                 m_widget->isInGraceMode());
    }
}

void NoteInserter::clearPreview()
{
    if (m_scene) {
        m_scene->clearPreviewNote(m_clickStaff);
    }
}

timeT
NoteInserter::getOffsetWithinRest(NotationStaff *staff,
                                  const NotationElementList::iterator &i,
                                  double &sceneX) // will be snapped
{
    //!!! To make this work correctly in tuplet mode, our divisor would
    // have to be the tupletified duration of the tuplet unit -- we can
    // do that, we just haven't yet
    if (m_widget->isInTripletMode())
        return 0;

    NotationElement* el = static_cast<NotationElement*>(*i);
    if (!el->getItem()) return 0;
    double offset = sceneX - el->getSceneX();

    if (offset < 0) return 0;

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
        sceneX = el->getSceneX() + offset;

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
    NOTATION_DEBUG << "doAddCommand: time " << time << ", endTime " << endTime
                   << ", pitch " << pitch << endl;

    timeT noteEnd = time + note.getDuration();

    // #1046934: make it possible to insert triplet at end of segment!
    if (m_widget->isInTripletMode()) {
        noteEnd = time + (note.getDuration() * 2 / 3);
    }

    if (time < segment.getStartTime() ||
        endTime > segment.getEndMarkerTime() ||
        noteEnd > segment.getEndMarkerTime()) {
        return 0;
    }

    pitch += getOttavaShift(segment, time) * 12;

    float targetSubordering = 0;
    if (m_widget->isInGraceMode()) {
        targetSubordering = m_targetSubordering;
    }

    NoteInsertionCommand *insertionCommand =
        new NoteInsertionCommand
        (segment, time, endTime, note, pitch, accidental,
         (m_autoBeam && !m_widget->isInTripletMode() && !m_widget->isInGraceMode()) ?
         NoteInsertionCommand::AutoBeamOn : NoteInsertionCommand::AutoBeamOff,
         m_matrixInsertType && !m_widget->isInGraceMode() ?
         NoteInsertionCommand::MatrixModeOn : NoteInsertionCommand::MatrixModeOff,
         m_widget->isInGraceMode() ?
         (m_widget->isInTripletMode() ?
          NoteInsertionCommand::GraceAndTripletModesOn :
          NoteInsertionCommand::GraceModeOn)
         : NoteInsertionCommand::GraceModeOff,
         targetSubordering,
         m_defaultStyle);

    Command *activeCommand = insertionCommand;

    if (m_widget->isInTripletMode() && !m_widget->isInGraceMode()) {
        Segment::iterator i(segment.findTime(time));
        if (i != segment.end() &&
            !(*i)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
            
            MacroCommand *command = new MacroCommand(insertionCommand->getName());
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

    CommandHistory::getInstance()->addCommand(activeCommand);

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
    QAction* dotsAction = findActionInParentView("toggle_dot");
	
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
    invokeInParentView("no_accidental");
}

void NoteInserter::slotFollowAccidental()
{
    invokeInParentView("follow_accidental");
}

void NoteInserter::slotSharp()
{
    invokeInParentView("sharp_accidental");
}

void NoteInserter::slotFlat()
{
    invokeInParentView("flat_accidental");
}

void NoteInserter::slotNatural()
{
    invokeInParentView("natural_accidental");
}

void NoteInserter::slotDoubleSharp()
{
    invokeInParentView("double_sharp_accidental");
}

void NoteInserter::slotDoubleFlat()
{
    invokeInParentView("double_flat_accidental");
}

void NoteInserter::slotToggleDot()
{
    m_noteDots = (m_noteDots) ? 0 : 1;
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");
	
    QAction* action = findActionInParentView(actionName);
	
    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
        action->setChecked(true);
    }
}

void NoteInserter::slotToggleAutoBeam()
{
    m_autoBeam = !m_autoBeam;
}

void NoteInserter::slotEraseSelected()
{
    invokeInParentView("erase");
}

void NoteInserter::slotSelectSelected()
{
    invokeInParentView("select");
}

void NoteInserter::slotRestsSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note, true));
    actionName.replace(QRegExp("-"), "_");
	
    QAction* action = findAction(actionName);
	
    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
        action->trigger();
    }
}

const char* NoteInserter::m_actionsAccidental[][4] =
{
    { "1slotNoAccidental()",  "no_accidental" },
    { "1slotFollowAccidental()",  "follow_accidental" },
    { "1slotSharp()",         "sharp_accidental" },
    { "1slotFlat()",          "flat_accidental" },
    { "1slotNatural()",       "natural_accidental" },
    { "1slotDoubleSharp()",   "double_sharp_accidental" },
    { "1slotDoubleFlat()",    "double_flat_accidental" }
};

const QString NoteInserter::ToolName     = "noteinserter";

}
#include "NoteInserter.moc"

