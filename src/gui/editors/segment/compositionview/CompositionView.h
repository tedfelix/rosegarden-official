
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

#ifndef RG_COMPOSITIONVIEW_H
#define RG_COMPOSITIONVIEW_H

#include "CompositionModelImpl.h"
#include "CompositionItem.h"
#include "gui/general/RosegardenScrollView.h"
#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QTimer>


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
 *
 * TrackEditor creates and owns the only instance of this class.  This
 * class works together with CompositionModelImpl to provide the composition
 * user interface (the segment canvas).
 */
class CompositionView : public RosegardenScrollView 
{
    Q_OBJECT
public:
    CompositionView(RosegardenDocument*, CompositionModelImpl *,
                    QWidget * parent=0);

    /// Move the playback position pointer to a new X coordinate.
    /**
     * @see getPointerPos(), setPointerPosition(), and drawPointer()
     */
    void setPointerPos(int pos);
    /// Get the X coordinate of the playback position pointer.
    /**
     * @see setPointerPos(), setPointerPosition(), and drawPointer()
     */
    int getPointerPos() { return m_pointerPos; }

    /// Sets the position of the guides.  See setDrawGuides().
    void setGuidesPos(int x, int y);
    /// Sets the position of the guides.  See setDrawGuides().
    void setGuidesPos(const QPoint& p);
    /// Enable/disable drawing of the guides.
    /**
     * The guides are blue crosshairs that stretch across the entire view.
     * They appear when selecting or moving a segment.
     *
     * @see setGuidesPos() and drawGuides()
     */
    void setDrawGuides(bool d);

    /// Get the rect for the "rubber band" selection.
    /**
     * @see setDrawSelectionRect()
     */
    QRect getSelectionRect() const { return m_selectionRect; }
    /// Set the position of the "rubber band" selection.
    /**
     * @see setDrawSelectionRect().
     */
    void setSelectionRectPos(const QPoint& pos);
    /// Set the size of the "rubber band" selection.
    /**
     * @see setDrawSelectionRect().
     */
    void setSelectionRectSize(int w, int h);
    /// Enable/disable drawing of the selection "rubber band" rectangle.
    /**
     * Clicking and dragging the arrow tool across the view enables this.
     *
     * @see getSelectionRect(), setSelectionRectPos(), and
     *      setSelectionRectSize()
     */
    void setDrawSelectionRect(bool d);

    /// Get the snap grid from the CompositionModelImpl.
    SnapGrid& grid() { return m_model->grid(); }

    /// Get the topmost item (segment) at the given position on the view.
    CompositionItemPtr getFirstItemAt(QPoint pos);

    /// Returns the segment tool box.  See setTool() and m_toolBox.
    SegmentToolBox* getToolBox() { return m_toolBox; }

    /// Returns the composition model.  See m_model.
    CompositionModelImpl* getModel() { return m_model; }

    /// See getTmpRect().
    void setTmpRect(const QRect& r);
    /// See getTmpRect().
    void setTmpRect(const QRect& r, const QColor &c);
    /// Get the temporary segment rect when drawing a new segment.
    /**
     * The "temp rect" is used by the pencil tool when first drawing
     * out the segment while the mouse button is being held down.
     *
     * @see setTmpRect()
     */
    const QRect& getTmpRect() const { return m_tmpRect; }

    /// Set the snap resolution of the grid to something suitable.
    /**
     * fine indicates whether the current tool is a fine-grain sort
     * (such as the resize or move tools) or a coarse one (such as the
     * segment creation pencil).  If the user is requesting extra-fine
     * resolution (through setFineGrain()) that will also be
     * taken into account.
     */
    void setSnapGrain(bool fine);

    /// Is the user requesting extra-fine resolution (shift key)?
    /**
     * Find out whether the user is requesting extra-fine resolution
     * (e.g. by holding Shift key).  This is seldom necessary -- most
     * client code will only need to query the snap grid that is
     * adjusted appropriately by the view when interactions take
     * place.
     *
     * @see setFineGrain()
     */
    bool isFineGrain() const { return m_fineGrain; }

