/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NoteRestInserter.h"
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
#include <QMenu>

namespace Rosegarden
{

NoteRestInserter::NoteRestInserter(NotationWidget* widget) :
    NotationTool("noterestinserter.rc", "NoteRestInserter", widget),
    m_noteType(Note::Quaver),
    m_noteDots(0),
    m_autoBeam(true),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(false)
{
    QIcon icon;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_autoBeam = qStrToBool( settings.value("autobeam", "true" ) ) ;
    m_autoTieBarlines = qStrToBool( settings.value("autotieatbarlines","true"));
    m_matrixInsertType = (settings.value("inserttype", 0).toInt()  > 0);
    m_defaultStyle = settings.value("style", NoteStyleFactory::DefaultStyle).toString();
    settings.endGroup();

    QAction *a;

    a = createAction("toggle_auto_beam", SLOT(slotToggleAutoBeam()));
    if (m_autoBeam) { a->setCheckable(true); a->setChecked(true); }

//  Obsolete?
//    for (unsigned int i = 0; i < 6; ++i) {
//        a = createAction(m_actionsAccidental[i][1], m_actionsAccidental[i][0]);
//    }

    createAction("switch_dots_on", SLOT(slotToggleDot()));
    createAction("switch_dots_off", SLOT(slotToggleDot()));

    createAction("select", SLOT(slotSelectSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("switch_to_notes", SLOT(slotRestsSelected()));
    createAction("switch_to_rests", SLOT(slotNotesSelected()));

    //connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
    //        this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");

}

NoteRestInserter::NoteRestInserter(QString rcFileName, QString menuName,
                           NotationWidget* widget) :
    NotationTool(rcFileName, menuName, widget),
    m_noteType(Note::Quaver),  //OverRide value with NotationView::initializeNoteRestInserter.
    m_noteDots(0),
    m_autoBeam(false),
    m_clickHappened(false),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(false)

{
    //connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
    //        this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");

    //!!! grace & triplet mode should be stored by this tool, not by widget!

    //!!! selection should be in scene, not widget!
}

NoteRestInserter::~NoteRestInserter()
{}

void NoteRestInserter::ready()
{
    m_clickHappened = false;
    m_clickStaff = 0;
    m_widget->setCanvasCursor(Qt::CrossCursor);
//!!!   m_widget->setHeightTracking(true);
}

void
NoteRestInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    computeLocationAndPreview(e);
}

NoteRestInserter::FollowMode
NoteRestInserter::handleMouseMove(const NotationMouseEvent *e)
{
    if (m_clickHappened) {
        computeLocationAndPreview(e);
    }

    return NoFollow;
}

void
NoteRestInserter::handleMouseRelease(const NotationMouseEvent *e)
{
    NOTATION_DEBUG << "NoteRestInserter::handleMouseRelease: staff = " <<
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
          m_lastAccidental : m_accidental), 100);  // Velocity hard coded for now,

    // Note lastInsertedEvent can be null only when a note fails to insert.
    if (lastInsertedEvent) {

        m_scene->setSingleSelectedEvent(&segment, lastInsertedEvent, false);

        if (!m_widget->isInChordMode()) {
            // Since a note could have been split and tied, we need to rely on
            // the full duration of the original note calculate the position of
            // the pointer.
            timeT nextLocation = m_clickTime + note.getDuration();
            m_widget->setPointerPosition(nextLocation);
        } else {
            m_widget->setPointerPosition(m_clickTime);
        }
    }
}

