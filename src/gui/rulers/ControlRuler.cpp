/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2026 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControlRuler]"
#define RG_NO_DEBUG_PRINT

#include "ControlRuler.h"

#include "ControlTool.h"
#include "ControlToolBox.h"
#include "ControlChangeCommand.h"

#include "base/Event.h"
#include "base/Selection.h"  // EventSelection
#include "base/RulerScale.h"
#include "base/SnapGrid.h"
#include "document/Command.h"  // MacroCommand
#include "document/CommandHistory.h"
#include "base/ViewSegment.h"
#include "misc/ConfigGroups.h"
#include "commands/edit/EraseCommand.h"
#include "misc/Debug.h"

//#include <QContextMenuEvent>
#include <QBrush>
#include <QColor>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPolygon>
#include <QPolygonF>
#include <QSettings>

#include <algorithm>  // std::min(), std::max(), std::find()
#include <memory>
#include <utility>  // std::pair

#include <float.h>  // FLT_MAX
#include <math.h>  // lround()


namespace Rosegarden
{


ControlRuler::ControlRuler(RulerScale *rulerScale,
                           QWidget *parent) :
    QWidget(parent),
    m_rulerScale(rulerScale),
    m_firstVisibleItem(m_controlItemMap.end()),
    m_lastVisibleItem(m_controlItemMap.end()),
    m_nextItemLeft(m_controlItemMap.end()),
    m_snapTimeFromEditor(SnapGrid::NoSnap)
{
    setFixedHeight(sizeHint().height());
    setMouseTracking(true);

    m_toolBox = new ControlToolBox(this);
    // Re-broadcast context help from the tools.  ControlRulerWidget connects
    // for this.
    connect(m_toolBox, &BaseToolBox::showContextHelp,
            this, &ControlRuler::showContextHelp);

    // Context menu actions.
    createAction("snap_none", &ControlRuler::slotSnap);
    createAction("snap_editor", &ControlRuler::slotSnap);
    createAction("snap_unit", &ControlRuler::slotSnap);
    createAction("snap_64", &ControlRuler::slotSnap);
    createAction("snap_48", &ControlRuler::slotSnap);
    createAction("snap_32", &ControlRuler::slotSnap);
    createAction("snap_24", &ControlRuler::slotSnap);
    createAction("snap_16", &ControlRuler::slotSnap);
    createAction("snap_12", &ControlRuler::slotSnap);
    createAction("snap_8", &ControlRuler::slotSnap);
    createAction("snap_dotted_8", &ControlRuler::slotSnap);
    createAction("snap_4", &ControlRuler::slotSnap);
    createAction("snap_dotted_4", &ControlRuler::slotSnap);
    createAction("snap_2", &ControlRuler::slotSnap);
    createAction("snap_beat", &ControlRuler::slotSnap);
    createAction("snap_bar", &ControlRuler::slotSnap);

    // Snap
    m_snapGrid = new SnapGrid(m_rulerScale);

    QSettings settings;
    settings.beginGroup(ControlRulerConfigGroup);
    QString snapString =
        settings.value("Snap Grid Size", "snap_editor").toString();

    setSnapTimeFromActionName(snapString);
}

ControlRuler::~ControlRuler()
{
    delete m_snapGrid;
    m_snapGrid = nullptr;

    delete m_eventSelection;
    m_eventSelection = nullptr;
}

void ControlRuler::setSegment(Segment *segment)
{
    m_segment = segment;

    if (m_eventSelection)
        delete m_eventSelection;

    // Create a new EventSelection connected to the Segment.
    m_eventSelection = new EventSelection(*segment);
}

void ControlRuler::setViewSegment(ViewSegment *viewSegment)
{
    m_viewSegment = viewSegment;

    setSegment(&m_viewSegment->getSegment());
}

ControlItemMap::iterator ControlRuler::findControlItem(const double x)
{
    return m_controlItemMap.upper_bound(x);
}

ControlItemMap::iterator ControlRuler::findControlItem(const Event *event)
{
#if 0
    ControlItemMap::iterator iter;

    // For each controlItem, find the provided Event.
    for (iter = m_controlItemMap.begin();
         iter != m_controlItemMap.end();
         ++iter) {
        if (iter->second->getEvent() == event)
            break;
    }

    return iter;
#else
    // Find the ControlItem for the given event.
    // Not a fan of STL algorithms, just trying this one out.
    return std::find_if(m_controlItemMap.begin(),
                        m_controlItemMap.end(),
                        [event](const ControlItemMap::value_type &x)
                            { return (x.second->getEvent() == event); });
#endif
}

ControlItemMap::iterator ControlRuler::findControlItem(const ControlItem *item)
{
#if 0
    const double xStart = item->xKey();

    // ??? Why equal_range()?  In a std::map there can only be exactly one
    //     element with a specific key value.  Why not use find() instead?
    const std::pair<ControlItemMap::iterator, ControlItemMap::iterator>
            range = m_controlItemMap.equal_range(xStart);
    // Not found?  Bail.
    //if (range.first == m_controlItemMap.end())
    //    return range.first;

    // now find in this range
    for (ControlItemMap::iterator it = range.first;
         it != range.second;
         ++it) {
        if (it->second == item)
            return it;
    }

    return m_controlItemMap.end();
#else
    return m_controlItemMap.find(item->xKey());
#endif
}

void ControlRuler::addControlItem(QSharedPointer<ControlItem> item)
{
    // ControlItem may not have an assigned event but must have x position
    item->setXKey(item->xStart());

    // Insert it.
    ControlItemMap::iterator it = m_controlItemMap.insert(
            ControlItemMap::value_type(item->xStart(), item));

    addCheckVisibleLimits(it);

    if (it->second->isSelected())
        m_selectedItems.push_back(it->second);
}

void ControlRuler::addCheckVisibleLimits(ControlItemMap::iterator it)
{
    // Referenced item has just been added to m_controlItemMap.
    // If it is visible, add it to the list and correct first/last
    // visible item iterators
    const QSharedPointer<ControlItem> item = it->second;

    // If this new item is visible
    if (visiblePosition(item) == 0) {
        // put it in the visible list
        m_visibleItems.push_back(item);

        // Update m_firstVisibleItem.

        // If there is no first visible item or this one is farthest left
        if (m_firstVisibleItem == m_controlItemMap.end()  ||
            item->xStart() < m_firstVisibleItem->second->xStart()) {
            // make it the first visible item
            m_firstVisibleItem = it;
        }

        // Update m_lastVisibleItem.

        // If there is no last visible item or this is farthest right
        if (m_lastVisibleItem == m_controlItemMap.end()  ||
            item->xStart() >= m_lastVisibleItem->second->xStart()) {
            // make it the last visible item
            m_lastVisibleItem = it;
        }
    }

    // Update m_nextItemLeft.

    // If the new item is invisible to the left
    if (visiblePosition(item) == -1) {
        // If there is no "next item left" or this one is after the
        // "next item left"...
        if (m_nextItemLeft == m_controlItemMap.end()  ||
            item->xStart() > m_nextItemLeft->second->xStart()) {
            // make it the next item to the left
            m_nextItemLeft = it;
        }
    }
}

void ControlRuler::removeControlItem(const ControlItemMap::iterator &it)
{
    if (it->second->isSelected())
        m_selectedItems.remove(it->second);

    removeCheckVisibleLimits(it);

    m_controlItemMap.erase(it);
}

void ControlRuler::removeCheckVisibleLimits(const ControlItemMap::iterator &it)
{
    // Referenced item is being removed from m_controlItemMap
    // If it was visible, remove it from the list and correct first/last
    // visible item iterators
    // Note, we can't check if it _was_ visible. It may have just become invisible
    // Try to remove from list and check iterators.
    m_visibleItems.remove(it->second);

    // Update m_firstVisibleItem.

    // If necessary, correct the first and lastVisibleItem iterators
    // If this was the first visible item
    if (it == m_firstVisibleItem) {
        // Check the next item to the right
        ++m_firstVisibleItem;
        // If the next item to the right is invisible, there are no visible items
        // Note we have to check .end() before we dereference ->second
        if (m_firstVisibleItem != m_controlItemMap.end() &&
                visiblePosition(m_firstVisibleItem->second)!=0)
            m_firstVisibleItem = m_controlItemMap.end();
    }

    // Update m_lastVisibleItem.

    // If this was the last visible item
    if (it == m_lastVisibleItem) {
        // and not the first in the list
        if (it != m_controlItemMap.begin()) {
            // check the next item to the left
            --m_lastVisibleItem;
            // If this is invisible, there are no visible items
            if (visiblePosition(m_lastVisibleItem->second)!=0) m_lastVisibleItem = m_controlItemMap.end();
        }
        // if it's first in the list then there are no visible items
        else m_lastVisibleItem = m_controlItemMap.end();
    }

    // Update m_nextItemLeft.

    // If this was the first invisible item left (could be part of a selection moved off screen)
    if (it == m_nextItemLeft) {
        // and not the first in the list
        if (it != m_controlItemMap.begin()) {
            // use the next to the left (we know it is invisible)
            --m_nextItemLeft;
        }
        // if it's first in the list then there are no invisible items to the left
        else m_nextItemLeft = m_controlItemMap.end();
    }
}

void ControlRuler::eraseControlItem(const Event *event)
{
    ControlItemMap::iterator it = findControlItem(event);
    if (it != m_controlItemMap.end())
        removeControlItem(it);
}

void ControlRuler::moveItem(ControlItem *item)
{
    ControlItemMap::iterator it = findControlItem(item);
    // Not found?  Bail.
    if (it == m_controlItemMap.end())
        return;

    // Copy the shared pointer so that the item isn't deleted.
    QSharedPointer<ControlItem> item2 = it->second;

    // Remove the original.
    removeCheckVisibleLimits(it);
    m_controlItemMap.erase(it);

    // Add the new.
    item2->setXKey(item2->xStart());
    it = m_controlItemMap.insert(
            ControlItemMap::value_type(item2->xStart(), item2));
    addCheckVisibleLimits(it);
}

int ControlRuler::visiblePosition(QSharedPointer<ControlItem> item)
{
    // Off screen left?
    if (item->xEnd() < m_pannedRect.left())
        return -1;

    // Off screen right?
    if (item->xStart() > m_pannedRect.right())
        return 1;

    // On screen.
    return 0;
}

float ControlRuler::getXMax()
{
    return (m_rulerScale->getXForTime(m_segment->getEndTime()));
}

float ControlRuler::getXMin()
{
    return (m_rulerScale->getXForTime(m_segment->getStartTime()));
}

void ControlRuler::updateSegment()
{
    // Bring the segment up to date with the ControlRuler's items
    // A number of different actions take place here:
    // 1) m_eventSelection is empty
    // 2) m_eventSelection has events
    //      a) Events in the selection have been modified in value only
    //      b) Events in the selection have moved in time
    //
    // Either run through the ruler's EventSelection, updating from each item
    //  or, if there isn't one, go through m_selectedItems

    QString commandLabel = "Adjust control/property";

    std::unique_ptr<MacroCommand> macro(new MacroCommand(commandLabel));

    // Find the extent of the selected items
    double xmin = FLT_MAX;
    double xmax = -1.0;

    // EventSelection::addEvent() adds timeT(1) to its extent for zero duration
    // events so need to mimic this here.

    timeT durationAdd = 0;

    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        if ((*it)->xStart() < xmin)
            xmin = (*it)->xStart();
        if ((*it)->xEnd() > xmax) {
            xmax = (*it)->xEnd();
            if ((*it)->xEnd() == (*it)->xStart())
                durationAdd = 1;
            else
                durationAdd = 0;
        }
    }

