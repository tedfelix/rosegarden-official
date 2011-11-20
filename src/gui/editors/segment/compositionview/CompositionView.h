
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COMPOSITIONVIEW_H_
#define _RG_COMPOSITIONVIEW_H_

#include "base/Selection.h"
#include "CompositionModel.h"
#include "CompositionItem.h"
#include "gui/general/RosegardenScrollView.h"
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include "base/Event.h"


class QWidget;
class QWheelEvent;
class QResizeEvent;
class QPaintEvent;
class QPainter;
class QMouseEvent;
class QEvent;


namespace Rosegarden
{

class SnapGrid;
class SegmentToolBox;
class SegmentTool;
class SegmentSelector;
class Segment;
class RosegardenDocument;
class CompositionRect;

/// Draws the Composition on the display.
/**
 * The key output routine is viewportPaintRect() which draws the segments
 * and the artifacts (playback position pointer, guides, "rubber band"
 * selection, ...) on the viewport (the portion of the composition that
 * is currently visible).
 */
class CompositionView : public RosegardenScrollView 
{
    Q_OBJECT
public:
    CompositionView(RosegardenDocument*, CompositionModel*,
                    QWidget * parent=0);

    /// Moves the playback position pointer to a new X coordinate.
    /// See getPointerPos().
    /// Also see setPointerPosition() and drawPointer().
    void setPointerPos(int pos);
    /// Gets the X coordinate of the playback position pointer.
    /// See setPointerPos().
    /// Also see setPointerPosition() and drawPointer().
    int getPointerPos() { return m_pointerPos; }

    /// Sets the position of the guides.  See setDrawGuides().
    void setGuidesPos(int x, int y);
    /// Sets the position of the guides.  See setDrawGuides().
    void setGuidesPos(const QPoint& p);
    /// Enables/disables drawing of the guides.  The guides are blue
    /// crosshairs that stretch across the entire view.  They
    /// appear when selecting or moving a segment.  See setGuidesPos()
    /// and drawGuides().
    void setDrawGuides(bool d);

    /// Gets the rect for the "rubber band" selection.
    /// See setDrawSelectionRect().
    QRect getSelectionRect() const { return m_selectionRect; }
    /// Sets the position of the "rubber band" selection.
    /// See setDrawSelectionRect().
    void setSelectionRectPos(const QPoint& pos);
    /// Sets the size of the "rubber band" selection.
    /// See setDrawSelectionRect().
    void setSelectionRectSize(int w, int h);
    /// Enables/disables drawing of the selection "rubber band" rectangle.
    /// Clicking and dragging the arrow tool across the view enables this.
    /// See getSelectionRect(), setSelectionRectPos(), and
    /// setSelectionRectSize().
    void setDrawSelectionRect(bool d);

    /// Gets the snap grid from the CompositionModel.
    SnapGrid& grid() { return m_model->grid(); }

    /// Gets the topmost item (segment) at the given position on the view.
    CompositionItem getFirstItemAt(QPoint pos);

    /// Returns the segment tool box.  See slotSetTool() and m_toolBox.
    SegmentToolBox* getToolBox() { return m_toolBox; }

    /// Returns the composition model.  See m_model.
    CompositionModel* getModel() { return m_model; }

    /// See getTmpRect().
    void setTmpRect(const QRect& r);
    /// See getTmpRect().
    void setTmpRect(const QRect& r, const QColor &c);
    /// The "temp rect" is used by the pencil tool when first drawing
    /// out the segment while the mouse button is being held down.
    /// See setTmpRect().
    const QRect& getTmpRect() const { return m_tmpRect; }

    /**
     * Set the snap resolution of the grid to something suitable.
     * 
     * fine indicates whether the current tool is a fine-grain sort
     * (such as the resize or move tools) or a coarse one (such as the
     * segment creation pencil).  If the user is requesting extra-fine
     * resolution (through slotSetFineGrain()) that will also be
     * taken into account.
     */
    void setSnapGrain(bool fine);

