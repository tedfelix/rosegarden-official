/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[NotationStaff]"
#define RG_NO_DEBUG_PRINT 1

#include "NotationStaff.h"

#include "base/Composition.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/NotationQuantizer.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/ViewElement.h"
#include "base/figuration/GeneratedRegion.h"
#include "base/figuration/SegmentID.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/editors/notation/NotationChord.h"
#include "gui/editors/notation/NotationElement.h"
#include "gui/editors/notation/NotationHLayout.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NoteFontFactory.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/editors/notation/NotePixmapParameters.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "gui/editors/notation/StaffLayout.h"
#include "gui/general/PixmapFunctions.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPainter>
#include <QPoint>
#include <QRect>

#include <iostream>


namespace Rosegarden
{

NotationStaff::NotationStaff(NotationScene *scene, Segment *segment,
                             SnapGrid *snapGrid, int id,
                             NotePixmapFactory *normalFactory,
                             NotePixmapFactory *smallFactory) :
    ViewSegment(*segment),
    StaffLayout(scene, this, snapGrid, id,
                normalFactory->getSize(),
                normalFactory->getStaffLineThickness(),
                LinearMode, 0, 0,  // pageMode, pageWidth and pageHeight set later
                0 // row spacing
        ),
    m_notePixmapFactory(normalFactory),
    m_graceNotePixmapFactory(smallFactory),
    m_previewItem(nullptr),
    m_staffName(nullptr),
    m_notationScene(scene),
    m_legerLineCount(8),
    m_barNumbersEvery(0),
    m_colourQuantize(true),
    m_showUnknowns(true),
    m_showRanges(true),
    m_showCollisions(true),
    m_hideRedundance(true),
    m_printPainter(nullptr),
    m_refreshStatusId(segment->getNewRefreshStatusId())
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_colourQuantize = qStrToBool( settings.value("colourquantize", "false" ) ) ;

    // Shouldn't change these  during the lifetime of the staff, really:
    m_showUnknowns = qStrToBool( settings.value("showunknowns", "false" ) ) ;
    m_showRanges = qStrToBool( settings.value("showranges", "true" ) ) ;
    m_showCollisions = qStrToBool( settings.value("showcollisions", "true" ) ) ;

    m_keySigCancelMode = settings.value("keysigcancelmode", 1).toInt() ;

    m_hideRedundance = settings.value("hideredundantclefkey", "true").toBool();

    m_distributeVerses =  settings.value("distributeverses", true).toBool();

    settings.endGroup();

    setLineThickness(m_notePixmapFactory->getStaffLineThickness());
}

NotationStaff::~NotationStaff()
{
    RG_DEBUG << "~NotationStaff()";
    deleteTimeSignatures();
}

SegmentRefreshStatus &
NotationStaff::getRefreshStatus() const
{
    return m_segment.getRefreshStatus(m_refreshStatusId);
}

void
NotationStaff::resetRefreshStatus()
{
    m_segment.getRefreshStatus(m_refreshStatusId).setNeedsRefresh(false);
}

void
NotationStaff::setNotePixmapFactories(NotePixmapFactory *normal,
                                      NotePixmapFactory *small)
{
    m_notePixmapFactory = normal;
    m_graceNotePixmapFactory = small;

    setResolution(m_notePixmapFactory->getSize());
    setLineThickness(m_notePixmapFactory->getStaffLineThickness());
}

void
NotationStaff::insertTimeSignature(double layoutX,
                                   const TimeSignature &timeSig, bool grayed)
{
    if (timeSig.isHidden())
        return ;

    m_notePixmapFactory->setSelected(false);
    m_notePixmapFactory->setShaded(grayed);
    QGraphicsItem *item = m_notePixmapFactory->makeTimeSig(timeSig);
//    QCanvasTimeSigSprite *sprite =
//        new QCanvasTimeSigSprite(layoutX, pixmap, m_canvas);

    StaffLayoutCoords sigCoords =
        getSceneCoordsForLayoutCoords(layoutX, getLayoutYForHeight(4));

    getScene()->addItem(item);
    item->setPos(sigCoords.first, (double)sigCoords.second);
    item->show();
    m_timeSigs.insert(item);
}

void
NotationStaff::deleteTimeSignatures()
{
    //    RG_DEBUG << "deleteTimeSignatures()";

    for (ItemSet::iterator i = m_timeSigs.begin(); i != m_timeSigs.end(); ++i) {
        delete *i;
    }

    m_timeSigs.clear();
}

void
NotationStaff::insertRepeatedClefAndKey(double layoutX, int barNo)
{
    bool needClef = false, needKey = false;
    timeT t;

    timeT barStart = getSegment().getComposition()->getBarStart(barNo);

    Clef clef = getSegment().getClefAtTime(barStart, t);
    if (t < barStart) {
        needClef = true;
    } else {
        if (m_hideRedundance &&
            m_notationScene->isEventRedundant(clef, barStart, getSegment())) {
            needClef = true;
        }
    }

    ::Rosegarden::Key key = getSegment().getKeyAtTime(barStart, t);
    if (t < barStart) {
        needKey = true;
    } else {
        if (m_hideRedundance &&
            m_notationScene->isEventRedundant(key, barStart, getSegment())) {
            needKey = true;
        }
    }

    double dx = m_notePixmapFactory->getBarMargin() / 2;

    if (!m_notationScene->isInPrintMode())
        m_notePixmapFactory->setShaded(true);

    if (needClef) {

        int layoutY = getLayoutYForHeight(clef.getAxisHeight());

        StaffLayoutCoords coords =
            getSceneCoordsForLayoutCoords(layoutX + dx, layoutY);

        QGraphicsItem *item = m_notePixmapFactory->makeClef(clef);
        getScene()->addItem(item);
        item->setPos(coords.first, coords.second);
        item->show();
        m_repeatedClefsAndKeys.insert(item);

        dx += item->boundingRect().width() +
            m_notePixmapFactory->getNoteBodyWidth() * 2 / 3;
    }

    if (needKey) {

        int layoutY = getLayoutYForHeight(12);

        StaffLayoutCoords coords =
            getSceneCoordsForLayoutCoords(layoutX + dx, layoutY);

        QGraphicsItem *item = m_notePixmapFactory->makeKey(key, clef);
        getScene()->addItem(item);
        item->setPos(coords.first, coords.second);
        item->show();
        m_repeatedClefsAndKeys.insert(item);

        dx += item->boundingRect().width();
    }

    m_notePixmapFactory->setShaded(false);
}

void
NotationStaff::deleteRepeatedClefsAndKeys()
{
    for (ItemSet::iterator i = m_repeatedClefsAndKeys.begin();
            i != m_repeatedClefsAndKeys.end(); ++i) {
        delete *i;
    }

    m_repeatedClefsAndKeys.clear();
}

void
NotationStaff::drawStaffName()
{
    delete m_staffName;

    m_staffNameText =
        getSegment().getComposition()->
        getTrackById(getSegment().getTrack())->getLabel();

    m_staffName =
        m_notePixmapFactory->makeText(Text(m_staffNameText, Text::StaffName));

    getScene()->addItem(m_staffName);

    int layoutY = getLayoutYForHeight(3);
    StaffLayoutCoords coords = getSceneCoordsForLayoutCoords(0, layoutY);
    m_staffName->setPos(getX() + getMargin() + m_notePixmapFactory->getNoteBodyWidth(),
                        coords.second - m_staffName->boundingRect().height() / 2);
    m_staffName->show();
}

timeT
NotationStaff::getTimeAtSceneCoords(double cx, int cy) const
{
    StaffLayoutCoords layoutCoords = getLayoutCoordsForSceneCoords(cx, cy);
    RulerScale * rs = m_notationScene->getHLayout();
    return rs->getTimeForX(layoutCoords.first);
}

