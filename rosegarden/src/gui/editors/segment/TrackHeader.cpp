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


#include "TrackHeader.h"

#include <qheader.h>
#include <qpainter.h>
#include <qrect.h>
#include <qwidget.h>


namespace Rosegarden
{

namespace Rosegarden
            TrackHeader::~TrackHeader()
    {}

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
            return ;
        else
            id = 0;
    for ( int i = id; i < count(); i++ ) {
        QRect r = sRect( i );
        paintSection( &p, i, r );
        if ( orientation() == Horizontal && r. right() >= e->rect().right() ||
                orientation() == Vertical && r. bottom() >= e->rect().bottom() )
            return ;
    }

}

}
