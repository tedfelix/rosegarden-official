/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
  
/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "BaseTextFloat.h"
#include "gui/general/GUIPalette.h"

#include <QPaintEvent>
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

#include <QTimer>

#include <iostream>


namespace Rosegarden
{

BaseTextFloat::BaseTextFloat(QWidget *parent) :
    QWidget(parent, Qt::ToolTip),
    m_text(""),
    m_timer(nullptr),
    m_widget(parent),
    m_totalPos(QPoint(0, 0)),
    m_width(20),
    m_height(20)
{
    if (parent) reparent(parent);
    resize(20, 20);
    hide();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
}

void
BaseTextFloat::reparent(QWidget *newParent)
{
    // If newParent is specified (not null) reparent text float to it
    // else keep current parent (ie m_widget)
    if (newParent) {
        m_widget = newParent;
    } else {
        newParent = m_widget;
    }

    // But a (no null) parent had to be specified
    if (!newParent) {
        std::cerr << "ERROR : "
                     "BaseTextFloat::reparent(0) called while m_widget = 0\n";
        m_totalPos = QPoint(0, 0);
        return;
    }

    // Reparent to either top level or dialog window
    // and compute the new parent widget position : we need to sum the
    // relative positions up to the topLevel or dialog.
    //
    QWidget *widget = newParent;
    m_totalPos = widget->pos();
    while (widget->parentWidget() && !widget->isWindow()) {
        widget = widget->parentWidget();
        m_totalPos += widget->pos();
    }

    setParent(widget, Qt::ToolTip);
    // TextFloat widget is now at top left corner of newParent (Qt4)
    // and m_totalPos is the move from this place to m_widget top left corner.
}

void
BaseTextFloat::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    paint.setPen(GUIPalette::getColour(GUIPalette::RotaryFloatForeground));
    paint.setBrush(GUIPalette::getColour(GUIPalette::RotaryFloatBackground));

    paint.drawRect(0, 0, m_width + 6, m_height + 6);
    paint.setPen(QColor(Qt::black));
    paint.drawText(QRect(3, 3, m_width, m_height), Qt::AlignLeft, m_text);
}

void
BaseTextFloat::setText(const QString &text)
{
    if (QString::compare(text, m_text) == 0) return;

    m_text = text;

    // Get the dimensions of the text-bounding rectangle and resize the widget.
    // With Qt5, attempting to resize within the paintEvent function causes a crash.
    QFont font(this->font());
    QFontMetrics metrics(font);
    QRect r = metrics.boundingRect(0, 0, 400, 400, Qt::AlignLeft, m_text);
    m_width = r.width();
    m_height = r.height();

    resize(m_width + 7, m_height + 7);

    update();
}

void
BaseTextFloat::display(QPoint offset)
{
    move(m_totalPos + offset);
    show();
}

void
BaseTextFloat::hideAfterDelay(int /* delay */)
{
    m_timer->start(500);
}

void
BaseTextFloat::slotTimeout()
{
    hide();
}

}

