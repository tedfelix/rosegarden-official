/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "ScrollBox.h"

#include <qapplication.h>
#include <qframe.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qwidget.h>


namespace Rosegarden
{

ScrollBox::ScrollBox(QWidget* parent, SizeMode sizeMode, const char* name) :
        QFrame(parent, name),
        m_sizeMode(sizeMode)
{
    setFrameStyle(Panel | Sunken);
}

void ScrollBox::mousePressEvent(QMouseEvent* e)
{
    m_mouse = e->pos();
    if (e->button() == RightButton)
        emit button3Pressed();
    if (e->button() == MidButton)
        emit button2Pressed();
}

void ScrollBox::mouseMoveEvent(QMouseEvent* e)
{
    if (e->state() != LeftButton)
        return ;

    int dx = (e->pos().x() - m_mouse.x()) * m_pagesize.width() / width();
    int dy = (e->pos().y() - m_mouse.y()) * m_pagesize.height() / height();

    emit valueChanged(QPoint(m_viewpos.x() + dx, m_viewpos.y() + dy));
    emit valueChangedRelative(dx, dy);

    m_mouse = e->pos();
}

void ScrollBox::drawContents(QPainter* paint)
{
    if (m_pagesize.isEmpty())
        return ;

    QRect c(contentsRect());

    paint->setPen(Qt::red);

    int len = m_pagesize.width();
    int x = c.x() + c.width() * m_viewpos.x() / len;
    int w = c.width() * m_viewsize.width() / len ;
    if (w > c.width())
        w = c.width();

    len = m_pagesize.height();
    int y = c.y() + c.height() * m_viewpos.y() / len;
    int h = c.height() * m_viewsize.height() / len;
    if (h > c.height())
        h = c.height();

    paint->drawRect(x, y, w, h);
}

void ScrollBox::setPageSize(const QSize& s)
{
    m_pagesize = s;

    setFixedWidth(100);
    setFixedHeight(100);

    int maxWidth = int(QApplication::desktop()->width() * 0.75);
    int maxHeight = int(QApplication::desktop()->height() * 0.75);

    if (m_sizeMode == FixWidth) {
        int height = s.height() * width() / s.width();
        if (height > maxHeight) {
            setFixedWidth(width() * maxHeight / height);
            height = maxHeight;
        }
        setFixedHeight(height);
    } else {
        int width = s.width() * height() / s.height();
        if (width > maxWidth) {
            setFixedHeight(height() * maxWidth / width);
            width = maxWidth;
        }
        setFixedWidth(width);
    }

    repaint();
}

void ScrollBox::setViewSize(const QSize& s)
{
    m_viewsize = s;
    repaint();
}

void ScrollBox::setViewPos(const QPoint& pos)
{
    m_viewpos = pos;
    repaint();
}

void ScrollBox::setViewX(int x)
{
    m_viewpos = QPoint(x, m_viewpos.y());
    repaint();
}

void ScrollBox::setViewY(int y)
{
    m_viewpos = QPoint(m_viewpos.x(), y);
    repaint();
}

void ScrollBox::setThumbnail(QPixmap img)
{
    setPaletteBackgroundPixmap(img.convertToImage().smoothScale(size()));
}

}
