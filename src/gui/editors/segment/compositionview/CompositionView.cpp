/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A MIDI and audio sequencer and musical notation editor.
  Copyright 2000-2015 the Rosegarden development team.
 
  Other copyrights also apply to some parts of this work.  Please
  see the AUTHORS file and individual file headers for details.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CompositionView]"

#include "CompositionView.h"

#include "misc/Debug.h"
#include "AudioPreviewThread.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "base/Profiler.h"
#include "CompositionColourCache.h"
#include "CompositionItemHelper.h"
#include "CompositionItem.h"
#include "CompositionRect.h"
#include "AudioPreviewPainter.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentSelector.h"
#include "SegmentToolBox.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
//#include <QScrollBar>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include <algorithm>  // std::min, std::max, std::find


namespace Rosegarden
{


CompositionView::CompositionView(RosegardenDocument *doc,
                                 CompositionModelImpl *model,
                                 QWidget *parent) :
    RosegardenScrollView(parent),
    m_model(model),
    m_currentTool(0),
    m_toolBox(new SegmentToolBox(this, doc)),
    m_enableDrawing(true),
    m_showPreviews(false),
    m_showSegmentLabels(true),
    m_pencilOverExisting(false),
    //m_minWidth(m_model->getCompositionLength()),
    m_stepSize(0),
    //m_rectFill(0xF0, 0xF0, 0xF0),
    //m_selectedRectFill(0x00, 0x00, 0xF0),
    m_pointerPos(0),
    m_pointerPen(GUIPalette::getColour(GUIPalette::Pointer), 4),
    m_tmpRect(QPoint(0, 0), QPoint( -1, -1)),  // invalid
    m_tmpRectFill(CompositionRect::DefaultBrushColor),
    m_trackDividerColor(GUIPalette::getColour(GUIPalette::TrackDivider)),
    m_drawGuides(false),
    m_guideColor(GUIPalette::getColour(GUIPalette::MovementGuide)),
    m_guideX(0),
    m_guideY(0),
    m_drawSelectionRect(false),
    m_drawTextFloat(false),
    m_segmentsLayer(viewport()->width(), viewport()->height()),
    m_segmentsRefresh(0, 0, viewport()->width(), viewport()->height()),
    m_lastContentsX(0),
    m_lastContentsY(0),
    m_lastPointerRefreshX(0),
    m_contextHelpShown(false),
    m_updateTimer(new QTimer(static_cast<QObject *>(this))),
    m_updateNeeded(false)
//  m_updateRect()
{
    if (!doc)
        return;
    if (!m_model)
        return;

    // Causing slow refresh issues on RG Main Window -- 10-12-2011 - JAS
    // ??? This appears to have no effect positive or negative now (2015).
    //viewport()->setAttribute(Qt::WA_PaintOnScreen);

    // Disable background erasing.  We redraw everything.  This would
    // just waste time.  (It's hard to measure any improvement here.)
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

    QSettings settings;

    // If background textures are enabled, load the texture pixmap.
    if (settings.value(
            QString(GeneralOptionsConfigGroup) + "/backgroundtextures",
            "true").toBool()) {

        IconLoader il;
        m_backgroundPixmap = il.loadPixmap("bg-segmentcanvas");
    }

    slotUpdateSize();

    // *** Connections

    connect(m_toolBox, SIGNAL(showContextHelp(const QString &)),
            this, SLOT(slotToolHelpChanged(const QString &)));

    connect(m_model, SIGNAL(needContentUpdate()),
            this, SLOT(slotUpdateAll()));
    connect(m_model, SIGNAL(needContentUpdate(const QRect&)),
            this, SLOT(slotAllNeedRefresh(const QRect&)));
    connect(m_model, SIGNAL(needArtifactsUpdate()),
            this, SLOT(slotUpdateArtifacts()));
    connect(m_model, SIGNAL(needSizeUpdate()),
            this, SLOT(slotUpdateSize()));

    connect(doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotRefreshColourCache()));

    // recording-related signals
    connect(doc, SIGNAL(newMIDIRecordingSegment(Segment*)),
            this, SLOT(slotNewMIDIRecordingSegment(Segment*)));
    connect(doc, SIGNAL(newAudioRecordingSegment(Segment*)),
            this, SLOT(slotNewAudioRecordingSegment(Segment*)));
    connect(doc, SIGNAL(stoppedAudioRecording()),
            this, SLOT(slotStoppedRecording()));
    connect(doc, SIGNAL(stoppedMIDIRecording()),
            this, SLOT(slotStoppedRecording()));
    connect(doc, SIGNAL(audioFileFinalized(Segment*)),
            m_model, SLOT(slotAudioFileFinalized(Segment*)));
    //connect(doc, SIGNAL(recordMIDISegmentUpdated(Segment*, timeT)),
    //        this, SLOT(slotRecordMIDISegmentUpdated(Segment*, timeT)));

    // Audio Preview Thread
    m_model->setAudioPreviewThread(&doc->getAudioPreviewThread());
    doc->getAudioPreviewThread().setEmptyQueueListener(this);

    // Update timer
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateTimer()));
    m_updateTimer->start(100);

    // *** Debugging

    settings.beginGroup("Performance_Testing");

    m_enableDrawing = (settings.value("CompositionView", 1).toInt() != 0);

    // Write it to the file to make it easier to find.
    settings.setValue("CompositionView", m_enableDrawing ? 1 : 0);

    settings.endGroup();
}

void CompositionView::endAudioPreviewGeneration()
{
    if (m_model) {
        m_model->setAudioPreviewThread(0);
    }
}

#if 0
// Dead Code.
void CompositionView::initStepSize()
{
    QScrollBar* hsb = horizontalScrollBar();
    m_stepSize = hsb->singleStep();
}
#endif

