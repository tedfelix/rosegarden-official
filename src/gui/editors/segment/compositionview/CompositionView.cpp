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

#include <QMessageBox>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QVector>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
//#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QSettings>
#include <QMouseEvent>

#include <algorithm>


namespace Rosegarden
{


CompositionView::CompositionView(RosegardenDocument* doc,
                                 CompositionModelImpl* model,
                                 QWidget * parent) :
    RosegardenScrollView(parent),
    m_model(model),
    m_currentIndex(0),
    m_tool(0),
    m_toolBox(0),
    m_enableDrawing(true),
    m_showPreviews(false),
    m_showSegmentLabels(true),
    m_fineGrain(false),
    m_pencilOverExisting(false),
    //m_minWidth(m_model->getCompositionLength()),
    m_stepSize(0),
    m_rectFill(0xF0, 0xF0, 0xF0),
    m_selectedRectFill(0x00, 0x00, 0xF0),
    m_pointerPos(0),
    m_pointerColor(GUIPalette::getColour(GUIPalette::Pointer)),
    m_pointerWidth(4),
    m_pointerPen(QPen(m_pointerColor, m_pointerWidth)),
    m_tmpRect(QRect(QPoint(0, 0), QPoint( -1, -1))),
    m_tmpRectFill(CompositionRect::DefaultBrushColor),
    m_trackDividerColor(GUIPalette::getColour(GUIPalette::TrackDivider)),
    m_drawGuides(false),
    m_guideColor(GUIPalette::getColour(GUIPalette::MovementGuide)),
    m_topGuidePos(0),
    m_foreGuidePos(0),
    m_drawSelectionRect(false),
    m_drawTextFloat(false),
    m_segmentsLayer(viewport()->width(), viewport()->height()),
    m_doubleBuffer(viewport()->width(), viewport()->height()),
    m_segmentsRefresh(0, 0, viewport()->width(), viewport()->height()),
    m_artifactsRefresh(0, 0, viewport()->width(), viewport()->height()),
    m_lastBufferRefreshX(0),
    m_lastBufferRefreshY(0),
    m_lastPointerRefreshX(0),
    m_contextHelpShown(false),
    m_updateTimer(new QTimer(static_cast<QObject *>(this))),
    m_updateNeeded(false)
//  m_updateRect()
{
    if (doc) {
        m_toolBox = new SegmentToolBox(this, doc);

        connect(m_toolBox, SIGNAL(showContextHelp(const QString &)),
                this, SLOT(slotToolHelpChanged(const QString &)));
    }
    
    // From Q3ScrollView, not in Qt4's QAbstractScrollArea.
    //setDragAutoScroll(true);

// Causing slow refresh issues on RG Mian Window -- 10-12-2011 - JAS
//    viewport()->setAttribute(Qt::WA_PaintOnScreen);
//    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

    QPalette pal;
    pal.setColor(viewport()->backgroundRole(), GUIPalette::getColour(GUIPalette::SegmentCanvas));
    viewport()->setPalette(pal);

    slotUpdateSize();

    // QScrollBar* hsb = horizontalScrollBar();

    // dynamically adjust content size when scrolling past current composition's end
    //   connect(hsb, SIGNAL(nextLine()),
    //          this, SLOT(scrollRight()));
    //   connect(hsb, SIGNAL(prevLine()),
    //          this, SLOT(scrollLeft()));

    //    connect(this, SIGNAL(contentsMoving(int, int)),
    //            this, SLOT(allNeedRefresh()));

    //     connect(this, SIGNAL(contentsMoving(int, int)),
    //             this, SLOT(slotContentsMoving(int, int)));

    connect(model, SIGNAL(needContentUpdate()),
            this, SLOT(slotUpdateAll()));
    connect(model, SIGNAL(needContentUpdate(const QRect&)),
            this, SLOT(slotUpdateAll(const QRect&)));
    connect(model, SIGNAL(needArtifactsUpdate()),
            this, SLOT(slotArtifactsNeedRefresh()));
    connect(model, SIGNAL(needSizeUpdate()),
            this, SLOT(slotUpdateSize()));

    if (doc) {
        connect(doc, SIGNAL(docColoursChanged()),
                this, SLOT(slotRefreshColourCache()));

        // recording-related signals
        connect(doc, SIGNAL(newMIDIRecordingSegment(Segment*)),
                this, SLOT(slotNewMIDIRecordingSegment(Segment*)));
        connect(doc, SIGNAL(newAudioRecordingSegment(Segment*)),
                this, SLOT(slotNewAudioRecordingSegment(Segment*)));
        //     connect(doc, SIGNAL(recordMIDISegmentUpdated(Segment*, timeT)),
        //             this, SLOT(slotRecordMIDISegmentUpdated(Segment*, timeT)));
        connect(doc, SIGNAL(stoppedAudioRecording()),
                this, SLOT(slotStoppedRecording()));
        connect(doc, SIGNAL(stoppedMIDIRecording()),
                this, SLOT(slotStoppedRecording()));
        connect(doc, SIGNAL(audioFileFinalized(Segment*)),
                getModel(), SLOT(slotAudioFileFinalized(Segment*)));
    }
    
    if (model) {
        model->setAudioPreviewThread(&doc->getAudioPreviewThread());
    }

    if (doc) {
        doc->getAudioPreviewThread().setEmptyQueueListener(this);
    }

    // Update timer
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(slotUpdateTimer()));
    m_updateTimer->start(100);