void
NotationStaff::getClefAndKeyAtSceneCoords(double cx, int cy,
                                           Clef &clef,
                                           ::Rosegarden::Key &key) const
{
    StaffLayoutCoords layoutCoords = getLayoutCoordsForSceneCoords(cx, cy);
    size_t i;

    for (i = 0; i < m_clefChanges.size(); ++i) {
        if (m_clefChanges[i].first > layoutCoords.first)
            break;
        clef = m_clefChanges[i].second;
    }

    for (i = 0; i < m_keyChanges.size(); ++i) {
        if (m_keyChanges[i].first > layoutCoords.first)
            break;
        key = m_keyChanges[i].second;
    }
}

/*!!!

ViewElementList::iterator
NotationStaff::getClosestElementToLayoutX(double x,
        Event *&clef,
        Event *&key,
        bool notesAndRestsOnly,
        int proximityThreshold)
{
    START_TIMING;

    double minDist = 10e9, prevDist = 10e9;

    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it, result;

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {
        NotationElement *el = static_cast<NotationElement*>(*it);

        bool before = ((*it)->getLayoutX() < x);

        if (!el->isNote() && !el->isRest()) {
            if (before) {
                if ((*it)->event()->isa(Clef::EventType)) {
                    clef = (*it)->event();
                } else if ((*it)->event()->isa(::Rosegarden::Key::EventType)) {
                    key = (*it)->event();
                }
            }
            if (notesAndRestsOnly)
                continue;
        }

        double dx = x - (*it)->getLayoutX();
        if (dx < 0)
            dx = -dx;

        if (dx < minDist) {
            minDist = dx;
            result = it;
        } else if (!before) {
            break;
        }

        prevDist = dx;
    }

    if (proximityThreshold > 0 && minDist > proximityThreshold) {
        RG_DEBUG << "getClosestElementToLayoutX() : element is too far away : "
        << minDist;
        return notes->end();
    }

    RG_DEBUG << "getClosestElementToLayoutX: found element at layout " << (*result)->getLayoutX() << " - we're at layout " << x;

    PRINT_ELAPSED("NotationStaff::getClosestElementToLayoutX");

    return result;
}
*/

ViewElementList::iterator
NotationStaff::getElementUnderLayoutX(double x,
                                      Event *&clef,
                                      Event *&key)
{
    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it;

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {
        NotationElement* el = static_cast<NotationElement*>(*it);

        bool before = ((*it)->getLayoutX() <= x);

        if (!el->isNote() && !el->isRest()) {
            if (before) {
                if ((*it)->event()->isa(Clef::EventType)) {
                    clef = (*it)->event();
                } else if ((*it)->event()->isa(::Rosegarden::Key::EventType)) {
                    key = (*it)->event();
                }
            }
        }

        double airX, airWidth;
        el->getLayoutAirspace(airX, airWidth);
        if (x >= airX && x < airX + airWidth) {
            return it;
        } else if (!before) {
            if (it != notes->begin())
                --it;
            return it;
        }
    }

    return notes->end();
}

QString
NotationStaff::getNoteNameAtSceneCoords(double x, int y,
        Accidental) const
{
    //!!! We have one version of this translate note name stuff in
    // MidiPitchLabel, another one in TrackParameterBox, and now this one.
    //
    // This needs to be refactored one day to avoid all the frigged up hackery,
    // and just have one unified way of making these strings, used everywhere.
    // I think putting the tr() nonsense in base/ probably makes sense for
    // this, and just have a Pitch method that returns a pre-translated string.
    //
    // But not just now.  I'll slap a couple of pieces of duct tape on it for
    // now, and we'll worry about bigger issues.

    Clef clef;
    ::Rosegarden::Key key;
    getClefAndKeyAtSceneCoords(x, y, clef, key);

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    int baseOctave = settings.value("midipitchoctave", -2).toInt() ;
    settings.endGroup();

    Pitch p(getHeightAtSceneCoords(x, y), clef, key);

    // This duplicates a lot of code in Pitch::getAsString and elsewhere, but
    // I'm not taking time out to gather all of this up and merge it together
    // into something nice and clean we could just call here.

    // get the note letter name in the key (eg. A)
    std::string s;
    s = p.getNoteName(key);

    // get the accidental, and append it immediately after the letter name, to
    // create an English style string (eg. Ab)
    Accidental acc = p.getAccidental(key);
    if (acc == Accidentals::Sharp) s += "#";
    else if (acc == Accidentals::Flat) s += "b";

    // run the English string through tr() to get it translated by way of the
    // QObject context (all variants are in the extracted QMenuStrings.cpp
    // file, and available for translation, so this *should* get us the best
    // spelling for a given key, since we're using the actual key, and not a
    // guess
    QString tmp = QObject::tr(s.c_str(), "note name");

    // now tack on the octave, so translators don't have to deal with it
    tmp += tr(" %1").arg(p.getOctave(baseOctave));

    return tmp;
}

void
NotationStaff::renderElements(timeT from, timeT to)
{
    NotationElementList::iterator i = getViewElementList()->findTime(from);
    NotationElementList::iterator j = getViewElementList()->findTime(to);
    renderElements(i, j);
}

void
NotationStaff::renderElements(NotationElementList::iterator from,
                              NotationElementList::iterator to)
{
    //    RG_DEBUG << "NotationStaff " << this << "::renderElements()";
    Profiler profiler("NotationStaff::renderElements");

    timeT endTime =
        (to != getViewElementList()->end() ? (*to)->getViewAbsoluteTime() :
         getSegment().getEndMarkerTime());
    timeT startTime = (from != to ? (*from)->getViewAbsoluteTime() : endTime);

    Clef currentClef = getSegment().getClefAtTime(startTime);
    // Since the redundant clefs and keys may be hide, the current key may
    // be defined on some other segment and whe have to find it in the whole
    // notation scene clef/key context.
//    ::Rosegarden::Key currentKey = getSegment().getKeyAtTime(startTime);
    ::Rosegarden::Key currentKey = m_notationScene->getClefKeyContext()->
                        getKeyFromContext(getSegment().getTrack(), startTime);

// m_notationScene->getClefKeyContext()->dumpKeyContext();

    for (NotationElementList::iterator it = from, nextIt = from;
         it != to; it = nextIt) {

        ++nextIt;

        bool selected = isSelected(it);
        //      RG_DEBUG << "Rendering at " << (*it)->getAbsoluteTime()
        //                           << " (selected = " << selected << ")";

        renderSingleElement(it, currentClef, currentKey, selected);
    }

    //    RG_DEBUG << "NotationStaff " << this << "::renderElements: "
    //                   << elementCount << " elements rendered";
}

const NotationProperties &
NotationStaff::getProperties() const
{
    return m_notationScene->getProperties();
}

bool
NotationStaff::elementNeedsRegenerating(NotationElementList::iterator i)
{
    NotationElement *elt = static_cast<NotationElement *>(*i);

    RG_DEBUG << "elementNeedsRegenerating: ";

    if (!elt->getItem()) {
        RG_DEBUG << "yes (no item)";
        return true;
    }

    if (isSelected(i) != elt->isSelected()) {
        RG_DEBUG << "yes (selection status changed)";
        return true;
    }

    if (elt->event()->isa(Indication::EventType)) {
        // determining whether to redraw an indication is complicated
        RG_DEBUG << "probably (is indication)";
        return !elt->isRecentlyRegenerated();
    }

    if (!elt->isNote()) {
        RG_DEBUG << "no (is not note)";
        return false;
    }

    // If the note's y-coordinate has changed, we should redraw it --
    // its stem direction may have changed, or it may need leger
    // lines.  This will happen e.g. if the user inserts a new clef;
    // unfortunately this means inserting clefs is rather slow.

    if (!elementNotMovedInY(elt)) {
        RG_DEBUG << "yes (is note, height changed)";
        return true;
    }

    // If the event is a beamed or tied-forward note, then we might
    // need a new item if the distance from this note to the next has
    // changed (because the beam or tie is part of the note's item).

    bool spanning = false;
    (void)(elt->event()->get<Bool>(getProperties().BEAMED, spanning));
    if (!spanning) {
        (void)(elt->event()->get<Bool>(BaseProperties::TIED_FORWARD, spanning));
    }
    if (!spanning) {
        RG_DEBUG << "no (is simple note, height unchanged)";
        return false;
    }

    if (elementShiftedOnly(i)) {
        RG_DEBUG << "no (is spanning note but only shifted)";
        return false;
    }

    RG_DEBUG << "yes (is spanning note with complex move)";
    return true;
}

