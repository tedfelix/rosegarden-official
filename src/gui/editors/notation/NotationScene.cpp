/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[NotationScene]"
#define RG_NO_DEBUG_PRINT 1

#include "NotationScene.h"

#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/BaseProperties.h"

#include "NotationStaff.h"
#include "NotationHLayout.h"
#include "NotationVLayout.h"
#include "NotePixmapFactory.h"
#include "ClefKeyContext.h"
#include "NotationProperties.h"
#include "NotationTool.h"
#include "NotationWidget.h"
#include "NotationMouseEvent.h"
#include "NoteFontFactory.h"
#include "gui/widgets/Panned.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include "misc/ConfigGroups.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "base/Profiler.h"

#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"

#include <QApplication>
#include <QSettings>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

using std::vector;

namespace Rosegarden
{

static int instanceCount = 0;

NotationScene::NotationScene() :
    m_widget(nullptr),
    m_document(nullptr),
    m_properties(),
    m_notePixmapFactory(nullptr),
    m_notePixmapFactorySmall(nullptr),
    m_clefKeyContext(new ClefKeyContext),
    m_selection(nullptr),
    m_hlayout(nullptr),
    m_vlayout(nullptr),
    m_title(nullptr),
    m_subtitle(nullptr),
    m_composer(nullptr),
    m_copyright(nullptr),
    m_pageMode(StaffLayout::LinearMode),
    m_printSize(5),
    m_leftGutter(0),
    m_currentStaff(0),
    m_visibleStaffs(0),
    m_compositionRefreshStatusId(0),
    m_timeSignatureChanged(false),
    m_updatesSuspended(false),
    m_minTrack(0),
    m_maxTrack(0),
    m_finished(false),
    m_sceneIsEmpty(false),
    m_showRepeated(false),
    m_editRepeated(false),
    m_haveInittedCurrentStaff(false),
    m_previewNoteStaff(nullptr)
{
    QString prefix(QString("NotationScene%1::").arg(instanceCount++));
    m_properties.reset(new NotationProperties(qstrtostr(prefix)));

//    qRegisterMetaType<NotationMouseEvent>("Rosegarden::NotationMouseEvent");

    setNotePixmapFactories();
}

NotationScene::~NotationScene()
{
      if (m_document) {
        if (!isCompositionDeleted()) { // implemented in CompositionObserver
            m_document->getComposition().removeObserver(this);
        }
    }
    delete m_hlayout;
    delete m_vlayout;
    delete m_notePixmapFactory;
    delete m_notePixmapFactorySmall;
    delete m_title;
    delete m_subtitle;
    delete m_composer;
    delete m_copyright;
    delete m_selection;

    for (unsigned int i = 0; i < m_segments.size(); ++i)
        m_segments[i]->removeObserver(m_clefKeyContext);
    delete m_clefKeyContext;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) delete m_staffs[i];

    for (std::vector<Segment *>::iterator it = m_clones.begin();
         it != m_clones.end(); ++it) {
        delete (*it);
    }
}

void
NotationScene::setNotePixmapFactories(QString fontName, int size)
{
    delete m_notePixmapFactory;
    delete m_notePixmapFactorySmall;

    m_notePixmapFactory = new NotePixmapFactory(fontName, size);

    fontName = m_notePixmapFactory->getFontName();
    size = m_notePixmapFactory->getSize();

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(fontName);
    int small = size;
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size || sizes[i] > size*3 / 4) break;
        small = sizes[i];
    }

    // small NPF needs to know target size for grace noteheads and normal size
    // so it can scale everything else sensibly
    m_notePixmapFactorySmall = new NotePixmapFactory(fontName, size, small);

    if (m_hlayout) m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    if (m_vlayout) m_vlayout->setNotePixmapFactory(m_notePixmapFactory);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setNotePixmapFactories(m_notePixmapFactory,
                                            m_notePixmapFactorySmall);
    }
}

QString
NotationScene::getFontName() const
{
    return m_notePixmapFactory->getFontName();
}

void
NotationScene::setFontName(const QString& name)
{
    if (name == getFontName()) return;
    setNotePixmapFactories(name, getFontSize());
    if (!m_updatesSuspended) {
        positionStaffs();
        layoutAll();
    }
}

int
NotationScene::getFontSize() const
{
    return m_notePixmapFactory->getSize();
}

void
NotationScene::setFontSize(int size)
{
    if (size == getFontSize()) return;
    setNotePixmapFactories(getFontName(), size);
    if (!m_updatesSuspended) {
        positionStaffs();
        layoutAll();
    }
}

int
NotationScene::getHSpacing() const
{
    return m_hlayout->getSpacing();
}

void
NotationScene::setHSpacing(int spacing)
{
    if (spacing == getHSpacing()) return;
    m_hlayout->setSpacing(spacing);
    if (!m_updatesSuspended) {
        positionStaffs();
        layoutAll();
    }
}

/* unused
int
NotationScene::getLeftGutter() const
{
    return m_leftGutter;
}
*/

void
NotationScene::setLeftGutter(int gutter)
{
    m_leftGutter = gutter;
    positionStaffs();
}

void
NotationScene::setNotationWidget(NotationWidget *w)
{
    m_widget = w;
}

const RulerScale *
NotationScene::getRulerScale() const
{
    return m_hlayout;
}

NotationStaff *
NotationScene::getCurrentStaff()
{
    if (m_currentStaff < (int)m_staffs.size()) {
        return m_staffs[m_currentStaff];
    } else {
        return nullptr;
    }
}

void
NotationScene::setCurrentStaff(NotationStaff *staff)
{
    if (! staff) return;
    // To unallow the direct edition of a repeated segment do it never be
    // the current one
    if (m_showRepeated && !m_editRepeated) {
        if (staff->getSegment().isTmp()) return;
    }

    for (int i = 0; i < int(m_staffs.size()); ++i) {
        if (m_staffs[i] == staff) {
            if (m_currentStaff != i) {
                m_currentStaff = i;
                emit currentStaffChanged();
                emit currentViewSegmentChanged(staff);
                break;
            }
        }
    }
    // update highlighting
    NotationStaff* currentStaff = getCurrentStaff();
    Segment* currentSegment = &(currentStaff->getSegment());
    TrackId currentTrack = currentSegment->getTrack();

    RG_DEBUG << "highlight current" <<
        currentStaff << currentSegment << currentTrack;
    for (int i = 0; i < int(m_staffs.size()); ++i) {
        NotationStaff* iStaff = m_staffs[i];
        Segment* iSegment = &(iStaff->getSegment());
        TrackId iTrack = iSegment->getTrack();
        bool onSameTrack = (currentTrack == iTrack);
        RG_DEBUG << "highlight iter" << iStaff << iSegment << onSameTrack;
        bool highlight = true;
        if (iSegment != currentSegment && onSameTrack &&
            m_highlightMode == "highlight_within_track") highlight = false;
        if (iStaff != currentStaff &&
            m_highlightMode == "highlight") highlight = false;
        // do not affect repeats
        if (iSegment->isTmp()) highlight = true;

        RG_DEBUG << "highlight staff" << highlight;
        m_staffs[i]->setHighlight(highlight);
    }
}

