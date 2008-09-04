/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CompositionView.h"

#include "misc/Debug.h"
#include "AudioPreviewThread.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "CompositionColourCache.h"
#include "CompositionItemHelper.h"
#include "CompositionItemImpl.h"
#include "CompositionModel.h"
#include "CompositionModelImpl.h"
#include "CompositionRect.h"
#include "AudioPreviewPainter.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenCanvasView.h"
#include "gui/general/RosegardenScrollView.h"
#include "SegmentSelector.h"
#include "SegmentToolBox.h"
#include "SegmentTool.h"
#include <kmessagebox.h>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <qmemarray.h>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QScrollBar>
#include <qscrollview.h>
#include <QSize>
#include <QString>
#include <QWidget>
#include <kapplication.h>
#include <kconfig.h>
#include <algorithm>


namespace Rosegarden
{

class PreviewRect : public QRect {
public:
    PreviewRect(int left, int top, int width, int height) :
        QRect(left, top, width, height) {};

    PreviewRect(const QRect& r) :
        QRect(r) {};

    const QColor& getColor() const { return m_color; }
    void setColor(QColor c) { m_color = c; }

protected:
    QColor m_color;
};

CompositionView::CompositionView(RosegardenGUIDoc* doc,
                                 CompositionModel* model,
                                 QWidget * parent, const char * name, WFlags f)
#if KDE_VERSION >= KDE_MAKE_VERSION(3,2,0)
        : RosegardenScrollView(parent, name, f | WNoAutoErase | WStaticContents),
#else
        :
        RosegardenScrollView(parent, name, f | WRepaintNoErase | WResizeNoErase | WStaticContents),
#endif
        m_model(model),
        m_currentIndex(0),
        m_tool(0),
        m_toolBox(0),
        m_showPreviews(false),
        m_showSegmentLabels(true),
        m_fineGrain(false),
	m_pencilOverExisting(false),
        m_minWidth(m_model->getLength()),
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
        m_segmentsDrawBuffer(visibleWidth(), visibleHeight()),
        m_artifactsDrawBuffer(visibleWidth(), visibleHeight()),
        m_segmentsDrawBufferRefresh(0, 0, visibleWidth(), visibleHeight()),
        m_artifactsDrawBufferRefresh(0, 0, visibleWidth(), visibleHeight()),
        m_lastBufferRefreshX(0),
        m_lastBufferRefreshY(0),
        m_lastPointerRefreshX(0),
        m_contextHelpShown(false)
{
    if (doc) {
        m_toolBox = new SegmentToolBox(this, doc);

        connect(m_toolBox, SIGNAL(showContextHelp(const QString &)),
                this, SLOT(slotToolHelpChanged(const QString &)));
    }
    
    setDragAutoScroll(true);
    setBackgroundMode(NoBackground);
    viewport()->setBackgroundMode(NoBackground);
    viewport()->setPaletteBackgroundColor(GUIPalette::getColour(GUIPalette::SegmentCanvas));

    slotUpdateSize();

    QScrollBar* hsb = horizontalScrollBar();

    // dynamically adjust content size when scrolling past current composition's end
    connect(hsb, SIGNAL(nextLine()),
            this, SLOT(scrollRight()));
    connect(hsb, SIGNAL(prevLine()),
            this, SLOT(scrollLeft()));

    //    connect(this, SIGNAL(contentsMoving(int, int)),
    //            this, SLOT(slotAllDrawBuffersNeedRefresh()));

    //     connect(this, SIGNAL(contentsMoving(int, int)),
    //             this, SLOT(slotContentsMoving(int, int)));

    connect(model, SIGNAL(needContentUpdate()),
            this, SLOT(slotUpdateSegmentsDrawBuffer()));
    connect(model, SIGNAL(needContentUpdate(const QRect&)),
            this, SLOT(slotUpdateSegmentsDrawBuffer(const QRect&)));
    connect(model, SIGNAL(needArtifactsUpdate()),
            this, SLOT(slotArtifactsDrawBufferNeedsRefresh()));
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
    
    CompositionModelImpl* cmi = dynamic_cast<CompositionModelImpl*>(model);
    if (cmi) {
        cmi->setAudioPreviewThread(&doc->getAudioPreviewThread());
    }

    if (doc) {
        doc->getAudioPreviewThread().setEmptyQueueListener(this);
    }
    
    m_segmentsDrawBuffer.setOptimization(QPixmap::BestOptim);
    m_artifactsDrawBuffer.setOptimization(QPixmap::BestOptim);

    viewport()->setMouseTracking(true);
}

void CompositionView::endAudioPreviewGeneration()
{
    CompositionModelImpl* cmi = dynamic_cast<CompositionModelImpl*>(m_model);
    if (cmi) {
        cmi->setAudioPreviewThread(0);
    }
}

void CompositionView::setBackgroundPixmap(const QPixmap &m)
{
    m_backgroundPixmap = m;
    //     viewport()->setErasePixmap(m_backgroundPixmap);
}

void CompositionView::initStepSize()
{
    QScrollBar* hsb = horizontalScrollBar();
    m_stepSize = hsb->lineStep();
}

void CompositionView::slotUpdateSize()
{
    int vStep = getModel()->grid().getYSnap();
    int height = std::max(getModel()->getNbRows() * vStep, (unsigned)visibleHeight());

    RulerScale *ruler = grid().getRulerScale();

    int minWidth = sizeHint().width();
    int computedWidth = int(nearbyint(ruler->getTotalWidth()));

    int width = std::max(computedWidth, minWidth);

    resizeContents(width, height);
}

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

void CompositionView::setSelectionRectPos(const QPoint& pos)
{
    m_selectionRect.setRect(pos.x(), pos.y(), 0, 0);
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setSelectionRectSize(int w, int h)
{
    m_selectionRect.setSize(QSize(w, h));
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setDrawSelectionRect(bool d)
{
    if (m_drawSelectionRect != d) {
        m_drawSelectionRect = d;
        slotArtifactsDrawBufferNeedsRefresh();
        slotUpdateSegmentsDrawBuffer(m_selectionRect);
    }
}

void CompositionView::clearSegmentRectsCache(bool clearPreviews)
{
    dynamic_cast<CompositionModelImpl*>(getModel())->clearSegmentRectsCache(clearPreviews);
}

SegmentSelection
CompositionView::getSelectedSegments()
{
    return (dynamic_cast<CompositionModelImpl*>(m_model))->getSelectedSegments();
}

void CompositionView::updateSelectionContents()
{
    if (!haveSelection())
        return ;


    QRect selectionRect = getModel()->getSelectionContentsRect();
    updateContents(selectionRect);
}

void CompositionView::slotContentsMoving(int x, int y)
{
    //     qDebug("contents moving : x=%d", x);
}

void CompositionView::slotSetTool(const QString& toolName)
{
    RG_DEBUG << "CompositionView::slotSetTool(" << toolName << ")"
    << this << "\n";

    if (m_tool)
        m_tool->stow();

    m_toolContextHelp = "";

    m_tool = m_toolBox->getTool(toolName);

    if (m_tool)
        m_tool->ready();
    else {
        KMessageBox::error(0, QString("CompositionView::slotSetTool() : unknown tool name %1").arg(toolName));
    }
}

void CompositionView::slotSelectSegments(const SegmentSelection &segments)
{
    RG_DEBUG << "CompositionView::slotSelectSegments\n";

    static QRect dummy;

    getModel()->clearSelected();

    for (SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) {
        getModel()->setSelected(CompositionItem(new CompositionItemImpl(**i, dummy)));
    }
    slotUpdateSegmentsDrawBuffer();
}

SegmentSelector*
CompositionView::getSegmentSelectorTool()
{
    return dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));
}

void CompositionView::slotSetSelectAdd(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool)
        return ;