void CompositionView::slotUpdateSize()
{
    const int height = std::max((int)m_model->getCompositionHeight(), viewport()->height());

    const RulerScale *rulerScale = grid().getRulerScale();
    const int compositionWidth = (int)ceil(rulerScale->getTotalWidth());
    const int minWidth = sizeHint().width();
    const int width = std::max(compositionWidth, minWidth);

    // If the width or height need to change...
    if (contentsWidth() != width  ||  contentsHeight() != height)
        resizeContents(width, height);
}

void CompositionView::setSelectionRectPos(const QPoint &pos)
{
    // Update the selection rect used for drawing the rubber band.
    m_selectionRect.setRect(pos.x(), pos.y(), 0, 0);
    // Pass on to CompositionModelImpl which will adjust the selected
    // segments and redraw them.
    m_model->setSelectionRect(m_selectionRect);
}

void CompositionView::setSelectionRectSize(int w, int h)
{
    // Update the selection rect used for drawing the rubber band.
    m_selectionRect.setSize(QSize(w, h));
    // Pass on to CompositionModelImpl which will adjust the selected
    // segments and redraw them.
    m_model->setSelectionRect(m_selectionRect);
}

void CompositionView::setDrawSelectionRect(bool draw)
{
    if (m_drawSelectionRect != draw) {
        m_drawSelectionRect = draw;

        // Redraw the selection rect.
        slotUpdateArtifacts();

        // Indicate that the segments in that area need updating.
        // ??? This isn't needed since slotUpdateArtifacts() calls
        //     updateContents() which causes a repaint of everything.
        //     Besides, on enable and disable, nothing much changes anyway.
        //slotAllNeedRefresh(m_selectionRect);
    }
}

void CompositionView::clearSegmentRectsCache(bool clearPreviews)
{
    m_model->clearSegmentRectsCache(clearPreviews);
}

SegmentSelection
CompositionView::getSelectedSegments()
{
    return m_model->getSelectedSegments();
}

void CompositionView::updateSelectedSegments()
{
    if (!m_model->haveSelection())
        return;

    updateContents(m_model->getSelectedSegmentsRect());
}

void CompositionView::setTool(const QString &toolName)
{
    if (m_currentTool)
        m_currentTool->stow();

    m_toolContextHelp = "";

    m_currentTool = m_toolBox->getTool(toolName);

    if (!m_currentTool) {
        QMessageBox::critical(0, tr("Rosegarden"), QString("CompositionView::setTool() : unknown tool name %1").arg(toolName));
        return;
    }

    m_currentTool->ready();
}

void CompositionView::selectSegments(const SegmentSelection &segments)
{
    m_model->selectSegments(segments);
}

void CompositionView::showSplitLine(int x, int y)
{
    m_splitLinePos.setX(x);
    m_splitLinePos.setY(y);
    viewport()->update();
}

void CompositionView::hideSplitLine()
{
    m_splitLinePos.setX( -1);
    m_splitLinePos.setY( -1);
    viewport()->update();
}

void CompositionView::slotExternalWheelEvent(QWheelEvent *e)
{
    // Pass it up to RosegardenScrollView.
    wheelEvent(e);
    // We've got this.  No need to propagate.
    e->accept();
}

void CompositionView::slotUpdateAll()
{
    // This one doesn't get called too often while recording.
    //Profiler profiler("CompositionView::slotUpdateAll()");

    // Redraw the segments and artifacts.
    updateAll();
}

void CompositionView::slotUpdateTimer()
{
    if (m_updateNeeded) {
        updateAll2(m_updateRect);
        m_updateNeeded = false;
    }
}

void CompositionView::updateAll2(const QRect &rect)
{
    Profiler profiler("CompositionView::updateAll2(rect)");

    //RG_DEBUG << "updateAll2() rect:" << rect << ", valid:" << rect.isValid();

    // If the incoming rect is invalid, just do everything.
    // ??? This is probably not necessary as an invalid rect
    //     cannot get in here.  See slotAllNeedRefresh(rect).
    if (!rect.isValid()) {
        updateAll();
        return;
    }

    segmentsNeedRefresh(rect);
    updateArtifacts(rect);
}

void CompositionView::slotAllNeedRefresh(const QRect &rect)
{
    // Bail if drawing is turned off in the settings.
    if (!m_enableDrawing)
        return;

    // This one gets hit pretty hard while recording.
    Profiler profiler("CompositionView::slotAllNeedRefresh(const QRect &rect)");

    // Note: This new approach normalizes the incoming rect.  This means
    //   that it will never trigger a full refresh given an invalid rect
    //   like it used to.  See updateAll2(rect).
    if (!rect.isValid())
        RG_DEBUG << "slotAllNeedRefresh(rect): Invalid rect";

    // If an update is now needed, set m_updateRect, otherwise accumulate it.
    if (!m_updateNeeded) {
        // Let slotUpdateTimer() know an update is needed next time.
        m_updateNeeded = true;
        m_updateRect = rect.normalized();
    } else {
        // Accumulate the update rect
        m_updateRect |= rect.normalized();
    }
}

void CompositionView::slotRefreshColourCache()
{
    CompositionColourCache::getInstance()->init();
    clearSegmentRectsCache();
    updateAll();
}