void
NotationScene::setStaffs(RosegardenDocument *document,
                          vector<Segment *> segments)
{

    if (m_document && document != m_document) {
        m_document->getComposition().removeObserver(this);
    }

    // ClefKeyContext doesn't keep any segments list. So notation scene
    // has to maintain segment observer connections for it.
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->removeObserver(m_clefKeyContext);
    }

     // Delete clones of repeating segment if any
    for (std::vector<Segment *>::iterator it = m_clones.begin();
        it != m_clones.end(); ++it) {
        delete (*it);
    }
    m_clones.clear();

    m_document = document;
    m_externalSegments = segments;
    Composition * composition = &m_document->getComposition();


    /// Look for repeating segments

    // Get display/edition settings
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    m_showRepeated =  settings.value("showrepeated", true).toBool();
    m_editRepeated =  settings.value("editrepeated", false).toBool();
    settings.endGroup();

    if (m_showRepeated) {
        createClonesFromRepeatedSegments();
        // External segments and clones are now mixed inside m_segments
    } else {
        m_segments = m_externalSegments;
        // No clone in that case
    }


    composition->addObserver(this);

    m_compositionRefreshStatusId = composition->getNewRefreshStatusId();

    delete m_hlayout;
    delete m_vlayout;

    m_hlayout = new NotationHLayout(composition,
                                    m_notePixmapFactory,
                                    *m_properties,
                                    this);

    m_vlayout = new NotationVLayout(composition,
                                    m_notePixmapFactory,
                                    *m_properties,
                                    this);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i];
    }
    m_staffs.clear();

    std::set<TrackId> trackIds;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        NotationStaff *staff = new NotationStaff
            (this,
             m_segments[i],
             nullptr, // no snap grid for notation
             i,
             m_notePixmapFactory,
             m_notePixmapFactorySmall);

        m_staffs.push_back(staff);

        // To assume segments are trackId ordered is no more true (was it ?)
        // since clones may be found at the end of segments vector.
        // The trackIds set is used to count how many visible staffs we have.
        TrackId id = m_segments[i]->getTrack();
        trackIds.insert(id);
    }

    m_visibleStaffs = trackIds.size();

    m_clefKeyContext->setSegments(this);

    // Remember the names of the tracks
    for (std::set<TrackId>::iterator i = trackIds.begin();
         i != trackIds.end(); ++i) {
        Track *track = composition->getTrackById(*i);
        Q_ASSERT(track);
        m_trackLabels[*i] = track->getLabel();
    }

    // ClefKeyContext doesn't keep any segments list. So notation scene
    // has to maintain segment observer connections for it.
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->addObserver(m_clefKeyContext);
    }

    // We don't know a good current staff now.  This is correct even
    // if we are resetting an existing NotationScene because the old
    // current staff may not even exist.
    m_haveInittedCurrentStaff = false;

    if (!m_updatesSuspended) {
        positionStaffs();
        layoutAll();
        initCurrentStaffIndex();
    }

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &NotationScene::slotCommandExecuted);
}


void
NotationScene::createClonesFromRepeatedSegments()
{
    const Segment::Participation participation =
        m_editRepeated ? Segment::editableClone : Segment::justForShow;

    // Create clones (if needed)
    for (std::vector<Segment *>::iterator it = m_externalSegments.begin();
        it != m_externalSegments.end(); ++it) {
        if ((*it)->isRepeating()) {
            timeT targetStart = (*it)->getStartTime();
            timeT targetEnd = (*it)->getEndMarkerTime();
            timeT repeatEnd = (*it)->getRepeatEndTime();
            timeT targetDuration = targetEnd - targetStart;
            TrackId track = (*it)->getTrack();
            int verse = (*it)->getVerse();
//             RG_DEBUG << "Creating clones   track=" << track
//                       << " targetStart=" << targetStart
//                       << " targetEnd=" << targetEnd
//                       << " repeatEnd=" << repeatEnd << "\n";
            for (timeT ts = targetStart + targetDuration;
                ts < repeatEnd; ts += targetDuration) {
                timeT te = ts + targetDuration;
                // RG_DEBUG << "   clone [" << ts << ", " << te << "]";

                /// Segment *s = (*it)->clone();
                Segment *s = SegmentLinker::createLinkedSegment(*it);
                s->setStartTime(ts);
                s->setTrack(track);
                s->setVerse(++verse);
                s->setParticipation(participation);
                s->setTmp();  // To avoid crash related to composition
                              // being undefined and to get notation
                              // with grey color
                if (repeatEnd < te) {
                    s->setEndMarkerTime(repeatEnd);
                    // RG_DEBUG << " shortened to " << repeatEnd;
                }
                m_clones.push_back(s);
            }
            (*it)->setAsReference();
        }
    }

    // Add possible clones to the list of segments
    m_segments = m_externalSegments;
    for (std::vector<Segment *>::iterator it = m_clones.begin();
        it != m_clones.end(); ++it) {
        m_segments.push_back(*it);
    }
}


void
NotationScene::suspendLayoutUpdates()
{
    m_updatesSuspended = true;
}

void
NotationScene::resumeLayoutUpdates()
{
    m_updatesSuspended = false;
    // may be more work than we really want to do, depending on what
    // happened while updates were suspended
    positionStaffs();
    layoutAll();
    initCurrentStaffIndex();
}

NotationStaff *
NotationScene::getStaffForSceneCoords(double x, int y) const
{
    // (i)  Do not change staff, if mouse was clicked within the current staff.

    StaffLayout *s = nullptr;

    if (m_currentStaff < (int)m_staffs.size()) {
        s = m_staffs[m_currentStaff];
    }

    if (s && s->containsSceneCoords(x, y)) {

        StaffLayout::StaffLayoutCoords coords =
            s->getLayoutCoordsForSceneCoords(x, y);

        timeT t = m_hlayout->getTimeForX(coords.first);

        if (m_staffs[m_currentStaff]->includesTime(t)) {
            return m_staffs[m_currentStaff];
        }
    }

    // (ii) Find staff under cursor, if clicked outside the current staff.

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        // Never return a staff which can't be edited directly
        if (m_showRepeated && !m_editRepeated) {
            if (m_staffs[i]->getSegment().isTmp()) continue;
        }

        StaffLayout *s = m_staffs[i];

        if (s->containsSceneCoords(x, y)) {

            StaffLayout::StaffLayoutCoords coords =
                s->getLayoutCoordsForSceneCoords(x, y);

	    timeT t = m_hlayout->getTimeForX(coords.first);

	    if (m_staffs[i]->includesTime(t)) {
                return m_staffs[i];
            }
        }
    }

    return nullptr;
}

NotationStaff *
NotationScene::getStaffAbove(timeT t)
{
    return getNextStaffVertically(-1, t);
}

NotationStaff *
NotationScene::getStaffBelow(timeT t)
{
    return getNextStaffVertically(1, t);
}

NotationStaff *
NotationScene::getPriorStaffOnTrack()
{
    return getNextStaffHorizontally(-1, false);
}

NotationStaff *
NotationScene::getNextStaffOnTrack()
{
    return getNextStaffHorizontally(1, false);
}

NotationStaff *
NotationScene::getStaffBySegmentMarking(const QString& marking) const
{
    for (unsigned int i=0; i<m_staffs.size(); ++i) {
        NotationStaff* staff = m_staffs[i];
        QString staffMarking = staff->getMarking();
        if (staffMarking == marking) {
            return staff;
        }
    }
    return nullptr;
}

NotationStaff *
NotationScene::getStaffbyTrackAndTime(const Track *track, timeT targetTime)
{
    // Prepare a fallback: If this is the right track but no staff
    // includes time t, we'll return the fallback instead.  We
    // don't try to find the best fallback.
    NotationStaff * fallback = nullptr;
    timeT minTime = 1.0e10;
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (m_staffs[i]->getSegment().getTrack() == track->getId()) {
            if(m_staffs[i]->includesTime(targetTime)) {
                return m_staffs[i];
            } else {
                // find the closest staff to the targetTime
                timeT dt1 = abs(targetTime - m_staffs[i]->getStartTime());
                if (dt1 < minTime) {
                    minTime = dt1;
                    fallback = m_staffs[i];
                }
                timeT dt2 = abs(targetTime - m_staffs[i]->getEndTime());
                if (dt2 < minTime) {
                    minTime = dt2;
                    fallback = m_staffs[i];
                }
            }
        }
    }
    // We found segments on the track, but none that include time
    // t.  In this circumstance, we still want to return a staff
    // so return the fallback.
    return fallback;
}

// @params
// direction is 1 if higher-numbered tracks are wanted, -1 if
// lower-numbered ones are.
// t is a time that the found staff should contain if possible.
NotationStaff *
NotationScene::getNextStaffVertically(int direction, timeT t)
{
    if (m_staffs.size() < 2 || m_currentStaff >= (int)m_staffs.size()) return nullptr;

    NotationStaff *current = m_staffs[m_currentStaff];
    Composition *composition = &m_document->getComposition();
    Track *track = composition->getTrackById(current->getSegment().getTrack());
    if (!track) return nullptr;

    int position = track->getPosition();
    Track *newTrack = nullptr;

    while ((newTrack = composition->getTrackByPosition(position + direction))) {
        NotationStaff * staff = getStaffbyTrackAndTime(newTrack, t);
        if (staff) { return staff; }
        position += direction;
    }

    return nullptr;
}

