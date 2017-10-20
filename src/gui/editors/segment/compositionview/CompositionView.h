/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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
#include "gui/general/RosegardenScrollView.h"
#include "base/Selection.h"  // SegmentSelection
#include "base/TimeT.h"  // timeT

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
class QKeyEvent;
class QEvent;


namespace Rosegarden
{

class SnapGrid;
class SegmentToolBox;
class SegmentTool;
class Segment;
class RosegardenDocument;
class SegmentRect;

/// Draws the Composition on the display.  The segment canvas.
/**
 * The key output routine is drawAll() which draws the segments
 * and the artifacts (playback position pointer, guides, "rubber band"
 * selection, ...) on the viewport (the portion of the Composition that
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

    // *** Miscellaneous

    CompositionModelImpl *getModel()  { return m_model; }

    /// CompositionModelImpl::m_grid
    SnapGrid &grid()  { return m_model->grid(); }

    /// Returns the segment tool box.  See setTool() and m_toolBox.
    SegmentToolBox *getToolBox()  { return m_toolBox; }

    /// Set the current segment editing tool.
    /**
     * @see getToolBox()
     */
    void setTool(const QString &toolName);

    // *** Segments

    /// Enables/disables display of the text labels on each segment.
    /**
     * From the menu: View > Show Segment Labels.
     *
     * @see drawCompRectLabel()
     */
    void setShowSegmentLabels(bool labels)  { m_showSegmentLabels = labels; }

    /// Set whether the segment items contain previews or not.
    /**
     * From the menu: View > Show Segment Previews.
     */
    void setShowPreviews(bool previews)  { m_showPreviews = previews; }

    /// Delegates to CompositionModelImpl::deleteCachedPreviews().
    /**
     * Redrawing previews is expensive.  This routine is primarily used
     * to indicate that the preview cache needs to be regenerated for
     * zoom and the commands (e.g. transpose).
     *
     * In a perfect world, the preview cache should be regenerated for
     * the following reasons only:
     *   1. The contents of a segment have changed.
     *   2. Zoom.
     *   3. Volume for an audio segment has changed.
     *   4. More?
     */
    void deleteCachedPreviews();

    /// Delegates to CompositionModelImpl::setAudioPeaksThread().
    /**
     * This is called when the document is being destroyed or the
     * application is going down.
     */
    void endAudioPreviewGeneration();

    // *** Selection

    /// Selects the segments via CompositionModelImpl::setSelected().
    /**
     * Used by RosegardenMainViewWidget.
     */
    void selectSegments(const SegmentSelection &segments);

    /// Delegates to CompositionModelImpl::getSelectedSegments().
    /**
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

    // *** Artifacts

    /// Move the playback position pointer to a new X coordinate.
    /**
     * @see getPointerPos(), setPointerPosition(), and drawArtifacts()
     */
    void drawPointer(int pos);
    /// Get the X coordinate of the playback position pointer.
    /**
     * @see drawPointer() and setPointerPosition()
     */
    int getPointerPos() { return m_pointerPos; }

    /**
     * The guides are blue crosshairs that stretch across the entire view.
     * They appear when selecting or moving a segment.
     *
     * @see hideGuides() and drawArtifacts()
     */
    void drawGuides(int x, int y);
    void hideGuides();

    /// Set the starting position of the "rubber band" selection.
    void drawSelectionRectPos1(const QPoint &pos);
    /// Set the ending position of the "rubber band" selection.
    void drawSelectionRectPos2(const QPoint &pos);
    /// Hide the "rubber band" selection rectangle.
    void hideSelectionRect();

    void setNewSegmentColor(const QColor &color)  { m_newSegmentColor = color; }
    /// Used by SegmentPencil to draw a new segment.
    void drawNewSegment(const QRect &r);
    const QRect &getNewSegmentRect() const  { return m_newSegmentRect; }
    void hideNewSegment()  { drawNewSegment(QRect()); }

    /**
     * Used by SegmentMover::mouseMoveEvent() to display time,
     * bar, and beat on the view while the user is moving a segment.
     * Also used by SegmentSelector::mouseMoveEvent() for the same
     * purpose.
     *
     * @see hideTextFloat()
     */
    void drawTextFloat(int x, int y, const QString &text);
    /// See drawTextFloat().
    void hideTextFloat()  { m_drawTextFloat = false; }

    /**
     * Used by SegmentSplitter.
     *
     * @see hideSplitLine()
     */
    void drawSplitLine(int x, int y);
    /// See drawSplitLine().
    void hideSplitLine();

public slots:

    /// Handle TrackButtons scroll wheel events.
    /**
     * Forwards the events to RosegardenScrollView to make sure we are
     * synced up with TrackButtons.
     *
     * @see TrackEditor::m_trackButtonScroll
     */
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

signals:
    /// Signal emitted when a segment is double-clicked.
    /**
     * Causes the default editor to be launched.
     *
     * Connected to RosegardenMainViewWidget::slotEditSegment().
     *
     * rename: segmentDoubleClicked()?
     */
    void editSegment(Segment *);

    /// Signal emitted when a segment repeat is double-clicked.
    /**
     * Connected to RosegardenMainViewWidget::slotEditRepeat() which converts
     * the repeat to a segment.  This doesn't actually start the segment
     * editor.  mouseDoubleClickEvent() emits editSegment() after
     * it emits this.
     *
     * rename: repeatDoubleClicked()?
     */
    void editRepeat(Segment *, timeT);

    /// Signal emitted when a double-click occurs on the background.
    /**
     * Causes the pointer to move to where the user double-clicked.
     *
     * Connected to RosegardenDocument::slotSetPointerPosition().
     * Connection is made by the RosegardenMainViewWidget ctor.
     *
     * @see drawPointer()
     *
     * rename: backgroundDoubleClicked()?
     */
    void setPointerPosition(timeT);

    /// Signal emitted when the context help needs to change.
    /**
     * The context help is displayed in the status bar at the bottom
     * of the main window.
     *
     * Connected to RosegardenMainWindow::slotShowToolHelp().
     *
     * rename: contextHelpChanged()
     */
    void showContextHelp(const QString &);

private slots:

    // *** Update

    /// Updates the artifacts in the entire viewport.
    /**
     * In addition to being used locally several times, this is also
     * connected to CompositionModelImpl::needArtifactsUpdate().
     */
    void slotUpdateArtifacts() {
        updateContents();
    }

    /// Used to reduce the frequency of updates.
    /**
     * slotAllNeedRefresh(rect) sets the m_updateNeeded flag to
     * tell slotUpdateTimer() that it needs to perform an update.
     */
    void slotUpdateTimer();

    /// Redraw everything with the new color scheme.
    /**
     * Connected to RosegardenDocument::docColoursChanged().
     */
    void slotRefreshColourCache();

    /// Updates the tool context help and shows it if the mouse is in the view.
    /**
     * The tool context help appears in the status bar at the bottom.
     *
     * Connected to SegmentToolBox::showContextHelp().
     *
     * @see showContextHelp()
     */
    void slotToolHelpChanged(const QString &);

    // *** Recording

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

    /**
     * Delegates to CompositionModelImpl::clearRecordingItems().
     * Connected to RosegardenDocument::stoppedAudioRecording() and
     * RosegardenDocument::stoppedMIDIRecording().
     */
    void slotStoppedRecording();

    /// Connected to InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

private:

    CompositionModelImpl *m_model;

    // --- Event Handlers ---------------------------------