    QSettings settings;
    settings.beginGroup("Performance_Testing");

    m_enableDrawing = (settings.value("CompositionView", 1).toInt() != 0);

    // Write it to the file to make it easier to find.
    settings.setValue("CompositionView", m_enableDrawing ? 1 : 0);

    settings.endGroup();

    IconLoader il;

    // If background textures are enabled.
    if (settings.value(
            QString(GeneralOptionsConfigGroup) + "/backgroundtextures",
            "true").toBool()) {

        QPixmap background = il.loadPixmap("bg-segmentcanvas");

        if (!background.isNull()) {
            setBackgroundPixmap(background);
            //viewport()->setBackgroundPixmap(background);
            QPalette palette;
            palette.setBrush(backgroundRole(), QBrush(background));
            palette.setBrush(viewport()->backgroundRole(), QBrush(background));
            setPalette(palette);
            viewport()->setPalette(palette);
        }
    }
}

void CompositionView::endAudioPreviewGeneration()
{
    if (m_model) {
        m_model->setAudioPreviewThread(0);
    }
}

void CompositionView::setBackgroundPixmap(const QPixmap &m)
{
    m_backgroundPixmap = m;
    //     viewport()->setErasePixmap(m_backgroundPixmap);
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
//    int vStep = getModel()->grid().getYSnap();
//    int height = std::max(getModel()->m_composition.getNbTracks() * vStep, (unsigned)viewport()->height());
    int height = std::max(getModel()->getCompositionHeight(), (unsigned) viewport()->height());

    const RulerScale *ruler = grid().getRulerScale();

    int minWidth = sizeHint().width();
    int computedWidth = int(nearbyint(ruler->getTotalWidth()));

    int width = std::max(computedWidth, minWidth);
    
    if (contentsWidth() != width || contentsHeight() != height) {
//        RG_DEBUG << "CompositionView::slotUpdateSize: Resizing contents from "
//                 << contentsWidth() << "x" << contentsHeight() << " to "
//                 << width << "x" << height << endl;

        resizeContents(width, height);
    }
}

#if 0
// Dead Code.
void CompositionView::scrollRight()
{
    RG_DEBUG << "CompositionView::scrollRight()\n";
    if (m_stepSize == 0)
        initStepSize();

    if (horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) {

        resizeContents(contentsWidth() + m_stepSize, contentsHeight());
        setContentsPos(contentsX() + m_stepSize, contentsY());
        getModel()->setLength(contentsWidth());
    }

}

void CompositionView::scrollLeft()
{
    RG_DEBUG << "CompositionView::scrollLeft()\n";
    if (m_stepSize == 0)
        initStepSize();

    int cWidth = contentsWidth();

    if (horizontalScrollBar()->value() < cWidth && cWidth > m_minWidth) {
        resizeContents(cWidth - m_stepSize, contentsHeight());
        getModel()->setLength(contentsWidth());
    }

}
#endif

void CompositionView::setSelectionRectPos(const QPoint& pos)
{
    //RG_DEBUG << "setSelectionRectPos(" << pos << ")";
    m_selectionRect.setRect(pos.x(), pos.y(), 0, 0);
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setSelectionRectSize(int w, int h)
{
    //RG_DEBUG << "setSelectionRectSize(" << w << "," << h << ")";
    m_selectionRect.setSize(QSize(w, h));
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setDrawSelectionRect(bool d)
{
    if (m_drawSelectionRect != d) {
        m_drawSelectionRect = d;
        slotArtifactsNeedRefresh();
        slotUpdateAll(m_selectionRect);
    }
}

void CompositionView::clearSegmentRectsCache(bool clearPreviews)
{
    getModel()->clearSegmentRectsCache(clearPreviews);
}

SegmentSelection
CompositionView::getSelectedSegments()
{
    return m_model->getSelectedSegments();
}

void CompositionView::updateSelectionContents()
{
    if (!haveSelection())
        return ;


    QRect selectionRect = getModel()->getSelectionContentsRect();
    updateContents(selectionRect);
//    update(selectionRect);
}

#if 0
void CompositionView::slotContentsMoving(int /* x */, int /* y */)
{
    //     qDebug("contents moving : x=%d", x);
}
#endif

void CompositionView::setTool(const QString& toolName)
{
    RG_DEBUG << "CompositionView::setTool(" << toolName << ")"
             << this << "\n";

    if (m_tool)
        m_tool->stow();

    m_toolContextHelp = "";

    m_tool = m_toolBox->getTool(toolName);

    if (m_tool)
        m_tool->ready();
    else {
        QMessageBox::critical(0, tr("Rosegarden"), QString("CompositionView::setTool() : unknown tool name %1").arg(toolName));
    }
}

void CompositionView::selectSegments(const SegmentSelection &segments)
{
    RG_DEBUG << "CompositionView::selectSegments\n";

    static QRect dummy;

    getModel()->clearSelected();

    for (SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) {
        getModel()->setSelected(CompositionItemPtr(new CompositionItem(**i, dummy)));
    }
    slotUpdateAll();
}

SegmentSelector*
CompositionView::getSegmentSelectorTool()
{
    return dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));
}