void
NotationStaff::positionElements(timeT from, timeT to)
{
    RG_DEBUG << "NotationStaff " << this << "::positionElements()"
                   << from << " -> " << to;
    Profiler profiler("NotationStaff::positionElements", true);

    // Following 4 lines are a workaround to not have m_clefChanges and
    // m_keyChanges truncated when positionElements() is called with
    // args outside current segment.
    // Maybe a better fix would be not to call positionElements() with
    // such args ...
    int startTime = getSegment().getStartTime();
    if (from < startTime) from = startTime;
    if (to < startTime) to = startTime;
    if (to == from) return;

    //emit setOperationName(tr("Positioning staff %1...").arg(getId() + 1));
    //emit setValue(0);
    //throwIfCancelled();

// not used
//    const NotationProperties &properties(getProperties());

    int elementsPositioned = 0;
    int elementsRendered = 0; // diagnostic

    Composition *composition = getSegment().getComposition();

// not used
//     timeT nextBarTime = composition->getBarEndForTime(to);

    NotationElementList::iterator beginAt =
        getViewElementList()->findTime(composition->getBarStartForTime(from));

    NotationElementList::iterator endAt =
        getViewElementList()->findTime(composition->getBarEndForTime(to));

    if (beginAt == getViewElementList()->end())
        return ;

    truncateClefsAndKeysAt(static_cast<int>((*beginAt)->getLayoutX()));

    Clef currentClef; // used for rendering key sigs
    bool haveCurrentClef = false;

    ::Rosegarden::Key currentKey;
    bool haveCurrentKey = false;

    for (NotationElementList::iterator it = beginAt, nextIt = beginAt;
         it != endAt; it = nextIt) {

        NotationElement * el = static_cast<NotationElement*>(*it);

        ++nextIt;

        if (el->event()->isa(Clef::EventType)) {

            currentClef = Clef(*el->event());
            m_clefChanges.push_back(ClefChange(int(el->getLayoutX()),
                                               currentClef));
            haveCurrentClef = true;

        } else if (el->event()->isa(::Rosegarden::Key::EventType)) {

            m_keyChanges.push_back(KeyChange(int(el->getLayoutX()),
                                             ::Rosegarden::Key(*el->event())));

            if (!haveCurrentClef) { // need this to know how to present the key
                currentClef = getSegment().getClefAtTime
                              (el->event()->getAbsoluteTime());
                haveCurrentClef = true;
            }

            if (!haveCurrentKey) { // stores the key _before_ this one
                // To correctly render the first key signature of a segment,
                // the current key has to be found from the whole view context,
                // not from the segment alone
//              currentKey = getSegment().getKeyAtTime
//                               (el->event()->getAbsoluteTime() - 1);
                currentKey = m_notationScene->getClefKeyContext()->
                    getKeyFromContext(getSegment().getTrack(),
                                      el->event()->getAbsoluteTime() - 1);
                haveCurrentKey = true;
            }
        }

        bool selected = isSelected(it);
        bool needNewItem = elementNeedsRegenerating(it);

        if (needNewItem) {
            renderSingleElement(it, currentClef, currentKey, selected);
            ++elementsRendered;
        }

        if (el->event()->isa(::Rosegarden::Key::EventType)) {
            // update currentKey after rendering, not before
            currentKey = ::Rosegarden::Key(*el->event());
        }

        if (!needNewItem) {
            StaffLayoutCoords coords = getSceneCoordsForLayoutCoords
                (el->getLayoutX(), (int)el->getLayoutY());
            el->reposition(coords.first, (double)coords.second);
        }

        el->setSelected(selected);

        ++elementsPositioned;

        //if ((to > from) && (elementsPositioned % 300 == 0)) {
            //timeT myTime = el->getViewAbsoluteTime();
            //emit setValue((myTime - from) * 100 / (to - from));
            //throwIfCancelled();
        //}
    }

    RG_DEBUG << "NotationStaff " << this << "::positionElements "
             << from << " -> " << to << ": "
             << elementsPositioned << " elements positioned, "
             << elementsRendered << " re-rendered"
            ;

    NotePixmapFactory::dumpStats(std::cerr);
}

void
NotationStaff::truncateClefsAndKeysAt(int x)
{
    for (std::vector<ClefChange>::iterator i = m_clefChanges.begin();
         i != m_clefChanges.end(); ++i) {
        if (i->first >= x) {
            m_clefChanges.erase(i, m_clefChanges.end());
            break;
        }
    }

    for (std::vector<KeyChange>::iterator i = m_keyChanges.begin();
         i != m_keyChanges.end(); ++i) {
        if (i->first >= x) {
            m_keyChanges.erase(i, m_keyChanges.end());
            break;
        }
    }
}

bool
NotationStaff::elementNotMovedInY(NotationElement *elt)
{
    if (!elt->getItem()) return false;

    StaffLayoutCoords coords = getSceneCoordsForLayoutCoords
                              (elt->getLayoutX(), (int)elt->getLayoutY());

    bool ok = (int)(elt->getSceneY()) == (int)(coords.second);

    if (!ok) {
        RG_DEBUG
            << "elementNotMovedInY: elt at " << elt->getViewAbsoluteTime()
            << ", ok is " << ok;
        RG_DEBUG << "(cf " << (int)(elt->getSceneY()) << " vs "
                       << (int)(coords.second) << ")";
    }
    return ok;
}

bool
NotationStaff::elementShiftedOnly(NotationElementList::iterator i)
{
    int shift = 0;
    bool ok = false;

    for (NotationElementList::iterator j = i;
         j != getViewElementList()->end(); ++j) {

        NotationElement *elt = static_cast<NotationElement*>(*j);
        if (!elt->getItem()) break;

        StaffLayoutCoords coords = getSceneCoordsForLayoutCoords
                                  (elt->getLayoutX(), (int)elt->getLayoutY());

        // regard any shift in y as suspicious
        if ((int)(elt->getSceneY()) != (int)(coords.second)) break;

        int myShift = (int)(elt->getSceneX()) - (int)(coords.first);
        if (j == i) shift = myShift;
        else if (myShift != shift) break;

        if (elt->getViewAbsoluteTime() > (*i)->getViewAbsoluteTime()) {
            // all events up to and including this one have passed
            ok = true;
            break;
        }
    }

    if (!ok) {
        RG_DEBUG
        << "elementShiftedOnly: elt at " << (*i)->getViewAbsoluteTime()
        << ", ok is " << ok;
    }

    return ok;
}

