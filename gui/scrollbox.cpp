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

ScrollBox::ScrollBox( QWidget* parent, const char* name )
    : QFrame( parent, name )
{
    setFrameStyle( Panel | Sunken );
}

void ScrollBox::mousePressEvent( QMouseEvent* e )
{
    mouse = e->pos();
    if( e->button() == RightButton )
	emit button3Pressed();
    if( e->button() == MidButton )
	emit button2Pressed();
}

void ScrollBox::mouseMoveEvent( QMouseEvent* e )
{
    if( e->state() != LeftButton )
	return;

    int dx = ( e->pos().x() - mouse.x() ) * pagesize.width()  / width();
    int dy = ( e->pos().y() - mouse.y() ) * pagesize.height() / height();

    emit valueChanged( QPoint( viewpos.x() + dx, viewpos.y() + dy ) );
    emit valueChangedRelative( dx, dy );

    mouse = e->pos();
}

void ScrollBox::drawContents( QPainter* paint )
{
    if ( pagesize.isEmpty() )
	return;

    QRect c( contentsRect() );

    paint -> setPen( Qt::red );

    int len = pagesize.width();
    int x = c.x() + c.width() * viewpos.x() / len;
    int w = c.width() * viewsize.width() / len ;
    if ( w > c.width() ) w = c.width();

    len = pagesize.height();
    int y = c.y() + c.height() * viewpos.y() / len;
    int h = c.height() * viewsize.height() / len;
    if ( h > c.height() ) h = c.height();

    paint->drawRect( x, y, w, h );
}

void ScrollBox::setPageSize( const QSize& s )
{
    pagesize = s;
    setFixedHeight( s.height() * width() / s.width() );
    repaint();
}

void ScrollBox::setViewSize( const QSize& s )
{
    viewsize = s;
    repaint();
}

void ScrollBox::setViewPos( const QPoint& pos )
{
    viewpos = pos;
    repaint();
}

void ScrollBox::setViewX( int x )
{
    viewpos = QPoint( x, viewpos.y() );
    repaint();
}

void ScrollBox::setViewY( int y )
{
    viewpos = QPoint( viewpos.x(), y );
    repaint();
}

void ScrollBox::setThumbnail( QPixmap img )
{
    setPaletteBackgroundPixmap( img.convertToImage().smoothScale( size() ) );
}


//!!! EXPERIMENTAL

ScrollBoxDialog::ScrollBoxDialog(QWidget *parent,
			   const char *name,
			   WFlags flags) :
    KDialog(parent, name, flags),
    m_panner(new ScrollBox(this))
{ }

ScrollBoxDialog::~ScrollBoxDialog()
{ }

void ScrollBoxDialog::closeEvent(QCloseEvent *e) {
    e->accept();
//    emit closed;
}



// vim:sw=4:sts=4:ts=8:noet