    /**
     * Find out whether the user is requesting extra-fine resolution
     * (e.g. by holding Shift key).  This is seldom necessary -- most
     * client code will only need to query the snap grid that is
     * adjusted appropriately by the view when interactions take
     * place.
     * See slotSetFineGrain().
     */
    bool isFineGrain() const { return m_fineGrain; }

    /**
     * Find out whether the user is requesting to draw over an existing segment
     * with the pencil, by holding the Ctrl key.  This is used by the segment
     * pencil to decide whether to abort or not if a user attempts to draw over
     * an existing segment, and this is all necessary in order to avoid breaking
     * the double-click-to-open behavior.
     * See slotSetPencilOverExisting().
     */
    bool pencilOverExisting() const { return m_pencilOverExisting; }

    /**
     * Set whether the segment items contain previews or not.
     * See isShowingPreviews().
     */
    void setShowPreviews(bool previews) { m_showPreviews = previews; }

    /**
     * Return whether the segment items contain previews or not.
     * See setShowPreviews()
     */
    bool isShowingPreviews() { return m_showPreviews; }

    /**
     * Delegates to CompositionModelImpl::clearSegmentRectsCache().
     */
    void clearSegmentRectsCache(bool clearPreviews = false);

    /// Return the selected Segments if we're currently using a "Selector"
    /// Delegates to CompositionModelImpl::getSelectedSegments().
    /// See haveSelection().
    SegmentSelection getSelectedSegments();

    /// Delegates to CompositionModelImpl::haveSelection().
    /// See getSelectedSegments().
    bool haveSelection() const { return m_model->haveSelection(); }

    /// Updates the portion of the view where the selected items are.
    /// See RosegardenScrollView::updateContents().
    void updateSelectionContents();

    /**
     * Set a text float on this canvas.  It can contain
     * anything and can be left to timeout or you can hide it
     * explicitly with hideTextFloat().
     * Used by SegmentMover::handleMouseMove() to display time,
     * bar, and beat on the view while the user is moving a segment.
     * Also used by SegmentSelector::handleMouseMove().
     * See slotTextFloatTimeout().
     */
    void setTextFloat(int x, int y, const QString &text);
    /// See setTextFloat().
    void hideTextFloat() { m_drawTextFloat = false; }

    /// Enables/disables display of the text labels on each segment.
    /// From the menu: View > Show Segment Labels.
    /// See drawCompRectLabel().
    void setShowSegmentLabels(bool b) { m_showSegmentLabels = b; }

    /// Sets the image (pixmap) that will appear behind the segments
    /// on the view.
    void setBackgroundPixmap(const QPixmap &m);

    /// Delegates to CompositionModelImpl::setAudioPreviewThread().
    void endAudioPreviewGeneration();
	
	

public slots:
    /// Dead Code.
//    void scrollRight();
    /// Dead Code.
//    void scrollLeft();
    void slotContentsMoving(int x, int y);

    /// Set the current segment editing tool.
    /// See getToolBox().
    void slotSetTool(const QString& toolName);

    /// Selects the segments via CompositionModelImpl::setSelected().
    /// Used by RosegardenMainViewWidget.
    void slotSelectSegments(const SegmentSelection &segment);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually Qt::SHIFT) and
    // selection copy (usually CONTROL)

    /// Delegates to SegmentSelector::setSegmentAdd().
    /// Used by contentsMousePressEvent() with "value" indicating whether
    /// the user is holding down the SHIFT key.
    void slotSetSelectAdd(bool value);
    /// Delegates to SegmentSelector::setSegmentCopy().
    /// Used by contentsMousePressEvent() with "value" indicating whether
    /// the user is holding down the CONTROL key.
    void slotSetSelectCopy(bool value);
    /// Delegates to SegmentSelector::setSegmentCopyingAsLink().
    /// Used by contentsMousePressEvent() with "value" indicating whether
    /// the user is holding down the ALT and CONTROL keys.
    void slotSetSelectCopyingAsLink(bool value);

    /// See isFineGrain().
    void slotSetFineGrain(bool value);
    /// See pencilOverExisting().
    void slotSetPencilOverExisting(bool value);

