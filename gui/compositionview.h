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

#include <qpen.h>
#include <qvaluevector.h>
#include <qptrdict.h>

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


class CompositionModel : public QObject, public Rosegarden::CompositionObserver, public Rosegarden::SegmentObserver
{
public:
    struct RectCompare {
	bool operator()(const PreviewRect &r1, const PreviewRect &r2) const {
	    return r1.x() < r2.x();
	}
    };

    typedef std::multiset<PreviewRect, RectCompare> RectList;

    typedef std::vector<CompositionRect> rectcontainer;
    typedef std::vector<CompositionItem> itemcontainer;

    typedef RectList NotationPreviewData;

    class AudioPreviewData {
    public:
        AudioPreviewData(bool showMinima, unsigned int channels) : m_showMinima(showMinima), m_channels(channels) {};
	~AudioPreviewData();

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
    virtual const rectcontainer& getRectanglesIn(const QRect& rect, RectList* notationRects, RectList* audioRects) = 0;

    virtual itemcontainer     getItemsAt      (const QPoint&) = 0;
    virtual Rosegarden::timeT getRepeatTimeAt (const QPoint&, const CompositionItem&) = 0;

    virtual Rosegarden::SnapGrid& grid() = 0;

    virtual void setSelected(const CompositionItem&, bool selected = true) = 0;
    virtual bool isSelected(const CompositionItem&) const = 0;
    virtual void setSelected(const itemcontainer&) = 0;
    virtual void clearSelected() = 0;
    virtual bool haveSelection() const = 0;
    virtual void signalSelection() = 0;
    virtual void setSelectionRect(const QRect&) = 0;
    virtual void finalizeSelectionRect() = 0;
    virtual QRect getSelectionContentsRect() = 0;

    virtual void addRecordingItem(const CompositionItem&) = 0;
    virtual void removeRecordingItem(const CompositionItem&) = 0;

    virtual void startMove(const CompositionItem&) = 0;
    virtual void startMoveSelection() = 0;
    virtual itemcontainer& getMovingItems() = 0;
    virtual void endMove() = 0;

    virtual void setLength(int width) = 0;
    virtual int  getLength() = 0;

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

    ~CompositionModelImpl();
    
    virtual unsigned int getNbRows();
    virtual const rectcontainer& getRectanglesIn(const QRect& rect, RectList* notationRects, RectList* audioRects);
    virtual itemcontainer     getItemsAt      (const QPoint&);
    virtual Rosegarden::timeT getRepeatTimeAt (const QPoint&, const CompositionItem&);

    virtual Rosegarden::SnapGrid& grid() { return m_grid; }

    virtual void setSelected(const CompositionItem&, bool selected = true);
    virtual bool isSelected(const CompositionItem&) const;
    virtual void setSelected(const itemcontainer&);
    virtual void clearSelected();
    virtual bool haveSelection() const { return !m_selectedSegments.empty(); }
    virtual void signalSelection();
    virtual void setSelectionRect(const QRect&);
    virtual void finalizeSelectionRect();
    virtual QRect getSelectionContentsRect();

    virtual void addRecordingItem(const CompositionItem&);
    virtual void removeRecordingItem(const CompositionItem &);

    virtual void startMove(const CompositionItem&);
    virtual void startMoveSelection();
    virtual itemcontainer& getMovingItems() { return m_movingItems; }
    virtual void endMove();

    virtual void setLength(int width);
    virtual int  getLength();

    void setAudioPreviewThread(AudioPreviewThread& thread);
    void refreshAllPreviews();
    void refreshDirtyPreviews();
    void clearPreviewCache();
    void clearDirtyPreviews();
    NotationPreviewData* makeNotationPreviewDataCache(const Rosegarden::Segment *s);
    AudioPreviewData*    makeAudioPreviewDataCache(const Rosegarden::Segment *s);

    CompositionRect computeSegmentRect(const Rosegarden::Segment&);
    QPoint computeSegmentOrigin(const Rosegarden::Segment&);

    Rosegarden::SegmentSelection getSelectedSegments() { return m_selectedSegments; }
    Rosegarden::Composition&     getComposition()      { return m_composition; }
    Rosegarden::Studio&          getStudio()           { return m_studio; }


    // CompositionObserver
    virtual void segmentAdded(const Rosegarden::Composition *, Rosegarden::Segment *);
    virtual void segmentRemoved(const Rosegarden::Composition *, Rosegarden::Segment *);

