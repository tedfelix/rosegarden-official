// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "Property.h"

using Rosegarden::RulerScale;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::PropertyName;
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

    virtual bool getValue(long&) = 0;
    virtual void setValue(long)  = 0;
    virtual Event* getEvent() = 0;
};

//////////////////////////////

class ViewElementAdapter : public ElementAdapter
{
public:
    ViewElementAdapter(ViewElement*, const PropertyName&);

    virtual bool getValue(long&);
    virtual void setValue(long);

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

//////////////////////////////

class ControllerEventAdapter : public ElementAdapter
{
public:
    ControllerEventAdapter(Event* e) : m_event(e) {}

    virtual bool getValue(long&);
    virtual void setValue(long);

    virtual Event* getEvent() { return m_event; }

protected:

    //--------------- Data members ---------------------------------

    Event* m_event;
};

bool ControllerEventAdapter::getValue(long& val)
{
    return m_event->get<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
}

void ControllerEventAdapter::setValue(long val)
{
    m_event->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
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

    ControlRuler* m_controlRuler;
    ElementAdapter* m_elementAdapter;

    static const unsigned int BorderThickness;
    static const unsigned int DefaultWidth;
};

const unsigned int ControlItem::BorderThickness = 1;
const unsigned int ControlItem::DefaultWidth    = 20;

ControlItem::ControlItem(ControlRuler* ruler, ElementAdapter* elementAdapter,
                         int xx, int width)
    : QCanvasRectangle(ruler->canvas()),
      m_controlRuler(ruler),
      m_elementAdapter(elementAdapter)
{
    setWidth(width);
    setPen(QPen(Qt::black, BorderThickness));
    setBrush(Qt::blue);

    setX(xx);
    setY(canvas()->height());
    updateFromValue();
    RG_DEBUG << "ControlItem x = " << x() << " - y = " << y() << " - width = " << width << endl;
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
        RG_DEBUG << "ControlItem::updateFromValue() : value = " << m_value << endl;
        setHeight(m_controlRuler->valueToHeight(m_value));
    }
}

void ControlItem::draw(QPainter &painter)
{
    setBrush(m_controlRuler->valueToColor(m_value));

    QCanvasRectangle::draw(painter);
}

void ControlItem::handleMouseButtonPress(QMouseEvent*)
{
}

void ControlItem::handleMouseButtonRelease(QMouseEvent*)
{
}

void ControlItem::handleMouseMove(QMouseEvent*, int /*deltaX*/, int deltaY)
{
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
    canvas()->update();
}

void ControlItem::handleMouseWheel(QWheelEvent*)
{
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
    getSelectionRectangle()->setX(e->x());
    getSelectionRectangle()->setY(e->y());
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
    int w = int(e->x() - getSelectionRectangle()->x());
    int h = int(e->y() - getSelectionRectangle()->y());
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


    virtual void modifySegment();

protected:

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
class ControllerEventInsertCommand : public BasicCommand
{
public:
    ControllerEventInsertCommand(timeT insertTime, long number, long initialValue, Segment &segment);

    virtual void modifySegment();

protected:
    long m_number;
    long m_initialValue;
};

ControllerEventInsertCommand::ControllerEventInsertCommand(timeT insertTime,
                                                           long number, long initialValue,
                                                           Segment &segment)
    : BasicCommand(i18n("Insert Controller Event"), segment, insertTime, insertTime),
      m_number(number),
      m_initialValue(initialValue)
{
}

void ControllerEventInsertCommand::modifySegment()
{
    Event* controllerEvent = new Event(Rosegarden::Controller::EventType,
                                       getStartTime());

    controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, m_initialValue);
    controllerEvent->set<Rosegarden::Int>(Rosegarden::Controller::NUMBER, m_number);
    
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


    virtual void modifySegment();

protected:

    QCanvasItemList m_selectedItems;
};

ControllerEventEraseCommand::ControllerEventEraseCommand(QCanvasItemList selectedItems,
                                                         Segment &segment,
                                                         Rosegarden::timeT start, Rosegarden::timeT end)
    : BasicCommand(i18n("Erase Controller Event(s)"), segment, start, end, true),
      m_selectedItems(selectedItems)
{
    RG_DEBUG << "ControllerEventEraseCommand : from " << start << " to " << end << endl;
}


void ControllerEventEraseCommand::modifySegment()
{
    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            getSegment().eraseSingle(item->getElementAdapter()->getEvent());
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

    emit stateChange("have_controller_item_selected", false);

}

ControlRuler::~ControlRuler()
{
}

void
ControlRuler::slotUpdate()
{
    RG_DEBUG << "ControlRuler::slotUpdate()\n";
    canvas()->update();
}

