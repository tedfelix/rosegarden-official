/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    This file is based on code from KGhostView, Copyright 1997-2002
        Markkhu Hihnala     <mah@ee.oulu.fi>
        and the KGhostView authors.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <QMouseEvent>
#include <QDesktopWidget>
#include "ScrollBox.h"

#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>


namespace Rosegarden
{

ScrollBox::ScrollBox(QWidget* parent, SizeMode sizeMode, const char* name) :
        QFrame(parent),
        m_sizeMode(sizeMode)
{
    QObject::setObjectName(name);
    setFrameStyle(Panel | Sunken);
}

void ScrollBox::mousePressEvent(QMouseEvent* e)
{
    m_mouse = e->pos();
    if (e->button() == Qt::RightButton)
        emit button3Pressed();
    if (e->button() == Qt::MidButton)
        emit button2Pressed();
}

void ScrollBox::mouseMoveEvent(QMouseEvent* e)
{
    //Qt3
    //if (e->state() != Qt::LeftButton)
    if (e->buttons() != Qt::LeftButton)
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

    paint->setPen(QColor(Qt::red));

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
    QPixmap bkPixmap  = img.fromImage(img.toImage().scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    QPalette palette;
    palette.setBrush(backgroundRole(), bkPixmap);
    setPalette(palette);

    // Qt3
    //setPaletteBackgroundPixmap(img.fromImage(img.toImage().scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

}
