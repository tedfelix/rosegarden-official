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

    // Init the halo offsets table.
    m_haloOffsets.push_back(QPoint(-1,-1));
    m_haloOffsets.push_back(QPoint(-1, 0));
    m_haloOffsets.push_back(QPoint(-1,+1));
    m_haloOffsets.push_back(QPoint( 0,-1));
    m_haloOffsets.push_back(QPoint( 0,+1));
    m_haloOffsets.push_back(QPoint(+1,-1));
    m_haloOffsets.push_back(QPoint(+1, 0));
    m_haloOffsets.push_back(QPoint(+1,+1));

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

        drawCompRect(&segmentsLayerPainter, clipRect, *i);
    }

    drawIntersections(&segmentsLayerPainter, clipRect, segmentRects);

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

            drawCompRectLabel(&segmentsLayerPainter, *i);
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
    viewportPainter.setPen(m_pointerPen);
    viewportPainter.drawLine(m_pointerPos, 0, m_pointerPos, contentsHeight() - 1);

    //
    // Tmp rect (rect displayed while drawing a new segment)
    //
    if (m_tmpRect.isValid() && m_tmpRect.intersects(viewportContentsRect)) {
        viewportPainter.setPen(CompositionColourCache::getInstance()->SegmentBorder);
        viewportPainter.setBrush(m_tmpRectFill);
        drawRect(&viewportPainter, viewportContentsRect, m_tmpRect);
    }

    //
    // Tool guides (crosshairs)
    //
    if (m_drawGuides) {
        viewportPainter.setPen(m_guideColor);
        // Vertical Guide
        viewportPainter.drawLine(m_guideX, 0, m_guideX, contentsHeight() - 1);
        // Horizontal Guide
        viewportPainter.drawLine(0, m_guideY, contentsWidth() - 1, m_guideY);
    }

    //
    // Selection Rect (rubber band)
    //
    if (m_drawSelectionRect) {
        viewportPainter.save();

        viewportPainter.setPen(CompositionColourCache::getInstance()->SegmentBorder);
        viewportPainter.setBrush(Qt::NoBrush);
        viewportPainter.drawRect(m_selectionRect);

        viewportPainter.restore();
    }

    //
    // Floating Text
    //
    if (m_drawTextFloat)
        drawTextFloat(&viewportPainter);

    //
    // Split line
    //
    if (m_splitLinePos.x() > 0) {
        viewportPainter.setPen(m_guideColor);
        viewportPainter.drawLine(m_splitLinePos.x(), m_splitLinePos.y(),
                    m_splitLinePos.x(), m_splitLinePos.y() + m_model->grid().getYSnap());
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

void CompositionView::drawImage(
        QPainter *painter,
        QPoint dest, const PixmapArray &tileVector, QRect source)
{
    // ??? This is an awful lot of complexity to accommodate the tiling
    //     of the audio previews.  Why are they tiled?  Can they be
    //     untiled so that all of this would reduce to a single drawImage()?

    // No tiles?  Bail.
    if (tileVector.empty())
        return;

    int tileWidth = AudioPreviewPainter::tileWidth();

    int firstTile = source.left() / tileWidth;
    int firstTileStartX = source.left() % tileWidth;
    int lastTile = source.right() / tileWidth;
    int lastTileStopX = source.right() % tileWidth;

    if (firstTile < 0  ||  lastTile < 0)
        return;

    // Most likely, the source rect needs normalizing.
    if (lastTile < firstTile)
        return;

    // If we are starting beyond the available tiles, bail.
    if (firstTile >= (int)tileVector.size())
        return;

    // If we are ending beyond the available tiles
    if (lastTile >= (int)tileVector.size()) {
        // Stop at the last.
        lastTile = (int)tileVector.size() - 1;
        lastTileStopX = tileWidth - 1;
    }

    // Special case: Drawing from a single tile.
    if (firstTile == lastTile) {
        QRect tileSource = source;  // get top, bottom, and width
        tileSource.setLeft(source.left() - tileWidth * firstTile);
        painter->drawImage(dest, tileVector[firstTile], tileSource);
        return;
    }

    // *** First Tile

    QRect firstTileSource = source;  // get the top and bottom
    firstTileSource.setLeft(firstTileStartX);
    firstTileSource.setRight(tileWidth - 1);
    painter->drawImage(dest, tileVector[firstTile], firstTileSource);
    dest.setX(dest.x() + firstTileSource.width());

    // *** Middle Tile(s)

    int firstMiddleTile = firstTile + 1;
    int lastMiddleTile = lastTile - 1;

    // An entire tile
    QRect tileRect(source.x(), source.y(), tileWidth, source.height());

    // for each middle tile
    for (int tile = firstMiddleTile; tile <= lastMiddleTile; ++tile) {
        // draw the middle tile entirely
        painter->drawImage(dest, tileVector[tile], tileRect);
        dest.setX(dest.x() + tileRect.width());
    }

    // *** Last Tile

    QRect lastTileSource = source;  // get the top and bottom
    lastTileSource.setLeft(0);
    lastTileSource.setRight(lastTileStopX);
    painter->drawImage(dest, tileVector[lastTile], lastTileSource);
}

void CompositionView::drawAudioPreviews(QPainter *segmentsLayerPainter, const QRect &clipRect)
{
    Profiler profiler("CompositionView::drawAudioPreviews");

    // for each audio preview
    for (CompositionModelImpl::AudioPreviewDrawData::const_iterator audioPreviewIter = m_audioPreview.begin();
         audioPreviewIter != m_audioPreview.end();
         ++audioPreviewIter) {

        const CompositionModelImpl::AudioPreviewDrawDataItem &audioPreviewDrawDataItem = *audioPreviewIter;

        // If this one isn't in the clip rect, try the next.
        if (!audioPreviewDrawDataItem.rect.intersects(clipRect))
            continue;

        QPoint destPoint = audioPreviewDrawDataItem.rect.topLeft();
        QRect sourceRect = audioPreviewDrawDataItem.rect;
        // Translate contents coords to preview coords.
        sourceRect.moveTo(0,0);

        // If the beginning is being resized to the right, clip the preview
        if (audioPreviewDrawDataItem.resizeOffset > 0)
            sourceRect.setLeft(audioPreviewDrawDataItem.resizeOffset);

        // draw the preview
        drawImage(segmentsLayerPainter,
                  destPoint,
                  audioPreviewDrawDataItem.pixmap,
                  sourceRect);
    }
}

void CompositionView::drawCompRect(
        QPainter *painter,
        const QRect &clipRect,
        const CompositionRect &rect,
        int intersectLvl)
{
    // Non repeating case, just draw the segment rect.
    if (!rect.isRepeating()) {
        painter->save();

        painter->setBrush(rect.getBrush());
        painter->setPen(rect.getPen());
        drawRect(painter, clipRect, rect, rect.isSelected(), intersectLvl);

        painter->restore();

        return;
    }

    // Repeating case.

    painter->save();

    // *** Base Segment

    QRect baseRect = rect;
    baseRect.setWidth(rect.getBaseWidth());

    painter->setPen(rect.getPen());
    painter->setBrush(rect.getBrush());
    drawRect(painter, clipRect, baseRect, rect.isSelected(), intersectLvl);

    // *** Repeat Area

    CompositionRect::repeatmarks repeatMarksX = rect.getRepeatMarks();
    QRect repeatRect = rect;
    repeatRect.setLeft(repeatMarksX[0]);

    // The repeat area is lighter.
    QBrush repeatBrush(rect.getBrush().color().lighter(150));

    painter->setBrush(repeatBrush);
    drawRect(painter, clipRect, repeatRect, rect.isSelected(), intersectLvl);

    // *** Repeat Marks

    painter->setPen(CompositionColourCache::getInstance()->RepeatSegmentBorder);

    // For each repeat mark, draw it.
    for (int i = 0; i < repeatMarksX.size(); ++i) {
        int x = repeatMarksX[i];
        painter->drawLine(x, rect.top(), x, rect.bottom());
    }

    painter->restore();
}

void CompositionView::drawCompRectLabel(
        QPainter *painter, const CompositionRect &rect)
{
    // No label?  Bail.
    if (rect.getLabel().isEmpty())
        return;

    painter->save();

    // Pick a font that will fit nicely within a segment rect.
    QFont font;
    font.setPixelSize(rect.height() / 2.2);
    font.setWeight(QFont::Bold);
    painter->setFont(font);

    QRect labelRect = rect;
    // Add a one character left margin.  Add a one pixel top margin to
    // make the text look a little more centered vertically.
    labelRect.adjust(painter->fontMetrics().width('x'), 1, 0, 0);

    QColor backgroundColor = rect.getBrush().color();

    // *** Draw the halo

    painter->setPen(backgroundColor);

    // For each halo offset, draw the text
    for (unsigned i = 0; i < m_haloOffsets.size(); ++i) {
        painter->drawText(labelRect.translated(m_haloOffsets[i]),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          rect.getLabel());
    }

    // *** Draw the text

    const bool lightBackground = (qGray(backgroundColor.rgb()) >= 127);
    // Based on the background, pick a contrasting pen.
    painter->setPen(lightBackground ? Qt::black : Qt::white);
    painter->drawText(labelRect,
                      Qt::AlignLeft | Qt::AlignVCenter, rect.getLabel());

    painter->restore();
}

void CompositionView::drawRect(QPainter *p, const QRect &clipRect,
        const QRect &r, bool isSelected, int intersectLvl)
{
    // If the rect isn't in the clip rect, bail.
    if (!r.intersects(clipRect))
        return;

    p->save();

    // Since we do partial updates when scrolling, make sure we don't
    // obliterate the previews.
    p->setClipRect(clipRect);

    // For a selected segment, go with a darker fill.
    if (isSelected) {
        QColor fillColor = p->brush().color().darker(200);
        p->setBrush(QBrush(fillColor));
    }

    // For intersecting segments, go with a darker fill.
    if (intersectLvl > 0) {
        QColor fillColor = p->brush().color().darker(intersectLvl * 105);
        p->setBrush(QBrush(fillColor));
    }

    QRect rect = r;
    // Shrink height by 1 to accommodate the dividers.
    // Shrink width by 1 so that adjacent segment borders don't overlap.
    // ??? Why isn't the CompositionRect already adjusted like this?
    rect.adjust(0, 0, -1, -1);

    p->drawRect(rect);

    p->restore();
}

void CompositionView::drawIntersections(
        QPainter *painter, const QRect &clipRect,
        const CompositionModelImpl::RectContainer &rects)
{
    // Intersections are most noticeable when recording over existing
    // segments.  They also play a part when moving a segment over top
    // of existing segments.

    // If there aren't enough rects for there to be an intersection, bail.
    if (rects.size() <= 1)
        return;

    CompositionModelImpl::RectContainer intersections;

    // For each rect
    for (CompositionModelImpl::RectContainer::const_iterator i = rects.begin();
         i != rects.end();
         ++i) {

        // For each rect after i
        for (CompositionModelImpl::RectContainer::const_iterator j = i + 1;
             j != rects.end();
             ++j) {

            CompositionRect intersectRect = i->intersected(*j);

            // If no intersection, try the next rect.
            if (intersectRect.isEmpty())
                continue;

            // Check if we've already encountered this intersection.
            CompositionModelImpl::RectContainer::iterator t =
                    std::find(intersections.begin(),
                              intersections.end(),
                              intersectRect);

            // If we've already seen this intersection, try the next rect.
            if (t != intersections.end())
                continue;

            // Add it to the intersections vector.
            intersections.push_back(intersectRect);
        }
    }

    // For each intersection, draw the rectangle
    for (CompositionModelImpl::RectContainer::iterator i =
             intersections.begin();
         i != intersections.end();
         ++i) {

        drawCompRect(painter, clipRect, *i, 1);
    }

#if 0
// This code is from when segments could overlap each other on a track.
// Overlapping segments are no longer allowed.  See:
//    CompositionModelImpl::setTrackHeights()
//    Composition::getMaxContemporaneousSegmentsOnTrack()
    //
    // draw this level of intersections then compute and draw further ones
    //
    int level = 1;

    while (!intersections.empty()) {

        // For each intersection, draw the segment rectangle
        for (CompositionModelImpl::RectContainer::iterator intersectionIter =
                 intersections.begin();
             intersectionIter != intersections.end();
             ++intersectionIter) {

            drawCompRect(painter, clipRect, *intersectionIter, level);
        }

        // Bail if more than 10 intersections.
        // Original comment: "put a limit on how many intersections we can
        //                    compute and draw - this grows exponentially"
        // That doesn't seem right.  Each time through, the intersections
        // will become fewer and fewer.
        if (intersections.size() > 10)
            break;

        ++level;

        CompositionModelImpl::RectContainer intersections2;

        // For each intersection rect
        for (CompositionModelImpl::RectContainer::iterator j =
                 intersections.begin();
             j != intersections.end();
             ++j) {

            const CompositionRect &testRect = *j;

            // For each rect after j
            for (CompositionModelImpl::RectContainer::iterator i = j + 1;
                 i != intersections.end();
                 ++i) {

                CompositionRect intersectRect = testRect.intersected(*i);

                // If we have an intersection, and it isn't simply the
                // "i" rect.
                // ??? Why?  If the i rect is contained within the j
                //     rect, we ignore it?
                if (!intersectRect.isEmpty()  &&  intersectRect != *i) {
                    // Check to see if we've found this intersection already.
                    CompositionModelImpl::RectContainer::iterator t =
                            std::find(intersections2.begin(),
                                      intersections2.end(),
                                      intersectRect);

                    // If we've not seen this intersection before
                    if (t == intersections2.end()) {
                        // Set the attributes.
                        // ??? What about selected?
                        intersectRect.setBrush(
                                mixBrushes(testRect.getBrush(), i->getBrush()));
                    }

                    // ??? Add all intersections that we find, even if we've
                    //     already seen them?
                    intersections2.push_back(intersectRect);
                }
            }
        }

        intersections = intersections2;
    }
#endif
}

void CompositionView::drawTextFloat(QPainter *p)
{
    if (!m_model)
        return;

    // Find out how big of a rect we need for the text.
    QRect boundingRect = p->boundingRect(
            QRect(),  // we want the "required" rectangle
            0,        // we want the "required" rectangle
            m_textFloatText);

    // Add some margins to give the text room to breathe.
    boundingRect.adjust(-4,-2,+4,+2);

    QPoint pos(m_textFloatPos);

    // If the text float would appear above the top of the viewport
    if (pos.y() < contentsY()) {
        // Move it down
        pos.setY(pos.y() + m_model->grid().getYSnap() * 2 +
                 boundingRect.height());
    }

    boundingRect.moveTopLeft(pos);

    p->save();

    p->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);
    p->setBrush(CompositionColourCache::getInstance()->RotaryFloatBackground);
    p->drawRect(boundingRect);
    p->drawText(boundingRect, Qt::AlignCenter, m_textFloatText);

    p->restore();
}