void ControlRuler::setControlTool(ControlTool* tool)
{
    if (m_tool) delete m_tool;
    m_tool = tool;
}

void ControlRuler::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton) return;

    RG_DEBUG << "ControlRuler::contentsMousePressEvent()\n";

    QCanvasItemList l=canvas()->collisions(e->pos());

    if (l.count() == 0) { // de-select current item
        clearSelectedItems();
        m_selecting = true;
        m_selector->handleMouseButtonPress(e);
        RG_DEBUG << "ControlRuler::contentsMousePressEvent : entering selection mode\n";
        return;
    }

    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            if (item->isSelected()) continue;

            // clear selection unless control was pressed, in which case add the event
            // to the current selection
            if (!(e->state() && QMouseEvent::ControlButton)) { clearSelectedItems(); }
            m_selectedItems << item;
            item->setSelected(true);
            item->handleMouseButtonPress(e);
            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());

        }
    }

    m_itemMoved = false;
    m_lastEventPos = e->pos();
}

void ControlRuler::contentsMouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton) return;
    
    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseButtonRelease(e);
        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : leaving selection mode\n";
        m_selecting = false;
        return;
    }

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            item->handleMouseButtonRelease(e);
    }

    if (m_itemMoved) {

        m_lastEventPos = e->pos();

        // Add command to history
        ControlChangeCommand* command = new ControlChangeCommand(m_selectedItems,
                                                                 m_segment,
                                                                 m_eventSelection->getStartTime(),
                                                                 m_eventSelection->getEndTime());

        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : adding command\n";
        m_parentEditView->addCommandToHistory(command);

        m_itemMoved = false;
    }
    
}

void ControlRuler::contentsMouseMoveEvent(QMouseEvent* e)
{
    int deltaX = e->x() - m_lastEventPos.x(),
        deltaY = e->y() - m_lastEventPos.y();
    m_lastEventPos = e->pos();

    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseMove(e, deltaX, deltaY);
        slotScrollHorizSmallSteps(e->pos().x());
        return;
    }

    m_itemMoved = true;

    for (QCanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
            item->handleMouseMove(e, deltaX, deltaY);
    
    }

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
        }
    }

    emit stateChange("have_controller_item_selected", haveSelectedItems);
}

