/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ThornStyle.h"
#include "gui/general/IconLoader.h"


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
    // NOTE: see src/gui/styles/qcommonstyle.cpp in the Qt source for examples
    // of how to extend this whenever more custom icons are called for
    switch (standardIcon) {

    // custom icons for QMessageBox
    case SP_MessageBoxInformation:
        return IconLoader().loadPixmap("messagebox-information");

    case SP_MessageBoxWarning:
        return IconLoader().loadPixmap("warning");

    case SP_MessageBoxCritical:
        return IconLoader().loadPixmap("messagebox-critical");

    case SP_MessageBoxQuestion:
        return IconLoader().loadPixmap("messagebox-question");

    default:
        // let QPlastiqueStyle handle the rest
        return QPlastiqueStyle::standardPixmap(standardIcon, option, parent);
    }
}


}

#include "ThornStyle.moc"
