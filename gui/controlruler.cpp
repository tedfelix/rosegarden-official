// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <algorithm>

#include <qvalidator.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qframe.h>
#include <qpopupmenu.h>

#include <klocale.h>
#include <klineeditdlg.h>

#include "MidiTypes.h"
#include "Selection.h"
#include "Staff.h"

#include "controlruler.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "Segment.h"
#include "RulerScale.h"
#include "velocitycolour.h"
#include "basiccommand.h"
#include "editcommands.h"
#include "editviewbase.h"
#include "ControlParameter.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "Property.h"
#include "widgets.h"
#include "linedstaff.h"
#include "editview.h"

using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::PropertyName;
using Rosegarden::ControlParameter;
using Rosegarden::ViewElement;
using Rosegarden::EventSelection;


/**
 * Control Tool - not really used yet
 */
class ControlTool
{
public:
    virtual ~ControlTool() {};
    virtual int operator()(double x, int val) = 0;
};

class TestTool : public ControlTool
{
public:
    int operator()(double, int);
};


int TestTool::operator()(double /*x*/, int val)
{
//     int res = (int)x / 10 + val;

    int res = val;

//     RG_DEBUG << "TestTool::operator() : x = " << x
//              << ", val = " << val
//              << ", res = " << res << endl;

    return res;
}

//
// ------------------
//  Element Adapters
// ------------------
//

class ElementAdapter
{
public:
    virtual ~ElementAdapter() {};

    virtual bool   getValue(long&) = 0;
    virtual void   setValue(long)  = 0;
    virtual timeT  getTime()       = 0;
    virtual timeT  getDuration()   = 0;
    virtual Event* getEvent()      = 0;
};

//////////////////////////////

class ViewElementAdapter : public ElementAdapter
{
public:
    ViewElementAdapter(ViewElement*, const PropertyName&);

    virtual bool  getValue(long&);
    virtual void  setValue(long);
    virtual timeT getTime();
    virtual timeT getDuration();

    virtual Event* getEvent() { return m_viewElement->event(); }
    ViewElement* getViewElement() { return m_viewElement; }

protected:

    //--------------- Data members ---------------------------------

    ViewElement* m_viewElement;
    const PropertyName& m_propertyName;
};

ViewElementAdapter::ViewElementAdapter(ViewElement* el, const PropertyName& p)
    : m_viewElement(el),
      m_propertyName(p)
{
}

bool ViewElementAdapter::getValue(long& val)
{
    return m_viewElement->event()->get<Rosegarden::Int>(m_propertyName, val);
}

void ViewElementAdapter::setValue(long val)
{
    m_viewElement->event()->set<Rosegarden::Int>(m_propertyName, val);
}

timeT ViewElementAdapter::getTime()
{
    return m_viewElement->getViewAbsoluteTime();
}

timeT ViewElementAdapter::getDuration()
{
    return m_viewElement->getViewDuration();
}

//////////////////////////////

class ControllerEventAdapter : public ElementAdapter
{
public:
    ControllerEventAdapter(Event* e) : m_event(e) {}

    virtual bool getValue(long&);
    virtual void setValue(long);
    virtual timeT getTime();
    virtual timeT getDuration();

    virtual Event* getEvent() { return m_event; }

protected:

    //--------------- Data members ---------------------------------

    Event* m_event;
};

bool ControllerEventAdapter::getValue(long& val)
{
    if (m_event->getType() == Rosegarden::Controller::EventType)
    {
        return m_event->get<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
    }
    else if (m_event->getType() == Rosegarden::PitchBend::EventType)
    {
        long value = 0;
        value = m_event->get<Rosegarden::Int>(Rosegarden::PitchBend::MSB);
        value <<= 7;
        value |= m_event->get<Rosegarden::Int>(Rosegarden::PitchBend::LSB);

        //RG_DEBUG << "PitchBend Get Value = " << value << endl;

        val = value;
        return true;
    }

    return false;
}

void ControllerEventAdapter::setValue(long val)
{
    if (m_event->getType() == Rosegarden::Controller::EventType)
    {
        m_event->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
    }
    else if (m_event->getType() == Rosegarden::PitchBend::EventType)
    {
        RG_DEBUG << "PitchBend Set Value = " << val << endl;

        int lsb = val & 0x7f;
        int msb = (val >> 7) & 0x7f;
        m_event->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        m_event->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
    }
}

timeT ControllerEventAdapter::getTime()
{
    return m_event->getAbsoluteTime();
}

timeT ControllerEventAdapter::getDuration()
{
    return m_event->getDuration();
}

//
// ------------------
// ControlItem
// ------------------
//
class ControlItem : public QCanvasRectangle
{
public:
    ControlItem(ControlRuler* controlRuler,
                ElementAdapter* adapter,
                int x, int width = DefaultWidth);

    ~ControlItem();
    
    virtual void setValue(long);
    int getValue() const { return m_value; }

    void setWidth(int w)  { setSize(w, height()); }
    void setHeight(int h) { setSize(width(), h); }
    int getHeight()       { return size().height(); }

    virtual void draw(QPainter &painter);

    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);
    virtual void handleMouseWheel(QWheelEvent *e);

    virtual void setSelected(bool yes);

    /// recompute height according to represented value prior to a canvas repaint
    virtual void updateFromValue();

    /// update value according to height after a user edit
    virtual void updateValue();

    ElementAdapter* getElementAdapter() { return m_elementAdapter; }

protected:

    //--------------- Data members ---------------------------------

    long m_value;
    bool m_handlingMouseMove;

    ControlRuler* m_controlRuler;
    ElementAdapter* m_elementAdapter;

    static const unsigned int BorderThickness;
    static const unsigned int DefaultWidth;
};

const unsigned int ControlItem::BorderThickness = 1;
const unsigned int ControlItem::DefaultWidth    = 20;
static int _canvasItemZ = 30;

ControlItem::ControlItem(ControlRuler* ruler, ElementAdapter* elementAdapter,
                         int xx, int width)
    : QCanvasRectangle(ruler->canvas()),
      m_value(0),
      m_controlRuler(ruler),
      m_elementAdapter(elementAdapter)
{
    setWidth(width);
    setPen(QPen(Qt::black, BorderThickness));
    setBrush(Qt::blue);

    setX(xx);
    setY(canvas()->height());
    setZ(_canvasItemZ++); // we should make this work against controlruler

    updateFromValue();
    setEnabled(false);
    //RG_DEBUG << "ControlItem x = " << x() << " - y = " << y() << " - width = " << width << endl;
    show();
}

ControlItem::~ControlItem()
{
    delete m_elementAdapter;
}


void ControlItem::setValue(long v)
{
//     RG_DEBUG << "ControlItem::setValue(" << v << ") x = " << x() << endl;

    m_value = v;
}

void ControlItem::updateValue()
{
    m_elementAdapter->setValue(m_value);
}