void ControlRuler::contentsContextMenuEvent(QContextMenuEvent* e)
{
    if (!m_menu && !m_menuName.isEmpty()) createMenu();

    if (m_menu) {
        RG_DEBUG << "ControlRuler::showMenu() - show menu with" << m_menu->count() << " items\n";
        m_lastEventPos = e->pos();
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
//     RG_DEBUG << "ControlRuler::valueToHeight : val = " << val << " - height = " << res
//              << " - scaleVal = " << scaleVal << endl;
    return res;
}

long ControlRuler::heightToValue(int h)
{
    long val = -h;
    val -= MinItemHeight;
    val *= getMaxItemValue();
    val /= (ItemHeightRange);
    val = std::min(val, long(getMaxItemValue()));

//     RG_DEBUG << "ControlRuler::heightToValue : height = " << h << " - val = " << val << endl;

    return val;
}

QColor ControlRuler::valueToColor(int val)
{
    return DefaultVelocityColour::getInstance()->getColour(val);
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
    m_staff(staff)
{
    m_staff->addObserver(this);
    init();
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

    for(ViewElementList::iterator i = viewElementList->begin();
        i != viewElementList->end(); ++i) {

 	double x = m_rulerScale->getXForTime((*i)->getViewAbsoluteTime());
 	new ControlItem(this, new ViewElementAdapter(*i, getPropertyName()), int(x),
                        int(m_rulerScale->getXForTime((*i)->getViewAbsoluteTime() +
                                                      (*i)->getViewDuration()) - x));

    }
}

void PropertyControlRuler::elementAdded(const Rosegarden::Staff *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementAdded()\n";

    double x = m_rulerScale->getXForTime(el->getViewAbsoluteTime());

    new ControlItem(this, new ViewElementAdapter(el, getPropertyName()), int(x),
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

//----------------------------------------
ControllerEventsRuler::ControllerEventsRuler(Rosegarden::Segment& segment,
                                             Rosegarden::RulerScale* rulerScale,
                                             EditViewBase* parentView,
                                             QCanvas* c,
                                             QWidget* parent,
                                             Rosegarden::ControlParameter *controller,
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
        m_controller = new Rosegarden::ControlParameter(*controller);
    else
        m_controller = 0;

    m_segment.addObserver(this);

    for(Segment::iterator i = m_segment.begin();
        i != m_segment.end(); ++i) {

        // skip if not a ControllerEvent
        if (!(*i)->isa(Rosegarden::Controller::EventType)) continue;

        // Check for specific controller value if we need to 
        //
        if (m_controller)
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
        
        RG_DEBUG << "ControllerEventsRuler: adding element\n";

 	double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime());
 	new ControlItem(this, new ControllerEventAdapter(*i), int(x),
                        getDefaultItemWidth());

    }
    
    setMenuName("controller_events_ruler_menu");
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
        QString name = QString("%1 (%2)").arg(strtoqstr(m_controller->getName()))
                                         .arg(int(m_controller->getControllerValue()));

        return name;
    }
    else return i18n("Controller Events");
}

void ControllerEventsRuler::eventAdded(const Segment*, Event *e)
{
    if (!e->isa(Rosegarden::Controller::EventType)) return;

    // Check for specific controller value if we need to 
    //
    if (m_controller)
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

    new ControlItem(this, new ControllerEventAdapter(e), int(x),
                    getDefaultItemWidth());
}

void ControllerEventsRuler::eventRemoved(const Segment*, Event *e)
{
    if (!e->isa(Rosegarden::Controller::EventType)) return;

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
    
    ControllerEventInsertCommand* command = 
        new ControllerEventInsertCommand(insertTime, number, initialValue, m_segment);

    m_parentEditView->addCommandToHistory(command);
}

void ControllerEventsRuler::eraseControllerEvent()
{
    RG_DEBUG << "ControllerEventsRuler::eraseControllerEvent() : deleting selected events\n";

    ControllerEventEraseCommand* command = new ControllerEventEraseCommand(m_selectedItems,
                                                                           m_segment,
                                                                           m_eventSelection->getStartTime(),
                                                                           m_eventSelection->getEndTime());
    m_parentEditView->addCommandToHistory(command);
    clearSelectedItems();
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
}

void ControllerEventsRuler::contentsMousePressEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        ControlRuler::contentsMousePressEvent(e); // send super
        return;
    }

    // cancel control line mode
    if (e->button() == RightButton)
    {
        m_controlLineShowing = false;
        m_controlLine->hide();
        return;
    }

    if (e->button() == LeftButton)
    {
        m_controlLine->show();
        m_controlLineX = e->x();
        m_controlLineY = e->y();
        m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
        canvas()->update();
    }
}

void ControllerEventsRuler::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        ControlRuler::contentsMouseReleaseEvent(e); // send super
        return;
    }
    else
    {
        timeT startTime = m_rulerScale->getTimeForX(m_lastEventPos.x());
        timeT endTime = m_rulerScale->getTimeForX(e->x());

        long startValue = heightToValue(m_lastEventPos.y() - canvas()->height());
        long endValue = heightToValue(e->y() - canvas()->height());

        /*
        std::cout << "ControllerEventsRuler::contentsMouseReleaseEvent - "
                  << "starttime = " << startTime
                  << ", endtime = " << endTime
                  << ", startValue = " << startValue
                  << ", endValue = " << endValue
                  << std::endl;
                  */

        drawControlLine(startTime, endTime, startValue, endValue);

        m_controlLineShowing = false;
        m_controlLine->hide();
        canvas()->update();
    }

}

void ControllerEventsRuler::contentsMouseMoveEvent(QMouseEvent *e)
{
    if (!m_controlLineShowing)
    {
        ControlRuler::contentsMouseMoveEvent(e); // send super
        return;
    }

    m_controlLine->setPoints(m_controlLineX, m_controlLineY, e->x(), e->y());
    canvas()->update();

}

void
ControllerEventsRuler::drawControlLine(Rosegarden::timeT startTime,
                                       Rosegarden::timeT endTime,
                                       int startValue,
                                       int endValue)
{
    if (m_controller == 0) return;

    Rosegarden::timeT quantDur = Rosegarden::Note(Rosegarden::Note::Quaver).getDuration();

    // for the moment enter a quantized set of events
    Rosegarden::timeT time = startTime, newTime = 0;
    double step = double(endValue - startValue) / double(endTime - startTime);

    KMacroCommand *macro = new KMacroCommand(i18n("Add line of controllers"));

    while (time < endTime)
    {
        int value = startValue + int(step * double(time));

        // hit the buffers
        if (value < m_controller->getMin())
            value = m_controller->getMin();
        else if (value > m_controller->getMax())
            value = m_controller->getMax();
        
        ControllerEventInsertCommand* command = 
            new ControllerEventInsertCommand(time, m_controller->getControllerValue(), value, m_segment);

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