void
NotationStaff::renderSingleElement(ViewElementList::iterator &vli,
                                   const Clef &currentClef,
                                   const ::Rosegarden::Key &currentKey,
                                   bool selected)
{
    const NotationProperties &properties(getProperties());
    static NotePixmapParameters restParams(Note::Crotchet, 0);

    NotationElement* elt = static_cast<NotationElement*>(*vli);

    bool invisible = false;
    if (elt->event()->get
            <Bool>(BaseProperties::INVISIBLE, invisible) && invisible) {
//        if (m_printPainter) return ;
        QSettings settings;
        settings.beginGroup( NotationOptionsConfigGroup );

        bool showInvisibles = qStrToBool( settings.value("showinvisibles", "true" ) ) ;
        settings.endGroup();

        if (!showInvisibles) return;
    }

    // Don't display clef or key already in use
    if (m_hideRedundance &&
        m_notationScene->isEventRedundant(elt->event(), getSegment())) return;

    // Look if element is part of a symlink segment to show it shaded
    bool tmp = false;
    elt->event()->get<Bool>(BaseProperties::TMP, tmp);

    try {
        m_notePixmapFactory->setNoteStyle
            (NoteStyleFactory::getStyleForEvent(elt->event()));

    } catch (const NoteStyleFactory::StyleUnavailable &u) {

        RG_WARNING << "WARNING: Note style unavailable: "
                  << u.getMessage();

        static bool warned = false;
        if (!warned) {
            QMessageBox::critical(nullptr, tr("Rosegarden"), tr( u.getMessage().c_str() ));
            warned = true;
        }
    }

    try {

        QGraphicsItem *item = nullptr;

        m_notePixmapFactory->setSelected(selected);
        m_notePixmapFactory->setShaded(invisible || tmp);
        int z = selected ? 3 : 0;

        FitPolicy policy = PretendItFittedAllAlong;

        RG_DEBUG << "renderSingleElement: Inspecting something at " << elt->event()->getAbsoluteTime();

        RG_DEBUG << "renderSingleElement: Setting selected at " << elt->event()->getAbsoluteTime() << " to " << selected;

        if (elt->isNote()) {
            renderNote(vli);
        } else if (elt->isRest()) {
            // rests can have marks
            int markCount = 0;
            if (elt->event()->has(BaseProperties::MARK_COUNT))
                markCount = elt->event()->get<Int>(BaseProperties::MARK_COUNT);

            if (markCount) {
                RG_DEBUG << "NotationStaff: Houston, we have a rest, and it has marks.  A fermata mayhap?";
                restParams.setMarks(Marks::getMarks(*elt->event()));
                RG_DEBUG << "    marks size: " << restParams.m_marks.size();
            }

            bool ignoreRest = false;
            // NotationHLayout sets this property if it finds the rest
            // in the middle of a chord -- Quantizer still sometimes gets
            // this wrong
            elt->event()->get<Bool>(properties.REST_TOO_SHORT, ignoreRest);

            if (!ignoreRest) {

                Note::Type note =
                    elt->event()->get<Int>(BaseProperties::NOTE_TYPE);
                int dots =
                    elt->event()->get<Int>(BaseProperties::NOTE_DOTS);
                restParams.setNoteType(note);
                restParams.setDots(dots);
                setTuplingParameters(elt, restParams);
                restParams.setQuantized(false);
                bool restOutside = false;
                elt->event()->get<Bool>
                    (properties.REST_OUTSIDE_STAVE, restOutside);
                restParams.setRestOutside(restOutside);
                if (restOutside) {
                    RG_DEBUG << "renderSingleElement() : rest outside staff";
                    if (note == Note::DoubleWholeNote) {
                        RG_DEBUG << "renderSingleElement() : breve rest needs leger lines";
                        restParams.setLegerLines(5);
                    }
                }

                RG_DEBUG << "renderSingleElement: It's a normal rest";
                item = m_notePixmapFactory->makeRest(restParams);
            }

        } else if (elt->event()->isa(Clef::EventType)) {

            RG_DEBUG << "renderSingleElement: It's a clef";
            item = m_notePixmapFactory->makeClef(Clef(*elt->event()));

        } else if (elt->event()->isa(Symbol::EventType)) {

            RG_DEBUG << "renderSingleElement: It's a symbol";
            item = m_notePixmapFactory->makeSymbol(Symbol(*elt->event()));

        } else if (elt->event()->isa(::Rosegarden::Key::EventType)) {

            ::Rosegarden::Key key(*elt->event());
            ::Rosegarden::Key cancelKey = currentKey;

            if (m_keySigCancelMode == 0) { // only when entering C maj / A min

                if (key.getAccidentalCount() != 0)
                    cancelKey = ::Rosegarden::Key();

            } else if (m_keySigCancelMode == 1) { // only when reducing acc count

                if (key.getAccidentalCount() &&
                    ! (key.isSharp() == cancelKey.isSharp() &&
                       key.getAccidentalCount() < cancelKey.getAccidentalCount()
                      )
                   ) {
                    cancelKey = ::Rosegarden::Key();
                }
            }

            RG_DEBUG << "renderSingleElement: It's a key";
            item = m_notePixmapFactory->makeKey(key, currentClef, cancelKey);

        } else if (elt->event()->isa(Text::EventType)) {

            // If event is a lyric and doesn't have the verse property, the
            // document was probably created before the multiple verses feature
            // was written. In such a case there should be only one verse and
            // the property is added with its value set to 0 (ie. first verse).
            //!!! This code should be moved to the place where the rg file is read
            if (elt->event()->has(Text::TextTypePropertyName) &&
                  elt->event()->get<String>(Text::TextTypePropertyName) ==
                  Text::Lyric) {
                if (!elt->event()->has(Text::LyricVersePropertyName)) {
                    elt->event()->set<Int>(Text::LyricVersePropertyName, 0);
                }
            }

            policy = MoveBackToFit;

            if (elt->event()->has(Text::TextTypePropertyName) &&
                elt->event()->get<String>(Text::TextTypePropertyName) ==
                Text::Annotation &&
                !m_notationScene->areAnnotationsVisible()) {

                // nothing I guess

            } else if (elt->event()->has(Text::TextTypePropertyName) &&
                       elt->event()->get<String>(Text::TextTypePropertyName) ==
                       Text::LilyPondDirective &&
                       !m_notationScene->areLilyPondDirectivesVisible()) {

                // nothing here either

            } else if (m_distributeVerses && 
                       elt->event()->has(Text::TextTypePropertyName) &&
                       elt->event()->get<String>(Text::TextTypePropertyName) ==
                       Text::Lyric &&
                       elt->event()->get<Int>(Text::LyricVersePropertyName) !=
                       getSegment().getVerseWrapped()) {

                // nothing here either
                // (this verse have to be displayed with another view of
                //  the current segment)

            } else {

                try {
                    RG_DEBUG << "renderSingleElement: It's a normal text";
                    item = m_notePixmapFactory->makeText(Text(*elt->event()));
                } catch (const Exception &e) { // Text ctor failed
                    RG_DEBUG << "Bad text event";
                }
            }

        } else if (elt->event()->isa(Indication::EventType)) {

            policy = SplitToFit;

            try {
                Indication indication(*elt->event());

                timeT indicationDuration = indication.getIndicationDuration();
                timeT indicationEndTime =
                    elt->getViewAbsoluteTime() + indicationDuration;

                NotationElementList::iterator indicationEnd =
                    getViewElementList()->findTime(indicationEndTime);

                std::string indicationType = indication.getIndicationType();

                int length;
                // int y1;

                if ((indicationType == Indication::Slur ||
                        indicationType == Indication::PhrasingSlur) &&
                        indicationEnd != getViewElementList()->begin()) {
                    --indicationEnd;
                }

                if ((indicationType != Indication::Slur &&
                        indicationType != Indication::PhrasingSlur) &&
                        indicationEnd != getViewElementList()->begin() &&
                        (indicationEnd == getViewElementList()->end() ||
                         indicationEndTime ==
                         getSegment().getBarStartForTime(indicationEndTime))) {

                    while (indicationEnd == getViewElementList()->end() ||
                            (*indicationEnd)->getViewAbsoluteTime() >= indicationEndTime)
                        --indicationEnd;

                    double x, w;
                    static_cast<NotationElement *>(*indicationEnd)->
                    getLayoutAirspace(x, w);
                    length = (int)(x + w - elt->getLayoutX() -
                                   m_notePixmapFactory->getBarMargin());

                } else {

                    length = (int)((*indicationEnd)->getLayoutX() -
                                   elt->getLayoutX());

                    if (indication.isOttavaType()) {
                        length -= m_notePixmapFactory->getNoteBodyWidth();
                    }
                }

                // y1 = (int)(*indicationEnd)->getLayoutY();

                if (length < m_notePixmapFactory->getNoteBodyWidth()) {
                    length = m_notePixmapFactory->getNoteBodyWidth();
                }

                if (indicationType == Indication::Crescendo ||
                    indicationType == Indication::Decrescendo) {

                    item = m_notePixmapFactory->makeHairpin
                            (length, indicationType == Indication::Crescendo);
                } else if (indicationType == Indication::TrillLine) {

                    // skip m_printPainter as it is no longer relevant
                    item = m_notePixmapFactory->makeTrillLine(length);

                } else if (indicationType == Indication::Slur ||
                           indicationType == Indication::PhrasingSlur) {

                    bool above = true;
                    long dy = 0;
                    long length = 10;

                    elt->event()->get<Bool>(properties.SLUR_ABOVE, above);
                    elt->event()->get<Int>(properties.SLUR_Y_DELTA, dy);
                    elt->event()->get<Int>(properties.SLUR_LENGTH, length);

                    item = m_notePixmapFactory->makeSlur
                            (length, dy, above,
                             indicationType == Indication::PhrasingSlur);

                } else if (indicationType == Indication::FigParameterChord) {
                    Text text = Text("Chord");
                    item = m_notePixmapFactory->makeText(text);
                } else if (indicationType == Indication::Figuration) {
                    Text text = Text("Figuration");
                    item = m_notePixmapFactory->makeText(text);
                } else {

                    int octaves = indication.getOttavaShift();

                    if (octaves != 0) {
                        item = m_notePixmapFactory->makeOttava
                                (length, octaves);
                    } else {

                        RG_DEBUG << "Unrecognised indicationType " << indicationType;
                        if (m_showUnknowns) {
                            item = m_notePixmapFactory->makeUnknown();
                        }
                    }
                }
            } catch (...) {
                RG_DEBUG << "Bad indication!";
            }

        } else if (elt->event()->isa(Controller::EventType)) {

            bool isSustain = false;

            long controlNumber = 0;
            elt->event()->get
            <Int>(Controller::NUMBER, controlNumber);

            Studio *studio = &m_notationScene->getDocument()->getStudio();
            Track *track = getSegment().getComposition()->getTrackById
                           (getSegment().getTrack());

            if (track) {

                Instrument *instrument = studio->getInstrumentById
                                         (track->getInstrument());
                if (instrument) {
                    MidiDevice *device = dynamic_cast<MidiDevice *>
                                         (instrument->getDevice());
                    if (device) {
                        for (ControlList::const_iterator i =
                                    device->getControlParameters().begin();
                                i != device->getControlParameters().end(); ++i) {
                            if (i->getType() == Controller::EventType &&
                                    i->getControllerNumber() == controlNumber) {
                                if (i->getName() == "Sustain" ||
                                        strtoqstr(i->getName()) == tr("Sustain")) {
                                    isSustain = true;
                                }
                                break;
                            }
                        }
                    } else if (instrument->getDevice() &&
                               instrument->getDevice()->getType() == Device::SoftSynth) {
                        if (controlNumber == 64) {
                            isSustain = true;
                        }
                    }
                }
            }

            if (isSustain) {
                long value = 0;
                elt->event()->get<Int>(Controller::VALUE, value);
                if (value > 0) {
                    item = m_notePixmapFactory->makePedalDown();
                } else {
                    item = m_notePixmapFactory->makePedalUp();
                }

            } else {

                if (m_showUnknowns) {
                    item = m_notePixmapFactory->makeUnknown();
                }
            }
        } else if (elt->event()->isa(Guitar::Chord::EventType)) {

            // Create a guitar chord pixmap
            try {

                Guitar::Chord chord (*elt->event());
                item = m_notePixmapFactory->makeGuitarChord
                    (chord.getFingering(), 0, 0);
                //                  }
            } catch (const Exception &e) { // GuitarChord ctor failed
                RG_DEBUG << "Bad guitar chord event";
            }

        } else if (elt->event()->isa(SegmentID::EventType)) {
            policy = MoveBackToFit;
            Text text(SegmentID(*elt->event()).NotationString());
            item = m_notePixmapFactory->makeText(text);
        } else if (elt->event()->isa(GeneratedRegion::EventType)) {
            policy = MoveBackToFit;
            Text text(GeneratedRegion(*elt->event()).NotationString());
            item = m_notePixmapFactory->makeText(text);
        } else {

            if (m_showUnknowns) {
                item = m_notePixmapFactory->makeUnknown();
            }
        }

        // Show the result, one way or another

        if (elt->isNote()) {

            // No need, we already set and showed it in renderNote

        } else if (item) {

            setItem(elt, item, z, policy);

        } else {
            elt->removeItem();
        }

    } catch (...) {
        RG_WARNING << "renderSingleElement(): Event lacks the proper properties:";
        RG_WARNING << elt->event();
    }

    m_notePixmapFactory->setSelected(false);
    m_notePixmapFactory->setShaded(false);
}