    timeT start = getRulerScale()->getTimeForX(xmin);
    timeT end = getRulerScale()->getTimeForX(xmax) + durationAdd;

    RG_DEBUG << "updateSegment(): added events" << m_eventSelection->size();

    if (m_eventSelection->empty()) {
        // We do not have a valid set of selected events to update
        if (m_selectedItems.empty())
            return;

        // Events will be added by the controlItem->updateSegment methods
        commandLabel = "Add control";
        macro->setName(commandLabel);

    } else {
        // Check for movement in time here and delete events if necessary
        if (start != m_eventSelection->getStartTime()  ||
            end != m_eventSelection->getEndTime()) {
            commandLabel = "Move control";
            macro->setName(commandLabel);

            // Get the limits of the change for undo
            start = std::min(start, m_eventSelection->getStartTime());
            end = std::max(end, m_eventSelection->getEndTime());

        }
    }

    EventSelection eventsToErase(*m_segment);

    if (!allowSimultaneousEvents()) {
        // check for events at the same time and delete them
        // For each selected item...
        for (QSharedPointer<const ControlItem> cItem : m_selectedItems) {
            const double xItem = cItem->xStart();

            RG_DEBUG << "updateSegment(): check for event at" << xItem;

            // For each control item starting at xItem...
            for (ControlItemMap::const_iterator otherItemIter =
                     m_controlItemMap.lower_bound(xItem);
                 otherItemIter != m_controlItemMap.end();
                 ++otherItemIter) {
                // Item not active?  Try the next.
                if (!otherItemIter->second->active())
                    continue;
                // If this is the same as the item we are checking,
                // try the next.
                if (cItem == otherItemIter->second) {
                    RG_DEBUG << "updateSegment(): ignoring new item";
                    continue;
                }
                const double xOther = otherItemIter->first;
                // ??? This should never happen due to lower_bound().
                //     Can we remove?
                if (xOther < xItem) {
                    RG_DEBUG << "updateSegment(): ignoring" << xOther << "before" << xItem;
                    continue;
                }
                // If we are past the original item position, we're done.
                if (xOther > xItem)
                    break;

                // Found a duplicate, add to eventsToErase.

                RG_DEBUG << "updateSegment(): erase old event at" << xOther;

                Event *eventToDelete = otherItemIter->second->getEvent();
                eventsToErase.addEvent(eventToDelete, false);
            }
        }
    }