    /// Is the user pressing the Ctrl key to draw over a segment?
    /**
     * Find out whether the user is requesting to draw over an existing segment
     * with the pencil, by holding the Ctrl key.  This is used by the segment
     * pencil to decide whether to abort or not if a user attempts to draw over
     * an existing segment, and this is all necessary in order to avoid breaking
     * the double-click-to-open behavior.
     *
     * @see setPencilOverExisting()
     */
    bool pencilOverExisting() const { return m_pencilOverExisting; }

    /// Set whether the segment items contain previews or not.
    /**
     * @see isShowingPreviews()
     */
    void setShowPreviews(bool previews) { m_showPreviews = previews; }

    /// Return whether the segment items contain previews or not.
    /**
     * @see setShowPreviews()
     */
    bool isShowingPreviews() { return m_showPreviews; }

    /**
     * Delegates to CompositionModelImpl::clearSegmentRectsCache().
     */
    void clearSegmentRectsCache(bool clearPreviews = false);

    /// Return the selected Segments if we're currently using a "Selector".
    /**
     * Delegates to CompositionModelImpl::getSelectedSegments().
     *
     * @see haveSelection()
     */
    SegmentSelection getSelectedSegments();

    /// Delegates to CompositionModelImpl::haveSelection().
    /**
     * @see getSelectedSegments()
     */
    bool haveSelection() const { return m_model->haveSelection(); }

    /// Updates the portion of the view where the selected items are.
    /**
     * @see RosegardenScrollView::updateContents()
     */
    void updateSelectionContents();

    /// Set a text float on this canvas.
    /**
     * The floating text can contain
     * anything and can be left to timeout or you can hide it
     * explicitly with hideTextFloat().
     *
     * Used by SegmentMover::handleMouseMove() to display time,
     * bar, and beat on the view while the user is moving a segment.
     * Also used by SegmentSelector::handleMouseMove().
     *
     * @see slotTextFloatTimeout()
     */
    void setTextFloat(int x, int y, const QString &text);
    /// See setTextFloat().
    void hideTextFloat() { m_drawTextFloat = false; }

    /// Enables/disables display of the text labels on each segment.
    /**
     * From the menu: View > Show Segment Labels.
     *
     * @see drawCompRectLabel()
     */
    void setShowSegmentLabels(bool b) { m_showSegmentLabels = b; }

    /// Set the image that will appear behind the segments on the view.
    void setBackgroundPixmap(const QPixmap &m);

    /// Delegates to CompositionModelImpl::setAudioPreviewThread().
    void endAudioPreviewGeneration();
	
    /// Set the current segment editing tool.
    /**
     * @see getToolBox()
     */
    void setTool(const QString& toolName);

    /// Selects the segments via CompositionModelImpl::setSelected().
    /**
     * Used by RosegardenMainViewWidget.
     */
    void selectSegments(const SegmentSelection &segment);

    /// Show the splitting line on a Segment.  Used by SegmentSplitter.
    /**
     * @see hideSplitLine()
     */
    void showSplitLine(int x, int y);
    /// See showSplitLine().
    void hideSplitLine();


public slots:
//    void scrollRight();
//    void scrollLeft();

    //void slotContentsMoving(int x, int y);

    /// Handle scroll wheel events from TrackEditor::m_trackButtonScroll.
    void slotExternalWheelEvent(QWheelEvent*);

    // TextFloat timer handler.
//    void slotTextFloatTimeout();

    /// Redraw everything.  Segments and artifacts.
    void slotUpdateAll();
    /// Redraw everything (segments and artifacts) within the specified rect.
    /**
     * Because this routine is called so frequently, it doesn't actually
     * do any work.  Instead it sets a flag, m_updateNeeded, and
     * slotUpdateTimer() does the actual work by calling
     * updateAll() on a more leisurely schedule.
     */
    void slotUpdateAll(const QRect &rect);

    /// Handles a view size change.
    /**
     * @see RosegardenScrollView::resizeContents().
     */
    void slotUpdateSize();

signals:
    /// Emitted when a segment is double-clicked to launch the default segment editor.
    /**
     * Connected to RosegardenMainViewWidget::slotEditSegment().
     */
    void editSegment(Segment*);
//    void editSegmentNotation(Segment*);
//    void editSegmentMatrix(Segment*);
//    void editSegmentAudio(Segment*);
//    void editSegmentEventList(Segment*);
//    void audioSegmentAutoSplit(Segment*);

