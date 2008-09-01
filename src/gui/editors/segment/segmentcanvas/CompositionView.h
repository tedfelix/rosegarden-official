
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
class RosegardenGUIDoc;
class CompositionRect;


class CompositionView : public RosegardenScrollView 
{
    Q_OBJECT
public:
    CompositionView(RosegardenGUIDoc*, CompositionModel*,
                    QWidget * parent=0, const char* name=0, WFlags f=0);

    void setPointerPos(int pos);
    int getPointerPos() { return m_pointerPos; }

    void setGuidesPos(int x, int y);
    void setGuidesPos(const QPoint& p);
    void setDrawGuides(bool d);

    QRect getSelectionRect() const { return m_selectionRect; }
    void setSelectionRectPos(const QPoint& pos);
    void setSelectionRectSize(int w, int h);
    void setDrawSelectionRect(bool d);

    SnapGrid& grid() { return m_model->grid(); }

    CompositionItem getFirstItemAt(QPoint pos);

    SegmentToolBox* getToolBox() { return m_toolBox; }

    CompositionModel* getModel() { return m_model; }

    void setTmpRect(const QRect& r);
    void setTmpRect(const QRect& r, const QColor &c);
    const QRect& getTmpRect() const { return m_tmpRect; }

    /**
     * Set the snap resolution of the grid to something suitable.
     * 
     * fineTool indicates whether the current tool is a fine-grain sort
     * (such as the resize or move tools) or a coarse one (such as the
     * segment creation pencil).  If the user is requesting extra-fine
     * resolution (through the setFineGrain method) that will also be
     * taken into account.
     */
    void setSnapGrain(bool fine);

    /**
     * Find out whether the user is requesting extra-fine resolution
     * (e.g. by holding Shift key).  This is seldom necessary -- most
     * client code will only need to query the snap grid that is
     * adjusted appropriately by the view when interactions take
     * place.
     */
    bool isFineGrain() const { return m_fineGrain; }

    /**
     * Find out whether the user is requesting to draw over an existing segment
     * with the pencil, by holding the Ctrl key.  This is used by the segment
     * pencil to decide whether to abort or not if a user attempts to draw over
     * an existing segment, and this is all necessary in order to avoid breaking
     * the double-click-to-open behavior.
     */
    bool pencilOverExisting() const { return m_pencilOverExisting; }

    /**
     * Set whether the segment items contain previews or not
     */
    void setShowPreviews(bool previews) { m_showPreviews = previews; }

    /**
     * Return whether the segment items contain previews or not
     */
    bool isShowingPreviews() { return m_showPreviews; }

    /**
     * clear all seg rect cache
     */
    void clearSegmentRectsCache(bool clearPreviews = false);

    /// Return the selected Segments if we're currently using a "Selector"
    SegmentSelection getSelectedSegments();

    bool haveSelection() const { return m_model->haveSelection(); }

    void updateSelectionContents();

    /**
     * Set and hide a text float on this canvas - it can contain
     * anything and can be left to timeout or you can hide it
     * explicitly.
     *
     */
    void setTextFloat(int x, int y, const QString &text);
    void hideTextFloat() { m_drawTextFloat = false; }

    void setShowSegmentLabels(bool b) { m_showSegmentLabels = b; }

    void setBackgroundPixmap(const QPixmap &m);

    void endAudioPreviewGeneration();

public slots:
    void scrollRight();
    void scrollLeft();
    void slotContentsMoving(int x, int y);

    /// Set the current segment editing tool
    void slotSetTool(const QString& toolName);

