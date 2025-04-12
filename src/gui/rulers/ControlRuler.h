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

#ifndef RG_CONTROLRULER_H
#define RG_CONTROLRULER_H

#include "ControlItem.h"

#include "gui/general/AutoScroller.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "gui/general/ActionFileClient.h"

#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>

#include <utility>

//class QWidget;
class QMenu;
class QWheelEvent;
class QScrollBar;
class QMouseEvent;
class QContextMenuEvent;


namespace Rosegarden
{

class ControlTool;
class ControlToolBox;
class ControlSelector;
class ControlMouseEvent;
class Segment;
class RulerScale;
class EventSelection;
class EditViewBase;
class NotationStaff;
class ViewSegment;
class SnapGrid;

/**
 * ControlRuler : base class for Control Rulers
 */
class ControlRuler : public QWidget, public ActionFileClient
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(ViewSegment*,
                 RulerScale*,
                 QWidget* parent = nullptr);
    ~ControlRuler() override;

    virtual QString getName() = 0;

    QSize sizeHint() const override { return QSize(1,100); }

    void paintEvent(QPaintEvent *) override;

    long getMaxItemValue() const { return m_maxItemValue; }
    void setMaxItemValue(long val) { m_maxItemValue = val; }

    long getMinItemValue() const { return m_minItemValue; }
    void setMinItemValue(long val) { m_minItemValue = val; }

    void clear();

    //void setControlTool(ControlTool *);
    //int applyTool(double x, int val);
    virtual void setTool(const QString &name);

    ControlItemList *getSelectedItems() { return &m_selectedItems; }

    QRectF* getSelectionRectangle() { return m_selectionRect; }
    void setSelectionRect(QRectF *rect) { m_selectionRect = rect; }

    virtual void setSegment(Segment *);
    virtual void setViewSegment(ViewSegment *);
    Segment* getSegment() { return m_segment; }

    void updateSegment();

    virtual void notationLayoutUpdated(timeT,timeT);

    void setRulerScale(RulerScale *rulerscale) { m_rulerScale = rulerscale; }
    RulerScale* getRulerScale() { return m_rulerScale; }

    void setXOffset(int offset) { m_xOffset = offset; }

    float valueToY(long val);
    long yToValue(float y);

    double getXScale() const { return m_xScale; }
    double getYScale() const { return m_yScale; }
    float getXMax();
    float getXMin();

    void clearSelectedItems();
    void addToSelection(QSharedPointer<ControlItem>);
    void removeFromSelection(QSharedPointer<ControlItem>);
    EventSelection *getEventSelection()
    { return m_eventSelection; }

    virtual ControlItemMap::iterator findControlItem(float x);
    virtual void moveItem(ControlItem*);

    /// EventSelectionObserver
//    virtual void eventSelected(EventSelection *,Event *);
//    virtual void eventDeselected(EventSelection *,Event *);
//    virtual void eventSelectionDestroyed(EventSelection *);

//    void assignEventSelection(EventSelection *);

    // SegmentObserver interface
//    virtual void viewSegmentDeleted(const ViewSegment *);

    // unused void flipForwards();
    // unused void flipBackwards();

    SnapGrid* getSnapGrid() const;

    void setSnapFromEditor(timeT snapSetting, bool forceFromEditor);

    virtual bool allowSimultaneousEvents() = 0;

signals:
    void mousePress();
    void mouseMove(FollowMode);
    void mouseRelease();

    /**
     * Eventually ends up calling MatrixView::slotUpdateMenuStates().
     */
    void rulerSelectionChanged(EventSelection *);
    /// Special case for velocity ruler.
    /**
     * See the emitter, ControlRuler::updateSelection(), for details.
     */
    void rulerSelectionUpdate();

    void showContextHelp(const QString &);

public slots:
//    virtual void slotUpdateElementsHPos();
    // unused virtual void slotScrollHorizSmallSteps(int);
    virtual void slotSetPannedRect(QRectF);
//    virtual void slotSetScale(double);

    void slotSnap();

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void resizeEvent(QResizeEvent *) override;

    virtual ControlMouseEvent createControlMouseEvent(QMouseEvent* e);

//    virtual QScrollBar* getMainHorizontalScrollBar();

//    virtual void computeViewSegmentOffset() {};

//    virtual void layoutItem(ControlItem*);
    virtual ControlItemMap::iterator findControlItem(const ControlItem*);
    virtual ControlItemMap::iterator findControlItem(const Event*);
    virtual void addControlItem(QSharedPointer<ControlItem>);
    virtual void addCheckVisibleLimits(ControlItemMap::iterator);
    virtual void removeControlItem(ControlItem*);
    virtual void removeControlItem(const Event*);
    virtual void removeControlItem(const ControlItemMap::iterator&);
    virtual void removeCheckVisibleLimits(const ControlItemMap::iterator&);
    virtual void eraseControlItem(const Event*);
    virtual void eraseControlItem(const ControlItemMap::iterator&);
    virtual int visiblePosition(QSharedPointer<ControlItem>);

    // Stacking of the SegmentItems on the canvas
    //
    // unused std::pair<int, int> getZMinMax();

//    virtual void init();
//virtual void drawBackground() = 0;

    int xMapToWidget(double x) {return (x-m_pannedRect.left())*width()/m_pannedRect.width();};
    int mapXToWidget(float);
    int mapYToWidget(float);
    QRect mapRectToWidget(QRectF *);
    QPolygon mapItemToWidget(QSharedPointer<ControlItem>);
    QPointF mapWidgetToItem(QPoint*);
    // unused QColor valueToColour(int max, int val);
    void updateSelection();
    virtual void createRulerMenu();

    //--------------- Data members ---------------------------------

//    EditViewBase*               m_parentEditView;
//    QScrollBar*                 m_mainHorizontalScrollBar;
    RulerScale*     m_rulerScale;
    EventSelection* m_eventSelection; //,*m_assignedEventSelection;

//    MatrixScene *m_scene;

    ViewSegment *m_viewSegment;
    NotationStaff *m_notationStaff;
    Segment *m_segment;

    // ??? MEMORY LEAK.
    //     This map stores pointers and never deletes them.
    //     Recommend switching to QSharedPointer.
    ControlItemMap m_controlItemMap;

    // Iterators to the first visible and the last visible item
    // NB these iterators are only really useful for zero duration items as the
    //   interval is determined by start position and will omit items that start
    //   to the left of the screen but end on screen. For this reason, the
    //   m_visibleItems list all includes items that are actually visible.
    ControlItemMap::iterator m_firstVisibleItem;
    ControlItemMap::iterator m_lastVisibleItem;
    ControlItemMap::iterator m_nextItemLeft;

    ControlItemList m_selectedItems;
    ControlItemList m_visibleItems;

    ControlItem *m_currentIndex;

    ControlTool *m_currentTool;
    ControlToolBox *m_toolBox;
    QString m_currentToolName;

    QRectF m_pannedRect;
    double m_xScale;
    double m_yScale;

    long m_maxItemValue;
    long m_minItemValue;

    double m_viewSegmentOffset;

    int m_xOffset;

    double m_currentX;

    QPoint m_lastEventPos;
    bool m_itemMoved;

    bool m_selecting;
    ControlSelector* m_selector;
    QRectF* m_selectionRect;

    QMenu* m_rulerMenu;
    SnapGrid* m_snapGrid;
    QString m_snapName;
    timeT m_snapTimeFromEditor;

    //bool m_hposUpdatePending;

    typedef std::list<Event *> SelectionSet;
    SelectionSet m_selectedEvents;

 private:
    void setSnapTimeFromActionName(const QString& actionName);
};


}

#endif