    /// Show the splitting line on a Segment.  Used by SegmentSplitter.
    /// See slotHideSplitLine().
    void slotShowSplitLine(int x, int y);
    /// See slotShowSplitLine().
    void slotHideSplitLine();

    /// Handles scroll wheel events from TrackEditor::m_trackButtonScroll.
    void slotExternalWheelEvent(QWheelEvent*);

    /// TextFloat timer handler.
    /// Dead Code.
//    void slotTextFloatTimeout();

    /// Redraws everything.  Segments and artifacts.
    void slotUpdateSegmentsDrawBuffer();
    /// Redraws everything (segments and artifacts) within the specified rect.
    void slotUpdateSegmentsDrawBuffer(const QRect&);

    /// Redraws everything with the new color scheme.
    /// Connected to RosegardenDocument::docColoursChanged().
    void slotRefreshColourCache();

    /// Delegates to CompositionModelImpl::addRecordingItem().
    /// Connected to RosegardenDocument::newMIDIRecordingSegment().
    ///
    /// Suggestion: Try eliminating this middleman.
    void slotNewMIDIRecordingSegment(Segment*);
    /// Delegates to CompositionModelImpl::addRecordingItem().
    /// Connected to RosegardenDocument::newAudioRecordingSegment().
    ///
    /// Suggestion: Try eliminating this middleman.
    void slotNewAudioRecordingSegment(Segment*);

    // no longer used, see RosegardenDocument::insertRecordedMidi()
//     void slotRecordMIDISegmentUpdated(Segment*, timeT updatedFrom);

    /// Delegates to CompositionModelImpl::clearRecordingItems().
    /// Connected to RosegardenDocument::stoppedAudioRecording() and
    /// RosegardenDocument::stoppedMIDIRecording().
    void slotStoppedRecording();

    /// Handles a view size change.
    /// See RosegardenScrollView::resizeContents().
    void slotUpdateSize();

signals:
    /// Emitted when a segment is double-clicked.  Launches the default
    /// segment editor.  Connected to
    /// RosegardenMainViewWidget::slotEditSegment().
    void editSegment(Segment*);
//    void editSegmentNotation(Segment*);
//    void editSegmentMatrix(Segment*);
//    void editSegmentAudio(Segment*);
//    void editSegmentEventList(Segment*);
//    void audioSegmentAutoSplit(Segment*);
    /// Emitted when a segment repeat is double-clicked.  Converts the repeat
    /// to a segment, then launches the default segment editor on the new
    /// segment.  Connected to RosegardenMainViewWidget::slotEditRepeat().
    void editRepeat(Segment*, timeT);

    /// Emitted when a double-click occurs on the ruler.
    /// Connected to RosegardenDocument::slotSetPointerPosition().
    /// Connection is made by the RosegardenMainViewWidget ctor.
    /// See setPointerPos() and drawPointer().
    void setPointerPosition(timeT);

    /// Connected to RosegardenMainWindow::slotShowToolHelp().
    void showContextHelp(const QString &);

protected:
    /// Qt event handler.  Only handles
    /// AudioPreviewThread::AudioPreviewQueueEmpty() which triggers a
    /// redraw.
    virtual bool event(QEvent *);

    /// Passes the event on to the current tool.
    virtual void contentsMousePressEvent(QMouseEvent*);
    /// Passes the event on to the current tool.
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    /// Handles a double-click by either moving the playback position
    /// pointer, or launching a segment editor.
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    /// Passes the event on to the current tool.  Also handles scrolling
    /// as needed.
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    /// Delegates to viewportPaintRect() for each rect that needs painting.
    virtual void viewportPaintEvent(QPaintEvent*);
    /// Handles resize.  Uses slotUpdateSize().
    virtual void resizeEvent(QResizeEvent*);

    /// Called when the mouse enters the view.
    /// Override of QWidget::enterEvent().
    /// Shows context help (in the status bar at the bottom) for the current
    /// tool.
    /// See leaveEvent() and slotToolHelpChanged().
    virtual void enterEvent(QEvent *);
    /// Called when the mouse leaves the view.
    /// Override of QWidget::leaveEvent().
    /// Hides context help (in the status bar at the bottom) for the current
    /// tool.
    /// See enterEvent() and slotToolHelpChanged().
    virtual void leaveEvent(QEvent *);

