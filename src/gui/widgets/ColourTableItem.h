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

#ifndef _RG_ROSEGARDENCOLOURTABLEITEM_H_
#define _RG_ROSEGARDENCOLOURTABLEITEM_H_

#include <QColor>
#include <Q3Table>

class Q3Table;
class QRect;
class QPainter;
class QColorGroup;


namespace Rosegarden
{



class ColourTableItem : public Q3TableItem
{
public:
    ColourTableItem(Q3Table *t, const QColor &input)
     : Q3TableItem(t, Q3TableItem::Never, ""),
       currentColour(input) {}
    void setColor(QColor &input);
    void paint(QPainter *p, const QColorGroup &cg, const QRect &cr, bool selected);

protected:
    QColor currentColour;
};



}

#endif
