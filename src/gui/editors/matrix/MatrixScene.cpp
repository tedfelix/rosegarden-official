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

#define RG_MODULE_STRING "[MatrixScene]"
#define RG_NO_DEBUG_PRINT 1

#include "MatrixScene.h"

#include "MatrixMouseEvent.h"
#include "MatrixViewSegment.h"
#include "MatrixWidget.h"
#include "MatrixElement.h"

#include "gui/application/RosegardenMainWindow.h"
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

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QSettings>
#include <QPointF>
#include <QRectF>

#include <algorithm>  // for std::sort

//#define DEBUG_MOUSE

namespace Rosegarden
{


using namespace BaseProperties;

MatrixScene::MatrixScene() :
    m_widget(nullptr),
    m_document(nullptr),
    m_currentSegmentIndex(0),
    m_scale(nullptr),
    m_referenceScale(nullptr),
    m_snapGrid(nullptr),
    m_resolution(8),
    m_selection(nullptr),
    m_highlightType(HT_BlackKeys)
{
    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &MatrixScene::slotCommandExecuted);
}

MatrixScene::~MatrixScene()
{
    RG_DEBUG << "MatrixScene::~MatrixScene() - start";

    if (m_document) {
        if (!isCompositionDeleted()) { // implemented in CompositionObserver
            m_document->getComposition().removeObserver(this);
        }
    }
    for (unsigned int i = 0; i < m_viewSegments.size(); ++i) {
        delete m_viewSegments[i];
    }
    delete m_snapGrid;
    delete m_referenceScale;
    delete m_scale;
    delete m_selection;

    RG_DEBUG << "MatrixScene::~MatrixScene() - end";
}

namespace
{
    // Functor for std::sort that compares the track positions of two Segments.
    struct TrackPositionLess {
        TrackPositionLess() :
            m_composition(RosegardenDocument::currentDocument->
                              getComposition())
        {
        }

        bool operator()(const Segment *lhs, const Segment *rhs) const
        {
            // ??? Could also sort by Segment name and Segment start time.
            const int lPos =
                    m_composition.getTrackById(lhs->getTrack())->getPosition();
            const int rPos =
                    m_composition.getTrackById(rhs->getTrack())->getPosition();
            return (lPos < rPos);
        }

    private:
        const Composition &m_composition;
    };
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

    // Sort the Segments into TrackPosition order.  This makes the
    // Segment changer wheel in the Matrix editor
    // (MatrixWidget::m_segmentChanger) easier to understand.
    std::sort(m_segments.begin(), m_segments.end(), TrackPositionLess());

    m_document->getComposition().addObserver(this);

    SegmentSelection selection;
    selection.insert(segments.begin(), segments.end());

    delete m_snapGrid;
    delete m_scale;
    delete m_referenceScale;
    m_scale = new SegmentsRulerScale(&m_document->getComposition(),
                                     selection,
                                     0,
                                     Note(Note::Shortest).getDuration() / 2.0);

    m_referenceScale = new ZoomableRulerScale(m_scale);
    m_snapGrid = new SnapGrid(m_referenceScale);

    for (unsigned int i = 0; i < m_viewSegments.size(); ++i) {
        delete m_viewSegments[i];
    }
    m_viewSegments.clear();

    // We should show diamonds instead of bars whenever we are in
    // "drum mode" (i.e. whenever we were invoked using the percussion
    // matrix menu option instead of the normal matrix one).

    // The question of whether to show the key names instead of the
    // piano keyboard is a separate one, handled in MatrixWidget, and
    // it depends solely on whether a key mapping exists for the
    // instrument (it is independent of whether this is a percussion
    // matrix or not).

    // Nevertheless, if the key names are shown, we need a little more space
    // between horizontal lines. That's why m_resolution depends from
    // keyMapping.

    // But since several segments may be edited in the same matrix, we
    // have to deal with simultaneous display of segments using piano keyboard
    // and segments using key mapping.
    // Key mapping may be displayed with piano keyboard resolution (even if
    // space is a bit short for the text) but the opposite is not possible.
    // So the only (easy) way I found is to use the resolution fitting with
    // piano keyboard when at least one segment needs it.