NotationStaff *
NotationScene::getNextStaffHorizontally(int direction, bool cycle)
{
    if (m_staffs.size() < 2 || m_currentStaff >= (int)m_staffs.size()) return nullptr;

    NotationStaff *current = m_staffs[m_currentStaff];
    //Composition *composition = &m_document->getComposition();
    TrackId trackId = current->getSegment().getTrack();

    QMultiMap<timeT, NotationStaff *> timeMap;
    for (size_t i = 0; i < m_staffs.size(); ++i) {
        if (m_staffs[i]->getSegment().getTrack() == trackId) {
            timeMap.insert(m_staffs[i]->getSegment().getClippedStartTime(), m_staffs[i]);
        }
    }

    QMultiMap<timeT, NotationStaff *>::iterator i =
        timeMap.find(current->getSegment().getClippedStartTime(), current);

    if (i == timeMap.end()) {
        RG_WARNING << "Argh! Can't find staff I just put in map";
        return nullptr;
    }

    if (direction < 0) {
        if (i == timeMap.begin()) {
            if (cycle) i = timeMap.end();
            else return nullptr;
        }
        --i;
    } else {
        ++i;
        if (i == timeMap.end()) {
            if (cycle) i = timeMap.begin();
            else return nullptr;
        }
    }

    return i.value();
}

// Initialize which staff is current.  We try to choose one containing
// the playback pointer.
void
NotationScene::initCurrentStaffIndex()
{
    // Only do this if we haven't done it before since the last reset,
    // otherwise we'll annoy the user.
    if (m_haveInittedCurrentStaff) { return; }
    m_haveInittedCurrentStaff = true;

    // Can't do much if we have no staffs.
    if (m_staffs.empty()) { return; }

    Composition &composition = m_document->getComposition();
    timeT targetTime = composition.getPosition();

    // Try the globally selected track (which we may not even include
    // any segments from)
    {
        const Track *track = composition.getTrackById(composition.getSelectedTrack());
        NotationStaff *staff = track ? getStaffbyTrackAndTime(track, targetTime) : nullptr;
        if (staff) {
            setCurrentStaff(staff);
            return;
        }
    }

    // Try m_minTrack, which we surely include some segment from.
    {
        // Careful, m_minTrack is an int indicating position, not a
        // TrackId, and must be converted.
        const Track *track =
            composition.getTrackByPosition(m_minTrack);
        NotationStaff *staff = getStaffbyTrackAndTime(track, targetTime);
        if (staff) {
            setCurrentStaff(staff);
            return;
        }
    }

    // We shouldn't reach here.
    RG_WARNING << "Argh! Failed to find a staff!";
}


Segment *
NotationScene::getCurrentSegment()
{
    NotationStaff *s = nullptr;

    if (m_currentStaff < (int)m_staffs.size()) {
        s = m_staffs[m_currentStaff];
    }

    if (s) return &s->getSegment();
    return nullptr;
}

bool
NotationScene::segmentsContainNotes() const
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        const Segment *segment = m_segments[i];

        for (Segment::const_iterator i = segment->begin();
             segment->isBeforeEndMarker(i); ++i) {

            if (((*i)->getType() == Note::EventType)) {
                return true;
            }
        }
    }

    return false;
}

void
NotationScene::setupMouseEvent(QGraphicsSceneMouseEvent *e,
                               NotationMouseEvent &nme)
{
    setupMouseEvent(e->scenePos(), e->buttons(), e->modifiers(), nme);
}

void
NotationScene::setupMouseEvent(QGraphicsSceneWheelEvent *e,
                               NotationMouseEvent &nme)
{
    setupMouseEvent(e->scenePos(), e->buttons(), e->modifiers(), nme);
}


void
NotationScene::setupMouseEvent(QPointF scenePos, Qt::MouseButtons buttons,
                               Qt::KeyboardModifiers modifiers,
                               NotationMouseEvent &nme)
{
    //Profiler profiler("NotationScene::setupMouseEvent");

    double sx = scenePos.x();
    int sy = lrint(scenePos.y());

    nme.sceneX = sx;
    nme.sceneY = sy;

    nme.modifiers = modifiers;
    nme.buttons = buttons;
    nme.element = nullptr;
    nme.staff = getStaffForSceneCoords(sx, sy);

    bool haveClickHeight = false;

    //!!! are any of our tools able to make proper use of e->time?
    // would it be more useful if it was the absolute time of e->element
    // rather than the click time on the staff?

    if (nme.staff) {

        Event *clefEvent = nullptr, *keyEvent = nullptr;
        NotationElementList::iterator i =
            nme.staff->getElementUnderSceneCoords(sx, sy, clefEvent, keyEvent);

        if (i != nme.staff->getViewElementList()->end()) {
            nme.element = dynamic_cast<NotationElement *>(*i);
        }
        if (clefEvent) nme.clef = Clef(*clefEvent);

//        RG_DEBUG << "clef = " << nme.clef.getClefType() << " (have = " << (clefEvent != 0) << ")";

        if (keyEvent) nme.key = ::Rosegarden::Key(*keyEvent);

//        RG_DEBUG << "key = " << nme.key.getName() << " (have = " << (keyEvent != 0) << ")";

        nme.time = nme.staff->getTimeAtSceneCoords(sx, sy);
        nme.height = nme.staff->getHeightAtSceneCoords(sx, sy);
        haveClickHeight = true;

    } else {
        nme.element = nullptr;
        nme.time = 0;
        nme.height = 0;
    }

    // we've discovered what the context is -- now check whether we're
    // clicking on something specific

    const QList<QGraphicsItem *> collisions = items(scenePos);
    //RG_DEBUG << "setupMouseEvent collisions:" << collisions.size();

    NotationElement *clickedNote = nullptr;
    NotationElement *clickedVagueNote = nullptr;
    NotationElement *clickedNonNote = nullptr;

    for (QList<QGraphicsItem *>::const_iterator i = collisions.begin();
         i != collisions.end(); ++i) {

        NotationElement *element = NotationElement::getNotationElement(*i);
        if (!element) continue;
        RG_DEBUG << "setupMouseEvent event:" << *(element->event());

        // #957364 (Notation: Hard to select upper note in chords of
        // seconds) -- adjust x-coord for shifted note head

        double cx = element->getSceneX();

        int nbw = m_notePixmapFactory->getNoteBodyWidth();
        bool shifted = false;

        if (element->event()->get<Bool>
            (m_properties->NOTE_HEAD_SHIFTED, shifted) && shifted) {
            bool stemUp = false;
            element->event()->get<Bool>(m_properties->VIEW_LOCAL_STEM_UP,
                                        stemUp);
            RG_DEBUG << "setupMouseEvent shift" << cx << nbw << stemUp;

            if (stemUp) {
                cx += nbw;
            } else {
                cx -= nbw;
            }
        }

        if (element->isNote() && haveClickHeight) {

            long eventHeight = 0;

            if (element->event()->get<Int>
                (NotationProperties::HEIGHT_ON_STAFF, eventHeight)) {

                if (eventHeight == nme.height) {

                    RG_DEBUG << "setupMouseEvent t1" << nme.sceneX << cx <<
                        nbw;
                    if (!clickedNote &&
                        nme.sceneX >= cx &&
                        nme.sceneX <= cx + nbw) {
                        RG_DEBUG << "setupMouseEvent cn1" << element;
                        clickedNote = element;
                    } else if (!clickedVagueNote &&
                               nme.sceneX >= cx - 2 &&
                               nme.sceneX <= cx + nbw + 2) {
                        RG_DEBUG << "setupMouseEvent cnv1" << element;
                        clickedVagueNote = element;
                    }

                } else if (eventHeight - 1 == nme.height ||
                           eventHeight + 1 == nme.height) {
                    if (!clickedVagueNote) {
                        RG_DEBUG << "setupMouseEvent cv2" << element;
                        clickedVagueNote = element;
                    }
                }
            }
        } else if (!element->isNote()) {
            if (!clickedNonNote) {
                RG_DEBUG << "setupMouseEvent cn2" << element;
                clickedNonNote = element;
            }
        }
        RG_DEBUG << "setupMouseEvent" << clickedNote;
    }

    nme.exact = false;

    if (clickedNote) {
        nme.element = clickedNote;
        nme.exact = true;
    } else if (clickedNonNote) {
        nme.element = clickedNonNote;
        nme.exact = true;
    } else if (clickedVagueNote) {
        nme.element = clickedVagueNote;
        nme.exact = true;
    }

    /*RG_DEBUG << "NotationScene::setupMouseEvent: sx = " << sx
                   << ", sy = " << sy
                   << ", modifiers = " << nme.modifiers
                   << ", buttons = " << nme.buttons
                   << ", element = " << nme.element
                   << ", staff = " << nme.staff
                   << " (id = " << (nme.staff ? nme.staff->getId() : -1) << ")"
                   << ", clef = " << nme.clef.getClefType()
                   << ", key = " << nme.key.getName()
                   << ", time = " << nme.time
                   << ", height = " << nme.height;*/
}

void
NotationScene::slotMouseLeavesView()
{
    clearPreviewNote();
}

