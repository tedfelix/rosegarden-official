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

#include "MatrixScene.h"

#include "MatrixMouseEvent.h"
#include "MatrixViewSegment.h"
#include "MatrixWidget.h"
#include "MatrixElement.h"

#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "misc/ConfigGroups.h"

#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "base/SnapGrid.h"

#include "gui/general/GUIPalette.h"
#include "gui/widgets/Panned.h"

#include "base/BaseProperties.h"
#include "base/NotationRules.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QSettings>
#include <QPointF>
#include <QRectF>

namespace Rosegarden
{

using namespace BaseProperties;
	
MatrixScene::MatrixScene() :
    m_widget(0),
    m_document(0),
    m_scale(0),
    m_snapGrid(0),
    m_resolution(8),
    m_selection(0),
    m_currentSegmentIndex(0)
{
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotCommandExecuted()));

    m_pointer = new QGraphicsLineItem;
    m_pointer->setZValue(5);
    m_pointer->setPen(QPen(GUIPalette::getColour(GUIPalette::Pointer), 3));
    m_pointer->setLine(0, 0, 0, height());
    addItem(m_pointer);
    repositionPointer();
}

MatrixScene::~MatrixScene()
{
    if (m_document) {
        if (!isCompositionDeleted()) { // implemented in CompositionObserver
            m_document->getComposition().removeObserver(this);
        }
    }
    for (unsigned int i = 0; i < m_viewSegments.size(); ++i) {
        delete m_viewSegments[i];
    }
    delete m_snapGrid;
    delete m_scale;
}

void
MatrixScene::setMatrixWidget(MatrixWidget *w)
{
    m_widget = w;
}

void
MatrixScene::setSegments(RosegardenDocument *document,
                         std::vector<Segment *> segments)
{
    if (m_document && document != m_document) {
        m_document->getComposition().removeObserver(this);
    }

    m_document = document;
    m_segments = segments;
    
    m_document->getComposition().addObserver(this);

    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    
    SegmentSelection selection;
    selection.insert(segments.begin(), segments.end());
    
    delete m_snapGrid;
    delete m_scale;
    m_scale = new SegmentsRulerScale(&m_document->getComposition(),
                                     selection,
                                     0, 
                                     Note(Note::Shortest).getDuration() / 2.0);
    m_snapGrid = new SnapGrid(m_scale);

    for (unsigned int i = 0; i < m_viewSegments.size(); ++i) {
        delete m_viewSegments[i];
    }
    m_viewSegments.clear();

    //!!! This logic is inherited from the older code, but I'm not
    //!!! sure about it.  The effect is that we only show percussion
    //!!! "diamonds" if there is a key mapping present for the
    //!!! instrument, but it seems to me that surely we should show
    //!!! percussion diamonds whenever we're asked for a percussion
    //!!! matrix window (and simply only show the key names if there
    //!!! is a key mapping).  Consider this

    bool havePercussion = false;

    if (m_widget && m_widget->isDrumMode()) {
        for (unsigned int i = 0; i < m_segments.size(); ++i) {
            Instrument *instrument =
                m_document->getStudio().getInstrumentFor(m_segments[i]);
            if (instrument && instrument->getKeyMapping()) {
                havePercussion = true;
            }
        }
    }
    
    m_resolution = 8;
    if (havePercussion) m_resolution = 11;

    bool haveSetSnap = false;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        int snapGridSize = m_segments[i]->getSnapGridSize();

        if (snapGridSize != -1) { 
            m_snapGrid->setSnapTime(snapGridSize);
            haveSetSnap = true;
        }

        bool isPercussion = false;
        
        if (havePercussion) {
            Instrument *instrument =
                m_document->getStudio().getInstrumentFor(m_segments[i]);
            if (instrument && instrument->getKeyMapping()) {
                isPercussion = true;
            }
        }
            
        MatrixViewSegment *vs = new MatrixViewSegment(this,
                                                      m_segments[i],
                                                      isPercussion);
        (void)vs->getViewElementList(); // make sure it has been created
        m_viewSegments.push_back(vs);
    }

    if (!haveSetSnap) {
        QSettings settings;
        settings.beginGroup(MatrixViewConfigGroup);
        timeT snap = settings.value("Snap Grid Size",
                                    (int)SnapGrid::SnapToBeat).toInt();
        m_snapGrid->setSnapTime(snap);
        settings.endGroup();
        for (unsigned int i = 0; i < m_segments.size(); ++i) {
            m_segments[i]->setSnapGridSize(snap);
        }
    }

    recreateLines();
    updateCurrentSegment();
}

