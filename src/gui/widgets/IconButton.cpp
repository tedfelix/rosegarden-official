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

#include "IconButton.h"

#include <QPainter>
#include <QWidget>
#include <QPixmap>
#include <QDebug>

IconButton::IconButton(QWidget* parent, const QPixmap& icon, const QString & name) :
        QAbstractButton(parent),
        m_margin(5),
        //@@@ these should pull from GUIPalette, and these hard-coded colors are
        // not stylesheet agnostic.  (So many other things already aren't that I
        // say stylesheet agnosticism is out the window anyway.)
        //
        // search key: localStyle
        m_textColor(Qt::black),
        m_checkedColor(QColor(0x80,0xAF,0xFF))
{
    // Store the icon and name, these will be rendered in the paint event
    m_pixmap = icon;
    m_labelText = name;
    
    // Get size of label
    m_font.setPixelSize(12);
    m_font.setBold(true);
    QFontMetrics metrics(m_font);
    m_labelSize = QSize(metrics.width(m_labelText),metrics.ascent());

    ///@@@ Shouldn't we use contentsmargins instead of a local m_margins member for this?
    setMinimumSize(std::max(icon.width(),m_labelSize.width())+2*m_margin, icon.height()+m_labelSize.height()+3*m_margin);
}

void IconButton::paintEvent(QPaintEvent* /* event */)
{
    QPainter paint(this);

    if (isChecked()) {
        paint.setPen(m_checkedColor);
        paint.setBrush(m_checkedColor);
        paint.drawRect(QRect(0,0,width(),height()));
    }

    paint.drawPixmap((width()-m_pixmap.width())/2,(height()-m_pixmap.height()-m_labelSize.height())/2,m_pixmap);
   
    //@@@ even uglier, we don't even name the constant for the selection
    // foreground color
    //
    // Make the selection foreground color lighter
    paint.setPen(isChecked() ? Qt::white : m_textColor);
    paint.setFont(m_font);
    paint.drawText((width()-m_labelSize.width())/2, height()-m_margin, m_labelText);
}

void IconButton::setCheckedColor(QColor color)
{
    m_checkedColor = color;
}

