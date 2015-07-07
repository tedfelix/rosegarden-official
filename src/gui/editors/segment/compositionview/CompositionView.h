
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
#include "base/Selection.h"  // SegmentSelection
#include "base/TimeT.h"  // timeT

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>


class QWidget;
class QWheelEvent;
class QTimer;
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
 * The key output routine is drawAll() which draws the segments
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
    CompositionView(RosegardenDocument *, CompositionModelImpl *,
                    QWidget *parent);

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
    //void setGuidesPos(const QPoint& p);
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
    void setSelectionRectPos(const QPoint &pos);
    /// Set the size of the "rubber band" selection.
    /**
     * @see setDrawSelectionRect().
     */
    void setSelectionRectSize(int w, int h);
    /// Enable/disable drawing of the selection "rubber band" rectangle.
    /**
     * The SegmentSelector tool (arrow) uses this to turn the selection
     * rectangle on and off.
     *
     * @see getSelectionRect(), setSelectionRectPos(), and
     *      setSelectionRectSize()
     */
    void setDrawSelectionRect(bool d);

    /// Get the snap grid from the CompositionModelImpl.
    SnapGrid &grid()  { return m_model->grid(); }

    /// Returns the segment tool box.  See setTool() and m_toolBox.
    SegmentToolBox *getToolBox()  { return m_toolBox; }

    /// Returns the composition model.  See m_model.
    CompositionModelImpl *getModel()  { return m_model; }

    /// See getTmpRect().
    void setTmpRect(const QRect &r);
    /// See getTmpRect().
    void setTmpRect(const QRect &r, const QColor &c);
    /// Get the temporary segment rect when drawing a new segment.
    /**
     * The "temp rect" is used by the pencil tool when first drawing
     * out the segment while the mouse button is being held down.
     *
     * @see setTmpRect()
     */
    const QRect &getTmpRect() const  { return m_tmpRect; }

    /// Is the user pressing the Ctrl key to draw over a segment?
    /**
     * Find out whether the user is requesting to draw over an existing segment
     * with the pencil, by holding the Ctrl key.  This is used by the segment
     * pencil to decide whether to abort or not if a user attempts to draw over
     * an existing segment, and this is all necessary in order to avoid breaking
     * the double-click-to-open behavior.
     */
    bool pencilOverExisting() const  { return m_pencilOverExisting; }

    /// Set whether the segment items contain previews or not.
    /**
     * @see isShowingPreviews()
     */
    void setShowPreviews(bool previews)  { m_showPreviews = previews; }

    /// Return whether the segment items contain previews or not.
    /**
     * @see setShowPreviews()
     */
    //bool isShowingPreviews()  { return m_showPreviews; }

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
    bool haveSelection() const  { return m_model->haveSelection(); }

    /// Updates the portion of the view where the selected items are.
    /**
     * This is only used for segment join and update figuration.  It
     * might be useful in more situations.  However, since the
     * CompositionView is redrawn every time a command is executed
     * (TrackEditor::slotCommandExecuted()), there's no incentive to
     * use this.
     *
     * @see RosegardenScrollView::updateContents()
     */
    void updateSelectedSegments();

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
    void hideTextFloat()  { m_drawTextFloat = false; }

    /// Enables/disables display of the text labels on each segment.
    /**
     * From the menu: View > Show Segment Labels.
     *
     * @see drawCompRectLabel()
     */
    void setShowSegmentLabels(bool b)  { m_showSegmentLabels = b; }

    /// Delegates to CompositionModelImpl::setAudioPreviewThread().
    void endAudioPreviewGeneration();
	
    /// Set the current segment editing tool.
    /**
     * @see getToolBox()
     */
    void setTool(const QString &toolName);

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

    /// Handle TrackButtons scroll wheel events.  See TrackEditor::m_trackButtonScroll.
    void slotExternalWheelEvent(QWheelEvent *);

    /// Redraw everything.  Segments and artifacts.
    /**
     * This is called in many places after making changes that affect the
     * segment canvas.
     *
     * RosegardenMainViewWidget connects this to
     * CommandHistory::commandExecuted().
     */
    void slotUpdateAll();
    /// Deferred update of segments and artifacts within the specified rect.
    /**
     * Because this routine is called so frequently, it doesn't actually
     * do any work.  Instead it sets a flag, m_updateNeeded, and
     * slotUpdateTimer() does the actual work by calling
     * updateAll2(rect) on a more leisurely schedule.
     */
    void slotAllNeedRefresh(const QRect &rect);

    /// Resize the contents to match the Composition.
    /**
     * @see RosegardenScrollView::resizeContents().
     */
    void slotUpdateSize();

    // TextFloat timer handler.
    //void slotTextFloatTimeout();

