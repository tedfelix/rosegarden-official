// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef COMPOSITIONVIEW_H
#define COMPOSITIONVIEW_H

#include <vector>
#include <set>
#include <map>

#include <qpen.h>
#include <qvaluevector.h>
#include <qptrdict.h>
#include <qpixmap.h>
#include <qimage.h>

#include "Event.h"
#include "Composition.h"
#include "SnapGrid.h"

#include "rosegardenscrollview.h"
#include "segmenttool.h"
#include "compositionitem.h"

#include "rosedebug.h"

class CompositionRect : public QRect
{
public:
    typedef QValueVector<int> repeatmarks;

    friend bool operator<(const CompositionRect&, const CompositionRect&);

    CompositionRect() : QRect(), m_selected(false),
                        m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};
    CompositionRect(const QRect& r) : QRect(r), m_selected(false),
                                      m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};
    CompositionRect(const QPoint & topLeft, const QPoint & bottomRight)
        : QRect(topLeft, bottomRight), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};
    CompositionRect(const QPoint & topLeft, const QSize & size)
        : QRect(topLeft, size), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};
    CompositionRect(int left, int top, int width, int height)
        : QRect(left, top, width, height), m_selected(false),
          m_needUpdate(false), m_brush(DefaultBrushColor), m_pen(DefaultPenColor) {};

    void setSelected(bool s)      { m_selected = s; }
    bool isSelected() const       { return m_selected; }
    bool needsFullUpdate() const  { return m_needUpdate; }
    void setNeedsFullUpdate(bool s) { m_needUpdate = s; }

    // brush, pen draw info
    void setBrush(QBrush b)       { m_brush = b; }
    QBrush getBrush() const       { return m_brush; }
    void setPen(QPen b)           { m_pen = b; }
    QPen getPen() const           { return m_pen; }

    // repeating segments
    void                setRepeatMarks(const repeatmarks& rm) { m_repeatMarks = rm; }
    const repeatmarks&  getRepeatMarks() const                { return m_repeatMarks; }
    bool                isRepeating() const                   { return m_repeatMarks.size() > 0; }
    int                 getBaseWidth() const                  { return m_baseWidth; }
    void                setBaseWidth(int bw)                  { m_baseWidth = bw; }
    QString             getLabel() const                      { return m_label; }
    void                setLabel(QString l)                   { m_label = l; }

    static const QColor DefaultPenColor;
    static const QColor DefaultBrushColor;

protected:
    bool        m_selected;
    bool        m_needUpdate;
    QBrush      m_brush;
    QPen        m_pen;
    repeatmarks m_repeatMarks;
    int         m_baseWidth;
    QString     m_label;
};

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

typedef std::vector<QImage> PixmapArray;


class CompositionModel : public QObject, public Rosegarden::CompositionObserver, public Rosegarden::SegmentObserver
{
    Q_OBJECT
public:

    struct CompositionItemCompare {
	bool operator()(const CompositionItem &c1, const CompositionItem &c2) const {
	    return c1->hashKey() < c2->hashKey();
	}
    };

    typedef std::vector<QRect> rectlist;
    typedef std::vector<CompositionRect> rectcontainer;
    typedef std::set<CompositionItem, CompositionItemCompare> itemcontainer;

    struct AudioPreviewDrawDataItem {
        AudioPreviewDrawDataItem(PixmapArray p, QPoint bp, QRect r) : pixmap(p), basePoint(bp), rect(r) {};
        PixmapArray pixmap;
        QPoint basePoint;
        QRect rect;
    };
    
    typedef std::vector<AudioPreviewDrawDataItem> AudioPreviewDrawData;

    struct RectRange {
        std::pair<rectlist::iterator, rectlist::iterator> range;
        QPoint basePoint;
        QColor color;
    };

    typedef std::vector<RectRange> RectRanges;

    class AudioPreviewData {
    public:
        AudioPreviewData(bool showMinima, unsigned int channels) : m_showMinima(showMinima), m_channels(channels) {};
	// ~AudioPreviewData();

        bool showsMinima()              { return m_showMinima; }
        void setShowMinima(bool s)      { m_showMinima = s;    }

        unsigned int getChannels()       { return m_channels;   }
        void setChannels(unsigned int c) { m_channels = c;      }

        const std::vector<float> &getValues() const { return m_values;  }
	void setValues(const std::vector<float>&v) { m_values = v; }

        QRect getSegmentRect()              { return m_segmentRect; }
        void setSegmentRect(const QRect& r) { m_segmentRect = r; }

    protected:
        std::vector<float> m_values;
        bool               m_showMinima;
        unsigned int       m_channels;
        QRect              m_segmentRect;

    private:
        // no copy ctor
        AudioPreviewData(const AudioPreviewData&);
    };