    /// Emitted when a segment repeat is double-clicked.
    /**
     * Connected to RosegardenMainViewWidget::slotEditRepeat() which converts
     * the repeat to a segment.  This doesn't actually start the segment
     * editor.  mouseDoubleClickEvent() emits editSegment() after
     * it emits this.
     *
     * rename: segmentRepeatDoubleClick(), repeatToSegment(), others?
     *         Not sure which might be most helpful to readers of the code.
     *         editRepeat() is misleading.  repeatToSegment() is more correct
     *         at the same level of abstraction.
     */
    void editRepeat(Segment*, timeT);

    /// Emitted when a double-click occurs on the ruler.
    /**
     * Connected to RosegardenDocument::slotSetPointerPosition().
     * Connection is made by the RosegardenMainViewWidget ctor.
     *
     * @see setPointerPos() and drawPointer()
     */
    void setPointerPosition(timeT);

    /// Emitted when hovering over the CompositionView to show help text.
    /**
     * Connected to RosegardenMainWindow::slotShowToolHelp().
     */
    void showContextHelp(const QString &);

protected:
    /// Redraw in response to AudioPreviewThread::AudioPreviewQueueEmpty.
    virtual bool event(QEvent *);

    /// Passes the event on to the current tool.
    virtual void mousePressEvent(QMouseEvent *);
    /// Passes the event on to the current tool.
    virtual void mouseReleaseEvent(QMouseEvent *);
    /// Launches a segment editor or moves the position pointer.
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    /// Passes the event on to the current tool.
    /**
     * Also handles scrolling as needed.
     */
    virtual void mouseMoveEvent(QMouseEvent *);