void NoteRestInserter::showMenu()
{
    NOTATION_DEBUG << "NoteRestInserter::showMenu() : enter."
        << endl;
    if (!hasMenu())
        return ;

    if (!m_menu)
        createMenu();

    if (m_menu) {
        NOTATION_DEBUG << "NoteRestInserter::showMenu() : morphing menu."
            << endl;
        //Morph Context menu.
        if (isaRestInserter()) {
            leaveActionState("in_note_mode");
        } else {
            enterActionState("in_note_mode");
        }

        if (!m_noteDots) {
            leaveActionState("in_dot_mode");
        } else {
            enterActionState("in_dot_mode");
        }

        // This code to manage shortest dotted note selection.
        // Disable the shortcut in the menu for shortest duration.
        if (m_noteType == Note::Shortest && !m_noteDots) {
            QAction *switchDots = findAction("switch_dots_on");
            switchDots->setEnabled(false);
            m_menu->exec(QCursor::pos());
            switchDots->setEnabled(true);
        } else {
            m_menu->exec(QCursor::pos());
        }

    } else {
        NOTATION_DEBUG << "NoteRestInserter::showMenu() : no menu to show."
            << endl;
    }
}

void
NoteRestInserter::insertNote(Segment &segment, timeT insertionTime,
                         int pitch, Accidental accidental,
                         int velocity,
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
                               (segment, insertionTime, endTime, note, pitch, accidental,
                                velocity);

    // Note lastInsertedEvent can be null only when a note fails to insert.
    if (lastInsertedEvent) {

        m_scene->setSingleSelectedEvent(&segment, lastInsertedEvent, false);

        if (!m_widget->isInChordMode()) {
            // Since a note could have been split and tied, we need to rely on
            // the full duration of the original note calculate the position of
            // the pointer.
            timeT nextLocation = insertionTime + note.getDuration();
            m_widget->setPointerPosition(nextLocation);
        }
    }

    if (!suppressPreview) {
        if (m_scene) {
            m_scene->playNote(segment, pitch);
        }
    }
}

bool
NoteRestInserter::computeLocationAndPreview(const NotationMouseEvent *e)
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
//    int y = e->sceneY;  //Shut up compile warning about non-use

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

void NoteRestInserter::showPreview()
{
    if (isaRestInserter()) {
        // Sorry, no preview available for rests.
        return;
    }
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

void NoteRestInserter::clearPreview()
{
    if (m_scene) {
        m_scene->clearPreviewNote(m_clickStaff);
    }
}

timeT
NoteRestInserter::getOffsetWithinRest(NotationStaff *staff,
                                  const NotationElementList::iterator &i,
                                  double &sceneX) // will be snapped
{
    //!!! To make this work correctly in tuplet mode, our divisor would
    // have to be the tupletified duration of the tuplet unit -- we can
    // do that, we just haven't yet
    if (m_widget->isInTupletMode())
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
NoteRestInserter::getOttavaShift(Segment &segment, timeT time)
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
NoteRestInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
                           const Note &note, int pitch, Accidental accidental, int velocity)
{
    NOTATION_DEBUG << "doAddCommand: time " << time << ", endTime " << endTime
                   << ", pitch " << pitch << ",isaRestInserter "
                   << isaRestInserter() << endl;

    Command *activeCommand = 0;  //Used in rest / note mode code
    NoteInsertionCommand *insertionCommand = 0; //Used in rest / note mode code

    // Insert Note (Code taken from old NoteInserter)
    timeT noteEnd = time + note.getDuration();

    // #1046934: make it possible to insert triplet at end of segment!
    if (m_widget->isInTupletMode()) {
        noteEnd = time + (note.getDuration() * m_widget->getTupledCount() / m_widget->getUntupledCount());
    }

    if (time < segment.getStartTime() ||
        endTime > segment.getEndMarkerTime() ||
        noteEnd > segment.getEndMarkerTime()) {
        return 0;
    }

    if (isaRestInserter()) {
        insertionCommand = new RestInsertionCommand(segment, time, endTime, note);
    } else {
        pitch += getOttavaShift(segment, time) * 12;

        float targetSubordering = 0;
        if (m_widget->isInGraceMode()) {
            targetSubordering = m_targetSubordering;
        }

        insertionCommand = new NoteInsertionCommand
            (segment, time, endTime, note, pitch, accidental,
             (m_autoBeam && !m_widget->isInTupletMode() && !m_widget->isInGraceMode()) ?
             NoteInsertionCommand::AutoBeamOn : NoteInsertionCommand::AutoBeamOff,
             m_autoTieBarlines ?
             NoteInsertionCommand::AutoTieBarlinesOn : NoteInsertionCommand::AutoTieBarlinesOff,
             m_matrixInsertType && !m_widget->isInGraceMode() ?
             NoteInsertionCommand::MatrixModeOn : NoteInsertionCommand::MatrixModeOff,
             m_widget->isInGraceMode() ?
             (m_widget->isInTupletMode() ?
              NoteInsertionCommand::GraceAndTripletModesOn :
              NoteInsertionCommand::GraceModeOn)
             : NoteInsertionCommand::GraceModeOff,
             targetSubordering,
             m_defaultStyle,
             velocity);
    }

    activeCommand = insertionCommand;

    if (m_widget->isInTupletMode() && !m_widget->isInGraceMode()) {
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
                                 Note::getNearestNote(note.getDuration() * 2))); // have to find out what these 2 mean
            //## }
            //# These comments should probably be deleted.

            command->addCommand(new TupletCommand
                                (segment, time, note.getDuration(),
                                 m_widget->getUntupledCount(),
                                 m_widget->getTupledCount(), true)); // #1046934: "has timing already"
            command->addCommand(insertionCommand);
            activeCommand = command;
        }
    }

    CommandHistory::getInstance()->addCommand(activeCommand);

    NOTATION_DEBUG << "NoteRestInserter::doAddCommand: accidental is "
                   << accidental << endl;

    return insertionCommand->getLastInsertedEvent();
}

