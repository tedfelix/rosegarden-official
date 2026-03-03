/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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

#include "ControlItem.h"  // ControlItemMap
#include "ControlMouseEvent.h"

#include "base/TimeT.h"
#include "gui/general/ActionFileClient.h"

#include <QPoint>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <list>

class QMenu;
class QWheelEvent;
class QMouseEvent;
class QContextMenuEvent;


namespace Rosegarden
{


class AutoScroller;
class ControlSelector;
class ControlTool;
class ControlToolBox;
class EventSelection;
class NotationStaff;
class RulerScale;
class Segment;
class SnapGrid;
class ViewSegment;


/// Abstract base class for control rulers.
/**
 * ??? rename: ControlRulerBase
 */
class ControlRuler : public QWidget, public ActionFileClient
{
    Q_OBJECT

public:

    ControlRuler(RulerScale *rulerScale, QWidget *parent);
    ~ControlRuler() override;

    void setAutoScroller(QPointer<AutoScroller> autoScroller)
            { m_autoScroller = autoScroller; }

    void setRulerScale(RulerScale *rulerscale)  { m_rulerScale = rulerscale; }
    RulerScale *getRulerScale()  { return m_rulerScale; }

    //virtual QString getName() = 0;

    // QWidget override
    QSize sizeHint() const override  { return QSize(1,100); }

    virtual void setTool(const QString & /*name*/)  { }

    ControlItemList *getSelectedItems()  { return &m_selectedItems; }

    // For ControlSelector tool.
    void setSelectionRect(QRectF *rect)  { m_selectionRect = rect; }
    QRectF *getSelectionRectangle()  { return m_selectionRect; }

    virtual void setSegment(Segment *);
    virtual void setViewSegment(ViewSegment *);

    /// Copy screen to document.  Uses a command for undo.
    /**
     * ??? rename: updateDocument()?
     */
    void updateSegment();

    // ??? endTime is unused.  Remove it.
    void notationLayoutUpdated(timeT startTime, timeT endTime);

    void setXOffset(int offset)  { m_xOffset = offset; }

    float valueToY(long val);
    long yToValue(float y);

    double getXScale() const  { return m_xScale; }
    double getYScale() const  { return m_yScale; }
    float getXMax();
    float getXMin();


    // Selection

    void addToSelection(QSharedPointer<ControlItem>);
    void removeFromSelection(QSharedPointer<ControlItem>);
    /// Deselect all selected items.
    /**
     * ??? rename: removeAllFromSelection()
     */
    void clearSelectedItems();
    EventSelection *getEventSelection()  { return m_eventSelection; }


    ControlItemMap::iterator findControlItem(float x);
    void moveItem(ControlItem *item);

    SnapGrid *getSnapGrid() const  { return m_snapGrid; }
    void setSnapFromEditor(timeT snapSetting, bool forceFromEditor);

    void setPannedRect(QRectF);

signals:

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

    /// Handles snap duration changes.
    void slotSnap();

protected:

    virtual bool allowSimultaneousEvents() = 0;

    void setMinItemValue(long val)  { m_minItemValue = val; }
    void setMaxItemValue(long val)  { m_maxItemValue = val; }

    // QWidget overrides
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    void addControlItem(QSharedPointer<ControlItem>);
    void eraseControlItem(const Event *);
    void eraseControlItem(const ControlItemMap::iterator &);

private:
    ControlItemMap::iterator findControlItem(const ControlItem *);
    ControlItemMap::iterator findControlItem(const Event *);
    void addCheckVisibleLimits(ControlItemMap::iterator);
    //void removeControlItem(ControlItem *);
    //void removeControlItem(const Event *);
    void removeControlItem(const ControlItemMap::iterator &);
    void removeCheckVisibleLimits(const ControlItemMap::iterator &);
    int visiblePosition(QSharedPointer<ControlItem>);
protected:

    void clear();

//    virtual void init();
//virtual void drawBackground() = 0;

    int xMapToWidget(double x) {return (x-m_pannedRect.left())*width()/m_pannedRect.width();};
    int mapXToWidget(float);
    int mapYToWidget(float);
    QRect mapRectToWidget(QRectF *);
    QPolygon mapItemToWidget(QSharedPointer<ControlItem>);
    QPointF mapWidgetToItem(QPoint*);
    void updateSelection();

    virtual void createRulerMenu();
    // Derivers can override to enable/disable menu items before the menu is
    // displayed.
    virtual void updateRulerMenu()  { }

//    EditViewBase*               m_parentEditView;
//    QScrollBar*                 m_mainHorizontalScrollBar;
    RulerScale*     m_rulerScale;
    EventSelection *m_eventSelection{nullptr}; //,*m_assignedEventSelection;

//    MatrixScene *m_scene;

    ViewSegment *m_viewSegment{nullptr};
    NotationStaff *m_notationStaff{nullptr};
    Segment *m_segment{nullptr};

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

    ControlItem *m_currentIndex{nullptr};

    ControlTool *m_currentTool{nullptr};
    ControlToolBox *m_toolBox;
    QString m_currentToolName;

    QRectF m_pannedRect;
    double m_xScale{1};
    double m_yScale{1};

    long m_maxItemValue{127};
    long m_minItemValue{0};

    double m_viewSegmentOffset{0};

    int m_xOffset{0};

    double m_currentX{0};

    QPoint m_lastEventPos;
    bool m_itemMoved{false};

    bool m_selecting{false};
    ControlSelector *m_selector{nullptr};
    QRectF *m_selectionRect{nullptr};

    QMenu *m_rulerMenu{nullptr};
    SnapGrid *m_snapGrid;
    QString m_snapName;
    timeT m_snapTimeFromEditor;

    // ??? Rename: SelectionList
    //typedef std::list<Event *> SelectionSet;
    //SelectionSet m_selectedEvents;

private:

    ControlMouseEvent createControlMouseEvent(QMouseEvent *e);

    void setSnapTimeFromActionName(const QString &actionName);

    QPointer<AutoScroller> m_autoScroller;

};


}

#endif