    /// Redraw in response to AudioPeaksThread::AudioPeaksQueueEmpty.
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

    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);

    /// Delegates to drawAll().
    virtual void paintEvent(QPaintEvent *);
    /// Handles resize.  Uses slotUpdateSize().
    virtual void resizeEvent(QResizeEvent *);

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

    // --- Segments ---------------------------------------

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

    /// Scroll and refresh the segment layer (m_segmentsLayer) if needed.
    /**
     * Used by drawAll().
     */
    void scrollSegmentsLayer();
    int m_lastContentsX;
    int m_lastContentsY;

    /// Portion of the viewport that needs segments refreshed.
    /**
     * Used only by scrollSegmentsLayer() to limit work done redrawing
     * the segment rectangles.
     */
    QRect m_segmentsRefresh;

    /// Draw the segments on the m_segmentsLayer.
    /**
     * Draws the background then calls drawSegments() to draw the segments on the
     * segments layer (m_segmentsLayer).  Used by
     * scrollSegmentsLayer().
     */
    void drawSegments(const QRect &);
    QPixmap m_backgroundPixmap;

    /// Draw the track dividers.
    void drawTrackDividers(QPainter *segmentsLayerPainter, const QRect &clipRect);
    const QColor m_trackDividerColor;

    /// Draw a rectangle on the given painter.
    /**
     * This is QPainter::drawRect() with special handling of colors.  It
     * darkens the color of the segment based on isSelected and intersectLvl.
     *
     * @see drawCompRect()
     */
    void drawRect(QPainter *painter, const QRect &clipRect, const QRect &rect,
                  bool isSelected = false, int intersectLvl = 0);

    /// A version of drawRect() that handles segment repeats.
    void drawCompRect(QPainter *painter, const QRect &clipRect,
                      const SegmentRect &rect, int intersectLvl = 0);

    /// Used by drawSegments() to draw the segment labels.
    /**
     * @see setShowSegmentLabels()
     */
    void drawCompRectLabel(QPainter *painter,
                           const SegmentRect &rect);
    /// Used by drawCompRectLabel() to draw a halo around a text label.
    std::vector<QPoint> m_haloOffsets;

    /// Used by drawSegments() to draw any intersections between rectangles.
    void drawIntersections(QPainter *painter, const QRect &clipRect,
                           const CompositionModelImpl::SegmentRects &rects);

    /// Draw the previews for audio segments on the m_segmentsLayer.
    /**
     * Used by drawSegments().
     */
    void drawAudioPreviews(QPainter *segmentsLayerPainter, const QRect &clipRect);

    /// drawImage() for tiled audio previews.
    /**
     * This routine hides the fact that audio previews are stored as a
     * series of QImage tiles.  It treats them as if they were one large
     * QImage.  This simplifies drawAudioPreviews().
     */
    void drawImage(
            QPainter *painter,
            QPoint dest, const CompositionModelImpl::QImageVector &tileVector,
            QRect source);

    bool m_showPreviews;
    bool m_showSegmentLabels;

    /// Layer that contains the segment rectangles.
    /**
     * @see drawAll() and drawSegments()
     */
    QPixmap m_segmentsLayer;

    /// Used by drawSegments().
    /**
     * This is a std::vector with one element per segment.  Each element
     * (a RectRange) contains a pair of iterators into a vector of
     * preview QRects.
     */
    CompositionModelImpl::NotationPreviewRanges m_notationPreview;
    /// Set by drawSegments(), used by drawAudioPreviews()
    CompositionModelImpl::AudioPreviews m_audioPreview;

    /// Drives slotUpdateTimer().
    QTimer m_updateTimer;
    /// Let slotUpdateTimer() know that audio previews need to be cleared.
    /**
     * Note that you'll also want to set m_updateNeeded and m_updateRect so
     * that the display actually gets updated.  Use slotAllNeedRefresh() for
     * that.
     */
    bool m_deleteAudioPreviewsNeeded;
    /// Lets slotUpdateTimer() know that segments need to be redrawn.
    bool m_updateNeeded;
    /// Accumulated update rectangle.
    QRect m_updateRect;

    // --- Artifacts --------------------------------------

    /// Updates the artifacts in the given rect.
    void updateArtifacts(const QRect &r) {
        updateContents(r);
    }

    /// Draw the artifacts on the viewport.
    /**
     * "Artifacts" include anything that isn't a segment.  E.g. The playback
     * position pointer, guides, and the "rubber band" selection.
     * Used by drawAll().
     */
    void drawArtifacts();

    /// Used by drawArtifacts() to draw floating text.
    /**
     * @see setTextFloat()
     */
    void drawTextFloat(QPainter *painter);
    bool m_drawTextFloat;
    QString m_textFloatText;
    QPoint m_textFloatPos;

    // Playback Position Pointer
    int m_pointerPos;
    const QPen m_pointerPen;

    // New Segment
    QRect m_newSegmentRect;
    QColor m_newSegmentColor;

    // Split Line
    QPoint m_splitLinePos;

    // Guides
    bool m_drawGuides;
    const QColor m_guideColor;
    int m_guideX;
    int m_guideY;

    // Selection Rect (rubber band)
    bool m_drawSelectionRect;
    QRect m_selectionRect;

    // --- Both Segments and Artifacts --------------------

    /// Update segments and artifacts within rect.
    void updateAll2(const QRect &rect);

    /// Update segments and artifacts within the entire viewport.
    void updateAll() {
        segmentsNeedRefresh();
        slotUpdateArtifacts();
    }

    /// Draw the segments and artifacts on the viewport (screen).
    void drawAll();

    // --- Tools ------------------------------------------

    SegmentToolBox *m_toolBox;
    /// The tool that receives mouse events.
    SegmentTool *m_currentTool;

    QString m_toolContextHelp;
    bool m_contextHelpShown;

    // --- DEBUG ------------------------------------------

    /// Performance testing.
    bool m_enableDrawing;
};


}

#endif