void
NotationScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    m_widget->dispatchMousePress(&nme);
}

void
NotationScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    m_widget->dispatchMouseMove(&nme);
}

void
NotationScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    m_widget->dispatchMouseRelease(&nme);
}

void
NotationScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    NotationMouseEvent nme;
    setupMouseEvent(e, nme);
    m_widget->dispatchMouseDoubleClick(&nme);
}

void
NotationScene::wheelEvent(QGraphicsSceneWheelEvent *e)
{
    if (m_widget->getCurrentTool()->needsWheelEvents()) {
        NotationMouseEvent nme;
        setupMouseEvent(e, nme);
        m_widget->dispatchWheelTurned(e->delta(), &nme);
        e->accept();    // Don't pass the event to the view
    }
}

void
NotationScene::keyPressEvent(QKeyEvent * keyEvent)
{
    processKeyboardEvent(keyEvent);
}

void
NotationScene::keyReleaseEvent(QKeyEvent * keyEvent)
{
    processKeyboardEvent(keyEvent);
}

void
NotationScene::processKeyboardEvent(QKeyEvent * keyEvent)
{
    int key = keyEvent->key();
    if ((key == Qt::Key_Shift) || (key == Qt::Key_Control)) {

        // Get the global coordinates of the cursor and convert them to
        // scene coordinates
        QPoint globalPos = QCursor::pos();
        QPoint pos = m_widget->getView()->viewport()->mapFromGlobal(globalPos);
        QPointF scenePos = m_widget->getView()->mapToScene(pos);

        // Create a NotationMouseEvent related to the QKeyEvent.
        // Use queryKeyboardModifiers() rather than keyboardModifiers()
        // to ensure the current value of modifiers is read.
        NotationMouseEvent nme;
        setupMouseEvent(scenePos, QApplication::mouseButtons(),
                        QApplication::queryKeyboardModifiers(), nme);

        // Handle it as a mouse event
        m_widget->dispatchMouseMove(&nme);;
    }
}

int
NotationScene::getPageWidth()
{
    if (m_pageMode != StaffLayout::MultiPageMode) {

        if (isInPrintMode()) {
            return sceneRect().width();
        }

        return m_widget->width() -
//!!!            m_widget->verticalScrollBar()->width() -
            m_leftGutter - 10;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(210.0 / mmPerPixel);
    }
}

int
NotationScene::getPageHeight()
{
    if (m_pageMode != StaffLayout::MultiPageMode) {

        if (isInPrintMode()) {
            return sceneRect().height();
        }

        return m_widget->height();

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        return (int)(297.0 / mmPerPixel);
    }
}

void
NotationScene::getPageMargins(int &left, int &top)
{
    if (m_pageMode != StaffLayout::MultiPageMode) {

        left = 0;
        top = 0;

    } else {

        //!!! For the moment we use A4 for this calculation

        double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
        double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
        left = (int)(20.0 / mmPerPixel);
        top = (int)(15.0 / mmPerPixel);
    }
}

void
NotationScene::setPageMode(StaffLayout::PageMode mode)
{
    if (m_pageMode == mode) return;
    m_pageMode = mode;

    int pageWidth = getPageWidth();
    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    m_hlayout->setPageMode(m_pageMode != StaffLayout::LinearMode);
    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    NOTATION_DEBUG << "NotationScene::setPageMode: set layout's page width to "
                   << (pageWidth - leftMargin * 2);

    if (!m_updatesSuspended) {
        positionStaffs();
        layoutAll();
    }
/*!!!
    positionPages();


    if (!m_printMode) {
        updateView();
        slotSetInsertCursorPosition(getInsertionTime(), false, false);
        slotSetPointerPosition(RosegardenDocument::currentDocument->getComposition().getPosition(), false);
    }
*/
}

void
NotationScene::slotCommandExecuted()
{
    checkUpdate();
}

void
NotationScene::timeSignatureChanged(const Composition *c)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;
    m_timeSignatureChanged = true;
}

timeT
NotationScene::getInsertionTime(bool allowEndTime) const
{
    if (!m_document) return 0;
    return snapTimeToNoteBoundary(m_document->getComposition().getPosition(),
                                  allowEndTime);
}

NotationScene::CursorCoordinates
NotationScene::getCursorCoordinates(timeT t) const
{
    if (m_staffs.empty() || !m_hlayout)
        return CursorCoordinates();

    const NotationStaff *topStaff = nullptr;
    const NotationStaff *bottomStaff = nullptr;

    // Find the topmost and bottom-most staves.
    for (uint i = 0; i < m_staffs.size(); ++i) {
        const NotationStaff *staff = m_staffs[i];
        if (!staff)
            continue;

        // If first or higher, save it.
        if (!topStaff  ||  staff->getY() < topStaff->getY())
            topStaff = staff;
        // If first or lower, save it.
        if (!bottomStaff  ||  staff->getY() > bottomStaff->getY())
            bottomStaff = staff;
    }

    const timeT snapped = snapTimeToNoteBoundary(t, true);

    const double x = m_hlayout->getXForTime(t);
    const double sx = m_hlayout->getXForTimeByEvent(snapped);

    StaffLayout::StaffLayoutCoords top =
        topStaff->getSceneCoordsForLayoutCoords
        (x, topStaff->getLayoutYForHeight(24));

    StaffLayout::StaffLayoutCoords bottom =
        bottomStaff->getSceneCoordsForLayoutCoords
        (x, bottomStaff->getLayoutYForHeight(-16));

    StaffLayout::StaffLayoutCoords singleTop = top;
    StaffLayout::StaffLayoutCoords singleBottom = bottom;

    const NotationStaff *currentStaff = nullptr;
    if (m_currentStaff < (int)m_staffs.size())
        currentStaff = m_staffs[m_currentStaff];

    if (currentStaff) {
        singleTop =
            currentStaff->getSceneCoordsForLayoutCoords
            (sx, currentStaff->getLayoutYForHeight(16));
        singleBottom =
            currentStaff->getSceneCoordsForLayoutCoords
            (sx, currentStaff->getLayoutYForHeight(-8));
    }

    CursorCoordinates cc;
    cc.allStaffs = QLineF(top.first, top.second,
                          bottom.first, bottom.second);

    // if the time is within the current segment we take the
    // singleTop/Bottom cursor for time accuracy within that
    // segment. If the time is outside the current segment we just use
    // the top/bottom x coordinate to avoid the cursor jumping to the
    // current segment.
    cc.currentStaff = QLineF(top.first, singleTop.second,
                             bottom.first, singleBottom.second);
    if (currentStaff && currentStaff->includesTime(t)) {
        // take the cursor position caluclated from the layout
        cc.currentStaff = QLineF(singleTop.first, singleTop.second,
                                 singleBottom.first, singleBottom.second);
    }

    return cc;
}

timeT
NotationScene::snapTimeToNoteBoundary(timeT t, bool allowEndTime) const
{
    NotationStaff *s = nullptr;
    if (m_currentStaff < (int)m_staffs.size()) {
        s = m_staffs[m_currentStaff];
    }
    if (!s) return t;

    ViewElementList *v = s->getViewElementList();
    ViewElementList::iterator i = v->findNearestTime(t);
    if (i == v->end()) i = v->begin();
    if (i == v->end()) return t;

    timeT eventTime = (*i)->getViewAbsoluteTime();
    if (allowEndTime && t > eventTime) {
        timeT staffEnd = s->getEndTime();
        if (t > staffEnd) t = staffEnd;
        return t;
    }
    return eventTime;
}

void
NotationScene::checkUpdate()
{
    bool need = false;
    bool all = false;
    timeT start = 0, end = 0;
    int count = 0;
    NotationStaff *single = nullptr;

    bool compositionModified = m_document &&
        m_document->getComposition().getRefreshStatus
        (m_compositionRefreshStatusId).needsRefresh();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        SegmentRefreshStatus &rs = m_staffs[i]->getRefreshStatus();

        if (m_timeSignatureChanged ||
            (rs.needsRefresh() && compositionModified)) {

            need = true;
            all = true;

            // don't break, because we want to reset refresh statuses
            // on other segments as well

        } else if (rs.needsRefresh()) {

            if (!need || rs.from() < start) start = rs.from();
            if (!need || rs.to() > end) end = rs.to();

            need = true;

            single = m_staffs[i];
            ++count;
        }

        rs.setNeedsRefresh(false);
    }

    m_timeSignatureChanged = false;
    m_document->getComposition().getRefreshStatus
        (m_compositionRefreshStatusId).setNeedsRefresh(false);

    if (need) {
        if (all) layoutAll();
        else {
            // Test count to fix bug #2973777
            if (count == 1) layout(single, start, end);
            else  layout(nullptr, start, end);
        }
    }
}

