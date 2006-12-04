/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include <kapplication.h>

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
#include "NotePixmapFactory.h"
#include "NoteStyleFactory.h"
#include <kaction.h>
#include <kcommand.h>
#include <kconfig.h>
#include <qiconset.h>
#include <qregexp.h>
#include <qstring.h>


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
    QIconSet icon;

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);
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

    connect(m_parentView, SIGNAL(changeAccidental(Accidental, bool)),
            this, SLOT(slotSetAccidental(Accidental, bool)));
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

    timeT time = (*itr)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()
    m_clickInsertX = (*itr)->getLayoutX();
    if (clefEvt)
        clef = Clef(*clefEvt);
    if (keyEvt)
        key = Rosegarden::Key(*keyEvt);

    NotationElement* el = static_cast<NotationElement*>(*itr);
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

        if (!segment.isBeforeEndMarker(i))
            break;

        if ((*i)->isa(Note::EventType) &&
                (*i)->has(NotationProperties::OTTAVA_SHIFT)) {
            ottavaShift = (*i)->get
                          <Int>(NotationProperties::OTTAVA_SHIFT);
            break;
        }

        if ((*i)->isa(Indication::EventType)) {
            try {
                Indication ind(**i);
                if (ind.isOttavaType()) {
                    ottavaShift = ind.getOttavaShift();
                    break;
                }
            } catch (...) { }
        }

        if (i == segment.begin())
            break;
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
            !(*i)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {

            KMacroCommand *command = new KMacroCommand(insertionCommand->name());

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
    m_noteDots = dots;

    KToggleAction *dotsAction = dynamic_cast<KToggleAction *>
                                (actionCollection()->action("toggle_dot"));
    if (dotsAction)
        dotsAction->setChecked(dots > 0);
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
