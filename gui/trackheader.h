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


#ifndef _SEGMENTHEADER_H_
#define _SEGMENTHEADER_H_

#include <qwidget.h>
#include <qheader.h>

namespace Rosegarden
{

class TrackHeader : public QHeader
{

public:
    TrackHeader(int number,
                          QWidget *parent=0,
                          const char *name=0 ):
                            QHeader(number, parent, name) {;}
    ~TrackHeader();

protected:
    virtual void paintEvent(QPaintEvent *pe);
//     void paintSection(QPainter * p, int index, QRect fr);
//     void paintSectionLabel (QPainter * p, int index, const QRect & fr);
//     QRect sRect (int index);
    
private:

};

}

#endif // _SEGMENTHEADER_H_