void CompositionView::setSelectAdd(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool)
        return ;

    selTool->setSegmentAdd(value);
}

void CompositionView::setSelectCopy(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool)
        return ;

    selTool->setSegmentCopy(value);
}

void CompositionView::setSelectCopyingAsLink(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool)
        return ;

    selTool->setSegmentCopyingAsLink(value);
}

void CompositionView::showSplitLine(int x, int y)
{
    m_splitLinePos.setX(x);
    m_splitLinePos.setY(y);
}

void CompositionView::hideSplitLine()
{
    m_splitLinePos.setX( -1);
    m_splitLinePos.setY( -1);
}

void CompositionView::slotExternalWheelEvent(QWheelEvent* e)
{
    e->accept();
    wheelEvent(e);
}

CompositionItemPtr CompositionView::getFirstItemAt(QPoint pos)
{
    CompositionModelImpl::ItemContainer items = getModel()->getItemsAt(pos);

    if (items.size()) {
        // find topmost item
        CompositionItemPtr res = *(items.begin());

        unsigned int maxZ = res->z();

        CompositionModelImpl::ItemContainer::iterator maxZItemPos = items.begin();

        for (CompositionModelImpl::ItemContainer::iterator i = items.begin();
             i != items.end(); ++i) {
            CompositionItemPtr ic = *i;
            if (ic->z() > maxZ) {
                res = ic;
                maxZ = ic->z();
                maxZItemPos = i;
            }
        }

        // get rid of the rest;
        items.erase(maxZItemPos);
        for (CompositionModelImpl::ItemContainer::iterator i = items.begin();
             i != items.end(); ++i)
            delete *i;

        return res;
    }


    return CompositionItemPtr();
}

void CompositionView::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
        grid().setSnapTime(SnapGrid::NoSnap);
    } else {
        grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}

void CompositionView::slotUpdateAll()
{
    // This one doesn't get called too often while recording.
    Profiler profiler("CompositionView::slotUpdateAll()");

    //RG_DEBUG << "CompositionView::slotUpdateAll()";
    allNeedRefresh();
    updateContents();
//    update();
}

void CompositionView::slotUpdateTimer()
{
    //RG_DEBUG << "CompositionView::slotUpdateTimer()";

    if (m_updateNeeded)
    {
        updateAll(m_updateRect);

        //m_updateRect.setRect(0,0,0,0);  // Not needed.
        m_updateNeeded = false;
    }
}

void CompositionView::updateAll(const QRect& rect)
{
    Profiler profiler("CompositionView::updateAll(const QRect& rect)");

    //RG_DEBUG << "CompositionView::updateAll() rect " << rect << " - valid : " << rect.isValid();

    allNeedRefresh(rect);

    if (rect.isValid()) {
        updateContents(rect);
//        update(rect);
    } else {
        updateContents();
//        update();
    }
}

void CompositionView::slotUpdateAll(const QRect& rect)
{
    // Bail if drawing is turned off in the settings.
    if (!m_enableDrawing)
        return;

    // This one gets hit pretty hard while recording.
    Profiler profiler("CompositionView::slotUpdateAll(const QRect& rect)");

#if 0
// Old way.  Just do the work for every update.  Very expensive.
    updateAll(rect);
#else
// Alternate approach with a timer to throttle updates

    // Note: This new approach normalizes the incoming rect.  This means
    //   that it will never trigger a full refresh given an invalid rect
    //   like it used to.  See updateAll().  Some rough
    //   testing reveals that the following test cases trigger this
    //   invalid rect situation:
    //       1. Move a segment.
    //       2. Click with the arrow tool where there is no segment.
    //   Testing of these situations reveals no (or minor) refresh issues.

    // If an update is now needed, set m_updateRect, otherwise accumulate it.
    if (!m_updateNeeded)
    {
        // Let slotUpdateTimer() know an update is needed next time.
        m_updateNeeded = true;
        m_updateRect = rect.normalized();
    }
    else
    {
        // Accumulate the update rect
        m_updateRect |= rect.normalized();
    }
#endif
}

void CompositionView::slotRefreshColourCache()
{
    CompositionColourCache::getInstance()->init();
    clearSegmentRectsCache();
    slotUpdateAll();
}

