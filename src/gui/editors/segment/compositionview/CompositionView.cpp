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

#define RG_MODULE_STRING "[CompositionView]"
#define RG_NO_DEBUG_PRINT

#include "CompositionView.h"

#include "misc/Debug.h"
#include "AudioPeaksThread.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "base/Profiler.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "CompositionColourCache.h"
#include "ChangingSegment.h"
#include "SegmentRect.h"
#include "AudioPreviewPainter.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/IconLoader.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentSelector.h"
#include "SegmentToolBox.h"
#include "sound/Midi.h"
#include "gui/general/ThornStyle.h"
#include "misc/Preferences.h"


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


static QColor getPointerColor()
{
    if (Rosegarden::Preferences::getTheme() ==
            Rosegarden::Preferences::DarkTheme)
        return QColor(0x87, 0xcd, 0xee);
    else
        return QColor(Qt::darkBlue);
}

CompositionView::CompositionView(RosegardenDocument *doc,
                                 CompositionModelImpl *model,
                                 QWidget *parent) :
    RosegardenScrollView(parent),
    m_model(model),
    m_lastContentsX(0),
    m_lastContentsY(0),
    m_segmentsRefresh(0, 0, viewport()->width(), viewport()->height()),
    //m_backgroundPixmap(),
    //m_trackDividerColor(),
    m_showPreviews(false),
    m_showSegmentLabels(true),
    m_segmentsLayer(viewport()->width(), viewport()->height()),
    //m_audioPreview(),
    //m_notationPreview(),
    //m_updateTimer(),
    m_deleteAudioPreviewsNeeded(false),
    m_updateNeeded(false),
    //m_updateRect()
    m_drawTextFloat(false),
    //m_textFloatText(),
    //m_textFloatPos(),
    m_pointerPos(0),
    m_pointerPen(getPointerColor(), 4),
    //m_newSegmentRect(),
    //m_newSegmentColor(),
    //m_splitLinePos(),
    m_drawGuides(false),
    m_guideColor(GUIPalette::getColour(GUIPalette::MovementGuide)),
    m_guideX(0),
    m_guideY(0),
    m_drawSelectionRect(false),
    //m_selectionRect(),
    m_toolBox(new SegmentToolBox(this, doc)),
    m_currentTool(nullptr),
    //m_toolContextHelp(),
    m_contextHelpShown(false),
    m_enableDrawing(true),
    m_modeTextChanged(false),
    m_modeTextWidth(0)
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

        if (Preferences::getTheme() == Preferences::DarkTheme)
            m_backgroundPixmap = IconLoader::loadPixmap("bg-paper-black");
        else
            m_backgroundPixmap = IconLoader::loadPixmap("bg-segmentcanvas");
    }

    slotUpdateSize();

    // *** Connections

    connect(m_toolBox, &BaseToolBox::showContextHelp,
            this, &CompositionView::slotToolHelpChanged);

    connect(m_model, SIGNAL(needUpdate()),
            this, SLOT(slotUpdateAll()));
    connect(m_model, SIGNAL(needUpdate(const QRect&)),
            this, SLOT(slotAllNeedRefresh(const QRect&)));
    connect(m_model, &CompositionModelImpl::needArtifactsUpdate,
            this, &CompositionView::slotUpdateArtifacts);
    connect(m_model, &CompositionModelImpl::needSizeUpdate,
            this, &CompositionView::slotUpdateSize);

    connect(doc, &RosegardenDocument::docColoursChanged,
            this, &CompositionView::slotRefreshColourCache);

    // recording-related signals
    connect(doc, &RosegardenDocument::newMIDIRecordingSegment,
            this, &CompositionView::slotNewMIDIRecordingSegment);
    connect(doc, &RosegardenDocument::newAudioRecordingSegment,
            this, &CompositionView::slotNewAudioRecordingSegment);
    connect(doc, &RosegardenDocument::stoppedAudioRecording,
            this, &CompositionView::slotStoppedRecording);
    connect(doc, &RosegardenDocument::stoppedMIDIRecording,
            this, &CompositionView::slotStoppedRecording);
    connect(doc, &RosegardenDocument::audioFileFinalized,
            m_model, &CompositionModelImpl::slotAudioFileFinalized);

    // Connect for high-frequency control change notifications.
    connect(Instrument::getStaticSignals().data(),
                &InstrumentStaticSignals::controlChange,
            this, &CompositionView::slotControlChange);

    // Audio Preview Thread
    m_model->setAudioPeaksThread(&doc->getAudioPeaksThread());
    doc->getAudioPeaksThread().setEmptyQueueListener(this);

    // Update timer
    connect(&m_updateTimer, &QTimer::timeout, this, &CompositionView::slotUpdateTimer);
    m_updateTimer.start(100);

    // Init the halo offsets table.
    m_haloOffsets.push_back(QPoint(-1,-1));
    m_haloOffsets.push_back(QPoint(-1, 0));
    m_haloOffsets.push_back(QPoint(-1,+1));
    m_haloOffsets.push_back(QPoint( 0,-1));
    m_haloOffsets.push_back(QPoint( 0,+1));
    m_haloOffsets.push_back(QPoint(+1,-1));
    m_haloOffsets.push_back(QPoint(+1, 0));
    m_haloOffsets.push_back(QPoint(+1,+1));

    // The various tools expect this.
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    if (Preferences::getTheme() == Preferences::DarkTheme)
        m_trackDividerColor.setRgb(48, 48, 48);
    else
        m_trackDividerColor = GUIPalette::getColour(GUIPalette::TrackDivider);


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
        m_model->setAudioPeaksThread(nullptr);
    }
}