    RG_DEBUG << "updateSegment(): selectedItems:" << m_selectedItems.size();

    // Add change command to macro
    // ControlChangeCommand calls each selected items updateSegment method
    // Note that updateSegment deletes and renews the event whether it has moved or not
    macro->addCommand(new ControlChangeCommand(m_selectedItems,
                                               *m_segment,
                                               start,
                                               end));

    if (eventsToErase.size() != 0)
        macro->addCommand(new EraseCommand(&eventsToErase));

    CommandHistory::getInstance()->addCommand(macro.release());

    updateEventSelection();
}

void ControlRuler::notationLayoutUpdated(timeT startTime)
{
    // notationLayoutUpdated() should be called after Notation has adjusted the
    // layout.  Clearly, for property control rulers, notes may have been moved
    // so their position needs updating.  The rulers may also have changed so
    // ControllerEventRulers also need updating.  Property control items may
    // now need to be repositioned within the ControlItemMap as new items are
    // all created with a zero x-position, and have now been put in place.
    // For this reason, we need to collect items into a separate list
    // (itemsToUpdate) otherwise we get the dreaded 'modifying a list within a
    // loop of the list' problem which can take quite a long time to fix!

    ControlItemVector itemsToUpdate;

    ControlItemMap::iterator it = m_controlItemMap.begin();

    // Add all new items (items at x position 0) to itemsToUpdate.
    while (it != m_controlItemMap.end()  &&  it->first == 0) {
        itemsToUpdate.push_back(it->second);
        ++it;
    }

    // Skip items up to the first whose x is at or after startTime.
    while (it != m_controlItemMap.end()  &&
           it->first < getRulerScale()->getXForTime(startTime)) {
        ++it;
    }

    // Would like to only update in the defined region but, unfortunately,
    // everything after this time may well have moved as well so we have to do
    // everything after startTime.

    // Copy all items up to the end to itemsToUpdate.
    while (it != m_controlItemMap.end()) {
        itemsToUpdate.push_back(it->second);
        ++it;
    }

    // For each item to update, call the item's update().
    for (QSharedPointer<ControlItem> controlItem : itemsToUpdate) {
        controlItem->update();
    }

    update();
}