    virtual ~CompositionModel() {};

    virtual unsigned int getNbRows() = 0;
    virtual const rectcontainer& getRectanglesIn(const QRect& rect,
                                                 RectRanges* notationRects, AudioPreviewDrawData* audioRects) = 0;

    virtual itemcontainer     getItemsAt      (const QPoint&) = 0;
    virtual Rosegarden::timeT getRepeatTimeAt (const QPoint&, const CompositionItem&) = 0;

    virtual Rosegarden::SnapGrid& grid() = 0;

    virtual void setPointerPos(int xPos) = 0;
    virtual void setSelected(const CompositionItem&, bool selected = true) = 0;
    virtual bool isSelected(const CompositionItem&) const = 0;
    virtual void setSelected(const itemcontainer&) = 0;
    virtual void clearSelected() = 0;
    virtual bool haveSelection() const = 0;
    virtual void signalSelection() = 0;
    virtual void setSelectionRect(const QRect&) = 0;
    virtual void finalizeSelectionRect() = 0;
    virtual QRect getSelectionContentsRect() = 0;
    virtual void signalContentChange() = 0;

    virtual void addRecordingItem(const CompositionItem&) = 0;
    virtual void removeRecordingItem(const CompositionItem&) = 0;
    virtual void clearRecordingItems() = 0;
    virtual bool haveRecordingItems() = 0;

    virtual void startMove(const CompositionItem&) = 0;
    virtual void startMoveSelection() = 0;
    virtual itemcontainer& getMovingItems() = 0;
    virtual void endMove() = 0;

    virtual void setLength(int width) = 0;
    virtual int  getLength() = 0;

signals:
    void needContentUpdate();
    void needContentUpdate(const QRect&);
    void needArtifactsUpdate();

protected:
    CompositionItem* m_currentCompositionItem;
};

namespace Rosegarden { class Segment; class Studio; class SnapGrid; class RulerScale; }
class AudioPreviewThread;
class AudioPreviewUpdater;

class CompositionModelImpl : public CompositionModel
{
    Q_OBJECT
public:

    CompositionModelImpl(Rosegarden::Composition& compo,
                         Rosegarden::Studio& studio,
                         Rosegarden::RulerScale *rulerScale,
                         int vStep);

    virtual ~CompositionModelImpl();
    
    virtual unsigned int getNbRows();
    virtual const rectcontainer& getRectanglesIn(const QRect& rect,
                                                 RectRanges* notationRects, AudioPreviewDrawData* audioRects);
    virtual itemcontainer     getItemsAt      (const QPoint&);
    virtual Rosegarden::timeT getRepeatTimeAt (const QPoint&, const CompositionItem&);

    virtual Rosegarden::SnapGrid& grid() { return m_grid; }

    virtual void setPointerPos(int xPos);
    virtual void setSelected(const CompositionItem&, bool selected = true);
    virtual bool isSelected(const CompositionItem&) const;
    virtual void setSelected(const itemcontainer&);
    virtual void clearSelected();
    virtual bool haveSelection() const { return !m_selectedSegments.empty(); }
    virtual void signalSelection();
    virtual void setSelectionRect(const QRect&);
    virtual void finalizeSelectionRect();
    virtual QRect getSelectionContentsRect();
    virtual void signalContentChange();

    virtual void addRecordingItem(const CompositionItem&);
    virtual void removeRecordingItem(const CompositionItem &);
    virtual void clearRecordingItems();
    virtual bool haveRecordingItems() { return m_recordingSegments.size() > 0; }

    virtual void startMove(const CompositionItem&);
    virtual void startMoveSelection();
    virtual itemcontainer& getMovingItems() { return m_movingItems; }
    virtual void endMove();

    virtual void setLength(int width);
    virtual int  getLength();

    void setAudioPreviewThread(AudioPreviewThread& thread);
    AudioPreviewThread* getAudioPreviewThread() { return m_audioPreviewThread; }

    void clearPreviewCache();
    void clearSegmentRectsCache(bool clearPreviews = false) { clearInCache(0, clearPreviews); }

    rectlist*            makeNotationPreviewDataCache(const Rosegarden::Segment *s);
    AudioPreviewData*    makeAudioPreviewDataCache(const Rosegarden::Segment *s);

    CompositionRect computeSegmentRect(const Rosegarden::Segment&);
    QColor          computeSegmentPreviewColor(const Rosegarden::Segment*);
    QPoint          computeSegmentOrigin(const Rosegarden::Segment&);
    void            computeRepeatMarks(CompositionItem&);

    Rosegarden::SegmentSelection getSelectedSegments() { return m_selectedSegments; }
    Rosegarden::Composition&     getComposition()      { return m_composition; }
    Rosegarden::Studio&          getStudio()           { return m_studio; }