signals:
    /// Emitted when a segment is double-clicked to launch the default segment editor.
    /**
     * Connected to RosegardenMainViewWidget::slotEditSegment().
     */
    void editSegment(Segment *);

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
    void editRepeat(Segment *, timeT);

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

    //void editSegmentNotation(Segment*);
    //void editSegmentMatrix(Segment*);
    //void editSegmentAudio(Segment*);
    //void editSegmentEventList(Segment*);
    //void audioSegmentAutoSplit(Segment*);

private slots:

    /// Updates the artifacts in the entire viewport.
    /**
     * In addition to being used locally several times, this is also
     * connected to CompositionModelImpl::needArtifactsUpdate().
     */
    void slotUpdateArtifacts() {
        m_artifactsRefresh =
            QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height());
        updateContents();
    }

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
    void slotNewMIDIRecordingSegment(Segment *);
    /**
     * Delegates to CompositionModelImpl::addRecordingItem().
     * Connected to RosegardenDocument::newAudioRecordingSegment().
     *
     * Suggestion: Try eliminating this middleman.
     */
    void slotNewAudioRecordingSegment(Segment *);

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
     * slotAllNeedRefresh(rect) sets the m_updateNeeded flag to
     * tell slotUpdateTimer() that it needs to perform an update.
     */
    void slotUpdateTimer();

