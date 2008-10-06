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

#ifndef _RG_CONTROLRULER_H_
#define _RG_CONTROLRULER_H_

#include <Q3Canvas>
#include <Q3CanvasItemList>
#include <Q3CanvasRectangle>
#include "base/Segment.h"
#include "gui/general/RosegardenCanvasView.h"
#include <QColor>
#include <QPoint>
#include <QString>
#include <utility>


class QWidget;
class QWheelEvent;
class QScrollBar;
class QMouseEvent;
class QContextMenuEvent;
class Q3CanvasRectangle;
class Q3Canvas;


namespace Rosegarden
{

class ControlTool;
class ControlSelector;
class ControlItem;
class TextFloat;
class Segment;
class RulerScale;
class EventSelection;
class EditViewBase;


/**
 * ControlRuler : base class for Control Rulers
 */
class ControlRuler : public RosegardenCanvasView, public SegmentObserver
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(Segment*,
                 RulerScale*,
                 EditViewBase* parentView,
                 Q3Canvas*,
                 QWidget* parent=0); //###  const char name is obsolete, and I'm almost sure WFlags is obsolete too
    virtual ~ControlRuler();

    virtual QString getName() = 0;

    int getMaxItemValue() { return m_maxItemValue; }
    void setMaxItemValue(int val) { m_maxItemValue = val; }

    void clear();

    void setControlTool(ControlTool*);

    int applyTool(double x, int val);

    Q3CanvasRectangle* getSelectionRectangle() { return m_selectionRect; }

    RulerScale* getRulerScale() { return m_rulerScale; }

    // SegmentObserver interface
    virtual void segmentDeleted(const Segment *);

    static const int DefaultRulerHeight;
    static const int MinItemHeight;
    static const int MaxItemHeight;
    static const int ItemHeightRange;

    void flipForwards();
    void flipBackwards();

    void setMainHorizontalScrollBar(QScrollBar* s ) { m_mainHorizontalScrollBar = s; }

signals:
    void stateChange(const QString&, bool);

public slots:
    /// override RosegardenCanvasView - we don't want to change the main hscrollbar
    virtual void slotUpdate();
    virtual void slotUpdateElementsHPos();
    
protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsWheelEvent(QWheelEvent*);
    virtual void contentsContextMenuEvent(QContextMenuEvent*);

    virtual QScrollBar* getMainHorizontalScrollBar();

    virtual void computeStaffOffset() {};

    virtual void layoutItem(ControlItem*);



    // Stacking of the SegmentItems on the canvas
    //
    std::pair<int, int> getZMinMax();

    virtual void init() = 0;
    virtual void drawBackground() = 0;

    int valueToHeight(long val);
    long heightToValue(int height);
    QColor valueToColour(int max, int val);

    void clearSelectedItems();
    void updateSelection();

    void setMenuName(QString menuName) { m_menuName = menuName; }
    void createMenu();

    //--------------- Data members ---------------------------------

    EditViewBase*               m_parentEditView;
    QScrollBar*                 m_mainHorizontalScrollBar;
    RulerScale*     m_rulerScale;
    EventSelection* m_eventSelection;
    Segment*        m_segment;

    ControlItem* m_currentIndex;
    Q3CanvasItemList m_selectedItems;

    ControlTool *m_tool;

    int m_maxItemValue;

    double m_staffOffset;

    double m_currentX;

    QPoint m_lastEventPos;
    bool m_itemMoved;

    bool m_selecting;
    ControlSelector* m_selector;
    Q3CanvasRectangle* m_selectionRect;

    QString m_menuName;
    QMenu* m_menu;

    TextFloat  *m_numberFloat;

    bool m_hposUpdatePending;
};


}

#endif