    selTool->setSegmentAdd(value);
}

void CompositionView::slotSetSelectCopy(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool)
        return ;

    selTool->setSegmentCopy(value);
}

void CompositionView::slotShowSplitLine(int x, int y)
{
    m_splitLinePos.setX(x);
    m_splitLinePos.setY(y);
}

void CompositionView::slotHideSplitLine()
{
    m_splitLinePos.setX( -1);
    m_splitLinePos.setY( -1);
}

void CompositionView::slotExternalWheelEvent(QWheelEvent* e)
{
    e->accept();
    wheelEvent(e);
}

CompositionItem CompositionView::getFirstItemAt(QPoint pos)
{
    CompositionModel::itemcontainer items = getModel()->getItemsAt(pos);

    if (items.size()) {
        // find topmost item
        CompositionItem res = *(items.begin());

        unsigned int maxZ = res->z();

        CompositionModel::itemcontainer::iterator maxZItemPos = items.begin();

        for (CompositionModel::itemcontainer::iterator i = items.begin();
                i != items.end(); ++i) {
            CompositionItem ic = *i;
            if (ic->z() > maxZ) {
                RG_DEBUG << k_funcinfo << "found new topmost at z=" << ic->z() << endl;
                res = ic;
                maxZ = ic->z();
                maxZItemPos = i;
            }
        }

        // get rid of the rest;
        items.erase(maxZItemPos);
        for (CompositionModel::itemcontainer::iterator i = items.begin();
                i != items.end(); ++i)
            delete *i;

        return res;
    } else {
        RG_DEBUG << k_funcinfo << "no item under cursor\n";
    }


    return CompositionItem();
}

void CompositionView::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
        grid().setSnapTime(SnapGrid::NoSnap);
    } else {
        grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}

void CompositionView::slotUpdateSegmentsDrawBuffer()
{
    //     RG_DEBUG << "CompositionView::slotUpdateSegmentsDrawBuffer()\n";
    slotAllDrawBuffersNeedRefresh();
    updateContents();
}