Segment *
MatrixScene::getCurrentSegment()
{
    if (m_segments.empty()) return 0;
    if (m_currentSegmentIndex >= m_segments.size()) {
        m_currentSegmentIndex = int(m_segments.size()) - 1;
    }
    return m_segments[m_currentSegmentIndex];
}

void
MatrixScene::setCurrentSegment(Segment *s)
{
    for (int i = 0; i < int(m_segments.size()); ++i) {
        if (s == m_segments[i]) {
            m_currentSegmentIndex = i;
            return;
        }
    }
    std::cerr << "WARNING: MatrixScene::setCurrentSegment: unknown segment "
              << s << std::endl;
}

bool
MatrixScene::segmentsContainNotes() const
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
MatrixScene::recreateLines()
{
    timeT start = 0, end = 0;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        if (i == 0 || m_segments[i]->getStartTime() < start) {
            start = m_segments[i]->getStartTime();
        }
        if (i == 0 || m_segments[i]->getEndMarkerTime() > end) {
            end = m_segments[i]->getEndMarkerTime();
        }
    }
    
//    double pw = 1.0 / 64.0;
//    double pw = 0.5;
    double pw = 0;

    double x0 = m_scale->getXForTime(start);
    double x1 = m_scale->getXForTime(end);

    int i = 0;

    while (i < 127) {
        int y = (i + 1) * (m_resolution + 1);
        QGraphicsLineItem *line;
        if (i < m_horizontals.size()) {
            line = m_horizontals[i];
        } else {
            line = new QGraphicsLineItem;
            line->setZValue(-9);
            line->setPen(QPen(GUIPalette::getColour
                              (GUIPalette::MatrixHorizontalLine), pw));
            addItem(line);
            m_horizontals.push_back(line);
        }
//        line->setLine(x0 + 0.5, y + 0.5, x1 + 0.5, y + 0.5);
        line->setLine(x0, y, x1, y);
        line->show();
        ++i;
    }
    while (i < m_horizontals.size()) {
        m_horizontals[i]->hide();
        ++i;
    }

    Composition *c = &m_document->getComposition();

    int firstbar = c->getBarNumber(start), lastbar = c->getBarNumber(end);
    i = 0;

    for (int bar = firstbar; bar <= lastbar; ++bar) {

        std::pair<timeT, timeT> range = c->getBarRange(bar);

        bool discard;
        TimeSignature timeSig = c->getTimeSignatureInBar(bar, discard);

        double x0 = m_scale->getXForTime(range.first);
        double x1 = m_scale->getXForTime(range.second);
        double width = x1 - x0;

        double gridLines; // number of grid lines per bar may be fractional

        // If the snap time is zero we default to beat markers
        //
        if (m_snapGrid && m_snapGrid->getSnapTime(x0)) {
            gridLines = double(timeSig.getBarDuration()) /
                        double(m_snapGrid->getSnapTime(x0));
        } else {
            gridLines = timeSig.getBeatsPerBar();
        }

        double dx = width / gridLines;
        double x = x0;

        for (int index = 0; index < gridLines; ++index) {

            // index 0 is the bar line

            QGraphicsLineItem *line;

            if (i < m_verticals.size()) {
                line = m_verticals[i];
            } else {
                line = new QGraphicsLineItem;
                line->setZValue(index > 0 ? -10 : -8);
                addItem(line);
                m_verticals.push_back(line);
            }

            if (index == 0) {
                line->setPen(QPen(GUIPalette::getColour(GUIPalette::MatrixBarLine), pw));
            } else {
                line->setPen(QPen(GUIPalette::getColour(GUIPalette::BeatLine), pw));
            }

            line->setLine(x, 0, x, 128 * (m_resolution + 1));
            line->show();

            x += dx;
            ++i;

            if (bar == lastbar) break; // only the bar line, no grid lines here
        }
    }
    while (i < m_verticals.size()) {
        m_verticals[i]->hide();
        ++i;
    }

    recreatePitchHighlights();

    repositionPointer();
}

void
MatrixScene::repositionPointer()
{
    if (!m_document) return;
    timeT t = m_document->getComposition().getPosition();
    double x = m_scale->getXForTime(t);
//  m_pointer->setLine(x + 0.5, 0.5, x + 0.5, 128 * (m_resolution + 1) + 0.5);
    m_pointer->setLine(0, 0.5, 0, 128 * (m_resolution + 1) + 0.5);
    m_pointer->setPos(x + 0.5, 0);
    if (m_widget && m_widget->getPlayTracking()) ensurePointerVisible();
}