void
NotationStaff::setItem(NotationElement *elt, QGraphicsItem *item, int z,
                       FitPolicy policy)
{
    double layoutX = elt->getLayoutX();
    int layoutY = (int)elt->getLayoutY();

    elt->removeItem();

    while (1) {

        StaffLayoutCoords coords =
            getSceneCoordsForLayoutCoords(layoutX, layoutY);

        double sceneX = coords.first;
        int sceneY = coords.second;

        QGraphicsPixmapItem *pitem = dynamic_cast<QGraphicsPixmapItem *>(item);
        NoteItem *nitem = dynamic_cast<NoteItem *>(item);

        if (m_pageMode != LinearMode &&
            policy != PretendItFittedAllAlong &&
            (pitem || nitem)) {

            QPixmap pixmap;
            QPointF offset;

            if (pitem) {
                pixmap = pitem->pixmap();
                offset = pitem->offset();
            } else {
                pixmap = nitem->makePixmap();
                offset = nitem->offset();
            }

            int row = getRowForLayoutX(layoutX);
            double rightMargin = getSceneXForRightOfRow(row);
            double extent = sceneX + pixmap.width();

            RG_DEBUG << "setPixmap: row " << row << ", right margin " << rightMargin << ", extent " << extent;

            if (extent > rightMargin + m_notePixmapFactory->getNoteBodyWidth()) {

                if (policy == SplitToFit) {

                    RG_DEBUG << "splitting at " << (rightMargin-sceneX);

                    std::pair<QPixmap, QPixmap> split =
                        PixmapFunctions::splitPixmap(pixmap,
                                                     int(rightMargin - sceneX));

                    QGraphicsPixmapItem *left = new QGraphicsPixmapItem(split.first);
                    left->setOffset(offset);

                    QGraphicsPixmapItem *right = new QGraphicsPixmapItem(split.second);
                    right->setOffset(QPointF(0, offset.y()));

                    getScene()->addItem(left);
                    left->setZValue(z);
                    left->show();

                    if (elt->getItem()) {
                        elt->addItem(left, sceneX, sceneY);
                    } else {
                        elt->setItem(left, sceneX, sceneY);
                    }

                    delete item;
                    item = right;

                    layoutX += rightMargin - sceneX + 0.01; // ensure flip to next row

                    continue;

                } else { // policy == MoveBackToFit

                    elt->setLayoutX(elt->getLayoutX() - (extent - rightMargin));
                    coords = getSceneCoordsForLayoutCoords(layoutX, layoutY);
                    sceneX = coords.first;
                }
            }
        }

        RG_DEBUG << "setItem: item = " << (void *)item << " (pitem = " << (void *)pitem << ", scene = " << item->scene() << ")";

        getScene()->addItem(item);
        item->setZValue(z);
        if (elt->getItem()) {
            elt->addItem(item, sceneX, sceneY);
        } else {
            elt->setItem(item, sceneX, sceneY);
        }
        item->show();
        break;
    }
}