///YG: Only for debug
void
NotationScene::dumpVectors()
{
    RG_DEBUG << "dumpVectors begin";
    for (unsigned int i=0; i<m_externalSegments.size(); ++i) {
        RG_DEBUG << "extern" << i << ":" << m_externalSegments[i] <<
            m_externalSegments[i]->getLabel().c_str();
        if (m_externalSegments[i]->isTmp()) RG_DEBUG << " TMP";
        if (m_externalSegments[i]->isLinked()) RG_DEBUG << " LINKED";
        if (m_externalSegments[i]->isTrulyLinked()) RG_DEBUG << " TRULYLINKED";
        RG_DEBUG << "start=" << m_externalSegments[i]->getStartTime()
                  << "endMrkr=" << m_externalSegments[i]->getEndMarkerTime();
    }
    for (unsigned int i=0; i<m_clones.size(); ++i) {
        RG_DEBUG << "clones" << i << ":" << m_clones[i];
        if (m_clones[i]->isTmp()) RG_DEBUG << " TMP";
        RG_DEBUG << "start=" << m_clones[i]->getStartTime()
                  << "endMrkr=" << m_clones[i]->getEndMarkerTime();
    }
    for (unsigned int i=0; i<m_segments.size(); ++i) {
        RG_DEBUG << "segment" << i << ":" << m_segments[i] <<
            m_segments[i]->getLabel().c_str();
        if (m_segments[i]->isTmp()) RG_DEBUG << " TMP";
        m_segments[i]->dumpObservers();
    }
    for (unsigned int i=0; i<m_staffs.size(); ++i) {
        RG_DEBUG << "staff" << i << ":" << &m_staffs[i]->getSegment() <<
            m_staffs[i]->getSegment().getLabel().c_str();
        if (m_staffs[i]->getSegment().isTmp()) RG_DEBUG << " TMP";
    }
    RG_DEBUG << "dumpVectors end";
}

void
NotationScene::segmentRemoved(const Composition *comp, Segment *segment)
{
    NOTATION_DEBUG << "NotationScene::segmentRemoved(" << comp << "," << segment << ")";

    if (!m_document)
        return;
    if (!comp)
        return;
    if (comp != &m_document->getComposition())
        return;

    // Determine whether we care about this Segment being deleted.

    std::vector<NotationStaff *>::iterator staffToDelete = m_staffs.end();

    // For each NotationStaff in the NotationScene...
    for (std::vector<NotationStaff *>::iterator i = m_staffs.begin();
         i != m_staffs.end();
         ++i) {
        // Found it?
        if (segment == &(*i)->getSegment()) {
            staffToDelete = i;
            break;
        }
    }

    // Not found?  Not a Segment we care about.  Bail.
    if (staffToDelete == m_staffs.end())
        return;

    // This is one of our Segments being deleted.

    // Collect all the staffs that need deleting into here.
    std::set<NotationStaff *> staffsToDelete;

    // Always delete the staff for the deleted segment.
    staffsToDelete.insert(*staffToDelete);

    // For each staff, check for staffs (repeats) linked to the deleted
    // segment and add to staffsToDelete.
    for (NotationStaff *staff : m_staffs) {
        Segment *sseg = &staff->getSegment();
        if (sseg->isLinkedTo(segment)) {
            staffsToDelete.insert(staff);
            RG_DEBUG << "segmentRemoved linked" << staff << segment << sseg <<
                sseg->isTmp() << sseg->isLinkedTo(segment) <<
                staffsToDelete.size();
        }
    }

    // Remember segment to be deleted.
    m_segmentsDeleted.push_back(segment);

    // The segmentDeleted() signal is about to be emitted.  Therefore
    // the whole scene is going to be deleted then restored (from
    // NotationView) and to continue processing at best is useless and
    // at the worst may cause a crash when the segment is deleted.
    disconnect(CommandHistory::getInstance(),
                   &CommandHistory::commandExecuted,
               this, &NotationScene::slotCommandExecuted);
    // Useful ???
    suspendLayoutUpdates();

    // All will be deleted?  Indicate empty.
    if (m_segmentsDeleted.size() == m_externalSegments.size())
        m_sceneIsEmpty = true;

    // Signal must be emitted only once. Nevertheless, all removed
    // segments have to be remembered.
    if (!m_finished)
        emit sceneNeedsRebuilding();

    // Stop further processing from this scene
    m_finished = true;

    // The sceneNeedsRebuilding signal will cause the scene to be
    // rebuilt.  However this uses a queued connection so the pointer
    // update after undo is done before this so we must remove the
    // deleted staff here.

    // Always clear preview note.
    clearPreviewNote();


    // Assemble the new list of staffs (m_staffs).

    std::vector<NotationStaff *> staffsNew;

    for (NotationStaff *staff : m_staffs) {
        // If we need to delete it, do so.
        if (staffsToDelete.find(staff) != staffsToDelete.end()) {
            delete staff;
        } else {  // Keep it in the staff list.
            staffsNew.push_back(staff);
        }
    }

    m_staffs = staffsNew;


    // Redo the layouts so that there aren't any stray pointers
    // to the removed staff.
    layout(nullptr, 0, 0);
}

void
NotationScene::segmentRepeatChanged(const Composition *c, Segment *s, bool)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;

    // Signal must be emitted only once (or the same scene will be recreated
    // several time which may be very time consuming).
    if (m_finished) return;

    for (std::vector<Segment *>::iterator i = m_externalSegments.begin();
         i != m_externalSegments.end(); ++i) {
        if (s == *i) {
            // The segmentRepeatModified() signal is about to be emitted.
            // Therefore the whole scene is going to be deleted then restored
            // (from NotationView) and there is no point to continue processing
            // signals from the current NotationScene.
            disconnect(CommandHistory::getInstance(),
                           &CommandHistory::commandExecuted,
                       this, &NotationScene::slotCommandExecuted);
            suspendLayoutUpdates();
            m_finished = true;    // Stop further processing from this scene

            emit sceneNeedsRebuilding();
            break;
        }
    }
}

void
NotationScene::segmentRepeatEndChanged(const Composition *c, Segment *s, timeT)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;

    // Signal must be emitted only once (or the same scene will be recreated
    // several time which may be very time consuming).
    if (m_finished) return;

    for (std::vector<Segment *>::iterator i = m_externalSegments.begin();
         i != m_externalSegments.end(); ++i) {
        if (s == *i) {

            // The segmentRepeatModified() signal is about to be emitted.
            // Therefore the whole scene is going to be deleted then restored
            // (from NotationView) and to continue processing at best is
            // useless and at worst may cause a crash related to deleted clones.
            disconnect(CommandHistory::getInstance(),
                           &CommandHistory::commandExecuted,
                       this, &NotationScene::slotCommandExecuted);
            suspendLayoutUpdates();
            m_finished = true;    // Stop further processing from this scene

            emit sceneNeedsRebuilding();
            break;
        }
    }
}

void
NotationScene::segmentStartChanged(const Composition *c, Segment *s, timeT)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;

    // Signal must be emitted only once (else the same scene will be recreated
    // several time which may be very time consuming).
    if (m_finished) return;

    for (std::vector<Segment *>::iterator i = m_externalSegments.begin();
         i != m_externalSegments.end(); ++i) {
        if ((s == *i) && (s->isRepeating())) {

            // The segmentRepeatModified() signal is about to be emitted.
            // Therefore the whole scene is going to be deleted then restored
            // (from NotationView) and to continue processing at best is
            // useless and at worst may cause a crash related to deleted clones.
            disconnect(CommandHistory::getInstance(),
                           &CommandHistory::commandExecuted,
                       this, &NotationScene::slotCommandExecuted);
            suspendLayoutUpdates();
            m_finished = true;    // Stop further processing from this scene

            emit sceneNeedsRebuilding();
            break;
        }
    }
}

void
NotationScene::segmentEndMarkerChanged(const Composition *c, Segment *s, bool)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;

    // Signal must be emitted only once (or the same scene will be recreated
    // several time which may be very time consuming).
    if (m_finished) return;

    for (std::vector<Segment *>::iterator i = m_externalSegments.begin();
         i != m_externalSegments.end(); ++i) {
        if ((s == *i) && (s->isRepeating())) {

            // The segmentRepeatModified() signal is about to be emitted.
            // Therefore the whole scene is going to be deleted then restored
            // (from NotationView) and to continue processing at best is
            // useless and at worst may cause a crash related to deleted clones.
            disconnect(CommandHistory::getInstance(),
                           &CommandHistory::commandExecuted,
                       this, &NotationScene::slotCommandExecuted);
            suspendLayoutUpdates();
            m_finished = true;    // Stop further processing from this scene

            emit sceneNeedsRebuilding();
            break;
        }
    }
}

