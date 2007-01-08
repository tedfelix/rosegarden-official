/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlItem.h"
#include "ControlRuler.h"
#include "ElementAdapter.h"
#include "misc/Debug.h"

namespace Rosegarden {

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
    RG_DEBUG << "ControlItem::updateFromValue() : " << this << endl;

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
//     RG_DEBUG << "ControlItem::handleMouseWheel - got wheel event" << endl;
}

void ControlItem::setSelected(bool s)
{
    QCanvasItem::setSelected(s);

    if (s) setPen(QPen(Qt::red, BorderThickness));
    else setPen(QPen(Qt::black, BorderThickness));

    canvas()->update();
}

}