void
NotationStaff::renderNote(ViewElementList::iterator &vli)
{
    NotationElement *elt = static_cast<NotationElement *>(*vli);

    const NotationProperties &properties(getProperties());
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = elt->event()->get<Int>(BaseProperties::NOTE_TYPE);
    int dots = elt->event()->get<Int>(BaseProperties::NOTE_DOTS);

    Accidental accidental = Accidentals::NoAccidental;
    (void)elt->event()->get<String>(properties.DISPLAY_ACCIDENTAL, accidental);

    bool cautionary = false;
    if (accidental != Accidentals::NoAccidental) {
        (void)elt->event()->get<Bool>
            (properties.DISPLAY_ACCIDENTAL_IS_CAUTIONARY, cautionary);
    }

    bool up = true;
    //    (void)(elt->event()->get<Bool>(properties.STEM_UP, up));
    (void)(elt->event()->get<Bool>(properties.VIEW_LOCAL_STEM_UP, up));

    bool flag = true;
    (void)(elt->event()->get<Bool>(properties.DRAW_FLAG, flag));

    bool beamed = false;
    (void)(elt->event()->get<Bool>(properties.BEAMED, beamed));

    bool shifted = false;
    (void)(elt->event()->get<Bool>(properties.NOTE_HEAD_SHIFTED, shifted));

    bool dotShifted = false;
    (void)(elt->event()->get<Bool>(properties.NOTE_DOT_SHIFTED, dotShifted));

    long stemLength = m_notePixmapFactory->getNoteBodyHeight();
    (void)(elt->event()->get<Int>(properties.UNBEAMED_STEM_LENGTH, stemLength));

    long heightOnStaff = 0;
    int legerLines = 0;

    (void)(elt->event()->get<Int>(properties.HEIGHT_ON_STAFF, heightOnStaff));
    if (heightOnStaff < 0) {
        legerLines = heightOnStaff;
    } else if (heightOnStaff > 8) {
        legerLines = heightOnStaff - 8;
    }

    long slashes = 0;
    (void)(elt->event()->get<Int>(properties.SLASHES, slashes));

    bool quantized = false;
    if (m_colourQuantize && !elt->isTuplet()) {
        quantized =
            (elt->getViewAbsoluteTime() != elt->event()->getAbsoluteTime() ||
             elt->getViewDuration() != elt->event()->getDuration());
    }
    params.setQuantized(quantized);

    Event *e = elt->event();
    NotePixmapParameters::Triggering triggering;
    if (e->has(BaseProperties::TRIGGER_EXPAND)) {
        bool sounds = (e->get<Bool>(BaseProperties::TRIGGER_EXPAND));
        if (sounds)
            { triggering = NotePixmapParameters::triggerYes; }
        else { triggering = NotePixmapParameters::triggerSkip; }
    } else {
        if (e->has(BaseProperties::TRIGGER_SEGMENT_ID))
            { triggering = NotePixmapParameters::triggerYes; }
        else { triggering = NotePixmapParameters::triggerNone; }
    }

    params.setTrigger(triggering);

    bool inRange = true;
    Pitch p(*elt->event());
    Segment *segment = &getSegment();
    if (m_showRanges) {
        int pitch = p.getPerformancePitch();
        if (pitch > segment->getHighestPlayable() ||
                pitch < segment->getLowestPlayable()) {
            inRange = false;
        }
    }
    params.setInRange(inRange);

    params.setNoteType(note);
    params.setDots(dots);
    params.setAccidental(accidental);
    params.setAccidentalCautionary(cautionary);
    params.setNoteHeadShifted(shifted);
    params.setNoteDotShifted(dotShifted);
    params.setDrawFlag(flag);
    params.setDrawStem(true);
    params.setStemGoesUp(up);
    params.setLegerLines(legerLines);
    params.setSlashes(slashes);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.removeMarks();
    params.setSafeVertDistance(0);

    bool primary = false;
    int safeVertDistance = 0;

    if (elt->event()->get<Bool>(properties.CHORD_PRIMARY_NOTE, primary)
        && primary) {

        long marks = 0;
        elt->event()->get<Int>(properties.CHORD_MARK_COUNT, marks);
        if (marks) {
            NotationChord chord(*getViewElementList(), vli,
                                m_segment.getComposition()->getNotationQuantizer(),
                                properties);
            params.setMarks(chord.getMarksForChord());
        }

        //          params.setMarks(Marks::getMarks(*elt->event()));

        if (up && note < Note::Semibreve) {
            safeVertDistance = m_notePixmapFactory->getStemLength();
            safeVertDistance = std::max(safeVertDistance, int(stemLength));
        }
    }

    long tieLength = 0;
    (void)(elt->event()->get<Int>(properties.TIE_LENGTH, tieLength));
    if (tieLength > 0) {
        params.setTied(true);
        params.setTieLength(tieLength);
    } else {
        params.setTied(false);
    }

    if (elt->event()->has(BaseProperties::TIE_IS_ABOVE)) {
        params.setTiePosition
            (true, elt->event()->get<Bool>(BaseProperties::TIE_IS_ABOVE));
    } else {
        params.setTiePosition(false, false); // the default
    }

    long accidentalShift = 0;
    bool accidentalExtra = false;
    if (elt->event()->get<Int>(properties.ACCIDENTAL_SHIFT, accidentalShift)) {
        elt->event()->get<Bool>(properties.ACCIDENTAL_EXTRA_SHIFT, accidentalExtra);
    }
    params.setAccidentalShift(accidentalShift);
    params.setAccExtraShift(accidentalExtra);

    double airX, airWidth;
    elt->getLayoutAirspace(airX, airWidth);
    params.setWidth(int(airWidth));

    if (beamed) {

        if (elt->event()->get<Bool>(properties.CHORD_PRIMARY_NOTE, primary)
            && primary) {

            int myY = elt->event()->get<Int>(properties.BEAM_MY_Y);

            stemLength = myY - (int)elt->getLayoutY();
            if (stemLength < 0)
                stemLength = -stemLength;

            int nextBeamCount =
                elt->event()->get<Int>(properties.BEAM_NEXT_BEAM_COUNT);
            int width =
                elt->event()->get<Int>(properties.BEAM_SECTION_WIDTH);
            int gradient =
                elt->event()->get<Int>(properties.BEAM_GRADIENT);

            bool thisPartialBeams(false), nextPartialBeams(false);
            (void)elt->event()->get<Bool>
                (properties.BEAM_THIS_PART_BEAMS, thisPartialBeams);
            (void)elt->event()->get<Bool>
                (properties.BEAM_NEXT_PART_BEAMS, nextPartialBeams);

            params.setBeamed(true);
            params.setNextBeamCount(nextBeamCount);
            params.setThisPartialBeams(thisPartialBeams);
            params.setNextPartialBeams(nextPartialBeams);
            params.setWidth(width);
            params.setGradient((double)gradient / 100.0);
            if (up)
                safeVertDistance = stemLength;

        }
        else {
            params.setBeamed(false);
            params.setDrawStem(false);
        }
    }

    if (heightOnStaff < 7) {
        int gap = (((7 - heightOnStaff) * m_notePixmapFactory->getLineSpacing()) / 2);
        if (safeVertDistance < gap)
            safeVertDistance = gap;
    }

    params.setStemLength(stemLength);
    params.setSafeVertDistance(safeVertDistance);
    setTuplingParameters(elt, params);

    NotePixmapFactory *factory = m_notePixmapFactory;

    if (elt->isGrace()) {
        // lift this code from elsewhere to fix #1930309, and it seems to work a
        // treat, as y'all Wrongpondians are wont to say
        params.setLegerLines(heightOnStaff < 0 ? heightOnStaff :
                                                 heightOnStaff > 8 ? heightOnStaff - 8 : 0);
        m_graceNotePixmapFactory->setSelected(m_notePixmapFactory->isSelected());
        m_graceNotePixmapFactory->setShaded(m_notePixmapFactory->isShaded());
        factory = m_graceNotePixmapFactory;
    }

    bool memberOfParallel = false;
    elt->event()->get<Bool>(BaseProperties::MEMBER_OF_PARALLEL, memberOfParallel);
    params.setMemberOfParallel(memberOfParallel);

    bool collision = false;
    QGraphicsItem *haloItem = nullptr;
    if (m_showCollisions) {
        collision = elt->isColliding();
        if (collision) {
            // Make collision halo
            haloItem = factory->makeNoteHalo(params);
            haloItem->setZValue(-1);
        }
    }

    QGraphicsItem *item = factory->makeNote(params);

    int z = 0;
    if (factory->isSelected()) z = 3;
    else if (quantized) z = 2;

    setItem(elt, item, z, SplitToFit);

    if (collision) {
        // Display collision halo
        StaffLayoutCoords coords =
                getSceneCoordsForLayoutCoords(elt->getLayoutX(),
                                              elt->getLayoutY());
        double sceneX = coords.first;
        int sceneY = coords.second;
        elt->addItem(haloItem, sceneX, sceneY);
        getScene()->addItem(haloItem);
        haloItem->show();
    }
}