void
NotationScene::trackChanged(const Composition *c, Track *t)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;

    // Signal must be emitted only once (or the same scene will be recreated
    // several time which may be very time consuming).
    if (m_finished) return;

    TrackId trackId = t->getId();   // Id of changed track

    for (std::vector<Segment *>::iterator i = m_externalSegments.begin();
         i != m_externalSegments.end(); ++i) {

        // Is the segment part of the changed track ?
        if ((*i)->getTrack() == trackId) {

            // The scene needs a rebuild only if what has changed is the
            // name of the track
            if (t->getLabel() == m_trackLabels[trackId]) break;

            // The whole scene is going to be deleted then restored
            // (from NotationView). To continue processing at best is
            // useless and at worst may cause a crash related to deleted clones.
            disconnect(CommandHistory::getInstance(),
                           &CommandHistory::commandExecuted,
                       this, &NotationScene::slotCommandExecuted);
            suspendLayoutUpdates();
            m_finished = true;    // Stop further processing from this scene

            emit sceneNeedsRebuilding();
            break;
        }
   }
}

void
NotationScene::positionStaffs()
{
    NOTATION_DEBUG << "NotationView::positionStaffs";
    if (m_staffs.empty()) return;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    m_printSize = settings.value("printingnotesize", 5).toUInt() ;

    m_minTrack = m_maxTrack = 0;
    bool haveMinTrack = false;

    m_trackHeights.clear();
    m_trackCoords.clear();

    int pageWidth, pageHeight, leftMargin, topMargin;
    pageWidth = getPageWidth();
    pageHeight = getPageHeight();
    leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);

    int accumulatedHeight;
    int rowsPerPage = 1;
    int legerLines = 8;
    if (m_pageMode != StaffLayout::LinearMode) legerLines = 7;
    int rowGapPercent = (m_staffs.size() > 1 ? 40 : 10);
    int aimFor = -1;

    bool done = false;

    int titleHeight = 0;

    delete m_title;
    delete m_subtitle;
    delete m_composer;
    delete m_copyright;
    m_title = m_subtitle = m_composer = m_copyright = nullptr;

    if (m_pageMode == StaffLayout::MultiPageMode) {

        const Configuration &metadata =
            m_document->getComposition().getMetadata();

        QFont defaultFont(NotePixmapFactory::defaultSerifFontFamily);

        QVariant fv = settings.value("textfont", defaultFont);
        QFont font(defaultFont);
        if (fv.canConvert<QFont>()) font = fv.value<QFont>();

        font.setPixelSize(m_notePixmapFactory->getSize() * 5);
        QFontMetrics metrics(font);

        if (metadata.has(CompositionMetadataKeys::Title)) {
            QString title(strtoqstr(metadata.get<String>
                                    (CompositionMetadataKeys::Title)));
            m_title = new QGraphicsTextItem(title);
            m_title->setDefaultTextColor(Qt::black);
            addItem(m_title);
            m_title->setFont(font);
            m_title->setPos(m_leftGutter + pageWidth / 2 - metrics.boundingRect(title).width() / 2,
                            20 + topMargin / 4 + metrics.ascent());
            m_title->show();
            titleHeight += metrics.height() * 3 / 2 + topMargin / 4;
        }

        font.setPixelSize(m_notePixmapFactory->getSize() * 3);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Subtitle)) {
            QString subtitle(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Subtitle)));
            m_subtitle = new QGraphicsTextItem(subtitle);
            m_subtitle->setDefaultTextColor(Qt::black);
            addItem(m_subtitle);
            m_subtitle->setFont(font);
            m_subtitle->setPos(m_leftGutter + pageWidth / 2 - metrics.boundingRect(subtitle).width() / 2,
                               20 + titleHeight + metrics.ascent());
            m_subtitle->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        if (metadata.has(CompositionMetadataKeys::Composer)) {
            QString composer(strtoqstr(metadata.get<String>
                                       (CompositionMetadataKeys::Composer)));
            m_composer = new QGraphicsTextItem(composer);
            m_composer->setDefaultTextColor(Qt::black);
            addItem(m_composer);
            m_composer->setFont(font);
            m_composer->setPos(m_leftGutter + pageWidth - metrics.boundingRect(composer).width() - leftMargin,
                               20 + titleHeight + metrics.ascent());
            m_composer->show();
            titleHeight += metrics.height() * 3 / 2;
        }

        font.setPixelSize(m_notePixmapFactory->getSize() * 2);
        metrics = QFontMetrics(font);

        if (metadata.has(CompositionMetadataKeys::Copyright)) {
            QString copyright(strtoqstr(metadata.get<String>
                                        (CompositionMetadataKeys::Copyright)));
            m_copyright = new QGraphicsTextItem(copyright);
            m_copyright->setDefaultTextColor(Qt::black);
            addItem(m_copyright);
            m_copyright->setFont(font);
            m_copyright->setPos(m_leftGutter + leftMargin,
                                20 + pageHeight - topMargin - metrics.descent());
            m_copyright->show();
        }
    }
    settings.endGroup();

    while (1) {

        accumulatedHeight = 0;
        int maxTrackHeight = 0;

        m_trackHeights.clear();

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->setLegerLineCount(legerLines);

            int height = m_staffs[i]->getHeightOfRow();
            Segment &segment = m_staffs[i]->getSegment();
            TrackId trackId = segment.getTrack();
            Composition *composition = segment.getComposition();
            Q_ASSERT(composition);
            Track *track = composition->getTrackById(trackId);

            if (!track)
                continue; // This Should Not Happen, My Friend

            int trackPosition = track->getPosition();

            TrackIntMap::iterator hi = m_trackHeights.find(trackPosition);
            if (hi == m_trackHeights.end()) {
                m_trackHeights.insert(TrackIntMap::value_type
                                    (trackPosition, height));
            } else if (height > hi->second) {
                hi->second = height;
            }

            if (height > maxTrackHeight)
                maxTrackHeight = height;

            if (trackPosition < m_minTrack || !haveMinTrack) {
                m_minTrack = trackPosition;
                haveMinTrack = true;
            }
            if (trackPosition > m_maxTrack) {
                m_maxTrack = trackPosition;
            }
        }

        for (int i = m_minTrack; i <= m_maxTrack; ++i) {
            TrackIntMap::iterator hi = m_trackHeights.find(i);
            if (hi != m_trackHeights.end()) {
                m_trackCoords[i] = accumulatedHeight;
                accumulatedHeight += hi->second;
            }
        }

        accumulatedHeight += maxTrackHeight * rowGapPercent / 100;

        if (done)
            break;

        if (m_pageMode != StaffLayout::MultiPageMode) {

            rowsPerPage = 0;
            done = true;
            break;

        } else {

            // Check how well all this stuff actually fits on the
            // page.  If things don't fit as well as we'd like, modify
            // at most one parameter so as to save some space, then
            // loop around again and see if it worked.  This iterative
            // approach is inefficient but the time spent here is
            // neglible in context, and it's a simple way to code it.

            int staffPageHeight = pageHeight - topMargin * 2 - titleHeight;
            rowsPerPage = staffPageHeight / accumulatedHeight;

            if (rowsPerPage < 1) {

                if (legerLines > 5)
                    --legerLines;
                else if (rowGapPercent > 20)
                    rowGapPercent -= 10;
                else if (legerLines > 4)
                    --legerLines;
                else if (rowGapPercent > 0)
                    rowGapPercent -= 10;
                else if (legerLines > 3)
                    --legerLines;
                else if (m_printSize > 3)
                    --m_printSize;
                else { // just accept that we'll have to overflow
                    rowsPerPage = 1;
                    done = true;
                }

            } else {

                if (aimFor == rowsPerPage) {

                    titleHeight +=
                        (staffPageHeight - (rowsPerPage * accumulatedHeight)) / 2;

                    done = true;

                } else {

                    if (aimFor == -1)
                        aimFor = rowsPerPage + 1;

                    // we can perhaps accommodate another row, with care
                    if (legerLines > 5)
                        --legerLines;
                    else if (rowGapPercent > 20)
                        rowGapPercent -= 10;
                    else if (legerLines > 3)
                        --legerLines;
                    else if (rowGapPercent > 0)
                        rowGapPercent -= 10;
                    else { // no, we can't
                        rowGapPercent = 0;
                        legerLines = 8;
                        done = true;
                    }
                }
            }
        }
    }

    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    int topGutter = 0;

    if (m_pageMode == StaffLayout::MultiPageMode) {

        topGutter = 20;

    } else if (m_pageMode == StaffLayout::ContinuousPageMode) {

        // fewer leger lines above staff than in linear mode --
        // compensate for this on the top staff
        topGutter = m_notePixmapFactory->getLineSpacing() * 2;
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        TrackId trackId = m_staffs[i]->getSegment().getTrack();
        Track *track =
            m_staffs[i]->getSegment().getComposition()->
            getTrackById(trackId);

        if (!track)
            continue; // Once Again, My Friend, You Should Never See Me Here

        int trackPosition = track->getPosition();

        m_staffs[i]->setTitleHeight(titleHeight);
        m_staffs[i]->setRowSpacing(accumulatedHeight);

        if (trackPosition < m_maxTrack) {
            m_staffs[i]->setConnectingLineLength(m_trackHeights[trackPosition]);
        }

        if (trackPosition == m_minTrack &&
            m_pageMode != StaffLayout::LinearMode) {
            m_staffs[i]->setBarNumbersEvery(5);
        } else {
            m_staffs[i]->setBarNumbersEvery(0);
        }

        m_staffs[i]->setX(m_leftGutter);
        m_staffs[i]->setY(topGutter + m_trackCoords[trackPosition] + topMargin);
        m_staffs[i]->setPageWidth(pageWidth - leftMargin * 2);
        m_staffs[i]->setRowsPerPage(rowsPerPage);
        m_staffs[i]->setPageMode(m_pageMode);
        m_staffs[i]->setMargin(leftMargin);

        NOTATION_DEBUG << "NotationScene::positionStaffs: set staff's page width to "
                       << (pageWidth - leftMargin * 2);

    }

    // Notation headers must be regenerated
    emit staffsPositionned();
}