void ControlRuler::paintEvent(QPaintEvent * /*event*/)
{
    // We just draw the background here.
    // Derivers call this then draw the items.

    QPainter painter(this);

    QPen pen;
    pen.setStyle(Qt::NoPen);
    painter.setPen(pen);

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(Qt::white);
    painter.setBrush(brush);

    // Fill with white.
    painter.drawRect(0,0,width(),height());

    const double xStartUnscaled =
        m_rulerScale->getXForTime(m_segment->getStartTime());
    const double xStart = mapXToWidget(xStartUnscaled * m_xScale);

    const double xEndUnscaled =
        m_rulerScale->getXForTime(m_segment->getEndTime());
    const double xEnd = mapXToWidget(xEndUnscaled * m_xScale);

    // Top/Middle/Bottom horizontal lines.
    painter.setPen(QColor(127, 127, 127));
    painter.drawLine(xStart, mapYToWidget(0.0f), xEnd, mapYToWidget(0.0f));
    painter.drawLine(xStart, mapYToWidget(0.5f), xEnd, mapYToWidget(0.5f));
    painter.drawLine(xStart, mapYToWidget(1.0f), xEnd, mapYToWidget(1.0f));

    // Quarter horizontal lines.
    painter.setPen(QColor(192, 192, 192));
    painter.drawLine(xStart, mapYToWidget(0.25f), xEnd, mapYToWidget(0.25f));
    painter.drawLine(xStart, mapYToWidget(0.75f), xEnd, mapYToWidget(0.75f));

    // vertical lines from snap grid

    timeT snaps = m_snapGrid->getSnapSetting();
    if (snaps != SnapGrid::NoSnap) {
        Composition *comp = m_rulerScale->getComposition();

        const double yTop = mapYToWidget(0.0f);
        const double yBottom = mapYToWidget(1.0f);
        const double xLeft = geometry().left();
        const double xRight = geometry().right();

        const int firstBar = comp->getBarNumber(m_segment->getStartTime());
        const int lastBar = comp->getBarNumber(m_segment->getEndMarkerTime());

        // For each bar in the Segment...
        for (int bar = firstBar; bar <= lastBar; ++bar) {
            std::pair<timeT, timeT> range = comp->getBarRange(bar);
            // ??? Why are these double when they are window coords?
            const double x0 = m_rulerScale->getXForTime(range.first);
            const double x1 = m_rulerScale->getXForTime(range.second);

            // Bar is to the left of the viewport?  Try the next.
            if (mapXToWidget(x1 * m_xScale) < xLeft) continue;
            // Bar is to the right of the viewport?  We are done.
            if (mapXToWidget(x0 * m_xScale) > xRight) break;

            bool newTimeSig = false;
            TimeSignature timeSig =
                comp->getTimeSignatureInBar(bar, newTimeSig);

            const double gridLines = double(timeSig.getBarDuration()) /
                double(m_snapGrid->getSnapTimeForX(x0));

            const double barWidth = x1 - x0;
            const double dx = barWidth / gridLines;
            double x = x0;

            // For each grid line within the bar...
            for (int index = 0; index < gridLines; ++index, x += dx) {

                if (x < xStartUnscaled)
                    continue;
                // Exit if we have passed the end of last segment end time.
                if (x > xEndUnscaled)
                    break;

                // If it's the bar line...
                if (index == 0) {
                    // Make it dark.
                    painter.setPen(QColor(127, 127, 127));
                } else {  // Not the bar line, not so dark.
                    painter.setPen(QColor(192, 192, 192));
                }
                int xmap = mapXToWidget(x * m_xScale);
                painter.drawLine(xmap, yTop, xmap, yBottom);

            }
        }
    }
}