void CompositionView::slotUpdateSegmentsDrawBuffer(const QRect& rect)
{
    //     RG_DEBUG << "CompositionView::slotUpdateSegmentsDrawBuffer() rect "
    //              << rect << " - valid : " << rect.isValid() << endl;

    slotAllDrawBuffersNeedRefresh(rect);

    if (rect.isValid()) {
        updateContents(rect);
    } else {
        updateContents();
    }
}

void CompositionView::slotRefreshColourCache()
{
    CompositionColourCache::getInstance()->init();
    clearSegmentRectsCache();
    slotUpdateSegmentsDrawBuffer();
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
    QScrollView::resizeEvent(e);
    slotUpdateSize();

    int w = std::max(m_segmentsDrawBuffer.width(), visibleWidth());
    int h = std::max(m_segmentsDrawBuffer.height(), visibleHeight());

    m_segmentsDrawBuffer.resize(w, h);
    m_artifactsDrawBuffer.resize(w, h);
    slotAllDrawBuffersNeedRefresh();
    //     RG_DEBUG << "CompositionView::resizeEvent() : drawBuffer size = " << m_segmentsDrawBuffer.size() << endl;
}

void CompositionView::viewportPaintEvent(QPaintEvent* e)
{
    QMemArray<QRect> rects = e->region().rects();

    for (unsigned int i = 0; i < rects.size(); ++i) {
        viewportPaintRect(rects[i]);
    }
}

void CompositionView::viewportPaintRect(QRect r)
{
    QRect updateRect = r;

    r &= viewport()->rect();
    r.moveBy(contentsX(), contentsY());

    //     RG_DEBUG << "CompositionView::viewportPaintRect() r = " << r
    //              << " - moveBy " << contentsX() << "," << contentsY() << " - updateRect = " << updateRect
    //              << " - refresh " << m_segmentsDrawBufferRefresh << " artrefresh " << m_artifactsDrawBufferRefresh << endl;


    bool scroll = false;
    bool changed = checkScrollAndRefreshDrawBuffer(r, scroll);

    if (changed || m_artifactsDrawBufferRefresh.isValid()) {

        // r was modified by checkScrollAndRefreshDrawBuffer
        QRect copyRect(r | m_artifactsDrawBufferRefresh);
        copyRect.moveBy( -contentsX(), -contentsY());

        //	RG_DEBUG << "copying from segment to artifacts buffer: " << copyRect << endl;

        bitBlt(&m_artifactsDrawBuffer,
               copyRect.x(), copyRect.y(),
               &m_segmentsDrawBuffer,
               copyRect.x(), copyRect.y(), copyRect.width(), copyRect.height());
        m_artifactsDrawBufferRefresh |= r;
    }

    if (m_artifactsDrawBufferRefresh.isValid()) {
        refreshArtifactsDrawBuffer(m_artifactsDrawBufferRefresh);
        m_artifactsDrawBufferRefresh = QRect();
    }

    if (scroll) {
        bitBlt(viewport(), 0, 0,
               &m_artifactsDrawBuffer, 0, 0,
               m_artifactsDrawBuffer.width(), m_artifactsDrawBuffer.height());
    } else {
        bitBlt(viewport(), updateRect.x(), updateRect.y(),
               &m_artifactsDrawBuffer, updateRect.x(), updateRect.y(),
               updateRect.width(), updateRect.height());
    }

    // DEBUG

    //     QPainter pdebug(viewport());
    //     static QPen framePen(QColor(Qt::red), 1);
    //     pdebug.setPen(framePen);
    //     pdebug.drawRect(updateRect);

}

bool CompositionView::checkScrollAndRefreshDrawBuffer(QRect &rect, bool& scroll)
{
    bool all = false;
    QRect refreshRect = m_segmentsDrawBufferRefresh;

    int w = visibleWidth(), h = visibleHeight();
    int cx = contentsX(), cy = contentsY();

    scroll = (cx != m_lastBufferRefreshX || cy != m_lastBufferRefreshY);

    if (scroll) {

        //	RG_DEBUG << "checkScrollAndRefreshDrawBuffer: scrolling by ("
        //		 << cx - m_lastBufferRefreshX << "," << cy - m_lastBufferRefreshY << ")" << endl;

        if (refreshRect.isValid()) {

            // If we've scrolled and there was an existing refresh
            // rect, we can't be sure whether the refresh rect
            // predated or postdated the internal update of scroll
            // location.  Cut our losses and refresh everything.

            refreshRect.setRect(cx, cy, w, h);

        } else {

            // No existing refresh rect: we only need to handle the
            // scroll

            if (cx != m_lastBufferRefreshX) {

                int dx = m_lastBufferRefreshX - cx;

                if (dx > -w && dx < w) {

                    QPainter cp(&m_segmentsDrawBuffer);
                    cp.drawPixmap(dx, 0, m_segmentsDrawBuffer);

                    if (dx < 0) {
                        refreshRect |= QRect(cx + w + dx, cy, -dx, h);
                    } else {
                        refreshRect |= QRect(cx, cy, dx, h);
                    }

                } else {

                    refreshRect.setRect(cx, cy, w, h);
                    all = true;
                }
            }

            if (cy != m_lastBufferRefreshY && !all) {

                int dy = m_lastBufferRefreshY - cy;

                if (dy > -h && dy < h) {

                    QPainter cp(&m_segmentsDrawBuffer);
                    cp.drawPixmap(0, dy, m_segmentsDrawBuffer);

                    if (dy < 0) {
                        refreshRect |= QRect(cx, cy + h + dy, w, -dy);
                    } else {
                        refreshRect |= QRect(cx, cy, w, dy);
                    }

                } else {

                    refreshRect.setRect(cx, cy, w, h);
                    all = true;
                }
            }
        }
    }

    bool needRefresh = false;

    if (refreshRect.isValid()) {
        needRefresh = true;
    }

    if (needRefresh)
        refreshSegmentsDrawBuffer(refreshRect);

    m_segmentsDrawBufferRefresh = QRect();
    m_lastBufferRefreshX = cx;
    m_lastBufferRefreshY = cy;

    rect |= refreshRect;
    if (scroll)
        rect.setRect(cx, cy, w, h);
    return needRefresh;
}

