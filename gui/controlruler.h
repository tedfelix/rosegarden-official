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


#ifndef _CONTROLRULER_H_
#define _CONTROLRULER_H_

#include <qstring.h>

#include "Staff.h"

#include "PropertyName.h"

#include "rosegardencanvasview.h"
#include "widgets.h"

namespace Rosegarden
{
    class RulerScale;
    class Segment;
}

class QFont;
class QFontMetrics;

class ControlItem;
class ControlTool;
class ControlSelector;

/**
 * Property Control Ruler : edit range of event properties
 */
class ControlRuler : public RosegardenCanvasView, public Rosegarden::StaffObserver
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(Rosegarden::PropertyName propertyName,
                 Rosegarden::Staff*,
                 Rosegarden::RulerScale*,
                 QScrollBar* hsb,
                 QCanvas*,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    ~ControlRuler();

    void clear();

    void setControlTool(ControlTool*);

    int applyTool(double x, int val);

    QCanvasRectangle* getSelectionRectangle() { return m_selectionRect; }


    void setMaxPropertyValue(int val) { m_maxPropertyValue = val; }
    int getMaxPropertyValue()         { return m_maxPropertyValue; }

    const Rosegarden::PropertyName& getPropertyName()     { return m_propertyName; }

    // StaffObserver interface
    virtual void elementAdded(Rosegarden::ViewElement*);
    virtual void elementRemoved(Rosegarden::ViewElement*);
    virtual void sourceDeleted();

    static const int DefaultRulerHeight;
    static const int MinItemHeight;
    static const int MaxItemHeight;
    static const int ItemHeightRange;

public slots:
    /// override RosegardenCanvasView - we don't want to change the main hscrollbar
    virtual void slotUpdate();
    
protected:
    virtual void contentsMousePressEvent(QMouseEvent*);
    virtual void contentsMouseReleaseEvent(QMouseEvent*);
    virtual void contentsMouseMoveEvent(QMouseEvent*);
    virtual void contentsWheelEvent(QWheelEvent*);

    int valueToHeight(long val);
    long heightToValue(int height);
    QColor valueToColor(int);

    void init();
    void clearSelectedItems();
    void updateSelection();

    //--------------- Data members ---------------------------------

    Rosegarden::PropertyName m_propertyName;

    Rosegarden::Staff*      m_staff;
    Rosegarden::RulerScale* m_rulerScale;

    ControlItem* m_currentItem;
    QCanvasItemList m_selectedItems;

    ControlTool *m_tool;

    double m_currentX;

    int m_maxPropertyValue;

    QPoint m_lastEventPos;

    bool m_selecting;
    ControlSelector* m_selector;
    QCanvasRectangle* m_selectionRect;
};


/**
 * PropertyViewRuler is a widget that shows a range of Property values
 * for a set of Rosegarden Events.
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

#endif // _CONTROLRULER_H_