bool CompositionView::event(QEvent *e)
{
    if (e->type() == AudioPreviewThread::AudioPreviewQueueEmpty) {
        // Audio previews have been generated, redraw the segments.
        segmentsNeedRefresh();
        viewport()->update();

        // No need to propagate to parent.
        e->accept();
        // Event was recognized.
        return true;
    }

    return RosegardenScrollView::event(e);
}

void CompositionView::enterEvent(QEvent *)
{
    // Ask RosegardenMainWindow to display the context help in the status bar.
    emit showContextHelp(m_toolContextHelp);
    m_contextHelpShown = true;
}

void CompositionView::leaveEvent(QEvent *)
{
    // Ask RosegardenMainWindow to clear the context help in the status bar.
    emit showContextHelp("");
    m_contextHelpShown = false;
}

void CompositionView::slotToolHelpChanged(const QString &text)
{
    // No change?  Bail.
    if (m_toolContextHelp == text)
        return;

    m_toolContextHelp = text;

    // If we're showing context help, ask RosegardenMainWindow to update
    // the context help in the status bar.
    if (m_contextHelpShown)
        emit showContextHelp(text);
}

void CompositionView::mousePressEvent(QMouseEvent *e)
{
    // Transform coordinates from viewport to contents.
    // ??? Can we push this further down into the tools?  Make them call
    //     viewportToContents() on their own.  I think this is a good idea,
    //     although it will be quite a bit of work.
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());

    // ??? Recommend getting rid of this switch/case.  Instead, SegmentTool
    //     derivers
    //     should provide a mousePressEvent() and all mouse press events
    //     should be forwarded to it.  The derivers also need to call the
    //     baseclass version (SegmentTool::mousePressEvent()) to make sure
    //     SegmentTool has a shot at the right mouse button.
    //if (e.button() == Qt::LeftButton)
    //    startAutoScroll();
    //if (m_currentTool)
    //    m_currentTool->mousePressEvent(e);
    switch (ce.button()) {
    case Qt::LeftButton:
    case Qt::MidButton:
        // ??? Important.  We need to keep this here.
        startAutoScroll();

        if (m_currentTool)
            m_currentTool->handleMouseButtonPress(&ce);
        else
            RG_DEBUG << "CompositionView::mousePressEvent() :" << this << " no tool";
        break;
    case Qt::RightButton:
        // ??? Why separate handling of the right button?  SegmentTool is the
        //     only one who handles this.
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

}
#include "CompositionView.moc"