void CompositionView::refreshSegmentsDrawBuffer(const QRect& rect)
{
    //    Profiler profiler("CompositionView::refreshDrawBuffer", true);
    //      RG_DEBUG << "CompositionView::refreshSegmentsDrawBuffer() r = "
    //  	     << rect << endl;

    QPainter p(&m_segmentsDrawBuffer, viewport());
    p.translate( -contentsX(), -contentsY());

    if (!m_backgroundPixmap.isNull()) {
        QPoint pp(rect.x() % m_backgroundPixmap.height(), rect.y() % m_backgroundPixmap.width());
        p.drawTiledPixmap(rect, m_backgroundPixmap, pp);
    } else {
        p.eraseRect(rect);
    }

    drawArea(&p, rect);

    // DEBUG - show what's updated
    //    QPen framePen(QColor(Qt::red), 1);
    //    p.setPen(framePen);
    //    p.drawRect(rect);

    //    m_segmentsDrawBufferNeedsRefresh = false;
}

void CompositionView::refreshArtifactsDrawBuffer(const QRect& rect)
{
    //      RG_DEBUG << "CompositionView::refreshArtifactsDrawBuffer() r = "
    //               << rect << endl;

    QPainter p;
    p.begin(&m_artifactsDrawBuffer, viewport());
    p.translate( -contentsX(), -contentsY());
    //     QRect r(contentsX(), contentsY(), m_artifactsDrawBuffer.width(), m_artifactsDrawBuffer.height());
    drawAreaArtifacts(&p, rect);
    p.end();

    //    m_artifactsDrawBufferNeedsRefresh = false;
}