    // SegmentObserver
    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);

signals:
    void selectedSegments(const Rosegarden::SegmentSelection &);

protected slots:
    void slotAudioPreviewComplete(AudioPreviewUpdater*);

protected:
    void setSelected(const Rosegarden::Segment*, bool selected = true);
    bool isSelected(const Rosegarden::Segment*) const;
    bool isTmpSelected(const Rosegarden::Segment*) const;
    bool wasTmpSelected(const Rosegarden::Segment*) const;
    bool isMoving(const Rosegarden::Segment*) const;
    void computeRepeatMarks(CompositionRect& sr, const Rosegarden::Segment* s);
    void updatePreviewCacheForNotationSegment(const Rosegarden::Segment* s, NotationPreviewData*);
    void updatePreviewCacheForAudioSegment(const Rosegarden::Segment* s, AudioPreviewData*);
    NotationPreviewData* getNotationPreviewData(const Rosegarden::Segment* s);
    AudioPreviewData* getAudioPreviewData(const Rosegarden::Segment* s);

    void makePreviewCache(Rosegarden::Segment* s);
    void removePreviewCache(Rosegarden::Segment* s);
    void makeNotationPreviewRects(RectList* apData, int baseY, const Rosegarden::Segment* s, const QRect&);
    void makeAudioPreviewRects(RectList* apData, const Rosegarden::Segment* s, const QRect&);

    //--------------- Data members ---------------------------------
    Rosegarden::Composition&     m_composition;
    Rosegarden::Studio&          m_studio;
    Rosegarden::SnapGrid         m_grid;
    Rosegarden::SegmentSelection m_selectedSegments;
    Rosegarden::SegmentSelection m_tmpSelectedSegments;
    Rosegarden::SegmentSelection m_previousTmpSelectedSegments;

    typedef std::set<Rosegarden::Segment *> RecordingSegmentSet;
    RecordingSegmentSet          m_recordingSegments;

    AudioPreviewThread*          m_audioPreviewThread;

    typedef QPtrDict<NotationPreviewData> NotationPreviewDataCache;
    typedef QPtrDict<AudioPreviewData>    AudioPreviewDataCache;
    
    NotationPreviewDataCache     m_notationPreviewDataCache;
    AudioPreviewDataCache        m_audioPreviewDataCache;
    
    // Segments which preview needs to be updated
    std::set<const Rosegarden::Segment*> m_dirtySegments;

    rectcontainer m_res;
    itemcontainer m_movingItems;
    
    QRect m_selectionRect;
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

    Rosegarden::Segment* getSegment()             { return &m_segment; }
    const Rosegarden::Segment* getSegment() const { return &m_segment; }

protected:
    // recompute the repeat marks after an X-wise move
    void refreshRepeatMarks(int newX, int newWidth = -1);

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

    void setGuidesPos(int x, int y) { m_topGuidePos = x; m_foreGuidePos = y; }
    void setGuidesPos(const QPoint& p) { m_topGuidePos = p.x(); m_foreGuidePos = p.y(); }
    void setDrawGuides(bool d) { m_drawGuides = d; }

    QRect getSelectionRect() const { return m_selectionRect; }
    void setSelectionRectPos(const QPoint& pos);
    void setSelectionRectSize(int w, int h);
    void setDrawSelectionRect(bool d) { m_drawSelectionRect = d; }

    Rosegarden::SnapGrid& grid() { return m_model->grid(); }

    CompositionItem getFirstItemAt(QPoint pos);

    /**
     * Add the given Segment to the selection, if we know anything about it
     */
    void addToSelection(Rosegarden::Segment *);

    SegmentToolBox* getToolBox() { return m_toolBox; }

    CompositionModel* getModel() { return m_model; }

    void setTmpRect(const QRect& r) { m_tmpRect = r; }
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
     * Refresh all previews
     */
    void refreshAllPreviews();

    /**
     * Refresh previews of segments which have been modified since last refresh
     */
    void refreshDirtyPreviews();

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

    void slotUpdate();

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

    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    virtual void drawContents(QPainter * p, int clipx, int clipy, int clipw, int cliph);
    void drawRect(const QRect& rect, QPainter * p, const QRect& clipRect,
                  bool isSelected = false, int intersectLvl = 0, bool fill = true);
    void drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                      int intersectLvl = 0, bool fill = true);
    void drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect);
    void drawIntersections(const CompositionModel::rectcontainer&, QPainter * p, const QRect& clipRect);

    void drawPointer(QPainter * p, const QRect& clipRect);
    void drawGuides(QPainter * p, const QRect& clipRect);
    void drawTextFloat(QPainter * p, const QRect& clipRect);
    void pointerMoveUpdate(int oldPos = -1);

    void initStepSize();
    void releaseCurrentItem();

    static QColor mixBrushes(QBrush a, QBrush b);

    SegmentSelector* getSegmentSelectorTool();


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

    bool         m_2ndLevelUpdate;

    mutable CompositionModel::RectList m_audioPreviewRects;
    mutable CompositionModel::RectList m_notationPreviewRects;
};

#endif