    /// Draws the segments and artifacts on the viewport (screen).
    /**
     * First, the segments draw buffer is copied to the artifacts
     * draw buffer.  Then the artifacts are drawn over top of the
     * segments in the artifacts (overlay) draw buffer by
     * refreshArtifactsDrawBuffer().
     * Finally, the artifacts draw buffer is copied to the viewport.
     */
    virtual void viewportPaintRect(QRect);
    
    /// Scrolls and refreshes the segment draw buffer if needed.
    /**
     * Returns enough information to determine how much additional work
     * needs to be done to update the viewport.
     * Used by viewportPaintRect().
     */
    bool scrollSegmentsDrawBuffer(QRect &rect, bool& scroll);

    /// Draws the background then calls drawArea() to draw the segments on the
    /// segments draw buffer.
    /// Used by scrollSegmentsDrawBuffer().
    void refreshSegmentsDrawBuffer(const QRect&);
    /// Calls drawAreaArtifacts() to draw the artifacts on the artifacts draw
    /// buffer.
    /// Used by viewportPaintRect().
    void refreshArtifactsDrawBuffer(const QRect&);

    /// Draws the segments on the specified painter (usually the segments
    /// draw buffer).
    /// Used by refreshSegmentsDrawBuffer().
    void drawArea(QPainter * p, const QRect& rect);
    /// Draws the audio previews for any audio segments on the specified
    /// painter (usually the segments draw buffer).
    /// Used by drawArea().
    void drawAreaAudioPreviews(QPainter * p, const QRect& rect);
    /// Draws the overlay artifacts (e.g. playback position pointer,
    /// guides, and the "rubber band" selection) on the specified painter
    /// (usually the artifacts draw buffer).
    /// Used by refreshArtifactsDrawBuffer().
    void drawAreaArtifacts(QPainter * p, const QRect& rect);
    /// Draws a rectangle on the given painter with proper clipping.
    /// This is an improved QPainter::drawRect().
    /// See drawCompRect().
    void drawRect(const QRect& rect, QPainter * p, const QRect& clipRect,
                  bool isSelected = false, int intersectLvl = 0, bool fill = true);
    /// A version of drawRect() that handles segment repeats.
    void drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                      int intersectLvl = 0, bool fill = true);
    /// Used by drawArea() to draw the segment labels.
    /// See setShowSegmentLabels().
    void drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect);
    /// Used by drawArea() to draw any intersections between rectangles.
    void drawIntersections(const CompositionModel::rectcontainer&, QPainter * p, const QRect& clipRect);

    /// Used by drawAreaArtifacts() to draw the playback position pointer on
    /// the given painter (usually the artifacts draw buffer).
    /// See setPointerPos() and setPointerPosition().
    void drawPointer(QPainter * p, const QRect& clipRect);
    /// Used by drawAreaArtifacts() to draw the guides on the given painter
    /// (usually the artifacts draw buffer).
    /// See setGuidesPos() and setDrawGuides().
    void drawGuides(QPainter * p, const QRect& clipRect);
    /// Used by drawAreaArtifacts() to draw the floating text on the view.
    /// See setTextFloat() for details.
    void drawTextFloat(QPainter * p, const QRect& clipRect);

    // Dead Code.
//    void initStepSize();
//    void releaseCurrentItem();

    /// Used by drawIntersections() to mix the brushes of intersecting
    /// rectangles.
    static QColor mixBrushes(QBrush a, QBrush b);

    /// Helper function to make it easier to get the segment selector tool.
    SegmentSelector* getSegmentSelectorTool();

    /// Adds the entire viewport to the segments draw buffer refresh rect.
    /// This will cause scrollSegmentsDrawBuffer() to refresh the entire
    /// segment draw buffer the next time it is called.  This in turn will
    /// cause viewportPaintRect() to redraw the entire viewport the next
    /// time it is called.
    void slotSegmentsDrawBufferNeedsRefresh() {
        m_segmentsDrawBufferRefresh =
            QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
    }

    /// Adds the specified rect to the segments draw buffer refresh rect.
    /// This will cause the given portion of the viewport to be refreshed
    /// the next time viewportPaintRect() is called.
    void slotSegmentsDrawBufferNeedsRefresh(QRect r) {
        m_segmentsDrawBufferRefresh |=
            (QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight())
             & r);
    }

