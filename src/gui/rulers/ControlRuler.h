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

    RulerScale *m_rulerScale;

    void clear();

    virtual bool allowSimultaneousEvents() = 0;

    void setMinItemValue(long val)  { m_minItemValue = val; }
    void setMaxItemValue(long val)  { m_maxItemValue = val; }

    // QWidget overrides
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    // ??? Do we need this?  It's empty.  Does it break the context menu if
    //     we remove it?
    void contextMenuEvent(QContextMenuEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    void addControlItem(QSharedPointer<ControlItem>);
    void eraseControlItem(const Event *);
    void eraseControlItem(const ControlItemMap::iterator &);

    int mapXToWidget(float x);
    int mapYToWidget(float y);
    QRect mapRectToWidget(QRectF *rect);
    QPolygon mapItemToWidget(QSharedPointer<ControlItem> controlItem);

    /// Copies from m_selectedItems to m_eventSelection.
    /**
     * ??? rename: UpdateEventSelection()
     */
    void updateSelection();

    /// Derivers override to create a right-click context menu for display.
    virtual void createRulerMenu()  { }
    /// Derivers override to enable/disable right-click context menu items.
    virtual void updateRulerMenu()  { }

    ViewSegment *m_viewSegment{nullptr};
    Segment *m_segment{nullptr};

    ControlItemMap m_controlItemMap;

    // Iterators to the first visible and the last visible item
    // NB these iterators are only really useful for zero duration items as the
    //   interval is determined by start position and will omit items that start
    //   to the left of the screen but end on screen. For this reason, the
    //   m_visibleItems list includes all items that are actually visible.
    ControlItemMap::iterator m_firstVisibleItem;
    ControlItemMap::iterator m_lastVisibleItem;
    ControlItemMap::iterator m_nextItemLeft;

    ControlItemList m_selectedItems;
    ControlItemList m_visibleItems;

    ControlTool *m_currentTool{nullptr};
    ControlToolBox *m_toolBox;

    QRectF m_pannedRect;

    QRectF *m_selectionRect{nullptr};

    QMenu *m_rulerMenu{nullptr};

private:

    long m_maxItemValue{127};
    long m_minItemValue{0};

    double m_xScale{1};
    double m_yScale{1};

    int m_xOffset{0};

    ControlItemMap::iterator findControlItem(const ControlItem *);
    ControlItemMap::iterator findControlItem(const Event *);
    void addCheckVisibleLimits(ControlItemMap::iterator);
    void removeControlItem(const ControlItemMap::iterator &);
    void removeCheckVisibleLimits(const ControlItemMap::iterator &);
    int visiblePosition(QSharedPointer<ControlItem>);

    QPointF mapWidgetToItem(QPoint *point);

    ControlMouseEvent createControlMouseEvent(QMouseEvent *e);

    SnapGrid *m_snapGrid;
    QString m_snapName;
    timeT m_snapTimeFromEditor;
    void setSnapTimeFromActionName(const QString &actionName);

    QPointer<AutoScroller> m_autoScroller;

    EventSelection *m_eventSelection{nullptr};

};


}

#endif