void
MatrixScene::slotPointerPositionChanged(timeT)
{
    repositionPointer();
}

void
MatrixScene::recreatePitchHighlights()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT k0 = segment->getStartTime();
    timeT k1 = segment->getStartTime();
    
    int i = 0;

    while (k0 < segment->getEndMarkerTime()) {
        
        Rosegarden::Key key = segment->getKeyAtTime(k0);
        if (!segment->getNextKeyTime(k0, k1)) k1 = segment->getEndMarkerTime();

        if (k0 == k1) {
            std::cerr << "WARNING: MatrixScene::recreatePitchHighlights: k0 == k1 == " 
                      << k0 << std::endl;
            break;
        }

        double x0 = m_scale->getXForTime(k0);
        double x1 = m_scale->getXForTime(k1);

        // rudimentary

        const int hcount = 3;
        int hsteps[hcount];
        hsteps[0] = scale_Cmajor[0]; // tonic
        hsteps[2] = scale_Cmajor[4]; // fifth
        
        if (key.isMinor()) {
            hsteps[1] = scale_Cminor[2]; // minor third
        } else {
            hsteps[1] = scale_Cmajor[2]; // major third
        }

        for (int j = 0; j < hcount; ++j) {

            int pitch = hsteps[j];
            while (pitch < 128) {

                QGraphicsRectItem *rect;

                if (i < m_highlights.size()) {
                    rect = m_highlights[i];
                } else {
                    rect = new QGraphicsRectItem;
                    rect->setZValue(-11);
                    rect->setPen(Qt::NoPen);
                    addItem(rect);
                    m_highlights.push_back(rect);
                }
                
                if (j == 0) {
                    rect->setBrush(GUIPalette::getColour
                                   (GUIPalette::MatrixTonicHighlight));
                } else {
                    rect->setBrush(GUIPalette::getColour
                                   (GUIPalette::MatrixPitchHighlight));
                }

//                rect->setRect(0.5, 0.5, x1 - x0, m_resolution + 1);
                rect->setRect(0, 0, x1 - x0, m_resolution + 1);
                rect->setPos(x0, (127 - pitch) * (m_resolution + 1));
                rect->show();

                pitch += 12;

                ++i;
            }
        }

        k0 = k1;
    }
    while (i < m_highlights.size()) {
        m_highlights[i]->hide();
        ++i;
    }
}

void
MatrixScene::setupMouseEvent(QGraphicsSceneMouseEvent *e,
                             MatrixMouseEvent &mme) const
{
    double sx = e->scenePos().x();
    int sy = lrint(e->scenePos().y());

    mme.viewpos = e->screenPos();

    mme.sceneX = sx;
    mme.sceneY = sy;

    mme.modifiers = e->modifiers();
    mme.buttons = e->buttons();

    mme.element = 0;

    QList<QGraphicsItem *> l = items(e->scenePos());
    MATRIX_DEBUG << "Found " << l.size() << " items at " << e->scenePos() << endl;
    for (int i = 0; i < l.size(); ++i) {
        MatrixElement *element = MatrixElement::getMatrixElement(l[i]);
        if (element) {
            // items are in z-order from top, so this is most salient
            mme.element = element;
            break;
        }
    }

    mme.viewSegment = m_viewSegments[m_currentSegmentIndex];

    mme.time = m_scale->getTimeForX(sx);

    if (e->modifiers() & Qt::ShiftModifier) {
        mme.snappedLeftTime = mme.time;
        mme.snappedRightTime = mme.time;
        mme.snapUnit = Note(Note::Shortest).getDuration();
    } else {
        mme.snappedLeftTime = m_snapGrid->snapX(sx, SnapGrid::SnapLeft);
        mme.snappedRightTime = m_snapGrid->snapX(sx, SnapGrid::SnapRight);
        mme.snapUnit = m_snapGrid->getSnapTime(sx);
    }

    if (mme.viewSegment) {
        timeT start = mme.viewSegment->getSegment().getStartTime();
        timeT end = mme.viewSegment->getSegment().getEndMarkerTime();
        if (mme.snappedLeftTime < start) mme.snappedLeftTime = start;
        if (mme.snappedLeftTime + mme.snapUnit > end) {
            mme.snappedLeftTime = end - mme.snapUnit;
        }
        if (mme.snappedRightTime < start) mme.snappedRightTime = start;
        if (mme.snappedRightTime > end) mme.snappedRightTime = end;
    }

    mme.pitch = 127 - (sy / (m_resolution + 1)); //!!! function for this, to be used by MatrixElement as well
    if (mme.pitch < 0) mme.pitch = 0;
    if (mme.pitch > 127) mme.pitch = 127;

    MATRIX_DEBUG << "MatrixScene::setupMouseEvent: sx = " << sx
                 << ", sy = " << sy
                 << ", modifiers = " << mme.modifiers
                 << ", buttons = " << mme.buttons
                 << ", element = " << mme.element
                 << ", viewSegment = " << mme.viewSegment
                 << ", time = " << mme.time
                 << ", pitch = " << mme.pitch
                 << endl;
}
   