protected slots:

    /// Updates the artifacts in the entire viewport.
    /// In addition to being used locally several times, this is also
    /// connected to CompositionModelImpl::needArtifactsUpdate().
    void slotArtifactsDrawBufferNeedsRefresh() {
        m_artifactsDrawBufferRefresh = 
            QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
        updateContents();
    }

protected:
    /// Updates the artifacts in the given rect.
    void slotArtifactsDrawBufferNeedsRefresh(QRect r) {
        m_artifactsDrawBufferRefresh |=
            (QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight())
             & r);
        updateContents(r);
    }

    /// Updates the entire viewport.
    void slotAllDrawBuffersNeedRefresh() {
        slotSegmentsDrawBufferNeedsRefresh();
        slotArtifactsDrawBufferNeedsRefresh();
    }

    /// Updates the given rect on the view.
    void slotAllDrawBuffersNeedRefresh(QRect r) {
        slotSegmentsDrawBufferNeedsRefresh(r);
        slotArtifactsDrawBufferNeedsRefresh(r);
    }

protected slots:
    /// Updates the tool context help (in the status bar at the bottom) and
    /// shows it if the mouse is in the view.
    /// Connected to SegmentToolBox::showContextHelp().
    /// See showContextHelp().
    void slotToolHelpChanged(const QString &);

protected:

    //--------------- Data members ---------------------------------

    CompositionModel* m_model;
    CompositionItem m_currentIndex;

    SegmentTool*    m_tool;
    SegmentToolBox* m_toolBox;

    bool         m_showPreviews;
    bool         m_showSegmentLabels;
    bool         m_fineGrain;
    bool         m_pencilOverExisting;

    int          m_minWidth;

    int          m_stepSize;
    QColor       m_rectFill;
    QColor       m_selectedRectFill;

    int          m_pointerPos;
    QColor       m_pointerColor;
    int          m_pointerWidth;
    QPen         m_pointerPen;

    QRect        m_tmpRect;
    QColor       m_tmpRectFill;
    QPoint       m_splitLinePos;

    QColor       m_trackDividerColor;

    bool         m_drawGuides;
    QColor       m_guideColor;
    int          m_topGuidePos;
    int          m_foreGuidePos;

    bool         m_drawSelectionRect;
    QRect        m_selectionRect;

    bool         m_drawTextFloat;
    QString      m_textFloatText;
    QPoint       m_textFloatPos;

    /// The segments draw buffer is drawn on by drawArea().  It contains
    /// the segment rectangles.
    /// refreshSegmentsDrawBuffer() draws on the segment buffer with
    /// drawArea().
    /// See viewportPaintRect().
    QPixmap      m_segmentsDrawBuffer;

    /// The artifacts draw buffer is drawn on by drawAreaArtifacts().
    /// This includes everything other than the segment rectangles.
    /// E.g. the playback position pointer, guides, and the "rubber
    /// band" selection rect.  These artifacts are drawn as an overlay
    /// on top of the segments.
    /// See viewportPaintRect().
    QPixmap      m_artifactsDrawBuffer;

    QRect        m_segmentsDrawBufferRefresh;
    QRect        m_artifactsDrawBufferRefresh;
    int          m_lastBufferRefreshX;
    int          m_lastBufferRefreshY;
    int          m_lastPointerRefreshX;
    QPixmap      m_backgroundPixmap;

    QString      m_toolContextHelp;
    bool         m_contextHelpShown;

    mutable CompositionModel::AudioPreviewDrawData m_audioPreviewRects;
    mutable CompositionModel::RectRanges m_notationPreviewRects;
};


}

#endif