void
NotationStaff::setTuplingParameters(NotationElement *elt,
                                    NotePixmapParameters &params)
{
    const NotationProperties &properties(getProperties());

    params.setTupletCount(0);
    long tuplingLineY = 0;
    bool tupled =
        (elt->event()->get<Int>(properties.TUPLING_LINE_MY_Y, tuplingLineY));

    if (tupled) {

        long tuplingLineWidth = 0;
        if (!elt->event()->get<Int>
            (properties.TUPLING_LINE_WIDTH, tuplingLineWidth)) {
            RG_WARNING << "WARNING: Tupled event at " << elt->event()->getAbsoluteTime() << " has no tupling line width";
        }

        long tuplingLineGradient = 0;
        if (!(elt->event()->get<Int>
              (properties.TUPLING_LINE_GRADIENT, tuplingLineGradient))) {
            RG_WARNING << "WARNING: Tupled event at " << elt->event()->getAbsoluteTime() << " has no tupling line gradient";
        }

        bool tuplingLineFollowsBeam = false;
        elt->event()->get<Bool>
            (properties.TUPLING_LINE_FOLLOWS_BEAM, tuplingLineFollowsBeam);

        long tupletCount;
        if (elt->event()->get<Int>
            (BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT, tupletCount)) {

            params.setTupletCount(tupletCount);
            params.setTuplingLineY(tuplingLineY - (int)elt->getLayoutY());
            params.setTuplingLineWidth(tuplingLineWidth);
            params.setTuplingLineGradient(double(tuplingLineGradient) / 100.0);
            params.setTuplingLineFollowsBeam(tuplingLineFollowsBeam);
        }
    }
}

bool
NotationStaff::isSelected(NotationElementList::iterator it)
{
    const EventSelection *selection = m_notationScene->getSelection();
    return selection && selection->contains((*it)->event());
}

void
NotationStaff::showPreviewNote(double layoutX, int heightOnStaff,
                               const Note &note, bool grace,
                               Accidental accidental, bool cautious,
                               QColor color)
{
    NotePixmapFactory *npf = m_notePixmapFactory;
    if (grace) npf = m_graceNotePixmapFactory;

    NotePixmapParameters params(note.getNoteType(), note.getDots());
    NotationRules rules;

    params.setAccidental(accidental);
    params.setAccidentalCautionary(cautious);
    params.setNoteHeadShifted(false);
    params.setDrawFlag(true);
    params.setDrawStem(true);
    params.setStemGoesUp(rules.isStemUp(heightOnStaff));
    params.setLegerLines(heightOnStaff < 0 ? heightOnStaff :
                         heightOnStaff > 8 ? heightOnStaff - 8 : 0);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.setTied(false);
    params.setBeamed(false);
    params.setTupletCount(0);
    params.setSelected(false);
    // params.setHighlighted(true);
    params.setForcedColor(color);

    delete m_previewItem;
    m_previewItem = npf->makeNote(params);

    int layoutY = getLayoutYForHeight(heightOnStaff);
    StaffLayoutCoords coords = getSceneCoordsForLayoutCoords(layoutX, layoutY);

    getScene()->addItem(m_previewItem);
    m_previewItem->setPos(coords.first, (double)coords.second);
    m_previewItem->setZValue(4);
    m_previewItem->show();
}

void
NotationStaff::clearPreviewNote()
{
    if (!m_previewItem) return;
    m_previewItem->hide();
    delete m_previewItem;
    m_previewItem = nullptr;
}

bool
NotationStaff::wrapEvent(Event *e)
{
    bool wrap = true;

    /*!!! always wrap unknowns, just don't necessarily render them?

        if (!m_showUnknowns) {
        std::string etype = e->getType();
        if (etype != Note::EventType &&
            etype != Note::EventRestType &&
            etype != Clef::EventType &&
            etype != Key::EventType &&
            etype != Indication::EventType &&
            etype != Text::EventType) {
            wrap = false;
        }
        }
    */

    if (wrap) wrap = ViewSegment::wrapEvent(e);

    return wrap;
}

void
NotationStaff::eventRemoved(const Segment *segment,
                            Event *event)
{
    ViewSegment::eventRemoved(segment, event);
    m_notationScene->handleEventRemoved(event);
}

void
NotationStaff::regenerate(timeT from, timeT to, bool secondary)
{
    Profiler profiler("NotationStaff::regenerate", true);

    // Secondary is true if this regeneration was caused by edits to
    // another staff, and the content of this staff has not itself
    // changed.

    // The staff must have been re-layed-out (by the layout engine)
    // before this is called to regenerate its elements.

    //!!! NB This does not yet correctly handle clef and key lists!

    if (to < from) {
        RG_WARNING << "NotationStaff::regenerate(" << from << ", " << to << ", "
                  << secondary << "): ERROR: to < from";
        to = from;
    }

    from = getSegment().getBarStartForTime(from);
    to = getSegment().getBarEndForTime(to);

    NotationElementList::iterator i = getViewElementList()->findTime(from);
    NotationElementList::iterator j = getViewElementList()->findTime(to);

    int resetCount = 0;
    if (!secondary) {
        for (NotationElementList::iterator k = i; k != j; ++k) {
            if (*k) {
                static_cast<NotationElement *>(*k)->removeItem();
                ++resetCount;
            }
        }
    }
    RG_DEBUG << "regenerate: explicitly reset items for " << resetCount << " elements";

    Profiler profiler2("NotationStaff::regenerate: repositioning", true);

    //!!! would be simpler if positionElements could also be called
    //!!! with iterators -- if renderElements/positionElements are
    //!!! going to be internal functions, it's OK and more consistent
    //!!! for them both to take itrs.  positionElements has a quirk
    //!!! that makes it not totally trivial to change (use of
    //!!! nextBarTime)

    if (i != getViewElementList()->end()) {
        positionElements((*i)->getViewAbsoluteTime(),
                         getSegment().getEndMarkerTime());
    } else {
        // Shouldn't happen; if it does, let's re-do everything just in case
        positionElements(getSegment().getStartTime(),
                         getSegment().getEndMarkerTime());
    }

}