    /// Delegates to viewportPaintRect() for each rect that needs painting.
    virtual void paintEvent(QPaintEvent *);
    /// Handles resize.  Uses slotUpdateSize().
    virtual void resizeEvent(QResizeEvent*);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually Qt::SHIFT) and
    // selection copy (usually CONTROL)

    /// Handle SHIFT key.
    /**
     * Delegates to SegmentSelector::setSegmentAdd().  Used by
     * mousePressEvent() with "value" indicating whether the user
     * is holding down the SHIFT key.
     */
    void setSelectAdd(bool value);
    /// Handle CONTROL key.
    /**
     * Delegates to SegmentSelector::setSegmentCopy().  Used by
     * mousePressEvent() with "value" indicating whether the user is
     * holding down the CONTROL key.
     */
    void setSelectCopy(bool value);
    /// Handle ALT and CONTROL keys.
    /**
     * Delegates to SegmentSelector::setSegmentCopyingAsLink().
     * Used by mousePressEvent() with "value" indicating whether the
     * user is holding down the ALT and CONTROL keys.
     */
    void setSelectCopyingAsLink(bool value);

    /// See isFineGrain().
    void setFineGrain(bool value);
    /// See pencilOverExisting().
    void setPencilOverExisting(bool value);

    /// Called when the mouse enters the view.
    /**
     * Override of QWidget::enterEvent().  Shows context help (in the status
     * bar at the bottom) for the current tool.
     *
     * @see leaveEvent() and slotToolHelpChanged()
     */
    virtual void enterEvent(QEvent *);
    /// Called when the mouse leaves the view.
    /**
     * Override of QWidget::leaveEvent().
     * Hides context help (in the status bar at the bottom) for the current
     * tool.
     *
     * @see enterEvent() and slotToolHelpChanged()
     */
    virtual void leaveEvent(QEvent *);

    /// Draw the segments and artifacts on the viewport (screen).
    /**
     * First, the appropriate portion of the segments layer (m_segmentsLayer)
     * is copied to the double-buffer (m_doubleBuffer).  Then the artifacts
     * are drawn over top of the segments in the double-buffer by
     * refreshArtifacts().  Finally, the double-buffer is copied to
     * the display (QAbstractScrollArea::viewport()).
     */
    virtual void viewportPaintRect(QRect);
    
    /// Scrolls and refreshes the segment layer (m_segmentsLayer) if needed.
    /**
     * Returns enough information to determine how much additional work
     * needs to be done to update the viewport.
     * Used by viewportPaintRect().
     *
     * This routine appears to mainly refresh the segments layer.  Scrolling
     * only happens if needed.  Having "scroll" in the name might be
     * misleading.  However, calling this refreshSegmentsLayer() confuses
     * it with refreshSegments().  Need to dig a bit more.
     */
    bool scrollSegmentsLayer(QRect &rect, bool& scroll);

    /// Draw the segments on the segment layer (m_segmentsLayer).
    /**
     * Draws the background then calls drawSegments() to draw the segments on the
     * segments layer (m_segmentsLayer).  Used by
     * scrollSegmentsLayer().
     */
    void refreshSegments(const QRect&);
    /// Draw the artifacts on the double-buffer (m_doubleBuffer).
    /*
     * Calls drawArtifacts() to draw the artifacts on the double-buffer
     * (m_doubleBuffer).  Used by viewportPaintRect().
     */
    void refreshArtifacts(const QRect&);

    /// Draws the segments on the segments layer (m_segmentsLayer).
    /**
     * Also draws the track dividers.
     *
     * Used by refreshSegments().
     */
    void drawSegments(QPainter *segmentLayerPainter, const QRect& rect);
    /// Draw the previews for audio segments on the segments layer (m_segmentsLayer).
    /**
     * Used by drawSegments().
     */
    void drawAudioPreviews(QPainter * p, const QRect& rect);
    /// Draws the overlay artifacts on the double-buffer.
    /**
     * "Artifacts" include anything that isn't a segment.  E.g. The playback
     * position pointer, guides, and the "rubber band" selection.  Used by
     * refreshArtifacts().
     */
    void drawArtifacts(QPainter * p, const QRect& rect);
    /// Draws a rectangle on the given painter with proper clipping.
    /**
     * This is an improved QPainter::drawRect().
     *
     * @see drawCompRect()
     */
    void drawRect(const QRect& rect, QPainter * p, const QRect& clipRect,
                  bool isSelected = false, int intersectLvl = 0, bool fill = true);
    /// A version of drawRect() that handles segment repeats.
    void drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                      int intersectLvl = 0, bool fill = true);
    /// Used by drawSegments() to draw the segment labels.
    /**
     * @see setShowSegmentLabels()
     */
    void drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect);
    /// Used by drawSegments() to draw any intersections between rectangles.
    void drawIntersections(const CompositionModelImpl::RectContainer &, QPainter *p, const QRect &clipRect);

    /// Used by drawArtifacts() to draw the playback position pointer.
    /**
     * @see setPointerPos() and setPointerPosition()
     */
    void drawPointer(QPainter * p, const QRect& clipRect);
    /// Used by drawArtifacts() to draw the guides on the double-buffer.
    /**
     * @see setGuidesPos() and setDrawGuides()
     */
    void drawGuides(QPainter * p, const QRect& clipRect);
    /// Used by drawArtifacts() to draw floating text.
    /**
     * @see setTextFloat()
     */
    void drawTextFloat(QPainter * p, const QRect& clipRect);

//    void initStepSize();
//    void releaseCurrentItem();

    /// Used by drawIntersections() to mix the brushes of intersecting rectangles.
    static QColor mixBrushes(QBrush a, QBrush b);

    /// Helper function to make it easier to get the segment selector tool.
    SegmentSelector* getSegmentSelectorTool();

    /// Adds the entire viewport to the segments refresh rect.
    /**
     * This will cause scrollSegmentsLayer() to refresh the entire
     * segments layer (m_segmentsLayer) the next time it is called.
     * This in turn will cause viewportPaintRect() to redraw the entire
     * viewport the next time it is called.
     */
    void segmentsNeedRefresh() {
        m_segmentsRefresh =
            QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height());
    }

    /// Adds the specified rect to the segments refresh rect.
    /**
     * This will cause the given portion of the viewport to be refreshed
     * the next time viewportPaintRect() is called.
     */
    void segmentsNeedRefresh(QRect r) {
        m_segmentsRefresh |=
            (QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height())
             & r);
    }

    /// Does the actual work for slotUpdateAll()
    void updateAll(const QRect& rect);

protected slots:

    /// Updates the artifacts in the entire viewport.
    /**
     * In addition to being used locally several times, this is also
     * connected to CompositionModelImpl::needArtifactsUpdate().
     */
    void slotArtifactsNeedRefresh() {
        m_artifactsRefresh = 
            QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height());
        updateContents();
    }

