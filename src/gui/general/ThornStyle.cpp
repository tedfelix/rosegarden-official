/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ThornStyle.h"


namespace Rosegarden
{

ThornStyle::ThornStyle()
{
    // ...
}

QIcon
ThornStyle::standardIconImplementation(StandardPixmap standardIcon,
                                        const QStyleOption *option,
                                        const QWidget *parent) const
{
    // custom implementation of icons for message boxes and file dialog
    // navigation arrows to follow shortly...  nothing here yet except
    // groundwork

    // let QPlastiqueStyle handle the rest
    return QPlastiqueStyle::standardPixmap(standardIcon, option, parent);
}


}

#include "ThornStyle.moc"