void ControlItem::updateFromValue()
{
    if (m_elementAdapter->getValue(m_value)) {
//         RG_DEBUG << "ControlItem::updateFromValue() : value = " << m_value << endl;
        setHeight(m_controlRuler->valueToHeight(m_value));
    }
}

typedef std::pair<int, QCanvasItem*> ItemPair;
struct ItemCmp
{
    bool operator()(const ItemPair &i1, const ItemPair &i2)
    {
        return i1.first > i2.first;
    }
};

void ControlItem::draw(QPainter &painter)
{
    if (!isEnabled())
        updateFromValue();

    setBrush(m_controlRuler->valueToColour(m_controlRuler->getMaxItemValue(), m_value));

    QCanvasRectangle::draw(painter);
    

    /*

    // Attempt to get overlapping rectangles ordered automatically - 
    // probably best not to do this here - rwb

    // calculate collisions and assign Z values accordingly
    //
    QCanvasItemList l = collisions(false);

    std::vector<ItemPair> sortList;

    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != QCanvasItem::Rtti_Rectangle) continue;

        if (QCanvasRectangle *rect = dynamic_cast<QCanvasRectangle*>(*it))
            sortList.push_back(ItemPair(rect->height(), *it));
    }

    // sort the list by height
    std::sort(sortList.begin(), sortList.end(), ItemCmp());

    int z = 20;

    for (std::vector<ItemPair>::iterator it = sortList.begin();
         it != sortList.end(); ++it)
    {
        RG_DEBUG << "HEIGHT = " << (*it).first << endl;
        (*it).second->setZ(z++);
    }

    RG_DEBUG << endl << endl;
            
    canvas()->update();

    */

}

void ControlItem::handleMouseButtonPress(QMouseEvent*)
{
//     RG_DEBUG << "ControlItem::handleMouseButtonPress()" << this << endl;
    setEnabled(true);
}

void ControlItem::handleMouseButtonRelease(QMouseEvent*)
{
//     RG_DEBUG << "ControlItem::handleMouseButtonRelease()"  << this << endl;
    setEnabled(false);
}

void ControlItem::handleMouseMove(QMouseEvent*, int /*deltaX*/, int deltaY)
{
//     RG_DEBUG << "ControlItem::handleMouseMove()" << this << endl;

    // height is always negative
    //

    m_controlRuler->applyTool(x(), deltaY);

    int absNewHeight = -(getHeight() + deltaY);

    // Make sure height is within bounds
    if (absNewHeight > ControlRuler::MaxItemHeight)
        absNewHeight = ControlRuler::MaxItemHeight;
    else if (absNewHeight < ControlRuler::MinItemHeight)
        absNewHeight = ControlRuler::MinItemHeight;
    
    setHeight(-absNewHeight);
    setValue(m_controlRuler->heightToValue(getHeight()));
}

void ControlItem::handleMouseWheel(QWheelEvent *)
{
    RG_DEBUG << "ControlItem::handleMouseWheel - got wheel event" << endl;
}

void ControlItem::setSelected(bool s)
{
    QCanvasItem::setSelected(s);

    if (s) setPen(QPen(Qt::red, BorderThickness));
    else setPen(QPen(Qt::black, BorderThickness));

    canvas()->update();
}

//////////////////////////////////////////////////////////////////////

/**
 * Selector tool for the ControlRuler
 *
 * Allow the user to select several ControlItems so he can change them
 * all at the same time
 */
class ControlSelector : public QObject
{
public:
    ControlSelector(ControlRuler* parent);
    virtual ~ControlSelector() {};
    
    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);

    QCanvasRectangle* getSelectionRectangle() { return m_ruler->getSelectionRectangle(); }
protected:
    //--------------- Data members ---------------------------------

    ControlRuler* m_ruler;
};

ControlSelector::ControlSelector(ControlRuler* parent)
    : QObject(parent),
      m_ruler(parent)
{
}

void ControlSelector::handleMouseButtonPress(QMouseEvent *e)
{
    QPoint p = m_ruler->inverseMapPoint(e->pos());

    getSelectionRectangle()->setX(p.x());
    getSelectionRectangle()->setY(p.y());
    getSelectionRectangle()->setSize(0,0);

    getSelectionRectangle()->show();
    m_ruler->canvas()->update();
}

void ControlSelector::handleMouseButtonRelease(QMouseEvent*)
{
    getSelectionRectangle()->hide();
    m_ruler->canvas()->update();
}

void ControlSelector::handleMouseMove(QMouseEvent *e, int, int)
{
    QPoint p = m_ruler->inverseMapPoint(e->pos());

    int w = int(p.x() - getSelectionRectangle()->x());
    int h = int(p.y() - getSelectionRectangle()->y());
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    getSelectionRectangle()->setSize(w, h);

    m_ruler->canvas()->update();
}

//////////////////////////////////////////////////////////////////////

/**
 * Command defining a change (property change or similar) from the control ruler
 */
class ControlChangeCommand : public BasicCommand
{
public:

    ControlChangeCommand(QCanvasItemList selectedItems,
                         Segment &segment,
                         Rosegarden::timeT start, Rosegarden::timeT end);
    virtual ~ControlChangeCommand() {;}


protected:

    virtual void modifySegment();

    QCanvasItemList m_selectedItems;
};

ControlChangeCommand::ControlChangeCommand(QCanvasItemList selectedItems,
                                           Segment &segment,
                                           Rosegarden::timeT start, Rosegarden::timeT end)
    : BasicCommand(i18n("Control Change"), segment, start, end, true),
      m_selectedItems(selectedItems)
{
    RG_DEBUG << "ControlChangeCommand : from " << start << " to " << end << endl;
}


void ControlChangeCommand::modifySegment()
{
    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            item->updateValue();
    }
}

/**
 * Command for inserting a new controller event
 */
class ControlRulerEventInsertCommand : public BasicCommand
{
public:
    ControlRulerEventInsertCommand(const std::string &type, 
                                   timeT insertTime, 
                                   long number, 
                                   long initialValue,
                                   Segment &segment);

    virtual ~ControlRulerEventInsertCommand() {;}

protected:

    virtual void modifySegment();

    std::string m_type;
    long m_number;
    long m_initialValue;
};

ControlRulerEventInsertCommand::ControlRulerEventInsertCommand(const std::string &type,
                                                               timeT insertTime,
                                                               long number, long initialValue,
                                                               Segment &segment)
    : BasicCommand(i18n("Insert Controller Event"),
                   segment,
                   insertTime, 
                   (insertTime + Rosegarden::Note(Rosegarden::Note::Quaver).getDuration())), // must have a duration other undo doesn't work
      m_type(type),
      m_number(number),
      m_initialValue(initialValue)
{
}

void ControlRulerEventInsertCommand::modifySegment()
{
    Event* controllerEvent = new Event(m_type, getStartTime());

    if (m_type == Rosegarden::Controller::EventType)
    {
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, m_initialValue);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::NUMBER, m_number);
    }
    else if (m_type == Rosegarden::PitchBend::EventType)
    {
        // Convert to PitchBend MSB/LSB
        int lsb = m_initialValue & 0x7f;
        int msb = (m_initialValue >> 7) & 0x7f;
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        controllerEvent->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
    }
    
    getSegment().insert(controllerEvent);
}