void CompositionView::slotNewMIDIRecordingSegment(Segment *s)
{
    m_model->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotNewAudioRecordingSegment(Segment *s)
{
    m_model->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotStoppedRecording()
{
    m_model->clearRecordingItems();
}

void CompositionView::resizeEvent(QResizeEvent *e)
{
    RosegardenScrollView::resizeEvent(e);

    // Resize the contents if needed.
    slotUpdateSize();

    // If the viewport has grown larger than the segments layer
    if (e->size().width() > m_segmentsLayer.width()  ||
        e->size().height() > m_segmentsLayer.height()) {

        // Reallocate the segments layer
        m_segmentsLayer = QPixmap(e->size().width(), e->size().height());
    }

    updateAll();
}

void CompositionView::paintEvent(QPaintEvent *)
{
    Profiler profiler("CompositionView::paintEvent()");

    // Just redraw the entire viewport.  Turns out that for the most
    // critical use case, recording, this is actually slightly faster
    // than trying to be frugal about drawing small parts of the viewport.
    // The code is certainly easier to read.
    drawAll();
}

void CompositionView::drawAll()
{
    Profiler profiler("CompositionView::drawAll()");

    // Scroll and refresh the segments layer.
    scrollSegmentsLayer();

    // ??? Try putting the artifacts on their own layer pixmap.  This might
    //     simplify the code a bit.  Should also allow us to avoid
    //     redrawing the artifacts just because the segments need to
    //     be updated.  This might help the recording use case a little.
    // ??? There are two key use cases that need to be optimized.
    //     The first is recording.  This is the most important as it uses
    //     a lot of CPU (and shouldn't).  The second is auto-scrolling.
    //     It's not quite as important since it is relatively rare.

    // Copy the entire segments layer to the viewport

    QRect viewportRect = viewport()->rect();
    // Copy the segments to the viewport.
    QPainter viewportPainter(viewport());
    viewportPainter.drawPixmap(
            viewportRect, m_segmentsLayer, viewportRect);
    viewportPainter.end();

    // Redraw all of the artifacts on the viewport.

    drawArtifacts();
}

void CompositionView::scrollSegmentsLayer()
{
    Profiler profiler("CompositionView::scrollSegmentsLayer()");

    // Portion of the segments layer that needs to be redrawn.
    QRect refreshRect = m_segmentsRefresh;

    const int w = viewport()->width();
    const int h = viewport()->height();
    const int cx = contentsX();
    const int cy = contentsY();
    // The entire viewport in contents coords.
    const QRect viewportContentsRect(cx, cy, w, h);

    bool scroll = (cx != m_lastContentsX || cy != m_lastContentsY);

    if (scroll) {

        // ??? All this scroll optimization saves about 7% cpu on my
        //     machine when auto-scrolling the BWV1048 example.  Doesn't
        //     seem worth the extra code.

        if (refreshRect.isValid()) {

            // If we've scrolled and there was an existing refresh
            // rect, we can't be sure whether the refresh rect
            // predated or postdated the internal update of scroll
            // location.  Cut our losses and refresh everything.

            refreshRect = viewportContentsRect;

        } else {

            // No existing refresh rect: we only need to handle the
            // scroll.

            // Horizontal scroll distance
            int dx = m_lastContentsX - cx;

            // If we're scrolling horizontally
            if (dx != 0) {

                // If we're scrolling less than the entire viewport
                if (abs(dx) < w) {

                    // Scroll the segments layer sideways
                    m_segmentsLayer.scroll(dx, 0, m_segmentsLayer.rect());

                    // Add the part that was exposed to the refreshRect
                    if (dx < 0) {
                        refreshRect |= QRect(cx + w + dx, cy, -dx, h);
                    } else {
                        refreshRect |= QRect(cx, cy, dx, h);
                    }

                } else {  // We've scrolled more than the entire viewport

                    // Refresh everything
                    refreshRect = viewportContentsRect;
                }
            }

            // Vertical scroll distance
            int dy = m_lastContentsY - cy;

            // If we're scrolling vertically and the sideways scroll didn't
            // result in a need to refresh everything,
            if (dy != 0  &&  refreshRect != viewportContentsRect) {

                // If we're scrolling less than the entire viewport
                if (abs(dy) < h) {

                    // Scroll the segments layer vertically
                    m_segmentsLayer.scroll(0, dy, m_segmentsLayer.rect());

                    // Add the part that was exposed to the refreshRect
                    if (dy < 0) {
                        refreshRect |= QRect(cx, cy + h + dy, w, -dy);
                    } else {
                        refreshRect |= QRect(cx, cy, w, dy);
                    }

                } else {  // We've scrolled more than the entire viewport

                    // Refresh everything
                    refreshRect = viewportContentsRect;
                }
            }
        }
    }

    m_lastContentsX = cx;
    m_lastContentsY = cy;

    // If we need to redraw the segments layer, do so.
    if (refreshRect.isValid()) {
        // Refresh the segments layer
        drawSegments(refreshRect);
        m_segmentsRefresh = QRect();
    }
}

void CompositionView::drawSegments(const QRect &clipRect)
{
    Profiler profiler("CompositionView::drawSegments(clipRect)");

    QPainter segmentsLayerPainter(&m_segmentsLayer);
    // Switch to contents coords.
    segmentsLayerPainter.translate(-contentsX(), -contentsY());

    // *** Draw the background

    if (!m_backgroundPixmap.isNull()) {
        QPoint offset(
                clipRect.x() % m_backgroundPixmap.height(),
                clipRect.y() % m_backgroundPixmap.width());
        segmentsLayerPainter.drawTiledPixmap(
                clipRect, m_backgroundPixmap, offset);
    } else {
        segmentsLayerPainter.eraseRect(clipRect);
    }

    // *** Draw the track dividers

    drawTrackDividers(&segmentsLayerPainter, clipRect);

    // *** Get Segment and Preview Rectangles

    // Assume we aren't going to show previews.
    CompositionModelImpl::RectRanges *notationPreview = 0;
    CompositionModelImpl::AudioPreviewDrawData *audioPreview = 0;

    if (m_showPreviews) {
        // Clear the previews.
        // ??? Move this clearing into CompositionModelImpl::getSegmentRects()?
        m_notationPreview.clear();
        m_audioPreview.clear();

        // Indicate that we want previews.
        notationPreview = &m_notationPreview;
        audioPreview = &m_audioPreview;
    }

    // Fetch segment rectangles and (optionally) previews
    const CompositionModelImpl::RectContainer &segmentRects =
            m_model->getSegmentRects(clipRect, notationPreview, audioPreview);

    // *** Draw Segment Rectangles

    // For each segment rectangle, draw it
    for (CompositionModelImpl::RectContainer::const_iterator i = segmentRects.begin();
         i != segmentRects.end(); ++i) {

        drawCompRect(*i, &segmentsLayerPainter, clipRect);
    }

    drawIntersections(segmentRects, &segmentsLayerPainter, clipRect);

    // *** Draw Segment Previews

    if (m_showPreviews) {
        // We'll be modifying the transform.  save()/restore() to be safe.
        segmentsLayerPainter.save();

        // Audio Previews

        drawAudioPreviews(&segmentsLayerPainter, clipRect);

        // Notation Previews

        QColor defaultColor = CompositionColourCache::getInstance()->SegmentInternalPreview;

        // For each preview range
        // ??? I think there is never more than one range here.  We should be
        //     able to switch from RectRanges to RectRange.
        for (CompositionModelImpl::RectRanges::const_iterator notationPreviewIter =
                 m_notationPreview.begin();
             notationPreviewIter != m_notationPreview.end();
             ++notationPreviewIter) {

            const CompositionModelImpl::RectRange &rectRange = *notationPreviewIter;

            QColor color = rectRange.color.isValid() ? rectRange.color : defaultColor;

            // translate() calls are cumulative, so we need to be able to get
            // back to where we were.  Note that resetTransform() would be
            // too extreme as it would reverse the contents translation that
            // is present in segmentsLayerPainter at this point in time.
            segmentsLayerPainter.save();
            // Coords WRT segment rect's base point.  (Upper left?)
            segmentsLayerPainter.translate(
                    rectRange.basePoint.x(), rectRange.basePoint.y());

            // For each event rectangle, draw it.
            for (CompositionModelImpl::RectList::const_iterator i =
                     rectRange.range.first;
                 i != rectRange.range.second;
                 ++i) {

                QRect eventRect = *i;
                // Make the rect thicker vertically to match the old
                // appearance.  Without this, the rect is thin, which gives
                // slightly more information.
                eventRect.adjust(0,0,0,1);

                // Per the Qt docs, fillRect() should be faster than
                // drawRect().  In practice, a small improvement was noted.
                segmentsLayerPainter.fillRect(eventRect, color);
            }
            // Restore the transformation.
            segmentsLayerPainter.restore();
        }

        segmentsLayerPainter.restore();
    }

    // *** Draw Segment Labels

    if (m_showSegmentLabels) {
        // For each segment rect, draw the label
        for (CompositionModelImpl::RectContainer::const_iterator i = segmentRects.begin();
             i != segmentRects.end(); ++i) {

            drawCompRectLabel(*i, &segmentsLayerPainter, clipRect);
        }
    }
}

void CompositionView::drawArtifacts()
{
    Profiler profiler("CompositionView::drawArtifacts()");

    // The entire viewport in contents coords.
    QRect viewportContentsRect(
            contentsX(), contentsY(),
            viewport()->rect().width(), viewport()->rect().height());

    QPainter viewportPainter(viewport());
    // Switch to contents coords.
    viewportPainter.translate(-contentsX(), -contentsY());

    //
    // Playback Pointer
    //
    drawPointer(&viewportPainter, viewportContentsRect);

    //
    // Tmp rect (rect displayed while drawing a new segment)
    //
    if (m_tmpRect.isValid() && m_tmpRect.intersects(viewportContentsRect)) {
        viewportPainter.setBrush(m_tmpRectFill);
        viewportPainter.setPen(CompositionColourCache::getInstance()->SegmentBorder);
        drawRect(m_tmpRect, &viewportPainter, viewportContentsRect);
    }

    //
    // Tool guides (crosshairs)
    //
    if (m_drawGuides)
        drawGuides(&viewportPainter, viewportContentsRect);

    //
    // Selection Rect (rubber band)
    //
    if (m_drawSelectionRect) {
        viewportPainter.setPen(CompositionColourCache::getInstance()->SegmentBorder);
        drawRect(m_selectionRect.normalized(), &viewportPainter, viewportContentsRect,
                 false,  // isSelected
                 0,      // intersectLvl
                 false); // fill
    }

    //
    // Floating Text
    //
    if (m_drawTextFloat)
        drawTextFloat(&viewportPainter, viewportContentsRect);

    //
    // Split line
    //
    if (m_splitLinePos.x() > 0 && viewportContentsRect.contains(m_splitLinePos)) {
        viewportPainter.save();
        viewportPainter.setPen(m_guideColor);
        viewportPainter.drawLine(m_splitLinePos.x(), m_splitLinePos.y(),
                    m_splitLinePos.x(), m_splitLinePos.y() + m_model->grid().getYSnap());
        viewportPainter.restore();
    }
}

void CompositionView::drawTrackDividers(QPainter *segmentsLayerPainter, const QRect &clipRect)
{
    // Fetch track Y coordinates within the clip rectangle.  We expand the
    // clip rectangle slightly because we are drawing a rather wide track
    // divider, so we need enough divider coords to do the drawing even
    // though the center of the divider might be slightly outside of the
    // viewport.
    CompositionModelImpl::YCoordList trackYCoords =
            m_model->getTrackDividersIn(clipRect.adjusted(0,-1,0,+1));

    // Nothing to do?  Bail.
    if (trackYCoords.empty())
        return;

    const int left = clipRect.left();
    const int right = clipRect.right();

    segmentsLayerPainter->save();

    // For each track Y coordinate
    for (CompositionModelImpl::YCoordList::const_iterator yi = trackYCoords.begin();
         yi != trackYCoords.end(); ++yi) {

        int y = *yi - 2;
        segmentsLayerPainter->setPen(m_trackDividerColor);
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor.light());
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor.light());
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor);
        segmentsLayerPainter->drawLine(left, y, right, y);
    }

    segmentsLayerPainter->restore();
}

