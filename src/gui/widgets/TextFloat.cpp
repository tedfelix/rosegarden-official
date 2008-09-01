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


#include "TextFloat.h"
#include <kapplication.h>

#include "gui/general/GUIPalette.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

TextFloat::TextFloat(QWidget *parent):
        QWidget(parent, "TextFloat",
                WStyle_Customize | WStyle_NoBorder | WStyle_StaysOnTop),
        m_text("")
{
    reparent(parentWidget());
    resize(20, 20);
}

void
TextFloat::reparent(QWidget *newParent)
{
    QPoint position = newParent->pos();

    // Get position and reparent to either top level or dialog
    //
    while (newParent->parentWidget() && !newParent->isTopLevel()
            && !newParent->isDialog()) {
        newParent = newParent->parentWidget();
        position += newParent->pos();
    }

    // Position this widget to the right of the parent
    //
    //move(pos + QPoint(parent->width() + 5, 5));

    QWidget::reparent(newParent,
                      WStyle_Customize | WStyle_NoBorder | WStyle_StaysOnTop,
                      position + QPoint(20, 5));
}

void
TextFloat::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));

    paint.setPen(GUIPalette::getColour(GUIPalette::RotaryFloatForeground));
    paint.setBrush(GUIPalette::getColour(GUIPalette::RotaryFloatBackground));

    QFontMetrics metrics(paint.fontMetrics());

    QRect r = metrics.boundingRect(0, 0, 400, 400, Qt::AlignLeft, m_text);
    resize(r.width() + 7, r.height() + 7);
    paint.drawRect(0, 0, r.width() + 6, r.height() + 6);
    paint.setPen(QColor(Qt::black));
    paint.drawText(QRect(3, 3, r.width(), r.height()), Qt::AlignLeft, m_text);

    /*
        QRect textBound = metrics.boundingRect(m_text);
     
        resize(textBound.width() + 7, textBound.height() + 7);
        paint.drawRect(0, 0, textBound.width() + 6, textBound.height() + 6);
     
        paint.setPen(QColor(Qt::black));
        paint.drawText(3, textBound.height() + 3, m_text);
    */
}

void
TextFloat::setText(const QString &text)
{
    m_text = text;
    repaint();
}

}