int ControlRuler::mapXToWidget(float x) const
{
    return lround((m_xOffset + x - m_pannedRect.left()) / m_xScale);
}

int ControlRuler::mapYToWidget(float y) const
{
    return lround((-y + 1.0) / m_yScale);
}

QRect ControlRuler::mapRectToWidget(const QRectF *rect) const
{
    QRect widgetRect;
    widgetRect.setLeft(mapXToWidget(rect->left()));
    widgetRect.setTop(mapYToWidget(rect->top()));
    widgetRect.setRight(mapXToWidget(rect->right()));
    widgetRect.setBottom(mapYToWidget(rect->bottom()));

    return widgetRect;
}

QPolygon ControlRuler::mapItemToWidget(
        QSharedPointer<const ControlItem> controlItem) const
{
    QPolygon newpoly;

    // For each point in the ControlItem (it's a QPolygonF)...
    for (const QPointF &point : *controlItem) {
        newpoly.push_back(QPoint{mapXToWidget(point.x()),
                                 mapYToWidget(point.y())});
    }

    return newpoly;
}

QPointF ControlRuler::mapWidgetToItem(const QPoint *point) const
{
    QPointF newpoint;
    newpoint.setX(m_xScale * point->x() + m_pannedRect.left() - m_xOffset);
    newpoint.setY(-m_yScale * point->y() + 1.0);

    return newpoint;
}