void CompositionView::drawAudioPreviews(QPainter * p, const QRect& clipRect)
{
    Profiler profiler("CompositionView::drawAudioPreviews");

    CompositionModelImpl::AudioPreviewDrawData::const_iterator api = m_audioPreview.begin();
    CompositionModelImpl::AudioPreviewDrawData::const_iterator apEnd = m_audioPreview.end();
    QRect rectToFill,  // rect to fill on canvas
        localRect; // the rect of the tile to draw on the canvas
    QPoint basePoint,  // origin of segment rect
        drawBasePoint; // origin of rect to fill on canvas
    QRect r;
    for (; api != apEnd; ++api) {
        rectToFill = api->rect;
        basePoint = api->basePoint;
        rectToFill.moveTopLeft(basePoint);
        rectToFill &= clipRect;
        r = rectToFill;
        drawBasePoint = rectToFill.topLeft();
        rectToFill.translate( -basePoint.x(), -basePoint.y());
        int firstPixmapIdx = (r.x() - basePoint.x()) / AudioPreviewPainter::tileWidth();
        if (firstPixmapIdx >= int(api->pixmap.size())) {
            //RG_DEBUG << "CompositionView::drawAudioPreviews : WARNING - miscomputed pixmap array : r.x = "
            //         << r.x() << " - basePoint.x = " << basePoint.x() << " - firstPixmapIdx = " << firstPixmapIdx
            //         << endl;
            continue;
        }
        int x = 0, idx = firstPixmapIdx;
        //RG_DEBUG << "CompositionView::drawAudioPreviews : clipRect = " << clipRect
        //         << " - firstPixmapIdx = " << firstPixmapIdx << endl;
        while (x < clipRect.width()) {
            int pixmapRectXOffset = idx * AudioPreviewPainter::tileWidth();
            localRect.setRect(basePoint.x() + pixmapRectXOffset, basePoint.y(),
                              AudioPreviewPainter::tileWidth(), api->rect.height());
            //RG_DEBUG << "CompositionView::drawAudioPreviews : initial localRect = "
            //         << localRect << endl;
            localRect &= r;
            if (idx == firstPixmapIdx && api->resizeOffset != 0) {
                // this segment is being resized from start, clip beginning of preview
                localRect.translate(api->resizeOffset, 0);
            }

            //RG_DEBUG << "CompositionView::drawAudioPreviews : localRect & clipRect = "
            //         << localRect << endl;
            if (localRect.isEmpty()) {
                //RG_DEBUG << "CompositionView::drawAudioPreviews : localRect & clipRect is empty\n";
                break;
            }
            localRect.translate( -(basePoint.x() + pixmapRectXOffset), -basePoint.y());

            //RG_DEBUG << "CompositionView::drawAudioPreviews : drawing pixmap "
            //         << idx << " at " << drawBasePoint << " - localRect = " << localRect
            //         << " - preResizeOrigin : " << api->preResizeOrigin << endl;

            p->drawImage(drawBasePoint, api->pixmap[idx], localRect,
                         Qt::ColorOnly | Qt::ThresholdDither | Qt::AvoidDither);

            ++idx;
            if (idx >= int(api->pixmap.size()))
                break;
            drawBasePoint.setX(drawBasePoint.x() + localRect.width());
            x += localRect.width();
        }
    }
}