    bool drumMode = false;
    bool keyMapping = false;
    if (m_widget) {
        drumMode = m_widget->isDrumMode();
        keyMapping = m_widget->hasOnlyKeyMapping();
    }
    m_resolution = 8;
    if (keyMapping) m_resolution = 11;

    bool haveSetSnap = false;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        int snapGridSize = m_segments[i]->getSnapGridSize();

        if (snapGridSize != -1) {
            m_snapGrid->setSnapTime(snapGridSize);
            haveSetSnap = true;
        }

        MatrixViewSegment *vs = new MatrixViewSegment(this,
                                                      m_segments[i],
                                                      drumMode);
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
    if (m_segments.empty()) return nullptr;
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
            recreatePitchHighlights();
            updateCurrentSegment();

            // ??? All callers call m_widget->updateSegmentChangerBackground()
            //     next.  We should just call it here.
            //if (m_widget)
            //    m_widget->updateSegmentChangerBackground();

            return;
        }
    }
    RG_WARNING << "WARNING: MatrixScene::setCurrentSegment: unknown segment" << s;
}

Segment *
MatrixScene::getPriorSegment()
{
    if (m_currentSegmentIndex == 0) return nullptr;
    return m_segments[m_currentSegmentIndex-1];
}

Segment *
MatrixScene::getNextSegment()
{
    if ((unsigned int) m_currentSegmentIndex + 1 >= m_segments.size()) return nullptr;
    return m_segments[m_currentSegmentIndex+1];
}

MatrixViewSegment *
MatrixScene::getCurrentViewSegment()
{
    if (m_viewSegments.empty())
        return nullptr;

    // ??? Why doesn't this use m_currentSegmentIndex?
    // return m_viewSegments[0];

    // It should. Otherwise these callers work incorrectly
    //      MatrixView::slotExtendSelectionBackward(bool)
    //      MatrixView::slotExtendSelectionForward(bool)
    //      MatrixWidget::slotKeyPressed(unsigned, bool)
    //      MatrixWidget::slotKeySelected(unsigned, bool)
    //      MatrixWidget::slotKeyReleased(unsigned, bool)
    return m_viewSegments[m_currentSegmentIndex];
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

    // Determine total distance that requires lines (considering multiple segments
    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        if (i == 0 || m_segments[i]->getClippedStartTime() < start) {
            start = m_segments[i]->getClippedStartTime();
        }
        if (i == 0 || m_segments[i]->getEndMarkerTime() > end) {
            end = m_segments[i]->getEndMarkerTime();
        }
    }

    // Pen Width?
    double pw = 0;

    double startPos = m_scale->getXForTime(start);
    double endPos = m_scale->getXForTime(end);

    // Draw horizontal lines
    int i = 0;
    while (i < 127) {
         int y = (i + 1) * (m_resolution + 1);
         QGraphicsLineItem *line;
         if (i < (int)m_horizontals.size()) {
             line = m_horizontals[i];
         } else {
             line = new QGraphicsLineItem;
             line->setZValue(MatrixElement::HORIZONTAL_LINE_Z);
             line->setPen(QPen(GUIPalette::getColour
                               (GUIPalette::MatrixHorizontalLine), pw));
             addItem(line);
             m_horizontals.push_back(line);
         }
         line->setLine(startPos, y, endPos, y);
         line->show();
         ++i;
     }


     // Hide the other lines, if there are any.  Just a double check.
     while (i < (int)m_horizontals.size()) {
         m_horizontals[i]->hide();
         ++i;
    }

    setSceneRect(QRectF(startPos, 0, endPos - startPos, 128 * (m_resolution + 1)));

    Composition *c = &m_document->getComposition();

    int firstbar = c->getBarNumber(start), lastbar = c->getBarNumber(end);

    // Draw Vertical Lines
    i = 0;
    for (int bar = firstbar; bar <= lastbar; ++bar) {

        std::pair<timeT, timeT> range = c->getBarRange(bar);

        bool discard = false;  // was not initalied...hmmm...try false?
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

        double beatLines = timeSig.getBeatsPerBar();
        double dxbeats = width / beatLines;

        double dx = width / gridLines;
        double x = x0;

        for (int index = 0; index < gridLines; ++index) {

            // Check to see if we are withing the first segments start time.
            if (x < startPos) {
                x += dx;
                continue;
            }

            // Exit if we have passed the end of last segment end time.
            if (x > endPos) {
                break;
            }

            QGraphicsLineItem *line;

            if (i < (int)m_verticals.size()) {
                line = m_verticals[i];
            } else {
                line = new QGraphicsLineItem;
                addItem(line);
                m_verticals.push_back(line);
            }

            if (index == 0) {
              // index 0 is the bar line
                line->setPen(QPen(GUIPalette::getColour(GUIPalette::MatrixBarLine), pw));
            } else {
                // check if we are on a a beat
                double br = x / dxbeats;
                int ibr = br + 0.5;
                double delta = br - ibr;
                if (fabs(delta) > 1.0e-6) {
                    line->setPen(QPen(GUIPalette::getColour(GUIPalette::SubBeatLine), pw));
                } else {
                    line->setPen(QPen(GUIPalette::getColour(GUIPalette::BeatLine), pw));
                }
            }

            line->setZValue(index > 0 ? MatrixElement::VERTICAL_BEAT_LINE_Z
                                      : MatrixElement::VERTICAL_BAR_LINE_Z);
            line->setLine(x, 0, x, 128 * (m_resolution + 1));

            line->show();
            x += dx;
            ++i;
        }
    }

    // Hide the other lines. We are not using them right now.
    while (i < (int)m_verticals.size()) {
        m_verticals[i]->hide();
        ++i;
    }

    recreatePitchHighlights();

    // Force update so all vertical lines are drawn correctly.
    // ??? Works fine without.  Seems like update() isn't needed in this
    //     class.  In fact it might be harmful.
    //update();
}

