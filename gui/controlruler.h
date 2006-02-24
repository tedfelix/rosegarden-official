// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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


#ifndef _CONTROLRULER_H_
#define _CONTROLRULER_H_

#include <qstring.h>
#include <klocale.h>

#include "Staff.h"
#include "Segment.h"
#include "PropertyName.h"

#include "rosegardencanvasview.h"
#include "hzoomable.h"

namespace Rosegarden { class RulerScale; class EventSelection; class ControlParameter; }

class QFont;
class QFontMetrics;
class RosegardenTextFloat;

class ControlItem;
class ControlTool;
class ControlSelector;
class EditViewBase;
class QPopupMenu;

/**
 * ControlRuler : base class for Control Rulers
 */
class ControlRuler : public RosegardenCanvasView, public Rosegarden::SegmentObserver
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(Rosegarden::Segment*,
                 Rosegarden::RulerScale*,
                 EditViewBase* parentView,
                 QCanvas*,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    virtual ~ControlRuler();

    virtual QString getName() = 0;

    int getMaxItemValue() { return m_maxItemValue; }
    void setMaxItemValue(int val) { m_maxItemValue = val; }

    void clear();

    void setControlTool(ControlTool*);

    int applyTool(double x, int val);

    QCanvasRectangle* getSelectionRectangle() { return m_selectionRect; }

    Rosegarden::RulerScale* getRulerScale() { return m_rulerScale; }

    // SegmentObserver interface
    virtual void segmentDeleted(const Rosegarden::Segment *);

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
    Rosegarden::RulerScale*     m_rulerScale;
    Rosegarden::EventSelection* m_eventSelection;
    Rosegarden::Segment*        m_segment;

    ControlItem* m_currentItem;
    QCanvasItemList m_selectedItems;

    ControlTool *m_tool;

    int m_maxItemValue;

    double m_staffOffset;

    double m_currentX;

    QPoint m_lastEventPos;
    bool m_itemMoved;

    bool m_selecting;
    ControlSelector* m_selector;
    QCanvasRectangle* m_selectionRect;

    QString m_menuName;
    QPopupMenu* m_menu;

    RosegardenTextFloat  *m_numberFloat;

    bool m_hposUpdatePending;
};

/**
 * PropertyControlRuler : edit a property on events on a staff (only
 * events with a ViewElement attached, mostly notes)
 */
class PropertyControlRuler : public ControlRuler, public Rosegarden::StaffObserver
{
public:
    PropertyControlRuler(Rosegarden::PropertyName propertyName,
                         Rosegarden::Staff*,
                         Rosegarden::RulerScale*,
                         EditViewBase* parentView,
                         QCanvas*,
                         QWidget* parent=0, const char* name=0, WFlags f=0);

    virtual ~PropertyControlRuler();

    virtual QString getName();

    const Rosegarden::PropertyName& getPropertyName()     { return m_propertyName; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setStaff(Rosegarden::Staff *);

    // StaffObserver interface
    virtual void elementAdded(const Rosegarden::Staff *, Rosegarden::ViewElement*);
    virtual void elementRemoved(const Rosegarden::Staff *, Rosegarden::ViewElement*);
    virtual void staffDeleted(const Rosegarden::Staff *);
    virtual void startPropertyLine();
    virtual void selectAllProperties();

    /// SegmentObserver interface
    virtual void endMarkerTimeChanged(const Rosegarden::Segment *, bool shorten);

protected:

    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsContextMenuEvent(QContextMenuEvent*);

    void drawPropertyLine(Rosegarden::timeT startTime,
                          Rosegarden::timeT endTime,
                          int startValue,
                          int endValue);

    virtual void init();
    virtual void drawBackground();
    virtual void computeStaffOffset();

    //--------------- Data members ---------------------------------

    Rosegarden::PropertyName       m_propertyName;
    Rosegarden::Staff*             m_staff;

    QCanvasLine                   *m_propertyLine;
    
    bool                           m_propertyLineShowing;
    int                            m_propertyLineX;
    int                            m_propertyLineY;
};


/**
 * ControllerEventsRuler : edit Controller events
 */
class ControllerEventsRuler : public ControlRuler
{
public:
    ControllerEventsRuler(Rosegarden::Segment*,
                          Rosegarden::RulerScale*,
                          EditViewBase* parentView,
                          QCanvas*,
                          QWidget* parent=0,
                          const Rosegarden::ControlParameter *controller = 0,
                          const char* name=0, WFlags f=0);