void
NotationScene::layoutAll()
{
    //Profiler profiler("NotationScene::layoutAll", true);
    layout(nullptr, 0, 0);
}

void
NotationScene::layout(NotationStaff *singleStaff,
                      timeT startTime, timeT endTime)
{
    //Profiler profiler("NotationScene::layout", true);
    NOTATION_DEBUG << "NotationScene::layout: from " << startTime << " to " << endTime;

    bool full = (singleStaff == nullptr && startTime == endTime);

    m_hlayout->setViewSegmentCount(m_staffs.size());

    if (full) {

        //Profiler profiler("NotationScene::layout: Reset layouts for full scan", true);

        m_hlayout->reset();
        m_vlayout->reset();

        bool first = true;

        for (unsigned int i = 0; i < m_segments.size(); ++i) {

            timeT thisStart = m_segments[i]->getClippedStartTime();
            timeT thisEnd = m_segments[i]->getEndMarkerTime();

            if (first || thisStart < startTime) startTime = thisStart;
            if (first || thisEnd > endTime) endTime = thisEnd;

            first = false;
        }
    }

    NOTATION_DEBUG << "overall start time =" << startTime << ", end time =" << endTime;

    {
        //Profiler profiler("NotationScene::layout: Scan layouts", true);
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff *staff = m_staffs[i];

        if (singleStaff && staff != singleStaff) continue;

        m_hlayout->scanViewSegment(*staff, startTime, endTime, full);
        m_vlayout->scanViewSegment(*staff, startTime, endTime, full);
    }
    }

    m_hlayout->finishLayout(startTime, endTime, full);
    m_vlayout->finishLayout(startTime, endTime, full);

    double maxWidth = 0.0;
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        StaffLayout &staff = *m_staffs[i];
        staff.sizeStaff(*m_hlayout);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }
    }

    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();

    if (m_pageMode == StaffLayout::LinearMode) {
        maxWidth = ((maxWidth / pageWidth) + 1) * pageWidth;
        if (maxHeight < pageHeight) {
            maxHeight = pageHeight;
        }
    } else {
        if (maxWidth < pageWidth) {
            maxWidth = pageWidth;
        }
        if (maxHeight < pageHeight + topMargin*2) {
            maxHeight = pageHeight + topMargin * 2;
        }
    }

    setSceneRect(QRectF(0, 0, maxWidth, maxHeight));

    {
        //Profiler profiler("NotationScene::layout: regeneration", true);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff *staff = m_staffs[i];

        // Secondary is true if this regeneration was caused by edits
        // to another staff, and the content of this staff has not
        // itself changed.

        // N.B. This test is exactly the opposite of the one used in
        // Rosegarden 1.7.x!  I think the prior code was simply wrong
        // and probably the cause of some refresh errors, but it seems
        // risky to "fix" it in a dead-end branch; at least here it
        // will necessarily get some testing.

        bool secondary = (singleStaff && (singleStaff != staff));
        staff->regenerate(startTime, endTime, secondary);
    }
    }

    emit layoutUpdated(startTime,endTime);
}

void
NotationScene::handleEventRemoved(Event *e)
{
    if (m_selection && m_selection->contains(e)) m_selection->removeEvent(e);
    emit eventRemoved(e);
}

void
NotationScene::setSelection(EventSelection *selection,
                            bool preview)
{
    //RG_DEBUG << "setSelection(): " << sselection;

    if (!m_selection && !selection) return;
    if (m_selection == selection) return;
    if (m_selection && selection && *m_selection == *selection) {
        // selections are identical, no need to update elements, but
        // still need to replace the old selection to avoid a leak
        // (can't just delete s in case caller still refers to it)
        EventSelection *oldSelection = m_selection;
        m_selection = selection;
        delete oldSelection;
        return;
    }

    EventSelection *oldSelection = m_selection;
    m_selection = selection;

    NotationStaff *oldStaff = nullptr, *newStaff = nullptr;

    if (oldSelection) {
        oldStaff = setSelectionElementStatus(oldSelection, false);
    }

    if (m_selection) {
        newStaff = setSelectionElementStatus(m_selection, true);
    }

    timeT oldFrom = 0;
    timeT oldTo = 0;
    timeT newFrom = 0;
    timeT newTo = 0;

    if (oldSelection) {
        oldFrom = std::min(oldSelection->getStartTime(),
                           oldSelection->getNotationStartTime());
        oldTo = std::max(oldSelection->getEndTime(),
                         oldSelection->getNotationEndTime());
    }
    if (m_selection) {
        newFrom = std::min(m_selection->getStartTime(),
                           m_selection->getNotationStartTime());
        newTo = std::max(m_selection->getEndTime(),
                         m_selection->getNotationEndTime());
    }

    RG_DEBUG << "From and to times:" << oldFrom << oldTo << newFrom << newTo;

    if (oldSelection && m_selection && oldStaff && newStaff &&
        (oldStaff == newStaff)) {


        // if the regions overlap, render once
        if ((oldFrom <= newFrom && oldTo >= newFrom) ||
            (newFrom <= oldFrom && newTo >= oldFrom)) {
            newStaff->renderElements(std::min(oldFrom, newFrom),
                                     std::max(oldTo, newTo));
        } else {
            newStaff->renderElements(oldFrom, oldTo);
            newStaff->renderElements(newFrom, newTo);
        }
    } else {
        if (oldSelection && oldStaff) {
            oldStaff->renderElements(oldFrom, oldTo);
        }
        if (m_selection && newStaff) {
            newStaff->renderElements(newFrom, newTo);
        }
    }

    if (preview) previewSelection(m_selection, oldSelection);

    delete oldSelection;

    emit selectionChangedES(m_selection);
    emit QGraphicsScene::selectionChanged();
}

void
NotationScene::setSingleSelectedEvent(NotationStaff *staff,
                                      NotationElement *e,
                                      bool preview)
{
    if (!staff || !e) return;
    EventSelection *s = new EventSelection(staff->getSegment());
    s->addEvent(e->event());
    setSelection(s, preview);
}

void
NotationScene::setSingleSelectedEvent(Segment *segment,
                                      Event *e,
                                      bool preview)
{
    if (!segment || !e) return;
    EventSelection *s = new EventSelection(*segment);
    s->addEvent(e);
    setSelection(s, preview);
}