void NoteRestInserter::slotSetNote(Note::Type nt)
{
    m_noteType = nt;
}

 void NoteRestInserter::slotSetDots(unsigned int dots)
{
    if (m_noteDots != dots) {
        m_noteDots = dots;
    }
}

void NoteRestInserter::slotSetAccidental(Accidental accidental,
                                     bool follow)
{
    NOTATION_DEBUG << "NoteRestInserter::setAccidental: accidental is "
                   << accidental << endl;
    m_accidental = accidental;
    m_followAccidental = follow;
}

void NoteRestInserter::slotToggleDot()
{
    QObject *s = sender();
    QString actionName = s->objectName();
    
    // Use fact that switch_dots_on/_off is same name
    // in parent view.  If changes, then a check
    // will need to be made.
    NOTATION_DEBUG << "NoteRestInserter::slotToggleDot: entered. "
        << "Calling action name = " << actionName << endl;

    invokeInParentView(actionName);

}

void NoteRestInserter::slotToggleAutoBeam()
{
    m_autoBeam = !m_autoBeam;
}

void NoteRestInserter::slotEraseSelected()
{
    invokeInParentView("erase");
}

void NoteRestInserter::slotSelectSelected()
{
    invokeInParentView("select");
}

void NoteRestInserter::slotRestsSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note, true));
    actionName.replace(QRegExp("-"), "_");
	
    QAction* action = findAction(actionName);
	
    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
        setToRestInserter(true);
        action->setChecked(true);
        action->trigger();
    }
}

void NoteRestInserter::slotNotesSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");
	
    QAction *action = findActionInParentView(actionName);

    if (!action) {
        std::cerr << "WARNING: No such action as " << actionName << std::endl;
    } else {
        setToRestInserter(false);
        action->setChecked(true);
        action->trigger();
    }
}

// Obsolete ?
//const char* NoteRestInserter::m_actionsAccidental[][4] =
//{
//    { "1slotNoAccidental()",  "no_accidental" },
//    { "1slotFollowAccidental()",  "follow_accidental" },
//    { "1slotSharp()",         "sharp_accidental" },
//    { "1slotFlat()",          "flat_accidental" },
//    { "1slotNatural()",       "natural_accidental" },
//    { "1slotDoubleSharp()",   "double_sharp_accidental" },
//    { "1slotDoubleFlat()",    "double_flat_accidental" }
//};

const QString NoteRestInserter::ToolName     = "noterestinserter";

}