protected:
    /// Updates the artifacts in the given rect.
    void artifactsNeedRefresh(QRect r) {
        m_artifactsRefresh |=
            (QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height())
             & r);
        updateContents(r);
    }

    /// Updates the entire viewport.
    void allNeedRefresh() {
        segmentsNeedRefresh();
        slotArtifactsNeedRefresh();
    }

    /// Updates the given rect on the view.
    void allNeedRefresh(QRect r) {
        segmentsNeedRefresh(r);
        artifactsNeedRefresh(r);
    }

protected slots:
    /// Redraw everything with the new color scheme.
    /**
     * Connected to RosegardenDocument::docColoursChanged().
     */
    void slotRefreshColourCache();

    /**
     * Delegates to CompositionModelImpl::addRecordingItem().
     * Connected to RosegardenDocument::newMIDIRecordingSegment().
     *
     * Suggestion: Try eliminating this middleman.
     */
    void slotNewMIDIRecordingSegment(Segment*);
    /**
     * Delegates to CompositionModelImpl::addRecordingItem().
     * Connected to RosegardenDocument::newAudioRecordingSegment().
     *
     * Suggestion: Try eliminating this middleman.
     */
    void slotNewAudioRecordingSegment(Segment*);

    // no longer used, see RosegardenDocument::insertRecordedMidi()
    //     void slotRecordMIDISegmentUpdated(Segment*, timeT updatedFrom);

    /**
     * Delegates to CompositionModelImpl::clearRecordingItems().
     * Connected to RosegardenDocument::stoppedAudioRecording() and
     * RosegardenDocument::stoppedMIDIRecording().
     */
    void slotStoppedRecording();

    /// Updates the tool context help and shows it if the mouse is in the view.
    /**
     * The tool context help appears in the status bar at the bottom.
     *
     * Connected to SegmentToolBox::showContextHelp().
     *
     * @see showContextHelp()
     */
    void slotToolHelpChanged(const QString &);

    /// Used to reduce the frequency of updates.
    /**
     * slotUpdateAll() sets the m_updateNeeded flag to
     * tell slotUpdateTimer() that it needs to perform an update.
     */
    void slotUpdateTimer();

protected:

    //--------------- Data members ---------------------------------

    CompositionModelImpl* m_model;
    CompositionItemPtr m_currentIndex;

    SegmentTool*    m_tool;
    SegmentToolBox* m_toolBox;

    /// Performance testing.
    bool         m_enableDrawing;

    bool         m_showPreviews;
    bool         m_showSegmentLabels;
    bool         m_fineGrain;
    bool         m_pencilOverExisting;

    //int          m_minWidth;

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

    /// Layer that contains the segment rectangles.
    /**
     * @see viewportPaintRect() and drawSegments()
     */
    QPixmap      m_segmentsLayer;

    /// The display double-buffer.
    /**
     * Double-buffers are used to reduce flicker and reduce the complexity
     * of drawing code (e.g. no need to erase anything, just redraw it all).
     *
     * ??? Qt already does double-buffering for us.  Is this still needed?
     *
     * @see viewportPaintRect()
     */
    QPixmap      m_doubleBuffer;

    /// Portion of the viewport that needs segments refreshed.
    /**
     * Used only by scrollSegmentsLayer() to limit work done redrawing
     * the segment rectangles.
     */
    QRect        m_segmentsRefresh;

    /// Portion of the viewport that needs artifacts refreshed.
    /**
     * Used only by viewportPaintRect() to limit work done redrawing the
     * artifacts.
     */
    QRect        m_artifactsRefresh;

    int          m_lastBufferRefreshX;
    int          m_lastBufferRefreshY;
    int          m_lastPointerRefreshX;
    QPixmap      m_backgroundPixmap;

    QString      m_toolContextHelp;
    bool         m_contextHelpShown;

    CompositionModelImpl::AudioPreviewDrawData m_audioPreview;
    CompositionModelImpl::RectRanges m_notationPreview;

    /// Drives slotUpdateTimer().
    QTimer *m_updateTimer;
    /// Lets slotUpdateTimer() know there's work to do.
    bool m_updateNeeded;
    /// Accumulated update rectangle.
    QRect m_updateRect;
};


}

#endif