NotationStaff *
NotationScene::setSelectionElementStatus(EventSelection *s, bool set)
{
    if (!s) return nullptr;

    NotationStaff *staff = nullptr;

    // Find the NotationStaff for the EventSelection's Segment.
    for (std::vector<NotationStaff *>::iterator i = m_staffs.begin();
         i != m_staffs.end(); ++i) {

        if (&(*i)->getSegment() == &s->getSegment()) {
            staff = *i;
            break;
        }
    }

    if (!staff) return nullptr;

    for (EventContainer::iterator i = s->getSegmentEvents().begin();
         i != s->getSegmentEvents().end(); ++i) {

        Event *e = *i;

        ViewElementList::iterator staffElementIter = staff->findEvent(e);
        // Not in the view?  Try the next.
        if (staffElementIter == staff->getViewElementList()->end())
            continue;

        NotationElement *element =
                dynamic_cast<NotationElement *>(*staffElementIter);

        if (element)
            element->setSelected(set);
    }

    return staff;
}

void
NotationScene::previewSelection(EventSelection *s,
                                EventSelection *oldSelection)
{
    if (!s) return;
    if (!m_document->isSoundEnabled()) return;

    for (EventContainer::iterator i = s->getSegmentEvents().begin();
         i != s->getSegmentEvents().end(); ++i) {

        Event *e = *i;
        if (oldSelection && oldSelection->contains(e)) continue;

        long pitch;
        if (e->get<Int>(BaseProperties::PITCH, pitch)) {
            long velocity = -1;
            (void)(e->get<Int>(BaseProperties::VELOCITY, velocity));
            if (!(e->has(BaseProperties::TIED_BACKWARD) &&
                  e->get<Bool>(BaseProperties::TIED_BACKWARD))) {
                playNote(s->getSegment(), pitch, velocity);
            }
        }
    }
}

void
NotationScene::showPreviewNote(NotationStaff *staff, double layoutX,
                               int pitch, int height,
                               const Note &note,
                               bool grace,
                               Accidental accidental,
                               bool cautious,
                               QColor color,
                               int velocity,
                               bool play
                              )
{
    if (staff) {
        staff->showPreviewNote(layoutX, height, note, grace,
                               accidental, cautious, color);
        m_previewNoteStaff = staff;

        if (play) playNote(staff->getSegment(), pitch, velocity);
    }
}

void
NotationScene::clearPreviewNote()
{
    if (m_previewNoteStaff) {
        m_previewNoteStaff->clearPreviewNote();
        m_previewNoteStaff = nullptr;
    }
}

void
NotationScene::playNote(const Segment &segment, int pitch, int velocity)
{
    if (!m_document) return;

    Instrument *instrument = m_document->getStudio().getInstrumentFor(&segment);

    StudioControl::playPreviewNote(instrument,
                                   pitch + segment.getTranspose(),
                                   velocity,
                                   RealTime(0, 250000000));
}

/* unused
bool
NotationScene::constrainToSegmentArea(QPointF &scenePos)
{
    bool ok = true;

    NotationStaff *currentStaff = getCurrentStaff();
    if (!currentStaff) return ok;

    QRectF area = currentStaff->getSceneArea();

    double y = scenePos.y();
    if (y < area.top()) {
        scenePos.setY(area.top());
        ok = false;
    } else if (y > area.bottom()) {
        scenePos.setY(area.bottom());
        ok = false;
    }

    double x = scenePos.x();
    if (x < area.left()) {
        scenePos.setX(area.left());
        ok = false;
    } else if (x > area.right()) {
        scenePos.setX(area.right());
        ok = false;
    }

    return ok;
}
*/

bool
NotationScene::isEventRedundant(Event *ev, Segment &seg) const
{
    if (ev->isa(Clef::EventType)) {
        Clef clef = Clef(*ev);
        timeT time = ev->getAbsoluteTime();
        TrackId track = seg.getTrack();
        Clef previousClef = m_clefKeyContext->getClefFromContext(track, time);

// std::cout << "time=" << time << " clef=" << clef.getClefType()
//           << " previous=" << previousClef.getClefType() << "\n";

// m_clefKeyContext->dumpClefContext();

        return clef == previousClef;
    }

    if (ev->isa(Key::EventType)) {
        Key key = Key(*ev);
        timeT time = ev->getAbsoluteTime();
        TrackId track = seg.getTrack();
        Key previousKey = m_clefKeyContext->getKeyFromContext(track, time);

        return key == previousKey;
    }

    return false;
}

bool
NotationScene::isEventRedundant(Clef &clef, timeT time, Segment &seg) const
{
    TrackId track = seg.getTrack();
    Clef previousClef = m_clefKeyContext->getClefFromContext(track, time);

// std::cout << "time=" << time << " clef=" << clef.getClefType()
//           << " previous=" << previousClef.getClefType() << "\n";

// m_clefKeyContext->dumpClefContext();

    return clef == previousClef;
}

bool
NotationScene::isEventRedundant(Key &key, timeT time, Segment &seg) const
{
    TrackId track = seg.getTrack();
    Key previousKey = m_clefKeyContext->getKeyFromContext(track, time);

    return key == previousKey;
}

/* unused
bool
NotationScene::isAnotherStaffNearTime(NotationStaff *currentStaff, timeT t)
{
    int bar = 0;
    Composition *composition = currentStaff->getSegment().getComposition();
    if (composition) bar = composition->getBarNumber(t);

    for (std::vector<NotationStaff *>::iterator i = m_staffs.begin();
         i != m_staffs.end(); ++i) {
        if (*i == currentStaff) continue;

        Segment &s = (*i)->getSegment();
        timeT start = s.getStartTime();
        timeT end = s.getEndMarkerTime();
        if ((start <= t) && (end >= t)) return true;

        if (composition) {
            int staffFirstBar = composition->getBarNumber(start);
            int staffLastBar = composition->getBarNumber(end);
            if ((staffFirstBar <= bar) && (staffLastBar >= bar)) return true;
        }
    }

    return false;
}
*/

void
NotationScene::updateRefreshStatuses(TrackId track, timeT time)
{
    std::vector<Segment *>::iterator it;
    for ( it = m_segments.begin(); it != m_segments.end(); ++it) {
        if ((*it)->getTrack() != track) continue;
        timeT segEndTime = (*it)->getEndMarkerTime();
        if (time < segEndTime) (*it)->updateRefreshStatuses(time, segEndTime);
    }
}

void
NotationScene::updatePageSize()
{
    layout(nullptr, 0, 0);
}

void NotationScene::setHighlightMode(const QString& highlightMode)
{
    RG_DEBUG << "setHighlightMode" << highlightMode;
    // update highlighting
    m_highlightMode = highlightMode;
    setCurrentStaff(getCurrentStaff());
}

///YG: Only for debug
void
NotationScene::dumpBarDataMap()
{
    m_hlayout->dumpBarDataMap();
}

void NotationScene::setExtraPreviewEvents(const EventWithSegmentMap& events)
{
    RG_DEBUG << "setExtraPreviewEvents" << events.size();
    for (auto pair : events) {
        const Event* e = pair.first;
        const Segment* segment = pair.second;
        if (m_additionalPreviewEvents.find(e) !=
            m_additionalPreviewEvents.end()) continue; // already previewed

        long pitch;
        if (e->get<Int>(BaseProperties::PITCH, pitch)) {
            long velocity = -1;
            (void)(e->get<Int>(BaseProperties::VELOCITY, velocity));
            if (!(e->has(BaseProperties::TIED_BACKWARD) &&
                  e->get<Bool>(BaseProperties::TIED_BACKWARD))) {
                playNote(*segment, pitch, velocity);
            }
        }
    }
    m_additionalPreviewEvents = events;
}

#if 0
void
NotationScene::setCurrentStaff(const timeT t)
{
    // ??? This is an attempt to fix Bug #1672.  It is currently not being
    //     used as it introduces new problems.  Need to come up with
    //     a better solution if possible.
    //
    //     See NotationWidget::updatePointer() which has a commented out
    //     call to this.

    NotationStaff *currentStaff = getCurrentStaff();

    // Get the pointer scene coords for t (regardless of staff).
    const double pointerSX = m_hlayout->getXForTime(t);
    // To avoid vertical jumps, use the current staff's Y.
    const double pointerSY = currentStaff->getY();
    // figure out which staff that is in
    // ??? Does this do "nearest"?  Probably need to test some
    //     wild staff arrangements to make sure this works for all.
    NotationStaff *staff =
            getStaffForSceneCoords(pointerSX, pointerSY);
    // If this is a different staff from the current...
    if (staff != currentStaff) {
        // set this as the current staff
        setCurrentStaff(staff);
        // ??? Should we change the selection to indicate new current staff?
        //     That's NotationView::slotEditSelectWholeStaff() that we
        //     usually use.  So the caller will likely need to do that.
    }
}
#endif


}
