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


#include "TrackHeader.h"

#include <Q3Header>
#include <QHeaderView>		// replaces Q3Header
#include <QPainter>
#include <QRect>
#include <QWidget>


namespace Rosegarden
{

TrackHeader::~TrackHeader()
{}

void
TrackHeader::paintEvent(QPaintEvent *e)
{
    QPainter p( this );
    p.setPen( colorGroup().buttonText() );
    int pos = (orientation() == Qt::Horizontal)
              ? e->rect().left()
              : e->rect().top();
    int id = mapToIndex( sectionAt( pos + offset() ) );
    if ( id < 0 )
        if ( pos > 0 )
            return ;
        else
            id = 0;
    for ( int i = id; i < count(); i++ ) {
        QRect r = sRect( i );
        paintSection( &p, i, r );
        if ( orientation() == Qt::Horizontal && r. right() >= e->rect().right() ||
			 orientation() == Qt::Vertical && r. bottom() >= e->rect().bottom() )
            return ;
    }

}

}
