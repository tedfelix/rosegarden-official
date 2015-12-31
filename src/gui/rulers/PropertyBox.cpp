/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <QPaintEvent>
#include "PropertyBox.h"

#include "gui/general/GUIPalette.h"
#include <QPainter>
#include <QSize>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

PropertyBox::PropertyBox(QString label,
                         int width,
                         int height,
                         QWidget *parent) :
        QWidget(parent),
        m_label(label),
        m_width(width),
        m_height(height)
{}

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

    paint.setPen(GUIPalette::getColour(GUIPalette::MatrixElementBorder));
    //paint.setBrush(GUIPalette::getColour(GUIPalette::MatrixElementBlock));

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    paint.drawRect(2, 2, m_width - 3, m_height - 3);
    paint.drawText(10, 2 * m_height / 3, m_label);
}

}