/**
 * Command erasing selected controller events
 */
class ControllerEventEraseCommand : public BasicCommand
{
public:

    ControllerEventEraseCommand(QCanvasItemList selectedItems,
                                Segment &segment,
                                Rosegarden::timeT start, Rosegarden::timeT end);
    virtual ~ControllerEventEraseCommand() {;}


protected:

    virtual void modifySegment();

    QCanvasItemList m_selectedItems;
};

ControllerEventEraseCommand::ControllerEventEraseCommand(QCanvasItemList selectedItems,
                                                         Segment &segment,
                                                         Rosegarden::timeT start, Rosegarden::timeT end)
    : BasicCommand(i18n("Erase Controller Event(s)"),
                   segment,
                   start,
                   (start == end) ? start + 10 : end, 
                   true),
      m_selectedItems(selectedItems)
{
    RG_DEBUG << "ControllerEventEraseCommand : from " << start << " to " << end << endl;
}


void ControllerEventEraseCommand::modifySegment()
{
    Segment &segment(getSegment());

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            segment.eraseSingle(item->getElementAdapter()->getEvent());
    }
}

//////////////////////////////////////////////////////////////////////

using Rosegarden::Staff;
using Rosegarden::ViewElementList;

const int ControlRuler::DefaultRulerHeight = 75;
const int ControlRuler::MinItemHeight = 5;
const int ControlRuler::MaxItemHeight = 64 + 5;
const int ControlRuler::ItemHeightRange = 64;

ControlRuler::ControlRuler(Segment& segment,
                           Rosegarden::RulerScale* rulerScale,
                           EditViewBase* parentView,
                           QCanvas* c, QWidget* parent,
                           const char* name, WFlags f) :
    RosegardenCanvasView(c, parent, name, f),
    m_parentEditView(parentView),
    m_rulerScale(rulerScale),
    m_eventSelection(new EventSelection(segment)),
    m_segment(segment),
    m_currentItem(0),
    m_tool(0),
    m_maxItemValue(127),
    m_staffOffset(0),
    m_currentX(0.0),
    m_itemMoved(false),
    m_selecting(false),
    m_selector(new ControlSelector(this)),
    m_selectionRect(new QCanvasRectangle(canvas())),
    m_menu(0)
{
    setHScrollBarMode(QScrollView::AlwaysOff);

    setControlTool(new TestTool);
    m_selectionRect->setPen(Qt::red);

    setFixedHeight(sizeHint().height());

    connect(this, SIGNAL(stateChange(const QString&, bool)),
            m_parentEditView, SLOT(slotStateChanged(const QString&, bool)));

    m_numberFloat = new RosegardenTextFloat(this);
    m_numberFloat->hide();

    emit stateChange("have_controller_item_selected", false);
}

ControlRuler::~ControlRuler()
{
}

void ControlRuler::slotUpdate()
{
    RG_DEBUG << "ControlRuler::slotUpdate()\n";

    canvas()->setAllChanged(); // TODO: be a bit more subtle, call setChanged(<time area>)

    canvas()->update();
    repaint();
}

void ControlRuler::slotUpdateElementsHPos()
{
    computeStaffOffset();

    QCanvasItemList list = canvas()->allItems();

    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
        ControlItem* item = dynamic_cast<ControlItem*>(*it);
        if (!item) continue;
        layoutItem(item);
    }

    canvas()->update();
}

void ControlRuler::layoutItem(ControlItem* item)
{
    timeT itemTime = item->getElementAdapter()->getTime();
    
    double x = m_rulerScale->getXForTime(itemTime);
    
    item->setX(x + m_staffOffset);
    int itemElementDuration = item->getElementAdapter()->getDuration();
    
    int width = int(m_rulerScale->getXForTime(itemTime + itemElementDuration) - x);

    item->setWidth(width);

//     RG_DEBUG << "ControlRuler::layoutItem ControlItem x = " << x << " - width = " << width << endl;
}

void ControlRuler::setControlTool(ControlTool* tool)
{
    if (m_tool) delete m_tool;
    m_tool = tool;
}

void ControlRuler::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        m_numberFloat->hide();
        m_selecting = false;
        return;
    }

    RG_DEBUG << "ControlRuler::contentsMousePressEvent()\n";

    QPoint p = inverseMapPoint(e->pos());

    QCanvasItemList l=canvas()->collisions(p);

    if (l.count() == 0) { // de-select current item
        clearSelectedItems();
        m_selecting = true;
        m_selector->handleMouseButtonPress(e);
        RG_DEBUG << "ControlRuler::contentsMousePressEvent : entering selection mode\n";
        return;
    }

    // clear selection unless control was pressed, in which case 
    // add the event to the current selection
    if (!(e->state() && QMouseEvent::ControlButton)) { 
        clearSelectedItems(); 
    }

    ControlItem *topItem = 0;
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            if (topItem == 0) topItem = item;

            if (item->isSelected()) { // if the item which was clicked
                                      // on is part of a selection,
                                      // propagate mousepress on all
                                      // selected items

                item->handleMouseButtonPress(e);
                
                for (QCanvasItemList::Iterator it=m_selectedItems.begin(); 
                        it!=m_selectedItems.end(); ++it) {
                    if (ControlItem *selectedItem = 
                            dynamic_cast<ControlItem*>(*it)) {
                        selectedItem->handleMouseButtonPress(e);
                    }
                }
                

            } else { // select it
            
                if (!(e->state() && QMouseEvent::ControlButton)) { 
                    if (item->z() > topItem->z()) topItem = item;

                } else {
                    m_selectedItems << item;
                    item->setSelected(true);
                    item->handleMouseButtonPress(e);
                    ElementAdapter* adapter = item->getElementAdapter();
                    m_eventSelection->addEvent(adapter->getEvent());
                }
            }
        }
    }

    if (topItem) { // select the top item
        m_selectedItems << topItem;
        topItem->setSelected(true);
        topItem->handleMouseButtonPress(e);
        ElementAdapter* adapter = topItem->getElementAdapter();
        m_eventSelection->addEvent(adapter->getEvent());
    }

    m_itemMoved = false;
    m_lastEventPos = p;
}

void ControlRuler::contentsMouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
    {
        m_numberFloat->hide();
        m_selecting = false;
        return;
    }
    
    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseButtonRelease(e);
        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : leaving selection mode\n";
        m_selecting = false;
        return;
    }

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
            item->handleMouseButtonRelease(e);
        }
    }

    emit stateChange("have_controller_item_selected", true);

    if (m_itemMoved) {

        m_lastEventPos = inverseMapPoint(e->pos());

        // Add command to history
        ControlChangeCommand* command = new ControlChangeCommand(m_selectedItems,
                                                                 m_segment,
                                                                 m_eventSelection->getStartTime(),
                                                                 m_eventSelection->getEndTime());

        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : adding command\n";
        m_parentEditView->addCommandToHistory(command);

        m_itemMoved = false;
    }

    m_numberFloat->hide();
}

