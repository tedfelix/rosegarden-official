/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONTROLRULER_H_
#define _RG_CONTROLRULER_H_

#include <QWidget>
//#include <Q3Canvas>
//#include <Q3CanvasItemList>
//#include <Q3CanvasRectangle>
#include "base/Segment.h"
#include "base/Selection.h"
//#include "gui/general/RosegardenCanvasView.h"
#include "gui/editors/matrix/MatrixViewSegment.h"
#include <QColor>
#include <QPoint>
#include <QString>
#include <utility>

#include "ControlItem.h"

//class QWidget;
class QMenu;
class QWheelEvent;
class QScrollBar;
class QMouseEvent;
class QContextMenuEvent;
//class Q3CanvasRectangle;
//class Q3Canvas;


namespace Rosegarden
{

class ControlTool;
class ControlToolBox;
class ControlSelector;
class ControlMouseEvent;
//class ControlItem;
//class ControlItemList;
class Segment;
class RulerScale;
class EventSelection;
class EditViewBase;

//typedef std::list<ControlItem*> ControlItemList;
//typedef std::list<ControlItem*>::iterator ControlItemListIterator;

/**
 * ControlRuler : base class for Control Rulers
 */
//class ControlRuler : public RosegardenCanvasView, public SegmentObserver, public EventSelectionObserver
//class ControlRuler : public QWidget, public SegmentObserver, public EventSelectionObserver
class ControlRuler : public QWidget //, public ViewSegmentObserver
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(MatrixViewSegment*,
                 RulerScale*,
//                 EditViewBase* parentView,
//                 Q3Canvas*,
                 QWidget* parent=0); //###  const char name is obsolete, and I'm almost sure WFlags is obsolete too
    virtual ~ControlRuler();

    virtual QString getName() = 0;

    virtual QSize sizeHint() { return QSize(1,100); }

    virtual void paintEvent(QPaintEvent *);

    long getMaxItemValue() { return m_maxItemValue; }
    void setMaxItemValue(long val) { m_maxItemValue = val; }

    long getMinItemValue() { return m_minItemValue; }
    void setMinItemValue(long val) { m_minItemValue = val; }

    void clear();

    void setControlTool(ControlTool*);

    int applyTool(double x, int val);
    ControlItemList *getSelectedItems() { return &m_selectedItems; }

//    Q3CanvasRectangle* getSelectionRectangle() { return m_selectionRect; }
    QRectF* getSelectionRectangle() { return m_selectionRect; }
    void setSelectionRect(QRectF *rect) { m_selectionRect = rect; }

    virtual void setSegment(Segment *);
    virtual void setViewSegment(MatrixViewSegment *);
    Segment* getSegment() { return m_segment; }

    void updateSegment();

    virtual void setRulerScale(RulerScale *rulerscale) { m_rulerScale = rulerscale; }
    RulerScale* getRulerScale() { return m_rulerScale; }

    float valueToY(long val);
    long yToValue(float height);

    double getXScale() {return m_xScale; }
    double getYScale() {return m_yScale; }
    float getXMax();
    float getXMin();
    
    void clearSelectedItems();
    void addToSelection(ControlItem*);
    void removeFromSelection(ControlItem*);

    virtual ControlItemMap::iterator findControlItem(float x);
    virtual void moveItem(ControlItem*);

    /// EventSelectionObserver
//    virtual void eventSelected(EventSelection *,Event *);
//    virtual void eventDeselected(EventSelection *,Event *);
//    virtual void eventSelectionDestroyed(EventSelection *);

//    void assignEventSelection(EventSelection *);

    // SegmentObserver interface
//    virtual void viewSegmentDeleted(const ViewSegment *);

    static const int DefaultRulerHeight;
    static const int MinItemHeight;
    static const int MaxItemHeight;
    static const int ItemHeightRange;

    void flipForwards();
    void flipBackwards();

//    void setMainHorizontalScrollBar(QScrollBar* s ) { m_mainHorizontalScrollBar = s; }

signals:
    void stateChange(const QString&, bool);
    void dragScroll(timeT);

public slots:
    /// override RosegardenCanvasView - we don't want to change the main hscrollbar
    virtual void slotUpdate();
//    virtual void slotUpdateElementsHPos();
    virtual void slotScrollHorizSmallSteps(int);
    virtual void slotSetPannedRect(QRectF);
//    virtual void slotSetScale(double);
    virtual void slotSetTool(const QString&);

protected:
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual void wheelEvent(QWheelEvent*);
    virtual void resizeEvent(QResizeEvent *);

    virtual ControlMouseEvent createControlMouseEvent(QMouseEvent* e);

//    virtual QScrollBar* getMainHorizontalScrollBar();

//    virtual void computeViewSegmentOffset() {};

//    virtual void layoutItem(ControlItem*);
    virtual ControlItemMap::iterator findControlItem(const ControlItem*);
    virtual ControlItemMap::iterator findControlItem(const Event*);
    virtual void addControlItem(ControlItem*);
    virtual void addCheckVisibleLimits(ControlItemMap::iterator);
    virtual void removeControlItem(ControlItem*);
    virtual void removeControlItem(const Event*);
    virtual void removeControlItem(const ControlItemMap::iterator&);
    virtual void removeCheckVisibleLimits(const ControlItemMap::iterator&);
    virtual void eraseControlItem(const Event*);
    virtual void eraseControlItem(const ControlItemMap::iterator&);
    virtual int visiblePosition(ControlItem*);

    // Stacking of the SegmentItems on the canvas
    //
    std::pair<int, int> getZMinMax();

//    virtual void init();
//virtual void drawBackground() = 0;

    int xMapToWidget(double x) {return (x-m_pannedRect.left())*width()/m_pannedRect.width();};
    int mapXToWidget(float);
    int mapYToWidget(float);
    QRect mapItemToWidget(QRectF*);
    QPolygon mapItemToWidget(QPolygonF*);
    QPointF mapWidgetToItem(QPoint*);

    QColor valueToColour(int max, int val);

    void updateSelection();

    void setMenuName(QString menuName) { m_menuName = menuName; }
    void createMenu();

    //--------------- Data members ---------------------------------

//    EditViewBase*               m_parentEditView;
//    QScrollBar*                 m_mainHorizontalScrollBar;
    RulerScale*     m_rulerScale;
    EventSelection* m_eventSelection; //,*m_assignedEventSelection;

    MatrixScene *m_scene;

    MatrixViewSegment *m_viewSegment;
    Segment *m_segment;

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

    ControlItem* m_currentIndex;
//    Q3CanvasItemList m_selectedItems;

    ControlTool *m_currentTool;
    ControlToolBox *m_toolBox;
    QString m_currentToolName;

    QRectF m_pannedRect;
    double m_xScale;
    double m_yScale;

    long m_maxItemValue;
    long m_minItemValue;

    double m_viewSegmentOffset;

    double m_currentX;

    QPoint m_lastEventPos;
    bool m_itemMoved;

    bool m_selecting;
    ControlSelector* m_selector;
//    Q3CanvasRectangle* m_selectionRect;
    QRectF* m_selectionRect;

    QString m_menuName;
    QMenu* 	m_menu;

    bool m_hposUpdatePending;

    typedef std::list<Event *> SelectionSet;
    SelectionSet m_selectedEvents;
};


}

#endif