void
MatrixScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    MatrixMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mousePressed(&nme);
}

void
MatrixScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    MatrixMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseMoved(&nme);
}

void
MatrixScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    MatrixMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseReleased(&nme);
}

void
MatrixScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    MatrixMouseEvent nme;
    setupMouseEvent(e, nme);
    emit mouseDoubleClicked(&nme);
}

void
MatrixScene::ensurePointerVisible()
{
    if (m_pointer) {

// This doesn't work because view->ensureVisible() moves the view
// a few pixels vertically each time it is called and I didn't find
// any way to avoid it.
//
//         double x = m_pointer->x();
//         Panned *view = m_widget->getView();
//         int w = view->width();
//         int h = view->height();
// 
//         // Horizontal position (%) of playback pointer 
//         // inside the view when play tracking is on.
//         const int percentPos = 50; 
// 
//         // Construct a rect around the pointer. The whole rect will be
//         // kept inside the view.
//         int wm = w * (100 - percentPos) / 100;
//         int eps = h / 10;
//         QRectF r = view->mapToScene(0, eps, wm, h - eps).boundingRect();
//         r.moveLeft(x);
// 
//         view->ensureVisible(r, 10, 0);

// Replaced with a rewrite of ensureVisible
        m_widget->ensureXVisible(m_pointer->x());
    }
}

void
MatrixScene::slotCommandExecuted()
{
}

void
MatrixScene::segmentEndMarkerTimeChanged(const Segment *, bool)
{
    MATRIX_DEBUG << "MatrixScene::segmentEndMarkerTimeChanged" << endl;
    recreateLines();
}

void
MatrixScene::timeSignatureChanged(const Composition *c)
{
    if (!m_document || !c || (c != &m_document->getComposition())) return;
}

void
MatrixScene::segmentRemoved(const Composition *c, Segment *s)
{
    MATRIX_DEBUG << "MatrixScene::segmentRemoved(" << c << "," << s << ")" << endl;

    if (!m_document || !c || (c != &m_document->getComposition())) return;

    for (std::vector<MatrixViewSegment *>::iterator i = m_viewSegments.begin();
         i != m_viewSegments.end(); ++i) {
        if (s == &(*i)->getSegment()) {
            delete *i;
            m_viewSegments.erase(i);
            break;
        }
    }

    if (m_viewSegments.empty()) {
        MATRIX_DEBUG << "(Scene is now empty)" << endl;
        emit sceneDeleted();
    }
}

void
MatrixScene::handleEventAdded(Event *e)
{
    if (e->getType() == Rosegarden::Key::EventType) {
        recreatePitchHighlights();
    }
}

void
MatrixScene::handleEventRemoved(Event *e)
{
    if (m_selection && m_selection->contains(e)) m_selection->removeEvent(e);
    if (e->getType() == Rosegarden::Key::EventType) {
        recreatePitchHighlights();
    }
    emit eventRemoved(e);
}

void
MatrixScene::setSelection(EventSelection *s,
                          bool preview)
{
    if (m_selection) {
        setSelectionElementStatus(m_selection, false);
    }

    if (s != m_selection) {
        delete m_selection;
        m_selection = s;
    }
    
    if (m_selection) {
        setSelectionElementStatus(m_selection, true, preview);
    }
}

void
MatrixScene::setSingleSelectedEvent(MatrixViewSegment *vs,
                                    MatrixElement *e,
                                    bool preview)
{
    if (!vs || !e) return;
    EventSelection *s = new EventSelection(vs->getSegment());
    s->addEvent(e->event());
    setSelection(s, preview);
}

void
MatrixScene::setSingleSelectedEvent(Segment *seg,
                                    Event *e,
                                    bool preview)
{
    if (!seg || !e) return;
    EventSelection *s = new EventSelection(*seg);
    s->addEvent(e);
    setSelection(s, preview);
}