void ControlRuler::contentsMouseMoveEvent(QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());

    int deltaX = p.x() - m_lastEventPos.x(),
        deltaY = p.y() - m_lastEventPos.y();
    m_lastEventPos = p;

    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseMove(e, deltaX, deltaY);
        slotScrollHorizSmallSteps(p.x());
        return;
    }

    m_itemMoved = true;

    // Borrowed from RosegardenRotary - compute total position within window
    //
    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));

    int scrollX = dynamic_cast<EditView*>(m_parentEditView)->getRawCanvasView()->
        horizontalScrollBar()->value();

    /*
    RG_DEBUG << "ControlRuler::contentsMouseMoveEvent - total pos = " << totalPos.x()
             << ",e pos = " << e->pos().x()
             << ", scroll bar = " << scrollX
             << endl;
             */

    // Allow for scrollbar
    //
    m_numberFloat->move(totalPos.x() + e->pos().x() - scrollX + 20, 
                        totalPos.y() + e->pos().y() - 10);

    int value = 0;

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            item->handleMouseMove(e, deltaX, deltaY);
//            ElementAdapter* adapter = item->getElementAdapter();

            // set value to highest in selection
            if (item->getValue() > value)
            {
                value = item->getValue();
                m_numberFloat->setText(QString("%1").arg(value));
            }
        }
    }
    canvas()->update();

    m_numberFloat->show();

}

void
ControlRuler::contentsWheelEvent(QWheelEvent *e)
{
    // not sure what to do yet
    QCanvasView::contentsWheelEvent(e);
}

void ControlRuler::updateSelection()
{
    clearSelectedItems();

    bool haveSelectedItems = false;

    QCanvasItemList l=getSelectionRectangle()->collisions(true);

    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            item->setSelected(true);
            m_selectedItems << item;
            haveSelectedItems = true;

            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
        }
    }

    emit stateChange("have_controller_item_selected", haveSelectedItems);
}

void ControlRuler::contentsContextMenuEvent(QContextMenuEvent* e)
{
    if (!m_menu && !m_menuName.isEmpty()) createMenu();

    if (m_menu) {
        RG_DEBUG << "ControlRuler::showMenu() - show menu with" << m_menu->count() << " items\n";
        m_lastEventPos = inverseMapPoint(e->pos());
        m_menu->exec(QCursor::pos());
    } else
        RG_DEBUG << "ControlRuler::showMenu() : no menu to show\n";

}

void ControlRuler::createMenu()
{
    RG_DEBUG << "ControlRuler::createMenu()\n";

    KMainWindow* parentMainWindow = dynamic_cast<KMainWindow*>(topLevelWidget());

    if (parentMainWindow && parentMainWindow->factory()) {
        m_menu = static_cast<QPopupMenu*>(parentMainWindow->factory()->container(m_menuName, parentMainWindow));

        if (!m_menu) {
            RG_DEBUG << "ControlRuler::createMenu() failed\n";
        }
    } else {
        RG_DEBUG << "ControlRuler::createMenu() failed: no parent factory\n";
    }
}

void
ControlRuler::clearSelectedItems()
{
    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        (*it)->setSelected(false);
    }
    m_selectedItems.clear();

    delete m_eventSelection;
    m_eventSelection = new EventSelection(m_segment);
}

void ControlRuler::clear()
{
    QCanvasItemList list = canvas()->allItems();
    QCanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
	if (*it != m_selectionRect)
	    delete *it;
    }
}

int ControlRuler::valueToHeight(long val)
{
    long scaleVal = val * (ItemHeightRange);
    
    int res = -(int(scaleVal / getMaxItemValue()) + MinItemHeight);

    //RG_DEBUG << "ControlRuler::valueToHeight : val = " << val << " - height = " << res
             //<< " - scaleVal = " << scaleVal << endl;

    return res;
}

long ControlRuler::heightToValue(int h)
{
    long val = -h;
    val -= MinItemHeight;
    val *= getMaxItemValue();
    val /= (ItemHeightRange);
    val = std::min(val, long(getMaxItemValue()));

     RG_DEBUG << "ControlRuler::heightToValue : height = " << h << " - val = " << val << endl;

    return val;
}

QColor ControlRuler::valueToColour(int max, int val)
{
    int maxDefault = DefaultVelocityColour::getInstance()->getMaxValue();

    int value = val;

    // Scale value accordingly
    //
    if (maxDefault != max) value = int(double(maxDefault) * double(val)/double(max));

    return DefaultVelocityColour::getInstance()->getColour(value);
}



int ControlRuler::applyTool(double x, int val)
{
    if (m_tool) return (*m_tool)(x, val);
    return val;
}

//----------------------------------------

PropertyControlRuler::PropertyControlRuler(Rosegarden::PropertyName propertyName,
                                           Staff* staff,
                                           Rosegarden::RulerScale* rulerScale,
                                           EditViewBase* parentView,
                                           QCanvas* c, QWidget* parent,
                                           const char* name, WFlags f) :
    ControlRuler(staff->getSegment(), rulerScale, parentView, c, parent, name, f),
    m_propertyName(propertyName),
    m_staff(staff),
    m_propertyLine(new QCanvasLine(canvas())),
    m_propertyLineShowing(false),
    m_propertyLineX(0),
    m_propertyLineY(0)
{
    m_staff->addObserver(this);
    m_propertyLine->setZ(1000); // bring to front

    setMenuName("property_ruler_menu");
    drawBackground();
    init();
}

void
PropertyControlRuler::drawBackground()
{
    // Draw some minimum and maximum controller value guide lines
    //
    QCanvasLine *topLine = new QCanvasLine(canvas());
    QCanvasLine *topQLine = new QCanvasLine(canvas());
    QCanvasLine *midLine = new QCanvasLine(canvas());
    QCanvasLine *botQLine = new QCanvasLine(canvas());
    QCanvasLine *bottomLine = new QCanvasLine(canvas());
    //m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
    int cHeight = canvas()->height();
    int cWidth = canvas()->width();

    topLine->setPen(QColor(127, 127, 127));
    topLine->setPoints(0, 0, cWidth, 0);
    topLine->setZ(-10);
    topLine->show();

    topQLine->setPen(QColor(192, 192, 192));
    topQLine->setPoints(0, cHeight/4, cWidth, cHeight/4);
    topQLine->setZ(-10);
    topQLine->show();

    midLine->setPen(QColor(127, 127, 127));
    midLine->setPoints(0, cHeight/2, cWidth, cHeight/2);
    midLine->setZ(-10);
    midLine->show();

    botQLine->setPen(QColor(192, 192, 192));
    botQLine->setPoints(0, 3*cHeight/4, cWidth, 3*cHeight/4);
    botQLine->setZ(-10);
    botQLine->show();

    bottomLine->setPen(QColor(127, 127, 127));
    bottomLine->setPoints(0, cHeight - 1, cWidth, cHeight - 1);
    bottomLine->setZ(-10);
    bottomLine->show();
}