void ControlRuler::setPannedRect(const QRectF &pannedRect)
{
    if (pannedRect.isNull())
        RG_WARNING << "slotSetPannedRect(): WARNING: Rect is null.";

    m_pannedRect = pannedRect;

    m_xScale = (double)m_pannedRect.width() / (double)width();
    m_yScale = 1.0f / (double)height();

    // Create the visible items list

    m_visibleItems.clear();
    bool anyVisibleYet = false;

    m_nextItemLeft = m_controlItemMap.end();
    m_firstVisibleItem = m_controlItemMap.end();
    m_lastVisibleItem = m_controlItemMap.end();

    // For each ControlItem...
    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        const int visPos = visiblePosition(it->second);

        // To the left?  Remember as "next item left".
        if (visPos == -1)
            m_nextItemLeft = it;

        // Within?  Add to visible items.
        if (visPos == 0) {
            if (!anyVisibleYet) {
                m_firstVisibleItem = it;
                anyVisibleYet = true;
            }

            m_visibleItems.push_back(it->second);
            m_lastVisibleItem = it;
        }

        // To the right?  We're done.
        if (visPos == 1)
            break;
    }
}

void ControlRuler::resizeEvent(QResizeEvent *)
{
    // Note slotSetPannedRect is called (from ControlRulerWidget::slotSetPannedRect)
    //   on a resize event. However, this call is too early and width() has not been
    //   updated. This event handler patches that problem. Could be more efficient.
    setPannedRect(m_pannedRect);
}

ControlMouseEvent ControlRuler::createControlMouseEvent(
        const QMouseEvent *e) const
{
    ControlMouseEvent controlMouseEvent;

    // Copy mouse position.
    QPoint widgetMousePos = e->pos();
    QPointF mousePos = mapWidgetToItem(&widgetMousePos);
    controlMouseEvent.x = mousePos.x();
    controlMouseEvent.y = mousePos.y();

    // Compute snapped coords, left and right.

    const int mouseTime = m_rulerScale->getTimeForX(
            controlMouseEvent.x / m_xScale);

    const int leftTime = m_snapGrid->snapTime(mouseTime, SnapGrid::SnapLeft);
    controlMouseEvent.snappedXLeft =
        m_rulerScale->getXForTime(leftTime) * m_xScale;

    const int rightTime = m_snapGrid->snapTime(mouseTime, SnapGrid::SnapRight);
    controlMouseEvent.snappedXRight =
        m_rulerScale->getXForTime(rightTime) * m_xScale;

    //RG_DEBUG << "createControlMouseEvent(): snap" <<
    //    controlMouseEvent.x << mouseTime <<
    //    controlMouseEvent.snappedXLeft << leftTime <<
    //    controlMouseEvent.snappedXRight << rightTime <<
    //    m_xScale;

    // Fill the itemList with each ControlItem under the mouse pointer.

    for (ControlItemList::const_iterator it = m_visibleItems.begin();
         it != m_visibleItems.end();
         ++it) {
        if ((*it)->containsPoint(mousePos, Qt::OddEvenFill))
            controlMouseEvent.itemList.push_back(*it);
    }

    // Copy buttons and modifiers.
    controlMouseEvent.buttons = e->buttons();
    controlMouseEvent.modifiers = e->modifiers();

    return controlMouseEvent;
}