void CompositionView::drawArea(QPainter *p, const QRect& clipRect)
{
    //     Profiler profiler("CompositionView::drawArea", true);

    //     RG_DEBUG << "CompositionView::drawArea() clipRect = " << clipRect << endl;

    //
    // Fetch track dividing lines
    //
    CompositionModel::heightlist lineHeights = getModel()->getTrackDividersIn(clipRect);

    if (!lineHeights.empty()) {

        p->save();
        QColor light = m_trackDividerColor.light();
        p->setPen(light);

        for (CompositionModel::heightlist::const_iterator hi = lineHeights.begin();
             hi != lineHeights.end(); ++hi) {
            int y = *hi;
            if (y-1 >= clipRect.y()) {
                p->drawLine(clipRect.x(), y-1,
                            clipRect.x() + clipRect.width() - 1, y-1);
            }
            if (y >= clipRect.y()) {
                p->drawLine(clipRect.x(), y,
                            clipRect.x() + clipRect.width() - 1, y);
            }
        }

        p->setPen(m_trackDividerColor);

        for (CompositionModel::heightlist::const_iterator hi = lineHeights.begin();
             hi != lineHeights.end(); ++hi) {
            int y = *hi;
            if (y-2 >= clipRect.y()) {
                p->drawLine(clipRect.x(), y-2,
                            clipRect.x() + clipRect.width() - 1, y-2);
            }
            if (y+1 >= clipRect.y()) {
                p->drawLine(clipRect.x(), y+1,
                            clipRect.x() + clipRect.width() - 1, y+1);
            }
        }

        p->restore();
    }

    CompositionModel::AudioPreviewDrawData* audioPreviewData = 0;
    CompositionModel::RectRanges* notationPreviewData = 0;

    //
    // Fetch previews
    //
    if (m_showPreviews) {
        notationPreviewData = &m_notationPreviewRects;
        m_notationPreviewRects.clear();
        audioPreviewData = &m_audioPreviewRects;
        m_audioPreviewRects.clear();
    }

    //
    // Fetch segment rectangles to draw
    //
    const CompositionModel::rectcontainer& rects = getModel()->getRectanglesIn(clipRect,
            notationPreviewData, audioPreviewData);
    CompositionModel::rectcontainer::const_iterator i = rects.begin();
    CompositionModel::rectcontainer::const_iterator end = rects.end();

    //
    // Draw Segment Rectangles
    //
    p->save();
    for (; i != end; ++i) {
        p->setBrush(i->getBrush());
        p->setPen(i->getPen());

        //         RG_DEBUG << "CompositionView::drawArea : draw comp rect " << *i << endl;
        drawCompRect(*i, p, clipRect);
    }

    p->restore();

    if (rects.size() > 1) {
        //         RG_DEBUG << "CompositionView::drawArea : drawing intersections\n";
        drawIntersections(rects, p, clipRect);
    }

    //
    // Previews
    //
    if (m_showPreviews) {
        p->save();

        // draw audio previews
        //
        drawAreaAudioPreviews(p, clipRect);

        // draw notation previews
        //
        CompositionModel::RectRanges::const_iterator npi = m_notationPreviewRects.begin();
        CompositionModel::RectRanges::const_iterator npEnd = m_notationPreviewRects.end();

        for (; npi != npEnd; ++npi) {
            CompositionModel::RectRange interval = *npi;
            p->save();
            p->translate(interval.basePoint.x(), interval.basePoint.y());
            //             RG_DEBUG << "CompositionView::drawArea : translating to x = " << interval.basePoint.x() << endl;
            for (; interval.range.first != interval.range.second; ++interval.range.first) {

                const PreviewRect& pr = *(interval.range.first);
                QColor defaultCol = CompositionColourCache::getInstance()->SegmentInternalPreview;
                QColor col = interval.color.isValid() ? interval.color : defaultCol;
                p->setBrush(col);
                p->setPen(col);
                //                RG_DEBUG << "CompositionView::drawArea : drawing preview rect at x = " << pr.x() << endl;
                p->drawRect(pr);
            }
            p->restore();
        }

        p->restore();
    }

    //
    // Draw segment labels (they must be drawn over the preview rects)
    //
    if (m_showSegmentLabels) {
        for (i = rects.begin(); i != end; ++i) {
            drawCompRectLabel(*i, p, clipRect);
        }
    }

    //    drawAreaArtifacts(p, clipRect);

}

void CompositionView::drawAreaAudioPreviews(QPainter * p, const QRect& clipRect)
{
    CompositionModel::AudioPreviewDrawData::const_iterator api = m_audioPreviewRects.begin();
    CompositionModel::AudioPreviewDrawData::const_iterator apEnd = m_audioPreviewRects.end();
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
        rectToFill.moveBy( -basePoint.x(), -basePoint.y());
        int firstPixmapIdx = (r.x() - basePoint.x()) / AudioPreviewPainter::tileWidth();
        if (firstPixmapIdx >= api->pixmap.size()) {
            //             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : WARNING - miscomputed pixmap array : r.x = "
            //                      << r.x() << " - basePoint.x = " << basePoint.x() << " - firstPixmapIdx = " << firstPixmapIdx
            //                      << endl;
            continue;
        }
        int x = 0, idx = firstPixmapIdx;
        //         RG_DEBUG << "CompositionView::drawAreaAudioPreviews : clipRect = " << clipRect
        //                  << " - firstPixmapIdx = " << firstPixmapIdx << endl;
        while (x < clipRect.width()) {
            int pixmapRectXOffset = idx * AudioPreviewPainter::tileWidth();
            localRect.setRect(basePoint.x() + pixmapRectXOffset, basePoint.y(),
                              AudioPreviewPainter::tileWidth(), api->rect.height());
            //             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : initial localRect = "
            //                      << localRect << endl;
            localRect &= r;
            if (idx == firstPixmapIdx && api->resizeOffset != 0) {
                // this segment is being resized from start, clip beginning of preview
                localRect.moveBy(api->resizeOffset, 0);
            }

            //             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : localRect & clipRect = "
            //                      << localRect << endl;
            if (localRect.isEmpty()) {
                //                 RG_DEBUG << "CompositionView::drawAreaAudioPreviews : localRect & clipRect is empty\n";
                break;
            }
            localRect.moveBy( -(basePoint.x() + pixmapRectXOffset), -basePoint.y());

            //             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : drawing pixmap "
            //                      << idx << " at " << drawBasePoint << " - localRect = " << localRect
            //                      << " - preResizeOrigin : " << api->preResizeOrigin << endl;

            p->drawImage(drawBasePoint, api->pixmap[idx], localRect,
                         Qt::ColorOnly | Qt::ThresholdDither | Qt::AvoidDither);

            ++idx;
            if (idx >= api->pixmap.size())
                break;
            drawBasePoint.setX(drawBasePoint.x() + localRect.width());
            x += localRect.width();
        }
    }
}