PropertyControlRuler::~PropertyControlRuler()
{
    if (m_staff)
        m_staff->removeObserver(this);
}

QString PropertyControlRuler::getName()
{
    return getPropertyName().c_str();
}

void PropertyControlRuler::init()
{
    ViewElementList* viewElementList = m_staff->getViewElementList();

    LinedStaff* lStaff = dynamic_cast<LinedStaff*>(m_staff);
    
    if (lStaff)
        m_staffOffset = lStaff->getX();

    for(ViewElementList::iterator i = viewElementList->begin();
        i != viewElementList->end(); ++i) {

 	double x = m_rulerScale->getXForTime((*i)->getViewAbsoluteTime());
 	new ControlItem(this, new ViewElementAdapter(*i, getPropertyName()), int(x + m_staffOffset),
                        int(m_rulerScale->getXForTime((*i)->getViewAbsoluteTime() +
                                                      (*i)->getViewDuration()) - x));

    }
}

void PropertyControlRuler::elementAdded(const Rosegarden::Staff *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementAdded()\n";

    double x = m_rulerScale->getXForTime(el->getViewAbsoluteTime());

    new ControlItem(this, new ViewElementAdapter(el, getPropertyName()), int(x + m_staffOffset),
                    int(m_rulerScale->getXForTime(el->getViewAbsoluteTime() +
                                                  el->getViewDuration()) - x));
}

void PropertyControlRuler::elementRemoved(const Rosegarden::Staff *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementRemoved(\n";

    clearSelectedItems();

    QCanvasItemList allItems = canvas()->allItems();

    for (QCanvasItemList::Iterator it=allItems.begin(); it!=allItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            ViewElementAdapter* adapter = dynamic_cast<ViewElementAdapter*>(item->getElementAdapter());
            if (adapter->getViewElement() == el) {
                delete item;
                break;
            }
        }
    }
}

void PropertyControlRuler::staffDeleted(const Rosegarden::Staff *)
{
    m_staff = 0;
}

void PropertyControlRuler::computeStaffOffset()
{
    LinedStaff* lStaff = dynamic_cast<LinedStaff*>(m_staff);
    if (lStaff)
        m_staffOffset = lStaff->getX();
}

void PropertyControlRuler::startPropertyLine()
{
    RG_DEBUG << "PropertyControlRuler::startPropertyLine" << endl;
    m_propertyLineShowing = true;
    this->setCursor(Qt::pointingHandCursor);
}

void
PropertyControlRuler::contentsMousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMousePressEvent" << endl;

    if (!m_propertyLineShowing)
    {
        if (e->button() == MidButton)
            m_lastEventPos = inverseMapPoint(e->pos());
        
        ControlRuler::contentsMousePressEvent(e); // send super

        return;
    }

    // cancel control line mode
    if (e->button() == RightButton)
    {
        m_propertyLineShowing = false;
        m_propertyLine->hide();

        this->setCursor(Qt::arrowCursor);
        return;
    }

    if (e->button() == LeftButton)
    {
        QPoint p = inverseMapPoint(e->pos());
        
        m_propertyLine->show();
        m_propertyLineX = p.x();
        m_propertyLineY = p.y();
        m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, m_propertyLineX, m_propertyLineY);
        canvas()->update();
    }
}

void 
PropertyControlRuler::contentsMouseReleaseEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMouseReleaseEvent" << endl;

    /*
    if (m_propertyLineShowing)
    {
        this->setCursor(Qt::arrowCursor);
        m_propertyLineShowing = false;
        canvas()->update();
    }
    */

    if (!m_propertyLineShowing)
    {
        /*
        if (e->button() == MidButton)
            insertControllerEvent();
            */

        ControlRuler::contentsMouseReleaseEvent(e); // send super
        return;
    }
    else
    {
        QPoint p = inverseMapPoint(e->pos());

        timeT startTime = m_rulerScale->getTimeForX(m_propertyLineX);
        timeT endTime = m_rulerScale->getTimeForX(p.x());

        long startValue = heightToValue(m_propertyLineY - canvas()->height());
        long endValue = heightToValue(p.y() - canvas()->height());

        RG_DEBUG << "PropertyControlRuler::contentsMouseReleaseEvent - "
                 << "starttime = " << startTime
                 << ", endtime = " << endTime
                 << ", startValue = " << startValue
                 << ", endValue = " << endValue
                 << endl;

        drawPropertyLine(startTime, endTime, startValue, endValue);

        m_propertyLineShowing = false;
        m_propertyLine->hide();
        this->setCursor(Qt::arrowCursor);
        canvas()->update();
    }
}

void 
PropertyControlRuler::contentsMouseMoveEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMouseMoveEvent" << endl;

    if (!m_propertyLineShowing)
    {
        // Don't send super if we're using the middle button
        //
        if (e->button() == MidButton)
        {
            m_lastEventPos = inverseMapPoint(e->pos());
            return;
        }

        ControlRuler::contentsMouseMoveEvent(e); // send super
        return;
    }

    QPoint p = inverseMapPoint(e->pos());

    m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, p.x(), p.y());
    canvas()->update();
}

void 
PropertyControlRuler::drawPropertyLine(Rosegarden::timeT startTime,
                                       Rosegarden::timeT endTime,
                                       int startValue,
                                       int endValue)
{
    if (startTime > endTime) 
    {
        std::swap(startTime, endTime);
        std::swap(startValue, endValue);
    }

    RG_DEBUG << "PropertyControlRuler::drawPropertyLine - "
             << "set velocity from " << startTime 
             << " to " << endTime << endl;

    // Add the "true" to catch Events overlapping this line
    //
    Rosegarden::EventSelection selection(m_segment, startTime, endTime, true);
    Rosegarden::PropertyPattern pattern = Rosegarden::DecrescendoPattern;

    SelectionPropertyCommand *command = 
        new SelectionPropertyCommand(&selection,
                                     Rosegarden::BaseProperties::VELOCITY,
                                     pattern,
                                     startValue,
                                     endValue);

    m_parentEditView->addCommandToHistory(command);

}

void
PropertyControlRuler::selectAllProperties()
{
    RG_DEBUG << "PropertyControlRuler::selectAllProperties" << endl;

    /*
    for(Segment::iterator i = m_segment.begin();
                    i != m_segment.end(); ++i)
        if (!m_eventSelection->contains(*i)) m_eventSelection->addEvent(*i);
    */

    clearSelectedItems();

    QCanvasItemList l=canvas()->allItems();
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) 
    {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
        {
            m_selectedItems << item;
            (*it)->setSelected(true);
            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
        }
    }

    /*
    m_eventSelection->addFromSelection(&selection);
    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
            item->handleMouseButtonRelease(e);
        }
    }
    */

    emit stateChange("have_controller_item_selected", true);
}