void
NotationStaff::checkAndCompleteClefsAndKeys(int bar)
{
    // Look for Clef or Key in current bar
    Composition *composition = getSegment().getComposition();
    timeT barStartTime = composition->getBarStart(bar);
    timeT barEndTime = composition->getBarEnd(bar);

    for (ViewElementList::iterator it =
             getViewElementList()->findTime(barStartTime);
         (it != getViewElementList()->end()) &&
             ((*it)->getViewAbsoluteTime() < barEndTime);
         ++it) {

        if ((*it)->event()->isa(Clef::EventType)) {

            // Clef found
            Clef clef = *(*it)->event();

            // Is this clef already in m_clefChanges list ?
            int xClef = int((*it)->getLayoutX());
            bool found = false;
            for (size_t i = 0; i < m_clefChanges.size(); ++i) {
                if (    (m_clefChanges[i].first == xClef)
                    && (m_clefChanges[i].second == clef)) {
                    found = true;
                    break;
                }
            }

            // If not, add it
            if (!found) {
                m_clefChanges.push_back(ClefChange(xClef, clef));
            }

        } else if ((*it)->event()->isa(::Rosegarden::Key::EventType)) {

            ::Rosegarden::Key key = *(*it)->event();

            // Is this key already in m_keyChanges list ?
            int xKey = int((*it)->getLayoutX());
            bool found = false;
            for (size_t i = 0; i < m_keyChanges.size(); ++i) {
                if (    (m_keyChanges[i].first == xKey)
                    && (m_keyChanges[i].second == key)) {
                    found = true;
                    break;
                }
            }

            // If not, add it
            if (!found) {
                m_keyChanges.push_back(KeyChange(xKey, key));
            }
        }
    }
}

StaffLayout::BarStyle
NotationStaff::getBarStyle(int barNo) const
{
    const Segment *s = &getSegment();
    Composition *c = s->getComposition();

    int firstBar = c->getBarNumber(s->getStartTime());
    int lastNonEmptyBar = c->getBarNumber(s->getEndMarkerTime() - 1);

    // Currently only the first and last bar in a segment have any
    // possibility of getting special treatment:
    if (barNo > firstBar && barNo <= lastNonEmptyBar)
        return PlainBar;

    // First and last bar in a repeating segment get repeat bars
    // unless segment is a temporary clone.

    if (s->isRepeating() && !s->isTmp()) {
        if (barNo == firstBar)
            return RepeatStartBar;
        else if (barNo == lastNonEmptyBar + 1)
            return RepeatEndBar;
    }

    if (barNo <= lastNonEmptyBar)
        return PlainBar;

    // Last bar on a given track gets heavy double bars.  Exploit the
    // fact that Composition's iterator returns segments in track
    // order.

    Segment *lastSegmentOnTrack = nullptr;

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
        if ((*i)->getTrack() == s->getTrack()) {
            lastSegmentOnTrack = *i;
        } else if (lastSegmentOnTrack != nullptr) {
            break;
        }
    }

    if (&getSegment() == lastSegmentOnTrack)
        return HeavyDoubleBar;
    else
        return PlainBar;
}

double
NotationStaff::getBarInset(int barNo, bool isFirstBarInRow) const
{
    StaffLayout::BarStyle style = getBarStyle(barNo);

    RG_DEBUG << "getBarInset(" << barNo << "," << isFirstBarInRow << ")";

    if (!(style == RepeatStartBar || style == RepeatBothBar))
        return 0.0;

    const Segment &s = getSegment();
    Composition *composition = s.getComposition();
    timeT barStart = composition->getBarStart(barNo);

    double inset = 0.0;

    RG_DEBUG << "ready";

    bool haveKey = false, haveClef = false;

    ::Rosegarden::Key key;
    ::Rosegarden::Key cancelKey;
    Clef clef;

    for (Segment::const_iterator i = s.findTime(barStart);
         s.isBeforeEndMarker(i) && ((*i)->getNotationAbsoluteTime() == barStart);
         ++i) {

        RG_DEBUG << "type " << (*i)->getType() << " at " << (*i)->getNotationAbsoluteTime();

        if ((*i)->isa(::Rosegarden::Key::EventType)) {

            try {
                key = ::Rosegarden::Key(**i);

                if (barNo > composition->getBarNumber(s.getStartTime())) {
                    // Since the redundant clefs and keys may be hide, the
                    // current key may be defined on some other segment and
                    // whe have to find it in the whole notation scene clef/key
                    // context.
//                  cancelKey = s.getKeyAtTime(barStart - 1);
                    cancelKey = m_notationScene->getClefKeyContext()->
                        getKeyFromContext(getSegment().getTrack(), barStart - 1);
                }

                if (m_keySigCancelMode == 0) { // only when entering C maj / A min

                    if (key.getAccidentalCount() != 0)
                        cancelKey = ::Rosegarden::Key();

                } else if (m_keySigCancelMode == 1) { // only when reducing acc count

                    if (key.getAccidentalCount() &&
                        ! (key.isSharp() == cancelKey.isSharp() &&
                           key.getAccidentalCount() < cancelKey.getAccidentalCount()
                          )
                       ) {
                        cancelKey = ::Rosegarden::Key();
                    }
                }

                // Is the key hide because redundant ?
                if (m_hideRedundance &&
                    m_notationScene->isEventRedundant(const_cast<Event *>(*i),
                                                      const_cast<Segment &>(getSegment()))) {
                    haveKey = false;
                } else {
                    haveKey = true;
                }

            } catch (...) {
                RG_DEBUG << "getBarInset: Bad key in event";
            }

        } else if ((*i)->isa(Clef::EventType)) {

            try {
                clef = Clef(**i);
 
                // Is the clef hide because redundant ?
                if (m_hideRedundance &&
                    m_notationScene->isEventRedundant(const_cast<Event *>(*i),
                                                      const_cast<Segment &>(getSegment()))) {
                    haveClef = false;
                } else {
                    haveClef = true;
                }
            } catch (...) {
                RG_WARNING << "getBarInset: Bad clef in event";
            }
        }
    }

    if (isFirstBarInRow) {
        if (!haveKey) {
            key = s.getKeyAtTime(barStart);
            haveKey = true;
        }
        if (!haveClef) {
            clef = s.getClefAtTime(barStart);
            haveClef = true;
        }
    }

    if (haveKey) {
        inset += m_notePixmapFactory->getKeyWidth(key, cancelKey);
    }
    if (haveClef) {
        inset += m_notePixmapFactory->getClefWidth(clef);
    }
    if (haveClef || haveKey) {
        inset += m_notePixmapFactory->getBarMargin() / 3;
    }
    if (haveClef && haveKey) {
        inset += m_notePixmapFactory->getNoteBodyWidth() / 2;
    }

    RG_DEBUG << "getBarInset(" << barNo << "," << isFirstBarInRow << "): inset " << inset;


    return inset;
}

Rosegarden::ViewElement *
NotationStaff::makeViewElement(Rosegarden::Event* e)
{
    return new NotationElement(e);
}

bool
NotationStaff::includesTime(timeT t)
{
    Composition *composition = getSegment().getComposition();
    
    // In order to find the correct starting and ending bar of the
    // segment, make infinitesimal shifts (+1 and -1) towards its
    // center.

    timeT t0 = composition->getBarStartForTime
        (getSegment().getClippedStartTime() + 1);

    timeT t1 = composition->getBarEndForTime
        (getSegment().getEndMarkerTime() - 1);

    return (t >= t0 && t < t1);
}

}