private:
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

    /// Delegates to drawAll() for each rect that needs painting.
    virtual void paintEvent(QPaintEvent *);
    /// Handles resize.  Uses slotUpdateSize().
    virtual void resizeEvent(QResizeEvent *);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually Qt::SHIFT) and
    // selection copy (usually CONTROL)

    /// See pencilOverExisting().
    //void setPencilOverExisting(bool value);

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
    void drawAll(QRect rect);
    
    /// Scrolls and refreshes the segment layer (m_segmentsLayer) if needed.
    /**
     * Returns enough information to determine how much additional work
     * needs to be done to update the viewport.
     * Used by drawAll().
     *
     * This routine appears to mainly refresh the segments layer.  Scrolling
     * only happens if needed.  Having "scroll" in the name might be
     * misleading.  However, calling this refreshSegmentsLayer() confuses
     * it with drawSegments(rect).  Need to dig a bit more.
     */
    bool scrollSegmentsLayer(QRect &rect, bool &scroll);

    /// Draw the segments on the segment layer (m_segmentsLayer).
    /**
     * Draws the background then calls drawSegments() to draw the segments on the
     * segments layer (m_segmentsLayer).  Used by
     * scrollSegmentsLayer().
     */
    void drawSegments(const QRect &);
    /// Draw the artifacts on the viewport.
    /*
     * Calls drawArtifacts() to draw the artifacts on the viewport.
     * Used by drawAll().
     */
    void drawArtifacts(const QRect &clipRect);

    /// Draw the track dividers on the segments layer.
    void drawTrackDividers(QPainter *segmentsLayerPainter, const QRect &clipRect);
    /// Draws the segments on the segments layer (m_segmentsLayer).
    /**
     * Also draws the track dividers.
     *
     * Used by drawSegments(rect).
     */
    void drawSegments(QPainter *segmentsLayerPainter, const QRect &clipRect);
    /// Draw the previews for audio segments on the segments layer (m_segmentsLayer).
    /**
     * Used by drawSegments().
     */
    void drawAudioPreviews(QPainter *segmentsLayerPainter, const QRect &clipRect);
    /// Draw the overlay artifacts on the viewport.
    /**
     * "Artifacts" include anything that isn't a segment.  E.g. The playback
     * position pointer, guides, and the "rubber band" selection.  Used by
     * drawArtifacts(rect).
     */
    void drawArtifacts(QPainter *viewportPainter, const QRect &clipRect);
    /// Draws a rectangle on the given painter with proper clipping.
    /**
     * This is an improved QPainter::drawRect().
     *
     * @see drawCompRect()
     */
    void drawRect(const QRect &rect, QPainter *p, const QRect &clipRect,
                  bool isSelected = false, int intersectLvl = 0, bool fill = true);
    /// A version of drawRect() that handles segment repeats.
    void drawCompRect(const CompositionRect &r, QPainter *p, const QRect &clipRect,
                      int intersectLvl = 0, bool fill = true);
    /// Used by drawSegments() to draw the segment labels.
    /**
     * @see setShowSegmentLabels()
     */
    void drawCompRectLabel(const CompositionRect &r, QPainter *p, const QRect &clipRect);
    /// Used by drawSegments() to draw any intersections between rectangles.
    void drawIntersections(const CompositionModelImpl::RectContainer &, QPainter *p, const QRect &clipRect);

    /// Used by drawArtifacts() to draw the playback position pointer.
    /**
     * @see setPointerPos() and setPointerPosition()
     */
    void drawPointer(QPainter *p, const QRect &clipRect);
    /// Used by drawArtifacts() to draw the guides on the viewport.
    /**
     * @see setGuidesPos() and setDrawGuides()
     */
    void drawGuides(QPainter *p, const QRect &clipRect);
    /// Used by drawArtifacts() to draw floating text.
    /**
     * @see setTextFloat()
     */
    void drawTextFloat(QPainter *p, const QRect &clipRect);

    //void initStepSize();

    /// Used by drawIntersections() to mix the brushes of intersecting rectangles.
    static QColor mixBrushes(const QBrush &a, const QBrush &b);

    /// Deferred update of the segments within the entire viewport.
    /**
     * This will cause scrollSegmentsLayer() to refresh the entire
     * segments layer (m_segmentsLayer) the next time it is called.
     * This in turn will cause drawAll() to redraw the entire
     * viewport the next time it is called.
     */
    void segmentsNeedRefresh() {
        m_segmentsRefresh.setRect(contentsX(), contentsY(), viewport()->width(), viewport()->height());
    }

    /// Deferred update of the segments within the specified rect.
    /**
     * This will cause the given portion of the viewport to be refreshed
     * the next time drawAll() is called.
     */
    void segmentsNeedRefresh(const QRect &r) {
        m_segmentsRefresh |=
            (QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height())
             & r);
    }

    /// Updates the artifacts in the given rect.
    void updateArtifacts(const QRect &r) {
        m_artifactsRefresh |=
            (QRect(contentsX(), contentsY(), viewport()->width(), viewport()->height())
             & r);
        updateContents(r);
    }

    /// Update segments and artifacts within rect.
    void updateAll2(const QRect &rect);

    /// Update segments and artifacts within the entire viewport.
    void updateAll() {
        segmentsNeedRefresh();
        slotUpdateArtifacts();
    }

    //--------------- Data members ---------------------------------

    CompositionModelImpl *m_model;

    /// The tool that receives mouse events.
    SegmentTool    *m_currentTool;
    SegmentToolBox *m_toolBox;

    /// Performance testing.
    bool         m_enableDrawing;

    bool         m_showPreviews;
    bool         m_showSegmentLabels;
    bool         m_pencilOverExisting;

    //int          m_minWidth;

    // This is always 0.  It used to be set to the single step size
    // of the horizontal scrollbar.  Now it's always 0.
    int          m_stepSize;
    //QColor       m_rectFill;
    //QColor       m_selectedRectFill;

    int          m_pointerPos;
    QPen         m_pointerPen;

    QRect        m_tmpRect;
    QColor       m_tmpRectFill;
    QPoint       m_splitLinePos;

    QColor       m_trackDividerColor;

    bool         m_drawGuides;
    QColor       m_guideColor;
    int          m_guideX;
    int          m_guideY;

    bool         m_drawSelectionRect;
    QRect        m_selectionRect;

    bool         m_drawTextFloat;
    QString      m_textFloatText;
    QPoint       m_textFloatPos;

    /// Layer that contains the segment rectangles.
    /**
     * @see drawAll() and drawSegments()
     */
    QPixmap      m_segmentsLayer;

    /// Portion of the viewport that needs segments refreshed.
    /**
     * Used only by scrollSegmentsLayer() to limit work done redrawing
     * the segment rectangles.
     */
    QRect        m_segmentsRefresh;

    /// Portion of the viewport that needs artifacts refreshed.
    /**
     * Used only by drawAll() to limit work done redrawing the
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