void ControlRuler::mousePressEvent(QMouseEvent *e)
{
    if (!m_currentTool)
        return;

    if (e->button() == Qt::LeftButton) {
        // Delegate left button to tool.
        ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
        m_currentTool->handleLeftButtonPress(&controlMouseEvent);
    } else if (e->button() == Qt::MiddleButton) {
        // Delegate middle button to tool.
        ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
        m_currentTool->handleMidButtonPress(&controlMouseEvent);
    } else if (e->button() == Qt::RightButton) {
        // Right button is context menu.
        if (!m_rulerMenu)
            createRulerMenu();
        if (m_rulerMenu) {
            QAction *setAction = findAction(m_snapName);
            setAction->setChecked(true);

            // Let derivers update the menu as needed.
            updateRulerMenu();

            m_rulerMenu->exec(QCursor::pos());
        }
    }

    if (m_autoScroller)
        m_autoScroller->start();
}

void ControlRuler::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_currentTool)
        return;

    // Delegate left and middle button release to tool.
    if (e->button() == Qt::LeftButton  ||  e->button() == Qt::MiddleButton) {
        ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
        m_currentTool->handleMouseRelease(&controlMouseEvent);
    }

    if (m_autoScroller)
        m_autoScroller->stop();
}

void ControlRuler::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);

    // Some tools (e.g. ControlPainter) return NO_FOLLOW to prevent following
    // while adding new points.
    // Others, (e.g. ControlSelector) return FOLLOW_HORIZONTAL to allow
    // auto scrolling while selecting.
    FollowMode mode = m_currentTool->handleMouseMove(&controlMouseEvent);

    if (m_autoScroller)
        m_autoScroller->setFollowMode(mode);
}

void
ControlRuler::wheelEvent(QWheelEvent * /*e*/)
{
    // not sure what to do yet
    // ??? It would be really nice if this would gently move the selected
    //     items up/down by one.  This would provide precision adjustment.

}

void ControlRuler::slotSnap()
{
    setSnapTimeFromActionName(sender()->objectName());

    repaint();
}

void ControlRuler::setSnapTimeFromActionName(const QString &actionName)
{
    QString snapName = actionName;

    // Convert action name to duration.
    int snapTime = SnapGrid::NoSnap;
    timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    if (actionName == "snap_none") {
        snapTime = SnapGrid::NoSnap;
    } else if (actionName == "snap_editor") {
        snapTime = m_snapTimeFromEditor;
    } else if (actionName == "snap_unit") {
        snapTime = SnapGrid::SnapToUnit;
    } else if (actionName == "snap_64") {
        snapTime = crotchetDuration / 16;
    } else if (actionName == "snap_48") {
        snapTime = crotchetDuration / 12;
    } else if (actionName == "snap_32") {
        snapTime = crotchetDuration / 8;
    } else if (actionName == "snap_24") {
        snapTime = crotchetDuration / 6;
    } else if (actionName == "snap_16") {
        snapTime = crotchetDuration / 4;
    } else if (actionName == "snap_12") {
        snapTime = crotchetDuration / 3;
    } else if (actionName == "snap_8") {
        snapTime = crotchetDuration / 2;
    } else if (actionName == "snap_dotted_8") {
        snapTime = (crotchetDuration * 3) / 4;
    } else if (actionName == "snap_4") {
        snapTime = crotchetDuration;
    } else if (actionName == "snap_dotted_4") {
        snapTime = (crotchetDuration * 3) / 2;
    } else if (actionName == "snap_2") {
        snapTime = crotchetDuration * 2;
    } else if (actionName == "snap_beat") {
        snapTime = SnapGrid::SnapToBeat;
    } else if (actionName == "snap_bar") {
        snapTime = SnapGrid::SnapToBar;
    } else {
        // unknown action name - use snap_none
        snapName = "snap_none";
    }

    m_snapGrid->setSnapTime(snapTime);
    m_snapName = snapName;

    QSettings settings;
    settings.beginGroup(ControlRulerConfigGroup);
    settings.setValue("Snap Grid Size", snapName);
}