void CompositionView::drawAreaArtifacts(QPainter * p, const QRect& clipRect)
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
        drawRect(m_selectionRect, p, clipRect, false, 0, false);
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

    p->save();
    p->setPen(m_guideColor);
    p->drawLine(guideOrig.x(), 0, guideOrig.x(), contentsHeight());
    p->drawLine(0, guideOrig.y(), contentsWidth(), guideOrig.y());
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

        //         RG_DEBUG << "CompositionView::drawCompRect() : drawing repeating rect " << r
        //                  << " nb repeat marks = " << repeatMarks.size() << endl;

        // draw 'start' rectangle with original brush
        //
        QRect startRect = r;
        startRect.setWidth(repeatMarks[0] - r.x());
        p->setBrush(r.getBrush());
        drawRect(startRect, p, clipRect, r.isSelected(), intersectLvl, fill);


        // now draw the 'repeat' marks
        //
        p->setPen(CompositionColourCache::getInstance()->RepeatSegmentBorder);
        int penWidth = std::max(r.getPen().width(), 1u);

        for (unsigned int i = 0; i < repeatMarks.size(); ++i) {
            int pos = repeatMarks[i];
            if (pos > clipRect.right())
                break;

            if (pos >= clipRect.left()) {
                QPoint p1(pos, r.y() + penWidth),
                p2(pos, r.y() + r.height() - penWidth - 1);

                //                 RG_DEBUG << "CompositionView::drawCompRect() : drawing repeat mark at "
                //                          << p1 << "-" << p2 << endl;
                p->drawLine(p1, p2);
            }

        }

    }

    p->restore();
}

void CompositionView::drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect)
{
    // draw segment label
    //
#ifdef NOT_DEFINED
    if (!r.getLabel().isEmpty() /* && !r.isSelected() */)
    {
        p->save();
        p->setPen(GUIPalette::getColour(GUIPalette::SegmentLabel));
        p->setBrush(white);
        QRect textRect(r);
        textRect.setX(textRect.x() + 3);
        QString label = " " + r.getLabel() + " ";
        QRect textBoundingRect = p->boundingRect(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
        p->drawRect(textBoundingRect & r);
        p->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
        p->restore();
    }
#else
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

        QBrush brush = r.getBrush();
        QColor surroundColour = brush.color().light(110);

        int h, s, v;
        surroundColour.hsv(&h, &s, &v);
        if (v < 150)
            surroundColour.setHsv(h, s, 225);
        p->setPen(surroundColour);

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

        p->setPen(GUIPalette::getColour
                  (GUIPalette::SegmentLabel));
        p->drawText(labelRect,
                    Qt::AlignLeft | Qt::AlignVCenter, r.getLabel());
        p->restore();
    }
#endif
}

void CompositionView::drawRect(const QRect& r, QPainter *p, const QRect& clipRect,
                               bool isSelected, int intersectLvl, bool fill)
{
    //     RG_DEBUG << "CompositionView::drawRect : intersectLvl = " << intersectLvl
    //              << " - brush col = " << p->brush().color() << endl;

    //     RG_DEBUG << "CompositionView::drawRect " << r << " - xformed : " << p->xForm(r)
    //              << " - contents x = " << contentsX() << ", contents y = " << contentsY() << endl;

    p->save();

    QRect rect = r;

    if (fill) {
        if (isSelected) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark(200);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
            //            RG_DEBUG << "CompositionView::drawRect : selected color : " << fillColor << endl;
        }

        if (intersectLvl > 0) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark((intersectLvl) * 105);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
            //            RG_DEBUG << "CompositionView::drawRect : intersected color : " << fillColor << " isSelected : " << isSelected << endl;
        }
    } else {
        p->setBrush(Qt::NoBrush);
    }

    // Paint using the small coordinates...
    QRect intersection = rect.intersect(clipRect);

    if (clipRect.contains(rect)) {
        p->drawRect(rect);
    } else {
        // draw only what's necessary
        if (!intersection.isEmpty() && fill)
            p->fillRect(intersection, p->brush());

        int rectTopY = rect.y();

        if (rectTopY >= clipRect.y() &&
                rectTopY <= (clipRect.y() + clipRect.height())) {
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.topLeft(), intersection.topRight());
        }

        int rectBottomY = rect.y() + rect.height();
        if (rectBottomY >= clipRect.y() &&
                rectBottomY <= (clipRect.y() + clipRect.height()))
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.bottomLeft(), intersection.bottomRight());

        int rectLeftX = rect.x();
        if (rectLeftX >= clipRect.x() &&
                rectLeftX <= (clipRect.x() + clipRect.width()))
            p->drawLine(rect.topLeft(), rect.bottomLeft());

        unsigned int rectRightX = rect.x() + rect.width(); // make sure we don't overflow
        if (rectRightX >= unsigned(clipRect.x()) &&
                rectRightX <= unsigned(clipRect.x() + clipRect.width()))
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