    // CompositionObserver
    virtual void segmentAdded(const Rosegarden::Composition *, Rosegarden::Segment *);
    virtual void segmentRemoved(const Rosegarden::Composition *, Rosegarden::Segment *);
    virtual void segmentRepeatChanged(const Rosegarden::Composition *, Rosegarden::Segment *, bool);

    // SegmentObserver
    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void appearanceChanged(const Rosegarden::Segment *);
    virtual void endMarkerTimeChanged(const Rosegarden::Segment *, bool /*shorten*/);
    virtual void segmentDeleted(const Rosegarden::Segment*) { /* nothing to do - handled by CompositionObserver::segmentRemoved() */ };

signals:
    void selectedSegments(const Rosegarden::SegmentSelection &);

public slots:
    void slotAudioFileFinalized(Rosegarden::Segment*);
    void slotInstrumentParametersChanged(Rosegarden::InstrumentId);

protected slots:
    void slotAudioPreviewComplete(AudioPreviewUpdater*);

protected:
    void setSelected(const Rosegarden::Segment*, bool selected = true);
    bool isSelected(const Rosegarden::Segment*) const;
    bool isTmpSelected(const Rosegarden::Segment*) const;
    bool wasTmpSelected(const Rosegarden::Segment*) const;
    bool isMoving(const Rosegarden::Segment*) const;
    bool isRecording(const Rosegarden::Segment*) const;
    
    void computeRepeatMarks(CompositionRect& sr, const Rosegarden::Segment* s);
    void updatePreviewCacheForNotationSegment(const Rosegarden::Segment* s, rectlist*);
    void updatePreviewCacheForAudioSegment(const Rosegarden::Segment* s, AudioPreviewData*);
    rectlist* getNotationPreviewData(const Rosegarden::Segment* s);
    AudioPreviewData* getAudioPreviewData(const Rosegarden::Segment* s);
    PixmapArray getAudioPreviewPixmap(const Rosegarden::Segment* s);
    QRect postProcessAudioPreview(AudioPreviewData*, const Rosegarden::Segment*);

    void makePreviewCache(const Rosegarden::Segment* s);
    void removePreviewCache(const Rosegarden::Segment* s);
    void makeNotationPreviewRects(RectRanges* npData, QPoint basePoint, const Rosegarden::Segment*, const QRect&);
    void makeAudioPreviewRects(AudioPreviewDrawData* apRects, const Rosegarden::Segment*,
                               const CompositionRect& segRect, const QRect& clipRect);

    void clearInCache(const Rosegarden::Segment*, bool clearPreviewCache = false);
    void putInCache(const Rosegarden::Segment*, const CompositionRect&);
    const CompositionRect& getFromCache(const Rosegarden::Segment*, Rosegarden::timeT& endTime);
    bool isCachedRectCurrent(const Rosegarden::Segment& s, const CompositionRect& r,
                             QPoint segmentOrigin, Rosegarden::timeT segmentEndTime);

    //--------------- Data members ---------------------------------
    Rosegarden::Composition&     m_composition;
    Rosegarden::Studio&          m_studio;
    Rosegarden::SnapGrid         m_grid;
    Rosegarden::SegmentSelection m_selectedSegments;
    Rosegarden::SegmentSelection m_tmpSelectedSegments;
    Rosegarden::SegmentSelection m_previousTmpSelectedSegments;

    Rosegarden::timeT            m_pointerTimePos;

    typedef std::set<Rosegarden::Segment *> recordingsegmentset;
    recordingsegmentset          m_recordingSegments;

    typedef std::vector<CompositionItem> itemgc;

    AudioPreviewThread*          m_audioPreviewThread;

    typedef QPtrDict<rectlist> NotationPreviewDataCache;
    typedef QPtrDict<AudioPreviewData>    AudioPreviewDataCache;

    NotationPreviewDataCache     m_notationPreviewDataCache;
    AudioPreviewDataCache        m_audioPreviewDataCache;

    rectcontainer m_res;
    itemcontainer m_movingItems;
    itemgc m_itemGC;

    QRect m_selectionRect;
    QRect m_previousSelectionUpdateRect;

    std::map<const Rosegarden::Segment*, CompositionRect> m_segmentRectMap;
    std::map<const Rosegarden::Segment*, Rosegarden::timeT> m_segmentEndTimeMap;
    std::map<const Rosegarden::Segment*, PixmapArray> m_audioSegmentPreviewMap;
    
    typedef std::map<const Rosegarden::Segment*, AudioPreviewUpdater *>
        AudioPreviewUpdaterMap;
    AudioPreviewUpdaterMap m_audioPreviewUpdaterMap;
};