//----------------------------------------
ControllerEventsRuler::ControllerEventsRuler(Rosegarden::Segment& segment,
                                             Rosegarden::RulerScale* rulerScale,
                                             EditViewBase* parentView,
                                             QCanvas* c,
                                             QWidget* parent,
                                             ControlParameter *controller,
                                             const char* name, WFlags f)
    : ControlRuler(segment, rulerScale, parentView, c, parent, name, f),
      m_segmentDeleted(false),
      m_defaultItemWidth(20),
      m_controlLine(new QCanvasLine(canvas())),
      m_controlLineShowing(false),
      m_controlLineX(0),
      m_controlLineY(0)
{
    // Make a copy of the ControlParameter if we have one
    //
    if (controller)
        m_controller = new ControlParameter(*controller);
    else
        m_controller = 0;

    // Reset range information for this controller type (for the moment
    // this assumes min is always 0.
    //
    setMaxItemValue(m_controller->getMax());

    m_segment.addObserver(this);

    for(Segment::iterator i = m_segment.begin();
        i != m_segment.end(); ++i) {

        // skip if not the same type of event that we're expecting
        //
        if (m_controller->getType() != (*i)->getType()) continue;

        int width = getDefaultItemWidth();

        // Check for specific controller value if we need to 
        //
        if (m_controller->getType() == Rosegarden::Controller::EventType)
        {
            try
            {
                if ((*i)->get<Rosegarden::Int>(Rosegarden::Controller::NUMBER)
                        !=  m_controller->getControllerValue())
                    continue;
            }
            catch(...)
            {
                continue;
            }
        }
        else if (m_controller->getType() == Rosegarden::PitchBend::EventType)
            width /= 4;

        //RG_DEBUG << "ControllerEventsRuler: adding element\n";

 	double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime());
 	new ControlItem(this, new ControllerEventAdapter(*i), int(x + m_staffOffset), width);

    }
    
    setMenuName("controller_events_ruler_menu");

    // Draw the background lines
    //
    drawBackground();
}

void
ControllerEventsRuler::drawBackground()
{
    // Draw some minimum and maximum controller value guide lines
    //
    QCanvasLine *topLine = new QCanvasLine(canvas());
    QCanvasLine *topQLine = new QCanvasLine(canvas());
    QCanvasLine *midLine = new QCanvasLine(canvas());
    QCanvasLine *botQLine = new QCanvasLine(canvas());
    QCanvasLine *bottomLine = new QCanvasLine(canvas());
    //m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
    int cHeight = canvas()->height();
    int cWidth = canvas()->width();

    topLine->setPen(QColor(127, 127, 127));
    topLine->setPoints(0, 0, cWidth, 0);
    topLine->setZ(-10);
    topLine->show();

    topQLine->setPen(QColor(192, 192, 192));
    topQLine->setPoints(0, cHeight/4, cWidth, cHeight/4);
    topQLine->setZ(-10);
    topQLine->show();

    midLine->setPen(QColor(127, 127, 127));
    midLine->setPoints(0, cHeight/2, cWidth, cHeight/2);
    midLine->setZ(-10);
    midLine->show();

    botQLine->setPen(QColor(192, 192, 192));
    botQLine->setPoints(0, 3*cHeight/4, cWidth, 3*cHeight/4);
    botQLine->setZ(-10);
    botQLine->show();

    bottomLine->setPen(QColor(127, 127, 127));
    bottomLine->setPoints(0, cHeight - 1, cWidth, cHeight - 1);
    bottomLine->setZ(-10);
    bottomLine->show();

    canvas()->update();
}


ControllerEventsRuler::~ControllerEventsRuler()
{
    if (!m_segmentDeleted)
        m_segment.removeObserver(this);
}


QString ControllerEventsRuler::getName()
{
    if (m_controller) 
    {
        QString name = i18n("Unsupported Event Type");

        if (m_controller->getType() == Rosegarden::Controller::EventType)
        {
            QString hexValue;
            hexValue.sprintf("0x%x", m_controller->getControllerValue());

            name = QString("%1 (%2 / %3)").arg(strtoqstr(m_controller->getName()))
                                          .arg(int(m_controller->getControllerValue()))
                                          .arg(hexValue);
        }
        else if (m_controller->getType() == Rosegarden::PitchBend::EventType)
        {
            name = i18n("Pitch Bend");
        }

        return name;
    }
    else return i18n("Controller Events");
}

void ControllerEventsRuler::eventAdded(const Segment*, Event *e)
{
    if (e->getType() != m_controller->getType()) return;

    // Check for specific controller value if we need to 
    //
    if (e->getType() == Rosegarden::Controller::EventType)
    {
        try
        {
            if (e->get<Rosegarden::Int>(Rosegarden::Controller::NUMBER) != 
                    m_controller->getControllerValue())
                return;
        }
        catch(...)
        {
            return;
        }
    }

    RG_DEBUG << "ControllerEventsRuler::elementAdded()\n";

    double x = m_rulerScale->getXForTime(e->getAbsoluteTime());

    int width = getDefaultItemWidth();

    if (m_controller->getType() == Rosegarden::PitchBend::EventType)
        width /= 4;

    new ControlItem(this, new ControllerEventAdapter(e), int(x + m_staffOffset), width);
}

void ControllerEventsRuler::eventRemoved(const Segment*, Event *e)
{
    if (e->getType() != m_controller->getType()) return;

    clearSelectedItems();

    QCanvasItemList allItems = canvas()->allItems();

    for (QCanvasItemList::Iterator it=allItems.begin(); it!=allItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            ControllerEventAdapter* adapter = dynamic_cast<ControllerEventAdapter*>(item->getElementAdapter());
            if (adapter->getEvent() == e) {
                delete item;
                break;
            }
        }
    }
}

void ControllerEventsRuler::endMarkerTimeChanged(const Segment*, bool)
{
    // nothing to do
}

void ControllerEventsRuler::segmentDeleted(const Segment*)
{
    m_segmentDeleted = false;
}

void ControllerEventsRuler::insertControllerEvent()
{
    timeT insertTime = m_rulerScale->getTimeForX(m_lastEventPos.x());


    // compute initial value from cursor height
    //
    long initialValue = heightToValue(m_lastEventPos.y() - canvas()->height());

    RG_DEBUG << "ControllerEventsRuler::insertControllerEvent() : inserting event at "
             << insertTime
             << " - initial value = " << initialValue
             << endl;

    // ask controller number to user
    long number = 0;
    
    if (m_controller)
    {
        number = m_controller->getControllerValue();
    }
    else
    {
        bool ok = false;
        QIntValidator intValidator(0, 128, this);
        QString res = KLineEditDlg::getText(i18n("Controller Event Number"), "0",
                                            &ok, this, &intValidator);
        if (ok) number = res.toULong();
    }
    
    ControlRulerEventInsertCommand* command = 
        new ControlRulerEventInsertCommand(m_controller->getType(),
                                           insertTime, number, initialValue, m_segment);

    m_parentEditView->addCommandToHistory(command);
}