void CompositionView::drawGuides(QPainter * p, const QRect& /*clipRect*/)
{
    // no need to check for clipping, these guides are meant to follow the mouse cursor
    QPoint guideOrig(m_guideX, m_guideY);
    
    int contentsHeight = this->contentsHeight();
    int contentsWidth = this->contentsWidth();
//    int contentsHeight = this->widget()->height();    //@@@
//    int contentsWidth = this->widget()->width();
    
    p->save();
    p->setPen(m_guideColor);
    p->drawLine(guideOrig.x(), 0, guideOrig.x(), contentsHeight);
    p->drawLine(0, guideOrig.y(), contentsWidth, guideOrig.y());
    p->restore();
}

void CompositionView::drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                                   int intersectLvl, bool fill)
{
    p->save();

    QBrush brush = r.getBrush();

    if (r.isRepeating()) {
        QColor brushColor = brush.color();
        brush.setColor(brushColor.light(150));
    }

    p->setBrush(brush);
    p->setPen(r.getPen());
    drawRect(r, p, clipRect, r.isSelected(), intersectLvl, fill);

    if (r.isRepeating()) {

        CompositionRect::repeatmarks repeatMarks = r.getRepeatMarks();

        //RG_DEBUG << "CompositionView::drawCompRect() : drawing repeating rect " << r
        //         << " nb repeat marks = " << repeatMarks.size() << endl;

        // draw 'start' rectangle with original brush
        //
        QRect startRect = r;
        startRect.setWidth(repeatMarks[0] - r.x());
        p->setBrush(r.getBrush());
        drawRect(startRect, p, clipRect, r.isSelected(), intersectLvl, fill);


        // now draw the 'repeat' marks
        //
        p->setPen(CompositionColourCache::getInstance()->RepeatSegmentBorder);
        int penWidth = int(std::max((unsigned int)r.getPen().width(), 1u));

        for (int i = 0; i < repeatMarks.size(); ++i) {
            int pos = repeatMarks[i];
            if (pos > clipRect.right())
                break;

            if (pos >= clipRect.left()) {
                QPoint p1(pos, r.y() + penWidth),
                    p2(pos, r.y() + r.height() - penWidth - 1);

                //RG_DEBUG << "CompositionView::drawCompRect() : drawing repeat mark at "
                //         << p1 << "-" << p2 << endl;
                p->drawLine(p1, p2);
            }

        }

    }

    p->restore();
}

