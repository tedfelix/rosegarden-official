// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is taken from KGhostView, Copyright 1997-2002
        Markkhu Hihnala     <mah@ee.oulu.fi>
        and the KGhostView authors.

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qdrawutil.h>
#include <qpainter.h>
#include <qpixmap.h>

#include "scrollbox.h"

ScrollBox::ScrollBox(QWidget* parent, SizeMode sizeMode, const char* name) :
    QFrame(parent, name),
    m_sizeMode(sizeMode)
{
    setFrameStyle(Panel|Sunken);
}

void ScrollBox::mousePressEvent(QMouseEvent* e)
{
    m_mouse = e->pos();
    if (e->button() == RightButton) emit button3Pressed();
    if (e->button() == MidButton)   emit button2Pressed();
}

void ScrollBox::mouseMoveEvent(QMouseEvent* e)
{
    if (e->state() != LeftButton) return;

    int dx = (e->pos().x() - m_mouse.x()) * m_pagesize.width()  / width();
    int dy = (e->pos().y() - m_mouse.y()) * m_pagesize.height() / height();

    emit valueChanged(QPoint(m_viewpos.x() + dx, m_viewpos.y() + dy));
    emit valueChangedRelative(dx, dy);

    m_mouse = e->pos();
}

void ScrollBox::drawContents(QPainter* paint)
{
    if (m_pagesize.isEmpty()) return;

    QRect c(contentsRect());

    paint->setPen(Qt::red);

    int len = m_pagesize.width();
    int x = c.x() + c.width() * m_viewpos.x() / len;
    int w = c.width() * m_viewsize.width() / len ;
    if (w > c.width()) w = c.width();

    len = m_pagesize.height();
    int y = c.y() + c.height() * m_viewpos.y() / len;
    int h = c.height() * m_viewsize.height() / len;
    if (h > c.height()) h = c.height();

    paint->drawRect(x, y, w, h);
}

void ScrollBox::setPageSize(const QSize& s)
{
    m_pagesize = s;
    if (m_sizeMode == FixWidth) {
	setFixedHeight(s.height() * width() / s.width());
    } else {
	setFixedWidth(s.width() * height() / s.height());
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


ScrollBoxDialog::ScrollBoxDialog(QWidget *parent,
				 ScrollBox::SizeMode sizeMode,
				 const char *name,
				 WFlags flags) :
    KDialog(parent, name, flags),
    m_scrollbox(new ScrollBox(this, sizeMode))
{ }

ScrollBoxDialog::~ScrollBoxDialog()
{ }

void ScrollBoxDialog::closeEvent(QCloseEvent *e) 
{
    e->accept();
    emit closed();
}

void ScrollBoxDialog::setPageSize(const QSize& s)
{
    m_scrollbox->setPageSize(s);
    setFixedHeight(m_scrollbox->height());
    setFixedWidth(m_scrollbox->width());
}



// vim:sw=4:sts=4:ts=8:noet