void ControllerEventsRuler::eraseControllerEvent()
{
    RG_DEBUG << "ControllerEventsRuler::eraseControllerEvent() : deleting selected events\n";

    ControllerEventEraseCommand* command = 
        new ControllerEventEraseCommand(m_selectedItems,
                                        m_segment,
                                        m_eventSelection->getStartTime(),
                                        m_eventSelection->getEndTime());
    m_parentEditView->addCommandToHistory(command);
    updateSelection();
}

void ControllerEventsRuler::clearControllerEvents()
{
    Rosegarden::EventSelection *es = new Rosegarden::EventSelection(m_segment);

    for(Segment::iterator it = m_segment.begin(); it != m_segment.end(); ++it)
    {
        if (!(*it)->isa(Rosegarden::Controller::EventType)) continue;
        {
            if (m_controller) // ensure we have only the controller events we want for this ruler
            {
                try
                {
                    if ((*it)->get<Rosegarden::Int>(Rosegarden::Controller::NUMBER)
                                                !=  m_controller->getControllerValue())
                        continue;
                }
                catch(...)
                {
                    continue;
                }

                es->addEvent(*it);
            }
        }
    }

    EraseCommand *command = new EraseCommand(*es);
    m_parentEditView->addCommandToHistory(command);

}

void ControllerEventsRuler::startControlLine()
{
    m_controlLineShowing = true;
    this->setCursor(Qt::pointingHandCursor);
}

void ControllerEventsRuler::flipForwards()
{
    std::pair<int, int> minMax = getZMinMax();

    QCanvasItemList l = canvas()->allItems();
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != QCanvasItem::Rtti_Rectangle) continue;

        // match min
        if ((*it)->z() == minMax.second) (*it)->setZ(minMax.first);
        else (*it)->setZ((*it)->z() + 1);
    }

    canvas()->update();
}

void ControllerEventsRuler::flipBackwards()
{
    std::pair<int, int> minMax = getZMinMax();

    QCanvasItemList l = canvas()->allItems();
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != QCanvasItem::Rtti_Rectangle) continue;

        // match min
        if ((*it)->z() == minMax.first) (*it)->setZ(minMax.second);
        else (*it)->setZ((*it)->z() - 1);
    }

    canvas()->update();
}

std::pair<int, int> ControllerEventsRuler::getZMinMax()
{
    QCanvasItemList l = canvas()->allItems();
    std::vector<int> zList;
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != QCanvasItem::Rtti_Rectangle) continue;
        zList.push_back(int((*it)->z()));
    }

    std::sort(zList.begin(), zList.end());

    return std::pair<int, int>(zList[0], zList[zList.size() - 1]);


}



void ControllerEventsRuler::contentsMousePressEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        if (e->button() == MidButton)
            m_lastEventPos = inverseMapPoint(e->pos());
        
        ControlRuler::contentsMousePressEvent(e); // send super

        return;
    }

    // cancel control line mode
    if (e->button() == RightButton)
    {
        m_controlLineShowing = false;
        m_controlLine->hide();

        this->setCursor(Qt::arrowCursor);
        return;
    }

    if (e->button() == LeftButton)
    {
        QPoint p = inverseMapPoint(e->pos());
        
        m_controlLine->show();
        m_controlLineX = p.x();
        m_controlLineY = p.y();
        m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
        canvas()->update();
    }
}

void ControllerEventsRuler::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        if (e->button() == MidButton)
            insertControllerEvent();

         ControlRuler::contentsMouseReleaseEvent(e); // send super

        return;
    }
    else
    {
        QPoint p = inverseMapPoint(e->pos());

        timeT startTime = m_rulerScale->getTimeForX(m_controlLineX);
        timeT endTime = m_rulerScale->getTimeForX(p.x());

        long startValue = heightToValue(m_controlLineY - canvas()->height());
        long endValue = heightToValue(p.y() - canvas()->height());

        RG_DEBUG << "ControllerEventsRuler::contentsMouseReleaseEvent - "
                 << "starttime = " << startTime
                 << ", endtime = " << endTime
                 << ", startValue = " << startValue
                 << ", endValue = " << endValue
                 << endl;

        drawControlLine(startTime, endTime, startValue, endValue);

        m_controlLineShowing = false;
        m_controlLine->hide();
        this->setCursor(Qt::arrowCursor);
        canvas()->update();
    }
}

void ControllerEventsRuler::contentsMouseMoveEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        // Don't send super if we're using the middle button
        //
        if (e->button() == MidButton)
        {
            m_lastEventPos = inverseMapPoint(e->pos());
            return;
        }

        ControlRuler::contentsMouseMoveEvent(e); // send super
        return;
    }

    QPoint p = inverseMapPoint(e->pos());

    m_controlLine->setPoints(m_controlLineX, m_controlLineY, p.x(), p.y());
    canvas()->update();

}

void ControllerEventsRuler::layoutItem(ControlItem* item)
{
    timeT itemTime = item->getElementAdapter()->getTime();
    
    double x = m_rulerScale->getXForTime(itemTime) + m_staffOffset;
    
    item->setX(x);
    
    int width = getDefaultItemWidth(); // TODO: how to scale that ??

    if (m_controller->getType() == Rosegarden::PitchBend::EventType)
        width /= 4;

    item->setWidth(width);

    //RG_DEBUG << "ControllerEventsRuler::layoutItem ControlItem x = " << x 
             //<< " - width = " << width << endl;
}

void
ControllerEventsRuler::drawControlLine(Rosegarden::timeT startTime,
                                       Rosegarden::timeT endTime,
                                       int startValue,
                                       int endValue)
{
    if (m_controller == 0) return;
    if (startTime > endTime) 
    {
        std::swap(startTime, endTime);
        std::swap(startValue, endValue);
    }

    Rosegarden::timeT quantDur = Rosegarden::Note(Rosegarden::Note::Quaver).getDuration();

    // If inserting a line of PitchBends then we want a smoother curve
    //
    if (m_controller->getType() == Rosegarden::PitchBend::EventType)
        quantDur = Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration();

    // for the moment enter a quantized set of events
    Rosegarden::timeT time = startTime, newTime = 0;
    double step = double(endValue - startValue) / double(endTime - startTime);

    KMacroCommand *macro = new KMacroCommand(i18n("Add line of controllers"));

    while (time < endTime)
    {
        int value = startValue + int(step * double(time - startTime));

        // hit the buffers
        if (value < m_controller->getMin())
            value = m_controller->getMin();
        else if (value > m_controller->getMax())
            value = m_controller->getMax();

        ControlRulerEventInsertCommand* command = 
            new ControlRulerEventInsertCommand(m_controller->getType(),
                    time, m_controller->getControllerValue(), value, m_segment);

        macro->addCommand(command);

        // get new time - do it by quantized distances
        newTime = (time / quantDur) * quantDur;
        if (newTime > time) time = newTime;
        else time += quantDur;
    }

    m_parentEditView->addCommandToHistory(macro);
}