void
MatrixScene::selectAll()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;
    Segment::iterator it = segment->begin();
    EventSelection *selection = new EventSelection(*segment);

    for (; segment->isBeforeEndMarker(it); it++) {
        if ((*it)->isa(Note::EventType)) {
            selection->addEvent(*it);
        }
    }

    setSelection(selection, false);
}

void
MatrixScene::setSelectionElementStatus(EventSelection *s, 
                                       bool set,
                                       bool preview)
{
    if (!s) return;

    MatrixViewSegment *vs = 0;

    for (std::vector<MatrixViewSegment *>::iterator i = m_viewSegments.begin();
         i != m_viewSegments.end(); ++i) {

        if (&(*i)->getSegment() == &s->getSegment()) {
            vs = *i;
            break;
        }
    }

    if (!vs) return;

    for (EventSelection::eventcontainer::iterator i = s->getSegmentEvents().begin();
         i != s->getSegmentEvents().end(); ++i) {

        Event *e = *i;
        
        ViewElementList::iterator vsi = vs->findEvent(e);
        if (vsi == vs->getViewElementList()->end()) continue;
        
        MatrixElement *el = static_cast<MatrixElement *>(*vsi);
        
        el->setSelected(set);

        if (set && preview) {
            long pitch;
            if (e->get<Int>(PITCH, pitch)) {
                long velocity = -1;
                (void)(e->get<Int>(VELOCITY, velocity));
                if (!(e->has(TIED_BACKWARD) && e->get<Bool>(TIED_BACKWARD))) {
                    playNote(s->getSegment(), pitch, velocity);
                }
            }
        }
    }
}

void
MatrixScene::updateCurrentSegment()
{
    for (int i = 0; i < m_viewSegments.size(); ++i) {
        bool current = (i == m_currentSegmentIndex);
        ViewElementList *vel = m_viewSegments[i]->getViewElementList();
        for (ViewElementList::const_iterator j = vel->begin();
             j != vel->end(); ++j) {
            MatrixElement *mel = dynamic_cast<MatrixElement *>(*j);
            if (!mel) continue;
            mel->setCurrent(current);
        }
    }

    // changing the current segment may have overridden selection border colours
    setSelectionElementStatus(m_selection, true, false);
}

static bool
canPreviewAnotherNote() //!!! dupe with NotationScene
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second or so, to
            // avoid gungeing up the sound drivers
            return false;
        }
    }

    return true;
}

void
MatrixScene::setSnap(timeT t)
{
    MATRIX_DEBUG << "MatrixScene::slotSetSnap: time is " << t << endl;
    m_snapGrid->setSnapTime(t);

    for (size_t i = 0; i < m_segments.size(); ++i) {
        m_segments[i]->setSnapGridSize(t);
    }

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    settings.setValue("Snap Grid Size", (int)t);
    settings.endGroup();

    recreateLines();
}

bool
MatrixScene::constrainToSegmentArea(QPointF &scenePos)
{
    bool ok = true;

    int pitch = 127 - (lrint(scenePos.y()) / (m_resolution + 1));
    if (pitch < 0) {
        ok = false;
        scenePos.setY(127 * (m_resolution + 1));
    } else if (pitch > 127) {
        ok = false;
        scenePos.setY(0);
    }

    timeT t = m_scale->getTimeForX(scenePos.x());
    timeT start = 0, end = 0;
    for (size_t i = 0; i < m_segments.size(); ++i) {
        timeT t0 = m_segments[i]->getStartTime();
        timeT t1 = m_segments[i]->getEndMarkerTime();
        if (i == 0 || t0 < start) start = t0;
        if (i == 0 || t1 > end) end = t1; 
    }
    if (t < start) {
        ok = false;
        scenePos.setX(m_scale->getXForTime(start));
    } else if (t > end) {
        ok = false;
        scenePos.setX(m_scale->getXForTime(end));
    }

    return ok;
}

void
MatrixScene::playNote(Segment &segment, int pitch, int velocity)
{
    if (!m_document) return;

    Instrument *instrument = m_document->getStudio().getInstrumentFor(&segment);
    if (!instrument) return;

    if (!canPreviewAnotherNote()) return;

    if (velocity < 0) velocity = 100;

    MappedEvent mE(instrument->getId(),
                   MappedEvent::MidiNoteOneShot,
                   pitch + segment.getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}    
    
}

#include "MatrixScene.moc"