void CompositionView::drawIntersections(const CompositionModel::rectcontainer& rects,
                                        QPainter * p, const QRect& clipRect)
{
    if (! (rects.size() > 1))
        return ;

    CompositionModel::rectcontainer intersections;

    CompositionModel::rectcontainer::const_iterator i = rects.begin(),
            j = rects.begin();

    for (; j != rects.end(); ++j) {

        CompositionRect testRect = *j;
        i = j;
        ++i; // set i to pos after j

        if (i == rects.end())
            break;

        for (; i != rects.end(); ++i) {
            CompositionRect ri = testRect.intersect(*i);
            if (!ri.isEmpty()) {
                CompositionModel::rectcontainer::iterator t = std::find(intersections.begin(),
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

        for (CompositionModel::rectcontainer::iterator intIter = intersections.begin();
                intIter != intersections.end(); ++intIter) {
            CompositionRect r = *intIter;
            drawCompRect(r, p, clipRect, intersectionLvl);
        }

        if (intersections.size() > 10)
            break; // put a limit on how many intersections we can compute and draw - this grows exponentially

        ++intersectionLvl;

        CompositionModel::rectcontainer intersections2;

        CompositionModel::rectcontainer::iterator i = intersections.begin(),
                j = intersections.begin();

        for (; j != intersections.end(); ++j) {

            CompositionRect testRect = *j;
            i = j;
            ++i; // set i to pos after j

            if (i == intersections.end())
                break;

            for (; i != intersections.end(); ++i) {
                CompositionRect ri = testRect.intersect(*i);
                if (!ri.isEmpty() && ri != *i) {
                    CompositionModel::rectcontainer::iterator t = std::find(intersections2.begin(),
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
    //     RG_DEBUG << "CompositionView::drawPointer: clipRect "
    // 	     << clipRect.x() << "," << clipRect.y() << " " << clipRect.width()
    // 	     << "x" << clipRect.height() << " pointer pos is " << m_pointerPos << endl;

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

    QRect bound = p->boundingRect(0, 0, 300, metrics.height() + 6, AlignLeft, m_textFloatText);

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
        slotSegmentsDrawBufferNeedsRefresh();
        viewport()->update();
        return true;
    }

    return RosegardenScrollView::event(e);
}

void CompositionView::enterEvent(QEvent *e)
{
    kapp->config()->beginGroup( GeneralOptionsConfigGroup );
    // 
    // manually-FIX, add:
    // kapp->config()->endGroup();		// corresponding to: kapp->config()->beginGroup( GeneralOptionsConfigGroup );
    //  
;
    if (! qStrToBool( kapp->config()->value("toolcontexthelp", "true" ) ) ) return;

    emit showContextHelp(m_toolContextHelp);
    m_contextHelpShown = true;
}

void CompositionView::leaveEvent(QEvent *e)
{
    emit showContextHelp("");
    m_contextHelpShown = false;
}

void CompositionView::slotToolHelpChanged(const QString &text)
{
    if (m_toolContextHelp == text) return;
    m_toolContextHelp = text;

    kapp->config()->beginGroup( GeneralOptionsConfigGroup );

    // 

    // manually-FIX, add:

    // kapp->config()->endGroup();		// corresponding to: kapp->config()->beginGroup( GeneralOptionsConfigGroup );

    //  
;
    if (! qStrToBool( kapp->config()->value("toolcontexthelp", "true" ) ) ) return;

    if (m_contextHelpShown) emit showContextHelp(text);
}

void CompositionView::contentsMousePressEvent(QMouseEvent* e)
{
    Qt::ButtonState bs = e->state();
    slotSetSelectCopy((bs & Qt::ControlModifier) != 0);
    slotSetSelectAdd((bs & Qt::ShiftModifier) != 0);
    slotSetFineGrain((bs & Qt::ShiftModifier) != 0);
    slotSetPencilOverExisting((bs & Qt::AltModifier + Qt::ControlModifier) != 0);

    switch (e->button()) {
    case LeftButton:
    case MidButton:
        startAutoScroll();

        if (m_tool)
            m_tool->handleMouseButtonPress(e);
        else
            RG_DEBUG << "CompositionView::contentsMousePressEvent() :"
            << this << " no tool\n";
        break;
    case RightButton:
        if (m_tool)
            m_tool->handleRightButtonPress(e);
        else
            RG_DEBUG << "CompositionView::contentsMousePressEvent() :"
            << this << " no tool\n";
        break;
    default:
        break;
    }
}

void CompositionView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    RG_DEBUG << "CompositionView::contentsMouseReleaseEvent()\n";

    stopAutoScroll();

    if (!m_tool)
        return ;

    if (e->button() == LeftButton ||
        e->button() == MidButton )
        m_tool->handleMouseButtonRelease(e);
}

void CompositionView::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    m_currentIndex = getFirstItemAt(e->pos());

    if (!m_currentIndex) {
        RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - no currentIndex\n";
        RulerScale *ruler = grid().getRulerScale();
        if (ruler) emit setPointerPosition(ruler->getTimeForX(e->pos().x()));
        return ;
    }

    RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - have currentIndex\n";

    CompositionItemImpl* itemImpl = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)m_currentIndex);

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

    Qt::ButtonState bs = e->state();
    slotSetFineGrain((bs & Qt::ShiftModifier) != 0);
    slotSetPencilOverExisting((bs & Qt::AltModifier) != 0);

    int follow = m_tool->handleMouseMove(e);
    setScrollDirectionConstraint(follow);

    if (follow != RosegardenCanvasView::NoFollow) {
        doAutoScroll();

        if (follow & RosegardenCanvasView::FollowHorizontal) {
            slotScrollHorizSmallSteps(e->pos().x());

            // enlarge composition if needed
            if (horizontalScrollBar()->value() == horizontalScrollBar()->maximum()) {
                resizeContents(contentsWidth() + m_stepSize, contentsHeight());
                setContentsPos(contentsX() + m_stepSize, contentsY());
                getModel()->setLength(contentsWidth());
                slotUpdateSize();
            }
        }

        if (follow & RosegardenCanvasView::FollowVertical)
            slotScrollVertSmallSteps(e->pos().y());
    }
}

void CompositionView::releaseCurrentItem()
{
    m_currentIndex = CompositionItem();
}

void CompositionView::setPointerPos(int pos)
{
    //     RG_DEBUG << "CompositionView::setPointerPos(" << pos << ")\n";
    int oldPos = m_pointerPos;
    if (oldPos == pos)
        return ;

    m_pointerPos = pos;
    getModel()->setPointerPos(pos);

    // automagically grow contents width if pointer position goes beyond right end
    //
    if (pos >= (contentsWidth() - m_stepSize)) {
        resizeContents(pos + m_stepSize, contentsHeight());
        // grow composition too, if needed (it may not be the case if
        if (getModel()->getLength() < contentsWidth())
            getModel()->setLength(contentsWidth());
    }


    // interesting -- isAutoScrolling() never seems to return true?
    //     RG_DEBUG << "CompositionView::setPointerPos(" << pos << "), isAutoScrolling " << isAutoScrolling() << ", contentsX " << contentsX() << ", m_lastPointerRefreshX " << m_lastPointerRefreshX << ", contentsHeight " << contentsHeight() << endl;

    if (contentsX() != m_lastPointerRefreshX) {
        m_lastPointerRefreshX = contentsX();
        // We'll need to shift the whole canvas anyway, so
        slotArtifactsDrawBufferNeedsRefresh();
        return ;
    }

    int deltaW = abs(m_pointerPos - oldPos);

    if (deltaW <= m_pointerPen.width() * 2) { // use one rect instead of two separate ones

        QRect updateRect
        (std::min(m_pointerPos, oldPos) - m_pointerPen.width(), 0,
         deltaW + m_pointerPen.width() * 2, contentsHeight());

        slotArtifactsDrawBufferNeedsRefresh(updateRect);

    } else {

        slotArtifactsDrawBufferNeedsRefresh
        (QRect(m_pointerPos - m_pointerPen.width(), 0,
               m_pointerPen.width() * 2, contentsHeight()));

        slotArtifactsDrawBufferNeedsRefresh
        (QRect(oldPos - m_pointerPen.width(), 0,
               m_pointerPen.width() * 2, contentsHeight()));
    }
}

void CompositionView::setGuidesPos(int x, int y)
{
    m_topGuidePos = x;
    m_foreGuidePos = y;
    slotArtifactsDrawBufferNeedsRefresh();
}

void CompositionView::setGuidesPos(const QPoint& p)
{
    m_topGuidePos = p.x();
    m_foreGuidePos = p.y();
    slotArtifactsDrawBufferNeedsRefresh();
}

void CompositionView::setDrawGuides(bool d)
{
    m_drawGuides = d;
    slotArtifactsDrawBufferNeedsRefresh();
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
    slotUpdateSegmentsDrawBuffer(m_tmpRect | pRect);
}

void CompositionView::setTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;
    slotArtifactsDrawBufferNeedsRefresh();

    // most of the time when the floating text is drawn
    // we want to update a larger part of the view
    // so don't update here
    //     QRect r = fontMetrics().boundingRect(x, y, 300, 40, AlignLeft, m_textFloatText);
    //     slotUpdateSegmentsDrawBuffer(r);


    //    rgapp->slotSetStatusMessage(text);
}

void CompositionView::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}

void CompositionView::slotSetPencilOverExisting(bool value)
{
    m_pencilOverExisting = value;
}

void
CompositionView::slotTextFloatTimeout()
{
    hideTextFloat();
    slotArtifactsDrawBufferNeedsRefresh();
    //    rgapp->slotSetStatusMessage(QString::null);
}

}
#include "CompositionView.moc"