    // This method only operates if we're of the "Selector"
    // tool type - it's called from the View to enable it
    // to automatically set the selection of Segments (say
    // by Track).
    //
    void slotSelectSegments(const SegmentSelection &segment);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually SHIFT) and
    // selection copy (usually CONTROL)
    //
    void slotSetSelectAdd(bool value);
    void slotSetSelectCopy(bool value);

    void slotSetFineGrain(bool value);
    void slotSetPencilOverExisting(bool value);

    // Show and hige the splitting line on a Segment
    //
    void slotShowSplitLine(int x, int y);
    void slotHideSplitLine();

    void slotExternalWheelEvent(QWheelEvent*);

    // TextFloat timer
    void slotTextFloatTimeout();

    void slotUpdateSegmentsDrawBuffer();
    void slotUpdateSegmentsDrawBuffer(const QRect&);

    void slotRefreshColourCache();

    void slotNewMIDIRecordingSegment(Segment*);
    void slotNewAudioRecordingSegment(Segment*);
    // no longer used, see RosegardenGUIDoc::insertRecordedMidi
//     void slotRecordMIDISegmentUpdated(Segment*, timeT updatedFrom);
    void slotStoppedRecording();

    void slotUpdateSize();

signals:
    void editSegment(Segment*); // use default editor
    void editSegmentNotation(Segment*);
    void editSegmentMatrix(Segment*);
    void editSegmentAudio(Segment*);
    void editSegmentEventList(Segment*);
    void audioSegmentAutoSplit(Segment*);
    void editRepeat(Segment*, timeT);

    void setPointerPosition(timeT);

    void showContextHelp(const QString &);

protected:
    virtual bool event(QEvent *);

    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    virtual void viewportPaintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);

    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    virtual void viewportPaintRect(QRect);
    
    /**
     * if something changed, returns true and sets rect accordingly
     * sets 'scroll' if some scrolling occurred
     */
    bool checkScrollAndRefreshDrawBuffer(QRect &, bool& scroll);
    void refreshSegmentsDrawBuffer(const QRect&);
    void refreshArtifactsDrawBuffer(const QRect&);
    void drawArea(QPainter * p, const QRect& rect);
    void drawAreaAudioPreviews(QPainter * p, const QRect& rect);
    void drawAreaArtifacts(QPainter * p, const QRect& rect);
    void drawRect(const QRect& rect, QPainter * p, const QRect& clipRect,
                  bool isSelected = false, int intersectLvl = 0, bool fill = true);
    void drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                      int intersectLvl = 0, bool fill = true);
    void drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect);
    void drawIntersections(const CompositionModel::rectcontainer&, QPainter * p, const QRect& clipRect);

    void drawPointer(QPainter * p, const QRect& clipRect);
    void drawGuides(QPainter * p, const QRect& clipRect);
    void drawTextFloat(QPainter * p, const QRect& clipRect);

    void initStepSize();
    void releaseCurrentItem();

    static QColor mixBrushes(QBrush a, QBrush b);

    SegmentSelector* getSegmentSelectorTool();

protected slots:
    void slotSegmentsDrawBufferNeedsRefresh() {
        m_segmentsDrawBufferRefresh =
            QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
    }

    void slotSegmentsDrawBufferNeedsRefresh(QRect r) {
        m_segmentsDrawBufferRefresh |=
            (QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight())
             & r);
    }

    void slotArtifactsDrawBufferNeedsRefresh() {
        m_artifactsDrawBufferRefresh = 
            QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
        updateContents();
    }

    void slotArtifactsDrawBufferNeedsRefresh(QRect r) {
        m_artifactsDrawBufferRefresh |=
            (QRect(contentsX(), contentsY(), visibleWidth(), visibleHeight())
             & r);
        updateContents(r);
    }

    void slotAllDrawBuffersNeedRefresh() {
        slotSegmentsDrawBufferNeedsRefresh();
        slotArtifactsDrawBufferNeedsRefresh();
    }

    void slotAllDrawBuffersNeedRefresh(QRect r) {
        slotSegmentsDrawBufferNeedsRefresh(r);
        slotArtifactsDrawBufferNeedsRefresh(r);
    }

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

    QPixmap      m_segmentsDrawBuffer;
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