void CompositionView::drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& /* clipRect */)
{
    // draw segment label
    //
    if (!r.getLabel().isEmpty()) {

        p->save();

        QFont font;
        font.setPixelSize(r.height() / 2.2);
        font.setWeight(QFont::Bold);
        font.setItalic(false);
        p->setFont(font);

        QRect labelRect = QRect
            (r.x(),
             r.y() + ((r.height() - p->fontMetrics().height()) / 2) + 1,
             r.width(),
             p->fontMetrics().height());

        int x = labelRect.x() + p->fontMetrics().width('x');
        int y = labelRect.y();

        // get the segment color as the foundation for drawing the surrounding
        // "halo" to ensure contrast against any preview dashes that happen to
        // be present right under the label
        QBrush brush = r.getBrush();

        // figure out the intensity of this base color, Yves style
        QColor surroundColor = brush.color();
        int intensity = qGray(surroundColor.rgb());

        bool lightSurround = false;

        // straight black or white as originally rewritten was too stark, so we
        // go back to using lighter() against the base color, and this time we
        // use darker() against the base color too
        if (intensity < 127) {
            surroundColor = surroundColor.darker(100);
        } else {
            surroundColor = surroundColor.lighter(100);
            lightSurround = true;
        }

        // draw the "halo" effect
        p->setPen(surroundColor);

        for (int i = 0; i < 9; ++i) {

            if (i == 4)
                continue;

            int wx = x, wy = y;

            if (i < 3)
                --wx;
            if (i > 5)
                ++wx;
            if (i % 3 == 0)
                --wy;
            if (i % 3 == 2)
                ++wy;

            labelRect.setX(wx);
            labelRect.setY(wy);

            p->drawText(labelRect,
                        Qt::AlignLeft | Qt::AlignTop,
                        r.getLabel());
        }

        labelRect.setX(x);
        labelRect.setY(y);

        // use black on light halo, white on dark halo for the text itself
        p->setPen((lightSurround ? Qt::black : Qt::white));

        p->drawText(labelRect,
                    Qt::AlignLeft | Qt::AlignVCenter, r.getLabel());
        p->restore();
    }
}

void CompositionView::drawRect(const QRect& r, QPainter *p, const QRect& clipRect,
                               bool isSelected, int intersectLvl, bool fill)
{
    //RG_DEBUG << "CompositionView::drawRect : intersectLvl = " << intersectLvl
    //         << " - brush col = " << p->brush().color() << endl;

    //RG_DEBUG << "CompositionView::drawRect " << r << " - xformed : " << p->xForm(r)
    //         << " - contents x = " << contentsX() << ", contents y = " << contentsY() << endl;

    p->save();

    QRect rect = r;
    rect.setSize(rect.size() - QSize(1, 1));                                    

    //RG_DEBUG << "drawRect: rect is " << rect << endl;

    if (fill) {
        if (isSelected) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark(200);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
            //RG_DEBUG << "CompositionView::drawRect : selected color : " << fillColor << endl;
        }

        if (intersectLvl > 0) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark((intersectLvl) * 105);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
            //RG_DEBUG << "CompositionView::drawRect : intersected color : " << fillColor << " isSelected : " << isSelected << endl;
        }
    } else {
        p->setBrush(Qt::NoBrush);
    }

    // Paint using the small coordinates...
    QRect intersection = rect.intersected(clipRect);

    if (clipRect.contains(rect)) {
        //RG_DEBUG << "note: drawing whole rect" << endl;
        p->drawRect(rect);
    } else {
        // draw only what's necessary
        if (!intersection.isEmpty() && fill)
            p->fillRect(intersection, p->brush());

        int rectTopY = rect.y();

        if (rectTopY >= clipRect.y() &&
            rectTopY <= (clipRect.y() + clipRect.height() - 1)) {
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.topLeft(), intersection.topRight());
        }

        int rectBottomY = rect.y() + rect.height();
        if (rectBottomY >= clipRect.y() &&
            rectBottomY < (clipRect.y() + clipRect.height() - 1))
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.bottomLeft() + QPoint(0, 1),
                        intersection.bottomRight() + QPoint(0, 1));

        int rectLeftX = rect.x();
        if (rectLeftX >= clipRect.x() &&
            rectLeftX <= (clipRect.x() + clipRect.width() - 1))
            p->drawLine(rect.topLeft(), rect.bottomLeft());

        int rectRightX = rect.x() + rect.width(); // make sure we don't overflow
        if (rectRightX >= clipRect.x() &&
            rectRightX < clipRect.x() + clipRect.width() - 1)
            p->drawLine(rect.topRight(), rect.bottomRight());

    }

    p->restore();
}

QColor CompositionView::mixBrushes(const QBrush &a, const QBrush &b)
{
    const QColor &ac = a.color();
    const QColor &bc = b.color();

    return QColor((ac.red()   + bc.red())   / 2,
                  (ac.green() + bc.green()) / 2,
                  (ac.blue()  + bc.blue())  / 2);
}

void CompositionView::drawIntersections(const CompositionModelImpl::RectContainer& rects,
                                        QPainter * p, const QRect& clipRect)
{
    if (! (rects.size() > 1))
        return ;

    CompositionModelImpl::RectContainer intersections;

    CompositionModelImpl::RectContainer::const_iterator i = rects.begin(),
        j = rects.begin();

    for (; j != rects.end(); ++j) {

        CompositionRect testRect = *j;
        i = j;
        ++i; // set i to pos after j

        if (i == rects.end())
            break;

        for (; i != rects.end(); ++i) {
            CompositionRect ri = testRect.intersected(*i);
            if (!ri.isEmpty()) {
                CompositionModelImpl::RectContainer::iterator t = std::find(intersections.begin(),
                                                                        intersections.end(), ri);
                if (t == intersections.end()) {
                    ri.setBrush(mixBrushes(testRect.getBrush(), i->getBrush()));
                    ri.setSelected(testRect.isSelected() || i->isSelected());
                    intersections.push_back(ri);
                }

            }
        }
    }

    //
    // draw this level of intersections then compute and draw further ones
    //
    int intersectionLvl = 1;

    while (!intersections.empty()) {

        for (CompositionModelImpl::RectContainer::iterator intIter = intersections.begin();
             intIter != intersections.end(); ++intIter) {
            CompositionRect r = *intIter;
            drawCompRect(r, p, clipRect, intersectionLvl);
        }

        if (intersections.size() > 10)
            break; // put a limit on how many intersections we can compute and draw - this grows exponentially

        ++intersectionLvl;

        CompositionModelImpl::RectContainer intersections2;

        CompositionModelImpl::RectContainer::iterator i = intersections.begin(),
            j = intersections.begin();

        for (; j != intersections.end(); ++j) {

            CompositionRect testRect = *j;
            i = j;
            ++i; // set i to pos after j

            if (i == intersections.end())
                break;

            for (; i != intersections.end(); ++i) {
                CompositionRect ri = testRect.intersected(*i);
                if (!ri.isEmpty() && ri != *i) {
                    CompositionModelImpl::RectContainer::iterator t = std::find(intersections2.begin(),
                                                                            intersections2.end(), ri);
                    if (t == intersections2.end())
                        ri.setBrush(mixBrushes(testRect.getBrush(), i->getBrush()));
                    intersections2.push_back(ri);
                }
            }
        }

        intersections = intersections2;
    }

}