void CompositionView::slotUpdateSize()
{
    const int height =
            std::max(m_model->getCompositionHeight(), viewport()->height());

    const RulerScale *rulerScale = grid().getRulerScale();
    const int compositionWidth = (int)ceil(rulerScale->getTotalWidth());
    const int minWidth = sizeHint().width();
    const int width = std::max(compositionWidth, minWidth);

    // If the width or height need to change...
    if (contentsWidth() != width  ||  contentsHeight() != height)
        resizeContents(width, height);
}

void CompositionView::drawSelectionRectPos1(const QPoint &pos)
{
    m_drawSelectionRect = true;

    // Update the selection rect used for drawing the rubber band.
    m_selectionRect.setRect(pos.x(), pos.y(), 0, 0);
    // Pass on to CompositionModelImpl which will adjust the selected
    // segments and redraw them.
    m_model->setSelectionRect(m_selectionRect);
}

void CompositionView::drawSelectionRectPos2(const QPoint &pos)
{
    m_drawSelectionRect = true;

    // Update the selection rect used for drawing the rubber band.
    m_selectionRect.setBottomRight(pos);

    // Pass on to CompositionModelImpl which will adjust the selected
    // segments and redraw them.
    m_model->setSelectionRect(m_selectionRect);
}

void CompositionView::hideSelectionRect()
{
    // If it's already hidden, bail.
    if (!m_drawSelectionRect)
        return;

    m_drawSelectionRect = false;

    // Redraw the selection rect.
    slotUpdateArtifacts();
}

void CompositionView::deleteCachedPreviews()
{
    m_model->deleteCachedPreviews();
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
        QMessageBox::critical(nullptr, tr("Rosegarden"), QString("CompositionView::setTool() : unknown tool name %1").arg(toolName));
        return;
    }

    m_currentTool->ready();
}

void CompositionView::selectSegments(const SegmentSelection &segments)
{
    m_model->selectSegments(segments);
}

void CompositionView::drawSplitLine(int x, int y)
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