void ControlRuler::contextMenuEvent(QContextMenuEvent *)
{
    // DO NOT REMOVE.
    // This is required for the context menu to launch.
    // ??? Why?
}

void
ControlRuler::clearSelection()
{
    for (ControlItemList::iterator it = m_selectedItems.begin();
         it != m_selectedItems.end();
         ++it) {
        (*it)->setSelected(false);
    }
    m_selectedItems.clear();

    delete m_eventSelection;
    m_eventSelection = new EventSelection(*m_segment);

    emit rulerSelectionChanged();
}

void ControlRuler::updateEventSelection()
{
    delete m_eventSelection;
    m_eventSelection = new EventSelection(*m_segment);

    for (ControlItemList::iterator it = m_selectedItems.begin();
         it != m_selectedItems.end();
         ++it) {
        m_eventSelection->addEvent((*it)->getEvent());
    }

    emit rulerSelectionChanged();

    // Special signal for the velocity ruler to make sure the user can
    // go from bar to bar and adjust velocities when nothing is selected
    // in the matrix or notation.  Travels through ControlRulerWidget and
    // MatrixWidget on its way to MatrixView::slotrulerSelectionUpdate().
    // There is also a Notation path through the corresponding classes
    // for Notation.  A more direct connection would be nice if we could
    // somehow have access to ControlRuler and MatrixView.  MatrixView should
    // probably have access to the control rulers somehow so it can make
    // this connection.
    // Be sure to regression test all callers.
    emit rulerSelectionUpdate();
}

void ControlRuler::addToSelection(QSharedPointer<ControlItem> item)
{
    // If we already have this one, bail.
    if (std::find(m_selectedItems.begin(), m_selectedItems.end(), item) !=
            m_selectedItems.end())
        return;

    m_selectedItems.push_back(item);
    item->setSelected(true);

    m_eventSelection->addEvent(item->getEvent());
    emit rulerSelectionChanged();
}

void ControlRuler::removeFromSelection(QSharedPointer<ControlItem> item)
{
    m_selectedItems.remove(item);
    item->setSelected(false);

    m_eventSelection->removeEvent(item->getEvent());
    emit rulerSelectionChanged();
}

void ControlRuler::clear()
{
    m_controlItemMap.clear();
    m_firstVisibleItem = m_controlItemMap.end();
    m_lastVisibleItem = m_controlItemMap.end();
    m_nextItemLeft = m_controlItemMap.end();

    m_visibleItems.clear();
    m_selectedItems.clear();
}

float ControlRuler::valueToY(long val)
{
    return float(val - m_minItemValue) / float(m_maxItemValue - m_minItemValue);
}

long ControlRuler::yToValue(float y)
{
    return (long)(y * (m_maxItemValue - m_minItemValue)) + m_minItemValue;
}

void ControlRuler::setSnapFromEditor(timeT snapSetting, bool forceFromEditor)
{
    m_snapTimeFromEditor = snapSetting;
    if (forceFromEditor)
        m_snapName = "snap_editor";

    if (m_snapName == "snap_editor") {
        m_snapGrid->setSnapTime(snapSetting);
        repaint();
    }
}


}