void CompositionView::drawPointer(QPainter *p, const QRect& clipRect)
{
    //RG_DEBUG << "CompositionView::drawPointer: clipRect "
    //         << clipRect.x() << "," << clipRect.y() << " " << clipRect.width()
    //         << "x" << clipRect.height() << " pointer pos is " << m_pointerPos << endl;

    if (m_pointerPos >= clipRect.x() && m_pointerPos <= (clipRect.x() + clipRect.width())) {
        p->save();
        p->setPen(m_pointerPen);
        p->drawLine(m_pointerPos, clipRect.y(), m_pointerPos, clipRect.y() + clipRect.height());
        p->restore();
    }

}

void CompositionView::drawTextFloat(QPainter *p, const QRect& clipRect)
{
    QFontMetrics metrics(p->fontMetrics());

    QRect bound = p->boundingRect(0, 0, 300, metrics.height() + 6, Qt::AlignLeft, m_textFloatText);

    p->save();

    bound.setLeft(bound.left() - 2);
    bound.setRight(bound.right() + 2);
    bound.setTop(bound.top() - 2);
    bound.setBottom(bound.bottom() + 2);

    QPoint pos(m_textFloatPos);
    if (pos.y() < 0 && m_model) {
        if (pos.y() + bound.height() < 0) {
            pos.setY(pos.y() + m_model->grid().getYSnap() * 3);
        } else {
            pos.setY(pos.y() + m_model->grid().getYSnap() * 2);
        }
    }

    bound.moveTopLeft(pos);

    if (bound.intersects(clipRect)) {

        p->setBrush(CompositionColourCache::getInstance()->RotaryFloatBackground);

        p->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);

        drawRect(bound, p, clipRect, false, 0, true);

        p->drawText(pos.x() + 2, pos.y() + 3 + metrics.ascent(), m_textFloatText);

    }

    p->restore();
}

bool CompositionView::event(QEvent* e)
{
    if (e->type() == AudioPreviewThread::AudioPreviewQueueEmpty) {
        RG_DEBUG << "CompositionView::event - AudioPreviewQueueEmpty\n";
        segmentsNeedRefresh();
        viewport()->update();
        return true;
    }

    return RosegardenScrollView::event(e);
}

void CompositionView::enterEvent(QEvent */* e */)
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("toolcontexthelp", "true" ) ) ) {
        settings.endGroup();
        return;
    }
    settings.endGroup();

    emit showContextHelp(m_toolContextHelp);
    m_contextHelpShown = true;
}

void CompositionView::leaveEvent(QEvent */* e */)
{
    emit showContextHelp("");
    m_contextHelpShown = false;
}

void CompositionView::slotToolHelpChanged(const QString &text)
{
    if (m_toolContextHelp == text) return;
    m_toolContextHelp = text;

    QSettings settings;

    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("toolcontexthelp", "true" ) ) ) {
        settings.endGroup();
        return;
    }
    settings.endGroup();

    if (m_contextHelpShown) emit showContextHelp(text);
}

void CompositionView::mousePressEvent(QMouseEvent *e)
{
    // Transform coordinates from viewport to contents.
    // ??? Can we push this further down into the tools?  Make them call
    //     viewportToContents() on their own.
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());

    // ??? Shouldn't SegmentPencil be responsible for this?
    m_pencilOverExisting = ((ce.modifiers() & (Qt::AltModifier + Qt::ControlModifier)) != 0);

    switch (ce.button()) {
    case Qt::LeftButton:
    case Qt::MidButton:
        startAutoScroll();

        if (m_currentTool)
            m_currentTool->handleMouseButtonPress(&ce);
        else
            RG_DEBUG << "CompositionView::mousePressEvent() :" << this << " no tool";
        break;
    case Qt::RightButton:  // ??? Why separate handling of the right button?
        if (m_currentTool)
            m_currentTool->handleRightButtonPress(&ce);
        else
            RG_DEBUG << "CompositionView::mousePressEvent() :" << this << " no tool";
        break;
    case Qt::MouseButtonMask:  // ??? Why drop anything?  Let the tool decide on its own.
    case Qt::NoButton:
    case Qt::XButton1:
    case Qt::XButton2:
    default:
        break;
    }

    // Transfer accept state to original event.
    if (!ce.isAccepted())
        e->ignore();
}

void CompositionView::mouseReleaseEvent(QMouseEvent *e)
{
    //RG_DEBUG << "mouseReleaseEvent()";

    slotStopAutoScroll();

    if (!m_currentTool)
        return ;

    // Transform coordinates from viewport to contents.
    // ??? Can we push this further down into the tools?
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());

    if (ce.button() == Qt::LeftButton ||
        ce.button() == Qt::MidButton )
        m_currentTool->handleMouseButtonRelease(&ce);

    // Transfer accept state to original event.
    if (!ce.isAccepted())
        e->ignore();
}