void CompositionView::setModeText(const QString& modeText)
{
    if (modeText == m_modeText) return;
    RG_DEBUG << "setModeText" << modeText;
    m_modeText = modeText;
    m_modeTextChanged = true;
    updateAll();
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
    Profiler profiler("CompositionView::slotUpdateAll()");

    // ??? This routine gets hit really hard when recording.
    //     Just holding down a single note results in 50 calls
    //     per second.

    // Redraw the segments and artifacts.

#if 1
    // Since we might be reacting to a user change (e.g. zoom), we
    // need to react immediately.
    updateAll();
#else
    QRect viewportContentsRect(
            contentsX(), contentsY(),
            viewport()->rect().width(), viewport()->rect().height());

    // Redraw the segments and artifacts.
    // Uses 55% less CPU than updateAll() when recording.  But introduces a
    // delay of up to 1/10 second for user interaction like zoom.  We need to
    // untangle user updates and automatic updates (like recording) so that we
    // can treat them differently.
    slotAllNeedRefresh(viewportContentsRect);
#endif
}

void CompositionView::slotUpdateTimer()
{
    if (m_deleteAudioPreviewsNeeded) {
        m_model->deleteCachedAudioPreviews();
        m_deleteAudioPreviewsNeeded = false;
    }

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

void CompositionView::updateAll()
{
    segmentsNeedRefresh();
    slotUpdateArtifacts();
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
    updateAll();
}

void CompositionView::slotNewMIDIRecordingSegment(Segment *s)
{
    m_model->addRecordingItem(ChangingSegmentPtr(
            new ChangingSegment(*s, SegmentRect())));
}

void CompositionView::slotNewAudioRecordingSegment(Segment *s)
{
    m_model->addRecordingItem(ChangingSegmentPtr(
            new ChangingSegment(*s, SegmentRect())));
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
        if (Preferences::getTheme() == Preferences::DarkTheme)
            segmentsLayerPainter.fillRect(clipRect, Qt::black);
        else
            segmentsLayerPainter.eraseRect(clipRect);
    }

    // *** Draw the track dividers

    drawTrackDividers(&segmentsLayerPainter, clipRect);

    // *** Get Segment and Preview Rectangles

    // Assume we aren't going to show previews.
    CompositionModelImpl::NotationPreviewRanges *notationPreview = nullptr;
    CompositionModelImpl::AudioPreviews *audioPreview = nullptr;

    if (m_showPreviews) {
        // Clear the previews.
        // ??? Move this clearing into CompositionModelImpl::getSegmentRects()?
        m_notationPreview.clear();
        m_audioPreview.clear();

        // Indicate that we want previews.
        notationPreview = &m_notationPreview;
        audioPreview = &m_audioPreview;
    }

    CompositionModelImpl::SegmentRects segmentRects;

    // Fetch segment rectangles and (optionally) previews
    m_model->getSegmentRects(clipRect, &segmentRects, notationPreview, audioPreview);

    // *** Draw Segment Rectangles

    // For each segment rectangle, draw it
    for (CompositionModelImpl::SegmentRects::const_iterator i = segmentRects.begin();
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

        // For each segment's preview range
        for (CompositionModelImpl::NotationPreviewRanges::const_iterator notationPreviewIter =
                 m_notationPreview.begin();
             notationPreviewIter != m_notationPreview.end();
             ++notationPreviewIter) {

            const CompositionModelImpl::NotationPreviewRange &notationPreviewRange =
                    *notationPreviewIter;

            QColor color = notationPreviewRange.color.isValid() ?
                           notationPreviewRange.color : defaultColor;

            // translate() calls are cumulative, so we need to be able to get
            // back to where we were.  Note that resetTransform() would be
            // too extreme as it would reverse the contents translation that
            // is present in segmentsLayerPainter at this point in time.
            segmentsLayerPainter.save();
            // Adjust the coordinate system to account for any segment
            // move offset and the vertical position of the segment.
            segmentsLayerPainter.translate(
                    notationPreviewRange.moveXOffset,
                    notationPreviewRange.segmentTop);

            // For each event rectangle, draw it.
            for (CompositionModelImpl::NotationPreview::const_iterator i =
                     notationPreviewRange.begin;
                 i != notationPreviewRange.end;
                 ++i) {

                QRect eventRect = *i;
                // Make the rect thicker vertically to match the old
                // appearance.  Without this, the rect is thin, which gives
                // slightly more information.
                // Also make the rect longer to close the gaps between the
                // events.  This is in keeping with the old appearance.
                eventRect.adjust(0,0,1,1);

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
        for (CompositionModelImpl::SegmentRects::const_iterator i = segmentRects.begin();
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

    // Draw the Playback Position Pointer.
    viewportPainter.setPen(m_pointerPen);
    viewportPainter.drawLine(m_pointerPos, 0, m_pointerPos, contentsHeight() - 1);

    //
    // New Segment (SegmentPencil)
    //
    if (m_newSegmentRect.isValid()  &&
        m_newSegmentRect.intersects(viewportContentsRect)) {

        viewportPainter.setPen(CompositionColourCache::getInstance()->SegmentBorder);
        viewportPainter.setBrush(m_newSegmentColor);
        drawRect(&viewportPainter, viewportContentsRect, m_newSegmentRect);
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
        if (m_modeTextChanged) {
            // get the new width
            QFontMetrics fm(viewportPainter.font());
            m_modeTextWidth = fm.horizontalAdvance(m_modeText);
            m_modeTextChanged = false;
        }
        viewportPainter.drawText(m_guideX - m_modeTextWidth - 5,
                                 m_guideY - 5,
                                 m_modeText);
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

void CompositionView::drawTrackDividers(
        QPainter *segmentsLayerPainter, const QRect &clipRect)
{
    // Fetch track Y coordinates within the clip rectangle.  We expand the
    // clip rectangle slightly because we are drawing a rather wide track
    // divider, so we need enough divider coords to do the drawing even
    // though the center of the divider might be slightly outside of the
    // viewport.
    CompositionModelImpl::YCoordVector trackYCoords =
            m_model->getTrackYCoords(clipRect.adjusted(0,-1,0,+1));

    // Nothing to do?  Bail.
    if (trackYCoords.empty())
        return;

    const int left = clipRect.left();
    const int right = clipRect.right();

    segmentsLayerPainter->save();

    // For each track Y coordinate
    for (CompositionModelImpl::YCoordVector::const_iterator yi =
             trackYCoords.begin();
         yi != trackYCoords.end();
         ++yi) {

        int y = *yi - 2;
        segmentsLayerPainter->setPen(m_trackDividerColor);
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor.lighter());
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor.lighter());
        segmentsLayerPainter->drawLine(left, y, right, y);

        ++y;
        segmentsLayerPainter->setPen(m_trackDividerColor);
        segmentsLayerPainter->drawLine(left, y, right, y);
    }

    segmentsLayerPainter->restore();
}

void CompositionView::drawImage(
        QPainter *painter,
        QPoint dest, const CompositionModelImpl::QImageVector &tileVector,
        QRect source)
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

void CompositionView::drawAudioPreviews(
        QPainter *segmentsLayerPainter, const QRect &clipRect)
{
    Profiler profiler("CompositionView::drawAudioPreviews");

    // for each audio preview
    for (CompositionModelImpl::AudioPreviews::const_iterator audioPreviewIter = m_audioPreview.begin();
         audioPreviewIter != m_audioPreview.end();
         ++audioPreviewIter) {

        const CompositionModelImpl::AudioPreview &audioPreviewDrawDataItem = *audioPreviewIter;

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
                  audioPreviewDrawDataItem.image,
                  sourceRect);
    }
}

void CompositionView::drawCompRect(
        QPainter *painter,
        const QRect &clipRect,
        const SegmentRect &rect,
        int intersectLvl)
{
    // Non repeating case, just draw the segment rect.
    if (!rect.isRepeating()) {
        painter->save();

        painter->setBrush(rect.brush);
        painter->setPen(rect.pen);
        drawRect(painter, clipRect, rect.rect, rect.selected, intersectLvl);

        painter->restore();

        return;
    }

    // Repeating case.

    painter->save();

    // *** Base Segment

    QRect baseRect = rect.rect;
    baseRect.setWidth(rect.baseWidth);

    painter->setPen(rect.pen);
    painter->setBrush(rect.brush);
    drawRect(painter, clipRect, baseRect, rect.selected, intersectLvl);

    // *** Repeat Area

    // ??? COPY.
    SegmentRect::RepeatMarks repeatMarksX = rect.repeatMarks;
    QRect repeatRect = rect.rect;
    repeatRect.setLeft(repeatMarksX[0]);

    // The repeat area is lighter.
    QBrush repeatBrush(rect.brush.color().lighter(150));

    painter->setBrush(repeatBrush);
    drawRect(painter, clipRect, repeatRect, rect.selected, intersectLvl);

    // *** Repeat Marks

    painter->setPen(CompositionColourCache::getInstance()->RepeatSegmentBorder);

    // For each repeat mark, draw it.
    for (size_t i = 0; i < repeatMarksX.size(); ++i) {
        int x = repeatMarksX[i];
        painter->drawLine(x, rect.rect.top(), x, rect.rect.bottom());
    }

    painter->restore();
}

void CompositionView::drawCompRectLabel(
        QPainter *painter, const SegmentRect &rect)
{
    // No label?  Bail.
    if (rect.label.isEmpty())
        return;

    painter->save();

    // Pick a font that will fit nicely within a segment rect.
    QFont font;
    font.setPixelSize(rect.rect.height() / 2.2);
    font.setWeight(QFont::Bold);
    painter->setFont(font);

    QRect labelRect = rect.rect;
    // Add a one character left margin.  Add a one pixel top margin to
    // make the text look a little more centered vertically.
    labelRect.adjust(painter->fontMetrics().boundingRect('x').width(), 1, 0, 0);

    QColor backgroundColor = rect.brush.color();

    // *** Draw the halo

    painter->setPen(backgroundColor);

    // For each halo offset, draw the text
    for (unsigned i = 0; i < m_haloOffsets.size(); ++i) {
        painter->drawText(labelRect.translated(m_haloOffsets[i]),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          rect.label);
    }

    // *** Draw the text

    const bool lightBackground = (qGray(backgroundColor.rgb()) >= 127);
    // Based on the background, pick a contrasting pen.
    painter->setPen(lightBackground ? Qt::black : Qt::white);
    painter->drawText(labelRect,
                      Qt::AlignLeft | Qt::AlignVCenter, rect.label);

    painter->restore();
}

void CompositionView::drawRect(QPainter *painter, const QRect &clipRect,
        const QRect &rect, bool isSelected, int intersectLvl)
{
    // If the rect isn't in the clip rect, bail.
    if (!rect.intersects(clipRect))
        return;

    painter->save();

    // Since we do partial updates when scrolling, make sure we don't
    // obliterate the previews.
    painter->setClipRect(clipRect);

    // For a selected segment, go with a darker fill.
    if (isSelected) {
        QColor fillColor = painter->brush().color().darker(200);
        painter->setBrush(QBrush(fillColor));
    }

    // For intersecting segments, go with a darker fill.
    if (intersectLvl > 0) {
        QColor fillColor = painter->brush().color().darker(intersectLvl * 105);
        painter->setBrush(QBrush(fillColor));
    }

    QRect rect2 = rect;
    // Shrink height by 1 to accommodate the dividers.
    // Shrink width by 1 so that adjacent segment borders don't overlap.
    // ??? Why isn't the SegmentRect already adjusted like this?
    rect2.adjust(0, 0, -1, -1);

    painter->drawRect(rect2);

    painter->restore();
}

// Functor to just compare the SegmentRect's QRect's.
class CompareSegmentRects
{
public:
    explicit CompareSegmentRects(const SegmentRect &sr) : r(sr.rect) { }
    bool operator()(const SegmentRect &sr) const
            { return (sr.rect == r); }
private:
    QRect r;
};

void CompositionView::drawIntersections(
        QPainter *painter, const QRect &clipRect,
        const CompositionModelImpl::SegmentRects &rects)
{
    // Intersections are most noticeable when recording over existing
    // segments.  They also play a part when moving a segment over top
    // of existing segments.

    // If there aren't enough rects for there to be an intersection, bail.
    if (rects.size() <= 1)
        return;

    CompositionModelImpl::SegmentRects intersections;

    // For each rect
    for (CompositionModelImpl::SegmentRects::const_iterator i = rects.begin();
         i != rects.end();
         ++i) {

        // For each rect after i
        for (CompositionModelImpl::SegmentRects::const_iterator j = i + 1;
             j != rects.end();
             ++j) {

            SegmentRect intersectRect = i->intersected(*j);

            // If no intersection, try the next rect.
            if (intersectRect.rect.isEmpty())
                continue;

            // Check if we've already encountered this intersection.
            CompositionModelImpl::SegmentRects::iterator t =
                    std::find_if(intersections.begin(),
                              intersections.end(),
                              CompareSegmentRects(intersectRect));

            // If we've already seen this intersection, try the next rect.
            if (t != intersections.end())
                continue;

            // Add it to the intersections vector.
            intersections.push_back(intersectRect);
        }
    }

    // For each intersection, draw the rectangle
    for (CompositionModelImpl::SegmentRects::iterator i =
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
        for (CompositionModelImpl::SegmentRects::iterator intersectionIter =
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

        CompositionModelImpl::SegmentRects intersections2;

        // For each intersection rect
        for (CompositionModelImpl::SegmentRects::iterator j =
                 intersections.begin();
             j != intersections.end();
             ++j) {

            const SegmentRect &testRect = *j;

            // For each rect after j
            for (CompositionModelImpl::SegmentRects::iterator i = j + 1;
                 i != intersections.end();
                 ++i) {

                SegmentRect intersectRect = testRect.intersected(*i);

                // If we have an intersection, and it isn't simply the
                // "i" rect.
                // ??? Why?  If the i rect is contained within the j
                //     rect, we ignore it?
                if (!intersectRect.isEmpty()  &&  intersectRect != *i) {
                    // Check to see if we've found this intersection already.
                    CompositionModelImpl::SegmentRects::iterator t =
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

void CompositionView::drawTextFloat(QPainter *painter)
{
    if (!m_model)
        return;

    // Find out how big of a rect we need for the text.
    QRect boundingRect = painter->boundingRect(
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

    painter->save();

    painter->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);
    painter->setBrush(CompositionColourCache::getInstance()->RotaryFloatBackground);
    painter->drawRect(boundingRect);
    painter->drawText(boundingRect, Qt::AlignCenter, m_textFloatText);

    painter->restore();
}

bool CompositionView::event(QEvent *e)
{
    if (e->type() == AudioPeaksThread::AudioPeaksQueueEmpty) {
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

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
void CompositionView::enterEvent(QEnterEvent *)
#else
void CompositionView::enterEvent(QEvent *)
#endif
{
    // Ask RosegardenMainWindow to display the context help in the status bar.
    emit showContextHelp(m_toolContextHelp);
    m_contextHelpShown = true;
    // So we can get shift/ctrl/alt key presses.
    setFocus();
}

void CompositionView::leaveEvent(QEvent *)
{
    // Ask RosegardenMainWindow to clear the context help in the status bar.
    emit showContextHelp("");
    m_contextHelpShown = false;
    clearFocus();
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
    // The left and middle buttons can be used for dragging out new
    // segments with SegmentSelector and SegmentPencil.  We want to
    // auto-scroll in those cases and others.
    // ??? Would it be better to push this down into the tools?
    if (e->button() == Qt::LeftButton  ||
        e->button() == Qt::MiddleButton)
        startAutoScroll();

    // Delegate to current tool
    if (m_currentTool)
        m_currentTool->mousePressEvent(e);
}

void CompositionView::mouseReleaseEvent(QMouseEvent *e)
{
    // In case there is no tool, and auto scroll is running.
    stopAutoScroll();

    if (m_currentTool)
        m_currentTool->mouseReleaseEvent(e);;
}

void CompositionView::mouseDoubleClickEvent(QMouseEvent *e)
{
    const QPoint contentsPos = viewportToContents(e->pos());

    ChangingSegmentPtr item = m_model->getSegmentAt(contentsPos);

    // If the user clicked on a space where there is no segment,
    // move the playback position pointer to that time.
    if (!item) {
        const RulerScale *ruler = grid().getRulerScale();
        if (ruler)
            emit setPointerPosition(ruler->getTimeForX(contentsPos.x()));

        return;
    }

    // Ask RosegardenMainViewWidget to launch the default editor for
    // the segment under the mouse pointer.

    if (item->isRepeating()) {
        const timeT time = item->getRepeatTimeAt(grid(), contentsPos);

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
        return;

    // Delegate to the current tool.
    int followMode = m_currentTool->mouseMoveEvent(e);

    // ??? Can we push the rest of this down into the tools?

    setFollowMode(followMode);
}

void CompositionView::keyPressEvent(QKeyEvent *e)
{
    // Let the baseclass have first dibs.
    RosegardenScrollView::keyPressEvent(e);

    if (!m_currentTool)
        return;

    // Delegate to the current tool.
    m_currentTool->keyPressEvent(e);
}

void CompositionView::keyReleaseEvent(QKeyEvent *e)
{
    // Let the baseclass have first dibs.
    RosegardenScrollView::keyReleaseEvent(e);

    if (!m_currentTool)
        return;

    // Delegate to the current tool.
    m_currentTool->keyReleaseEvent(e);
}

void CompositionView::drawPointer(int pos)
{
    // If we've not moved, bail.
    if (m_pointerPos == pos)
        return;

    Profiler profiler("CompositionView::drawPointer()");

    const int oldPos = m_pointerPos;
    m_pointerPos = pos;

    m_model->pointerPosChanged(pos);

    int deltaPos = abs(m_pointerPos - oldPos);

    // If the pointer has only moved slightly
    if (deltaPos <= m_pointerPen.width() * 2) {

        // Use one update rect instead of two.

        updateArtifacts(
                QRect(std::min(m_pointerPos, oldPos) - m_pointerPen.width()/2, 0,
                      deltaPos + m_pointerPen.width(), contentsHeight()));
    } else {

        // Update the new pointer position.
        updateArtifacts(
                QRect(m_pointerPos - m_pointerPen.width()/2, 0,
                      m_pointerPen.width(), contentsHeight()));

        // Update the old pointer position.
        updateArtifacts(
                QRect(oldPos - m_pointerPen.width()/2, 0,
                      m_pointerPen.width(), contentsHeight()));
    }
}

void CompositionView::drawGuides(int x, int y)
{
    m_drawGuides = true;
    m_guideX = x;
    m_guideY = y;

    slotUpdateArtifacts();
}

void CompositionView::hideGuides()
{
    m_drawGuides = false;

    slotUpdateArtifacts();
}

void CompositionView::drawNewSegment(const QRect &r)
{
    QRect previousRect = m_newSegmentRect;
    m_newSegmentRect = r;

    slotAllNeedRefresh(m_newSegmentRect | previousRect);
}

void CompositionView::drawTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;

    slotUpdateArtifacts();
}

void CompositionView::slotControlChange(Instrument *instrument, int cc)
{
    // If an audio instrument's volume or pan is changed, we need to redraw
    // the previews since the audio previews show the effects of volume and
    // pan.

    // This approach is a bit heavy-handed.  Even if the relevant audio
    // segment isn't visible, we still force an update.  This is simple.
    // Making it smarter probably isn't worth the time or the code.

    if (instrument->getType() != Instrument::Audio)
        return;
    if (cc != MIDI_CONTROLLER_VOLUME  &&  cc != MIDI_CONTROLLER_PAN)
        return;

    // Signal that the audio previews need to be deleted on the next timer.
    m_deleteAudioPreviewsNeeded = true;

    // The entire viewport in contents coords.
    // ??? This is copied all over.  Factor into a getViewportContentsRect().
    QRect viewportContentsRect(
            contentsX(), contentsY(),
            viewport()->rect().width(), viewport()->rect().height());

    // Signal that a refresh is needed on the next timer.
    slotAllNeedRefresh(viewportContentsRect);
}

void CompositionView::makeTrackPosVisible(int trackPos)
{
    if (!m_model)
        return;

    // Figure out where it is.
    const int y = m_model->grid().getYBinCoordinate(trackPos);

    // Make it visible.
    scrollVert(y);
}


}