    virtual ~ControllerEventsRuler();

    virtual QString getName();
    int getDefaultItemWidth() { return m_defaultItemWidth; }

    // Allow something external to reset the selection of Events
    // that this ruler is displaying
    //
    void setSegment(Rosegarden::Segment *);

    /// SegmentObserver interface
    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);

    virtual void insertControllerEvent();
    virtual void eraseControllerEvent();
    virtual void clearControllerEvents();
    virtual void startControlLine();

    Rosegarden::ControlParameter* getControlParameter() { return m_controller; }

protected:

    virtual void init();
    virtual void drawBackground();

    // Let's override these again here
    //
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    virtual void layoutItem(ControlItem*);

    void drawControlLine(Rosegarden::timeT startTime,
                         Rosegarden::timeT endTime,
                         int startValue,
                         int endValue);

    //--------------- Data members ---------------------------------
    int                           m_defaultItemWidth;

    Rosegarden::ControlParameter  *m_controller;
    QCanvasLine                   *m_controlLine;
    
    bool                           m_controlLineShowing;
    int                            m_controlLineX;
    int                            m_controlLineY;
};

/**
 * PropertyViewRuler is a widget that shows a range of Property
 * (velocity, typically) values for a set of Rosegarden Events.
 */
class PropertyViewRuler : public QWidget, public HZoomable
{
    Q_OBJECT

public:
    PropertyViewRuler(Rosegarden::RulerScale *rulerScale,
                      Rosegarden::Segment *segment,
                      const Rosegarden::PropertyName &property,
                      double xorigin = 0.0,
                      int height = 0,
                      QWidget* parent = 0,
                      const char *name = 0);

    ~PropertyViewRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

    /**
     * Get the property name
     */
    Rosegarden::PropertyName getPropertyName() const { return m_propertyName; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

    //--------------- Data members ---------------------------------

    Rosegarden::PropertyName m_propertyName;

    double m_xorigin;
    int    m_height;
    int    m_currentXOffset;
    int    m_width;

    Rosegarden::Segment     *m_segment;
    Rosegarden::RulerScale  *m_rulerScale;

    QFont                    m_font;
    QFont                    m_boldFont;
    QFontMetrics             m_fontMetrics;
};

/**
 * We use a ControlBox to help modify events on the ruler - set tools etc.
 * and provide extra information or options.
 *
 */
class PropertyBox : public QWidget
{
    Q_OBJECT

public:
    PropertyBox(QString label,
               int width,
               int height,
               QWidget *parent=0,
               const char *name = 0);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent *);

    //--------------- Data members ---------------------------------

    QString m_label;
    int     m_width;
    int     m_height;
};

/**
 * A repository for QCanvas objects used by control rulers
 *
 * All control ruler for a given segment and property (or controller)
 * should use the same canvas
 *
 * Bad idea, works only in the case of 2 matrix views opened on the same segment
 * leaving it just in case.
 */
// class ControlRulerCanvasRepository : public QObject
// {
// public:
//     static void clear();
//     static QCanvas* getCanvas(Rosegarden::Segment*, Rosegarden::PropertyName,
//                               QSize viewSize);
//     static QCanvas* getCanvas(Rosegarden::Segment*, Rosegarden::ControlParameter*,
//                               QSize viewSize);

// protected:
//     typedef std::map<Rosegarden::PropertyName, QCanvas*> propertycanvasmap;
//     typedef std::map<Rosegarden::ControlParameter, QCanvas*> controllercanvasmap;

//     typedef std::map<Rosegarden::Segment*, propertycanvasmap*> segmentpropertycanvasmap;
//     typedef std::map<Rosegarden::Segment*, controllercanvasmap*> segmentcontrollercanvasmap;
    
//     ControlRulerCanvasRepository();
//     static ControlRulerCanvasRepository* getInstance();

//     static ControlRulerCanvasRepository* m_instance;
    
//     segmentpropertycanvasmap   m_segmentPropertyCanvasMap;
//     segmentcontrollercanvasmap m_segmentControllerCanvasMap;
    
// };


#endif // _CONTROLRULER_H_

