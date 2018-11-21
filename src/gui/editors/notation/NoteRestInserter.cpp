/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "misc/Debug.h"
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
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/widgets/Panned.h"
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

#include <QApplication>
#include <QAction>
#include <QSettings>
#include <QIcon>
#include <QRegExp>
#include <QString>
#include <QMenu>

#include <QCursor>
#include <QPixmap>
#include <QGraphicsItem>


namespace Rosegarden
{

// Table of durations and associated actions where mouse wheel is cycling
static const struct wheelAction {
            const char *action; int note; unsigned int dots;
} wheelActions[] = {
        { "hemidemisemi",      Note::Hemidemisemiquaver, 0 },
        { "demisemi",          Note::Demisemiquaver,     0 },
        { "dotted_demisemi",   Note::Demisemiquaver,     1 },
        { "semiquaver",        Note::Semiquaver,         0 },
        { "dotted_semiquaver", Note::Semiquaver,         1 },
        { "quaver",            Note::Quaver,             0 },
        { "dotted_quaver",     Note::Quaver,             1 },
        { "crotchet",          Note::Crotchet,           0 },
        { "dotted_crotchet",   Note::Crotchet,           1 },
        { "minim",             Note::Minim,              0 },
        { "dotted_minim",      Note::Minim,              1 },
        { "semibreve",         Note::Semibreve,          0 },
        { "dotted_semibreve",  Note::Semibreve,          1 },
        { "breve",             Note::Breve,              0 },
        { "dotted_breve",      Note::Breve,              1 }
};

static const int wheelActionsSize =
    sizeof(wheelActions) / sizeof(struct wheelAction);

// Resynchronize m_wheelIndex with the current note and dot
void
NoteRestInserter::synchronizeWheel()
{
    // Synchronisation may have a wrong result if a wheel event is
    // currently in process
    if (m_processingWheelTurned) return;

    // If already synchronized, return
    if ((wheelActions[m_wheelIndex].note == m_noteType) &&
            (wheelActions[m_wheelIndex].dots == m_noteDots)) {
        return;
    }

    // Look for a m_wheelIndex value related to {m_noteType, m_noteDots}
    for (int i = 0; i < wheelActionsSize; i++) {
        if ((wheelActions[i].note == m_noteType) &&
                (wheelActions[i].dots == m_noteDots)) {
            m_wheelIndex = i;
            return;   // Found
        }
    }
    // {m_noteType, m_noteDots} not found in wheelActions[]
    // Something goes wrong : force a reset to quaver
    m_wheelIndex = 5;   // Quaver without dot
    m_noteType = wheelActions[m_wheelIndex].note;
    m_noteDots = wheelActions[m_wheelIndex].dots;
}

NoteRestInserter::NoteRestInserter(NotationWidget* widget) :
    NotationTool("noterestinserter.rc", "NoteRestInserter", widget),
    m_noteType(Note::Quaver),
    m_noteDots(0),
    m_autoBeam(true),
    m_leftButtonDown(false),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(false),
    m_wheelIndex(0),
    m_processingWheelTurned(false),
    m_ready(false)
{
    QIcon icon;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_autoBeam = qStrToBool( settings.value("autobeam", "true" ) ) ;
    m_autoTieBarlines = qStrToBool( settings.value("autotieatbarlines","true"));
    m_matrixInsertType = (settings.value("inserttype", 0).toInt()  > 0);
    m_defaultStyle = settings.value("style", NoteStyleFactory::DefaultStyle).toString();
    m_alwaysPreview = qStrToBool(settings.value("alwayspreview", "false"));
    m_quickEdit = qStrToBool(settings.value("quickedit", "false"));

    int accOctaveMode = settings.value("accidentaloctavemode", 1).toInt() ;
    m_octaveType =
        (accOctaveMode == 0 ? AccidentalTable::OctavesIndependent :
         accOctaveMode == 1 ? AccidentalTable::OctavesCautionary :
         AccidentalTable::OctavesEquivalent);

    int accBarMode = settings.value("accidentalbarmode", 0).toInt() ;
    m_barResetType =
        (accBarMode == 0 ? AccidentalTable::BarResetNone :
         accBarMode == 1 ? AccidentalTable::BarResetCautionary :
         AccidentalTable::BarResetExplicit);

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
    createAction("switch_to_notes", SLOT(slotNotesSelected()));
    createAction("switch_to_rests", SLOT(slotRestsSelected()));

    //connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
    //        this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");

    // Setup wheelIndex accordingly to m_noteType and m_noteDots
    synchronizeWheel();
}

NoteRestInserter::NoteRestInserter(QString rcFileName, QString menuName,
                           NotationWidget* widget) :
    NotationTool(rcFileName, menuName, widget),
    m_noteType(Note::Quaver),  //OverRide value with NotationView::initializeNoteRestInserter.
    m_noteDots(0),
    m_autoBeam(false),
    m_leftButtonDown(false),
    m_clickHappened(false),
    m_accidental(Accidentals::NoAccidental),
    m_lastAccidental(Accidentals::NoAccidental),
    m_followAccidental(false),
    m_isaRestInserter(false),
    m_wheelIndex(0),
    m_processingWheelTurned(false),
    m_ready(false)
{
    //connect(m_widget, SIGNAL(changeAccidental(Accidental, bool)),
    //        this, SLOT(slotSetAccidental(Accidental, bool)));

    // Push down the default RadioAction on Accidentals.
    invokeInParentView("no_accidental");

    //!!! grace & triplet mode should be stored by this tool, not by widget!

    //!!! selection should be in scene, not widget!

    // Setup wheelIndex accordingly to m_noteType and m_noteDots
    synchronizeWheel();
}

NoteRestInserter::~NoteRestInserter()
{}

void NoteRestInserter::ready()
{
    m_ready = true;
    m_clickHappened = false;
    m_clickStaff = nullptr;
    
    // The pencil tool uses the wheel for selecting note values.
    // Disable Panned's handling of the wheel.
    m_widget->getView()->setWheelZoomPan(false);

    if (m_alwaysPreview) {
        setCursorShape();
        m_widget->getView()->setMouseTracking(true);
    } else {
        m_widget->setCanvasCursor(Qt::CrossCursor);
    }

//!!!   m_widget->setHeightTracking(true);

}

void NoteRestInserter::stow()
{
    // Fix bug #1528: NotationScene::clearPreviewNote() crash
    // This is a hack to avoid calling stow() twice, which causes this crash
    // when notation scene has been removed before the second call to stow().
    // Of course it would be better to call stow() only once, but this would
    // need some rework of notation editor code I have no time to deal with now.
    if (!m_ready) return;

    if (m_alwaysPreview) {
        clearPreview();
        m_widget->getView()->setMouseTracking(false);
    }
    m_ready = false;
}

void
NoteRestInserter::handleLeftButtonPress(const NotationMouseEvent *e)
{
    m_leftButtonDown = true;
    m_clickHappened = false;   // Force to play the note
    computeLocationAndPreview(e, true);
}

void
NoteRestInserter::handleMidButtonPress(const NotationMouseEvent *)
{
    if (!m_quickEdit) return;
    
    if (isaRestInserter()) {
        slotNotesSelected();
    } else {
        slotRestsSelected();
    }
}

NoteRestInserter::FollowMode
NoteRestInserter::handleMouseMove(const NotationMouseEvent *e)
{
    if (m_alwaysPreview) { 
        computeLocationAndPreview(e, (e->buttons & Qt::LeftButton));
    } else {
        if (m_clickHappened) {
            showPreview(false);
        }
    }
    return NoFollow;
}

Accidental
NoteRestInserter::getAccidentalFromModifierKeys(Qt::KeyboardModifiers modifiers)
{
    Accidental accidental = Accidentals::NoAccidental;

    if (!m_quickEdit) return accidental;
    
//     Qt::KeyboardModifiers modifiers;
//     modifiers = QApplication::queryKeyboardModifiers();
    
    // Use Maj or Ctrl keys to add sharp or flat on the fly
    // Use Maj + Ctrl to add natural  
    if (modifiers == Qt::ShiftModifier) {
        accidental = Accidentals::Sharp;
    } else if (modifiers == Qt::ControlModifier) {
        accidental = Accidentals::Flat;
    } else if (modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
        accidental = Accidentals::Natural;
    }
    return accidental;
}

void
NoteRestInserter::handleMouseRelease(const NotationMouseEvent *e)
{
    m_leftButtonDown = false;

    NOTATION_DEBUG << "NoteRestInserter::handleMouseRelease: staff = " <<
        m_clickStaff << ", clicked = " << m_clickHappened;

        NotationStaff *staff = m_clickStaff;
    if (!m_clickHappened || !staff) return;

    bool okay = computeLocationAndPreview(e, true);
    clearPreview();
    m_clickHappened = false;
    m_clickStaff = nullptr;
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

    Accidental accidental = getAccidentalFromModifierKeys(e->modifiers);
    if (accidental == Accidentals::NoAccidental) {
        accidental =
            (m_accidental == Accidentals::NoAccidental && m_followAccidental)
                ? m_lastAccidental : m_accidental;
    }

    Event *lastInsertedEvent =
        doAddCommand(segment, m_clickTime, endTime, note, m_clickPitch,
                     accidental, 100);  // Velocity hard coded for now,

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

void
NoteRestInserter::setCursorShape()
{
    if (!m_scene) {
        if (m_widget) m_widget->setCanvasCursor(Qt::CrossCursor);
        return;
    }

    NotePixmapParameters params(Note::QuarterNote,
                         0,  // dots
                         Accidentals::NoAccidental);
    
    NotePixmapFactory * pixmapFactory = m_scene->getNotePixmapFactory();
    NOTATION_DEBUG << "pixmap factory =" << pixmapFactory;

    if (isaRestInserter()) {
        // In rest inserter mode, the preview doesn't work (currently) and
        // the shape of the cursor is the rest.
        params.setNoteType(getCurrentNote().getNoteType());
        params.setDots(getCurrentNote().getDots());
        params.setForcedColor(GUIPalette::PreviewColor);

        QGraphicsPixmapItem * gitem =
            dynamic_cast<QGraphicsPixmapItem *>(pixmapFactory->makeRest(params));
        NOTATION_DEBUG << "rest: gitem =" << gitem;

        QPixmap pixmap = gitem->pixmap();
        QCursor cursor(pixmap);
        m_widget->setCanvasCursor(cursor);
    } else {
        // In note inserter mode, the preview is working and the cursor
        // keeps a cross shape.
        m_widget->setCanvasCursor(Qt::CrossCursor);
    }

    // Resynchronize the wheel (needed if duration was not selected with it)
    synchronizeWheel();
}

void
NoteRestInserter::handleWheelTurned(int delta, const NotationMouseEvent *e)
{
    if (!m_scene) return; 

    if (m_quickEdit) {
        // Set tool according to the new wheel position

        // Prevent synchronizeWheel() from modifying m_wheelIndex
        m_processingWheelTurned = true;
    
        // Uncheck current tool
        QAction * action =
            findActionInParentView(wheelActions[m_wheelIndex].action);
        action->setChecked(false);
        
        // Select new tool
        if (delta > 0) {
            m_wheelIndex++;
            if (m_wheelIndex >= wheelActionsSize) m_wheelIndex = 0;
        } else {
            m_wheelIndex--;
            if (m_wheelIndex < 0) m_wheelIndex = wheelActionsSize - 1;
        }

        // Check dot tool related to the new tool
        // Actions switch_dots_on et switch_dots_off trigs a toggle and
        // have the same effect...
        if (QString(wheelActions[m_wheelIndex].action).startsWith("dotted_")) {
            if (m_noteDots == 0) {
                invokeInParentView("switch_dots_on");
            }
        } else {
            if (m_noteDots == 1) {
                invokeInParentView("switch_dots_off");
            }
        }

        // Check the new tool
        action = findActionInParentView(wheelActions[m_wheelIndex].action);
        action->setChecked(true);

        // Prepare to use tool
        invokeInParentView(wheelActions[m_wheelIndex].action);

        if (m_alwaysPreview) {
            // Set the new cursor shape
            setCursorShape();

            // Update preview
            clearPreview();
            computeLocationAndPreview(e, false);
        }

        // Allow synchronizeWheel() to modify m_wheelIndex if needed
        m_processingWheelTurned = false;
    }
}

void NoteRestInserter::showMenu()
{
    NOTATION_DEBUG << "NoteRestInserter::showMenu() : enter.";
    if (!hasMenu())
        return ;

    if (!m_menu)
        createMenu();

    if (m_menu) {
        NOTATION_DEBUG << "NoteRestInserter::showMenu() : morphing menu.";
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
        NOTATION_DEBUG << "NoteRestInserter::showMenu() : no menu to show.";
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
                               (segment, insertionTime, endTime,
                                note, pitch, accidental, velocity);

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
NoteRestInserter::computeLocationAndPreview(const NotationMouseEvent *e,
                                            bool play)
{
    if (!e->staff || !e->element) {
        NOTATION_DEBUG << "computeLocationAndPreview: staff and/or element not supplied";
        clearPreview();
        return false;
    }

    if (m_clickHappened && (e->staff != m_clickStaff)) {
        NOTATION_DEBUG << "computeLocationAndPreview: staff changed from originally clicked one (" << e->staff << " vs " << m_clickStaff << ")";
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
        NOTATION_DEBUG << "computeLocationAndPreview: element provided is not found in staff";
        return false;
    }

    timeT time = el->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    m_clickInsertX = el->getLayoutX();

    int subordering = el->event()->getSubOrdering();
    float targetSubordering = subordering;

    if (grace && el->getItem()) {

        NOTATION_DEBUG << "x=" << x << ", el->getSceneX()=" << el->getSceneX();

        if (el->isRest()) NOTATION_DEBUG << "elt is a rest";
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

    int pitch;
    Accidental quickAccidental = getAccidentalFromModifierKeys(e->modifiers);
    if (quickAccidental != Accidentals::NoAccidental) {
        Pitch p(e->height, e->clef, e->key, quickAccidental);
        pitch = p.getPerformancePitch();
    } else {
        Pitch p(e->height, e->clef, e->key, m_accidental);
        pitch = p.getPerformancePitch();

        // [RFE 987960] When inserting via mouse, if no accidental is
        // selected, we use the same accidental (and thus the same pitch)
        // as of the previous note found at this height -- if such a note
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
    }

    bool changed = false;

    if (m_clickHappened) {
        if (time != m_clickTime ||
            subordering != m_clickSubordering ||
            pitch != m_clickPitch ||
            e->height != m_clickHeight ||
            e->staff != m_clickStaff ||
            quickAccidental != m_clickQuickAccidental) {
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
        m_clickQuickAccidental = quickAccidental;
        m_targetSubordering = targetSubordering;

        showPreview(play);
    }

    return true;
}

void NoteRestInserter::showPreview(bool play)
{
    if (isaRestInserter()) {
        // Sorry, no preview available for rests.
        return;
    }
    if (!m_clickStaff) return;
    Segment &segment = m_clickStaff->getSegment();

    int pitch = m_clickPitch;
    pitch += getOttavaShift(segment, m_clickTime) * 12;

    bool inRange = true;
    if (pitch > segment.getHighestPlayable() ||
            pitch < segment.getLowestPlayable()) {
        inRange = false;
    }


    // Look for an accidental to preview

    // Default is forced accidental
    Accidental cursorAccidental = m_clickQuickAccidental;

    // Maybe follow accidental if default is no accidental
    if (cursorAccidental == Accidentals::NoAccidental) {
        cursorAccidental =
            (m_accidental == Accidentals::NoAccidental && m_followAccidental)
                ? m_lastAccidental : m_accidental;
    }
    

    // Get the start time of the bar where the insertion time is
    timeT startOfBar = segment.getBarStartForTime(m_clickTime);

    // Get the clef and key signature in effect and the times of change
    timeT currentKeyTime;
    Key currentKey = segment.getKeyAtTime(m_clickTime, currentKeyTime);
    timeT currentClefTime;
    Clef currentClef = segment.getClefAtTime(m_clickTime, currentClefTime);
    
    // Pitch related to the cursor position
    Pitch cursorPitch(pitch, cursorAccidental);
    int cursorHeight = cursorPitch.getHeightOnStaff(currentClef, currentKey);

    // Select a default accidental related to the current key signature
    cursorAccidental = cursorPitch.getDisplayAccidental(currentKey);
    if (cursorAccidental == Accidentals::Natural) {
        // Don't use natural if not needed
        if (currentKey.getAccidentalAtHeight(cursorHeight, currentClef)
                                            == Accidentals::NoAccidental) {
            cursorAccidental = Accidentals::NoAccidental;
        }
    }
    
    
    // Get an iterator of the current event at m_clickTime
    Segment::iterator it = segment.findNearestTime(m_clickTime);

    bool cursorCautious = false;
    bool lookForNoteOnSameOctave = false;

    // If the insertion point is immediately following a key change
    // there is no need to look at accidentals of previous notes
    if ((*it)->isa(Key::EventType)) {
        // Nothing to do
        // done = true;
    } else {
        // Else we have to look at the events inside the current bar

        // Get an iterator of the first event of the bar
        Segment::iterator itFirst = segment.findTime(startOfBar);
        
        // Prepare to get the elements of the bar in reverse order
        typedef std::reverse_iterator<Segment::iterator> RevIt;
        RevIt rit(it);
        RevIt last(itFirst);

        // While done is false whe have to look back to some previous event
        // which may modify the accidental preview
        bool done = false;

        // Walk through the bar
        for ( ; rit != last; ++rit) {
            Event *ev = *rit;

            if (ev->isa(Key::EventType)) {
                if (lookForNoteOnSameOctave) {

                    // If cautionary and accidental is not one of the key
                    // then remove cautionary
                    if (cursorCautious) {

                        // Select a default accidental related to the current
                        // key signature
                        Accidental keyAccidental = 
                            cursorPitch.getDisplayAccidental(currentKey);
                        if (keyAccidental == Accidentals::Natural) {
                            // Don't use natural if not needed
                            if (currentKey.getAccidentalAtHeight(
                                    cursorHeight, currentClef) ==
                                            Accidentals::NoAccidental) {
                                keyAccidental = Accidentals::NoAccidental;
                            }
                        }

                        if (cursorAccidental == keyAccidental) {
                            cursorCautious = false;
                        } 
                    }
                }
                
                // Stop looking for notes as soon as a key signature is found
                done = true;
                break;
            }

            // We are only interested by notes (ie events with pitch)
            if (!ev->has(BaseProperties::PITCH)) {
                // If the event is not a note, ignore it and continue
                continue;
            } else {
                // Event is a note, get its performance pitch
                int p = ev->get<Int>(BaseProperties::PITCH);

                // get its accidental
                const NotationProperties &prop(m_scene->getProperties());
                Accidental accidental = Accidentals::NoAccidental;
                (void)ev->get<String>(prop.DISPLAY_ACCIDENTAL, accidental);

                Pitch notePitch(p, accidental);

                // If a note is found at the same height than the cursor
                if (notePitch.getHeightOnStaff(currentClef, currentKey)
                                                        == cursorHeight) {
                    if (!lookForNoteOnSameOctave) {
                        if (p == pitch) {
                            // If they have the same pitch, the accidental is
                            // already drawn in the bar: no need to duplicate it
                            cursorAccidental = Accidentals::NoAccidental;
                        } else {
                            // Else use the accidental the key signature requires
                            cursorAccidental = cursorPitch.getAccidental(currentKey);
                            // And if the key requires no accidental, use natural
                            if (cursorAccidental == Accidentals::NoAccidental) {
                                cursorAccidental = Accidentals::Natural;
                            }
                        }
                    // Same height on staff but maybe cautionary accidental
                    // is not needed
                    } else {
                        if (cursorCautious) {
                            if (p != pitch) {
                                cursorCautious = false;
                            } else {
                                if (cursorAccidental == Accidentals::NoAccidental) {
                                    cursorAccidental = Accidentals::Natural;
                                }
                            }
                        }
                            
                        if (!cursorCautious) {
                            if (p == pitch) {
                                // If they have the same pitch, the accidental is
                                // already drawn in the bar: no need to duplicate it
                                cursorAccidental = Accidentals::NoAccidental;
                            } else {
                                // Else use the accidental the key signature requires
                                cursorAccidental = cursorPitch.getAccidental(currentKey);
                                // And if the key requires no accidental, use natural
                                if (cursorAccidental == Accidentals::NoAccidental) {
                                    cursorAccidental = Accidentals::Natural;
                                }
                            }
                        }
                    }
                    // Then stop walking through the bar
                    done = true;
                    break;
                }
                
                // If a note is found with the same name in another octave (the
                // octave is different otherwise we should have break in the
                // preceding if)
                if (cursorPitch.getNoteName(currentKey) ==
                            notePitch.getNoteName(currentKey)) {
                    
                    if (lookForNoteOnSameOctave) continue;

                    if (m_octaveType == AccidentalTable::OctavesCautionary) {
                        
                        if (cursorPitch.getPitchInOctave() ==
                                notePitch.getPitchInOctave()) {
                            // If they have the same pitch in octave, the
                            // accidental is needed
                            cursorAccidental =
                                notePitch.getDisplayAccidental(currentKey);
                            cursorCautious = false;
                            // If Natural, force use of NoAccidental
                            if (cursorAccidental == Accidentals::Natural) {
                                cursorAccidental = Accidentals::NoAccidental;
                            }
                        } else {
                            // Else use the accidental the key signature
                            // requires
                            cursorAccidental = 
                                cursorPitch.getAccidental(currentKey);

                            // Display it as cautionary if any accidental
                            // of the key signature is matched
                            Accidental accidentalForKey =
                                currentKey.getAccidentalForStep(
                                    cursorPitch.getNoteInScale(currentKey));
                            // Always use Natural rather than NoAccidental
                            // while comparing
                            if (accidentalForKey == Accidentals::NoAccidental) {
                                accidentalForKey = Accidentals::Natural;
                            }
                            Accidental cursorTestAccidental = cursorAccidental;
                            if (cursorAccidental == Accidentals::NoAccidental) {
                                cursorTestAccidental = Accidentals::Natural;
                            }
                            if (cursorTestAccidental != accidentalForKey) {
                                cursorCautious = true;
                                // If cautionary, force use of natural
                                cursorAccidental = cursorTestAccidental;
                            }

                cursorCautious = true; //!!! preceding code is useless
                        }

                        // We don't break here because we have to look for
                        // another note with the same height in the same octave
                        lookForNoteOnSameOctave = true;
                        
                        // Nevertheless a preview related to the current bar
                        // is already found : no need to look at the previous
                        // bar in the next step.
                        done = true;

                    } else if (m_octaveType == AccidentalTable::OctavesEquivalent) {

                        if (cursorPitch.getPitchInOctave() ==
                                notePitch.getPitchInOctave()) {
                            // If they have the same pitch in octave, the
                            // accidental is already drawn in the bar: no need
                            // to duplicate it
                            cursorAccidental = Accidentals::NoAccidental;
                        } else {
                            // Else use the accidental the key signature
                            // requires
                            cursorAccidental = cursorPitch.getAccidental(currentKey);
                            // And if the key requires no accidental, use
                            // natural
                            if (cursorAccidental == Accidentals::NoAccidental) {
                                cursorAccidental = Accidentals::Natural;
                            }
                        }
                        done = true;
                        break;

                    } else {
                        // m_octaveType == AccidentalTable::OctavesIndependent
                        // Do nothing
                    }
                }
            }
        }

        // Should we look at the previous bar ?
        if (   // Yes if no preview accidental already found
               // and if look at previous bar asked in preferences
            !done && (m_barResetType != AccidentalTable::BarResetNone)
               // and if no accidental explicitely specified
                  && (cursorAccidental == Accidentals::NoAccidental)
               // and if the previous bar exists
                  && (segment.getStartTime() < startOfBar)) {

            // Get the start time of the previous bar
            int barNum = segment.getComposition()->getBarNumber(startOfBar);
            timeT startOfPreviousBar = segment.getComposition()
                                                ->getBarStart(barNum - 1);


            // Get an iterator on the first event after the end of the previous
            // bar (ie the first event of the current bar)
            Segment::iterator itLast = itFirst;

            // Get an iterator of the first event of the previous bar
            itFirst = segment.findTime(startOfPreviousBar);

            // Prepare to get the elements of the previous bar in reverse order
            RevIt rit(itLast);
            RevIt last(itFirst);

            // Walk through the bar
            for ( ; rit != last; ++rit) {
                Event *ev = *rit;

                if (ev->isa(Key::EventType)) {
                    // Stop looking for notes as soon as a key signature is found
                    done = true;
                    break;
                }

                // We are only interested by notes (ie events with pitch)
                if (!ev->has(BaseProperties::PITCH)) {
                    // If the event is not a note, ignore it and continue
                    continue;
                } else {
                    // Event is a note, get its performance pitch
                    int p = ev->get<Int>(BaseProperties::PITCH);

                    // get its accidental
                    const NotationProperties &prop(m_scene->getProperties());
                    Accidental accidental = Accidentals::NoAccidental;
                    (void)ev->get<String>(prop.DISPLAY_ACCIDENTAL, accidental);

                    Pitch notePitch(p, accidental);

                    // If a note is found at the same height than the cursor
                    if (notePitch.getHeightOnStaff(currentClef, currentKey)
                                                            == cursorHeight) {

                        if (p == pitch) {
                            // If they have the same pitch, the accidental is
                            // already drawn in the bar: no need to duplicate it
                            cursorAccidental = Accidentals::NoAccidental;
                        } else {
                            // Else use the accidental the key signature requires
                            cursorAccidental = cursorPitch.getAccidental(currentKey);
                            // And if the key requires no accidental, use natural
                            if (cursorAccidental == Accidentals::NoAccidental) {
                                cursorAccidental = Accidentals::Natural;
                            }

                            // Cautionary wanted ?
                            // If we are in this place, done should be false and
                            // cursorCautious keeps its inital value (false).
                            if (m_barResetType == AccidentalTable::BarResetCautionary) {
                                cursorCautious = true;
                            }
                        }

                        // Then stop walking through the bar
                        break;
                    }
                }
            }
        }
    }



    // Select the color of the preview
    QColor color;
    if (!inRange) {
        color = GUIPalette::OutRangeColor;   // Red when out of range
    } else if (m_leftButtonDown) {
        color = GUIPalette::SelectionColor;  // Blue when button pressed
    } else {
        color = GUIPalette::PreviewColor;    // Dark green otherwise
    }

    if (m_scene) {
        m_scene->showPreviewNote(m_clickStaff, m_clickInsertX,
                                 pitch, m_clickHeight,
                                 Note(m_noteType, m_noteDots),
                                 m_widget->isInGraceMode(),
                                 cursorAccidental, cursorCautious,
                                 color, -1, play);
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
                   << isaRestInserter();

    Command *activeCommand = nullptr;  //Used in rest / note mode code
    NoteInsertionCommand *insertionCommand = nullptr; //Used in rest / note mode code

    // Insert Note (Code taken from old NoteInserter)
    timeT noteEnd = time + note.getDuration();

    // #1046934: make it possible to insert triplet at end of segment!
    if (m_widget->isInTupletMode()) {
        noteEnd = time + (note.getDuration() * m_widget->getTupledCount() / m_widget->getUntupledCount());
    }

    if (time < segment.getStartTime() ||
        endTime > segment.getEndMarkerTime() ||
        noteEnd > segment.getEndMarkerTime()) {
        return nullptr;
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
                   << accidental;

    return insertionCommand->getLastInsertedEvent();
}

void NoteRestInserter::slotSetNote(Note::Type nt)
{
    m_noteType = nt;
}

void NoteRestInserter::slotSetDots(unsigned int dots)
{
    m_noteDots = dots;
}

void NoteRestInserter::slotSetAccidental(Accidental accidental,
                                     bool follow)
{
    NOTATION_DEBUG << "NoteRestInserter::setAccidental: accidental is "
                   << accidental;
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
        << "Calling action name = " << actionName;

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

    QAction* action = findActionInParentView(actionName);

    if (!action) {
        RG_WARNING << "WARNING: No such action as " << actionName;
    } else {
        setToRestInserter(true);
        action->setChecked(true);
        action->trigger();
        invokeInParentView("switch_to_rests");
    }
}

void NoteRestInserter::slotNotesSelected()
{
    Note note(m_noteType, m_noteDots);
    QString actionName(NotationStrings::getReferenceName(note));
    actionName.replace(QRegExp("-"), "_");

    QAction *action = findActionInParentView(actionName);

    if (!action) {
        RG_WARNING << "WARNING: No such action as " << actionName;
    } else {
        setToRestInserter(false);
        action->setChecked(true);
        action->trigger();
        invokeInParentView("switch_to_notes");
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

QString NoteRestInserter::ToolName() { return "noterestinserter"; }

}