class CompositionItemImpl : public _CompositionItem {
public:
    CompositionItemImpl(Rosegarden::Segment& s, const CompositionRect&);
    virtual bool isRepeating() const              { return m_rect.isRepeating(); }
    virtual QRect rect() const;
    virtual void moveBy(int x, int y)             { m_rect.moveBy(x, y); }
    virtual void moveTo(int x, int y)             { m_rect.setRect(x, y, m_rect.width(), m_rect.height()); }
    virtual void setX(int x)                      { m_rect.setX(x); }
    virtual void setY(int y)                      { m_rect.setY(y); }
    virtual void setWidth(int w)                  { m_rect.setWidth(w); }
    // use segment address as hash key
    virtual long hashKey()                        { return (long)getSegment(); }

    Rosegarden::Segment* getSegment()             { return &m_segment; }
    const Rosegarden::Segment* getSegment() const { return &m_segment; }
    CompositionRect& getCompRect()                { return m_rect; }

protected:

    //--------------- Data members ---------------------------------
    Rosegarden::Segment& m_segment;
    CompositionRect m_rect;
};

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

    Rosegarden::SnapGrid& grid() { return m_model->grid(); }

    CompositionItem getFirstItemAt(QPoint pos);

    /**
     * Add the given Segment to the selection, if we know anything about it
     */
    void addToSelection(Rosegarden::Segment *);

    SegmentToolBox* getToolBox() { return m_toolBox; }

    CompositionModel* getModel() { return m_model; }

    void setTmpRect(const QRect& r);
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
    Rosegarden::SegmentSelection getSelectedSegments();

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

    void updateSize(bool shrinkWidth=false);

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
    void slotSelectSegments(const Rosegarden::SegmentSelection &segment);

    // These are sent from the top level app when it gets key
    // depresses relating to selection add (usually SHIFT) and
    // selection copy (usually CONTROL)
    //
    void slotSetSelectAdd(bool value);
    void slotSetSelectCopy(bool value);

    void slotSetFineGrain(bool value);

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

    void slotNewMIDIRecordingSegment(Rosegarden::Segment*);
    void slotNewAudioRecordingSegment(Rosegarden::Segment*);
    // no longer used, see RosegardenGUIDoc::insertRecordedMidi
//     void slotRecordMIDISegmentUpdated(Rosegarden::Segment*, Rosegarden::timeT updatedFrom);
    void slotStoppedRecording();

signals:
    void editSegment(Rosegarden::Segment*); // use default editor
    void editSegmentNotation(Rosegarden::Segment*);
    void editSegmentMatrix(Rosegarden::Segment*);
    void editSegmentAudio(Rosegarden::Segment*);
    void editSegmentEventList(Rosegarden::Segment*);
    void audioSegmentAutoSplit(Rosegarden::Segment*);
    void editRepeat(Rosegarden::Segment*, Rosegarden::timeT);

    void selectedSegments(const Rosegarden::SegmentSelection &);
    
protected:
    virtual bool event(QEvent *);

    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    virtual void viewportPaintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);

    virtual void viewportPaintRect(QRect);
    
    // if something changed, returns true and sets rect accordingly
    bool checkScrollAndRefreshDrawBuffer(QRect &);
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
	m_artifactsDrawBufferNeedsRefresh = true; 
    }

    void slotAllDrawBuffersNeedRefresh() {
	m_artifactsDrawBufferNeedsRefresh = true;
	slotSegmentsDrawBufferNeedsRefresh();
    }

    void slotAllDrawBuffersNeedRefresh(QRect r) {
	m_artifactsDrawBufferNeedsRefresh = true;
	slotSegmentsDrawBufferNeedsRefresh(r);
    }

protected:         

    //--------------- Data members ---------------------------------

    CompositionModel* m_model;
    CompositionItem m_currentItem;

    SegmentTool*    m_tool;
    SegmentToolBox* m_toolBox;

    bool         m_showPreviews;
    bool         m_showSegmentLabels;
    bool         m_fineGrain;

    int          m_minWidth;

    int          m_stepSize;
    QColor       m_rectFill;
    QColor       m_selectedRectFill;

    int          m_pointerPos;
    QColor       m_pointerColor;
    int          m_pointerWidth;
    QPen         m_pointerPen;

    QRect        m_tmpRect;
    QPoint       m_splitLinePos;

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
    bool         m_artifactsDrawBufferNeedsRefresh;
    int          m_lastBufferRefreshX;
    int          m_lastBufferRefreshY;
    int          m_lastPointerRefreshX;
    QPixmap      m_backgroundPixmap;

    mutable CompositionModel::AudioPreviewDrawData m_audioPreviewRects;
    mutable CompositionModel::RectRanges m_notationPreviewRects;
};

#endif
