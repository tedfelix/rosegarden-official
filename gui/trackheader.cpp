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

#include <qpainter.h>

#include "trackheader.h"

namespace Rosegarden
{

TrackHeader::~TrackHeader()
{
}


// ach
//
void
TrackHeader::paintEvent(QPaintEvent *e)
{
    QPainter p( this );
    p.setPen( colorGroup().buttonText() );
    int pos = (orientation() == Horizontal)
        ? e->rect().left()
        : e->rect().top();
    int id = mapToIndex( sectionAt( pos + offset() ) );
    if ( id < 0 )
	if ( pos > 0 )
	    return;
	else
	    id = 0;
    for ( int i = id; i < count(); i++ ) {
	QRect r = sRect( i );
	paintSection( &p, i, r );
	if ( orientation() == Horizontal && r. right() >= e->rect().right() ||
	     orientation() == Vertical && r. bottom() >= e->rect().bottom() )
	    return;
    }

}

// QRect QHeader::sRect( int index )
// {

//     int section = mapToSection( index );
//     if ( section < 0 )
// 	return rect(); // ### eeeeevil

//     if ( orient == Horizontal )
// 	return QRect(  d->positions[index]-offset(), 0, d->sizes[section], height() );
//     else
// 	return QRect( 0, d->positions[index]-offset(), width(), d->sizes[section] );
// }

// void TrackHeader::paintSection(QPainter *p, int index, QRect fr)
// {
//     int section = mapToSection( index );
//     if ( section < 0 )
// 	return;

//     bool down = (index==handleIdx) && ( state == Pressed || state == Moving );
//     p->setBrushOrigin( fr.topLeft() );
//     if ( d->clicks[section] ) {
// 	style().drawBevelButton( p, fr.x(), fr.y(), fr.width(), fr.height(),
// 				 colorGroup(), down );
//     } else {
// 	// ##### should be somhow styled in 3.0
// 	if ( orientation() == Horizontal ) {
// 	    p->save();

// 	    // ### Hack to keep styles working
// 	    p->setClipRect( fr );
// 	    style().drawBevelButton( p, fr.x() - 2, fr.y() - 2, fr.width() + 4, fr.height() + 4,
// 				     colorGroup(), down );
	
// 	    p->setPen( colorGroup().color( QColorGroup::Mid ) );
// 	    p->drawLine( fr.x(), fr.y() + fr.height() - 1, fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 	    p->drawLine( fr.x() + fr.width() - 1, fr.y(), fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 	    p->setPen( colorGroup().color( QColorGroup::Light ) );
// 	    if ( index > 0 )
// 		p->drawLine( fr.x(), fr.y(), fr.x(), fr.y() + fr.height() - 1 );
// 	    if ( index == count() - 1 ) {
// 		p->drawLine( fr.x() + fr.width() - 1, fr.y(), fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 		p->setPen( colorGroup().color( QColorGroup::Mid ) );
// 		p->drawLine( fr.x() + fr.width() - 2, fr.y(), fr.x() + fr.width() - 2, fr.y() + fr.height() - 1 );
// 	    }
// 	    p->restore();
// 	} else {
// 	    p->save();

// 	    // ### Hack to keep styles working
// 	    p->setClipRect( fr );
// 	    style().drawBevelButton( p, fr.x() - 2, fr.y() - 2, fr.width() + 4, fr.height() + 4,
// 				     colorGroup(), down );
	
// 	    p->setPen( colorGroup().color( QColorGroup::Mid ) );
// 	    p->drawLine( fr.x() + width() - 1, fr.y(), fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 	    p->drawLine( fr.x(), fr.y() + fr.height() - 1, fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 	    p->setPen( colorGroup().color( QColorGroup::Light ) );
// 	    if ( index > 0 )
// 		p->drawLine( fr.x(), fr.y(), fr.x() + fr.width() - 1, fr.y() );
// 	    if ( index == count() - 1 ) {
// 		p->drawLine( fr.x(), fr.y() + fr.height() - 1, fr.x() + fr.width() - 1, fr.y() + fr.height() - 1 );
// 		p->setPen( colorGroup().color( QColorGroup::Mid ) );
// 		p->drawLine( fr.x(), fr.y() + fr.height() - 2, fr.x() + fr.width() - 1, fr.y() + fr.height() - 2 );
// 	    }
// 	    p->restore();
// 	}
//     }

//     paintSectionLabel( p, index, fr );
// }

// /*!
//   Paints the label of actual section \a index of the header, inside rectangle \a fr in
//   widget coordinates.

//   Called by paintSection()
// */
// void TrackHeader::paintSectionLabel(QPainter *p, int index, const QRect& fr)
// {
//     int section = mapToSection( index );
//     if ( section < 0 )
// 	return;
//     QString s;
//     if ( d->labels[section] )
// 	s = *(d->labels[section]);
//     else if ( orientation() == Horizontal )
// 	s = tr("Col %1").arg(section);
//     else
// 	s = tr("Row %1").arg(section);

//     int m = 0;
//     if ( style() == WindowsStyle  &&
// 	 index==handleIdx && ( state == Pressed || state == Moving ) )
// 	m = 1;

//     QRect r( fr.x() + QH_MARGIN+m, fr.y() + 2+m,
// 	     fr.width() - 6, fr.height() - 4 );

//     int pw = 0;
//     if ( d->iconsets[section] ) {
// 	QIconSet::Mode mode = isEnabled()?QIconSet::Normal:QIconSet::Disabled;
// 	QPixmap pixmap = d->iconsets[section]->pixmap( QIconSet::Small, mode );
// 	int pixw = pixmap.width();
// 	pw = pixw;
// 	int pixh = pixmap.height();
// 	p->drawPixmap( r.left(), r.center().y()-pixh/2, pixmap );
// 	r.setLeft( r.left() + pixw + 2 );
//     }

//     p->drawText ( r, AlignLeft|AlignVCenter|SingleLine, s );

//     int arrowWidth = orientation() == Qt::Horizontal ? height() / 2 : width() / 2;
//     int arrowHeight = fr.height() - 6;
//     int tw = p->fontMetrics().width( s ) + 16;
//     if ( d->sortColumn == section && pw + tw + arrowWidth + 2 < fr.width() ) {
// 	p->save();
// 	if ( d->sortDirection ) {
// 	    QPointArray pa( 3 );
// 	    int x = fr.x() + pw + tw;
// 	    p->setPen( colorGroup().light() );
// 	    p->drawLine( x + arrowWidth, 4, x + arrowWidth / 2, arrowHeight );
// 	    p->setPen( colorGroup().dark() );
// 	    pa.setPoint( 0, x + arrowWidth / 2, arrowHeight );
// 	    pa.setPoint( 1, x, 4 );
// 	    pa.setPoint( 2, x + arrowWidth, 4 );
// 	    p->drawPolyline( pa );
// 	} else {
// 	    QPointArray pa( 3 );
// 	    int x = fr.x() + pw + tw;
// 	    p->setPen( colorGroup().light() );
// 	    pa.setPoint( 0, x, arrowHeight );
// 	    pa.setPoint( 1, x + arrowWidth, arrowHeight );
// 	    pa.setPoint( 2, x + arrowWidth / 2, 4 );
// 	    p->drawPolyline( pa );
// 	    p->setPen( colorGroup().dark() );
// 	    p->drawLine( x, arrowHeight, x + arrowWidth / 2, 4 );
// 	}
// 	p->restore();
//     }
// }


}