//----------------------------------------

PropertyViewRuler::PropertyViewRuler(RulerScale *rulerScale,
                                     Segment *segment,
                                     const PropertyName &property,
                                     double xorigin,
                                     int height,
                                     QWidget *parent,
                                     const char *name) :
    QWidget(parent, name),
    m_propertyName(property),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_segment(segment),
    m_rulerScale(rulerScale),
    m_fontMetrics(m_boldFont)
{
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);

    setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    QString tip = i18n("%1 controller").arg(strtoqstr(property));
    QToolTip::add(this, tip);
}

PropertyViewRuler::~PropertyViewRuler()
{
    // nothing
}

void
PropertyViewRuler::slotScrollHoriz(int x)
{
    int w = width(), h = height();
    x = int(double(x)/getHScaleFactor());
    int dx = x - (-m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3/4 || dx < -w*3/4) {
	update();
	return;
    }

    if (dx > 0) { // moving right, so the existing stuff moves left
	bitBlt(this, 0, 0, this, dx, 0, w-dx, h);
	repaint(w-dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
	bitBlt(this, -dx, 0, this, 0, 0, w+dx, h);
	repaint(0, 0, -dx, h);
    }
}

QSize
PropertyViewRuler::sizeHint() const
{
    double width =
       m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
       m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
       m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
PropertyViewRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
PropertyViewRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);

    if (getHScaleFactor() != 1.0) paint.scale(getHScaleFactor(), 1.0);

    paint.setPen(RosegardenGUIColours::MatrixElementBorder);

    QRect clipRect = e->rect().normalize();

    timeT from = m_rulerScale->getTimeForX
       (clipRect.x() - m_currentXOffset - m_xorigin);

    Segment::iterator it = m_segment->findNearestTime(from);

    for (; m_segment->isBeforeEndMarker(it); it++) {
        long value = 0;
        
        if (!(*it)->get<Rosegarden::Int>(m_propertyName, value))
            continue;
        
        int x = int(m_rulerScale->getXForTime((*it)->getAbsoluteTime()))
            + m_currentXOffset + int(m_xorigin);

        int xPos = x * int(getHScaleFactor());

        if (xPos < clipRect.x()) continue;

        if (xPos > (clipRect.x() + clipRect.width())) break;

        // include fiddle factor (+2)
        int width = 
            int(m_rulerScale->getXForTime((*it)->getAbsoluteTime() +
                                          (*it)->getDuration()) + 2)
            + m_currentXOffset + int(m_xorigin) - x;

        int blockHeight = int(double(height()) * (value/127.0));

        paint.setBrush(DefaultVelocityColour::getInstance()->getColour(value));
            
        paint.drawRect(x, height() - blockHeight, width, blockHeight);
    }
}


// ----------------------------- PropertyBox -------------------------------
//

PropertyBox::PropertyBox(QString label,
                       int width,
                       int height,
                       QWidget *parent,
                       const char *name):
        QWidget(parent, name),
        m_label(label),
        m_width(width),
        m_height(height)
{
}

QSize
PropertyBox::sizeHint() const
{
    return QSize(m_width, m_height);
}


QSize
PropertyBox::minimumSizeHint() const
{
    return QSize(m_width, m_height);
}

void
PropertyBox::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setPen(RosegardenGUIColours::MatrixElementBorder);
    //paint.setBrush(RosegardenGUIColours::MatrixElementBlock);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.drawRect(2, 2, m_width - 3, m_height - 3);
    paint.drawText(10, 2 * m_height / 3, m_label);
}

// ----------------------------- ControlRulerCanvasRepository -------------------------------
//


// void ControlRulerCanvasRepository::clear()
// {
//     segmentpropertycanvasmap& segmentPropertyMap = getInstance()->m_segmentPropertyCanvasMap;

//     for(segmentpropertycanvasmap::iterator i = segmentPropertyMap.begin();
//         i != segmentPropertyMap.end(); ++i) {

//         delete i->second;
//     }

//     segmentcontrollercanvasmap& segmentControllerMap = getInstance()->m_segmentControllerCanvasMap;

//     for(segmentcontrollercanvasmap::iterator i = segmentControllerMap.begin();
//         i != segmentControllerMap.end(); ++i) {

//         delete i->second;
//     }
    
// }

// QCanvas* ControlRulerCanvasRepository::getCanvas(Rosegarden::Segment* segment,
//                                                  PropertyName propertyName,
//                                                  QSize viewSize)
// {
//     segmentpropertycanvasmap& segmentPropertyMap = getInstance()->m_segmentPropertyCanvasMap;

//     // first fetch the propertymap for this segment,
//     // create it if it doesn't exist
//     //
//     propertycanvasmap* propCanvasMap = segmentPropertyMap[segment];
//     if (propCanvasMap == 0) {
//         propCanvasMap = new propertycanvasmap;
//         segmentPropertyMap[segment] = propCanvasMap;
//     }

//     // look up the map if the canvas is there, otherwise create it
//     //
//     QCanvas* canvas = (*propCanvasMap)[propertyName];
//     if (!canvas) {
//         canvas = new QCanvas(getInstance());
//         (*propCanvasMap)[propertyName] = canvas;
//         canvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight);
//     }
    
//     return canvas;
// }

// QCanvas* ControlRulerCanvasRepository::getCanvas(Rosegarden::Segment* segment,
//                                                  ControlParameter* controller,
//                                                  QSize viewSize)
// {
//     segmentcontrollercanvasmap& segmentControllerMap = getInstance()->m_segmentControllerCanvasMap;

//     // first fetch the controllermap for this segment,
//     // create it if it doesn't exist
//     //
//     controllercanvasmap* controllerCanvasMap = segmentControllerMap[segment];
//     if (controllerCanvasMap == 0) {
//         controllerCanvasMap = new controllercanvasmap;
//         segmentControllerMap[segment] = controllerCanvasMap;
//     }

//     // look up the map if the canvas is there, otherwise create it
//     //
//     QCanvas* canvas = (*controllerCanvasMap)[*controller];
//     if (!canvas) {
//         canvas = new QCanvas(getInstance());
//         (*controllerCanvasMap)[*controller] = canvas;
//         canvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight);
//     }
    
//     return canvas;
// }

// ControlRulerCanvasRepository* ControlRulerCanvasRepository::getInstance()
// {
//     if (!m_instance)
//         m_instance = new ControlRulerCanvasRepository();
    
//     return m_instance;
// }

// ControlRulerCanvasRepository::ControlRulerCanvasRepository()
// {
// }

// ControlRulerCanvasRepository* ControlRulerCanvasRepository::m_instance = 0;