void CompositionView::slotNewMIDIRecordingSegment(Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotNewAudioRecordingSegment(Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotStoppedRecording()
{
    getModel()->clearRecordingItems();
}

void CompositionView::resizeEvent(QResizeEvent* e)
{
    if (e->size() == e->oldSize()) return;

//    RG_DEBUG << "CompositionView::resizeEvent() : from " << e->oldSize() << " to " << e->size() << endl;

    RosegardenScrollView::resizeEvent(e);
    slotUpdateSize();

    int w = std::max(m_segmentsLayer.width(), viewport()->width());
    int h = std::max(m_segmentsLayer.height(), viewport()->height());

    m_segmentsLayer = QPixmap(w, h);
    m_doubleBuffer = QPixmap(w, h);
    allNeedRefresh();

    RG_DEBUG << "CompositionView::resizeEvent() : segments layer size = " << m_segmentsLayer.size() << endl;
}

void CompositionView::paintEvent(QPaintEvent *e)
{
    Profiler profiler("CompositionView::paintEvent");

    QVector<QRect> rects = e->region().rects();

    for (int i = 0; i < rects.size(); ++i) {
        viewportPaintRect(rects[i]);
    }
}

void CompositionView::viewportPaintRect(QRect r)
{
    Profiler profiler("CompositionView::viewportPaintRect");

    QRect updateRect = r;

    // Limit the requested rect to the viewport.
    r &= viewport()->rect();
    // Convert from viewport coords to contents coords.
    r.translate(contentsX(), contentsY());

//    std::cerr << "CompositionView::viewportPaintRect updateRect = "
//              << updateRect.x() << "," << updateRect.y()
//              << " " << updateRect.width() << "x" << updateRect.height()
//              << std::endl;

    bool scroll = false;

    // Scroll and refresh the segments layer.
    bool changed = scrollSegmentsLayer(r, scroll);

    // r is now the combination of the requested refresh rect and the refresh
    // needed by any scrolling.

    if (changed || m_artifactsRefresh.isValid()) {

        QRect copyRect(r | m_artifactsRefresh);
        copyRect.translate(-contentsX(), -contentsY());

//        std::cerr << "changed = " << changed << ", artrefresh " << m_artifactsRefresh.x() << "," << m_artifactsRefresh.y() << " " << m_artifactsRefresh.width() << "x" << m_artifactsRefresh.height() << ": copying from segment to artifacts buffer: " << copyRect.width() << "x" << copyRect.height() << std::endl;

        // Copy the segments to the double buffer.
        QPainter ap;
        ap.begin(&m_doubleBuffer);
        ap.drawPixmap(copyRect.x(), copyRect.y(),
                      m_segmentsLayer,
                      copyRect.x(), copyRect.y(),
                      copyRect.width(), copyRect.height());
        ap.end();

        m_artifactsRefresh |= r;
    }

    if (m_artifactsRefresh.isValid()) {
        // Draw the artifacts over top of the segments on the double-buffer.
        refreshArtifacts(m_artifactsRefresh);
        m_artifactsRefresh = QRect();
    }

    // Display the double buffer on the viewport.

    QPainter p;
    p.begin(viewport());
    if (scroll) {
        // Redraw the entire double buffer.
        p.drawPixmap(0, 0, 
                     m_doubleBuffer,
                     0, 0,
                     m_doubleBuffer.width(),
                     m_doubleBuffer.height());
    } else {
        p.drawPixmap(updateRect.x(), updateRect.y(),
                     m_doubleBuffer,
                     updateRect.x(), updateRect.y(),
                     updateRect.width(), updateRect.height());
    }
    p.end();

    // DEBUG

    //     QPainter pdebug(viewport());
    //     static QPen framePen(QColor(Qt::red), 1);
    //     pdebug.setPen(framePen);
    //     pdebug.drawRect(updateRect);

}

bool CompositionView::scrollSegmentsLayer(QRect &rect, bool& scroll)
{
    Profiler profiler("CompositionView::scrollSegmentsLayer");

    bool all = false;
    QRect refreshRect = m_segmentsRefresh;

    int w = viewport()->width(), h = viewport()->height();
    int cx = contentsX(), cy = contentsY();

    scroll = (cx != m_lastBufferRefreshX || cy != m_lastBufferRefreshY);

    if (scroll) {

        //RG_DEBUG << "scrollSegmentsLayer: scrolling by ("
        //         << cx - m_lastBufferRefreshX << "," << cy - m_lastBufferRefreshY << ")" << endl;

        if (refreshRect.isValid()) {

            // If we've scrolled and there was an existing refresh
            // rect, we can't be sure whether the refresh rect
            // predated or postdated the internal update of scroll
            // location.  Cut our losses and refresh everything.

            refreshRect.setRect(cx, cy, w, h);

        } else {

            // No existing refresh rect: we only need to handle the
            // scroll.  Formerly we dealt with this by copying part of
            // the pixmap to itself, but that only worked by accident,
            // and it fails with the raster renderer or on non-X11
            // platforms.  Use a temporary pixmap instead

            static QPixmap map;
            if (map.size() != m_segmentsLayer.size()) {
                map = QPixmap(m_segmentsLayer.size());
            }

            // If we're scrolling sideways
            if (cx != m_lastBufferRefreshX) {

                // compute the delta
                int dx = m_lastBufferRefreshX - cx;

                // If we're scrolling less than the entire viewport
                if (dx > -w && dx < w) {

                    // Scroll the segments layer sideways
                    QPainter cp;
                    cp.begin(&map);
                    cp.drawPixmap(0, 0, m_segmentsLayer);
                    cp.end();
                    cp.begin(&m_segmentsLayer);
                    cp.drawPixmap(dx, 0, map);
                    cp.end();

                    // Add the part that was exposed to the refreshRect
                    if (dx < 0) {
                        refreshRect |= QRect(cx + w + dx, cy, -dx, h);
                    } else {
                        refreshRect |= QRect(cx, cy, dx, h);
                    }

                } else {  // We've scrolled more than the entire viewport

                    // Refresh everything
                    refreshRect.setRect(cx, cy, w, h);
                    all = true;
                }
            }

            // If we're scrolling vertically and the sideways scroll didn't
            // result in a need to refresh everything,
            if (cy != m_lastBufferRefreshY && !all) {

                // compute the delta
                int dy = m_lastBufferRefreshY - cy;

                // If we're scrolling less than the entire viewport
                if (dy > -h && dy < h) {

                    // Scroll the segments layer vertically
                    QPainter cp;
                    cp.begin(&map);
                    cp.drawPixmap(0, 0, m_segmentsLayer);
                    cp.end();
                    cp.begin(&m_segmentsLayer);
                    cp.drawPixmap(0, dy, map);
                    cp.end();

                    // Add the part that was exposed to the refreshRect
                    if (dy < 0) {
                        refreshRect |= QRect(cx, cy + h + dy, w, -dy);
                    } else {
                        refreshRect |= QRect(cx, cy, w, dy);
                    }

                } else {  // We've scrolled more than the entire viewport

                    // Refresh everything
                    refreshRect.setRect(cx, cy, w, h);
                    all = true;
                }
            }
        }
    }

    // Refresh the segments layer for the exposed portion.

    bool needRefresh = false;

    if (refreshRect.isValid()) {
        needRefresh = true;
    }

    if (needRefresh)
        refreshSegments(refreshRect);

    // ??? Move these lines to the end of refreshSegments()?
    //     Or do they still need to run even when needRefresh is false?
    m_segmentsRefresh = QRect();
    m_lastBufferRefreshX = cx;
    m_lastBufferRefreshY = cy;

    // Compute the final rect for the caller.

    rect |= refreshRect;
    if (scroll)
        rect.setRect(cx, cy, w, h);  // everything

    return needRefresh;
}

void CompositionView::refreshSegments(const QRect& rect)
{
    Profiler profiler("CompositionView::refreshSegments");

    //RG_DEBUG << "CompositionView::refreshSegments() r = "
    //         << rect << endl;

//### This constructor used to mean "start painting on the segments layer, taking your default paint configuration from the viewport".  I don't think it's supported any more -- I had to look it up (I'd never known it was possible to do this in the first place!)
//@@@    QPainter p(&m_segmentsLayer, viewport());
// Let's see how we get on with:
    QPainter p(&m_segmentsLayer);

    p.setRenderHint(QPainter::Antialiasing, false);

    p.translate( -contentsX(), -contentsY());

    if (!m_backgroundPixmap.isNull()) {
        QPoint pp(rect.x() % m_backgroundPixmap.height(), rect.y() % m_backgroundPixmap.width());
        p.drawTiledPixmap(rect, m_backgroundPixmap, pp);
    } else {
        p.eraseRect(rect);
    }

    drawSegments(&p, rect);

    // DEBUG - show what's updated
    //    QPen framePen(QColor(Qt::red), 1);
    //    p.setPen(framePen);
    //    p.drawRect(rect);

    //    m_segmentsNeedRefresh = false;
}

void CompositionView::refreshArtifacts(const QRect& rect)
{
    Profiler profiler("CompositionView::refreshArtifacts");

    //RG_DEBUG << "CompositionView::refreshArtifacts() r = "
    //         << rect << endl;

    QPainter p;

//@@@ see comment in refreshSegments    p.begin(&m_doubleBuffer, viewport());
    p.begin(&m_doubleBuffer);

    p.translate( -contentsX(), -contentsY());
    //     QRect r(contentsX(), contentsY(), m_doubleBuffer.width(), m_doubleBuffer.height());
    drawArtifacts(&p, rect);
    p.end();

    //    m_artifactsNeedRefresh = false;
}

void CompositionView::drawSegments(QPainter *segmentLayerPainter, const QRect &clipRect)
{
    Profiler profiler("CompositionView::drawSegments");

    //RG_DEBUG << "CompositionView::drawSegments() clipRect = " << clipRect;

    // *** Track Dividers

    // Fetch track Y coordinates within the clip rectangle.  We expand the
    // clip rectangle slightly because we are drawing a rather wide track
    // divider, so we need enough divider coords to do the drawing even
    // though the center of the divider might be slightly outside of the
    // viewport.
    // This is not a height list.
    CompositionModelImpl::YCoordList trackYCoords =
            getModel()->getTrackDividersIn(clipRect.adjusted(0,-1,0,+1));

    if (!trackYCoords.empty()) {

        Profiler profiler("CompositionView::drawSegments: dividing lines");

        segmentLayerPainter->save();

        // Select the lighter (middle) divider color.
        QColor light = m_trackDividerColor.light();
        segmentLayerPainter->setPen(light);

        // For each track Y coordinate, draw the two light lines in the middle
        // of the track divider.
        for (CompositionModelImpl::YCoordList::const_iterator yi = trackYCoords.begin();
             yi != trackYCoords.end(); ++yi) {
            // Upper line.
            int y = *yi - 1;
            // If it's in the clipping area, draw it
            if (y >= clipRect.top()  &&  y <= clipRect.bottom()) {
                segmentLayerPainter->drawLine(
                        clipRect.left(), y,
                        clipRect.right(), y);
            }
            // Lower line.
            ++y;
            if (y >= clipRect.top()  &&  y <= clipRect.bottom()) {
                segmentLayerPainter->drawLine(
                        clipRect.left(), y,
                        clipRect.right(), y);
            }
        }

        // Switch to the darker divider color.
        segmentLayerPainter->setPen(m_trackDividerColor);

        // For each track Y coordinate, draw the two dark lines on the outside
        // of the track divider.
        for (CompositionModelImpl::YCoordList::const_iterator yi = trackYCoords.begin();
             yi != trackYCoords.end(); ++yi) {
            // Upper line
            int y = *yi - 2;
            if (y >= clipRect.top()  &&  y <= clipRect.bottom()) {
                segmentLayerPainter->drawLine(
                        clipRect.x(), y,
                        clipRect.x() + clipRect.width() - 1, y);
            }
            // Lower line
            y += 3;
            if (y >= clipRect.top()  &&  y <= clipRect.bottom()) {
                segmentLayerPainter->drawLine(
                        clipRect.x(), y,
                        clipRect.x() + clipRect.width() - 1, y);
            }
        }

        segmentLayerPainter->restore();
    }

    // *** Get Segment and Preview Rectangles

    // Assume we aren't going to show previews.
    CompositionModelImpl::RectRanges *notationPreview = 0;
    CompositionModelImpl::AudioPreviewDrawData *audioPreview = 0;

    if (m_showPreviews) {
        // Clear the previews.  Apparently getSegmentRects() will not.
        // Given that this is the only caller, this should be moved into
        // getSegmentRects().
        m_notationPreview.clear();
        m_audioPreview.clear();

        // Indicate that we want previews.
        notationPreview = &m_notationPreview;
        audioPreview = &m_audioPreview;
    }

    // Fetch segment rectangles and (optionally) previews
    const CompositionModelImpl::RectContainer &segmentRects =
        getModel()->getSegmentRects(clipRect, notationPreview, audioPreview);

    CompositionModelImpl::RectContainer::const_iterator end = segmentRects.end();

    // *** Draw Segment Rectangles

    {
        Profiler profiler("CompositionView::drawSegments(): segment rectangles");

        segmentLayerPainter->save();

        // For each segment rectangle, draw it
        for (CompositionModelImpl::RectContainer::const_iterator i = segmentRects.begin();
             i != end; ++i) {
            segmentLayerPainter->setBrush(i->getBrush());
            segmentLayerPainter->setPen(i->getPen());

            //RG_DEBUG << "CompositionView::drawSegments : draw comp rect " << *i << endl;
            drawCompRect(*i, segmentLayerPainter, clipRect);
        }

        segmentLayerPainter->restore();
    }

    {
        Profiler profiler("CompositionView::drawSegments(): intersections");

        if (segmentRects.size() > 1) {
            //RG_DEBUG << "CompositionView::drawSegments : drawing intersections\n";
            drawIntersections(segmentRects, segmentLayerPainter, clipRect);
        }
    }

    // *** Draw Segment Previews

    if (m_showPreviews) {
        segmentLayerPainter->save();

        // Audio Previews

        {
            Profiler profiler("CompositionView::drawSegments: audio previews");

            drawAudioPreviews(segmentLayerPainter, clipRect);
        }

        // Notation Previews

        Profiler profiler("CompositionView::drawSegments: note previews");

        CompositionModelImpl::RectRanges::const_iterator npi = m_notationPreview.begin();
        CompositionModelImpl::RectRanges::const_iterator npEnd = m_notationPreview.end();

        // For each preview range
        // ??? I think there is never more than one range here.  We should be
        //     able to switch from RectRanges to RectRange.
        for (; npi != npEnd; ++npi) {
            CompositionModelImpl::RectRange interval = *npi;
            segmentLayerPainter->save();
            segmentLayerPainter->translate(
                    interval.basePoint.x(), interval.basePoint.y());
            //RG_DEBUG << "CompositionView::drawSegments : translating to x = " << interval.basePoint.x() << endl;

            // For each event rectangle
            for (; interval.range.first != interval.range.second; ++interval.range.first) {
                const QRect &pr = *(interval.range.first);
                QColor defaultCol = CompositionColourCache::getInstance()->SegmentInternalPreview;
                QColor col = interval.color.isValid() ? interval.color : defaultCol;
                segmentLayerPainter->setBrush(col);
                segmentLayerPainter->setPen(QPen(col, 0));
                //RG_DEBUG << "CompositionView::drawSegments : drawing preview rect at x = " << pr.x() << endl;
                segmentLayerPainter->drawRect(pr);
            }
            segmentLayerPainter->restore();
        }

        segmentLayerPainter->restore();
    }

    // *** Draw Segment Labels

    //
    // Draw segment labels (they must be drawn over the preview rects)
    //
    if (m_showSegmentLabels) {
        Profiler profiler("CompositionView::drawSegments: labels");
        for (CompositionModelImpl::RectContainer::const_iterator i = segmentRects.begin();
             i != end; ++i) {
            drawCompRectLabel(*i, segmentLayerPainter, clipRect);
        }
    }

    //    drawArtifacts(p, clipRect);

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

void CompositionView::drawArtifacts(QPainter * p, const QRect& clipRect)
{
    //
    // Playback Pointer
    //
    drawPointer(p, clipRect);

    //
    // Tmp rect (rect displayed while drawing a new segment)
    //
    if (m_tmpRect.isValid() && m_tmpRect.intersects(clipRect)) {
        p->setBrush(m_tmpRectFill);
        p->setPen(CompositionColourCache::getInstance()->SegmentBorder);
        drawRect(m_tmpRect, p, clipRect);
    }

    //
    // Tool guides (crosshairs)
    //
    if (m_drawGuides)
        drawGuides(p, clipRect);

    //
    // Selection Rect
    //
    if (m_drawSelectionRect) {
        //RG_DEBUG << "about to draw selection rect" << endl;
        drawRect(m_selectionRect.normalized(), p, clipRect, false, 0, false);
    }

    //
    // Floating Text
    //
    if (m_drawTextFloat)
        drawTextFloat(p, clipRect);

    //
    // Split line
    //
    if (m_splitLinePos.x() > 0 && clipRect.contains(m_splitLinePos)) {
        p->save();
        p->setPen(m_guideColor);
        p->drawLine(m_splitLinePos.x(), m_splitLinePos.y(),
                    m_splitLinePos.x(), m_splitLinePos.y() + getModel()->grid().getYSnap());
        p->restore();
    }
}

void CompositionView::drawGuides(QPainter * p, const QRect& /*clipRect*/)
{
    // no need to check for clipping, these guides are meant to follow the mouse cursor
    QPoint guideOrig(m_topGuidePos, m_foreGuidePos);
    
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

QColor CompositionView::mixBrushes(QBrush a, QBrush b)
{
    QColor ac = a.color(), bc = b.color();

    int aR = ac.red(), aG = ac.green(), aB = ac.blue(),
        bR = bc.red(), bG = bc.green(), bB = ac.blue();

    ac.setRgb((aR + bR) / 2, (aG + bG) / 2, (aB + bB) / 2);

    return ac;
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
    if (pos.y() < 0 && getModel()) {
        if (pos.y() + bound.height() < 0) {
            pos.setY(pos.y() + getModel()->grid().getYSnap() * 3);
        } else {
            pos.setY(pos.y() + getModel()->grid().getYSnap() * 2);
        }
    }

    bound.moveTopLeft(pos);

    if (bound.intersects(clipRect)) {

        p->setBrush(CompositionColourCache::getInstance()->RotaryFloatBackground);

        drawRect(bound, p, clipRect, false, 0, true);

        p->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);

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
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());

    setSelectCopy((ce.modifiers() & Qt::ControlModifier) != 0);
    setSelectCopyingAsLink(((ce.modifiers() & Qt::AltModifier) != 0) &&
                               ((ce.modifiers() & Qt::ControlModifier) != 0));
    setSelectAdd((ce.modifiers() & Qt::ShiftModifier) != 0);
    setFineGrain((ce.modifiers() & Qt::ShiftModifier) != 0);
    setPencilOverExisting((ce.modifiers() & (Qt::AltModifier + Qt::ControlModifier)) != 0);

    switch (ce.button()) {
    case Qt::LeftButton:
    case Qt::MidButton:
        startAutoScroll();

        if (m_tool)
            m_tool->handleMouseButtonPress(&ce);
        else
            RG_DEBUG << "CompositionView::mousePressEvent() :" << this << " no tool";
        break;
    case Qt::RightButton:
        if (m_tool)
            m_tool->handleRightButtonPress(&ce);
        else
            RG_DEBUG << "CompositionView::mousePressEvent() :" << this << " no tool";
        break;
    case Qt::MouseButtonMask:
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

void CompositionView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    RG_DEBUG << "CompositionView::contentsMouseReleaseEvent()\n";

    slotStopAutoScroll();

    if (!m_tool)
        return ;

    if (e->button() == Qt::LeftButton ||
        e->button() == Qt::MidButton )
        m_tool->handleMouseButtonRelease(e);
}

void CompositionView::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    m_currentIndex = getFirstItemAt(e->pos());

    if (!m_currentIndex) {
        RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - no currentIndex\n";
        const RulerScale *ruler = grid().getRulerScale();
        if (ruler) emit setPointerPosition(ruler->getTimeForX(e->pos().x()));
        return ;
    }

    RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - have currentIndex\n";

    CompositionItem* itemImpl = m_currentIndex;

    if (m_currentIndex->isRepeating()) {
        timeT time = getModel()->getRepeatTimeAt(e->pos(), m_currentIndex);

        RG_DEBUG << "editRepeat at time " << time << endl;
        if (time > 0)
            emit editRepeat(itemImpl->getSegment(), time);
        else
            emit editSegment(itemImpl->getSegment());

    } else {

        emit editSegment(itemImpl->getSegment());
    }
}

void CompositionView::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (!m_tool)
        return ;

    setFineGrain((e->modifiers() & Qt::ShiftModifier) != 0);
    setPencilOverExisting((e->modifiers() & Qt::AltModifier) != 0);

    int follow = m_tool->handleMouseMove(e);
    setFollowMode(follow);

    if (follow != RosegardenScrollView::NoFollow) {
        doAutoScroll();

        if (follow & RosegardenScrollView::FollowHorizontal) {
            int mouseX = e->pos().x();
            scrollHorizSmallSteps(mouseX);

//&& JAS - Deactivate auto expand feature when resizing / moving segments past
//&& Compostion end.  Though current code works, this creates lots of corner
//&& cases that are not reversible using the REDO / UNDO commands.
//&& Additionally, this makes accidentally altering the compostion length too easy.
//&& Currently leaving code here until a full debate is complete.

//            // enlarge composition if needed
//            if ((horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) &&
//               // This code minimizes the chances of auto expand when moving segments
//               // Not a perfect fix -- but fixes several auto expand errors
//               (mouseX > (contentsX() + viewport()->width() * 0.95))) {
//                resizeContents(contentsWidth() + m_stepSize, contentsHeight());
//                setContentsPos(contentsX() + m_stepSize, contentsY());
//                getModel()->setLength(contentsWidth());
//                slotUpdateSize();
//            }
        }

        if (follow & RosegardenScrollView::FollowVertical)
            scrollVertSmallSteps(e->pos().y());
    }
}

#if 0
// Dead Code.
void CompositionView::releaseCurrentItem()
{
    m_currentIndex = CompositionItemPtr();
}
#endif

void CompositionView::setPointerPos(int pos)
{
    //RG_DEBUG << "CompositionView::setPointerPos(" << pos << ")\n";
    int oldPos = m_pointerPos;
    if (oldPos == pos) return;

    m_pointerPos = pos;
    getModel()->pointerPosChanged(pos);

    // automagically grow contents width if pointer position goes beyond right end
    //
    if (pos >= (contentsWidth() - m_stepSize)) {
        resizeContents(pos + m_stepSize, contentsHeight());
        // grow composition too, if needed (it may not be the case if
        if (getModel()->getCompositionLength() < contentsWidth())
            getModel()->setCompositionLength(contentsWidth());
    }


    // interesting -- isAutoScrolling() never seems to return true?
    //RG_DEBUG << "CompositionView::setPointerPos(" << pos << "), isAutoScrolling " << isAutoScrolling() << ", contentsX " << contentsX() << ", m_lastPointerRefreshX " << m_lastPointerRefreshX << ", contentsHeight " << contentsHeight() << endl;

    if (contentsX() != m_lastPointerRefreshX) {
        m_lastPointerRefreshX = contentsX();
        // We'll need to shift the whole canvas anyway, so
        slotArtifactsNeedRefresh();
        return ;
    }

    int deltaW = abs(m_pointerPos - oldPos);

    if (deltaW <= m_pointerPen.width() * 2) { // use one rect instead of two separate ones

        QRect updateRect
            (std::min(m_pointerPos, oldPos) - m_pointerPen.width(), 0,
             deltaW + m_pointerPen.width() * 2, contentsHeight());

        artifactsNeedRefresh(updateRect);

    } else {

        artifactsNeedRefresh
            (QRect(m_pointerPos - m_pointerPen.width(), 0,
                   m_pointerPen.width() * 2, contentsHeight()));

        artifactsNeedRefresh
            (QRect(oldPos - m_pointerPen.width(), 0,
                   m_pointerPen.width() * 2, contentsHeight()));
    }
}

void CompositionView::setGuidesPos(int x, int y)
{
    m_topGuidePos = x;
    m_foreGuidePos = y;
    slotArtifactsNeedRefresh();
}

void CompositionView::setGuidesPos(const QPoint& p)
{
    m_topGuidePos = p.x();
    m_foreGuidePos = p.y();
    slotArtifactsNeedRefresh();
}

void CompositionView::setDrawGuides(bool d)
{
    m_drawGuides = d;
    slotArtifactsNeedRefresh();
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
    slotUpdateAll(m_tmpRect | pRect);
}

void CompositionView::setTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;
    slotArtifactsNeedRefresh();

    // most of the time when the floating text is drawn
    // we want to update a larger part of the view
    // so don't update here
    //     QRect r = fontMetrics().boundingRect(x, y, 300, 40, AlignLeft, m_textFloatText);
    //     slotUpdateAll(r);


    //    mainWindow->slotSetStatusMessage(text);
}

void CompositionView::setFineGrain(bool value)
{
    m_fineGrain = value;
}

void CompositionView::setPencilOverExisting(bool value)
{
    m_pencilOverExisting = value;
}

#if 0
// Dead Code.
void
CompositionView::slotTextFloatTimeout()
{
    hideTextFloat();
    slotArtifactsNeedRefresh();
    //    mainWindow->slotSetStatusMessage(QString::null);
}
#endif

}
#include "CompositionView.moc"