void CompositionView::mouseDoubleClickEvent(QMouseEvent *e)
{
    const QPoint contentsPos = viewportToContents(e->pos());

    CompositionItemPtr item = m_model->getFirstItemAt(contentsPos);

    if (!item) {
        RG_DEBUG << "mouseDoubleClickEvent(): no item";

        const RulerScale *ruler = grid().getRulerScale();
        if (ruler)
            emit setPointerPosition(ruler->getTimeForX(contentsPos.x()));

        return;
    }

    RG_DEBUG << "mouseDoubleClickEvent(): have item";

    if (item->isRepeating()) {
        const timeT time = m_model->getRepeatTimeAt(contentsPos, item);

        RG_DEBUG << "mouseDoubleClickEvent(): editRepeat at time " << time;

        if (time > 0)
            emit editRepeat(item->getSegment(), time);
        else
            emit editSegment(item->getSegment());

    } else {

        emit editSegment(item->getSegment());
    }
}

void CompositionView::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_currentTool)
        return ;

    // Transform coordinates from viewport to contents.
    // ??? Can we push this further down into the tools?
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());

    m_pencilOverExisting = ((ce.modifiers() & Qt::AltModifier) != 0);

    int follow = m_currentTool->handleMouseMove(&ce);
    setFollowMode(follow);

    if (follow != RosegardenScrollView::NoFollow) {
        doAutoScroll();

//&& JAS - Deactivate auto expand feature when resizing / moving segments past
//&& Compostion end.  Though current code works, this creates lots of corner
//&& cases that are not reversible using the REDO / UNDO commands.
//&& Additionally, this makes accidentally altering the compostion length too easy.
//&& Currently leaving code here until a full debate is complete.

//        if (follow & RosegardenScrollView::FollowHorizontal) {
//            int mouseX = ce.pos().x();
//            // enlarge composition if needed
//            if ((horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) &&
//               // This code minimizes the chances of auto expand when moving segments
//               // Not a perfect fix -- but fixes several auto expand errors
//               (mouseX > (contentsX() + viewport()->width() * 0.95))) {
//                resizeContents(contentsWidth() + m_stepSize, contentsHeight());
//                setContentsPos(contentsX() + m_stepSize, contentsY());
//                m_model->setLength(contentsWidth());
//                slotUpdateSize();
//            }
//        }

    }

    // Transfer accept state to original event.
    if (!ce.isAccepted())
        e->ignore();
}

void CompositionView::setPointerPos(int pos)
{
    //RG_DEBUG << "CompositionView::setPointerPos(" << pos << ")\n";
    int oldPos = m_pointerPos;
    if (oldPos == pos) return;

    m_pointerPos = pos;
    m_model->pointerPosChanged(pos);

    // automagically grow contents width if pointer position goes beyond right end
    //
    // ??? m_stepSize is always 0.
    if (pos >= (contentsWidth() - m_stepSize)) {
        resizeContents(pos + m_stepSize, contentsHeight());
        // grow composition too, if needed (it may not be the case if
        if (m_model->getCompositionLength() < contentsWidth())
            m_model->setCompositionLength(contentsWidth());
    }


    // interesting -- isAutoScrolling() never seems to return true?
    //RG_DEBUG << "CompositionView::setPointerPos(" << pos << "), isAutoScrolling " << isAutoScrolling() << ", contentsX " << contentsX() << ", m_lastPointerRefreshX " << m_lastPointerRefreshX << ", contentsHeight " << contentsHeight() << endl;

    if (contentsX() != m_lastPointerRefreshX) {
        m_lastPointerRefreshX = contentsX();
        // We'll need to shift the whole canvas anyway, so
        slotUpdateArtifacts();
        return ;
    }

    int deltaW = abs(m_pointerPos - oldPos);

    if (deltaW <= m_pointerPen.width() * 2) { // use one rect instead of two separate ones

        QRect updateRect
            (std::min(m_pointerPos, oldPos) - m_pointerPen.width(), 0,
             deltaW + m_pointerPen.width() * 2, contentsHeight());

        updateArtifacts(updateRect);

    } else {

        updateArtifacts
            (QRect(m_pointerPos - m_pointerPen.width(), 0,
                   m_pointerPen.width() * 2, contentsHeight()));

        updateArtifacts
            (QRect(oldPos - m_pointerPen.width(), 0,
                   m_pointerPen.width() * 2, contentsHeight()));
    }
}

void CompositionView::setGuidesPos(int x, int y)
{
    m_guideX = x;
    m_guideY = y;
    slotUpdateArtifacts();
}

#if 0
void CompositionView::setGuidesPos(const QPoint& p)
{
    m_guideX = p.x();
    m_guideY = p.y();
    slotUpdateArtifacts();
}
#endif

void CompositionView::setDrawGuides(bool d)
{
    m_drawGuides = d;
    slotUpdateArtifacts();
}

void CompositionView::setTmpRect(const QRect& r)
{
    setTmpRect(r, m_tmpRectFill);
}

void CompositionView::setTmpRect(const QRect& r, const QColor &c)
{
    QRect pRect = m_tmpRect;
    m_tmpRect = r;
    m_tmpRectFill = c;
    slotAllNeedRefresh(m_tmpRect | pRect);
}

void CompositionView::setTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;
    slotUpdateArtifacts();

    // most of the time when the floating text is drawn
    // we want to update a larger part of the view
    // so don't update here
    //     QRect r = fontMetrics().boundingRect(x, y, 300, 40, AlignLeft, m_textFloatText);
    //     slotAllNeedRefresh(r);


    //    mainWindow->slotSetStatusMessage(text);
}

#if 0
void CompositionView::setPencilOverExisting(bool value)
{
    m_pencilOverExisting = value;
}
#endif
#if 0
// Dead Code.
void
CompositionView::slotTextFloatTimeout()
{
    hideTextFloat();
    slotUpdateArtifacts();
    //    mainWindow->slotSetStatusMessage(QString::null);
}
#endif

}
#include "CompositionView.moc"
