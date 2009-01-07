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


#include "ColourTableItem.h"

#include <QColor>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QBrush>


namespace Rosegarden
{

ColourTableItem::ColourTableItem(QTableWidget *t, const QColor &input)
        : currentColour(input)
{
    //@@@ If this is crap in the real world, refer to the old line of code to
    // take another look at what it was supposed to do, and how it works, as
    // this is no longer recognizeable; not that anyone is left who would have
    // recognized it anyway (Mark Hymers?  That was so long ago.)

    // try doing this in the ctor to replace what used to be the paint() method,
    // which never seems to have been called explicitly, so I assume the whole
    // mechanism this used to rely on no longer exists, and am trying to
    // guesstimate what all of this was supposed to do (fill cells in a table
    // with rectangles of color, presumably) and try to recreate it

    QTableWidgetItem *ti = new QTableWidgetItem();

    // default state appears to be with no flags set, thus should not be
    // editable, etc. by default, so no need for old QTableItem::Never it seems

    QBrush b (input);     // create brush with current color
    ti->setBackground(b); // set background to brush (will this be enough?  no more QRect?
                          // I really have no clue, because I've never done any
                          // manual drawing stuff in any flavor of Qt new or
                          // old, and this all amounts to a wild ass guess)
}

void
ColourTableItem::setColor(QColor &input)
{
    currentColour = input;
}

}