void
MatrixScene::recreateTriadHighlights()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT k0 = segment->getClippedStartTime();
    timeT k1 = segment->getClippedStartTime();

    int i = 0;

    while (k0 < segment->getEndMarkerTime()) {

        Rosegarden::Key key = segment->getKeyAtTime(k0);

        // offset the highlights according to how far this key's tonic pitch is
        // from C major (0)
        int offset = key.getTonicPitch();

        // correct for segment transposition, moving representation the opposite
        // of pitch to cancel it out (C (MIDI pitch 0) in Bb (-2) is concert
        // Bb (10), so 0 -1 == 11 -1 == 10, we have to go +1 == 11 +1 == 0)
        int correction = segment->getTranspose(); // eg. -2
        correction *= -1;                         // eg.  2

        // key is Bb for Bb instrument, getTonicPitch() returned 10, correction
        // is +2
        offset -= correction;
        offset += 12;
        offset %= 12;

        if (!segment->getNextKeyTime(k0, k1)) k1 = segment->getEndMarkerTime();

        if (k0 == k1) {
            RG_WARNING << "WARNING: MatrixScene::recreatePitchHighlights: k0 == k1 ==" << k0;
            break;
        }

        double x0 = m_scale->getXForTime(k0);
        double x1 = m_scale->getXForTime(k1);

        // calculate the highlights relative to C major, plus offset
        // (I think this enough to do the job.  It passes casual tests.)
        const int hcount = 3;
        int hsteps[hcount];
        hsteps[0] = scale_Cmajor[0] + offset; // tonic
        hsteps[2] = scale_Cmajor[4] + offset; // fifth

        if (key.isMinor()) {
            hsteps[1] = scale_Cminor[2] + offset; // minor third
        } else {
            hsteps[1] = scale_Cmajor[2] + offset; // major third
        }

        for (int j = 0; j < hcount; ++j) {

            int pitch = hsteps[j];
            while (pitch < 128) {

                QGraphicsRectItem *rect;

                if (i < (int)m_highlights.size()) {
                    rect = m_highlights[i];
                } else {
                    rect = new QGraphicsRectItem;
                    rect->setZValue(MatrixElement::HIGHLIGHT_Z);
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
    while (i < (int)m_highlights.size()) {
        m_highlights[i]->hide();
        ++i;
    }
}

void
MatrixScene::recreateBlackkeyHighlights()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT k0 = segment->getClippedStartTime();
    timeT k1 = segment->getEndMarkerTime();

    int i = 0;

    double x0 = m_scale->getXForTime(k0);
    double x1 = m_scale->getXForTime(k1);

    int bkcount = 5;
    int bksteps[bkcount];
    bksteps[0] = 1;
    bksteps[1] = 3;
    bksteps[2] = 6;
    bksteps[3] = 8;
    bksteps[4] = 10;
    for (int j = 0; j < bkcount; ++j) {

        int pitch = bksteps[j];
        while (pitch < 128) {

            QGraphicsRectItem *rect;

            if (i < (int)m_highlights.size()) {
                rect = m_highlights[i];
            } else {
                rect = new QGraphicsRectItem;
                rect->setZValue(MatrixElement::HIGHLIGHT_Z);
                rect->setPen(Qt::NoPen);
                addItem(rect);
                m_highlights.push_back(rect);
            }

            rect->setBrush(GUIPalette::getColour
                           (GUIPalette::MatrixPitchHighlight));

            rect->setRect(0, 0, x1 - x0, m_resolution + 1);
            rect->setPos(x0, (127 - pitch) * (m_resolution + 1));
            rect->show();

            pitch += 12;

            ++i;
        }
    }

    while (i < (int)m_highlights.size()) {
        m_highlights[i]->hide();
        ++i;
    }
}

void
MatrixScene::recreatePitchHighlights()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    HighlightType chosenHighlightType = static_cast<HighlightType>(
        settings.value("highlight_type", HT_BlackKeys).toInt());
    settings.endGroup();

    if (chosenHighlightType == HT_BlackKeys) {
        RG_DEBUG << "highlight the black notes";
        // highlight the black notes
        if (m_highlightType != chosenHighlightType) {
            // hide all highlights from triad highlight
            RG_DEBUG << "hide all old triad highlights";
            int i = 0;
            while (i < (int)m_highlights.size()) {
                m_highlights[i]->hide();
                ++i;
            }
        }
        m_highlightType = HT_BlackKeys;
        recreateBlackkeyHighlights();
        return;
    }

    // Not highlighting black notes so highlight the major/minor triad

    RG_DEBUG << "highlight key triad";
    if (m_highlightType != HT_Triads) {
        // hide all highlights of black notes
        RG_DEBUG << "hide all old blacknote highlights";
        int i = 0;
        while (i < (int)m_highlights.size()) {
            m_highlights[i]->hide();
            ++i;
        }
        m_highlightType = HT_BlackKeys;
    }

    recreateTriadHighlights();

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

    mme.element = nullptr;

    QList<QGraphicsItem *> l = items(e->scenePos());
//   MATRIX_DEBUG << "Found " << l.size() << " items at " << e->scenePos();
    for (int i = 0; i < l.size(); ++i) {
        MatrixElement *element = MatrixElement::getMatrixElement(l[i]);
        if (element && ! element->isPreview()) {
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
//        mme.snappedLeftTime = m_snapGrid->snapX(sx, SnapGrid::SnapLeft);
//        mme.snappedRightTime = m_snapGrid->snapX(sx, SnapGrid::SnapRight);
        mme.snappedLeftTime = m_snapGrid->snapTime(mme.time, SnapGrid::SnapLeft);
        mme.snappedRightTime = m_snapGrid->snapTime(mme.time, SnapGrid::SnapRight);
        mme.snapUnit = m_snapGrid->getSnapTime(sx);
    }

    if (mme.viewSegment) {
        timeT start = mme.viewSegment->getSegment().getClippedStartTime();
        timeT end = mme.viewSegment->getSegment().getEndMarkerTime();
        if (mme.snappedLeftTime < start) mme.snappedLeftTime = start;
        if (mme.snappedLeftTime + mme.snapUnit > end) {
            mme.snappedLeftTime = end - mme.snapUnit;
        }
        if (mme.snappedRightTime < start) mme.snappedRightTime = start;
        if (mme.snappedRightTime > end) mme.snappedRightTime = end;
    }

   mme.pitch = calculatePitchFromY(sy);

#ifdef DEBUG_MOUSE
    MATRIX_DEBUG << "MatrixScene::setupMouseEvent: sx = " << sx
                 << ", sy = " << sy
                 << ", modifiers = " << mme.modifiers
                 << ", buttons = " << mme.buttons
                 << ", element = " << mme.element
                 << ", viewSegment = " << mme.viewSegment
                 << ", time = " << mme.time
                 << ", pitch = " << mme.pitch
                 << endl;
#endif
}

int MatrixScene::calculatePitchFromY(int y) const {
    int pitch = 127 - (y / (m_resolution + 1));
    if (pitch < 0) pitch = 0;
    if (pitch > 127) pitch = 127;
    return pitch;
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
MatrixScene::slotCommandExecuted()
{
    checkUpdate();
}

void
MatrixScene::checkUpdate()
{
    bool updateSelectionElementStatus = false;

    for (unsigned int i = 0; i < m_viewSegments.size(); ++i) {

        SegmentRefreshStatus &rs = m_viewSegments[i]->getRefreshStatus();

        if (rs.needsRefresh()) {
            // Refresh the required range.
            // Note that updateElements() does not handle deleted
            // ViewElements.  See MatrixViewSegment::eventRemoved().
            m_viewSegments[i]->updateElements(rs.from(), rs.to());

            if (!updateSelectionElementStatus && m_selection) {
                updateSelectionElementStatus =
                    (m_viewSegments[i]->getSegment() == m_selection->getSegment());
            }
        }

        rs.setNeedsRefresh(false);
    }

    if (updateSelectionElementStatus) {
        setSelectionElementStatus(m_selection, true);
    }
}

void
MatrixScene::segmentEndMarkerTimeChanged(const Segment *, bool)
{
    MATRIX_DEBUG << "MatrixScene::segmentEndMarkerTimeChanged";
    recreateLines();
}

void
MatrixScene::segmentRemoved(const Composition *, Segment *removedSegment)
{
    if (!removedSegment)
        return;
    if (m_segments.empty())
        return;

    const int removedSegmentIndex = findSegmentIndex(removedSegment);

    // Not found?  Bail.
    if (removedSegmentIndex == -1)
        return;

    // If we're about to remove the one they are looking at and
    // there is another to switch to...
    if (removedSegmentIndex == static_cast<int>(m_currentSegmentIndex) &&
        m_segments.size() > 1) {

        // Switch to another Segment.

        // Try the next.
        size_t newSegmentIndex = m_currentSegmentIndex + 1;
        // No next, go with the previous.
        if (newSegmentIndex == m_segments.size())
            newSegmentIndex = m_currentSegmentIndex - 1;

        setCurrentSegment(m_segments[newSegmentIndex]);
        if (m_widget)
            m_widget->updateSegmentChangerBackground();

    }

    emit segmentDeleted(removedSegment);

    delete m_viewSegments[removedSegmentIndex];
    m_viewSegments.erase(m_viewSegments.cbegin() + removedSegmentIndex);

    m_segments.erase(m_segments.cbegin() + removedSegmentIndex);

    // Adjust m_currentSegmentIndex
    if (static_cast<int>(m_currentSegmentIndex) > removedSegmentIndex)
        --m_currentSegmentIndex;

    // No more Segments?
    if (m_segments.empty())
        emit sceneDeleted();
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
    if (m_selection && m_selection->contains(e))
        m_selection->removeEvent(e);

    // we can not use e here (already deleted) but if it was a
    // Rosegarden::Key::EventType we must recreatePitchHighlights

    recreatePitchHighlights();

    // ??? Oddly, this causes refresh failures that leave deleted notes
    //     up on the display (only when doing toolbar undo!?).  Removing
    //     it seems to result in solid and correct updates in all cases.
    //     Why?!  See discussion on mailing list early June 2022.
    //update();

    // Notify MatrixToolBox.
    emit eventRemoved(e);
}

void
MatrixScene::setSelection(EventSelection *s, bool preview)
{
    if (!m_selection && !s) return;
    if (m_selection == s) return;
    if (m_selection && s && *m_selection == *s) {
        // selections are identical, no need to update elements, but
        // still need to replace the old selection to avoid a leak
        // (can't just delete s in case caller still refers to it)
        EventSelection *oldSelection = m_selection;
        m_selection = s;
        delete oldSelection;
        return;
    }

    EventSelection *oldSelection = m_selection;
    m_selection = s;

    if (oldSelection) {
        setSelectionElementStatus(oldSelection, false);
    }

    if (m_selection) {
        setSelectionElementStatus(m_selection, true);
        // ??? But we are going to do this at the end of this routine.
        //     Is this needed?  Notation only does this at the end.
        emit QGraphicsScene::selectionChanged();
        emit selectionChangedES(m_selection);
    }

    if (preview) previewSelection(m_selection, oldSelection);
    delete oldSelection;
    emit QGraphicsScene::selectionChanged();
    emit selectionChangedES(m_selection);
}

void
MatrixScene::setSingleSelectedEvent(MatrixViewSegment *viewSegment,
                                    MatrixElement *e,
                                    bool preview)
{
    if (!viewSegment || !e) return;
    EventSelection *s = new EventSelection(viewSegment->getSegment());
    s->addEvent(e->event());
    setSelection(s, preview);
}

void
MatrixScene::setSingleSelectedEvent(Segment *segment,
                                    Event *e,
                                    bool preview)
{
    if (!segment || !e) return;
    EventSelection *s = new EventSelection(*segment);
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

    for (; segment->isBeforeEndMarker(it); ++it) {
        if ((*it)->isa(Note::EventType)) {
            selection->addEvent(*it);
        }
    }

    setSelection(selection, false);
}

void
MatrixScene::setSelectionElementStatus(EventSelection *s, bool set)
{
    if (!s) return;

    MatrixViewSegment *vs = nullptr;

    // Find the MatrixViewSegment for the EventSelection's Segment.
    for (MatrixViewSegment *currentViewSegment : m_viewSegments) {

        if (&currentViewSegment->getSegment() == &s->getSegment()) {
            vs = currentViewSegment;
            break;
        }

    }

    if (!vs) return;

    for (const Event *e : s->getSegmentEvents()) {

        ViewElementList::iterator viewElementIter = vs->findEvent(e);
        // Not in the view?  Try the next.
        if (viewElementIter == vs->getViewElementList()->end())
            continue;

        MatrixElement *element =
                dynamic_cast<MatrixElement *>(*viewElementIter);
        if (element)
            element->setSelected(set);

    }
}

void
MatrixScene::previewSelection(EventSelection *s,
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
MatrixScene::updateCurrentSegment()
{
    MATRIX_DEBUG << "MatrixScene::updateCurrentSegment: current is " << m_currentSegmentIndex;
    for (unsigned i = 0; i < m_viewSegments.size(); ++i) {
        bool current = (i == m_currentSegmentIndex);
        ViewElementList *vel = m_viewSegments[i]->getViewElementList();
        for (ViewElementList::const_iterator j = vel->begin();
             j != vel->end(); ++j) {
            MatrixElement *mel = dynamic_cast<MatrixElement *>(*j);
            if (!mel) continue;
            mel->setCurrent(current);
        }
        if (current) emit currentViewSegmentChanged(m_viewSegments[i]);
    }

    // changing the current segment may have overridden selection border colours
    setSelectionElementStatus(m_selection, true);
}

void
MatrixScene::setSnap(timeT t)
{
    MATRIX_DEBUG << "MatrixScene::slotSetSnap: time is " << t;
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

/* unused
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
        timeT t0 = m_segments[i]->getClippedStartTime();
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
*/

void
MatrixScene::playNote(const Segment &segment, int pitch, int velocity)
{
//    std::cout << "Scene is playing a note of pitch: " << pitch
//              << " + " <<  segment.getTranspose();
    if (!m_document) return;

    Instrument *instrument = m_document->getStudio().getInstrumentFor(&segment);

    StudioControl::playPreviewNote(instrument,
                                   pitch + segment.getTranspose(),
                                   velocity,
                                   RealTime(0, 250000000));
}

void
MatrixScene::setHorizontalZoomFactor(double factor)
{
    for (Segment *segment : m_segments) {
        segment->matrixHZoomFactor = factor;
    }
}

void
MatrixScene::setVerticalZoomFactor(double factor)
{
    for (Segment *segment : m_segments) {
        segment->matrixVZoomFactor = factor;
    }
}

void
MatrixScene::updateAll()
{
    for (std::vector<MatrixViewSegment *>::iterator i = m_viewSegments.begin();
         i != m_viewSegments.end(); ++i) {
        (*i)->updateAll();
    }
    recreatePitchHighlights();
    updateCurrentSegment();
}

void
MatrixScene::setExtraPreviewEvents(const EventWithSegmentMap& events)
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

int
MatrixScene::findSegmentIndex(const Segment *segment) const
{
    for (int i = 0; i < static_cast<int>(m_segments.size()); ++i) {
        if (m_segments[i] == segment)
            return i;
    }

    // Not found.
    return -1;
}


}
